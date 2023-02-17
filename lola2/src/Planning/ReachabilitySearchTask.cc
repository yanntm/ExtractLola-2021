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


#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <CoverGraph/CoverPayload.h>
#include <Symmetry/Constraints.h>
#include <SweepLine/Sweep.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/StoreCreator.h>
#include <Planning/ReachabilitySearchTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ReachabilityExplorationRelaxed.h>
#include <Exploration/FirelistStubbornReachabilityRelaxed.h>
#include <Exploration/ParallelExploration.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>

#include "../../libs/lp_solve_5.5/lp_lib.h"


/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/


ReachabilitySearchTask::ReachabilitySearchTask(Petrinet * n, int par, tFormula  f, int fid)
{

	preprocessingfinished = false;
     tandem = false;
     net = n;
    parent = par; 
    formula = f; 
    formula_id = fid;
    result = TERNARY_UNKNOWN;
    memory = new Mara();
    taskname = deconst("state space");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,EXCL_MEM_TASK,memory);
    tokenthreshold = 0;
    goStatus = false;
    // extract state predicate from formula
    // check is also used
    kc::tFormula TheFormulaRS = formula;

	pthread_mutex_lock(&kimwitu_mutex);
    TheFormulaRS = TheFormulaRS->rewrite(kc::singletemporal);
    TheFormulaRS = TheFormulaRS->rewrite(kc::simpleneg);
    TheFormulaRS = TheFormulaRS->rewrite(kc::booleanlists);

Petrinet::InitialNet = net;
	unparsed.clear();
        TheFormulaRS->unparse(myprinter, kc::internal);      
        spFormula = TheFormulaRS->formula->copy(NULL);
	pthread_mutex_unlock(&kimwitu_mutex);

}
ReachabilitySearchTask::ReachabilitySearchTask(Petrinet * n, int par, tFormula  f, int fid,bool fi)
{
	preprocessingfinished = false;
    tandem = true;
    forceinsertion = fi;
    net = n;
    parent = par;
    formula = f;
    formula_id = fid;
    result = TERNARY_UNKNOWN;
    memory = new Mara();
    tokenthreshold = 0;
	if(forceinsertion)
	{
	    taskname = deconst("tandem / insertion");
	    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,SEARCH_TASK,memory);
	    relaxed = false;
		portfoliomanager::taskjson[portfolio_id]["taskanme"] = "tandem / insertion";
	}
	else
	{
	    taskname = deconst("tandem  / relaxed");
	    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,EXCL_MEM_TASK,memory);
	    relaxed = true;
		portfoliomanager::taskjson[portfolio_id]["taskanme"] = "tandem / relaxed";
	}
    goStatus = false;
    // extract state predicate from formula
    // copy formula for additional rewrite to avoid conflict if stateequation 
    // check is also used
    kc::tFormula TheFormulaRS;
	pthread_mutex_lock(&kimwitu_mutex);
    TheFormulaRS = formula;
    TheFormulaRS = TheFormulaRS->rewrite(kc::singletemporal);
    TheFormulaRS = TheFormulaRS->rewrite(kc::simpleneg);
    TheFormulaRS = TheFormulaRS->rewrite(kc::booleanlists);

Petrinet::InitialNet = net;
	unparsed.clear();
        TheFormulaRS->unparse(myprinter, kc::internal);      
        spFormula = TheFormulaRS->formula->copy(NULL);
	pthread_mutex_unlock(&kimwitu_mutex);
}

ReachabilitySearchTask::~ReachabilitySearchTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete covStore;
    delete p;
    //delete spFormula;
#endif
}

