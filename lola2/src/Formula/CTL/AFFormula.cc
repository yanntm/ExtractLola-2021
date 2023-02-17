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
\author KW 
\status new
\ingroup ctl

\brief Implementation of the CTL AF operator.
*/

#include <Formula/CTL/AFFormula.h>
#include <Exploration/DFSStackEntry.h>
#include <Exploration/Firelist.h>
#include <Exploration/SearchStack.h>

AFFormula::AFFormula(Petrinet * n, CTLFormula *psi) : psi(psi)
{
    net = n;
    assert(psi);
}

void AFFormula::initAtomics(NetState &ns)
{
    psi->initAtomics(ns);
}

void AFFormula::updateAtomics(NetState &ns, arrayindex_t t)
{
    psi->updateAtomics(ns, t);
}

void AFFormula::revertAtomics(NetState &ns, arrayindex_t t)
{
    psi->revertAtomics(ns, t);
}

// LCOV_EXCL_START
void AFFormula::DEBUG_print()
{
    printf("[%u,%u,%zu]AF(", index, dfsindex, payloadsize);
    psi->DEBUG_print();
    printf(")");
}
// LCOV_EXCL_STOP

void AFFormula::gatherPayloadInformation(arrayindex_t &numDFS,
        arrayindex_t &numCachedResults)
{
    dfsindex = (numDFS++) * SIZEOF_STATENUMBER_T;
    index = (numCachedResults++) * 2;
    psi->gatherPayloadInformation(numDFS, numCachedResults);
}

void AFFormula::setPayloadInformation(arrayindex_t cachedResultOffset,
                                      size_t payloadSize)
{
    index += cachedResultOffset * 8;
    payloadsize = payloadSize;
    psi->setPayloadInformation(cachedResultOffset, payloadSize);
}

