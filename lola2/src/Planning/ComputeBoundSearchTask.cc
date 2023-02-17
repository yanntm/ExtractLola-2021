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
#include <Planning/ComputeBoundSearchTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornComputeBound.h>
#include <Exploration/FirelistStubbornComputeBoundCombined.h>
#include <Exploration/ComputeBoundExploration.h>
#include <Exploration/ComputeBoundExplorationRelaxed.h>



/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/


ComputeBoundSearchTask::ComputeBoundSearchTask(Petrinet * n,int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula_id = fid;
	formula = f;
	memory = new Mara();
	taskname = deconst("state space");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid, FORMULA_BOUND,EXCL_MEM_TASK,memory);
	portfoliomanager::isnumerical[portfolio_id] = true;
	portfoliomanager::isnumerical[parent] = true;
	goStatus = false;
        // extract state predicate from formula
	assert(formula);

	pthread_mutex_lock(&kimwitu_mutex);
        formula->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
	// compute structural bound to make it available to all tasks
	

}

ComputeBoundSearchTask::~ComputeBoundSearchTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete rstore;
    delete covStore;
    delete p;
    delete spFormula;
    delete fl;
    delete exploration;
    delete rexploration;
#endif
}

ternary_t ComputeBoundSearchTask::getResult()
{
        // choose a store
	if(RT::args.relaxed_given)
	{
		rstore = StoreCreator<statenumber_t>::createStore(net,memory,1); // 1 = nr_of_threads
		store = NULL;
	}
	else
	{
		store = StoreCreator<void>::createStore(net,memory,1); // 1 = nr_of_threads
		rstore = NULL;
	}
	covStore =NULL;
        // choose a simple property
        p = new StatePredicateProperty(net,spFormula);
            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_off:
		fl = new Firelist(net);
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		break;
	   case stubborn_arg_tarjan:
		if(RT::args.relaxed_given)
		{
			fl = new FirelistStubbornDeadlock(net);
			portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "bound preserving/relaxed/insertion";
			break;
		}
		else
		{
			fl = new FirelistStubbornComputeBound(net,spFormula);
			portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "bound preserving/insertion";
			break;
		}
	    default: 
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "bound preserving/combined";
		fl = new FirelistStubbornComputeBoundCombined(net,spFormula);
            ; 
            }

		if(RT::args.relaxed_given)
		{
			rexploration = new ComputeBoundExplorationRelaxed(net);
			exploration = NULL;
		}
		else
		{
			exploration = new ComputeBoundExploration(net);
			rexploration = NULL;
		}
    ns = NetState::createNetStateFromInitial(net);

	// compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = NULL;
	SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
	if(store) store = reinterpret_cast<SymmetryStore<void> *>(store)->setGeneratingSet(SymmetryCalculator::G);
	if(rstore) rstore = reinterpret_cast<SymmetryStore<statenumber_t> *>(rstore)->setGeneratingSet(SymmetryCalculator::G);
        delete SC;
    }
    goStatus = true;

	    if(exploration)
	    {
		    resultvalue = reinterpret_cast<ComputeBoundExploration*>(exploration)->depth_first_num(*p, *ns, *store, *fl, 1); // 1 = nr_of_threads
	    }
	    else
	    {
		    resultvalue = reinterpret_cast<ComputeBoundExplorationRelaxed*>(rexploration)->depth_first(*p, *ns, *rstore, *fl, 1); // 1 = nr_of_threads
	    }
    portfoliomanager::reportNumerical(portfolio_id,resultvalue);
    return result;
}

char * ComputeBoundSearchTask::interpreteResult(ternary_t r)
{
	char * value = new char[1000];
	if(r == TERNARY_TRUE)
	{
		sprintf(value,"The maximum value of the given expression is %d.",resultvalue);
		return value;
	}
	sprintf(value, "The maximum value of the given expression is between %d and %d",resultvalue,exploration -> StructuralBound);
	return value;
}
 
statusrecord * ComputeBoundSearchTask::getStatistics()
{
	statusrecord * result = new statusrecord();
	if(store)
	{
		result -> markings = store->get_number_of_markings();
	}
	else if(rstore)
	{
		result -> markings  = rstore->get_number_of_markings();
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
    else if(rstore)
    {
        result -> calls = rstore->get_number_of_calls();
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

void ComputeBoundSearchTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	ComputeBoundSearchTask * C = new ComputeBoundSearchTask(n,par,f,fid);
	C -> preprocessingfinished = true;
}


statusrecord  * ComputeBoundSearchTask::getStatus()
{
	if(!goStatus)
	{
		return NULL;
	}
	statusrecord * result = new statusrecord;
	result -> markings = store ? store->get_number_of_markings() : (rstore ? rstore->get_number_of_markings() : covStore->get_number_of_markings());
	result -> calls = store ? store->get_number_of_calls() : (rstore ? rstore -> get_number_of_calls() : covStore -> get_number_of_calls());
	result -> upperbound = exploration ? exploration -> StructuralBound : rexploration -> StructuralBound;
	result -> lowerbound = exploration ? exploration -> result : rexploration -> result;
        return result;
}

void ComputeBoundSearchTask::derail()
{
	memory->stop = true;
}

Task * ComputeBoundSearchTask::copy()
{
	return new ComputeBoundSearchTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
