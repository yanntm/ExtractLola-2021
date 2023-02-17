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

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Exploration/EXEFExploration.h>
#include <Exploration/ChooseTransition.h>
#include <SweepLine/Sweep.h>

/*!
The result will be
- true, if a marking fulfilling the property was found
- false, if all markings have been explored and no such state was found
- no termination (in time), if the state space is too big and no 'good' marking
  was found

\param property  the property to check
\param ns  The initial state of the net has to be given as a net state object.
If the search has found a state, fulfilling the property this state will be
returned in this parameter.
\param myStore  the store to be used. The selection of the store may greatly influence the
performance of the program
\param myFirelist  the firelists to use in this search. The firelist _must_ be
applicable to the given property, else the result of this function may be
wrong. It is not guaranteed that the given firelist will actually be used. In
the parallel work-mode the given list will just be used as a base list form
which all other lists will be generated
\param threadNumber  will be ignored by the standard seach. In the parallel
execution mode this number indicates the number of threads to be used for the
search
*/

bool EXEFExploration::depth_first(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                                 Firelist &myFirelist, int)
{
    //// copy initial marking into current marking

    if(ns.CardEnabled == 0) return false;
    // prepare property
    property.value = property.initProperty(ns);

    // add initial marking to store
    // we do not care about return value since we know that store is empty

    // get first firelist
    arrayindex_t *currentFirelist;
    Firelist * fullfirelist = new Firelist(net);
    arrayindex_t currentEntry = fullfirelist->getFirelist(ns, &currentFirelist);

    while (true) // exit when trying to pop from empty stack
    {
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new king::Current
            net->fire(ns, currentFirelist[currentEntry]);

            if (myStore.searchAndInsert(ns, 0, 0))
            {
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
                // State does not exist!
                net->updateEnabled(ns, currentFirelist[currentEntry]);
                // check current marking for property
                property.value = property.checkProperty(ns, currentFirelist[currentEntry]);
                if (property.value)
                {
                    // current  marking satisfies property
                    // push put current transition on stack
                    // this way, the stack contains ALL transitions
                    // of witness path
                    SimpleStackEntry *stack = property.stack.push();
                    stack = new(reinterpret_cast<void *>(stack)) SimpleStackEntry(currentFirelist, currentEntry);
                    return true;
                }

                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = property.stack.push();
                stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            } // end else branch for "if state exists"
        }
        else
        {
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
            if (property.stack.StackPointer == 0)
            {
                // have completely processed initial marking --> state not found
                return false;
            }
            SimpleStackEntry &stack = property.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            property.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}


Path EXEFExploration::path() const
{
    return _p;
}
