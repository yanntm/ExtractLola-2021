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

\brief Evaluates EU property 
Actual property is a parameter of the constructor
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Exploration/EXEUExploration.h>
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
bool EXEUExploration::depth_first(SimpleProperty &phiproperty, SimpleProperty &psiproperty, NetState &ns, Store<void> &myStore,
                                  Firelist &myFirelist, int)
{
    // prepare property
    psiproperty.value = psiproperty.initProperty(ns);
    phiproperty.value = phiproperty.initProperty(ns);

    // In the first state get the full firelist
    Firelist *fullfl = new Firelist(net);
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = fullfl->getFirelist(ns, &currentFirelist);

    while (true) // exit when trying to pop from empty stack
    {
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new arking::Current
            net->fire(ns, currentFirelist[currentEntry]);
            net->updateEnabled(ns, currentFirelist[currentEntry]);
            psiproperty.value = psiproperty.checkProperty(ns, currentFirelist[currentEntry]);
            if (psiproperty.value)
            {
                return true;
	    }
            phiproperty.value = phiproperty.checkProperty(ns, currentFirelist[currentEntry]);
            if (!phiproperty.value)
            {
                net->backfire(ns, currentFirelist[currentEntry]);
                net->revertEnabled(ns, currentFirelist[currentEntry]);
                psiproperty.value = psiproperty.updateProperty(ns, currentFirelist[currentEntry]);
                phiproperty.value = phiproperty.updateProperty(ns, currentFirelist[currentEntry]);
                continue;
            }
            if (myStore.searchAndInsert(ns, 0, 0))
            {
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
                net->revertEnabled(ns, currentFirelist[currentEntry]);
                psiproperty.value = psiproperty.updateProperty(ns, currentFirelist[currentEntry]);
                phiproperty.value = phiproperty.updateProperty(ns, currentFirelist[currentEntry]);
            }
            else
            {
                // State does not exist!

                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = phiproperty.stack.push();
                stack = new (stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            } // end else branch for "if state exists"
        }
        else
        {
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
            if (phiproperty.stack.StackPointer == 0)
            {
                // have completely processed initial marking --> state not found
                return false;
            }
            SimpleStackEntry &stack = phiproperty.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            phiproperty.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            phiproperty.value = phiproperty.updateProperty(ns, currentFirelist[currentEntry]);
            psiproperty.value = psiproperty.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}

Path EXEUExploration::path() const
{
    return _p;
}
