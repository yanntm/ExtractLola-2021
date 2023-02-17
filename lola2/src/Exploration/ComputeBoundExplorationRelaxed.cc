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

\brief compute reachability using relaxed stubborn sets
*/

#include <Core/Dimensions.h>
#include <Exploration/ComputeBoundExplorationRelaxed.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/FirelistStubbornCloseGiven.h>
#include "../../libs/lp_solve_5.5/lp_lib.h"
#include <Formula/StatePredicate/AtomicStatePredicate.h>

int ComputeBoundExplorationRelaxed::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    StructuralBound = lp(property,ns,myStore,myFirelist, 1);
    StatePredicateProperty * p = reinterpret_cast<StatePredicateProperty *> (&property);
    AtomicStatePredicate * a = reinterpret_cast<AtomicStatePredicate *>(p -> predicate);
    StructuralBound -= a -> threshold;
    RT::rep->status("Structural Bound: %ld",StructuralBound);

    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    memset(lastfired,0,net->Card[TR] * sizeof(arrayindex_t));

    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);
    

    property.value = property.initProperty(ns);
    result = a -> sum - a -> threshold;
    if(result == StructuralBound) return result;


    bool * inUpset = new bool[net->Card[TR]];
    memset(inUpset,0,sizeof(bool) * net->Card[TR]);
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset = 0;

    bool * inDownSet = new bool[net->Card[TR]];
    memset(inDownSet,0,sizeof(bool) * net->Card[TR]);

    for(arrayindex_t i = 0; i < a -> cardPos; i++)
    {
	arrayindex_t p = a -> posPlaces[i];
	for(arrayindex_t j = 0; j < net->PlCardIncreasing[p]; j++)
	{
		arrayindex_t t = net->PlIncreasing[p][j];
		if(inUpset[t]) continue;
		inUpset[t] = true;
		Upset[cardUpset++] = t;
	}
	for(arrayindex_t j = 0; j < net->PlCardDecreasing[p];j++)
	{
		inDownSet[net->PlDecreasing[p][j]] = true;
	}
    }
    for(arrayindex_t i = 0; i < a -> cardNeg; i++)
    {
	arrayindex_t p = a -> negPlaces[i];
	for(arrayindex_t j = 0; j < net->PlCardDecreasing[p]; j++)
	{
		arrayindex_t t = net->PlDecreasing[p][j];
		if(inUpset[t]) continue;
		inUpset[t] = true;
		Upset[cardUpset++] = t;
	}
	for(arrayindex_t j = 0; j < net->PlCardIncreasing[p];j++)
	{
		inDownSet[net->PlIncreasing[p][j]] = true;
	}
    }

    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
    bool containsdown = false;
    for(arrayindex_t i = 0; i < currentEntry; i++)
    {
	if(inDownSet[currentFirelist[i]]) 
	{
		containsdown = true;
		break;
	}
    }
    if(containsdown)
    {
	delete [] currentFirelist;
        arrayindex_t * stubbornstack = new arrayindex_t[net->Card[TR]];
	memcpy(stubbornstack,Upset,cardUpset * sizeof(arrayindex_t));
        bool * onstack = new bool[net->Card[TR]];
	memcpy(onstack,inUpset, net->Card[TR] * sizeof(bool));
        currentEntry = flclose->getFirelist(ns,&currentFirelist,stubbornstack, onstack, cardUpset, &containsdown);
	delete [] stubbornstack;
	delete [] onstack;
    }
    

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 1;

    //set initial dfs to one
    setDFS(payload, ++currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

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
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
                setDFS(newPayload, ++currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put an entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
 		property.value =  property.checkProperty(ns, currentFirelist[currentEntry]);
		if (a->sum - a -> threshold> result) 
		{
			result = a -> sum - a -> threshold;
			if(result == StructuralBound) return result ;
		}

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
		for(arrayindex_t i = 0; i < currentEntry; i++)
    		{
        		if(inDownSet[currentFirelist[i]])
        		{
                		containsdown = true;
                		break;
        		}
    		}
    		if(containsdown)
    		{
        		delete [] currentFirelist;
        		arrayindex_t * stubbornstack = new arrayindex_t[net->Card[TR]];
        		memcpy(stubbornstack,Upset,cardUpset * sizeof(arrayindex_t));
        		bool * onstack = new bool[net->Card[TR]];
        		memcpy(onstack,inUpset, net->Card[TR] * sizeof(bool));
        		currentEntry = flclose->getFirelist(ns,&currentFirelist,stubbornstack, onstack, cardUpset, &containsdown);
			delete [] stubbornstack;
			delete [] onstack;
    		}

            }
        }
        else
        {
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
		// this is root of tscc 
		// --> check for ignored transitions in an up-set of this marking
		bool needEnabled = false;

		arrayindex_t * ups = new arrayindex_t[net->Card[TR]];
		memcpy(ups,Upset,cardUpset * sizeof(arrayindex_t));
		bool * inups = new bool[net->Card[TR]];
		memcpy(inups,inUpset,net->Card[TR] * sizeof(bool));
		arrayindex_t cardups = cardUpset;
		for(arrayindex_t i = 0; i < cardups;i++)
		{
			arrayindex_t t = ups[i];
			if(lastfired[t] >= getDFS(payload))
			{
				// this transition is not ignored
				inups[t] = false;
				ups[i--] = ups[--cardups];
			}
			else
			{
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardups)
		{
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    	delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,ups,inups,cardups,&needEnabled);
			delete [] ups;
			delete [] inups;
			if(currentEntry) continue;
		}
		// this tscc is finally closed

		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
                //set the property value
                return result;
            }
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
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