ternary_t ReachabilitySearchTask::getResult()
{
	number_of_threads = 1;
	if(tandem)
	{
	    if(relaxed)
	    {
		// choose a store
		store1 = StoreCreator<statenumber_t>::createStore(net,memory,number_of_threads);
		store = NULL;
		covStore = NULL;
	    }
	    else
	    {
	    switch (RT::args.search_arg)
	    {
	    case search_arg_sweepline:
	    {
		assert(false); // not supported in tandem search
		break;
	    }
	    case search_arg_covergraph:
	    {
		assert(false); // not supported in tandem search
		break;
	    }
	    case search_arg_depth:
	    {

		// choose a store
		store = StoreCreator<void>::createStore(net,memory,number_of_threads);
		covStore = NULL;
		store1 = NULL;
		break;
	    }
	    case search__NULL:
		assert(false);
	    default: ;
	    }
	    }

	    // choose a simple property
		    p = new StatePredicateProperty(net,spFormula);
		    if(forceinsertion)
		    {
				portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "reachability preserving/insertion";
				fl = new FirelistStubbornStatePredicate(net,spFormula);
		    }
		    else
		    {
				portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "relaxed reachability preserving/deletion";
				bool * visible = new bool[net->Card[TR]];
				memset(visible,0,net->Card[TR] * sizeof(bool));
				p -> predicate -> setNondestroying(visible);
						int vis = 0;
						for(arrayindex_t i = 0; i < net->Card[TR];i++)
						{
							if(visible[i]) vis++;
						}
						portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;
				fl = new FirelistStubbornReachabilityRelaxed(net,spFormula);
				fl -> visible = visible;
		    }

	    // set the correct exploration algorithm
			if(relaxed)
			{
				exploration1 = new ReachabilityExplorationRelaxed(net);
			}
			else
			{
				exploration = new DFSExploration(net,memory);
			}
	}
	else
	{
	    if(RT::args.relaxed_given)
	    {
		portfoliomanager::taskjson[portfolio_id]["taskanme"] = "state space";
		relaxed = true;

		// choose a store
		store1 = StoreCreator<statenumber_t>::createStore(net,memory,number_of_threads);
		store = NULL;
		covStore = NULL;

	    }
	    else
	    {
		relaxed = false;
	    switch (RT::args.search_arg)
	    {
	    case search_arg_sweepline:
	    {
		// dummy store for the sweepline method, only counts markings and calls
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "sweepline";
		store = new SweepEmptyStore(net,memory);
		covStore = NULL;
		store1 = NULL;
		break;
	    }
	    case search_arg_covergraph:
	    {
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "cover";

		if (RT::args.encoder_arg != encoder_arg_fullcopy)
		{
		    RT::rep->status("state space: warning: encoder does not fully support coverability graphs");
		}
		covStore = StoreCreator<CoverPayload>::createStore(net,memory,number_of_threads);
		store = NULL;
		store1 = NULL;
		break;
	    }
	    case search_arg_depth:
	    {
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "dfs";

		// choose a store
		store = StoreCreator<void>::createStore(net,memory,number_of_threads);
		covStore = NULL;
		store1 = NULL;
		break;
	    }
	    case search__NULL:
		assert(false);
	    default: ;
	    }
	    }

	    // choose a simple property
		    p = new StatePredicateProperty(net, spFormula);

	    // prepare stubborn set method 
		if(RT::args.stubborn_arg != stubborn_arg_off)
		{
			if(RT::args.relaxed_given)
			{
				bool * visible = new bool[net->Card[TR]];
				memset(visible,0,net->Card[TR] * sizeof(bool));
				p -> predicate -> setNondestroying(visible);
						int vis = 0;
						for(arrayindex_t i = 0; i < net->Card[TR];i++)
						{
							if(visible[i]) vis++;
						}
						portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;

				if(RT::args.stubborn_arg != stubborn_arg_tarjan)
				{
					portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "relaxed reachability preserving/insertion";
					fl = new FirelistStubbornReachabilityRelaxed(net,spFormula);
				}
				else
				{
					portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "relaxed reachability preserving/deletion";
					fl = new FirelistStubbornReachabilityRelaxed(net,spFormula);
				}
				fl -> visible = visible;

			}
			else
			{
				portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "reachability preserving/insertion";
				fl = new FirelistStubbornStatePredicate(net,spFormula);
			}
		}
		else
		{
		
		portfoliomanager::taskjson[portfolio_id]["cwconfiguration"]["stubborn"] = "no";
			fl = new Firelist(net);
		    }

	    // set the correct exploration algorithm
		portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = number_of_threads;
			if(RT::args.relaxed_given)
			{
				exploration1 = new ReachabilityExplorationRelaxed(net);
			}
			else
			{
			    if (number_of_threads == 1)
			    {
				exploration = new DFSExploration(net,memory);
			    }
			    else
			    {
				exploration = new ParallelExploration(net,memory);
			    }
			}
	}
    ns = NetState::createNetStateFromInitial(net);
try
{

    // Apply specific modification in case we are a tokenquickcheck

    if(RT::args.tokenquickcheck_arg != tokenquickcheck_arg_off && tokenthreshold != 0)
    {
	if(RT::args.tokenquickcheck_arg == tokenquickcheck_arg_mono)
	{
		if(! spFormula->monotonous())
		{
			portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
			return TERNARY_UNKNOWN;
		}
	}
	int * newm0 = new int[net -> Card[PL]];
	bool change_made = false;
	switch(RT::args.tokenthresholdbase_arg)
	{
	case tokenthresholdbase_arg_one:
		for(int i = 0; i < net -> Card[PL];i++)
		{
			if(ns->Current[i])
			{
				newm0[i] = RT::args.tokenthresholdfactor_arg / 100 + RT::args.tokenthresholdsummand_arg;
				if(newm0[i] <= 0) newm0[i] = 1;
				if(newm0[i] > ns->Current[i]) newm0[i] = ns->Current[i];
				
			}
			else
			{
				newm0[i] = 0;
			}
			if(ns->Current[i] != newm0[i]) change_made = true;
		}
		break;
	case tokenthresholdbase_arg_all:
		for(int i = 0; i < net -> Card[PL];i++)
		{
			if(ns->Current[i])
			{
				newm0[i] = ns->Current[i] * RT::args.tokenthresholdfactor_arg  / 100 + RT::args.tokenthresholdsummand_arg;
				if(newm0[i] <= 0) newm0[i] = 1;
				if(newm0[i] > ns->Current[i]) newm0[i] = ns->Current[i];
				
			}
			else
			{
				newm0[i] = 0;
			}
			if(ns->Current[i] != newm0[i]) change_made = true;
		}
		break;
	case tokenthresholdbase_arg_largestconstant:
		{
		int cutoff = spFormula->getLargestConstant();
		for(arrayindex_t p = 0; p < net -> Card[PL]; p++)
		{
			for(int i = 0; i < net -> CardArcs[PL][PRE][p]; i++)
			{
				int result1 = net -> Mult[PL][PRE][p][i];
				if(result1 > cutoff) cutoff = result1;
			}
			for(int i = 0; i < net -> CardArcs[PL][POST][p]; i++)
			{
				int result1 = net -> Mult[PL][POST][p][i];
				if(result1 > cutoff) cutoff = result1;
			}
		}
		for(int i = 0; i < net -> Card[PL];i++)
		{
			if(ns->Current[i])
			{
				newm0[i] = cutoff * RT::args.tokenthresholdfactor_arg  / 100 + RT::args.tokenthresholdsummand_arg;
				if(newm0[i] <= 0) newm0[i] = 1;
				if(newm0[i] > ns->Current[i]) newm0[i] = ns->Current[i];
				
			}
			else
			{
				newm0[i] = 0;
			}
			if(ns->Current[i] != newm0[i]) change_made = true;
		}
		break;
		}
	case tokenthresholdbase_arg_stateequation:
		{
		pthread_mutex_lock(&kimwitu_mutex);	
		AtomicBooleanPredicate * dnf = spFormula->copy(NULL)->DNF();
		pthread_mutex_unlock(&kimwitu_mutex);
		if(!dnf)
		{
			return TERNARY_UNKNOWN;
		}
		if(dnf -> magicnumber == MAGIC_NUMBER_TRUE)
		{
			return TERNARY_TRUE;
		}
		if(dnf -> magicnumber == MAGIC_NUMBER_FALSE)
		{
			return TERNARY_FALSE;
		}
	        if((!(dnf -> isAnd)) && (dnf->cardSub == 0))
		{
			portfoliomanager::report(portfolio_id,TERNARY_FALSE);
			return TERNARY_FALSE;
		}
		if((dnf -> isAnd) && (dnf->cardSub == 0))
		{
			portfoliomanager::report(portfolio_id,TERNARY_TRUE);
			return TERNARY_TRUE;
		}

		if(dnf->isAnd)
		{
			AtomicBooleanPredicate * temp = new AtomicBooleanPredicate(net,false);
			temp -> addSub(dnf);
			dnf = temp;
		}

		// prepare transition order
		bool tseen[net -> Card[TR]];
		bool pseen[net -> Card[PL]];
		memset(tseen, 0, net -> Card[TR] * sizeof(bool));
		memset(pseen, 0, net -> Card[PL] * sizeof(bool));
		arrayindex_t torder[net -> Card[TR]];
		int orderindex = 0;
		int orderprocessed = 0;
		
		for(int i = 0; i < net -> Card[TR];i++)
		{
			if(ns -> Enabled[i])
			{
				torder[orderindex++] = i;
				tseen[i] = true;
			}
		}
		for(orderprocessed = 0; orderprocessed < orderindex; orderprocessed++)
		{
			int t = torder[orderprocessed];
RT::rep->status("EXPAND %s IDX %d PRO %d", net -> Name[TR][t], orderindex,orderprocessed);
			for(int j = 0; j < net -> CardArcs[TR][POST][t];j++)
			{
				int p = net -> Arc[TR][POST][t][j];
RT::rep->status("CONSIDER POSTPLACE %s", net -> Name[PL][p]);
				if(pseen[p]) continue;
RT::rep->status("NEW");
				pseen[p] = true;
				for(int k = 0; k < net -> CardArcs[PL][POST][p];k++)
				{
					int tt = net -> Arc[PL][POST][p][k];
RT::rep->status("INSERT %s", net -> Name[TR][tt]);
					if(tseen[tt]) continue;
RT::rep->status("NEW");
					tseen[tt] = true;
					torder[orderindex++] = tt;
RT::rep->status("IDX %d PRO %d", orderindex,orderprocessed);
				}
			}
		}
		memset(newm0,0,net -> Card[PL] * sizeof(int));
		for(int i = 0; i < dnf -> cardSub;i++)
		{
			AtomicBooleanPredicate * conj = (AtomicBooleanPredicate *) dnf -> sub[i];
RT::rep->status("CREATE LP FOR %s", conj -> toString());
			//create the lp and solve it
			lprec *lp;
			int Ncol, *colno = NULL, j, ret = 0;
			REAL *row = NULL;
			Ncol = net->Card[PL] + net->Card[TR];
			lp = make_lp(0, Ncol);
			if(lp == NULL)
			ret = 1; /* couldn't construct a new model... */

			if(ret == 0) 
			{
				/* create space large enough for one row */
				//colno = (int *) malloc(Ncol * sizeof(*colno));
				//row = (REAL *) malloc(Ncol * sizeof(*row));
				colno = new int[Ncol];
				row = new REAL[Ncol + 1];
				if((colno == NULL) || (row == NULL))
					ret = 2;
			}
			/* The enumeration of variables is as follows: p0~1, p1~2,...,pMAX_PLACES-1~MAX_PLACES,t0~MAX_PLACES+1, t1~MAX_PLACES+2,... */

			if(ret == 0)
			{
				/* set all variables to integer */
				for (int i=1; i<=net->Card[PL] + net->Card[TR]; i++) 
				{
					set_int(lp, i, TRUE);
					//set_col_name(lp,i, deconst(i <= net->Card[PL] ? net -> Name[PL][i-1] : net -> Name[TR][i-1 - net->Card[PL]]));
				}
			}
			/* set the objective function: minimise Parikh vector */
			j = 0;

			for (int i = 0; i < net->Card[TR]; i++) 
			{
				colno[j] = net->Card[PL] + 1 + i;
				row[j++] = 1.0;
			}

			/* set the objective in lpsolve */
			if(!set_obj_fnex(lp, j, row, colno))
				ret = 4;

			if(ret == 0) 
			{
				/* set the object direction to minimise */
				set_minim(lp);
				set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
			}

			/* add the constraits row by row. These are the rows of the state equation*/
			for (int i=0; i<net->Card[PL]; i++) 
			{
				std::fill(row, row + Ncol + 1, 0);
				int num_tin = net->CardArcs[PL][PRE][i];
				for (int k=0; k<num_tin; k++) 
				{
					int tin = net->Arc[PL][PRE][i][k];
					int mult = net->Mult[PL][PRE][i][k];
					row[net->Card[PL] + tin + 1] = mult;
				}

				int num_tout = net->CardArcs[PL][POST][i];
				for (int k=0; k<num_tout; k++) 
				{
					int tout = net->Arc[PL][POST][i][k];
					int mult = net->Mult[PL][POST][i][k];
					row[net->Card[PL] + tout + 1] += -1 * mult;
				}

				row[i+1] = -1.0;
				int m0 = net->Initial[i];
				if(!add_constraint(lp, row, EQ, static_cast<double>(-m0)))
				{
					ret = 3;
				}
			}
			/* now the constraints of the predicate */
			j = 0;

			for(int k = 0; k < conj -> cardSub; k++)
			{
				AtomicStatePredicate * a = (AtomicStatePredicate *) conj -> sub[k];
				for (int i = 0; i < a->cardPos; i++) 
				{
					colno[j] = a->posPlaces[i] +1;
					row[j++] = static_cast<double> (a->posMult[i]);
				}

				for (int i = 0; i < a->cardNeg; i++) 
				{
					colno[j] = a->negPlaces[i] +1;
					row[j++] = -1 * static_cast<double> (a->negMult[i]);
				}
				if(!add_constraintex(lp,j, row, colno,LE, a->threshold))
				{
					ret = 3;
				}
			}

			if(ret == 0) 
			{
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
			if(ret == 0) 
			{
				/* variable values */
				get_variables(lp, row);
				int mmm [net -> Card[PL]];
				for(int i = 0; i < net -> Card[PL];i++)
				{
					mmm[i] = 0;
				}
				for(int j = 0; j <  orderindex; j++)
				{
					int t = torder[j];
RT::rep->status("ROW %f", row[net -> Card[PL] + t]);
					int parikh = row[net -> Card[PL] + t];
RT::rep->status("FIRE %s %d times", net -> Name[TR][t], parikh);
					for(int k = 0; k < net -> CardArcs[TR][PRE][t];k++)
					{
						int p = net -> Arc[TR][PRE][t][k];
						int d = net -> Mult[TR][PRE][t][k] * parikh;
						if(d > mmm[p])
						{
							newm0[p] += d - mmm[p];
							mmm[p] = 0;
						}
						else
						{
							mmm[p] -= d;
						}
					}
					for(int k = 0; k < net -> CardArcs[TR][POST][t];k++)
					{
						int p = net -> Arc[TR][POST][t][k];
						int d = net -> Mult[TR][POST][t][k] * parikh;
						mmm[p] += d;
					}
				}
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

			if(lp != NULL) 
			{
				/* clean up such that all used memory by lpsolve is freed */
				delete_lp(lp);
			}

		}
		for(int i = 0; i < net -> Card[PL];i++)
		{
			if(ns->Current[i])
			{
				newm0[i] = newm0[i] * RT::args.tokenthresholdfactor_arg  / 100 + RT::args.tokenthresholdsummand_arg;
				if(newm0[i] <= 0) newm0[i] = 1;
				if(newm0[i] > ns->Current[i]) newm0[i] = ns->Current[i];
RT::rep->status("NEW MO %s; %d", net -> Name[PL][i],newm0[i]);
				
			}
			else
			{
				newm0[i] = 0;
			}
			if(ns->Current[i] != newm0[i]) change_made = true;
		}
		break;
		}
		default: break;
	}
	if(!change_made) 
	{
		portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
		return TERNARY_UNKNOWN;
	}
	if(RT::args.tokenquickcheck_arg == tokenquickcheck_arg_shift)
	{
		if(! spFormula->shiftable())
		{
			portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
			return TERNARY_UNKNOWN;
		}
		spFormula->exec_shift(newm0,ns->Current);
	}
	// adjust netstate
	ns->HashCurrent = 0;
	for(int i = 0; i < net->Card[PL];i++)
	{
		if(ns->Current[i] > newm0[i])
		{
			ns->Current[i] = newm0[i];
		}
		ns -> HashCurrent += ns->Current[i] * net -> Hash[i];
		ns -> HashCurrent %= SIZEOF_MARKINGTABLE;
	}
	for(int i = 0; i < net->Card[TR];i++)
	{
		net->checkEnabled(*ns,i);
	}
	spFormula -> evaluate(*ns);
    }

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);

    // compute symmetries
    if (RT::args.symmetry_given && RT::args.search_arg != search_arg_sweepline && !tokenthreshold)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	if(store) store = reinterpret_cast<SymmetryStore<void>*>(store)->setGeneratingSet(SymmetryCalculator::G);
	if(covStore) covStore = reinterpret_cast<SymmetryStore<CoverPayload>*>(covStore)->setGeneratingSet(SymmetryCalculator::G);
    }

    goStatus = true; // start reporting progress

    if(relaxed)
    {
	  bool_result = exploration1->depth_first(*p, *ns, *store1, *fl, number_of_threads);
    }
    else
    {
	    switch (RT::args.search_arg)
	    {
		case search_arg_depth:
		    bool_result = exploration->depth_first(*p, *ns, *store, *fl, number_of_threads);
		    break;

		case search_arg_sweepline:
		    // no choice of stores for sweepline method here
		    bool_result = exploration->sweepline(*p, *ns, *reinterpret_cast<SweepEmptyStore *>(store), *fl,
							 RT::args.sweepfronts_arg, number_of_threads,memory);
		    break;

		case search_arg_covergraph:
		    result = exploration->cover_breadth_first(*p, *ns, *covStore, *fl, number_of_threads, FORMULA_REACHABLE);
		    break;

		default:
		    assert(false);
	    }
    }

    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    if(tokenthreshold && (result == TERNARY_FALSE)) result = TERNARY_UNKNOWN;
    if (RT::args.store_arg == store_arg_bloom)
    {
            if (result == TERNARY_FALSE)
            {
                result = TERNARY_UNKNOWN;
            }
    }
	portfoliomanager::report(portfolio_id,result);
    return result;
}
catch(quickcheckexception)
{
	portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
	return TERNARY_UNKNOWN;
}
}

char * ReachabilitySearchTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is reachable.");
    case TERNARY_FALSE: return deconst( "The predicate is unreachable.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be reachable.");
	default: assert(false);
    }
return NULL;
}
 
Path ReachabilitySearchTask::getWitnessPath()
{
    if(relaxed) return *(new Path(net));
    if (RT::args.search_arg == search_arg_covergraph)
    {
        // cover graph
        return exploration->path();
    }
    else
    {
        // simple property
        return p->path();
    }
}

capacity_t *ReachabilitySearchTask::getMarking()
{
    // we only provide witness states for simple properties where we found
    // a result
    if (p and p->value)
    {
        return ns->Current;
    }
    else
    {
        return NULL;
    }
}

statusrecord * ReachabilitySearchTask::getStatistics()
{
	statusrecord * result = new statusrecord();
	if(store)
	{
		result -> markings = store->get_number_of_markings();
	}
	else if(store1)
	{
		result -> markings = store1->get_number_of_markings();
	}
	else if(covStore)
	{
		result -> markings = covStore->get_number_of_markings();
	}
	else
	{
		assert(false);
	}
    if (store)
    {
        result -> calls  = store->get_number_of_calls();
    }
    else if (store1)
    {
        result -> calls  = store1->get_number_of_calls();
    }
	else if (covStore)
    {
        result -> calls  = covStore->get_number_of_calls();
    }
    else
    {
	assert(false);
    }
	return result;
} 

