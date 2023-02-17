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

\brief Evaluates state predicate

Actual property is virtual, default (base class) is full exploration
*/

#include <Core/Dimensions.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Formula/StatePredicate/DeadlockPredicate.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <Formula/StatePredicate/StatePredicate.h>

StatePredicateProperty::StatePredicateProperty(Petrinet * n, StatePredicate *f)
:SimpleProperty(n)
{
	net = n;
    // set formula
    predicate = f;
    const arrayindex_t cardAtomic = f->countAtomic();
    AtomicStatePredicate **atomic = new AtomicStatePredicate *[cardAtomic ? cardAtomic : 1];
    f->collectAtomic(atomic);
    cardDeadlock = f->countDeadlock();
    changedDeadlock = new DeadlockPredicate *[cardDeadlock ? cardDeadlock : 1];
    arrayindex_t c = f->collectDeadlock(changedDeadlock);
    assert(c == cardDeadlock);
    const arrayindex_t cardFireable = f -> countFireable();
    FireablePredicate ** fireable = new FireablePredicate *[cardFireable ? cardFireable : 1];
    c = f -> collectFireable(fireable);
    assert(c == cardFireable);
    cardChanged = new arrayindex_t[net->Card[TR]]();
    cardChangedFireable = new arrayindex_t[net->Card[TR]]();
    // cannot be allocated with new[] due to a later realloc
    changedPredicate = (AtomicStatePredicate ** *) malloc(net->Card[TR] * SIZEOF_VOIDP);
    changedFireable = (FireablePredicate ** *) malloc(net->Card[TR] * SIZEOF_VOIDP);
    // cannot be allocated with new[] due to a later realloc
    changedSum = reinterpret_cast<int **>(malloc(net->Card[TR] * SIZEOF_VOIDP));

    for (arrayindex_t t = 0; t < net->Card[TR]; t++)
    {
	// handle atomic propositions
        changedPredicate[t] = reinterpret_cast<AtomicStatePredicate **>(malloc((cardAtomic ? cardAtomic : 1) * SIZEOF_VOIDP));
        changedSum[t] = reinterpret_cast<int *>(malloc((cardAtomic ? cardAtomic : 1) * SIZEOF_INT));
        for (arrayindex_t i = 0; i < cardAtomic; i++)
        {
            int s = 0;
            // evaluate delta that t causes on this predicate
            for (arrayindex_t j = 0; j < net->CardDeltaT[PRE][t]; j++)
            {
                const arrayindex_t p = net->DeltaT[PRE][t][j];
                for (arrayindex_t k = 0; k < atomic[i]->cardPos; k++)
                {
                    if (atomic[i]->posPlaces[k] == p)
                    {
                        s -= net->MultDeltaT[PRE][t][j]
                             * atomic[i]->posMult[k];
                        break;
                    }
                }
                for (arrayindex_t k = 0; k < atomic[i]->cardNeg; k++)
                {
                    if (atomic[i]->negPlaces[k] == p)
                    {
                        s += net->MultDeltaT[PRE][t][j]
                             * atomic[i]->negMult[k];
                        break;
                    }
                }
            }
            for (arrayindex_t j = 0; j < net->CardDeltaT[POST][t]; j++)
            {
                const arrayindex_t p = net->DeltaT[POST][t][j];
                for (arrayindex_t k = 0; k < atomic[i]->cardPos; k++)
                {
                    if (atomic[i]->posPlaces[k] == p)
                    {
                        s += net->MultDeltaT[POST][t][j]
                             * atomic[i]->posMult[k];
                        break;
                    }
                }
                for (arrayindex_t k = 0; k < atomic[i]->cardNeg; k++)
                {
                    if (atomic[i]->negPlaces[k] == p)
                    {
                        s -= net->MultDeltaT[POST][t][j]
                             * atomic[i]->negMult[k];
                        break;
                    }
                }
            }
            if (s)
            {
                changedPredicate[t][cardChanged[t]] = atomic[i];
                changedSum[t][cardChanged[t]++] = s;
            }
        }
        changedPredicate[t] = reinterpret_cast<AtomicStatePredicate **>(realloc(changedPredicate[t],
                              (cardChanged[t] ? cardChanged[t] : 1) * SIZEOF_VOIDP));
        changedSum[t] = reinterpret_cast<int *>(realloc(changedSum[t],
                                                (cardChanged[t] ? cardChanged[t] : 1) * SIZEOF_ARRAYINDEX_T));
    }
    // handle fireability predicates
    for (arrayindex_t t = 0; t < net->Card[TR]; t++)
    {
        changedFireable[t] = reinterpret_cast<FireablePredicate **>(malloc(1001 * SIZEOF_VOIDP)); // will be realloced in chunks of 1000
	cardChangedFireable[t] = 0;
    }
    for(arrayindex_t i = 0; i < cardFireable;i++)
    {
	arrayindex_t t = fireable[i]->t;
	for(arrayindex_t j = 0; j < net->CardArcs[TR][PRE][t]; j++)
	{
		arrayindex_t p = net->Arc[TR][PRE][t][j];

		// the pre-transitions of the pre-places may validate this predicate
		for(arrayindex_t k = 0; k < net->CardArcs[PL][PRE][p];k++)
		{
			arrayindex_t tt = net->Arc[PL][PRE][p][k];
			// insert this in ChangedFireable[tt]
			if(!(cardChangedFireable[tt] % 1000))
			{
				// list of tt full, need to extend
				changedFireable[tt] = reinterpret_cast<FireablePredicate**>(realloc(changedFireable[tt],((cardChangedFireable[tt]+1001) * SIZEOF_VOIDP)));
			}
			changedFireable[tt][cardChangedFireable[tt]++] = fireable[i];
		}
		// the conflicting transitions may invalidate this predicate
		for(arrayindex_t k = 0; k < net->CardArcs[PL][POST][p];k++)
		{
			arrayindex_t tt = net->Arc[PL][POST][p][k];
			// insert this in ChangedFireable[tt]
			if(!(cardChangedFireable[tt] % 1000))
			{
				// list of tt full, need to extend
				changedFireable[tt] = reinterpret_cast<FireablePredicate**>(realloc(changedFireable[tt],((cardChangedFireable[tt]+1001) * SIZEOF_VOIDP)));
			}
			changedFireable[tt][cardChangedFireable[tt]++] = fireable[i];
		}
	}
	// remove duplicates
	for(arrayindex_t j = 0; j < net->Card[TR];j++)
	{
		while(cardChangedFireable[j] > 1 && 
                      (changedFireable[j][cardChangedFireable[j]-1] ==
		      changedFireable[j][cardChangedFireable[j]-2]))
		{
			cardChangedFireable[j]--;
		}
	}
    }


    delete [] fireable;
    // initialize up sets
    for (arrayindex_t i = 0; i < cardAtomic; i++)
    {
	atomic[i] -> initUpSet();
    }
    for(arrayindex_t t = 0; t < net->Card[TR]; t++)
    {
	for(arrayindex_t j = 0; j < cardChanged[t]; j++)
	{
		if(changedSum[t][j] < 0) // t can turn <= predicate #j from false to true
		{
			changedPredicate[t][j]->addToUpSet(t);
		}
	}
    }

    //for(arrayindex_t i = 0; i < cardAtomic; i++)
    //{
//	atomic[i] -> finitUpSet();
 //   }

    delete [] atomic;

//for(int t = 0; t < net->Card[TR];t++)
//{
	//for(int i = 0; i < cardChanged[t];i++)
	//{
		//RT::rep->status("PROP N %s T %s P %s S %d", net -> name, net->Name[TR][t],changedPredicate[t][i]->toString(), changedSum[t][i]);
	//}
//}
}


