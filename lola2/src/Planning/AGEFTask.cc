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
#include <Planning/AGEFTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/FirelistStubbornTsccAlwaysUpset.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/FirelistStubbornTsccUpset.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>
#include <Exploration/TSCCExploration.h>



/*!
\brief the verification task

This class wraps the liveness check by statespace exploration

*/

class TSCCExplorationAGEF;
class TSCCExplorationAGEFRelaxed;

// TODO update JSON to the new format

AGEFTask::AGEFTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	result = TERNARY_UNKNOWN;
	formula_id = fid;
	memory = new Mara();
        taskname = deconst("tscc_search");
	portfolio_id = portfoliomanager::addTask(n,this, par,fid, FORMULA_LIVENESS,EXCL_MEM_TASK,memory);
	
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

AGEFTask::~AGEFTask()
{
#ifndef USE_PERFORMANCE
    //delete ns;
    //delete store;
    //delete p;
    //delete spFormula;
    //delete fl;
    //delete exploration;
#endif
}

ternary_t AGEFTask::getResult()
{

        // choose a store
	store = StoreCreator<statenumber_t>::createStore(net,memory,number_of_threads);

    	// choose a simple property
        p = new StatePredicateProperty(net,spFormula);
        if(RT::args.stubborn_arg==stubborn_arg_off)
        {
portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
	}
	else
	{
	if(RT::args.relaxed_given)
	{
        bool * visible = new bool[net->Card[TR]];
	memset(visible,0, net->Card[TR] * sizeof(bool));
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
		exploration = new TSCCExplorationAGEFRelaxed(net);
	}
	else
	{
		exploration = new TSCCExplorationAGEF(net);
	}

    ns = NetState::createNetStateFromInitial(net);

	// compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = NULL;
	SC = new SymmetryCalculator(net,spFormula);
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

char * AGEFTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is live.");
    case TERNARY_FALSE: return deconst("The predicate is not live.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be live.");
	default: assert(false);
    }
return NULL;
}
 
Path AGEFTask::getWitnessPath()
{
        // simple property
        return p->path();
}

capacity_t *AGEFTask::getMarking()
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

statusrecord * AGEFTask::getStatistics()
{
	statusrecord * result = new statusrecord;
	if(store)
	{
		result -> markings = store->get_number_of_markings();
	}
	else
	{
		assert(false);
	}
    if (store)
    {
        result -> calls = store->get_number_of_calls();
    }
    else
    {
	assert(false);
    }
return result;
} 

void AGEFTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	if(!RT::args.quickchecks_given)
	{
		AGEFTask * A = new AGEFTask(n,par, f, fid);
		A -> preprocessingfinished = true;
		return;
	}
	// build real portfolio:
	//
	//				|f
	//                       Aggregate_task
	//                              |
	//       +----------------------+------------------------+
	//       |agformula             |liveformula             | efformula
	//   Quickcheck(suff)        AGEFTask         Quickcheck (necc)
	//       |ag1                                            | ef1
	//   Invariant                                         StateEq
	//       |ag2
	//   Stateequation
	//
	

	int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_LIVENESS,AGGREGATE_TASK);

	kc::tFormula liveformula = f;
	kc::tFormula agformula = portfoliomanager::copyFormula(f);
	kc::tFormula efformula = portfoliomanager::copyFormula(f);

	int live_id = portfoliomanager::addFormula(liveformula);
	AGEFTask * A = new AGEFTask(n,agg,liveformula,live_id);
	A -> preprocessingfinished = true;

	pthread_mutex_lock(&kimwitu_mutex);
	agformula = agformula->rewrite(kc::agef_to_ag);
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

	pthread_mutex_lock(&kimwitu_mutex);
	efformula = efformula->rewrite(kc::agef_to_ef);
	pthread_mutex_unlock(&kimwitu_mutex);
	efformula -> id =NULL;
	int efid = portfoliomanager::addFormula(efformula);
	int qc_ef = portfoliomanager::addTask(n,NULL, agg, efid, FORMULA_REACHABLE,NECESSARY_QUICKCHECK_TASK);

	int ef1id = portfoliomanager::addFormula(efformula);
	Task * stateeqnecc = new StateEquationTask(n,qc_ef, efformula, ef1id);
	stateeqnecc->preprocessingfinished = true;
}


statusrecord * AGEFTask::getStatus()
{
	statusrecord * result = new statusrecord;
        result -> text  = NULL;
	result -> markings = store->get_number_of_markings(); 
	result -> calls = store -> get_number_of_calls();
        return result;
}

void AGEFTask::derail()
{
	memory -> stop = true;
}

Task * AGEFTask::copy()
{
	return new AGEFTask(net,parent,portfoliomanager::copyFormula(formula) , formula_id);
}
