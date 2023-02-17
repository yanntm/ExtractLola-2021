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

\brief Class for firelist generation. LTL preserving, traditional, Tarjan based.
*/

#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Net/NetState.h>

FirelistStubbornLTLTarjan::FirelistStubbornLTLTarjan(Petrinet * n) : Firelist(n),
    dfsStack(new arrayindex_t[n->Card[TR]]),
    dfs(new arrayindex_t[n->Card[TR]]),
    lowlink(new arrayindex_t[n->Card[TR]]),
    currentIndex(new arrayindex_t[n->Card[TR]]),
    TarjanStack(new arrayindex_t[n->Card[TR]]),
    mustBeIncluded(new arrayindex_t *[n->Card[TR]]),
    visited(new int[n->Card[TR] ]()),
    onTarjanStack(new int[n->Card[TR] ]()),
    tried(new bool [n->Card[TR]]),
    stamp(0)
{net = n;}

FirelistStubbornLTLTarjan::~FirelistStubbornLTLTarjan()
{
    delete[] dfsStack;
    delete[] dfs;
    delete[] lowlink;
    delete[] currentIndex;
    delete[] TarjanStack;
    delete[] mustBeIncluded;
    delete[] visited;
    delete[] onTarjanStack;
    delete[] tried;
}

void FirelistStubbornLTLTarjan::newStamp()
{
    // 0xFFFFFFFF = max int
    if (UNLIKELY(++stamp == 1 << 30))
    {
        // This happens rarely and only in long runs. Thus it is
        // hard to be tested
        // LCOV_EXCL_START
        for (arrayindex_t i = 0; i < net->Card[TR]; ++i)
        {
            visited[i] = 0;
            onTarjanStack[i] = 0;
        }
        stamp = 1;
        // LCOV_EXCL_STOP
    }
}

