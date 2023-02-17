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

\brief Class for firelist generation for bound computation using deletion algorithm subsequently to up set based insertion algorithm
sets.
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornComputeBoundCombined.h>
#include <Net/NetState.h>
#include <Core/Runtime.h>
#include <Exploration/FirelistStubbornDeadlock.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>


#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif

/*!
 * \brief A constructor for firelists of stubborn sets using the deletion algorithm.
 *        Used for deadlock checks.
 */
FirelistStubbornComputeBoundCombined::FirelistStubbornComputeBoundCombined(Petrinet * n, StatePredicate * pp): Firelist(n),
    predicate(pp),
    dfsStack(new arrayindex_t[n->Card[TR]]),
    onStack(new bool[n->Card[TR]]),
    status(new int[n->Card[TR]+n->Card[PL]])
{
	net = n;
}

/*!
 * \brief Destructor.
 */
FirelistStubbornComputeBoundCombined::~FirelistStubbornComputeBoundCombined()
{
    delete[] dfsStack;
    delete[] onStack;
}


/*!
 * \brief The function to be called when a stubborn set at a given marking
 *        should be constructed
 * \param ns The marking for which the stubborn set should be built.
 * \param result A pointer to NULL. Will be replaced by a pointer to an array
 *               containing the stubborn set.
 * \return The number of element in the stubborn set.
 */