/* compute the upper bound using linear programming, lp_solve */
int ComputeBoundExplorationRelaxed::lp(SimpleProperty &property, NetState &ns, Store<statenumber_t> &myStore,
                                    Firelist &myFirelist, int)
{
        StatePredicateProperty * sp = reinterpret_cast<StatePredicateProperty*>(&property);
        sp -> createDownSets(sp -> predicate);
        AtomicStatePredicate * a = reinterpret_cast<AtomicStatePredicate *>(sp -> predicate);

        arrayindex_t *posPlaces = a -> posPlaces;
        arrayindex_t *negPlaces = a -> negPlaces;
        arrayindex_t cardPos = a -> cardPos;
        arrayindex_t cardNeg = a -> cardNeg;
        capacity_t *posMult = a -> posMult;
        capacity_t *negMult = a -> negMult;

        //create the lp and solve it
        lprec *lp;
        int Ncol, *colno = NULL, j, ret = 0;
        REAL *row = NULL;
        Ncol = net->Card[PL] + net->Card[TR];
        lp = make_lp(0, Ncol);
        if(lp == NULL)
                ret = 1; /* couldn't construct a new model... */

        if(ret == 0) {

                /* create space large enough for one row */
                //colno = (int *) malloc(Ncol * sizeof(*colno));
                //row = (REAL *) malloc(Ncol * sizeof(*row));
                colno = new int[Ncol];
                row = new REAL[Ncol + 1];
                if((colno == NULL) || (row == NULL))
                        ret = 2;
        }
        /* The enumeration of variables is as follows: p0~1, p1~2,...,pMAX_PLACES-1~MAX_PLACES,t0~MAX_PLACES+1, t1~MAX_PLACES+2,... */

        if(ret == 0){
                /* set all variables to integer */
                for (int i=1; i<=net->Card[PL]; i++) {
                        set_int(lp, i, TRUE);
                }
                for (int i=net->Card[PL] + 1; i<=net->Card[TR]; i++) {
                        set_int(lp, i, TRUE);
                }
        }

        /* set the objective function */
        j = 0;

        for (int i = 0; i < cardPos; i++) {
                colno[j] = posPlaces[i] +1;
                row[j++] = static_cast<double> (posMult[i]);
        }

        for (int i = 0; i < cardNeg; i++) {
                colno[j] = negPlaces[i] +1;
                row[j++] = -1 * static_cast<double> (negMult[i]);
        }

        /* set the objective in lpsolve */
        if(!set_obj_fnex(lp, j, row, colno))
                ret = 4;

        if(ret == 0) {
      /* set the object direction to maximize */
                set_maxim(lp);
                set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
        }

        /* add the constraits row by row. These are the rows of the state equation*/
        for (int i=0; i<net->Card[PL]; i++) {
                std::fill(row, row + Ncol + 1, 0);
                j = 0;
                int num_tin = net->CardArcs[PL][PRE][i];
                for (int k=0; k<num_tin; k++) {
                        int tin = net->Arc[PL][PRE][i][k];
                        int mult = net->Mult[PL][PRE][i][k];
                        //colno[j] = net_>Card[PL] + tin + 1;
                        row[net->Card[PL] + tin + 1] = mult;
                }

                int num_tout = net->CardArcs[PL][POST][i];
                for (int k=0; k<num_tout; k++) {
                        int tout = net->Arc[PL][POST][i][k];
                        int mult = net->Mult[PL][POST][i][k];
                        //colno[j] = net->Card[PL] + tout + 1;
                        row[net->Card[PL] + tout + 1] += -1 * mult;
                }

                //colno[j] = i+1;
                row[i+1] = -1;
                int m0 = net->Initial[i];
                if(!add_constraint(lp, row, EQ, -m0)){
                        ret = 3;
                        break;
                }
        }
   if(ret == 0) {
                set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

                /*to show the model in lp format on screen */
                //write_LP(lp, stdout);

                /* I only want to see important messages on screen while solving */
                set_verbose(lp, IMPORTANT);

                /* Now let lpsolve calculate a solution */
                set_timeout(lp,30.0);
                ret = solve(lp);
                if(ret == OPTIMAL)
                        ret = 0;
        }
        capacity_t result;

        if(ret == 0) {
                /* objective value */
                result = static_cast<int> (get_objective(lp) + 0.5);

                //RT::rep->status("Objective value: %d\n", result);

                /* variable values */
                /*get_variables(lp, row);
                for(j = 0; j < Ncol; j++)
                        RT::rep->status("%s: %f\n", get_col_name(lp, j + 1), row[j]);*/
        }
        else
        {
                result = 1 << 30;
        }

     /* free allocated memory */
        if(row != NULL)
                free(row);
        if(colno != NULL)
                free(colno);

        if(lp != NULL) {
                /* clean up such that all used memory by lpsolve is freed */
                delete_lp(lp);
        }

        if(ret == 0)
                return result;
        else
                return MAX_CAPACITY;
}


