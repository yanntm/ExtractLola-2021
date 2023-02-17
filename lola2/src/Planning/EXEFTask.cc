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


#include <Planning/ReachabilityTask.h>
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
#include <Planning/EXEFTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/EXEFExploration.h>
#include <Formula/StatePredicate/MagicNumber.h>



/*!
\brief the verification task

This class wraps the EX reachability check by statespace exploration

*/


EXEFTask::EXEFTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
     net = n;
    parent = par;
    formula = f;
    formula_id = fid;
     result = TERNARY_UNKNOWN;
    memory = new Mara();
    taskname = deconst("state space /EXEF");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EXEF,EXCL_MEM_TASK,memory);
    goStatus = false;
    // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
    formula = formula->rewrite(kc::singletemporal);
    formula = formula->rewrite(kc::simpleneg);
    formula = formula->rewrite(kc::booleanlists);
Petrinet::InitialNet = net;

	unparsed.clear();
        formula->unparse(myprinter, kc::internal);      
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;

}

EXEFTask::~EXEFTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete covStore;
    delete p;
    //delete spFormula;
#endif
}

ternary_t EXEFTask::getResult()
{

        // choose a store
	store = StoreCreator<void>::createStore(net,memory,number_of_threads);
	covStore = NULL;


    // choose a simple property
            p = new StatePredicateProperty(net,spFormula);
            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_deletion:
	    case stubborn_arg_tarjan:
	    case stubborn_arg_combined:
	    case stubborn_arg_both:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "reachability preserving/insertion";
                fl = new FirelistStubbornStatePredicate(net,spFormula);
		break;
	    case stubborn_arg_off:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
		break;
	    default: assert(false); // exhaustive enumeration
            }

    // set the correct exploration algorithm
	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = 1;
                exploration = new EXEFExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);

    // compute symmetries
    if (RT::args.symmetry_given && RT::args.search_arg != search_arg_sweepline)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	if(store) store = reinterpret_cast<SymmetryStore<void>*>(store)->setGeneratingSet(SymmetryCalculator::G);
	if(covStore) covStore = reinterpret_cast<SymmetryStore<CoverPayload>*>(covStore)->setGeneratingSet(SymmetryCalculator::G);
    }

    goStatus = true; // start reporting progress

	    bool_result = exploration->depth_first(*p, *ns, *store, *fl, number_of_threads);


    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    // if the Bloom store did not find anything, the result is unknown
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

char * EXEFTask::interpreteResult(ternary_t result)
{
	
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is reachable from some successor.");
    case TERNARY_FALSE: return deconst("The predicate is unreachable from any successor.");
    case TERNARY_UNKNOWN: return deconst("The predicate may or may not be reachable from successors.");
	default: assert(false);
    }

	return NULL;
}
 
Path EXEFTask::getWitnessPath()
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

capacity_t *EXEFTask::getMarking()
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

statusrecord * EXEFTask::getStatistics()
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

void  EXEFTask::buildTask(Petrinet * n,int par, tFormula  f, int fid)
{
        if(!RT::args.quickchecks_given)
        {
                EXEFTask *E = new EXEFTask(n,par,f,fid);
		E -> preprocessingfinished = true;
		return;
        }
        // build real portfolio:
        // 
        //                              |f
        //                       Aggregate_task
        //                              |
        //       +----------------------+
        //       |efformula             |liveformula
        //   Quickcheck(necc)         EXEFTask
        //       |ef1
        //   Stateequation
        //   
        int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_EXEF,AGGREGATE_TASK);
        
        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EXEFTask * E = new EXEFTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;
        
        kc::tFormula efformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        efformula = efformula->rewrite(kc::exef_to_ef);
	pthread_mutex_unlock(&kimwitu_mutex);
        efformula -> id = NULL;
        int efid = portfoliomanager::addFormula(efformula);
        int qc_necc = portfoliomanager::addTask(n,NULL,agg, efid,FORMULA_REACHABLE,NECESSARY_QUICKCHECK_TASK);
        
        int ef1id = portfoliomanager::addFormula(efformula);
        Task * stateeqnecc = new StateEquationTask(n,qc_necc, efformula,ef1id);
	stateeqnecc->preprocessingfinished = true;

}


statusrecord  * EXEFTask::getStatus()
{
	if(!goStatus)
	{
		return NULL; // do not report anything since symmetry calculation is still under way
	}
	statusrecord * result = new statusrecord();
	result -> markings = store->get_number_of_markings();
	result -> calls = store->get_number_of_calls();
        return result;
}

void EXEFTask::derail()
{
	memory -> stop = true;
}

Task * EXEFTask::copy()
{
	return new EXEFTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
