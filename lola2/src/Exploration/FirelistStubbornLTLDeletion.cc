/* This file is part of LoLA.

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
#include <Exploration/FirelistStubbornLTLDeletion.h>
#include <Net/NetState.h>
#include <Core/Runtime.h>

#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif


/*!
 * \brief A constructor for firelists of stubborn sets using the deletion algorithm.
 *        Used for deadlock checks.
 */
FirelistStubbornLTLDeletion::FirelistStubbornLTLDeletion(Petrinet * n): Firelist(n),
    status(new int[n->Card[TR]+n->Card[PL]] )
{
	net = n;
}

/*!
 * \brief Destructor.
 */
FirelistStubbornLTLDeletion::~FirelistStubbornLTLDeletion()
{
    delete[] status;
}


arrayindex_t FirelistStubbornLTLDeletion::getFirelist(NetState &ns, arrayindex_t **result)
{
	// STEP 1: Take care of case where no or just one transition is enabled
	if (UNLIKELY(ns.CardEnabled == 0))
    	{
		* result = new arrayindex_t[1];
		return 0;
	}
	if (UNLIKELY(ns.CardEnabled == 1))
    	{
		* result = new arrayindex_t[1];
		for(arrayindex_t i = 0; i < net->Card[TR];i++)
		{
			if(ns.Enabled[i]) 
			{
				*result[0] = i;
				return 1;
			}
		}
		assert(false); // should have found enabled transition
	}
	// STEP 2: If enabled invisible transition exists in 
	// singleton conflict cluster, this is our preferred choice
        int firstenabled;
        for(firstenabled = 0; firstenabled < net->Card[TR]; firstenabled++)
        {
            if(ns.Enabled[net->StubbornPriority[firstenabled]] &&
               !visible[net->StubbornPriority[firstenabled]])
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
	// STEP 3: try to find stubborn set with only invisible transitions
	// using deletion algorithm

	// init status record
	// during init:
	// 2: in
	// 1: out, unprocessed
	// 0: out, processed

	int enabled_in = 0;
	int card_unprocessed = 0;
	for(arrayindex_t t = 0; t < net->Card[TR];t++)
	{
		if(visible[t])
		{
			status[t] = 1;
			card_unprocessed++;
		}
		else
		{
			status[t] = 2;
			if(ns.Enabled[t]) 
			{
				enabled_in++;
			}
		}
	}
	for(arrayindex_t p = 0; p < net->Card[PL];p++)
	{
		status[net->Card[TR]+p] = 2;
	}
	// remove inconsistencies
	while(card_unprocessed)
	{
		// check transitions
		for(arrayindex_t t = 0; t < net->Card[TR];t++)	
		{
			if(status[t] != 1) continue;
			status[t] = 0;
			card_unprocessed--;
			for(arrayindex_t i = 0; i < net->TrCardDecreased[t];i++)
			{
				// enabled transitions decreased by t
				// have t in their must-be-included
				arrayindex_t tt = net->TrDecreased[t][i];
				if(ns.Enabled[tt] && status[tt] == 2)
				{
					status[tt] = 1;
					card_unprocessed++;
					if(!--enabled_in) break;
				}
			}
			for(arrayindex_t i = 0; i < net->CardDeltaT[POST][t];i++)
			{
				// scapegoats increased by t have t in their
				// must-be-included
				arrayindex_t p = net->DeltaT[POST][t][i];
				if(status[p+net->Card[TR]] == 2)
				{
					status[p+net->Card[TR]] = 1;
					card_unprocessed++;
				}
			}
		}
		if(!enabled_in) break;
		// check places
		for(arrayindex_t p = 0; p < net->Card[PL];p++)
		{
			arrayindex_t pindex = p + net->Card[TR];
			if(status[pindex] != 1) continue;
			status[pindex] = 0;
			card_unprocessed--;
			for(arrayindex_t i = 0; i < net->CardArcs[PL][POST][p];i++)
			{
				// disabled post-transitions may have p in their
				// must-be-included
				if(ns.Current[p] >= net->Mult[PL][POST][p][i]) continue; // p no scapegoat 
				arrayindex_t t = net->Arc[PL][POST][p][i];
				if(status[t] != 2) continue;
				bool hasscapegoat = false;
                        	for(arrayindex_t jj = 0; jj< net->CardArcs[TR][PRE][t];jj++)
                        	{
                                	arrayindex_t pp = net->Arc[TR][PRE][t][jj];
					if(status[pp + net->Card[TR]] < 2) continue; // not in
					if(ns.Current[pp] >= net->Mult[TR][PRE][t][jj]) continue; // not scapegoat
					hasscapegoat = true;
					break;
				}
				if(!hasscapegoat) // removing last sc
				{
					if(status[t] == 2) status[t] = 1;
					card_unprocessed++;
					if(ns.Enabled[t]) 
					{
						if(!--enabled_in) break;
					}
				}
			}
		}
	}
	// check for presence of key transition
       arrayindex_t a_key_transition = net->Card[TR];
	for(arrayindex_t t = 0; t < net->Card[TR]; t++)
	{
		if(!ns.Enabled[t]) continue;
		if(status[t] < 2) continue; // out
		arrayindex_t j ;
		arrayindex_t cardOnlyDecreased = net->TrDecreasing[t] - net->TrDecreased[t];
		for(j = 0; j < cardOnlyDecreased; j++)
		{
			int stat = status[net->TrDecreased[t][j]];
			if(stat < 1 || stat == counter) break;
		}
		if(j < cardOnlyDecreased) continue; // not a key transition
		a_key_transition = t;
		break;  
	}
	if(a_key_transition >= net->Card[TR]) enabled_in = 0; // cannot use this stubborn set -> proceed to STEP 4

	if(enabled_in == 1)
	{
            * result = new arrayindex_t[1];
            ** result = net->StubbornPriority[firstenabled];
            return 1;
	}
	if(enabled_in)
	{
	    // apply actual deletion on invisible
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
			* result = new arrayindex_t [enabled_in];
			arrayindex_t f = 0;
			for(arrayindex_t i = 0; i < net->Card[TR];i++)
			{
				if(status[i] == 0) continue;
				if(!ns.Enabled[i]) continue;
				(*result)[f++] = i;
			}
			assert(f == enabled_in);
			return enabled_in;
		}
		temp_card_enabled = enabled_in;
		if(speculate(ns,candidate_transition))
		{
			arrayindex_t a_key_transition = net->Card[TR];
			// check whether there is still a key transition
			for(arrayindex_t t = 0; t < net->Card[TR]; t++)
			{
				if(!ns.Enabled[t]) continue;
				if(status[t] == 0 || status[t] == counter) continue; // perm out or temp out
				arrayindex_t j ;
				arrayindex_t cardOnlyDecreased = net->TrDecreasing[t] - net->TrDecreased[t];
				for(j = 0; j < cardOnlyDecreased; j++)
				{
					int stat = status[net->TrDecreased[t][j]];
					if(stat < 1 || stat == counter) break;
				}
				if(j < cardOnlyDecreased) continue; // not a key transition
				a_key_transition = t;
				break;
			}
			if(a_key_transition < net->Card[TR])
			{

				// candidate_transition + consequences can be safely removed
				enabled_in = temp_card_enabled;
				if(enabled_in == 1)
				{
					// no further candidate for removal -->
					// return stubborn set
					* result = new arrayindex_t [enabled_in];
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
	// STEP 4: apply deletion algorithm to whole T and nail visible
	for(arrayindex_t i = 0; i < net->Card[TR];i++)
	{
		status[i] = 2;
	}
	for(arrayindex_t i = 0; i < net->Card[PL];i++)
	{
		status[i+ net->Card[TR]] = 2;
	}
	for(arrayindex_t i = 0; i < net->Card[PL];i++)
	{
		if(visible[i]) nail(ns,i);
	}
	enabled_in = ns.CardEnabled;
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
			* result = new arrayindex_t [enabled_in];
			arrayindex_t f = 0;
			for(arrayindex_t i = 0; i < net->Card[TR];i++)
			{
				if(status[i] == 0) continue;
				if(!ns.Enabled[i]) continue;
				(*result)[f++] = i;
			}
			assert(f == enabled_in);
			return enabled_in;
		}
		temp_card_enabled = enabled_in;
		if(speculate(ns,candidate_transition))
		{
			arrayindex_t a_key_transition = net->Card[TR];
			// check whether there is still a key transition and invisible transition
			bool invisible_enabled = false;
			bool invisible_instub = false;
			for(arrayindex_t t = 0; t < net->Card[TR]; t++)
			{
				if(!ns.Enabled[t]) continue;
				if(status[t] == 0 || status[t] == counter) 
				{
					if(!visible[t]) invisible_enabled = true;
					continue; // perm out or temp out
				}
				if(!visible[t]) invisible_instub=true;
				arrayindex_t j ;
				arrayindex_t cardOnlyDecreased = net->TrDecreasing[t] - net->TrDecreased[t];
				for(j = 0; j < cardOnlyDecreased; j++)
				{
					int stat = status[net->TrDecreased[t][j]];
					if(stat < 1 || stat == counter) break;
				}
				if(j < cardOnlyDecreased) continue; // not a key transition
				a_key_transition = t;
				break;
			}
			if(a_key_transition < net->Card[TR] && (invisible_instub || !invisible_enabled))
			{

				// candidate_transition + consequences can be safely removed
				enabled_in = temp_card_enabled;
				if(enabled_in == 1)
				{
					// no further candidate for removal -->
					// return stubborn set
					* result = new arrayindex_t [enabled_in];
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

bool FirelistStubbornLTLDeletion::speculate(NetState & ns,arrayindex_t node)
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

void FirelistStubbornLTLDeletion::nail(NetState & ns,arrayindex_t node)
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

