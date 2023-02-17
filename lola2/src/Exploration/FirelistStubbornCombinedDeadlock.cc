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

\brief Class for firelist generation by the deletion algorithm for stubborn
sets.
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornCombinedDeadlock.h>
#include <Net/NetState.h>
#include <Core/Runtime.h>

#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif

/*!
 * \brief A constructor for firelists of stubborn sets using the deletion algorithm.
 *        Used for deadlock checks.
 */
FirelistStubbornCombinedDeadlock::FirelistStubbornCombinedDeadlock(Petrinet * n): Firelist(n),
    dfsStack(new arrayindex_t[n->Card[TR]]),
    dfs(new arrayindex_t[n->Card[TR]]),
    lowlink(new arrayindex_t[n->Card[TR]]),
    currentIndex(new arrayindex_t[n->Card[TR]]),
    TarjanStack(new arrayindex_t[n->Card[TR]]),
    mustBeIncluded(new arrayindex_t *[n->Card[TR]]),
    visited(new bool[n->Card[TR] ]()),
    onTarjanStack(new bool[n->Card[TR] ]()),
    status(new int[n->Card[TR]+n->Card[PL]] )
{
	net = n;
}

/*!
 * \brief Destructor.
 */
FirelistStubbornCombinedDeadlock::~FirelistStubbornCombinedDeadlock()
{
    delete[] dfsStack;
    delete[] dfs;
    delete[] lowlink;
    delete[] currentIndex;
    delete[] TarjanStack;
    delete[] mustBeIncluded;
    delete[] visited;
    delete[] onTarjanStack;
    delete[] status;
}

/*!
 * \brief The function to be called when a stubborn set at a given marking
 *        should be constructed
 * \param ns The marking for which the stubborn set should be built.
 * \param result A pointer to NULL. Will be replaced by a pointer to an array
 *               containing the stubborn set.
 * \return The number of element in the stubborn set.
 */
