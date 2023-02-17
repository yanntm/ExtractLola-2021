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
\status approved 23.05.2012, changed

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Planning/StoreCreator.h>
#include <Exploration/ComputeBoundExploration.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/ChooseTransition.h>
#include <Net/LinearAlgebra.h>
#include <SweepLine/Sweep.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/ChooseTransitionHashDriven.h>
#include <Exploration/ChooseTransitionRandomly.h>
#include <Frontend/Parser/ParserPnml.h>


#include "../../libs/lp_solve_5.5/lp_lib.h"

extern pthread_mutex_t nestedmutex;
extern nestedunit * startunit;

/*!
The result will be the maximum (over all reachable markings) of the given formal sum of places.

\param property  contains the expression to be checked as atomic proposition
\param ns  The initial state of the net has to be given as a net state object.
\param myStore  the store to be used. The selection of the store may greatly influence the
performance of the program
\param myFirelist  the firelists to use in this search. The firelist _must_ be
applicable to the given property, else the result of this function may be
wrong. It is not guaranteed that the given firelist will actually be used. In
the parallel work-mode the given list will just be used as a base list form
which all other lists will be generated
\param threadNumber  will be ignored by the standard seach. In the parallel
execution mode this number indicates the number of threads to be used for the
search
*/
int ComputeBoundExploration::depth_first_num(SimpleProperty &property, NetState &ns, Store<void> &myStore,
        Firelist &myFirelist, int)
{
	StatePredicateProperty * sp = reinterpret_cast<StatePredicateProperty*>(&property);
//RT::rep->status("SEARCH START ON %s", sp -> predicate -> toString());
	sp -> createDownSets(sp -> predicate);
	AtomicStatePredicate * a = reinterpret_cast<AtomicStatePredicate *>(sp -> predicate);
	StructuralBound = a -> upper_bound;
	StructuralBound -= a -> threshold;
	RT::rep->status("Structural Bound: %ld",StructuralBound);

	// prepare property
	property.value = property.initProperty(ns);
	result = a -> sum - a -> threshold;
	if(result == StructuralBound) return result ;

	// add initial marking to store
	// we do not care about return value since we know that store is empty

	myStore.searchAndInsert(ns, 0, 0);

	// get first firelist
	arrayindex_t *currentFirelist;
	arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
	while (true) { // exit when trying to pop from empty stack
		if (currentEntry-- > 0) {
			// there is a next transition that needs to be explored in current marking

			// fire this transition to produce new Marking::Current
			net->fire(ns, currentFirelist[currentEntry]);

			if (myStore.searchAndInsert(ns, 0, 0)) {
				// State exists! -->backtracking to previous state
				net->backfire(ns, currentFirelist[currentEntry]);
			} else {
				// State does not exist!
				net->updateEnabled(ns, currentFirelist[currentEntry]);
				// check current marking for property
				property.value = property.checkProperty(ns, currentFirelist[currentEntry]);
				if (a->sum - a -> threshold> result) {
					result = a -> sum - a -> threshold;
					if(result == StructuralBound) return result;
				}

			SimpleStackEntry *stack = property.stack.push();
				stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
				currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
			} // end else branch for "if state exists"
		} else {
			// firing list completed -->close state and return to previous state
			delete[] currentFirelist;
			if (property.stack.StackPointer == 0) {
				// have completely processed initial marking --> state not found
				return result ;
			}
			SimpleStackEntry &stack = property.stack.top();
			currentEntry = stack.current;
			currentFirelist = stack.fl;
			stack.fl = NULL;
			property.stack.pop();
			assert(currentEntry < Net::Card[TR]);
			net->backfire(ns, currentFirelist[currentEntry]);
			net->revertEnabled(ns, currentFirelist[currentEntry]);
			property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
		}
	}
	return false;
}