bool AFFormula::check(Store<void *> &s, NetState &ns, Firelist &firelist, Firelist &sfirelist,
                      std::vector<int> &witness)
{

    Firelist & myfirelist = containsNext ? firelist : sfirelist;
    void **pInitialPayload;
    if (!s.searchAndInsert(ns, &pInitialPayload, 0))
    {
        // all-zeros is starting state for all values
        // memset is required to mimick a calloc on a void* in C++
        *pInitialPayload = ::operator new(payloadsize);
        memset(*pInitialPayload, 0, payloadsize);
    }
    void *payload = *pInitialPayload;

    //RT::rep->status("init AF check at %x (payload: %lx)",ns.HashCurrent,payload);

    CTLFormulaResult cachedResult = getCachedResult(payload);
    if (cachedResult & 2)  // value known
    {
        //RT::rep->status("AF cached %d at %x",cachedResult,ns.HashCurrent);

        return cachedResult & 1; // return result
    }
    assert(cachedResult != IN_PROGRESS); // impossible for first state

    // psi -> AF psi
    if (psi->check(s, ns, firelist, sfirelist, witness))
    {
        setCachedResult(payload, KNOWN_TRUE);

        //RT::rep->status("AF initial TRUE at %x",ns.HashCurrent);

        return true;
    }

    witness.clear();

    //RT::rep->status("starting AF check at %x (payload: %lx)",ns.HashCurrent,payload);

    // dfs stack will contain all gray nodes
    SearchStack<DFSStackEntry> dfsStack;

    // tarjan stack will contain all _black_ nodes the SCC of which is not yet finished
    // see doc/Tarjan for an explanation on how and why this works
    // SearchStack<void *> tarjanStack;

    statenumber_t currentDFSNumber = 1; // starting with 1 to leave 0 for recognizing uninitialized values
    // statenumber_t currentLowlink;

    // fetch firelist
    arrayindex_t *currentFirelist;
    myfirelist.visible = visible;
    arrayindex_t currentFirelistIndex = myfirelist.getFirelist(ns, &currentFirelist);
    arrayindex_t firedAll; // highest dfs where all enabled transition were fired
			   // since stubborn sets for CTL are singleton, we
			   // approximate as: highest dfs where more than 				  
			   // one transition has been fired.
			   // This value is used for controlling the
			   // ignorance problem in stubborn set method

    if(currentFirelistIndex > 1)
    {
	firedAll = 1;
    }
    else
    {
        firedAll = 0;
    }
    // test if initial state is deadlock
    if (!currentFirelistIndex)
    {
        setCachedResult(payload, KNOWN_FALSE);
        delete[] currentFirelist;

        return false;
    }

    // initialize dfs number, lowlink; mark state to be on tarjan stack
    setDFS(payload, 1);
    // currentLowlink = 1;
    setCachedResult(payload, KNOWN_FALSE);

    bool revertEnabledNeeded = false;
    bool backfireNeeded = true;

    while (true)
    {
        if (currentFirelistIndex--)
        {
	    // there is a transition to be fired
            net->fire(ns, currentFirelist[currentFirelistIndex]);
            // don't update enabledness and atomics yet, since it'll maybe not be needed at all.

            void **pNewPayload;
            if (!s.searchAndInsert(ns, &pNewPayload, 0))
            {
                // all-zeros is starting state for all values
                // memset is required to mimick a calloc on a void* in C++
                *pNewPayload = ::operator new(payloadsize);
                memset(*pNewPayload, 0, payloadsize);
            }
            void *newpayload = *pNewPayload;

            //RT::rep->status("AF check fire %s to %x (payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,newpayload);

            CTLFormulaResult newCachedResult = getCachedResult(newpayload);
            if (newCachedResult == UNKNOWN)
            {
		// state not yet seen
                // update enabledness and atomic propositions for current state (needed for further checking)
                net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);
                updateAtomics(ns, currentFirelist[currentFirelistIndex]);

                witness.clear();
		bool psiresult;
                if ((psiresult =psi->check(s, ns, firelist, sfirelist,witness)))
                {
                    setCachedResult(newpayload, KNOWN_TRUE);
                    // continue;

                    //RT::rep->status("AF check backfire (psi) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                    net->backfire(ns, currentFirelist[currentFirelistIndex]);
                    net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                    revertAtomics(ns, currentFirelist[currentFirelistIndex]);
                    continue;
                }
                witness.clear();

                // recursive descent
                new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, payload, 0);

                // get new firelist
                payload = newpayload;
                setDFS(payload, ++currentDFSNumber);
                //currentLowlink = currentDFSNumber;
                setCachedResult(payload, KNOWN_FALSE);
    		myfirelist.visible = visible;
                currentFirelistIndex = myfirelist.getFirelist(ns, &currentFirelist);

                // test if new state is deadlock
                if (!currentFirelistIndex)
                {
		
                    delete[] currentFirelist;
                    backfireNeeded = false;
                    break;
                }

                if((currentFirelistIndex > 1) && (currentDFSNumber > firedAll))
		{
			firedAll = currentDFSNumber;
		}
                continue;
            }
            else if (newCachedResult == KNOWN_FALSE)
            {
                // break; set all nodes to false
                break;
            }
            else     // KNOWN_TRUE
            {
                // continue;

                //RT::rep->status("AF check backfire (KNOWN_TRUE) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                // enabledness and atomics weren't updated, so no revert needed
                continue;
            }
        }
        else     // if(currentFirelistIndex--)
        {
	    // there is no transition to be fired
            // free memory for finished firelist

            // check if SCC is finished
	    // answer is always yes: checkAF cannot have nontrivial SCC since a nontrivial
  	    // SCC contains a cycle that, in turn, would have caused a break.
            statenumber_t dfs = getDFS(payload);
            //if (dfs == currentLowlink)
            //{
                //RT::rep->status("AF found SCC at %x",ns.HashCurrent);

		// First test: do we have ignored transitions?
		if(dfs > firedAll)
		{
			// we have ignored transitions.
			// -->Extend firelist and continue
			firedAll = dfs;
			if(ns.CardEnabled > 1)
			{
				arrayindex_t alreadyFired = currentFirelist[0];
				delete[] currentFirelist;
				currentFirelist = new arrayindex_t[ns.CardEnabled-1];
				currentFirelistIndex = 0;
				for(arrayindex_t i = 0; i < net->Card[TR];i++)
				{
					if(ns.Enabled[i] && (i != alreadyFired))
					{
						currentFirelist[currentFirelistIndex++] = i;
					}
				}
				continue;
			}
		}
                delete[] currentFirelist;

       		//while (tarjanStack.StackPointer && getDFS(tarjanStack.top()) >= dfs)
                		//{
                   		// setCachedResult(tarjanStack.top(), KNOWN_TRUE);
                    		//tarjanStack.pop();
                		//}
                setCachedResult(payload, KNOWN_TRUE);

            //}
            //else
            //{
             //   delete[] currentFirelist;
              //  // SCC not yet finished, push self onto tarjan stack
               // assert(dfs > currentLowlink);
                //*tarjanStack.push() = payload;
            //}

            // check if there are any states to backtrack to
            if (dfsStack.StackPointer)
            {
                currentFirelist = dfsStack.top().fl;
                currentFirelistIndex = dfsStack.top().flIndex;
                payload = dfsStack.top().payload;
                //if (currentLowlink > dfsStack.top().lowlink) // propagate lowlink to parent
                //{
                 //   currentLowlink = dfsStack.top().lowlink;
                //}
                dfsStack.pop();

                //RT::rep->status("AF check backfire (fl) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                revertAtomics(ns, currentFirelist[currentFirelistIndex]);
            }
            else
            {
                //assert(!tarjanStack.StackPointer); // tarjan stack empty
                assert(dfs == currentLowlink); // first node is always start of SCC
                assert(*pInitialPayload == payload); // returned to initial state

                // no (negative) witness path found
                witness.clear();

                //RT::rep->status("AF proven TRUE at %x",ns.HashCurrent);

                return true;
            }
        }
    }
    // revert transition that brought us to the counterexample state
    if (backfireNeeded)
    {
        //RT::rep->status("AF check backfire (backfireNeeded) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

        net->backfire(ns, currentFirelist[currentFirelistIndex]);

        // add transition to witness path
        witness.push_back(currentFirelist[currentFirelistIndex]);

        if (revertEnabledNeeded)
        {
            net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
            revertAtomics(ns, currentFirelist[currentFirelistIndex]);
        }

        // free memory for current firelist
        delete[] currentFirelist;
    }

    // current state can reach counterexample state -> formula false
    setCachedResult(payload, KNOWN_FALSE);

    // all elements that are still on tarjan stack can reach this state -> formula false
    //while (tarjanStack.StackPointer)
    //{
     //   setCachedResult(tarjanStack.top(), KNOWN_FALSE);
      //  tarjanStack.pop();
    //}

    // all elements that are still on dfs stack (and hence on tarjan stack)
    // can reach this state -> formula false
    // revert all the transitions to restore original NetState
    while (dfsStack.StackPointer)
    {
        setCachedResult(dfsStack.top().payload, KNOWN_FALSE);

        //RT::rep->status("AU check backfire (dfsStack) %s",net->Name[TR][dfsStack.top().fl[dfsStack.top().flIndex]]);

        net->backfire(ns, dfsStack.top().fl[dfsStack.top().flIndex]);
        net->revertEnabled(ns, dfsStack.top().fl[dfsStack.top().flIndex]);
        revertAtomics(ns, dfsStack.top().fl[dfsStack.top().flIndex]);

        witness.push_back(dfsStack.top().fl[dfsStack.top().flIndex]);

        // free memory for stacked firelist
        delete[] dfsStack.top().fl;

        dfsStack.pop();
    }
    // (negative) witness found

    //RT::rep->status("AF proven FALSE at %x",ns.HashCurrent);

    return false;
}

