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

#include <config.h>
#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/FirelistStubbornDeadlock.h>
#include <Exploration/FirelistStubbornCombinedDeadlock.h>
#include <Exploration/ParallelExploration.h>
#include <Exploration/DeadlockExploration.h>
#include <Exploration/StatePredicateProperty.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <InputOutput/InputOutput.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Planning/DeadlockSearchTask.h>
#include <Planning/StoreCreator.h>
#include <Symmetry/Symmetry.h>
#include <Symmetry/Constraints.h>
#include <SweepLine/Sweep.h>
#include <CoverGraph/CoverGraph.h>
#include <Core/Handlers.h>

DeadlockSearchTask::DeadlockSearchTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net= n;
     tandem = false;
     parent = par;
     formula = f;
     formula_id = fid;
      result = TERNARY_UNKNOWN;
	memory = new Mara();
     taskname = deconst("state space");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_DEADLOCK,EXCL_MEM_TASK,memory);
     goStatus = false;

    // prepare task

}

DeadlockSearchTask::DeadlockSearchTask(Petrinet * n, int par, tFormula  f, int fid,bool fi)
{
	preprocessingfinished = false;
	tandem = true;
	forceinsertion = fi;
	net = n;
     parent = par;
     formula = f;
     formula_id = fid;
    memory = new Mara();
     if(forceinsertion)
     {
	     taskname = deconst("state space / insertion");
	     portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_DEADLOCK,SEARCH_TASK,memory);
     }
     else
     {
	     taskname = deconst("state space / deletion");
	     portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_DEADLOCK,EXCL_MEM_TASK,memory);
     }
     goStatus = false;
}


/*!
\post memory for all members is deallocated
*/
DeadlockSearchTask::~DeadlockSearchTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete covStore;
    delete store;
    delete p;
    delete fl;
    delete exploration;
#endif
}


ternary_t DeadlockSearchTask::getResult()
{
     if(tandem)
     {

	    // prepare task

	    switch (RT::args.search_arg)
	    {
	    case search_arg_sweepline:
	    {
		assert(false); // not supported
		break;
	    }
	    case search_arg_covergraph:
	    {
		assert(false); // not supported
		break;
	    }
		default:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "dfs";
		// choose a store
		store = StoreCreator<void>::createStore(net,memory,number_of_threads);

		covStore = NULL;
	    }

	    
	    switch(RT::args.search_arg)
	    {
	    case search_arg_covergraph:
	    case search_arg_sweepline:
			assert(false);
		    break;
	    default:
		    p = new DeadlockExploration(net);
	    }
	    if (forceinsertion)
	    {
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/insertion";
			fl = new FirelistStubbornDeadlock(net);
	    }
	    else
	    {
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/combined";
			fl = new FirelistStubbornCombinedDeadlock(net);
	    }
	    // set the correct exploration algorithm
		portfoliomanager::taskjson[portfolio_id]["configuration"]["treads"] = 1;
			exploration = new DFSExploration(net,memory);
     }
     else
     {

	    switch (RT::args.search_arg)
	    {
	    case search_arg_sweepline:
	    {
		// dummy store for the sweepline method, only counts markings and calls
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "sweepline";
		store = new SweepEmptyStore(net,memory);
		covStore = NULL;
		    p = new DeadlockExploration(net);
		break;
	    }
	    case search_arg_covergraph:
	    {
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "cover";
		if (RT::args.encoder_arg != encoder_arg_fullcopy)
		{
		    RT::rep->status("state space: warning: encoder does not support coverability graphs -- replacing");
		}
		covStore = StoreCreator<CoverPayload>::createStore(net,memory,number_of_threads);
		store = NULL;
		p = new StatePredicateProperty(net,new DeadlockPredicate(net,true));
		break;
	    }
		default:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "dfs";
		// choose a store
		store = StoreCreator<void>::createStore(net,memory,number_of_threads);
		covStore = NULL;
		    p = new DeadlockExploration(net);
	    }
	    
	    switch (RT::args.stubborn_arg)
	    {
	    case stubborn_arg_deletion:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/deletion";
			fl = new FirelistStubbornCombinedDeadlock(net);
			break;
	    case stubborn_arg_tarjan:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/insertion";
			fl = new FirelistStubbornDeadlock(net);
			break;
	    case stubborn_arg_combined:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/combined";
			fl = new FirelistStubbornCombinedDeadlock(net);
			break;
	    case stubborn_arg_off:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
			fl = new Firelist(net);
			break;
	    default:
		assert(false); // enumeration exhaustive
	    }
	    // set the correct exploration algorithm
		portfoliomanager::taskjson[portfolio_id]["configuration"]["treads"] = number_of_threads;
	    if (number_of_threads == 1)
	    {
			exploration = new DFSExploration(net,memory);
	    }
	    else
	    {
			exploration = new ParallelExploration(net,memory);
	    }
     }
    ns = NetState::createNetStateFromInitial(net);

    //TODO can we make these assumptions clearer that the asserts are creating
    assert(ns);
    assert(store);
    assert(exploration);
    assert(p);
    assert(fl);

    // compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	if(store) store = reinterpret_cast<SymmetryStore<void>*>(store)->setGeneratingSet(SymmetryCalculator::G);
	if(covStore) covStore = reinterpret_cast<SymmetryStore<CoverPayload>*>(covStore)->setGeneratingSet(SymmetryCalculator::G);
    }
 	goStatus = true;

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);
                switch (RT::args.search_arg)
                {
                case search_arg_depth:
                    bool_result = exploration->depth_first(*p, *ns, *store, *fl, 1);
                    break;
                case search_arg_sweepline:
                    // no choice of stores for sweepline method here
                    bool_result = exploration->sweepline(*p, *ns, *reinterpret_cast<SweepEmptyStore *>(store), *fl,
                                                         RT::args.sweepfronts_arg, number_of_threads,memory);
                    break;

                case search_arg_covergraph:

                    result = exploration->cover_breadth_first(*p, *ns, *covStore, *fl, number_of_threads, FORMULA_DEADLOCK);
                    break;

                default:
                    assert(false);
                }

    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }

    if (RT::args.store_arg == store_arg_bloom)
    {
	    // if the Bloom store did not find anything, the result is unknown
            if (result == TERNARY_FALSE)
            {
                result = TERNARY_UNKNOWN;
            }
    }
    //TODO can we make these assumptions clearer that the asserts are creating
    portfoliomanager::report(portfolio_id,result);
    return result;
}


char * DeadlockSearchTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The net has deadlock(s).");
    case TERNARY_FALSE: return deconst( "The net does not have deadlocks.");
    case TERNARY_UNKNOWN: return deconst( "The net may or may not have deadlocks.");
	default: assert(false);
    }
	return NULL;
}


Path DeadlockSearchTask::getWitnessPath() 
{
        return p->path();
}


capacity_t *DeadlockSearchTask::getMarking() 
{
    if (p and p->value)
    {
        return ns->Current;
    }
    else
    {
        return NULL;
    }

}

statusrecord *  DeadlockSearchTask::getStatistics()
{
	statusrecord * result = new statusrecord();
	//if (RT::args.store_arg == store_arg_bloom)
    //{
        //const double n = static_cast<double>(store->get_number_of_markings());
        //const double k = RT::args.hashfunctions_arg;
        //const double m = static_cast<double>(BLOOM_FILTER_SIZE);
	//result -> probability_false_positive = pow((1.0 - exp((-k * n) / m)), k);
	//result -> optimal_hash_functions =  log(m / n) / log(2.0);
    //}

	result -> markings = store ? store -> get_number_of_markings() : 
                                    covStore -> get_number_of_markings();
        result -> calls = store ? store -> get_number_of_calls() :
                                covStore -> get_number_of_calls();
	return result;
}

statusrecord * DeadlockSearchTask::getStatus()
{
	if(!goStatus)
	{
		return NULL;
	}
	statusrecord * result = new statusrecord();
	switch(RT::args.search_arg)
	{
	case search_arg_sweepline:
		exploration -> s -> getStatus(result);
		break;
	case search_arg_covergraph:
	result -> markings = covStore->get_number_of_markings();
	result -> calls = covStore->get_number_of_calls();
	default:
	result -> markings = store->get_number_of_markings();
	result -> calls = store->get_number_of_calls();
	}
	return result;
}

void DeadlockSearchTask::derail()
{
	memory -> stop = true;
}

Task * DeadlockSearchTask::copy()
{
	return new DeadlockSearchTask(net, parent, portfoliomanager::copyFormula(formula),formula_id);
}