/* compute the upper bound using linear programming, lp_solve */
int ComputeBoundExploration::lp(Petrinet * net, AtomicStatePredicate * a)
{
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
		for (int i=1; i<=net->Card[PL] + net->Card[TR]; i++) {
			set_int(lp, i, TRUE);
			//set_col_name(lp,i, deconst(i <= net->Card[PL] ? net -> Name[PL][i-1] : net -> Name[TR][i-1 - net->Card[PL]]));
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
		int num_tin = net->CardArcs[PL][PRE][i];
		for (int k=0; k<num_tin; k++) {
			int tin = net->Arc[PL][PRE][i][k];
			int mult = net->Mult[PL][PRE][i][k];
			row[net->Card[PL] + tin + 1] = mult;
		}

		int num_tout = net->CardArcs[PL][POST][i];
		for (int k=0; k<num_tout; k++) {
			int tout = net->Arc[PL][POST][i][k];
			int mult = net->Mult[PL][POST][i][k];
			row[net->Card[PL] + tout + 1] += -1 * mult;
		}

		row[i+1] = -1.0;
		int m0 = net->Initial[i];
		if(!add_constraint(lp, row, EQ, static_cast<double>(-m0))){
			ret = 3;
			break;
		}
	}
	
	// nested unit info
        pthread_mutex_lock(&nestedmutex);
        for(nestedunit * uuu = startunit; uuu; uuu = uuu -> nextunit)
        {       
                if(!uuu->leafunit) continue;
                for(nestedunit * nnn = uuu; nnn; nnn = nnn -> nextinunit)
                {       
                        nnn -> marked = true;
                }
                std::fill(row, row + Ncol + 1, 0);
                for(int i  = 0;i < net -> Card[PL];i++)
                {       
                        if(!(((PlaceSymbol *)(net->thesymbol[PL][i])) -> nested->marked)) continue;
                        row[i+ 1] = 1.0;
                }
                if(!add_constraint(lp, row, LE, 1.0 ))
                {       
                        delete [] row;
                        delete_lp(lp);
                        lp = NULL;
                        return 10;
                }
                for(nestedunit * nnn = uuu; nnn; nnn = nnn -> nextinunit)
                {       
                        nnn -> marked = false;
                }
        }
        pthread_mutex_unlock(&nestedmutex);


	if(ret == 0) {
		set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */
		
		/*to show the model in lp format on screen */
		//write_LP(lp, stdout);

		/* I only want to see important messages on screen while solving */
		set_verbose(lp, IMPORTANT);

		/* Now let lpsolve calculate a solution */
		set_timeout(lp,300.0);
		ret = solve(lp);
		if(ret == OPTIMAL)
			ret = 0;
	}

	int rrr;
	if(ret == 0) {
		/* objective value */
		rrr = static_cast<int> (get_objective(lp) + 0.5);

		//RT::rep->status("Objective value: %d\n", result);

		/* variable values */
		/*get_variables(lp, row);
		for(j = 0; j < Ncol; j++)
			RT::rep->status("%s: %f\n", get_col_name(lp, j + 1), row[j]); */
	}
	else
	{
		rrr = 1 << 30;
	}

	/* free allocated memory */
	if(row != NULL)
		delete [] row;
	if(colno != NULL)
		delete [] colno;

	if(lp != NULL) {
		/* clean up such that all used memory by lpsolve is freed */
		delete_lp(lp);
	}
	
	if(ret == 0)
	{
		a -> upper_bound = rrr;
		return rrr;
	}
	else
	{
		a -> upper_bound = MAX_CAPACITY;
		return MAX_CAPACITY;
	}
}