bool AFFormula::checkfair(Store<void *> &s, NetState &ns, Firelist &firelist,
                      std::vector<int> &witness)
{
    void **pInitialPayload;
    if (!s.searchAndInsert(ns, &pInitialPayload, 0))
    {
        // all-zeros is starting state for all values
        // memset is required to mimick a calloc on a void* in C++
        *pInitialPayload = ::operator new(payloadsize);
        memset(*pInitialPayload, 0, payloadsize);
    }
    void *payload = *pInitialPayload;

    //RT::rep->status("init AF check at %x (payload: %lx)",ns.HashCurrent,payload);

    CTLFormulaResult cachedResult = getCachedResult(payload);
    if (cachedResult & 2)  // value known
    {
        //RT::rep->status("AU cached %d at %x",cachedResult,ns.HashCurrent);

        return cachedResult & 1; // return result
    }
    assert(cachedResult != IN_PROGRESS); // impossible for first state

    // psi -> A phi U psi
    if (psi->checkfair(s, ns, firelist, witness))
    {
        setCachedResult(payload, KNOWN_TRUE);

        //RT::rep->status("AF initial TRUE at %x",ns.HashCurrent);

        return true;
    }

    witness.clear();

    //rep->status("starting AF check at %x (payload: %lx)",ns.HashCurrent,payload);

    // initialize fairness data
    fairness_data fairness;

    // prepare strong fairness assumptions
    fairness.card_strong = 0;
    for (arrayindex_t i = 0; i < net->Card[TR]; i++)
        if (net->Fairness[i] == STRONG_FAIRNESS)
        {
            fairness.card_strong++;
        }
    fairness.strong_fairness = new arrayindex_t[fairness.card_strong]();
    fairness.strong_backlist = new arrayindex_t[net->Card[TR]]();
    // put all strong fair transitions into an array
    arrayindex_t __card_on_sf = 0;
    for (arrayindex_t i = 0; i < net->Card[TR]; i++)
    {
        if (net->Fairness[i] == STRONG_FAIRNESS)
        {
            fairness.strong_fairness[__card_on_sf] = i;
            fairness.strong_backlist[i] = __card_on_sf++;
        }
        else
        {
            fairness.strong_backlist[i] = -1;
        }
    }

    // prepare weak fairness assumptions
    fairness.card_weak = 0;
    for (arrayindex_t i = 0; i < net->Card[TR]; i++)
        if (net->Fairness[i] == WEAK_FAIRNESS)
        {
            fairness.card_weak++;
        }
    fairness.weak_fairness = new arrayindex_t[fairness.card_weak]();
    fairness.weak_backlist = new arrayindex_t[net->Card[TR]]();
    // put all weak fair transitions into an array
    arrayindex_t __card_on_wf = 0;
    for (arrayindex_t i = 0; i < net->Card[TR]; i++)
    {
        if (net->Fairness[i] == WEAK_FAIRNESS)
        {
            fairness.weak_fairness[__card_on_wf] = i;
            fairness.weak_backlist[i] = __card_on_wf++;
        }
        else
        {
            fairness.weak_backlist[i] = -1;
        }
    }
    fairness.forbidden_transitions = new arrayindex_t[fairness.card_strong]();
    fairness.card_forbidden_transitions = 0;


    // dfs stack will contain all gray nodes
    SearchStack<DFSStackEntry> dfsStack;

    // tarjan stack will contain all _black_ nodes the SCC of which is not yet finished
    // see doc/Tarjan for an explanation on how and why this works
    SearchStack<void *> tarjanStack;

    statenumber_t currentDFSNumber = 1; // starting with 1 to leave 0 for recognizing uninitialized values
    statenumber_t currentLowlink;

    // fetch firelist
    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);
    arrayindex_t firedAll; // highest dfs where all enabled transition were fired
			   // since stubborn sets for CTL are singleton, we
			   // approximate as: highest dfs where more than 				   // one transition has been fired.
			   // This value is used for controlling the
			   // ignorance problem in stubborn set method

    if(currentFirelistIndex > 1)
    {
	firedAll = 1;
    }
    else
    {
        firedAll = 0;
    }
    // test if initial state is deadlock
    if (!currentFirelistIndex)
    {
        setCachedResult(payload, KNOWN_FALSE);
        delete[] currentFirelist;

        return false;
    }

    // initialize dfs number, lowlink; mark state to be on tarjan stack
    setDFS(payload, 1);
    currentLowlink = 1;
    setCachedResult(payload, IN_PROGRESS);

    bool revertEnabledNeeded = false;
    bool backfireNeeded = true;

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);
            // don't update enabledness and atomics yet, since it'll maybe not be needed at all.

            void **pNewPayload;
            if (!s.searchAndInsert(ns, &pNewPayload, 0))
            {
                // all-zeros is starting state for all values
                // memset is required to mimick a calloc on a void* in C++
                *pNewPayload = ::operator new(payloadsize);
                memset(*pNewPayload, 0, payloadsize);
            }
            void *newpayload = *pNewPayload;

            //rep->status("AF check fire %s to %x (payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,newpayload);

            CTLFormulaResult newCachedResult = getCachedResult(newpayload);
            if (newCachedResult == UNKNOWN)
            {
                // update enabledness and atomic propositions for current state (needed for further checking)
                net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);
                updateAtomics(ns, currentFirelist[currentFirelistIndex]);

                witness.clear();
                if (psi->checkfair(s, ns, firelist, witness))
                {
                    setCachedResult(newpayload, KNOWN_TRUE);
                    // continue;

                    //rep->status("AU check backfire (psi) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                    net->backfire(ns, currentFirelist[currentFirelistIndex]);
                    net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                    revertAtomics(ns, currentFirelist[currentFirelistIndex]);
                    continue;
                }

                witness.clear();

                // recursive descent
                new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, payload, currentLowlink);

                // get new firelist
                payload = newpayload;
    firelist.visible = visible;
                currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

                // test if new state is deadlock
                if (!currentFirelistIndex)
                {
                    delete[] currentFirelist;
                    backfireNeeded = false;
                    break;
                }

                setDFS(newpayload, ++currentDFSNumber);
                currentLowlink = currentDFSNumber;
                setCachedResult(newpayload, IN_PROGRESS);
                if((currentFirelistIndex > 1) && (currentDFSNumber > firedAll))
		{
			firedAll = currentDFSNumber;
		}
                continue;
            }
            else if (newCachedResult == KNOWN_FALSE)
            {
                // break; set all nodes to false
                break;
            }
            else if (newCachedResult == IN_PROGRESS)
            {
                // update lowlink and continue
                statenumber_t newdfs = getDFS(newpayload);
                if (newdfs < currentLowlink)
                {
                    currentLowlink = newdfs;
                }

                //rep->status("AF check backfire (IN_PROGRESS) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                // enabledness and atomics weren't updated, so no revert needed
                continue;
            }
            else     // KNOWN_TRUE
            {
                // continue;

                //rep->status("AF check backfire (KNOWN_TRUE) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                // enabledness and atomics weren't updated, so no revert needed
                continue;
            }
        }
        else     // if(currentFirelistIndex--)
        {
            // free memory for finished firelist

            // check if SCC is finished
            statenumber_t dfs = getDFS(payload);
            if (dfs == currentLowlink)
            {
                //rep->status("AF found SCC at %x",ns.HashCurrent);

		// First test: do we have ignored transitions?
		if(dfs > firedAll)
		{
			// we have ignored transitions.
			// -->Extend firelist and continue
			firedAll = dfs;
			if(ns.CardEnabled > 1)
			{
				arrayindex_t alreadyFired = currentFirelist[0];
				delete[] currentFirelist;
				currentFirelist = new arrayindex_t[ns.CardEnabled-1];
				currentFirelistIndex = 0;
				for(arrayindex_t i = 0; i < net->Card[TR];i++)
				{
					if(ns.Enabled[i] && (i != alreadyFired))
					{
						currentFirelist[currentFirelistIndex++] = i;
					}
				}
				continue;
			}
		}
                delete[] currentFirelist;

                // SCC finished, test if fair witness can be found
                fairness.card_forbidden_transitions = 0; // reset fairness data
                if (getFairWitness(s, ns, firelist, witness, fairness))
                {
                    //rep->status("AF found fair SCC at %x",ns.HashCurrent);
                    // no need to pop elements from Tarjan stack to set them to KNOWN_FALSE, this is done later anyway
                    backfireNeeded = false;
                    break;
                }
                //rep->status("AF found unfair SCC at %x",ns.HashCurrent);

                // all elements on tarjan stack that have a higher dfs number then ours belong to the finished SCC -> formula true
                while (tarjanStack.StackPointer && getDFS(tarjanStack.top()) >= dfs)
                {
                    setCachedResult(tarjanStack.top(), KNOWN_TRUE);
                    tarjanStack.pop();
                }
                setCachedResult(payload, KNOWN_TRUE);
            }
            else
            {
                delete[] currentFirelist;
                // SCC not yet finished, push self onto tarjan stack
                assert(dfs > currentLowlink);
                *tarjanStack.push() = payload;
            }

            // check if there are any states to backtrack to
            if (dfsStack.StackPointer)
            {
                currentFirelist = dfsStack.top().fl;
                currentFirelistIndex = dfsStack.top().flIndex;
                payload = dfsStack.top().payload;
                if (currentLowlink > dfsStack.top().lowlink) // propagate lowlink to parent
                {
                    currentLowlink = dfsStack.top().lowlink;
                }
                dfsStack.pop();

                //rep->status("AU check backfire (fl) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                revertAtomics(ns, currentFirelist[currentFirelistIndex]);
            }
            else
            {
                assert(!tarjanStack.StackPointer); // tarjan stack empty
                assert(dfs == currentLowlink); // first node is always start of SCC
                assert(*pInitialPayload == payload); // returned to initial state

                // no (negative) witness path found
                witness.clear();

                delete[] fairness.strong_backlist;
                delete[] fairness.strong_fairness;
                delete[] fairness.weak_backlist;
                delete[] fairness.weak_fairness;
                delete[] fairness.forbidden_transitions;


                //rep->status("AF proven TRUE at %x",ns.HashCurrent);

                return true;
            }
        }
    }
    // revert transition that brought us to the counterexample state
    if (backfireNeeded)
    {
        //rep->status("AF check backfire (backfireNeeded) %s",net->Name[TR][currentFirelist[currentFirelistIndex]]);

        net->backfire(ns, currentFirelist[currentFirelistIndex]);

        // add transition to witness path
        witness.push_back(currentFirelist[currentFirelistIndex]);

        if (revertEnabledNeeded)
        {
            net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
            revertAtomics(ns, currentFirelist[currentFirelistIndex]);
        }

        // free memory for current firelist
        delete[] currentFirelist;
    }

    // current state can reach counterexample state -> formula false
    setCachedResult(payload, KNOWN_FALSE);

    // all elements that are still on tarjan stack can reach this state -> formula false
    while (tarjanStack.StackPointer)
    {
        setCachedResult(tarjanStack.top(), KNOWN_FALSE);
        tarjanStack.pop();
    }

    // all elements that are still on dfs stack (and hence on tarjan stack)
    // can reach this state -> formula false
    // revert all the transitions to restore original NetState
    while (dfsStack.StackPointer)
    {
        setCachedResult(dfsStack.top().payload, KNOWN_FALSE);

        //rep->status("AF check backfire (dfsStack) %s",net->Name[TR][dfsStack.top().fl[dfsStack.top().flIndex]]);

        net->backfire(ns, dfsStack.top().fl[dfsStack.top().flIndex]);
        net->revertEnabled(ns, dfsStack.top().fl[dfsStack.top().flIndex]);
        revertAtomics(ns, dfsStack.top().fl[dfsStack.top().flIndex]);

        witness.push_back(dfsStack.top().fl[dfsStack.top().flIndex]);

        // free memory for stacked firelist
        delete[] dfsStack.top().fl;

        dfsStack.pop();
    }
    // (negative) witness found

    delete[] fairness.strong_backlist;
    delete[] fairness.strong_fairness;
    delete[] fairness.weak_backlist;
    delete[] fairness.weak_fairness;
    delete[] fairness.forbidden_transitions;

    //rep->status("AF proven FALSE at %x",ns.HashCurrent);

    return false;
}