StatePredicateProperty::~StatePredicateProperty()
{
    for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
    {
        free(changedPredicate[t]);
        free(changedSum[t]);
	free(changedFireable[t]);
    }
    free(changedPredicate);
    free(changedFireable);
    delete[] changedDeadlock;
    free(changedSum);
    delete[] cardChanged;
    delete[] cardChangedFireable;
}


bool StatePredicateProperty::initProperty(NetState &ns)
{
    predicate->evaluate(ns);
    assert(predicate->DEBUG__consistency(ns));
    return predicate->value;
}

bool StatePredicateProperty::checkProperty(NetState &ns, arrayindex_t t)
{
    for (arrayindex_t i = 0; i < cardChanged[t]; i++)
    {
        changedPredicate[t][i]->update(ns, changedSum[t][i]);
    }
    for (arrayindex_t i = 0; i < cardChangedFireable[t]; i++)
    {
        changedFireable[t][i]->update(ns);
    }
    for (arrayindex_t i = 0; i < cardDeadlock; i++)
    {
        changedDeadlock[i]->update(ns);
    }
    assert(predicate->DEBUG__consistency(ns));
    return predicate->value;
}

bool StatePredicateProperty::updateProperty(NetState &ns, arrayindex_t t)
{
    for (arrayindex_t i = 0; i < cardChanged[t]; i++)
    {
        changedPredicate[t][i]->update(ns,-changedSum[t][i]);
    }
    for (arrayindex_t i = 0; i < cardChangedFireable[t]; i++)
    {
        changedFireable[t][i]->update(ns);
    }
    for (arrayindex_t i = 0; i < cardDeadlock; i++)
    {
        changedDeadlock[i]->update(ns);
    }
    assert(predicate->DEBUG__consistency(ns));
    return predicate->value;
}


SimpleProperty *StatePredicateProperty::copy()
{
    StatePredicateProperty *spp = new StatePredicateProperty(net,predicate->copy(NULL));
    spp->stack = stack;
    spp->value = value;
    return spp;
}

bool StatePredicateProperty::checkOmegaProperty(NetState &ns)
{
    predicate->evaluateOmega(ns);
    //    assert(predicate->DEBUG__consistency(ns));
    return predicate->value;
}

bool StatePredicateProperty::isUnknown()
{
    return predicate->unknown;
}

void StatePredicateProperty::createDownSets(StatePredicate * f)
{

    // TODO: down set approach not yet fit for FIREABLE atomic propositions!!!!
    // TODO: down set approach not yet fit for DEADLOCK atomic propositions!!!!
    predicate = f;
    //f->top = f;

    const arrayindex_t cardAtomic = f->countAtomic();
    AtomicStatePredicate **atomic = new AtomicStatePredicate *[cardAtomic];
    f->collectAtomic(atomic);
    // initialize down sets
    for (arrayindex_t i = 0; i < cardAtomic; i++)
    {
	atomic[i] -> initDownSet();
    }
    for(arrayindex_t t = 0; t < net->Card[TR]; t++)
    {
	for(arrayindex_t j = 0; j < cardChanged[t]; j++)
	{
		if(changedSum[t][j] > 0) // t can increase expression
		{
			changedPredicate[t][j]->addToDownSet(t);
		}
	}
    }
    for(arrayindex_t i = 0; i < cardAtomic; i++)
    {
	atomic[i] -> finitDownSet();
    }

    delete[] atomic;
}