void* ComputeBoundExploration::zerooneinvariantthread(void * vvv)
{
	Petrinet * nnn = Petrinet::InitialNet;
	ternary_t * resultvector = new ternary_t[nnn -> Card[PL]];
	lprec * lp = NULL;
	int Ncol = nnn -> Card[PL];
	REAL * row = NULL;
	row = new REAL[Ncol + 1];
	for(arrayindex_t i = 0; i < nnn -> Card[PL];i++)
	{
		resultvector[i] = TERNARY_VOID;
		if(nnn -> Initial[i] > 1)
		{
			resultvector[i] = TERNARY_FALSE;
			portfoliomanager::synchroniseplacevector(resultvector, GLOBAL_PREPROCESSING);
			goto finish;
			
		}
	}
	while(true)
	{
		//create the lp and solve it

		lp = make_lp(0, Ncol);
		if(!lp) goto finish;
		/* create space large enough for one row */
		if(row == NULL) goto finish;
		
		/* set all variables to integer and bound 1 */
		for (int i=1; i<=nnn->Card[PL]; i++) 
		{
			set_int(lp, i, TRUE);
			set_upbo(lp, i , 1.0);
			//set_col_name(lp,i, deconst(nnn -> Name[PL][i-1]));
		}
			
		/* set the objective function */

		std::fill(row, row + Ncol + 1, 0);
		for (int i = 0; i < nnn -> Card[PL]; i++) 
		{
			row[i+1] = resultvector[i] == TERNARY_VOID ? 1.0 : 0.0;
		}

		/* set the objective in lpsolve */
		if(!set_obj_fn(lp, row)) goto finish;
			
		/* set the object direction to maximize */
		set_maxim(lp);
		set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
		
		/* add the constraits row by row. These are the balance equations for the transitions */
		for (int i=0; i<nnn->Card[TR]; i++) 
		{
			std::fill(row, row + Ncol + 1, 0);
			for (int k=0; k<nnn->CardArcs[TR][PRE][i]; k++) 
			{
				row[nnn->Arc[TR][PRE][i][k] + 1] = -1.0 * static_cast<double>(nnn->Mult[TR][PRE][i][k]);
			}
			for (int k=0; k<nnn->CardArcs[TR][POST][i]; k++) 
			{
				row[nnn->Arc[TR][POST][i][k] + 1] += static_cast<double>(nnn->Mult[TR][POST][i][k]);
			}
//////////////////////////
			if(!add_constraint(lp, row, LE, 0.0))
			{
				goto finish;
			}
		}

		/* additional constraint states that precisely one token is in support */
		std::fill(row, row + Ncol + 1, 0);
		for(int i = 0; i < nnn->Card[PL]; i++)	
		{
			if(nnn -> Initial[i]) 
			{
				row[i+1] = 1.0;
			}
		}
		if(!add_constraint(lp, row, EQ, 1.0))
		{
			goto finish;
		}

		/* additional constraint states that at least one new place be covered*/
		std::fill(row, row + Ncol + 1, 0);
		int cardopen = 0;
		for (int i = 0; i < nnn -> Card[PL]; i++) 
		{
			if(resultvector[i] == TERNARY_VOID)
			{
				row[i+1] = 1.0 ;
				cardopen++;
			}
		}
		if(!add_constraint(lp, row, GE, 1.0))
		{
			goto finish;
		}
	// nested unit info
        pthread_mutex_lock(&nestedmutex);
        for(nestedunit * uuu = startunit; uuu; uuu = uuu -> nextunit)
        {       
                if(!uuu->leafunit) continue;
                for(nestedunit * nnn = uuu; nnn; nnn = nnn -> nextinunit)
                {       
                        nnn -> marked = true;
                }
                std::fill(row, row + Ncol + 1, 0);
                for(int i = 0 ; i < nnn -> Card[PL];i++)
                {       
                        if(!(((PlaceSymbol *)(nnn -> thesymbol[PL][i])) -> nested ->marked)) continue;
                        row[i + 1] = 1.0;
                }
                if(!add_constraint(lp, row, LE, 1.0 ))
                {       
                        delete [] row;
                        delete_lp(lp);
                        lp = NULL;
                        goto finish;
                }
                for(nestedunit * nn = uuu; nn; nn = nn -> nextinunit)
                {       
                        nn -> marked = false;
                }
        }
        pthread_mutex_unlock(&nestedmutex);

		set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

		/* I only want to see important messages on screen while solving */
		set_verbose(lp, IMPORTANT);
		//write_LP(lp, stdout);

		/* Now let lpsolve calculate a solution */
		int ret = solve(lp);
		if(ret != OPTIMAL && ret != SUBOPTIMAL) goto finish;

		if (get_objective(lp) <= 0.5)
		{
			// no new variable in support
			goto finish;
		}
		//print_objective(lp);
		/* variable values */
		get_variables(lp, row);
		int cardnewsolved = 0;
		for(int i = 0; i < nnn->Card[PL]; i++)
		{
			if(row[i] > 0.5) // in support of invariant
			{
				if(resultvector[i] == TERNARY_VOID)
				{
					cardnewsolved++;
				}
				resultvector[i] = TERNARY_TRUE;
			}
		}
		portfoliomanager::synchroniseplacevector(resultvector,GLOBAL_ZEROONE);
		if(lp) 
		{
			delete_lp(lp);
			lp  = NULL;
		}
		if(!portfoliomanager::cardtodo)
		{
			goto finish;
		}
	

	}
finish: // report to be ready

	if(lp) 
	{
		delete_lp(lp);
		lp  = NULL;
	}
	// release memory
	if(row) delete [] row;
	return NULL;
}

// LCOV_EXCL_START
Path ComputeBoundExploration::path() const
{
	return * (new Path(net));
}
// LCOV_EXCL_STOP


