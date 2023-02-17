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
#include <Planning/EUTask.h>
#include <Planning/StateEquationTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornEU.h>
#include <Exploration/SearchStack.h>
#include <Formula/StatePredicate/MagicNumber.h>

/*!
\brief the verification task

This class executes the EGEF check by statespace exploration

*/


EUTask::EUTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
    net = n;
    parent = par;
    formula = f;
    formula_id = fid;
    result = TERNARY_UNKNOWN;
    memory  = new Mara();
    taskname = deconst("state space /EU");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EU,EXCL_MEM_TASK,memory);
    goStatus = false;
    // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
    kc::tFormula FPhi = formula;
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

EUTask::~EUTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    //delete spFormula;
#endif
}

ternary_t EUTask::getResult()
{

    // choose a store
    store = StoreCreator<void>::createStore(net,memory,1);


    // choose a simple property
    phi = new StatePredicateProperty(net,phiFormula);
    psi = new StatePredicateProperty(net,psiFormula);
    bool * visible = new bool[net->Card[TR]];
   memset(visible,0,net->Card[TR] * sizeof(bool));
    phi->predicate->setNondestroying(visible);
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
        portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "eu preserving";
        fl = new FirelistStubbornEU(net,phiFormula, psiFormula);
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
    exploration = new EUExploration(net);
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

char * EUTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("There is a path where Predicate Phi holds until Predicate Psi.");
    case TERNARY_FALSE: return deconst( "There is no path where Predicate Phi holds until Predicate Psi.");
    case TERNARY_UNKNOWN: return deconst( "There may or may not be a path where Predicate Phi holds until Predicate Psi.");
    default: assert(false);
    }
	return NULL;
}

Path EUTask::getWitnessPath()
{
    // simple property
    return phi->path();
}

capacity_t *EUTask::getMarking()
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

statusrecord * EUTask::getStatistics()
{
	statusrecord * result = new statusrecord();
    if (store)
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

void EUTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
    if (!RT::args.quickchecks_given)
    {
        EUTask * E = new EUTask(n,par,f,fid);
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
        //   Quickcheck(necc)         EUTask
        //       |ef1
        //   Stateequation
        //
        int agg = portfoliomanager::addTask(n,NULL, par, fid,FORMULA_EU, AGGREGATE_TASK,NULL);

        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EUTask * E = new EUTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;

        kc::tFormula efformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        efformula = efformula->rewrite(kc::eu_to_ef);
	pthread_mutex_unlock(&kimwitu_mutex);
        efformula -> id = NULL;
        int efid = portfoliomanager::addFormula(efformula);
        int qc_necc = portfoliomanager::addTask(n,NULL,agg,efid,FORMULA_REACHABLE,NECESSARY_QUICKCHECK_TASK);

        int ef1id = portfoliomanager::addFormula(efformula);
        Task * stateeqnecc = new StateEquationTask(n,qc_necc, efformula,ef1id);
	stateeqnecc -> preprocessingfinished = true;
}

statusrecord *EUTask::getStatus()
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

void EUTask::derail()
{
	memory->stop = true;
}

Task * EUTask::copy()
{
	return new EUTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
