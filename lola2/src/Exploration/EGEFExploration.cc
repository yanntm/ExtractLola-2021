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
\status approved 23.05.2012, changed

\brief Evaluates EGEF property
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Exploration/EGEFExploration.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/Firelist.h>

bool EGEFExploration::depth_first(StatePredicateProperty &property, NetState &ns, Store<int> &myStore,
                                 Firelist &myFirelist, int)
{
//net->print();
    // main original ideas for this type of search:
    // 1. store remembers: 
    // - on stack (bit 0)
    // - some successor satisfies phi (bit 1)
    // 2. remember earliest back egde (--> "cycle")
    //     satisfies if between cycle and phi-satisfying state
    // remember dfs (bits 2....)

    // prepare property
    int earliestcycle = 0; // no cycle known yet
    property.value = property.initProperty(ns);

    int * stateinfo;
    myStore.searchAndInsert(ns, &stateinfo, 0);

    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry;
    int dfsnum = 1;
    int maxdfsnum = 1;
   currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
   if(property.value)
   {
//RT::rep->status("PROP HOLDS IN INIT");
	* stateinfo = 7; // state 1 + on stack + has phi-successor
   }
   else
   {
//RT::rep->status("PROP DOES NOT HOLD IN INIT");
	* stateinfo = 5; // state 1 + on stack + has no phi-successor;
   }

    while (true) // exit when trying to pop from empty stack
    {
//RT::rep->status("considering... dfs: %d earliest: %d onstack: %d  phisucc: %d", dfsnum, earliestcycle, *stateinfo & 1, *stateinfo & 2);
//for(arrayindex_t i = 0; i < net->Card[PL];i++)
//{
//	if(ns.Current[i]) RT::rep->status("%s: %d", net->Name[PL][i],ns.Current[i]);
//}
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new arking::Current
RT::rep->status("firing %s at list pos %d", net->Name[TR][currentFirelist[currentEntry]],currentEntry);
            net->fire(ns, currentFirelist[currentEntry]);

	    int * newstateinfo;
            if (myStore.searchAndInsert(ns, &newstateinfo, 0))
            {
//RT::rep->status("succ exists. dfs: %d onstack: %d efphi: %d",*newstateinfo/4,*newstateinfo&1,*newstateinfo&2);
                // State exists!

		// 1. check back edge:
		if((*newstateinfo) & 1) 
		{
			int otherdfs = * newstateinfo / 4;
			if((otherdfs < earliestcycle) || earliestcycle == 0) earliestcycle = otherdfs;
		}

		// 2. check phi-succ
		if((*newstateinfo) & 2)
		{
			* stateinfo |= 2;
		}

		// 3. check EGEF prop
		if(((* stateinfo) & 2) && earliestcycle) 
		{	
//RT::rep->status("return true by fresh cycle");
			return true; 
		}
//RT::rep->status("backtracking to dfs:  %d onstack %d efphi %d", *stateinfo / 4, *stateinfo & 1, *stateinfo & 2);
		// 4. backtrack
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
//RT::rep->status("state is new.");
                // State does not exist!
	    new(stack.push()) EGEFStackFrame(currentFirelist, currentEntry,dfsnum, stateinfo);
            net->updateEnabled(ns, currentFirelist[currentEntry]);
            property.value = property.checkProperty(ns, currentFirelist[currentEntry]);
	    dfsnum = ++maxdfsnum;
	    stateinfo = newstateinfo;
	    *stateinfo = 4 * dfsnum + 1; // state + onstack
	    if(property.value)
	    {
//RT::rep->status("new state sat phi.");
		*stateinfo |= 2;
		// check EGEF prop
		if(earliestcycle) 
		{
//RT::rep->status("return true by fresh phi");
			return true;
		}
		if(ns.CardEnabled == 0) 
		{
//RT::rep->status("return true by deadlock phi");
			return true; // special case: deadlock sat phi -> EGEF phi
		}
            }
		    currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
		    continue;
            } // end else branch for "if state exists"
        }
        else
        {
//RT::rep->status("close state dfs %d SP %d",dfsnum,stack.StackPointer);
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
	    int efphi = (*stateinfo) & 2;
	    * stateinfo ^= 1; // toggle "on stack" bit
            if (stack.StackPointer == 0)
            {

                // have completely processed initial marking --> state not found
                return false;
            }
            EGEFStackFrame &frame = stack.top();
            currentEntry = frame.current;
            currentFirelist = frame.fl;
	    dfsnum = frame.dfs;
	    stateinfo = frame.stinfo;
            stack.pop();
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
	    if(efphi)
	    {
		* stateinfo |= 2;
			    if(earliestcycle)
			    {
				return true;
			    }
            }
	    if(dfsnum < earliestcycle) earliestcycle = 0;
        }
    }
}

Path EGEFExploration::path() const
{
    return _p;
}
