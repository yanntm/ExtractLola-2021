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

\brief Evaluates EFEG property
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Exploration/EFEGExploration.h>

bool EFEGExploration::depth_first(SimpleProperty &property, NetState &ns, Store<int> &myStore,
                                 Firelist &myFirelistnotphi, Firelist &myFirelistphi, int)
{
    // main original ideas for this type of search:
    // We need a nested depth-first search
    // - Outer search goes through states that cannot satisfiy EG phi, because
    // - they do not satisfy phi, or
    // - inner search has already proven that EG phi does not hold
    // - Inner search starts from a state that satsísfies phi, and tries to
    //   prove EG phi, going through states that satisfy phi only.
    // 1. use up set based stubborn sets in states that violate phi
    //    use invisible stubborn sets in states that satisfy phi

    // Closed states carry the following information
    // 0 - closed in both searches
    // 1 - closed in inner search only
    // a state that is closed in outer search, can be regarded as closed in inner search as well, due
    // to the above mentioned invariant for outer search.
    // stateinfo > 1 is a dfs number.
    // even dfs numbers represent states on outer stack
    // odd dfs numbers represent states on inner stack

    bool outer = true; // true if we are in outer search

    // prepare property
    property.value = property.initProperty(ns);

    int * stateinfo;
    myStore.searchAndInsert(ns, &stateinfo, 0);

    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry;

    int dfsnum; // smallest outer dfs number
    int maxdfsnumouter = 0;
    int maxdfsnuminner; // to be initialised when switching from outer to inner search
   if(property.value)
   {
//RT::rep->status("phi");
	// switch to inner search
   	dfsnum = maxdfsnuminner = (*stateinfo) = 3;
   	currentEntry = myFirelistphi.getFirelist(ns, &currentFirelist);
	if(currentEntry == 0) // initial deadlock
	{
		return true;
	}
	outer = false;
   }
   else
   {
//RT::rep->status("not phi");
	// stay in outer search
	dfsnum = (* stateinfo) = ++++maxdfsnumouter;
   	currentEntry = myFirelistnotphi.getFirelist(ns, &currentFirelist);
   }
	

    while (true) // exit when trying to pop from empty stack
    {
//RT::rep->status("new round in %s dfs %d",outer ? "outer" : "inner", dfsnum);
//for(int i = 0; i < net->Card[PL];i++)
//{
	//if(ns.Current[i]) 
	//{
		//std::cerr << net->Name[PL][i] << ":" << ns.Current[i] << " ";
	//}
//}
//std::cerr << std::endl;
//for(int i = 0; i < currentEntry;i++)
//{
	//std::cerr << net->Name[TR][currentFirelist[i]] << " ";
//}
//std::cerr << std::endl;
        if (currentEntry-- > 0)
        {
//RT::rep->status("there is a transition to be fired");
           // there is a next transition that needs to be explored in current marking

           // fire this transition to produce new arking::Current
           net->fire(ns, currentFirelist[currentEntry]);
           net->updateEnabled(ns, currentFirelist[currentEntry]);
           property.value = property.checkProperty(ns, currentFirelist[currentEntry]);

	   // in inner search, we backtrack if new state does not sat phi
	   if(!outer && !property.value)
	   {
//RT::rep->status("inner and not phi -> backtrack");
                        net->backfire(ns, currentFirelist[currentEntry]);
			net->revertEnabled(ns,currentFirelist[currentEntry]);
			property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
			continue;
           }
	   int * newstateinfo;
           if (myStore.searchAndInsert(ns, &newstateinfo, 0))
           {
//RT::rep->status("state exists");
                // State exists!

	        // explore existing state in inner search
		if(!outer )
		{
//RT::rep->status("inner dfs %d",(*newstateinfo));
			// existing state is on inner search stack
			if( (*newstateinfo) > 2 && (*newstateinfo) % 2) // dfs num belongs to state in inner search
			{
//RT::rep->status("on stack -> return true");
				return true;
			}
//RT::rep->status("not on stack -> backtrack");
			// existing state is closed w.r.t. inner search
			net->backfire(ns, currentFirelist[currentEntry]);
			net->revertEnabled(ns,currentFirelist[currentEntry]);
			property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
			continue;
		}
//RT::rep->status("outer dfs %d",(*newstateinfo));
		// deadlock
		if(ns.CardEnabled == 0 && property.value)
		{
//RT::rep->status("outer deadlock");
			return true;
		}
		// explore existing state in outer search
		if((*newstateinfo) != 1) // 0 = closed, >1 = on stack (if current search is outer, all stack entries are outer)
		{
//RT::rep->status("backtrack");
			net->backfire(ns, currentFirelist[currentEntry]);
                        net->revertEnabled(ns,currentFirelist[currentEntry]);
                        property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
                        continue;
		}
//RT::rep->status("seen only in inner search");
		// here: state seen, but only in inner search. This state has to be considered new.
		(*newstateinfo) = dfsnum = ++++maxdfsnumouter;
            }
            else
            {
//RT::rep->status("state is new");
		// state is new
                if (outer)
		{
//RT::rep->status("outer");
			if( property.value)
			{
//RT::rep->status("phi -> switch to inner");
				if(ns.CardEnabled == 0)
				{
					// phi-Deadlock
					return true;
				}
				//switch to inner search
				*newstateinfo = dfsnum = maxdfsnuminner = 3;
				outer = false;
			}
			else
			{
//RT::rep->status("not phi");
				*newstateinfo = dfsnum = ++++maxdfsnumouter;
			}
		}
		else
		{
//RT::rep->status("inner");
			if(property.value && ns.CardEnabled == 0) 
			{
//RT::rep->status("inner deadlock");
				return true;  // EG phi satisfied in deadlock
			}
			*newstateinfo = dfsnum = ++++maxdfsnuminner;
		}
	   }
//RT::rep->status("continue exploratioN");
	   // if arriving here, we want to continue the current exploration
	   new(stack.push()) EFEGStackFrame(currentFirelist, currentEntry,stateinfo);
	   stateinfo = newstateinfo;
	   if(property.value)
	   {
		   currentEntry = myFirelistphi.getFirelist(ns,&currentFirelist);
	   }
           else
	   {
		   currentEntry = myFirelistnotphi.getFirelist(ns,&currentFirelist);
	   }
	   continue;
        }
        else
        {
//RT::rep->status("close state");
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
	    if(dfsnum == 3)
	    {
//RT::rep->status("inner completed -> back to outer");
		// inner serach completed -> continue with outer search
		dfsnum = (* stateinfo) = ++++maxdfsnumouter;
		currentEntry = myFirelistphi.getFirelist(ns,&currentFirelist); // start of inner search sat phi!
		outer = true;
		continue;
	    }
	    * stateinfo = outer ? 0 : 1; // mark state as "not on stack"
            if (stack.StackPointer == 0)
            {
//RT::rep->status("back at initial -> return false");
                // have completely processed initial marking --> state not found
                return false;
            }
//RT::rep->status("backtrack");
            EFEGStackFrame &frame = stack.top();
            currentEntry = frame.current;
            currentFirelist = frame.fl;
	    stateinfo = frame.dfs;
	    dfsnum = *stateinfo;
            stack.pop();
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}

Path EFEGExploration::path() const
{
    return _p;
}
