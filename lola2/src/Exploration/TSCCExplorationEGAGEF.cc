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

\brief do a depth_first search an find tscc's to evaluate a property
*/

#include <Core/Dimensions.h>
#include <Exploration/TSCCExploration.h>
#include <Exploration/ChooseTransition.h>

bool TSCCExplorationEGAGEF::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    //last dfs where property was true
    arrayindex_t lasttrue = 0;

    if (property.initProperty(ns))
    {
        lasttrue = 1;
    }
    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t currentDFSNumber = 0;
    statenumber_t highest_lowlink = -1;

    //set initial dfs to one
    setDFS(payload, ++currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);

    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
            net->fire(ns, currentFirelist[currentEntry]);

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
                    if (newDFS < dfsstack.top().lowlink)
                    {
                        dfsstack.top().lowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                setDFS(newPayload, ++currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;

                //put a entry on the searchstack
                //with the current dfs number and the current lowlink
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, newPayload, currentLowlink);

                //check the given property and
                //save lasttrue dfs number for the given property
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
            //delete the finished firelist
            delete[] currentFirelist;

            //getting v'
            DFSStackEntry &stackentry = dfsstack.top();
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
            stackentry.fl = NULL;
            
            statenumber_t tmpLowlink = stackentry.lowlink;

            //check if this is the root of a tscc
            if (getDFS( (statenumber_t *) stackentry.payload) == stackentry.lowlink
                    && highest_lowlink < stackentry.lowlink)
            {
                //RT::rep->status("root of tscc detected, %u %u", stackentry.lowlink, 
                //        getDFS((statenumber_t *) stackentry.payload));
                //valid for agef :)
                if (lasttrue >= stackentry.lowlink)
                {
                    //RT::rep->status("true");
                    //for the current tscc "add" one more true
                    //just for clarification of the code...
                    property.value = true;
                    return property.value;
                }
                else
                {
                    //RT::rep->status("false");
                    //there is a tscc in which the property is not true
                    //then we can exit going forward to find one path where it
                    //is true
                    property.value = false;
                }
                //update the highest lowlink to the current
                highest_lowlink = stackentry.lowlink;
            }
            //if the stack is empty we are finished
            if (dfsstack.StackPointer == 1)
            {
                //set the property value
                return false;
            }
            // pop the top element on the stack
            dfsstack.pop();
            if (dfsstack.top().lowlink > tmpLowlink)
            {
                dfsstack.top().lowlink = tmpLowlink;
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