void ComputeBoundExploration::check_safety(SimpleProperty & property, NetState &ns, Store<void> &myStore,
        FirelistStubbornComputeBound &myFirelist, int)
{
	arrayindex_t p = portfoliomanager::gettargetplace();
	// add initial marking to store
	// we do not care about return value since we know that store is empty

	Petrinet * net = Petrinet::InitialNet;
		myStore.searchAndInsert(ns, 0, 0);

	// get first firelist
	arrayindex_t *currentFirelist;
	arrayindex_t currentEntry = myFirelist.getFirelist(p,ns, &currentFirelist);
	while (true) { // exit when trying to pop from empty stack
		if (currentEntry-- > 0) {
			arrayindex_t ttt = currentFirelist[currentEntry];
			// there is a next transition that needs to be explored in current marking

			// fire this transition to produce new Marking::Current
			net->fire(ns, ttt);

			if (myStore.searchAndInsert(ns, 0, 0)) {
				// State exists! -->backtracking to previous state
				net->backfire(ns, ttt);
			} else {
				// State does not exist!
				net->updateEnabled(ns, ttt);
				// check current marking for property
				for(arrayindex_t i = 0; i < net -> CardArcs[TR][POST][ttt];i++)
				{
					if(ns.Current[net->Arc[TR][POST][ttt][i]] > 1)
					{
						portfoliomanager::synchroniseplace(p,TERNARY_FALSE,GLOBAL_SEARCH);
						p = -1;
						return;
					}
				}
				

			SimpleStackEntry *stack = property.stack.push();
				stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
				currentEntry = myFirelist.getFirelist(p,ns, &currentFirelist);
			}
		} else {
			// firing list completed -->close state and return to previous state
			delete[] currentFirelist;
			if (property.stack.StackPointer == 0) {
				// have completely processed initial marking --> state not found
				portfoliomanager::synchroniseplace(p,TERNARY_TRUE,GLOBAL_SEARCH);
				return  ;
			}
			SimpleStackEntry &stack = property.stack.top();
			currentEntry = stack.current;
			currentFirelist = stack.fl;
			stack.fl = NULL;
			property.stack.pop();
			assert(currentEntry < Net::Card[TR]);
			net->backfire(ns, currentFirelist[currentEntry]);
			net->revertEnabled(ns, currentFirelist[currentEntry]);
		}
	}
}

void * ComputeBoundExploration::Safety(void *)
{
	// do whole planning stuff
	// create Mara object


	Petrinet * net = Petrinet::InitialNet;
	Mara * memory = new Mara();
	memory -> taskid = 3;
	portfoliomanager::taskjson[3] = JSON();
	NetState * ns;

	// init target place

	while(true)
	{
		Store<void> * store = StoreCreator<void>::createStore(net,memory,1); 
		ns = NetState::createNetStateFromInitial(net);
		SimpleProperty * property = new SimpleProperty(net);
		// run for target place
		check_safety(*property, *ns, *store, * new FirelistStubbornComputeBound(net,NULL), 1);
		
		// reset Mara memory
		memory->reset();
		delete property;

	}
}