/// finds witness path that fulfills all fairness assumptions if any exists
/// assumes that a SCC was just found that represents a potential (counter-)example, with ns being the first state of that SCC
/// returns true, if witness path is found, otherwise false
bool AFFormula::getFairWitness(Store<void *> &s, NetState &ns,
                               Firelist &firelist, std::vector<int> &witness,
                               fairness_data &fairness)
{
    void **pPayload;
    s.searchAndInsert(ns, &pPayload, 0);
    statenumber_t initialDFS = getDFS(
                                   *pPayload); // all states in the SCC will be set to this DFS number to mark the area for later steps

    //rep->status("getFairWitness init at %x initialDFS %ld (pPayload: %lx, payload: %lx)",ns.HashCurrent, initialDFS, pPayload, *pPayload);

    SearchStack<DFSStackEntry> dfsStack;
    bool nonTrivial = false; // states whether current SCC contains any cycles

    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

    // flags which weak fairness assumptions are met
    bool *fulfilled_weak = new bool[fairness.card_weak]();
    // flags which strong fairness assumptions are met
    bool *fulfilled_strong = new bool[fairness.card_strong]();
    // flags which strong fair transitions were enabled in at least one of the SCC states
    bool *enabled_strong = new bool[fairness.card_strong]();

    // check for weak fairness enabledness (not enabled -> fulfilled)
    for (size_t t = 0; t < fairness.card_weak; t++)
    {
        if (!ns.Enabled[fairness.weak_fairness[t]])
        {
            fulfilled_weak[t] = true;
        }
    }

    // check for strong fairness enabledness
    for (size_t t = 0; t < currentFirelistIndex; t++)
    {
        if (fairness.strong_backlist[currentFirelist[t]] != (int) - 1)
        {
            enabled_strong[fairness.strong_backlist[currentFirelist[t]]] = true;
        }
    }

    // re-initialize state for possible recursive DFS search. Will be overridden again once the fairness check is done.
    setCachedResult(*pPayload, UNKNOWN);

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);

            void **pNewPayload;
            s.searchAndInsert(ns, &pNewPayload, 0);
            statenumber_t curDFS = getDFS(*pNewPayload);
            // test if new state belongs to this SCC

            //rep->status("getFairWitness fire %s to %x (pPayload: %lx, payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,pNewPayload,*pNewPayload);

            //rep->status("getFairWitness dfs %ld result %d",curDFS,getCachedResult(*pNewPayload));

            CTLFormulaResult result = getCachedResult(*pNewPayload);
            if (result & 2 || (curDFS != initialDFS && (result != IN_PROGRESS || curDFS < initialDFS)))
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // found edge to another SCC state (maybe self-loop)
            nonTrivial = true;

            // check if used transition is needed for weak fairness
            if (fairness.weak_backlist[currentFirelist[currentFirelistIndex]] != (int) - 1)
            {
                fulfilled_weak[fairness.weak_backlist[currentFirelist[currentFirelistIndex]]] = true;
            }

            // check if used transition is needed for strong fairness
            if (fairness.strong_backlist[currentFirelist[currentFirelistIndex]] != (int) - 1)
            {
                fulfilled_strong[fairness.strong_backlist[currentFirelist[currentFirelistIndex]]] = true;
            }

            // test if state was already visited
            if (curDFS == initialDFS)
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // mark state as visited
            setDFS(*pNewPayload, initialDFS);
            // re-initialize state for possible recursive DFS search. Will be overridden again once the fairness check is done.
            setCachedResult(*pNewPayload, UNKNOWN);

            // update enabledness for further search
            net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);

            // store current firelist to stack
            new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, *pNewPayload, 0);
            // get new firelists
    firelist.visible = visible;
            currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

            // check for weak fairness enabledness (not enabled -> fulfilled)
            for (arrayindex_t t = 0; t < fairness.card_weak; t++)
            {
                if (!ns.Enabled[fairness.weak_fairness[t]])
                {
                    fulfilled_weak[t] = true;
                }
            }

            // check for strong fairness enabledness
            for (arrayindex_t t = 0; t < currentFirelistIndex; t++)
            {
                if (fairness.strong_backlist[currentFirelist[t]] != (int) - 1)
                {
                    enabled_strong[fairness.strong_backlist[currentFirelist[t]]] = true;
                }
            }
        }
        else
        {
            delete[] currentFirelist;

            //rep->status("getFairWitness backtrack from %x",ns.HashCurrent);

            // check if there are any states to backtrack to
            if (dfsStack.StackPointer)
            {
                currentFirelist = dfsStack.top().fl;
                currentFirelistIndex = dfsStack.top().flIndex;
                dfsStack.pop();

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                //rep->status("backfire %s (%x %d) to %x",net->Name[TR][currentFirelist[currentFirelistIndex]],currentFirelist,currentFirelistIndex,ns.HashCurrent);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
            }
            else
            {
                break;
            }
        }
    }

    bool weakFair = true;
    bool strongFair = true;

    // if area is trivial, there is no witness
    if (!nonTrivial)
    {
        weakFair = false;
    }

    // test if weak fairness assumption is violated
    for (arrayindex_t t = 0; t < fairness.card_weak; t++)
        if (!fulfilled_weak[t])
        {
            weakFair = false;
        }

    // test if strong fairness assumptions are violated, add them to forbidden transitions in case
    for (arrayindex_t t = 0; t < fairness.card_strong; t++)
        if (!fulfilled_strong[t] && enabled_strong[t])
        {
            strongFair = false;
            fairness.forbidden_transitions[fairness.card_forbidden_transitions++] = fairness.strong_fairness[t];
        }

    // construct witness path if fairness test passed
    if (weakFair && strongFair)
    {
        constructWitness(s, ns, firelist, witness, fairness, enabled_strong);
    }

    // clean up
    delete[] fulfilled_weak;
    delete[] fulfilled_strong;
    delete[] enabled_strong;

    if (!weakFair)
    {
        return false;
    }

    if (!strongFair)
    {
        return subdivideFairnessCheck(s, ns, firelist, witness, fairness);
    }
    else
    {
        return true;
    }
}