void ReachabilitySearchTask::buildTask(Petrinet * n,int par, tFormula  f, int fid)
{
	Task * result;
	if(RT::args.stubborn_arg != stubborn_arg_both && 
           RT::args.tokenquickcheck_arg==tokenquickcheck_arg_off)
	{
		ReachabilitySearchTask * R = new ReachabilitySearchTask(n,par,f,fid);
		R -> preprocessingfinished = true;
		return;
	}
	f -> id = NULL;
	tFormula  ff = portfoliomanager::copyFormula(f);
	tFormula  fff = portfoliomanager::copyFormula(f);
	if(RT::args.stubborn_arg == stubborn_arg_both)
	{
		int agg = portfoliomanager::addTask(n,NULL,par,fid, FORMULA_REACHABLE,AGGREGATE_TASK);
		f -> id = NULL;
		tFormula  ff = portfoliomanager::copyFormula(f);
		tFormula  fff = portfoliomanager::copyFormula(f);
		int f1 = portfoliomanager::addFormula(f);
		int f2 = portfoliomanager::addFormula(ff);
		ReachabilitySearchTask * R1 = new ReachabilitySearchTask(n,agg,f,f1,true);
		ReachabilitySearchTask * R2 = new ReachabilitySearchTask(n,agg,ff,f2,false);
		R1 -> preprocessingfinished = true;
		R2 -> preprocessingfinished = true;
	}
	else
	{
		ReachabilitySearchTask * R = new ReachabilitySearchTask(n,par,f,fid);
		R -> preprocessingfinished = true;
		
	}
	if(RT::args.tokenquickcheck_arg!=tokenquickcheck_arg_off)
	{
		int f3 = portfoliomanager::addFormula(fff);
		ReachabilitySearchTask * tqc = new ReachabilitySearchTask(n,par,fff,f3,true);
		tqc -> tokenthreshold = 1;
		tqc -> taskname = deconst("search / frozen tokens");
		tqc -> preprocessingfinished = true;
	}
}


statusrecord * ReachabilitySearchTask::getStatus()
{
	if(!goStatus)
	{
		return NULL; // do not report anything since symmetry calculation is still under way
	}
	statusrecord * result = new statusrecord();
	switch(RT::args.search_arg)
	{
	case search_arg_covergraph:
	{
		result -> markings = covStore->get_number_of_markings();
		result -> calls = covStore->get_number_of_calls(); 
		break;
	}
	case search_arg_sweepline:
		exploration -> s -> getStatus(result);
		break;
	default:
	{
		if(relaxed)  
		{
			result -> markings = store1->get_number_of_markings();
			result -> calls = store1->get_number_of_calls(); 
		}
		else 
		{
			result -> markings = store->get_number_of_markings();
			result -> calls = store->get_number_of_calls(); 
		}
	}
	}
        return result;
}

void ReachabilitySearchTask::derail()
{
	memory->stop = true;
}


Task * ReachabilitySearchTask::copy()
{
	return new ReachabilitySearchTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
