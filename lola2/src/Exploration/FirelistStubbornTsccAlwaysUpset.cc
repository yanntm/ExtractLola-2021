/*!
\file
\author Markus
\status new
\brief Class for firelist generation. Generates a firelist based on Stubborn
sets created by upsets and downsets
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornTsccAlwaysUpset.h>
#include <Net/NetState.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>

FirelistStubbornTsccAlwaysUpset::FirelistStubbornTsccAlwaysUpset(Petrinet * n,SimpleProperty *simpleproperty) : Firelist(n),
    sp(static_cast<StatePredicateProperty *>(simpleproperty)),
    dfsStack(new arrayindex_t[n->Card[TR]]),
    onStack(new bool[n->Card[TR]]()),
    dl(new FirelistStubbornDeadlock(n))
{net = n;}

FirelistStubbornTsccAlwaysUpset::~FirelistStubbornTsccAlwaysUpset()
{
    delete[] dfsStack;
    delete[] onStack;
}


/*!
  \param ns the net-state for which the firelist should be determined
  \param[in,out] result the actual fire list (contains the enabled transitions for this netstate)
  \result number of elements in fire list
  */
arrayindex_t FirelistStubbornTsccAlwaysUpset::getFirelist(NetState &ns, arrayindex_t **result)
{
    scapegoatNewRound();
    //if property is true then upset is empty
    // other cases for upset calulation:
    // Property := State Property
    // M(p)     := Marking (eg. ns)
    // k        := the natural number defined by property that
    //             creates a border to M(p)
    //
    //
    // Property == M(p) <= k : up(M)
    // Property == M(p) = k : up(M)
    // Property == M(p) >= k : up(M)
    // Property == M(p) != k : up(M)
    arrayindex_t cardEnabled = 0;
    arrayindex_t stackpointer = 0;
    //get the upset
    bool needEnabled;
    if (!sp->getPredicate()->value)
    {
        stackpointer = sp->getPredicate()->getUpSet(dfsStack, onStack, &needEnabled);
    }
    else
    {
        //do bruteforce firelist or just set upset empty?
        //stackpointer = 0;
        // for now use bruteforce
        assert(ns.CardEnabled <= n->Card[TR]);

        *result = new arrayindex_t[ns.CardEnabled];
        arrayindex_t i = 0;
        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            if (ns.Enabled[t])
            {
                assert(i < ns.CardEnabled);
                (*result)[i++] = t;
            }
        }
        return ns.CardEnabled;
    }


    // loop until all stack elements processed
    for (arrayindex_t firstunprocessed = 0; firstunprocessed < stackpointer; ++firstunprocessed)
    {
        arrayindex_t currenttransition = dfsStack[firstunprocessed];
        arrayindex_t *mustbeincluded;
        arrayindex_t cardmustbeincluded;
        if (ns.Enabled[currenttransition])
        {
            ++cardEnabled;
            mustbeincluded = net->TrDecreasing[currenttransition];
            cardmustbeincluded = net->TrCardDecreasing[currenttransition];
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
        // an up set for deadlock atomic propositions is any
        // enabled transition. If, through the other up sets,
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
        // If the other up sets to not yield an enabled transition,
        // we need to return a stubborn set with an enabled
        // transition. This is exactly any deadlock preserving
        // stubborn set.
        for (arrayindex_t i = 0; i < stackpointer; ++i)
        {
            onStack[dfsStack[i]] = false;
        }
        //TODO if ns.CardEnabled is == 0 dl->getFirelist fail is one assertion
        // why this happens?
        if(ns.CardEnabled!=0)
        {
            return dl->getFirelist(ns, result);
        }
        else{
	    *result = new arrayindex_t[1];
            return ns.CardEnabled;
        }
        
    }
}

arrayindex_t FirelistStubbornTsccAlwaysUpset::getFirelist(arrayindex_t ttt, NetState &ns, arrayindex_t **result)
{
    scapegoatNewRound();
    arrayindex_t cardEnabled = 0;
    arrayindex_t stackpointer = 0;
    //get the upset
    bool needEnabled;
    if (!ns.Enabled[ttt])
    {
	dfsStack[0] = ttt;
	onStack[ttt] = true;
	stackpointer = 1;
    }
    else
    {
        //do bruteforce firelist or just set upset empty?
        //stackpointer = 0;
        // for now use bruteforce
        assert(ns.CardEnabled <= n->Card[TR]);

        *result = new arrayindex_t[ns.CardEnabled];
        arrayindex_t i = 0;
        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            if (ns.Enabled[t])
            {
                assert(i < ns.CardEnabled);
                (*result)[i++] = t;
            }
        }
        return ns.CardEnabled;
    }


    // loop until all stack elements processed
    for (arrayindex_t firstunprocessed = 0; firstunprocessed < stackpointer; ++firstunprocessed)
    {
        arrayindex_t currenttransition = dfsStack[firstunprocessed];
        arrayindex_t *mustbeincluded;
        arrayindex_t cardmustbeincluded;
        if (ns.Enabled[currenttransition])
        {
            ++cardEnabled;
            mustbeincluded = net->TrDecreasing[currenttransition];
            cardmustbeincluded = net->TrCardDecreasing[currenttransition];
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

arrayindex_t FirelistStubbornTsccAlwaysUpset::getFirelist(arrayindex_t * tlist, int cardtlist, NetState &ns, arrayindex_t **result)
{
    scapegoatNewRound();
    arrayindex_t cardEnabled = 0;
    arrayindex_t stackpointer = 0;
    //get the upset
    bool needEnabled;
    // check whether up set has enabled transitions

    bool hasenabled = false;
    for(int i = 0; i < cardtlist;i++)
    {
	if(ns.Enabled[tlist[i]])
	{
		hasenabled = true;
		break;
	}
    }
    if (!hasenabled)
    {
	stackpointer = 0;
	for(int i = 0; i < cardtlist;i++)
	{
		dfsStack[stackpointer++] = tlist[i];
		onStack[tlist[i]] = true;
	}		
    }
    else
    {
        //do bruteforce firelist or just set upset empty?
        //stackpointer = 0;
        // for now use bruteforce
        assert(ns.CardEnabled <= n->Card[TR]);

        *result = new arrayindex_t[ns.CardEnabled];
        arrayindex_t i = 0;
        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            if (ns.Enabled[t])
            {
                assert(i < ns.CardEnabled);
                (*result)[i++] = t;
            }
        }
        return ns.CardEnabled;
    }


    // loop until all stack elements processed
    for (arrayindex_t firstunprocessed = 0; firstunprocessed < stackpointer; ++firstunprocessed)
    {
        arrayindex_t currenttransition = dfsStack[firstunprocessed];
        arrayindex_t *mustbeincluded;
        arrayindex_t cardmustbeincluded;
        if (ns.Enabled[currenttransition])
        {
            ++cardEnabled;
            mustbeincluded = net->TrDecreasing[currenttransition];
            cardmustbeincluded = net->TrCardDecreasing[currenttransition];
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


Firelist *FirelistStubbornTsccAlwaysUpset::createNewFireList(Petrinet * n,SimpleProperty *property)
{
    return new FirelistStubbornTsccAlwaysUpset(n,property);
}