arrayindex_t FirelistStubbornCombinedDeadlock::getFirelist(NetState &ns, arrayindex_t **result)
{
    // Step 1: Take care of case where no transition is enabled
    // This branch is here only for the case that exploration continues
    // after having found a deadlock. In current LoLA, it cannot happen
    // since check property will raise its flag before firelist is
    // requested
    // LCOV_EXCL_START
    if (UNLIKELY(ns.CardEnabled == 0))
    {
        assert(false);
        * result = new arrayindex_t[1];
        return 0;
    }
    // LCOV_EXCL_STOP

    // Step 2: find 1st enabled transition and return immediately 
    // singleton stubborn set if found transition
    // is the only transition in its conflict cluster
    // The list ensures that transitions from small conflict clusters
    // get priority over transitions in large conflict clusters.
    // Relevant preprocessing has taken place in DeadlockTask::buildTask().

    int firstenabled;
    for(firstenabled = 0; firstenabled < net->Card[TR]; firstenabled++)
    {
        if(ns.Enabled[net->StubbornPriority[firstenabled]])
        {
                break;
        }
    }
    // If start transition is alone in its conflict cluster, we
    // can immediately return it as singleton stubborn set.

	    if(firstenabled < net->SingletonClusters)
	    {
		* result = new arrayindex_t[1];
		** result = net->StubbornPriority[firstenabled];
        return 1;
    }

    // STEP 3: Now we execute the actual insertion agorithm in full
    // length. Its result will serve as initial situation for the 
    // subsequent deletion algorithm

    // switch from index in priority list to transition name
    firstenabled = net->StubbornPriority[firstenabled];

    // initialize DFS for stubborn closure
    arrayindex_t nextDfs = 1;
    arrayindex_t stackpointer = 0;
    arrayindex_t tarjanstackpointer = 0;
    arrayindex_t dfsLastEnabled;
    memset(onTarjanStack,0,net->Card[TR] * sizeof(bool));
    memset(visited,0,net->Card[TR] * sizeof(bool));

    // A deadlock preserving stubborn set is computed by depth first search
    // in a graph. Nodes are transitions.
    // The root of the graph is an arbitrary enabled transition. The edges
    // form a "must be included" relation where, for an enabled transition,
    // its conflicting transitions must be included, and for a disabled 
    // transition, all pre-transitions of an arbitrary insufficiently marked
    // place, called scapegoat, must be included. The resulting stubborn set
    // consists of all enabed transitions in the first encountered SCC of
    // the graph that contains enabled transitions (i.e. a set of transitions
    // that is closed under "must be included" and has at least one enabled
    // tansition).


    scapegoatNewRound();

    // For detecting SCC, we perform depth-first search
    // with the Tarjan extension
    dfs[firstenabled] = lowlink[firstenabled] = 0;
    dfsStack[0] = TarjanStack[0] = firstenabled;

    // a fresh value of stamp signals that a transition has been visited
    // in this issue of dfs search. Using stamps, we do not need to reset
    // "visited" flags
    visited[firstenabled] = onTarjanStack[firstenabled] = true;
    mustBeIncluded[0] = net->TrDecreased[firstenabled];

    // we record the largest dfs of an encountered enabled transition.
    // This way, we can easily check whether an SCC contains enabled
    // transitions.

    dfsLastEnabled = 0;

    // CurrentIndex controls the loop over all edges leaving the current
    // transition. By starting from the largest value and counting
    // backwards, we do not need to memorize the overall number of edges.
    // For making sure that we have key transitions, we include the whole conflicting set of firstenabled.

    currentIndex[0] = net->TrCardDecreasing[firstenabled] 
		      + (net->TrDecreasing[firstenabled]-net->TrDecreased[firstenabled]);

    arrayindex_t currenttransition;

    // depth first search
    // quit when scc with enabled transitions is found.
    while (true) // this loop is exited by break statements
    {
        // consider the transition on top of the stack
        currenttransition = dfsStack[stackpointer];
        if ((currentIndex[stackpointer]) > 0)
        {
            // current transition has another successor: newtransition
            arrayindex_t newtransition = mustBeIncluded[stackpointer][--(currentIndex[stackpointer])];
            if (visited[newtransition])
            {
                // transition already seen
                // update lowlink of currenttransition and stay at currenttransition

                if (onTarjanStack[newtransition] && dfs[newtransition] < lowlink[currenttransition])
                {
                    lowlink[currenttransition] = dfs[newtransition];
                }
            }
            else
            {
                // transition not yet seen: proceed to newtransition
                dfs[newtransition] = lowlink[newtransition] = nextDfs++;
                visited[newtransition] = onTarjanStack[newtransition] = true;
                dfsStack[++stackpointer] = newtransition;
                TarjanStack[++tarjanstackpointer] = newtransition;
                if (ns.Enabled[newtransition])
                {
                    // must include conflicting transitions
                    mustBeIncluded[stackpointer] = net->TrDecreased[newtransition];
                    currentIndex[stackpointer] = net->TrCardDecreasing[newtransition] 
                    + (net->TrDecreasing[newtransition]-net->TrDecreased[newtransition]);
                    dfsLastEnabled = nextDfs - 1;

                }
                else
                {
                    // must include pretransitions of scapegoat

                    arrayindex_t scapegoat = Firelist::selectScapegoat(ns,newtransition);
                    mustBeIncluded[stackpointer] = net->PlIncreasing[scapegoat];
                    currentIndex[stackpointer] = net->PlCardIncreasing[scapegoat];
                }
            }
        }
        else
        {
            // current transition does not have another successor

            // check for closed scc
            if (dfs[currenttransition] == lowlink[currenttransition])
            {
                // scc closed
                // check whether scc contains enabled transitions
                if (dfsLastEnabled >= dfs[currenttransition])
                {
			// proceed with deletion agorithm
			break;
                }
                else
                {
                    // no enabled transitions
                    // pop current scc from tarjanstack and continue
		    arrayindex_t poppedtransition;
                    while (currenttransition != (poppedtransition = TarjanStack[tarjanstackpointer--]))
                    {
			onTarjanStack[poppedtransition] = false;
                    }
                    assert(stackpointer > 0);
                    --stackpointer;
		    onTarjanStack[currenttransition] = false;
                }
            }
            else
            {
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
    
    // STEP 4: Init graph structure for deletion algorithm

    // 4.1: read transitions from scc into status

     memset(status,0,(net->Card[TR]+net->Card[PL])*sizeof(int));

    card_enabled = 0;
    for (arrayindex_t i = tarjanstackpointer; ;i--)
    {
	   arrayindex_t t = TarjanStack[i];
	   status[t] = 2; // status IN
	   if(ns.Enabled[t]) 
	   {
		card_enabled++;
	   }
	   if(TarjanStack[i] == currenttransition) break;
    }
    if(card_enabled == 1)
    {
	// nothing to do for deletion algorithm
        * result = new arrayindex_t[card_enabled];
	arrayindex_t rindex = 0;
        for (arrayindex_t i = tarjanstackpointer; TarjanStack[i] != currenttransition;i--)
	{
		arrayindex_t t = TarjanStack[i];
		if(ns.Enabled[t]) 
		{
			(*result)[0] = t;
			return 1;
		}
	}
	// arriving here menas that currentransition is the only enabled one
	(*result)[0] = currenttransition;
	return 1;
    }

    // second round: scan through disabled transitions and set scapegoats.
    // At this point: all transitions in (insertion) stubborn set have 
    // status != 0.
		
    for (arrayindex_t i = tarjanstackpointer; ;i--)
    {
	   arrayindex_t t = TarjanStack[i];
	   if(ns.Enabled[t])
	   {
		   if(t == currenttransition) break;
		   continue; // only interested in disabled trans.
           
	   }
	   // now: t is a disabled transition
	   for(arrayindex_t pi = 0; pi < net->CardArcs[TR][PRE][t];pi++)
	   {
		arrayindex_t p = net->Arc[TR][PRE][t][pi];
		
		// p is scapegoat candidate if not sufficiently marked 
	        // and all its increasing transitions are in insertion stubborn set

		if(ns.Current[p] >= net->Mult[TR][PRE][t][pi]) continue; // sufficiently marked --> no scapegoat
		if(status[p + net->Card[TR]]) // already present
		{
			continue; 
		} 
		bool candidate = true;
		for(arrayindex_t tti = 0; tti < net->PlCardIncreasing[p]; tti++)
		{
			arrayindex_t ttt = net->PlIncreasing[p][tti];
			if(status[ttt] == 0)
			{
				candidate = false;
				break;
			}
			
		}
		if(candidate)
		{
			status[p + net->Card[TR]] = 2;
		}
	   }
	   if(t == currenttransition) break;
    }

    // STEP 5: run actual deletion procedure;

    // From this point, the following is invariant for status:
    // status = 0: node permanently removed (or never been in) stubborn set
    // status = 1: node is "nailed", that is, not to be removed any more
    // status = 2..(counter-1): node is in stubborn set but not nailed
    // status = counter: node is temporarily removed from stubborn set

    counter = 3;
    arrayindex_t candidate_transition = 0;
    while(true)
    {
	// temp-removed transitions of previous rounds are IN again
	counter++; 
	// search first enabled transition >= candidate.
	// idea: all enabled transitions left of candidate are either
 	// nailed or permanently removed.
	for( ; candidate_transition < net->Card[TR]; candidate_transition++)
	{
		// start transition should be IN
		if(status[candidate_transition] <= 1) continue;
		// start speculation only at enabled transition
		if(ns.Enabled[candidate_transition]) break; 
	}
	if(candidate_transition >= net->Card[TR])
	{
		// no further candidate for removal -->
		// return stubborn set
		* result = new arrayindex_t [card_enabled];
		arrayindex_t f = 0;
		for(arrayindex_t i = 0; i < net->Card[TR];i++)
		{
			if(status[i] == 0) continue;
			if(!ns.Enabled[i]) continue;
			(*result)[f++] = i;
		}
		assert(f == card_enabled);
		return card_enabled;
	}
	temp_card_enabled = card_enabled;
	if(speculate(ns,candidate_transition)) 
	{
		arrayindex_t a_key_transition = net->Card[TR];
		// check whether there is still a key transition
		for(arrayindex_t t = 0; t < net->Card[TR]; t++)
		{
			if(!ns.Enabled[t]) continue;
			if(status[t] == 0 || status[t] == counter) continue; // perm out or temp out
			arrayindex_t j ;
			for(j = 0; j < net->TrCardDecreasing[t]; j++)
			{
				int stat = status[net->TrDecreasing[t][j]];
				if(stat < 1 || stat == counter) break;
			}
			if(j < net->TrCardDecreasing[t]) continue; // not a key transition
			a_key_transition = t;
			break;
		}
		if(a_key_transition < net->Card[TR])
		{
			// candidate_transition + consequences can be safely removed
			card_enabled = temp_card_enabled;
			if(card_enabled == 1)
			{
				// no further candidate for removal -->
				// return stubborn set
				* result = new arrayindex_t [card_enabled];
				for(arrayindex_t i = 0; i < net->Card[TR];i++)
				{
					if(status[i] == 0) continue;
					if(!ns.Enabled[i]) continue;
					if(status[i] == counter) continue;
					(*result)[0] = i;
					return 1;
				}
				assert(false);
			}
			for(arrayindex_t k = 0; k < net->Card[TR] + net->Card[PL];k++)
			{
				// mark temp out as perm out
				if(status[k] == counter) status[k] = 0;
			}
			continue;
		}
		nail(ns,candidate_transition);
	}
	else
	{
		nail(ns,candidate_transition); // let candidate_transition+consequences be NAILED (permanently IN)
	}
    }
}

void andorprintname(arrayindex_t);
bool FirelistStubbornCombinedDeadlock::speculate(NetState & ns,arrayindex_t node)
{
	if(status[node] == 1) return false; // trying to remove nailed node
	if(status[node] == 0) return true; // trying to remove absent node
	if(status[node] == counter) return true; // trying to remove removed node
	if(node < net->Card[TR])
	{
		// node is transition
		if(ns.Enabled[node])
		{
			if(--temp_card_enabled == 0) 
			{
				// trying to remove the last enabled transtions
				return false;
			}
		}
		// 1. set node to tmp_out
		status[node] = counter;
		// 2. propagate:
		// - enabled transitions decreased by node have node in their must-be-included
		for(arrayindex_t i = 0; i < net->TrCardDecreasing[node];i++)
		{
			arrayindex_t t = net->TrDecreasing[node][i];
			if(ns.Enabled[t])
			{
				if(!speculate(ns,t)) return false;
			}
		}
		// - scapegoat places to which node has a positive impact, have node in their must-be-included
		for(arrayindex_t i = 0; i < net->CardDeltaT[POST][node];i++)
		{
			if(!speculate(ns,net->DeltaT[POST][node][i]+net->Card[TR])) return false;
		}
		return true;
	}
	else	
	{
		// Node is scapegoat place
		arrayindex_t p = node - net->Card[TR];
		// 1. set node to tmp_out
		status[node] = counter; 
		// disabled post-transitions may have this in their must-be-included
		for(arrayindex_t j = 0; j < net->CardArcs[PL][POST][p]; j++)
		{
			if(ns.Current[p] >= net->Mult[PL][POST][p][j]) continue; // p is no scapegoat for t
			arrayindex_t t = net->Arc[PL][POST][p][j];
			if(status[t] == 0) continue; // do not need to care about absent transition
			if(status[t] == counter) continue; // do not need to card about temp removed transition

			// here: t is in (temp or nailed)
			// ...check whether p is last scapegoat for t
			bool hasscapegoat = false;
			for(arrayindex_t jj = 0; jj< net->CardArcs[TR][PRE][t];jj++)
			{
				arrayindex_t pp = net->Arc[TR][PRE][t][jj];
				if((status[pp + net->Card[TR]] == 0) || (status[pp + net->Card[TR]] == counter)) continue; // not in
				if(ns.Current[pp] >= net->Mult[TR][PRE][t][jj]) continue; // not scapegoat
				hasscapegoat = true;
				break;
			}
			if(!hasscapegoat) // removing last sc
			{
				if(!speculate(ns,t)) return false;
			}
		}
	}
	return true;
}

void FirelistStubbornCombinedDeadlock::nail(NetState & ns,arrayindex_t node)
{
	// mark nodes a "permanently in" after failed speculation
	// propagate this via Post

	assert(status[node] != 0);

	if(status[node] == 1) return; // already nailed
	if(status[node] == 0) return; // not in our graph
	status[node] = 1;

	if(node < net->Card[TR])
	{
		// node is transition
		if(ns.Enabled[node])
		{
			// node is enabled transition
			for(arrayindex_t k = 0; k < net->TrCardDecreased[node];k++)
			{
				nail(ns,net->TrDecreased[node][k]);
			}
		}
		else
		{
			// node is disabled transition
			
			// check if node has only one scapegoat
			arrayindex_t cardsc = 0;
			arrayindex_t sc;
			
			for(arrayindex_t l = 0; l < net->CardArcs[TR][PRE][node];l++)
			{
				arrayindex_t p = net->Arc[TR][PRE][node][l];
				if(status[p+net->Card[TR]] == 0) continue;  // not in 
				if(ns.Current[p] >= net->Mult[TR][PRE][node][l]) continue; // no scapegoat
				cardsc++;
				sc = p;
			}
			assert(cardsc > 0);
			if(cardsc == 1)
			{
				nail(ns,sc + net->Card[TR]);
			}
			
		}
	}
	else
	{
		// node is place
		arrayindex_t p = node - net->Card[TR];
		for(arrayindex_t m = 0; m < net->PlCardIncreasing[p];m++)
		{
			nail(ns,net->PlIncreasing[p][m]);
		}
		
	}
}

Firelist *FirelistStubbornCombinedDeadlock::createNewFireList(Petrinet * n, SimpleProperty *)
{
    return new FirelistStubbornCombinedDeadlock(n);
}