arrayindex_t FirelistStubbornComputeBoundCombined::getFirelist(NetState &ns, arrayindex_t **result)
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
    memset(onStack,0,net->Card[TR]*sizeof(bool));
    scapegoatNewRound();
    bool needEnabled = false;
    arrayindex_t stackpointer = reinterpret_cast<AtomicStatePredicate*>(predicate)->getDownSet(dfsStack, onStack, &needEnabled);
    arrayindex_t carddown = stackpointer;
    arrayindex_t cardEnabled = 0;

    // loop until all stack elements processed
    for (arrayindex_t firstunprocessed = 0; firstunprocessed < stackpointer; ++firstunprocessed)
    {
        arrayindex_t currenttransition = dfsStack[firstunprocessed];
        arrayindex_t *mustbeincluded;
        arrayindex_t  cardmustbeincluded;
        if (ns.Enabled[currenttransition])
        {
            ++cardEnabled;
            mustbeincluded = net->TrDecreased[currenttransition];
            cardmustbeincluded = net->TrCardDecreased[currenttransition];
        }

        else
        {
            arrayindex_t scapegoat = Firelist::selectScapegoat(ns,currenttransition);
            mustbeincluded = net->Arc[PL][PRE][scapegoat];
            cardmustbeincluded = net->CardArcs[PL][PRE][scapegoat];
        }
        for (arrayindex_t i = 0; i < cardmustbeincluded; ++i)
        {
            const arrayindex_t t = mustbeincluded[i];
            if (!onStack[t])
            {
                dfsStack[stackpointer++] = t;
                onStack[t] = true;
            }
        }
    }
    if(cardEnabled == 0)
    {
	// early exit
        * result = new arrayindex_t[1];
	return 0;
    }
    
    // STEP 4: Init graph structure for deletion algorithm

    // 4.1: read transitions from scc into status

    memset(status,0,sizeof(int) * (net->Card[PL]+net->Card[TR]));

    counter = 3;
    card_enabled = 0;
    for (arrayindex_t i = 0; i < stackpointer;i++)
    {
	   arrayindex_t t = dfsStack[i];
	   if(ns.Enabled[t]) card_enabled++;
	   status[t] = 2; // status IN
  	   onStack[t] = false;
    }
    

    // second round: scan through disabled transitions and set scapegoats.
    // At this point: all transitions in (insertion) stubborn set have 
    // status != 0.
		
    for (arrayindex_t i = 0; i < stackpointer;i++)
    {
	   arrayindex_t t = dfsStack[i];
	   if(ns.Enabled[t])
	   {
		   continue; // only interested in disabled trans.
	   }
	   for(arrayindex_t pi = 0; pi < net->CardArcs[TR][PRE][t];pi++)
	   {
		arrayindex_t p = net->Arc[TR][PRE][t][pi];
		
		// p is scapegoat candidate if not sufficiently marked 
	        // and all its pre-transitions are in insertin stubborn set

		if(ns.Current[p] >= net->Mult[TR][PRE][t][pi]) continue;
		if(status[p + net->Card[TR]])
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
    }

    // nail up set
    // exploit the fact that the down set is the first part of dfsStack.
    // carddown has been set initially, when getting the down set
    for(arrayindex_t i = 0; i < carddown;i++)
    {
	nail(ns,dfsStack[i]);
    }
    // STEP 5: run actual deletion procedure;

    // From this point, the following is invariant for status:
    // status = 0: node permanently removed (or never been in) stubborn set
    // status = 1: node is "nailed", that is, not to be removed any more
    // status = 2..(counter-1): node is in stubborn set but not nailed
    // status = counter: node is temporarily removed from stubborn set

    arrayindex_t candidate_transition = 0;
    while(true)
    {
	// search first enabled transition >= candidate.
	// idea: all enabled transitions left of candidate are either
 	// nailed or permanently removed.
	// temp-removed transitions of previous rounds are IN again
	counter++; 
	for( ; candidate_transition < net->Card[TR]; candidate_transition++)
	{
		if(status[candidate_transition] <= 1) continue;
		// start speculation only at enabled transition
		if(ns.Enabled[candidate_transition]) break; 
	}
	if(candidate_transition >= net->Card[TR])
	{
		// no further candidate for removal -->
		// return stubborn set
		* result = new arrayindex_t [card_enabled ? card_enabled : 1];
		arrayindex_t f = 0;
		for(arrayindex_t i = 0; i < net->Card[TR];i++)
		{
			if(status[i] == 0) continue;
			if(!ns.Enabled[i]) continue;
			(*result)[f++] = i;
		}
		return f;
	}
	temp_card_enabled = card_enabled;

	if(speculate(ns,candidate_transition)) 
	{
		// candidate_transition + consequences can be safely removed
		card_enabled = temp_card_enabled;
		if(card_enabled == 0)
		{
			// no further candidate for removal -->
			// return stubborn set
			* result = new arrayindex_t [1];
			return 0;
		}
		for(arrayindex_t k = 0; k < net->Card[TR] + net->Card[PL];k++)
		{
			// mark temp out as perm out
			if(status[k] == counter) status[k] = 0;
		}
	}
	else
	{
		nail(ns,candidate_transition); // let candidate_transition+consequences be NAILED (permanently IN)
	}
    }
}
void Aandorprintname(arrayindex_t);

bool FirelistStubbornComputeBoundCombined::speculate(NetState & ns,arrayindex_t node)
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
		// node is enabled transition
		status[node] = counter;
		// 2. propagate
		// - transitions decreased by t have t in their must-be-included
		for(arrayindex_t i = 0; i < net->TrCardDecreasing[node];i++)
		{
			
			arrayindex_t t = net->TrDecreasing[node][i];
			if(ns.Enabled[t] && !speculate(ns,t)) return false;
		}
		// - scapegoat places to which t has a positive impact, have t in their must-be-included
		for(arrayindex_t i = 0; i < net->CardDeltaT[POST][node];i++)
		{
			if(!speculate(ns,net->DeltaT[POST][node][i]+net->Card[TR])) return false;
		}
		return true;
	}
	else	
	{
		// node is scapegoat place
		// 1. set node to tmp_out
		arrayindex_t p = node  - net->Card[TR];
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

void FirelistStubbornComputeBoundCombined::nail(NetState & ns,arrayindex_t node)
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
                                if(status[p + net->Card[TR]] == 0) continue;  // not in
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

Firelist *FirelistStubbornComputeBoundCombined::createNewFireList(Petrinet * n, StatePredicate * p)
{
    return new FirelistStubbornComputeBoundCombined(n,p);
}


