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
#include <Planning/EGTask.h>
#include <Planning/StoreCreator.h>
#include <Planning/ERTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/SearchStack.h>
#include <Formula/StatePredicate/MagicNumber.h>

/*!
\brief the verification task

This class executes the ER check by statespace exploration

*/


ERTask::ERTask(Petrinet * n,int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
    net = n;
    parent = par;
    formula = f;
    formula_id = fid;
    result = TERNARY_UNKNOWN;
    memory = new Mara();
    taskname = deconst("state space /ER");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_ER,EXCL_MEM_TASK,memory);
    goStatus = false;
    // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
    kc::tFormula FPhi = reinterpret_cast<kc::tFormula>(formula->copy(true));
    kc::tFormula FPsi = reinterpret_cast<kc::tFormula>(formula->copy(true));
    FPhi = FPhi->rewrite(kc::singletemporal);
    FPsi = FPsi->rewrite(kc::singletemporal1);
    FPhi = FPhi->rewrite(kc::simpleneg);
    FPsi = FPsi->rewrite(kc::simpleneg);
    FPhi = FPhi->rewrite(kc::booleanlists);
    FPsi = FPsi->rewrite(kc::booleanlists);
Petrinet::InitialNet = net;

    unparsed.clear();
    FPhi->unparse(myprinter, kc::internal);
    FPsi->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
    phiFormula = FPhi->formula;
    psiFormula = FPsi->formula;

}

ERTask::~ERTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    //delete spFormula;
#endif
}

ternary_t ERTask::getResult()
{

    // choose a store
    store = StoreCreator<int>::createStore(net,memory,1);


    // choose a simple property
    phi = new StatePredicateProperty(net,phiFormula);
    psi = new StatePredicateProperty(net,psiFormula);
    bool * visible = new bool[net->Card[TR]];
    memset(visible,0,net->Card[TR] * sizeof(bool));
    phi->predicate->setNondestroying(visible);
    psi->predicate->setNondestroying(visible);
            int vis = 0;
            for (arrayindex_t i = 0; i < net->Card[TR]; i++)
            {
                if (visible[i])
                    vis++;
            }
            portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;

    switch (RT::args.stubborn_arg)
    {
    case stubborn_arg_deletion:
    case stubborn_arg_tarjan:
    case stubborn_arg_combined:
    case stubborn_arg_both:
        portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "ltl";
        fl = new FirelistStubbornLTLTarjan(net);
        break;
    case stubborn_arg_off:
        portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
        fl = new Firelist(net);
        break;
    default:
        assert(false); // exhaustive enumeration
    }
	fl -> visible = visible;

    // set the correct exploration algorithm
    portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = 1;
    exploration = new ERExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    // quick check based on magic number

    goStatus = true; // start reporting progress

    bool_result = exploration->depth_first(*phi, *psi, *ns, *store, *fl, number_of_threads);

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

char * ERTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("There is a path where Predicate Phi releases Predicate Psi.");
    case TERNARY_FALSE: return deconst( "There is no path where Predicate Phi releases Predicate Psi.");
    case TERNARY_UNKNOWN: return deconst( "There may or may not be a path where Predicate Phi releases Predicate Psi.");
    default: assert(false);
    }
	return NULL;
}

Path ERTask::getWitnessPath()
{
    // simple property
    return phi->path();
}

capacity_t *ERTask::getMarking()
{
    // we only provide witness states for simple properties where we found
    // a result
    if (phi and phi->value)
    {
        return ns->Current;
    }
    else
    {
        return NULL;
    }
}

statusrecord * ERTask::getStatistics()
{
	statusrecord * result = new statusrecord();
    if (store)
    {
        result->markings = store->get_number_of_markings();
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

void ERTask::buildTask(Petrinet * n,int par, tFormula f, int fid)
{
        if(!RT::args.quickchecks_given)
        {
		ERTask * E = new ERTask(n,par,f,fid);
		E -> preprocessingfinished = true;
                return;
        }
        // build real portfolio:
        //
        //                              |f
        //                       Aggregate_task
        //                              |
        //       +----------------------+
        //       |agformula             |liveformula
        //   Quickcheck(suff)         ERTask
        //       |ag1
        //   Invariant
        //       |ag2
        //   Stateequation
        //
        int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_ER,AGGREGATE_TASK,NULL);        
        kc::tFormula agformula = portfoliomanager::copyFormula(f);
	kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        ERTask * E = new ERTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;

	pthread_mutex_lock(&kimwitu_mutex);
        agformula = agformula->rewrite(kc::er_to_ag);
	pthread_mutex_unlock(&kimwitu_mutex);
        agformula -> id = NULL;
        int agid = portfoliomanager::addFormula(agformula);
        int qc_suff = portfoliomanager::addTask(n,NULL,agg,agid,FORMULA_INVARIANT,SUFFICIENT_QUICKCHECK_TASK,NULL);

        int ag1id = portfoliomanager::addFormula(agformula);
        Task * inv = new InvariantTask(n,qc_suff,agformula,ag1id);
	inv -> preprocessingfinished = true;

        int ag2id = portfoliomanager::addFormula(agformula);
        Task * stateeqsuff = new StateEquationTask(n,inv->portfolio_id, agformula,ag2id);
	stateeqsuff -> preprocessingfinished = true;

}

statusrecord *ERTask::getStatus()
{
    if (!goStatus)
    {
        return NULL; // do not report anything since symmetry calculation is still under way
    }
	statusrecord * result = new statusrecord();
        result -> markings = store->get_number_of_markings();
	result -> calls = store->get_number_of_calls();
    return result;
}

void ERTask::derail()
{
	memory->stop = true;
}

Task * ERTask::copy()
{
	return new ERTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
