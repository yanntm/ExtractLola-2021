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


#include <CoverGraph/CoverPayload.h>
#include <Symmetry/Constraints.h>
#include <SweepLine/Sweep.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/FullTask.h>
#include <Planning/StoreCreator.h>
#include <Planning/ReachabilitySearchTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornCombinedDeadlock.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>



/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/

FullTask::FullTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
       net = n;
        formula_id = fid; 
        memory = new Mara();
	taskname = deconst("state space");
        portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_INITIAL,EXCL_MEM_TASK,memory);
	goStatus = false;
}

FullTask::~FullTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete covStore;
    delete p;
    delete fl;
    delete exploration;
#endif
}

ternary_t FullTask::getResult()
{

    number_of_threads = 1;
    switch (RT::args.search_arg)
    {
    case search_arg_sweepline:
    {
        // dummy store for the sweepline method, only counts markings and calls
        portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "sweepline";
        store = new SweepEmptyStore(net,memory);
	covStore = NULL;
        break;
    }
    case search_arg_covergraph:
    {
        portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "cover";
        if (RT::args.encoder_arg != encoder_arg_fullcopy)
        {
            RT::rep->status("warning: encoder does not fully support coverability graphs");
        }
        covStore = StoreCreator<CoverPayload>::createStore(net,memory,number_of_threads);
	store = NULL;
        break;
    }
    case search_arg_depth:
    {
        portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "dfs";

        // choose a store
	store = StoreCreator<void>::createStore(net,memory,number_of_threads);
	covStore = NULL;
        break;
    }
    case search__NULL:
        assert(false);
    default: ;
    }
    // choose a simple property
        p = new SimpleProperty(net);
	switch(RT::args.stubborn_arg)
	{
	case stubborn_arg_deletion:  
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock/deletion";
                fl = new FirelistStubbornCombinedDeadlock(net);
		break;
	case stubborn_arg_off: 
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
		break;
	case stubborn_arg_tarjan:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock/insertion";
		fl = new FirelistStubbornDeadlock(net);
		break;
	case stubborn_arg_combined:
	case stubborn_arg_both:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock/combined";
                fl = new FirelistStubbornCombinedDeadlock(net);
		break;
	default: assert(false);
        }
    // set the correct exploration algorithm
	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = number_of_threads;
            if (number_of_threads == 1)
            {
                exploration = new DFSExploration(net,memory);
            }
            else
            {
                exploration = new ParallelExploration(net,memory);
            }
    ns = NetState::createNetStateFromInitial(net);
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

    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    portfoliomanager::report(portfolio_id,result);
    return result;
}

char * FullTask::interpreteResult(ternary_t result)
{
	return deconst( "State space generated.");
}
 
Path FullTask::getWitnessPath()
{
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

capacity_t *FullTask::getMarking()
{
        return NULL;
}

statusrecord * FullTask::getStatistics()
{
	statusrecord * result = new statusrecord();
	if(store)
	{
		result -> markings = store->get_number_of_markings();
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
        result -> calls = store->get_number_of_calls();
    }
	else if (covStore)
    {
        result -> calls = covStore->get_number_of_calls();
    }
    else
    {
	assert(false);
    }
	return result;
} 

void FullTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	FullTask * F = new FullTask(n,par,f,fid);
	F -> preprocessingfinished = true;
}


statusrecord * FullTask::getStatus()
{
	if(!goStatus) return NULL;
	statusrecord * result = new statusrecord();
        result -> markings = store->get_number_of_markings();
	result -> calls = store->get_number_of_calls(); 
        return result;

}

void FullTask::derail()
{
	memory->stop =true;
}

Task * FullTask::copy()
{
	return new FullTask(net,parent, formula, formula_id);
}


