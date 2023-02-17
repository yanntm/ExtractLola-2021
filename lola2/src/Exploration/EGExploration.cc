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
#include <Exploration/EGExploration.h>
#include <Exploration/ChooseTransition.h>

bool EGExploration::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<bool> &myStore, Firelist &myFirelist, int)
{

    // add initial marking to store

    // payload is a bit that is true as long as state is on stack.
    bool *payload;
    myStore.searchAndInsert(ns, &payload, 0);
    * payload = true;

    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    if (!property.initProperty(ns))
    {
	// EG phi is false if phi is false in the initial state
        return false;
    }

    while (true)
    {
//RT::rep->status("considering at depth %d with %d transitions in fl...",dfsstack.StackPointer,currentEntry);
//for(arrayindex_t i = 0; i < net->Card[PL];i++)
//{
	//if(ns.Current[i]) RT::rep->status("%s: %d",net->Name[PL][i],ns.Current[i]);
//}
//for(arrayindex_t i = 0; i < net->Card[TR];i++)
//{
	//if(ns.Enabled[i]) RT::rep->status("en %s",net->Name[TR][i]);
//}
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
//RT::rep->status("fire %s at pos %d.",net->Name[TR][currentFirelist[currentEntry]],currentEntry);
            net->fire(ns, currentFirelist[currentEntry]);
            net->updateEnabled(ns, currentFirelist[currentEntry]);
            if (!property.checkProperty(ns, currentFirelist[currentEntry]))
            {
//RT::rep->status("not phi --> backtrack");
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
			// new state on stack -> back arc -> witness for EG phi found
			ppp.initialized = true;
			ppp.endCycle(true);
			ppp.addTransition(currentFirelist[currentEntry],true);
			if(newPayload == payload) ppp.beginCycle(true);
			while(dfsstack.StackPointer > 0)
			{
				DFSStackEntry se = dfsstack.top();
				ppp.addTransition(se.fl[se.flIndex],true);
				if(newPayload == se.payload) ppp.beginCycle(true);
				dfsstack.pop();
			}
			return true;   // cycle found!
		}
//RT::rep->status("not on stack -> backtrack");
		// here: known but not on stack anymore 
                // ->backfire the transition
                net->backfire(ns, currentFirelist[currentEntry]);
	    	net->revertEnabled(ns, currentFirelist[currentEntry]);
		property.updateProperty(ns, currentFirelist[currentEntry]);
		continue;
            }
            else
            {
//RT::rep->status("state is new");
                //not yet visited

		if(!ns.CardEnabled)
		{
			// Deadlock reached -> max. path of phi-states found
			// --> witness for EG phi
			ppp.initialized = true;
			ppp.addTransition(currentFirelist[currentEntry],true);
			while(dfsstack.StackPointer > 1)
			{
				DFSStackEntry se = dfsstack.top();
				ppp.addTransition(se.fl[se.flIndex],true);
				dfsstack.pop();
			}
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