// traverses the fair SCC and finds a cycle that will represent the (infinite) the witness path
// assumes that ns is the first SCC state, the SCC is non-trivial and all SCC states are set to the same DFS number and UNKNOWN result
// assures that ns is unchanged and witness contains a fair witness path starting from the initial state
void AFFormula::constructWitness(Store<void *> &s, NetState &ns,
                                 Firelist &firelist, std::vector<int> &witness,
                                 fairness_data &fairness, bool *enabled_strong)
{
    void **pPayload;
    s.searchAndInsert(ns, &pPayload, 0);

    //rep->status("constructWitness init at %x (pPayload: %lx, payload: %lx)",ns.HashCurrent,pPayload,*pPayload);

    bool *fulfilled_weak = new bool[fairness.card_weak]();
    bool *fulfilled_strong = new bool[fairness.card_strong]();

    arrayindex_t fulfilled_conditions = 0;

    // if a strong fair transition is never enabled, it is fulfilled
    for (arrayindex_t t = 0; t < fairness.card_strong; t++)
        if (!enabled_strong[t])
        {
            fulfilled_strong[t] = true;
            fulfilled_conditions++;
        }

    // check for weak fairness enabledness (not enabled -> fulfilled)
    for (arrayindex_t t = 0; t < fairness.card_weak; t++)
        if (!fulfilled_weak[t] && !ns.Enabled[fairness.weak_fairness[t]])
        {
            fulfilled_weak[t] = true;
            fulfilled_conditions++;
        }

    produceWitness(s, ns, firelist, witness, fairness, *pPayload, getDFS(*pPayload), fulfilled_weak,
                   fulfilled_strong, fulfilled_conditions);

    witness.push_back(-1); // mark begin of cycle

    delete[] fulfilled_weak;
    delete[] fulfilled_strong;
}

