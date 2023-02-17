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

\brief do a depth_first search to evaluate EG property
*/


#include <Planning/Task.h>
#include <Core/Dimensions.h>
#include <Exploration/EXEGExploration.h>
#include <Exploration/ChooseTransition.h>

bool EXEGExploration::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<bool> &myStore, Firelist &myFirelist, int)
{

Firelist * fullfl = new Firelist(net);
    // add initial marking to store

    if(!ns.CardEnabled) return false;
    // payload is a bit that is true as long as state is on stack.
    bool *payload = new bool;
    * payload = true;

    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = fullfl->getFirelist(ns, &currentFirelist);

    property.initProperty(ns);


    while (true)
    {
//RT::rep->status("considering...");
//for(arrayindex_t i = 0; i < net->Card[PL];i++)
//{
//	if(ns.Current[i]) RT::rep->status("%s: %d",net->Name[PL][i],ns.Current[i]);
//}
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
//RT::rep->status("fire %s at pos %d",net->Name[TR][currentFirelist[currentEntry]],currentEntry);
            net->fire(ns, currentFirelist[currentEntry]);
            net->updateEnabled(ns, currentFirelist[currentEntry]);
            if (!property.checkProperty(ns, currentFirelist[currentEntry]))
            {
//RT::rep->status("not phi -> backtrack");
		// if new state does not satisfy phi, it is not part of our search space
		net->backfire(ns,currentFirelist[currentEntry]);
	    	net->revertEnabled(ns, currentFirelist[currentEntry]);
		property.updateProperty(ns, currentFirelist[currentEntry]);
		continue;
            }

//RT::rep->status("sat phi");
            // here: new state satisfies phi.
            bool *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {

//RT::rep->status("visited");
                //already visited
		if(*newPayload) 
		{	
//RT::rep->status("on stack");
			return true;   // cycle found!
		}

//RT::rep->status("not on stack _> baktrack");
		// here: known but not on stack anymore 
                // ->backfire the transition
                net->backfire(ns, currentFirelist[currentEntry]);
	    	net->revertEnabled(ns, currentFirelist[currentEntry]);
		property.updateProperty(ns, currentFirelist[currentEntry]);
		continue;
            }
            else
            {
                //not yet visited
//RT::rep->status("state new");
		if(!ns.CardEnabled)
		{
			return true;
		}
                //put a entry on the searchstack
                //with the current dfs number and the current lowlink
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, 0);
		*newPayload = true;
		payload = newPayload;

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
//RT::rep->status("close state");
	    // no transition to be fired
	    // close state

            delete[] currentFirelist;
	    // mark state as "not on stack"
	    * payload = false;

            //if the stack is empty we are finished
            if (dfsstack.StackPointer == 0)
            {
                //set the property value
                return false;
            }
            // revert to previous state on stack
            DFSStackEntry &stackentry = dfsstack.top();
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
	    payload = (bool *) stackentry.payload;
            stackentry.fl = NULL;
            

            // pop the top element on the stack
            dfsstack.pop();
            assert(currentEntry < net->Card[TR]);

            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}
