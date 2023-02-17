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

\brief Class for firelist generation. Use up sets for state predicate
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/FirelistStubbornDeadlock.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Net/NetState.h>

FirelistStubbornStatePredicate::FirelistStubbornStatePredicate(Petrinet * n, StatePredicate *p) : Firelist(n),
    predicate(p), dfsStack(new arrayindex_t[n->Card[TR]]), onStack(new bool[n->Card[TR]]()),
    dl(new FirelistStubbornDeadlock(n))
{net = n;}

FirelistStubbornStatePredicate::~FirelistStubbornStatePredicate()
{
    delete[] dfsStack;
    delete[] onStack;
    delete dl;
}

arrayindex_t FirelistStubbornStatePredicate::getFirelist(NetState &ns, arrayindex_t **result)
{

    if (ns.CardEnabled == 0)
    {
        // found a deadlock - return empty firelist
        * result = new arrayindex_t[1];
        return 0;
    }
    scapegoatNewRound();
    bool needEnabled = false;
    memset(onStack,0,net->Card[TR]*sizeof(bool));
    arrayindex_t stackpointer = predicate->getUpSet(dfsStack, onStack, &needEnabled);
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
	    // select scapegoat
	    arrayindex_t scapegoat = Firelist::selectScapegoat(ns,currenttransition);
            mustbeincluded = net->PlIncreasing[scapegoat];
            cardmustbeincluded = net->PlCardIncreasing[scapegoat];
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
    if (cardEnabled || !needEnabled)
    {
        // transition so far, this enabled transition serves as a valid
        // up set
        arrayindex_t size = cardEnabled;
        * result = new arrayindex_t [cardEnabled];
        for (arrayindex_t i = 0; i < stackpointer; ++i)
        {
            const arrayindex_t t = dfsStack[i];
            if (ns.Enabled[t])
            {
                (*result)[--cardEnabled] = t;
            }
            onStack[t] = false;
        }
        return size;
    }
    else
    {
        // if no enabled transition is in the stubborn set yet an up set
        // for deadlock is required, we return a deadlock preserving
        // up set.
        for (arrayindex_t i = 0; i < stackpointer; ++i)
        {
            onStack[dfsStack[i]] = false;
        }
        return dl->getFirelist(ns, result);
    }
}

Firelist *FirelistStubbornStatePredicate::createNewFireList(Petrinet * n, SimpleProperty *property)
{
    return new FirelistStubbornStatePredicate(n,(reinterpret_cast<StatePredicateProperty *>
            (property))->getPredicate());
}

arrayindex_t FirelistStubbornStatePredicate::getFirelist(arrayindex_t ttt, NetState &ns, arrayindex_t **result)
{   
    if (ns.CardEnabled == 0)
    {           
        // found a deadlock - return empty firelist
        * result = new arrayindex_t[1];
        return 0;
    }
    scapegoatNewRound();
    bool needEnabled = false;
    arrayindex_t stackpointer = 1;
    dfsStack[0] = ttt;
    onStack[ttt] = true;
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
        
            mustbeincluded = net->PlIncreasing[scapegoat];
            cardmustbeincluded = net->PlCardIncreasing[scapegoat];
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
        arrayindex_t size = cardEnabled;
        * result = new arrayindex_t [cardEnabled ? cardEnabled : 1];
        for (arrayindex_t i = 0; i < stackpointer; ++i)
        {
            const arrayindex_t t = dfsStack[i];
//RT::rep->status("IN STUBB: %s", net ->Name[TR][t]);
            if (ns.Enabled[t])
            {
//RT::rep->status("ENABLED");
                (*result)[--cardEnabled] = t;
            }
            onStack[t] = false;
        }
        return size;
}


