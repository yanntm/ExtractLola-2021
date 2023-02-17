/*!
\file
\status new
\brief Class for firelist generation. Generates a firelist based on Stubborn
sets created by upsets and downsets
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornCloseGiven.h>
#include <Net/NetState.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>

FirelistStubbornCloseGiven::FirelistStubbornCloseGiven(Petrinet * n) : Firelist(n),
    dl(new FirelistStubbornDeadlock(n))
{net =n;}

/*!
  \param ns the net-state for which the firelist should be determined
  \param[in,out] result the actual fire list (contains the enabled transitions for this netstate)
  \result number of elements in fire list
  */
arrayindex_t FirelistStubbornCloseGiven::getFirelist(NetState &ns, arrayindex_t **result, arrayindex_t * dfsStack, bool * onStack, arrayindex_t stackpointer, bool *need)
{
   arrayindex_t cardEnabled = 0;
    scapegoatNewRound();
    //get the upset
    * need = true;
    bool needEnabled = true;

    // loop until all stack elements processed
    for (arrayindex_t firstunprocessed = 0; firstunprocessed < stackpointer; ++firstunprocessed)
    {
        arrayindex_t currenttransition = dfsStack[firstunprocessed];
//RT::rep->status("CONSIDER %s", net -> Name[TR][currenttransition]);
        arrayindex_t *mustbeincluded;
        arrayindex_t cardmustbeincluded;
        if (ns.Enabled[currenttransition])
        {
//RT::rep->status("ENABLED");
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
//RT::rep->status("DISABLED BY %s", net -> Name[PL][scapegoat]);
        }
        for (arrayindex_t i = 0; i < cardmustbeincluded; ++i)
        {
            const arrayindex_t t = mustbeincluded[i];
//RT::rep->status("INCLUDE %s", net->Name[TR][t]);
            if (!onStack[t])
            {
//RT::rep->status("... IS NEW");
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
//RT::rep->status("RETURNING STUB OF SOZE %d", size);
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


