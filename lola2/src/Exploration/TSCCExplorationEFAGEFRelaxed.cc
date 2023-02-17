/****************************************************************************
  This file is part of LoLA.

  LoLA is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  LoLA is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
  more details.

  You should have received a copy of the GNU Affero General Public License
  along with LoLA. If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

/*!
\file
\author Markus
\status new

\brief compute EFAGEF using relaxed stubborn sets
*/

#include <Core/Dimensions.h>
#include <Exploration/TSCCExploration.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/FirelistStubbornCloseGiven.h>

bool TSCCExplorationEFAGEFRelaxed::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    statenumber_t lasttrue = 0;
    StatePredicateProperty * p = reinterpret_cast<StatePredicateProperty *> (&property);
    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    if(property.initProperty(ns)) lasttrue = 1;

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	inUpset[i] = false;
    }
    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 1;

    //set initial dfs to one
    setDFS(payload, ++currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                if (dfsstack.StackPointer != 0)
                {
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
                setDFS(newPayload, ++currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put a entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
                if (property.checkProperty(ns, currentFirelist[currentEntry]))
                {
                    lasttrue = currentDFSNumber;
                }

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
		// this is root of tscc 
		// --> check for ignored transitions in an up-set of this marking
		bool needEnabled = false;

		for(arrayindex_t i = 0; i < net->Card[TR]; i++)
		{
			inUpset[i] = false;
		}
		cardUpset = p->predicate -> getUpSet(Upset,inUpset,&needEnabled);
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(myFirelist.visible[t] && !inUpset[t])
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
			}
		}
		for(arrayindex_t i = 0; i < cardUpset;i++)
		{
			arrayindex_t t = Upset[i];
			if(lastfired[t] >= getDFS(payload))
			{
				// this transition is not ignored
				inUpset[t] = false;
				Upset[i--] = Upset[--cardUpset];
			}
			else
			{
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed

		if(lasttrue >= getDFS(payload)) return true;
		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
                //set the property value
                return false;
            }
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            
            //update the property
            property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}