void AFFormula::produceWitness(Store<void *> &s, NetState &ns,
                               Firelist &firelist, std::vector<int> &witness,
                               fairness_data &fairness, void *initialPayload,
                               statenumber_t initialDFS, bool *fulfilled_weak,
                               bool *fulfilled_strong,
                               arrayindex_t initial_fulfilled_conditions)
{
    statenumber_t myDFS = initialDFS + initial_fulfilled_conditions + 1;
    arrayindex_t current_fulfilled_conditions = initial_fulfilled_conditions;

    void **pPayload;
    s.searchAndInsert(ns, &pPayload, 0);

    //rep->status("produceWitness init at %x (payload: %lx, myDFS: %ld, fc: %d)",ns.HashCurrent,*pPayload,myDFS,initial_fulfilled_conditions);

    SearchStack<DFSStackEntry> dfsStack;

    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

    // starting state is not marked as visited to detect loops if any are present

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);

            void **pNewPayload;
            s.searchAndInsert(ns, &pNewPayload, 0);
            statenumber_t curDFS = getDFS(*pNewPayload);

            //rep->status("produceWitness fire %s to %x (payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,*pPayload);

            //rep->status("produceWitness dfs %ld result %d",curDFS,getCachedResult(*pNewPayload));

            // test if new state belongs to this SCC
            if (getCachedResult(*pNewPayload) != UNKNOWN || curDFS < initialDFS || curDFS > myDFS)
            {
                //rep->status("produceWitness backfire");
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // check if used transition is needed for weak fairness
            if (fairness.weak_backlist[currentFirelist[currentFirelistIndex]] != (int) - 1
                    && !fulfilled_weak[fairness.weak_backlist[currentFirelist[currentFirelistIndex]]])
            {
                fulfilled_weak[fairness.weak_backlist[currentFirelist[currentFirelistIndex]]] = true;
                current_fulfilled_conditions++;
            }

            // check if used transition is needed for strong fairness
            if (fairness.strong_backlist[currentFirelist[currentFirelistIndex]] != (int) - 1
                    && !fulfilled_strong[fairness.strong_backlist[currentFirelist[currentFirelistIndex]]])
            {
                fulfilled_strong[fairness.strong_backlist[currentFirelist[currentFirelistIndex]]] = true;
                current_fulfilled_conditions++;
            }

            // update enabledness for further tests
            net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);

            // check for weak fairness enabledness (not enabled -> fulfilled)
            for (arrayindex_t t = 0; t < fairness.card_weak; t++)
                if (!fulfilled_weak[t] && !ns.Enabled[fairness.weak_fairness[t]])
                {
                    fulfilled_weak[t] = true;
                    current_fulfilled_conditions++;
                }

            // test if all assumptions are met now, or at least some new
            // note that initial_fulfilled_conditions can be fairness.card_strong + fairness.card_weak as well if all fairness assumptions were met in the initial state. Still, a cycle needs to be found.
            if (current_fulfilled_conditions == fairness.card_strong + fairness.card_weak
                    || current_fulfilled_conditions > initial_fulfilled_conditions)
            {
                //rep->status("produceWitness fc: %d, cs: %d, cw: %d",current_fulfilled_conditions,fairness.card_strong,fairness.card_weak);

                if (current_fulfilled_conditions == fairness.card_strong + fairness.card_weak)
                {
                    // all assumptions are met now, find a path back to the initial node
                    findWitnessPathTo(s, ns, firelist, witness, initialPayload, initialDFS, myDFS + 1);
                }
                else
                {
                    // some new assumptions are met, recursively find the rest of the witness path that needs to fulfill fewer assumptions
                    produceWitness(s, ns, firelist, witness, fairness, initialPayload, initialDFS, fulfilled_weak,
                                   fulfilled_strong, current_fulfilled_conditions);
                }

                // extend witness path with the transitions used to reach this state

                // fired transitions are not backfired since the last state we reached to complete the loop was the initial state
                witness.push_back(currentFirelist[currentFirelistIndex]);

                delete[] currentFirelist;

                while (dfsStack.StackPointer)
                {
                    currentFirelist = dfsStack.top().fl;
                    currentFirelistIndex = dfsStack.top().flIndex;
                    dfsStack.pop();

                    witness.push_back(currentFirelist[currentFirelistIndex]);

                    delete[] currentFirelist;
                }
                return;
            }


            // test if new state belongs to this SCC and is unvisited
            if (curDFS == myDFS)
            {
                //rep->status("produceWitness backfire2");
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // mark state visited
            setDFS(*pNewPayload, myDFS);

            // store current firelist to stack
            new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, *pNewPayload, 0);
            // get new firelists
    firelist.visible = visible;
            currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);
        }
        else
        {
            delete[] currentFirelist;

            //rep->status("produceWitness backtrack from %x",ns.HashCurrent);

            // we can't run out of stack frames since a valid witness path is guaranteed to exist
            assert(dfsStack.StackPointer);

            currentFirelist = dfsStack.top().fl;
            currentFirelistIndex = dfsStack.top().flIndex;
            dfsStack.pop();

            net->backfire(ns, currentFirelist[currentFirelistIndex]);
            net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
        }
    }
}


void AFFormula::findWitnessPathTo(Store<void *> &s, NetState &ns,
                                  Firelist &firelist, std::vector<int> &witness,
                                  void *destinationPayload,
                                  statenumber_t initialDFS, statenumber_t myDFS)
{
    void **pPayload;
    s.searchAndInsert(ns, &pPayload, 0);

    SearchStack<DFSStackEntry> dfsStack;

    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

    witness.clear();

    //rep->status("findWitnessPathTo init at %x (payload: %lx)",ns.HashCurrent, destinationPayload);

    //rep->status("findWitnessPathTo start %lx", *pPayload);

    // starting state is the destination, witness path is empty
    if (*pPayload == destinationPayload)
    {
        return;
    }

    // mark state visited
    setDFS(*pPayload, myDFS);

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);

            void **pNewPayload;
            s.searchAndInsert(ns, &pNewPayload, 0);
            statenumber_t curDFS = getDFS(*pNewPayload);

            //rep->status("findWitnessPathTo fire %s to %x (payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,*pNewPayload);

            //rep->status("findWitnessPathTo dfs %ld result %d",curDFS,getCachedResult(*pNewPayload));

            // test if new state belongs to this SCC and is unvisited
            if (getCachedResult(*pNewPayload) != UNKNOWN || curDFS < initialDFS || curDFS >= myDFS)
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // update enabledness for further search
            net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);

            // current state is the destination
            if (*pNewPayload == destinationPayload)
            {
                // fired transitions are not backfired since the state we just found is the initial state
                witness.push_back(currentFirelist[currentFirelistIndex]);

                delete[] currentFirelist;

                while (dfsStack.StackPointer)
                {
                    currentFirelist = dfsStack.top().fl;
                    currentFirelistIndex = dfsStack.top().flIndex;
                    dfsStack.pop();

                    witness.push_back(currentFirelist[currentFirelistIndex]);

                    delete[] currentFirelist;
                }
                return;
            }

            // mark state visited
            setDFS(*pNewPayload, myDFS);

            // store current firelist to stack
            new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, *pNewPayload, 0);
            // get new firelists
    firelist.visible = visible;
            currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);
        }
        else
        {
            delete[] currentFirelist;

            // we can't run out of stack frames since a valid witness path is guaranteed to exist
            assert(dfsStack.StackPointer);

            currentFirelist = dfsStack.top().fl;
            currentFirelistIndex = dfsStack.top().flIndex;
            dfsStack.pop();

            net->backfire(ns, currentFirelist[currentFirelistIndex]);
            net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
        }
    }
}