void * ComputeBoundExploration::findpathsafetythread( void *)
{

	// initialise and get target
	Petrinet * net = Petrinet::InitialNet;
	FirelistStubbornComputeBound * myFirelist = new FirelistStubbornComputeBound(net, NULL);
	NetState ns = * NetState::createNetStateFromInitial(net);

	arrayindex_t targetplace = portfoliomanager::gettargetplace();
	while(true)
	{
	    unsigned int attempts = RT::args.retrylimit_arg;
	    if(attempts == 0) attempts = 1000;
	    unsigned int maxdepth = RT::args.depthlimit_arg;
	    unsigned int currentattempt = 0;

	    // loop #attempts times
	    while (currentattempt++ < attempts)
	    {

		// copy initial marking into current marking
		memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
		ns.HashCurrent = net->HashInitial;
		

		// reset enabledness information
		ns.CardEnabled = net->Card[TR];
		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    ns.Enabled[t] = true;
		}

		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    net->checkEnabled(ns, t);
		}
		
		// check property
		for(arrayindex_t i = 0; i < net->Card[PL];i++)
		{
			if(ns.Current[i] > 1)
			{
				portfoliomanager::synchroniseplace(i,TERNARY_FALSE,GLOBAL_FINDPATH);
				return NULL;
			}
		}


		// generate a firing sequence until given depth or deadlock is reached
		for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
		{

		    // get firelist
		    arrayindex_t *currentFirelist;
		    arrayindex_t cardFirelist = myFirelist->getFirelist(targetplace,ns, &currentFirelist);
		    if (cardFirelist == 0)
		    {
			// deadlock or empty up set (i.e. property not reachable)
			break; // go to next attempt
		    }

		    arrayindex_t chosen = rand() % ((cardFirelist * (cardFirelist +1 )) / 2);
		    int i;
		    for(i = 0; i < cardFirelist; i++)
		    {
			if(chosen > ((cardFirelist - i - 1) * ( cardFirelist - i)) / 2) 
			{
				chosen = currentFirelist[i]; 
				break;
			}
		    }
		    if(i >= cardFirelist) chosen = currentFirelist[0];
		    if(i >= cardFirelist) // safety belt, condition should be always false
		    {
			chosen = currentFirelist[0];
		    }
		    delete [] currentFirelist;

		    net->fire(ns, chosen);
		    net->updateEnabled(ns, chosen);

		    // check property
		    for(int i = 0; i < net -> CardOnlyPost[TR][chosen]; i++)
		    {
			if(ns.Current[net->OnlyPost[TR][chosen][i]] > 1)
			{
				portfoliomanager::synchroniseplace(net->OnlyPost[TR][chosen][i],TERNARY_FALSE,GLOBAL_FINDPATH);
				return NULL;
			}
		    }
		    for(int i = 0; i < net -> CardGreaterPost[TR][chosen]; i++)
		    {
			if(ns.Current[net->GreaterPost[TR][chosen][i]] > 1)
			{
				portfoliomanager::synchroniseplace(net->GreaterPost[TR][chosen][i],TERNARY_FALSE,GLOBAL_FINDPATH);
				return NULL;
			}
		    }

                }
            }
	   // we did not find a state after #attempts
	   // select new target place
	   portfoliomanager::synchroniseplace(targetplace,TERNARY_VOID,GLOBAL_FINDPATH);
	   targetplace = portfoliomanager::gettargetplace();
	}
}

bool ComputeBoundExploration::find_path(SimpleProperty &property, NetState &ns,
                               Firelist &myFirelist, EmptyStore<void> &s,
                               ChooseTransition &c)
{
    // this table counts hits for various hash buckets. This is used for
    // steering search into less frequently entered areas of the state space.

attempts = RT::args.retrylimit_arg;
maxdepth = RT::args.depthlimit_arg;
StatePredicateProperty * spp = (StatePredicateProperty *) &property;
  
    AtomicStatePredicate * a = (AtomicStatePredicate *) spp->predicate;
    if(a -> upper_bound == MAX_CAPACITY)
    {
	return false;
    }
	spp -> createDownSets(a);
    unsigned int currentattempt = 0;
    capacity_t result;

    // get memory for witness path info
    arrayindex_t *witness = new arrayindex_t[maxdepth];


    // loop #attempts times
    while (attempts == 0 || currentattempt++ < attempts)
    {
        // register this try
        s.tries++;

        // copy initial marking into current marking
        memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
        ns.HashCurrent = net->HashInitial;

        // reset enabledness information
        ns.CardEnabled = net->Card[TR];
        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            ns.Enabled[t] = true;
        }

        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            net->checkEnabled(ns, t);
        }

        // prepare property
        property.value = property.initProperty(ns);
	result = a -> sum - a -> threshold;
	StructuralBound = a -> upper_bound;
	StructuralBound -= a -> threshold;
	if(result == StructuralBound) return true ;

        // generate a firing sequence until given depth or deadlock is reached
        for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
        {
            // register this transition's firing
            s.searchAndInsert(ns, 0, 0);

            // get firelist
            arrayindex_t *currentFirelist;
            arrayindex_t cardFirelist = myFirelist.getFirelist(ns, &currentFirelist);
            if (cardFirelist == 0)
            {
                // deadlock or empty up set (i.e. property not reachable)
                break; // go to next attempt
            }


            arrayindex_t chosen = c.choose(ns, cardFirelist, currentFirelist);
            delete [] currentFirelist;


            net->fire(ns, chosen);
            net->updateEnabled(ns, chosen);

            property.value = property.checkProperty(ns, chosen);
	    result = a -> sum - a -> threshold;
	    if(result == StructuralBound) return true ;
        }
    }

    // we did not find a state after #attempts
    return false;
}