arrayindex_t FirelistStubbornLTLTarjan::getFirelist(NetState &ns, arrayindex_t **result)
{
//RT::rep->status("------------ NEW STUBBORN SET --------------");
    // STEP 1: take care of no transition enabled
    // This branch is here only for the case that exploration continues
    // after having found a deadlock. In current LoLA, it cannot happen
    // since check property will raise its flag before firelist is
    // requested
    // LCOV_EXCL_START
    if (UNLIKELY(ns.CardEnabled == 0))
    {
//RT::rep->status("deadlock -> empty stubborn set");
        assert(false);
        * result = new arrayindex_t[1];
        return 0;
    }
    // LCOV_EXCL_STOP

    // STEP 2: find 1st enabled and invisible transition according to 
    // priority list.
    // The list ensures that transitions from small condlict clusters
    // get priority over transitions in large conflict clusters.
    // Relevant preprocessing has taken place in buildTask().
   
    int firstenabled;
    for(firstenabled = 0; firstenabled < net->Card[TR]; firstenabled++)
    {
	arrayindex_t t = net->StubbornPriority[firstenabled];
	if(visible[t]) continue;
	if(ns.Enabled[t])
	{
		break;
	}
    }


    // STEP 3a: If no invisible transition is enabled, return all
    // transitions

    if(firstenabled >= net->Card[TR])
    {
//RT::rep->status("no invisible enabled -> stub = T");
	* result = new arrayindex_t [ns.CardEnabled];
	arrayindex_t rindex = 0;
	for(arrayindex_t i = 0; i < net->Card[TR];i++)
	{
		if(ns.Enabled[i])
		{
			(*result)[rindex++] = i;
      		}
	}
	return ns.CardEnabled;
    }

    // STEP 3b: If start transition is alone in its conflict cluster, we
    // can immediately return it as singleton stubborn set.
 
    if(firstenabled < net->SingletonClusters)
    {
//RT::rep->status("invisible in singleton cluster: %s", net->Name[TR][firstenabled]);
	* result = new arrayindex_t[1];
	** result = net->StubbornPriority[firstenabled];
	return 1;
    }
    // during dfs search, we will record all visited enabled
    // transitions as "tried". This way, we will not attempt
    // to use them as start transition in subsequent rounds
    for(arrayindex_t i = firstenabled; i < net->Card[TR];i++)
    {
	tried[net->StubbornPriority[i]] = false;
    }



    // try all possible start transitions...
    while(true)
    {
	// firstenabled is just an index in the StubbornPriority list
	// StubbornPriority lists all transition, sorted by size of their conflict clusters
	// --> early transitions likely to form small stubborn sets.
    arrayindex_t first = net->StubbornPriority[firstenabled];
//RT::rep->status("Try %s as start transition.",net->Name[TR][first]);
    // initialize DFS for stubborn closure
    arrayindex_t nextDfs = 1;
    arrayindex_t stackpointer = 0;
    arrayindex_t tarjanstackpointer = 0;
    arrayindex_t dfsLastEnabled;
    newStamp();

    // A deadlock preserving stubborn set is computed by depth first search
    // in a graph. Nodes are transitions.
    // The root of the graph is an arbitrary enabled transition. The edges
    // form a "must be included relation" where, for an enabled transition,
    // its conflicting transitions must be included, and for a disabled i
    // transition, all pre-transitions of an arbitrary insufficiently marked
    // place, called scapegoat, must be included. The resulting stubborn set
    // consists of all enabed transitions in the first encountered SCC of
    // the graph that contains enabled transitions (i.e. a set of transitions
    // that is closed under "must be included" and has at least one enabled
    // tansition).

    scapegoatNewRound();

    // For detecting SCC, we perform depth-first search
    // with the Tarjan extension
    dfs[first] = lowlink[first] = 0;
    dfsStack[0] = TarjanStack[0] = first;

    // a fresh value of stamp signals that a transition has been visited
    // in this issue of dfs search. Using stamps, we do not need to reset
    // "visited" flags
    visited[first] = onTarjanStack[first] = stamp;
    mustBeIncluded[0] = net->TrDecreased[first];

    // we record the largest dfs of an encountered enabled transition.
    // This way, we can easily check whether an SCC contains enabled
    // transitions.
    dfsLastEnabled = 0;

    // CurrentIndex controls the loop over all edges leaving the current
    // transition. By starting from the largest value and counting
    // backwards, we do not need to memorize the overall number of edges.
    currentIndex[0] = net->TrCardDecreasing[first] + (net->TrDecreasing[first] - net->TrDecreased[first]);

    arrayindex_t currenttransition;

    // depth first search
    // quit when scc with enabled transitions is found.
    while (true) // this loop is exited by break statements
    {
        // consider the transition on top of the stack
        currenttransition = dfsStack[stackpointer];
//RT::rep->status("consider %s",net->Name[TR][currenttransition]);
        if ((currentIndex[stackpointer]) > 0)
        {
            // current transition has another successor: newtransition
            arrayindex_t newtransition = mustBeIncluded[stackpointer][--(currentIndex[stackpointer])];
//RT::rep->status("has successor %s",net->Name[TR][newtransition]);
            if (visited[newtransition] == stamp)
            {
//RT::rep->status("already seen -> backtrack");
                // transition already seen
                // update lowlink of currenttransition: and stay at
                // currenttransition
                if (onTarjanStack[newtransition] == stamp && dfs[newtransition] < lowlink[currenttransition])
                {
                    lowlink[currenttransition] = dfs[newtransition];
                }
            }
            else
            {
//RT::rep->status("Not yet seen.");
                // transition not yet seen: proceed to newtransition
                dfs[newtransition] = lowlink[newtransition] = nextDfs++;
                visited[newtransition] = onTarjanStack[newtransition] = stamp;
                dfsStack[++stackpointer] = newtransition;
                TarjanStack[++tarjanstackpointer] = newtransition;
                if (ns.Enabled[newtransition])
                {
			// we do not need to try this as
			// start transition any more
  		 	tried[newtransition] = true; 
//RT::rep->status("Reject %s as start transition",net->Name[TR][newtransition]);

			// no stubborn set
			// can include visible transition
			// --> try other start transition
			if(visible[newtransition]) 
			{
//RT::rep->status("visible transition --> proceed to next start transition");
				break; 
			}

                    // must include conflicting transitions
                    mustBeIncluded[stackpointer] = net->TrDecreased[newtransition];
                    currentIndex[stackpointer] = net->TrCardDecreasing[newtransition] + (net->TrDecreasing[newtransition] - net->TrDecreased[newtransition]);;
                    dfsLastEnabled = nextDfs - 1;
                }
                else
                {

		    arrayindex_t scapegoat = Firelist::selectScapegoat(ns,newtransition);
                    // must include pretransitions of scapegoat
                    mustBeIncluded[stackpointer] = net->PlIncreasing[scapegoat];
                    currentIndex[stackpointer] = net->PlCardIncreasing[scapegoat];
                }
            }
        }
        else
        {
            // current transition does not have another successor
//RT::rep->status("close transition");
            // check for closed scc
            if (dfs[currenttransition] == lowlink[currenttransition])
            {
//RT::rep->status("scc closed");
                // scc closed
                // check whether scc contains enabled transitions
                if (dfsLastEnabled >= dfs[currenttransition])
                {
//RT::rep->status("scc contains enabled --> gen firelist");
                    // build firelist from current scc,
                    // pop all other scc for resetting data structures

                    arrayindex_t CardStubborn = 0;
                    for (arrayindex_t i = tarjanstackpointer; TarjanStack[i] != currenttransition;)
                    {
                        if (ns.Enabled[TarjanStack[i--]])
                        {
                            ++CardStubborn;
                        }
                    }
                    if (ns.Enabled[currenttransition])
                    {
                        ++CardStubborn;
                    }
                    assert(CardStubborn > 0);
                    assert(CardStubborn <= ns.CardEnabled);
                    * result = new arrayindex_t [CardStubborn];
                    arrayindex_t resultindex = CardStubborn;
                    while (currenttransition != TarjanStack[tarjanstackpointer])
                    {
                        arrayindex_t poppedTransition = TarjanStack[tarjanstackpointer--];
                        if (ns.Enabled[poppedTransition])
                        {
//RT::rep->status("add %s",net->Name[TR][poppedTransition]);
                            (*result)[--resultindex] = poppedTransition;
                        }
                    }
                    if (ns.Enabled[currenttransition])
                    {
//RT::rep->status("add %s",net->Name[TR][currenttransition]);
                        (*result)[--resultindex] = currenttransition;
                    }
                    assert(resultindex == 0);
                    return (CardStubborn);
                }
                else
                {
//RT::rep->status("no enabled --> pop scc and continue");
                    // no enabled transitions
                    // pop current scc from tarjanstack and continue
                    while (currenttransition != TarjanStack[tarjanstackpointer--])
                    {
                    }
                    assert(stackpointer > 0);
                    --stackpointer;

                    // In this case: update of lowlink is not necessary:
                    // We have lowlink[currenttransition] == dfs[currenttransition]
                    // dfsStack[stackpointer] is parent of currenttransition
                    // Hence, it has smaller dfs than currenttransition
                    // Hence, it has smaller lowlink anyway.
                    assert(lowlink[currenttransition] >= lowlink[dfsStack[stackpointer]]);
                    //                    if (lowlink[currenttransition] < lowlink[dfsStack[stackpointer]])
                    //                    {
                    //                        lowlink[dfsStack[stackpointer]] = lowlink[currenttransition];
                    //                    }
                }
            }
            else
            {
//RT::rep->status("no scc closed -> backtrack");
                // scc not closed
                assert(stackpointer > 0);
                --stackpointer;
                if (lowlink[currenttransition] < lowlink[dfsStack[stackpointer]])
                {
                    lowlink[dfsStack[stackpointer]] = lowlink[currenttransition];
                }
            }
        }
    }
    // switch to next start transition
    for(firstenabled++;firstenabled<net->Card[TR];firstenabled++)
    {
	arrayindex_t t = net->StubbornPriority[firstenabled];
	if(visible[t]) continue;
	if(tried[t]) continue;
	if(ns.Enabled[t])
	{
		break;
	}
    }
    if(firstenabled >= net->Card[TR])
    {
	* result = new arrayindex_t [ns.CardEnabled];
	arrayindex_t rindex = 0;
	for(arrayindex_t i = 0; i < net->Card[TR];i++)
	{
		if(ns.Enabled[i])
		{
			(*result)[rindex++] = i;
      		}
	}
	return ns.CardEnabled;
    }
    // try to find stubborn set with new start transition
    }   
}

Firelist *FirelistStubbornLTLTarjan::createNewFireList(Petrinet * n)
{
    return new FirelistStubbornLTLTarjan(n);
}