// traverses the SCC and starts recursive fairness checks for sub-SCCs resulting from the new forbidden transitions.
// assumes that ns is the first SCC state and all SCC states are set to the same DFS number and UNKNOWN result
// returns true, if fair witness path was found, otherwise false
// if returning false, assures that ns is unchanged and all SCC states kept their (identical) DFS number and have KNOWN_TRUE result (the result will always be overridden afterwards by the calling method)
// if returning true, assures that ns is unchanged and witness contains a fair witness path starting from the initial state
bool AFFormula::subdivideFairnessCheck(Store<void *> &s, NetState &ns,
                                       Firelist &firelist,
                                       std::vector<int> &witness,
                                       fairness_data &fairness)
{
    void **pPayload;
    s.searchAndInsert(ns, &pPayload, 0);
    statenumber_t initialDFS = getDFS(
                                   *pPayload); // all states in the SCC are assumed to have this DFS number (set by getFairWitness)

    SearchStack<DFSStackEntry> dfsStack;

    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

    //rep->status("subdivideFairnessCheck init at %x",ns.HashCurrent);

    // start SCC search from this state
    if (fairSCC(s, ns, firelist, witness, fairness))
    {
        return true;
    }
    setCachedResult(*pPayload, KNOWN_TRUE);

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);

            void **pNewPayload;
            s.searchAndInsert(ns, &pNewPayload, 0);
            statenumber_t curDFS = getDFS(*pNewPayload);

            //rep->status("subdivideFairnessCheck fire %s to %x (pPayload: %lx, payload: %lx)",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent,pNewPayload,*pNewPayload);

            // test if new state belongs to this SCC
            if (curDFS != initialDFS)
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                //rep->status("subdivideFairnessCheck backfire (initialDFS) %s to %x",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent);
                continue;
            }

            CTLFormulaResult newResult = getCachedResult(*pNewPayload);

            // test if state was already visited by this DFS (or some previous search that happened to use the same DFS number)
            if (newResult == KNOWN_TRUE)
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                //rep->status("subdivideFairnessCheck backfire (KNOWN_TRUE) %s to %x",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent);
                continue;
            }

            // update enabledness for further search
            net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);

            // test if node not already visited by a previous SCC search
            if (newResult == UNKNOWN)
            {
                // start SCC search from this state
                //rep->status("subdivideFairnessCheck testing SCC at %x",ns.HashCurrent);
                if (fairSCC(s, ns, firelist, witness, fairness))
                {
                    //rep->status("subdivideFairnessCheck SCC at %x is fair",ns.HashCurrent);
                    // extend witness path with all transitions that were fired to get here; revert ns to initial state; cleanup

                    // add transition that was used to get here to witness
                    net->backfire(ns, currentFirelist[currentFirelistIndex]);
                    net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                    witness.push_back(currentFirelist[currentFirelistIndex]);

                    delete[] currentFirelist;

                    // add all transitions on DFS stack to witness
                    while (dfsStack.StackPointer)
                    {
                        currentFirelist = dfsStack.top().fl;
                        currentFirelistIndex = dfsStack.top().flIndex;
                        dfsStack.pop();

                        net->backfire(ns, currentFirelist[currentFirelistIndex]);
                        net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                        witness.push_back(currentFirelist[currentFirelistIndex]);

                        delete[] currentFirelist;
                    }

                    return true;
                }
                //rep->status("subdivideFairnessCheck SCC at %x is not fair",ns.HashCurrent);
            }
            // getCachedResult(*pNewPayload) is set to KNOWN_FALSE by one of the fairSCC searches at this point
            assert(getCachedResult(*pNewPayload) == KNOWN_FALSE);

            // mark state visited
            setCachedResult(*pNewPayload, KNOWN_TRUE);

            // store current firelist to stack
            new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, *pNewPayload, 0);
            // get new firelists
    firelist.visible = visible;
            currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);
        }
        else
        {
            delete[] currentFirelist;

            // check if there are any states to backtrack to
            if (dfsStack.StackPointer)
            {
                currentFirelist = dfsStack.top().fl;
                currentFirelistIndex = dfsStack.top().flIndex;
                dfsStack.pop();

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                //rep->status("backfire %s (%x %d) to %x",net->Name[TR][currentFirelist[currentFirelistIndex]],currentFirelist,currentFirelistIndex,ns.HashCurrent);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                //rep->status("subdivideFairnessCheck backfire (StackPointer) %s to %x",net->Name[TR][currentFirelist[currentFirelistIndex]],ns.HashCurrent);
            }
            else
            {
                // SCC fully explored and no fair path was found
                return false;
            }
        }
    }
}

