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


#include <Planning/InvariantTask.h>
#include <Planning/ReachabilityTask.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <CoverGraph/CoverPayload.h>
#include <Symmetry/Constraints.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/StateEquationTask.h>
#include <Planning/StoreCreator.h>
#include <Planning/EFAGEFTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/FirelistStubbornTsccAlwaysUpset.h>
#include <Exploration/FirelistStubbornTsccUpset.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>
#include <Exploration/TSCCExploration.h>

// \todo: exploration by coverability anaylsis


/*!
\brief the verification task

This class wraps the possible liveness check by statespace exploration

*/


EFAGEFTask::EFAGEFTask(Petrinet * n, int par, tFormula  f, int fid)
{
	
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	memory = new Mara();
	taskname = deconst("tscc_search");
	portfolio_id = portfoliomanager::addTask(n,this, par,fid,FORMULA_EFAGEF,EXCL_MEM_TASK,memory);
	result = TERNARY_UNKNOWN;
        // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
        formula = formula->rewrite(kc::singletemporal);
        formula = formula->rewrite(kc::simpleneg);
        formula = formula->rewrite(kc::booleanlists);

	Petrinet::InitialNet = net;
        formula->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
	goStatus = false;

}

EFAGEFTask::~EFAGEFTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete covStore;
    delete p;
    delete spFormula;
    delete fl;
#endif
}

ternary_t EFAGEFTask::getResult()
{
    // select an appropriate store
    store = NULL;
    covStore = NULL;
    switch (RT::args.search_arg)
    {
    case search_arg_covergraph:
    {
        portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "cover";
        if (RT::args.encoder_arg != encoder_arg_fullcopy)
        {
            RT::rep->status("warning: encoder does not fully support coverability graphs");
        }
        covStore = StoreCreator<CoverPayload>::createStore(net,memory,number_of_threads);
        break;
    }
    default:
    {
        portfoliomanager::taskjson[portfolio_id]["configuration"]["method"] = "dfs/tscc";

        // choose a store
	store = StoreCreator<statenumber_t>::createStore(net,memory,number_of_threads);

        break;
    }
    }
    // choose a simple property
            p = new StatePredicateProperty(net,spFormula);
    // choose a firelist generator
	    if(RT::args.stubborn_arg == stubborn_arg_off)
            {
portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
	    }
	    else
            {
		if(RT::args.relaxed_given)
		{
        bool * visible = new bool[net->Card[TR]];
	memset(visible,0,net->Card[TR] * sizeof(bool));
        reinterpret_cast<StatePredicateProperty *>(p) -> predicate -> setNondestroying(visible);
                int vis = 0;
                for(arrayindex_t i = 0; i < net->Card[TR];i++)
                {
                        if(visible[i]) vis++;
                }
                portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;

			fl = new FirelistStubbornLTLTarjan(net);
			fl -> visible = visible;
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "tscc preserving/relaxed";
		}
		else
		{
			fl = new FirelistStubbornTsccAlwaysUpset(net,p);
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "tscc preserving/always up";
		}
            }
		if(RT::args.relaxed_given)
		{
		    exploration = new TSCCExplorationEFAGEFRelaxed(net);
		}
		else
		{
		    exploration = new TSCCExplorationEGAGEF(net);
		}
    ns = NetState::createNetStateFromInitial(net);

	// compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
    }

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);
    goStatus = true;
    bool_result = exploration->depth_first(*p, *ns, *store, *fl, number_of_threads);

    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    portfoliomanager::report(portfolio_id,result);
    return result;
}

char * EFAGEFTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is possibly live.");
    case TERNARY_FALSE: return deconst( "The predicate is not possibly live.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be possibly live.");
	default: assert(false);
    }
	return NULL;

}
 
Path EFAGEFTask::getWitnessPath()
{
        // simple property
        return p->path();
}

capacity_t *EFAGEFTask::getMarking()
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

statusrecord * EFAGEFTask::getStatistics()
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

void EFAGEFTask::buildTask(Petrinet * n, int par, tFormula f, int fid)
{
	if(!RT::args.quickchecks_given)
        {
                EFAGEFTask * E = new EFAGEFTask(n,par,f,fid);
		E -> preprocessingfinished = true;
		return;
        }
        // build real portfolio:
        //
        //                              |f
        //                       Aggregate_task
        //                              |
        //       +----------------------+------------------------+
        //       |agformula             |liveformula             | efformula
        //   Quickcheck(suff)        EFAGEFTask        Quickcheck (necc)
        //       |ag1                                            | ef1
        //   Invariant                                         StateEq
        //       |ag2
        //   Stateequation
        //

        int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_EFAGEF,AGGREGATE_TASK);

        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EFAGEFTask * E = new EFAGEFTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;

        kc::tFormula agformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        agformula = agformula->rewrite(kc::efagef_to_ag);
	pthread_mutex_unlock(&kimwitu_mutex);
        agformula -> id = NULL;
        int agid = portfoliomanager::addFormula(agformula);
        int qc_suff = portfoliomanager::addTask(n,NULL,agg, agid,FORMULA_INVARIANT,SUFFICIENT_QUICKCHECK_TASK);

        int ag1id = portfoliomanager::addFormula(agformula);
        Task * inv = new InvariantTask(n,qc_suff,agformula,ag1id);
	inv -> preprocessingfinished = true;

        int ag2id = portfoliomanager::addFormula(agformula);
        Task * stateeqsuff = new StateEquationTask(n,inv->portfolio_id, agformula,ag2id);
	stateeqsuff -> preprocessingfinished = true;

        kc::tFormula efformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        efformula = efformula->rewrite(kc::efagef_to_ef);
	pthread_mutex_unlock(&kimwitu_mutex);
        efformula -> id =NULL;
        int efid = portfoliomanager::addFormula(efformula);
        int qc_ef = portfoliomanager::addTask(n,NULL, agg, efid, FORMULA_REACHABLE, NECESSARY_QUICKCHECK_TASK);

        int ef1id = portfoliomanager::addFormula(efformula);
        Task * stateeqnecc = new StateEquationTask(n,qc_ef, efformula, ef1id);
	stateeqnecc -> preprocessingfinished = true;

}


statusrecord * EFAGEFTask::getStatus()
{
	statusrecord * result = new statusrecord();
        result -> markings = store ? store->get_number_of_markings() : covStore->get_number_of_markings();
	result -> calls = store->get_number_of_calls();
        return result;
}

void EFAGEFTask::derail()
{
	memory->stop = true;
}

Task * EFAGEFTask::copy()
{
	return new EFAGEFTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}

