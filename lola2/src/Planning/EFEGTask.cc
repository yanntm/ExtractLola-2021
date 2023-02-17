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
#include <Planning/InvariantTask.h>
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
#include <Planning/EFEGTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/EFEGExploration.h>
#include <Formula/StatePredicate/MagicNumber.h>



/*!
\brief the verification task

This class wraps the EFEG check by statespace exploration

*/


EFEGTask::EFEGTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
    parent = par;
    formula = f;
    formula_id = fid;
    result = TERNARY_UNKNOWN;
	memory = new Mara();
    taskname = deconst("state space /EFEG");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EFEG,EXCL_MEM_TASK,memory);
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

EFEGTask::~EFEGTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete p;
    //delete spFormula;
#endif
}

ternary_t EFEGTask::getResult()
{

        // choose a store
	store = StoreCreator<int>::createStore(net,memory,1);


    // choose a simple property
            p = new StatePredicateProperty(net,spFormula);
      bool * visible = new bool[net->Card[TR]];
	memset(visible,0,net->Card[TR] * sizeof(bool));
	p -> predicate -> setNondestroying(visible);
                int vis = 0;
                for(arrayindex_t i = 0; i < net->Card[TR];i++)
                {
                        if(visible[i]) vis++;
                }
                portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;

            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_deletion:
	    case stubborn_arg_tarjan:
	    case stubborn_arg_combined:
	    case stubborn_arg_both:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "reachability preserving/insertion";
                flnotphi = new FirelistStubbornStatePredicate(net,spFormula);
		flnotphi -> visible = visible;
		flphi = new FirelistStubbornLTLTarjan(net);
		flphi -> visible = visible;
		break;
	    case stubborn_arg_off:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		flnotphi = new Firelist(net);
		flphi = new Firelist(net);
		break;
	    default: assert(false); // exhaustive enumeration
            }

    // set the correct exploration algorithm
	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = 1;
                exploration = new EFEGExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    // quick check based on magic number

    // compute symmetries
    if (RT::args.symmetry_given && RT::args.search_arg != search_arg_sweepline)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	if(store) store = reinterpret_cast<SymmetryStore<int>*>(store)->setGeneratingSet(SymmetryCalculator::G);
    }

    goStatus = true; // start reporting progress

    bool_result = exploration->depth_first(*p, *ns, *store, *flnotphi,* flphi, number_of_threads);


    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    else
    {
	result = TERNARY_FALSE;
    }
	portfoliomanager::report(portfolio_id,result);
    return result;
}

char * EFEGTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is possibly preserved from a reachable marking.");
    case TERNARY_FALSE:return deconst( "The predicate is not possibly preserved from any reachable marking.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be possibly preserved from reachable markings.");
	default: assert(false);
    }
	return NULL;
}
 
Path EFEGTask::getWitnessPath()
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

capacity_t *EFEGTask::getMarking()
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

statusrecord * EFEGTask::getStatistics()
{
	statusrecord * result = new statusrecord();
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
        result->calls = store->get_number_of_calls();
    }
    else
    {
	assert(false);
    }
	return result;
} 

void EFEGTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
        if(!RT::args.quickchecks_given)
        {
                EFEGTask * E = new EFEGTask(n,par,f,fid);
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
        //   Quickcheck(suff)        EFEGTask         Quickcheck (necc)
        //       |ag1                                            | ef1
        //   Invariant                                         StateEq
        //       |ag2
        //   Stateequation
        //
	int agg = portfoliomanager::addTask(n,NULL,par,fid,FORMULA_EFEG,AGGREGATE_TASK);

        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EFEGTask * E = new EFEGTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;

        kc::tFormula agformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        agformula = agformula->rewrite(kc::efeg_to_ag);
	pthread_mutex_unlock(&kimwitu_mutex);
        agformula -> id = NULL;
        int agid = portfoliomanager::addFormula(agformula);
        int qc_suff = portfoliomanager::addTask(n,NULL,agg,agid,FORMULA_INVARIANT,SUFFICIENT_QUICKCHECK_TASK);

        int ag1id = portfoliomanager::addFormula(agformula);
        Task * inv = new InvariantTask(n,qc_suff,agformula,ag1id);
	inv -> preprocessingfinished = true;

        int ag2id = portfoliomanager::addFormula(agformula);
        Task * stateeqsuff = new StateEquationTask(n,inv->portfolio_id, agformula,ag2id);
	stateeqsuff -> preprocessingfinished = true;

        kc::tFormula efformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        efformula = efformula->rewrite(kc::efeg_to_ef);
	pthread_mutex_unlock(&kimwitu_mutex);
        efformula -> id =NULL;
        int efid = portfoliomanager::addFormula(efformula);
        int qc_ef = portfoliomanager::addTask(n,NULL, agg, efid, FORMULA_REACHABLE,NECESSARY_QUICKCHECK_TASK);

        int ef1id = portfoliomanager::addFormula(efformula);
        Task * stateeqnecc = new StateEquationTask(n,qc_ef, efformula, ef1id);
	stateeqnecc -> preprocessingfinished = true;
}


statusrecord * EFEGTask::getStatus()
{
	if(!goStatus)
	{
		return NULL; // do not report anything since symmetry calculation is still under way
	}
	statusrecord * result = new statusrecord();
	result -> markings = store->get_number_of_markings();
	result -> calls = store -> get_number_of_calls();
        return result;
}

void EFEGTask::derail()
{
	memory->stop = true;
}

Task * EFEGTask::copy()
{
	return new EFEGTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}