// performs a SCC search (Tarjan) starting from ns and considering all forbidden transitions. For each SCC, the fairness conditions are checked.
// assumes that all states that need to be explored have the same DFS number and UNKNOWN result
// returns true, if fair witness path was found, otherwise false
// if returning false, it assures that ns is unchanged and all states that were explored kept their (identical) DFS number and have KNOWN_FALSE result
// if returning true, it assures that ns is unchanged and the witness variable contains a fair witness path starting from the initial state of the search
bool AFFormula::fairSCC(Store<void *> &s, NetState &ns,
                        Firelist &firelist, std::vector<int> &witness,
                        fairness_data &fairness)
{
    void **pInitialPayload;
    s.searchAndInsert(ns, &pInitialPayload, 0);
    void *payload = *pInitialPayload;
    statenumber_t initialDFS = getDFS(
                                   payload); // all states in the considered area are assumed to have this DFS number

    // test if any forbidden transition is enabled in the initial state
    for (arrayindex_t i = 0; i < fairness.card_forbidden_transitions; i++)
        if (ns.Enabled[fairness.forbidden_transitions[i]])
        {
            setCachedResult(payload, KNOWN_FALSE);
            return false;
        }

    // dfs stack will contain all gray nodes
    SearchStack<DFSStackEntry> dfsStack;

    // tarjan stack will contain all _black_ nodes the SCC of which is not yet finished
    // see doc/Tarjan for an explanation on how and why this works
    SearchStack<void *> tarjanStack;

    statenumber_t currentDFSNumber =
        initialDFS; // starting with initialDFS+1 to leave initialDFS for recognizing uninitialized values
    statenumber_t currentLowlink;

    // fetch firelist
    arrayindex_t *currentFirelist;
    firelist.visible = visible;
    arrayindex_t currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

    // initialize dfs number, lowlink; mark state to be on tarjan stack
    setDFS(payload, ++currentDFSNumber);
    currentLowlink = currentDFSNumber;
    setCachedResult(payload, IN_PROGRESS);

    while (true)
    {
        if (currentFirelistIndex--)
        {
            net->fire(ns, currentFirelist[currentFirelistIndex]);

            void **pNewPayload;
            s.searchAndInsert(ns, &pNewPayload, 0);

            CTLFormulaResult newCachedResult = getCachedResult(*pNewPayload);
            statenumber_t newDFS = getDFS(*pNewPayload);

            if (newCachedResult & 2)  // result is KNOWN_TRUE or KNOWN_FALSE
            {
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }

            // check if new state belongs to this area and is UNKNOWN
            if (newCachedResult == UNKNOWN && newDFS == initialDFS)
            {
                // updating enabledness is necessary for further tests
                net->updateEnabled(ns, currentFirelist[currentFirelistIndex]);

                // check if new state enables forbidden transitions
                bool forbidden = false;
                for (arrayindex_t i = 0; i < fairness.card_forbidden_transitions; i++)
                    if (ns.Enabled[fairness.forbidden_transitions[i]])
                    {
                        forbidden = true;
                        break;
                    }
                if (forbidden)
                {
                    net->backfire(ns, currentFirelist[currentFirelistIndex]);
                    net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                    continue;
                }

                // recursive descent
                new (dfsStack.push()) DFSStackEntry(currentFirelist, currentFirelistIndex, payload, currentLowlink);

                // get new firelist
                payload = *pNewPayload;
    firelist.visible = visible;
                currentFirelistIndex = firelist.getFirelist(ns, &currentFirelist);

                setDFS(payload, ++currentDFSNumber);
                currentLowlink = currentDFSNumber;
                setCachedResult(payload, IN_PROGRESS);
                continue;
            }
            else
            {
                if (newCachedResult == IN_PROGRESS
                        || newDFS >=
                        initialDFS)  // test if state belongs to this search (there may also be other running SCC searches on the call stack)
                {
                    // update lowlink
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                continue;
            }
        }
        else     // if(currentFirelistIndex--)
        {
            // free memory for finished firelist
            delete[] currentFirelist;

            // check if SCC is finished
            statenumber_t dfs = getDFS(payload);
            if (dfs == currentLowlink)
            {
                // SCC finished, test if fair witness can be found
                if (getFairWitness(s, ns, firelist, witness, fairness))
                {
                    // fair witness found, extend witness path and cleanup

                    // add all transitions on DFS stack to witness
                    while (dfsStack.StackPointer)
                    {
                        currentFirelist = dfsStack.top().fl;
                        currentFirelistIndex = dfsStack.top().flIndex;
                        payload = dfsStack.top().payload;
                        dfsStack.pop();

                        net->backfire(ns, currentFirelist[currentFirelistIndex]);
                        net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                        witness.push_back(currentFirelist[currentFirelistIndex]);

                        delete[] currentFirelist;
                    }
                    return true;
                }
                // SCC contained no fair witness, setting all nodes to KNOWN_FALSE and reset their DFS number to initialDFS
                // note that KNOWN_FALSE cannot occur in any reachable node (otherwise it would be a counterexample path that would have been found in CheckAU)
                while (tarjanStack.StackPointer && getDFS(tarjanStack.top()) > dfs)
                {
                    setCachedResult(tarjanStack.top(), KNOWN_FALSE);
                    setDFS(tarjanStack.top(), initialDFS);
                    tarjanStack.pop();
                }
                setCachedResult(payload, KNOWN_FALSE);
                setDFS(payload, initialDFS);
            }
            else
            {
                // SCC not yet finished, push self onto Tarjan stack
                assert(dfs > currentLowlink);
                *tarjanStack.push() = payload;
            }

            // check if there are any states to backtrack to
            if (dfsStack.StackPointer)
            {
                currentFirelist = dfsStack.top().fl;
                currentFirelistIndex = dfsStack.top().flIndex;
                payload = dfsStack.top().payload;
                if (currentLowlink > dfsStack.top().lowlink) // propagate lowlink to parent
                {
                    currentLowlink = dfsStack.top().lowlink;
                }
                dfsStack.pop();

                net->backfire(ns, currentFirelist[currentFirelistIndex]);
                net->revertEnabled(ns, currentFirelist[currentFirelistIndex]);
                revertAtomics(ns, currentFirelist[currentFirelistIndex]);
            }
            else
            {
                assert(!tarjanStack.StackPointer); // tarjan stack empty
                assert(dfs == currentLowlink); // first node is always start of SCC
                assert(*pInitialPayload == payload); // returned to initial state

                // no fair witness path found. The result of all states have been set to KNOWN_FALSE and the DFS numbers were reset to initialDFS.
                return false;
            }
        }
    }
}

FormulaInfo *AFFormula::getInfo() const
{
    FormulaInfo *Info = new FormulaInfo();
    Info->tag = formula_af;
    Info->cardChildren = 1;
    Info->ctlChildren = new CTLFormula * [1];
    Info->ctlChildren[0] = psi;
    return Info;
}

int AFFormula::countSubFormulas() const
{
    return 1 + psi->countSubFormulas();
}

void AFFormula::print()
{
	std::cout << "AF ( ";
	psi->print();
	std::cout << ")";
}

void AFFormula::setVisible()
{
	psi->setVisible();
	visible = psi->visible;
}
