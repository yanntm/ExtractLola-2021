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
\author Karsten
\status new

\brief compute reachability using relaxed stubborn sets
*/

#include <Core/Dimensions.h>
#include <Exploration/ReachabilityExplorationRelaxed.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/FirelistStubbornCloseGiven.h>
#include "../../libs/lp_solve_5.5/lp_lib.h"
#include <Formula/StatePredicate/AtomicStatePredicate.h>

bool ReachabilityExplorationRelaxed::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    bool inUpset[net->Card[TR]];
    arrayindex_t Upset[net->Card[TR]];
    arrayindex_t cardUpset;
    StatePredicateProperty * p = reinterpret_cast<StatePredicateProperty *> (&property);

    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    memset(lastfired,0,net->Card[TR] * sizeof(int));

    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);
    

    property.value = property.initProperty(ns);
    if(property.value)
    {
	return true;
    }



    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

//for(int i = 0; i < currentEntry;i++)
//{
	//RT::rep->status("INITIAL FL: %s", net -> Name[TR][currentFirelist[i]]);
//}

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 1;

    //set initial dfs to one
    setDFS(payload, ++currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
//RT::rep->status("NEW ROUND DEPTH %d", dfsstack.StackPointer );
//RT::rep->status("=========");
//for(int i = 0; i < net -> Card[PL]; i++)
//{
	//if(ns.Current[i]) RT::rep->status("MARKING %s: %d",net->Name[PL][i],ns.Current[i]);
//}
//for(int i = 0; i < net -> Card[TR];i++)
//{	
	//if(ns.Enabled[i]) RT::rep->status("ENABLED: %s", net -> Name[TR][i]);
//}
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
//RT::rep->status("ALREADY SEEN --> BACKTRACK");
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
//RT::rep->status("STATE IS NEW");
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
                setDFS(newPayload, ++currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put an entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
 		property.value =  property.checkProperty(ns, currentFirelist[currentEntry]);
		if (property.value) 
		{
			return true;
		}

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
//for(int i = 0; i < currentEntry;i++)
//{
	//RT::rep->status("NEW FL: %s", net -> Name[TR][currentFirelist[i]]);
//}
            }
        }
        else
        {
//RT::rep->status("FIRELIST EXHAUSTED");
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
//RT::rep->status("ROOT OF TSCC");
		// this is root of tscc 
		// --> check for ignored transitions in an up-set of this marking
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
                cardUpset = p->predicate -> getUpSet(Upset,inUpset,&needEnabled);
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
//RT::rep->status("HAVE IGNORED");
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    	delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
//for(int i = 0; i < currentEntry; i++)
//{
	//RT::rep->status("ADDED TO FL: %s", net->Name[TR][currentFirelist[i]]);
//}
			if(currentEntry) continue;
		}
//RT::rep->status("FINALLY CLOSED");
		// this tscc is finally closed

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

