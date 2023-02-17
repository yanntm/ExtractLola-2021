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
#include <Planning/StateEquationTask.h>
#include <Planning/EGTask.h>
#include <Planning/StoreCreator.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/EGExploration.h>
#include <Formula/StatePredicate/MagicNumber.h>



/*!
\brief the verification task

This class implements the dedicated search routine for EG formulas

*/


EGTask::EGTask(Petrinet * n,int par, tFormula  f, int fid)
{
    preprocessingfinished = false;
    net = n;
    parent = par;
    formula = f;
    formula_id = fid;
    result = TERNARY_UNKNOWN;
    memory = new Mara();
    taskname = deconst("state space / EG");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EG,EXCL_MEM_TASK,memory);
    goStatus = false;
    // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
    formula = formula->rewrite(kc::singletemporal);
    formula = formula->rewrite(kc::simpleneg);
    formula = formula->rewrite(kc::booleanlists);

	unparsed.clear();
Petrinet::InitialNet = net;
        formula->unparse(myprinter, kc::internal);      
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
}

EGTask::~EGTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete p;
    //delete spFormula;
#endif
}

ternary_t EGTask::getResult()
{


//RT::rep->status("EG FORMULA %s", spFormula -> toString());
        // choose a store
	store = StoreCreator<bool>::createStore(net,memory,1);
	covStore = NULL;


    // choose a simple property
            p = new StatePredicateProperty(net,spFormula);
            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_off:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
		break;
	    default: // use stubborn sets
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "reachability preserving/insertion";
                fl = new FirelistStubbornLTLTarjan(net);
	if(net -> Card[TR])
	{
		fl -> visible = new bool[net->Card[TR]];
		memset(fl->visible,0,net->Card[TR]*sizeof(bool));
		p -> predicate -> setVisible(fl->visible);
	}
	else
	{
		fl -> visible = NULL;
	}

                int vis = 0;
                for(arrayindex_t i = 0; i < net->Card[TR];i++)
                {
                        if(fl->visible[i]) vis++;
                }
                portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;

            }

	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = 1;
	exploration = new EGExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);

    // compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,spFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	store = reinterpret_cast<SymmetryStore<bool>*>(store)->setGeneratingSet(SymmetryCalculator::G);
    }

    goStatus = true; // start reporting progress

    bool_result = exploration->depth_first(*p, *ns, *store, *fl, number_of_threads);


    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    portfoliomanager::report(portfolio_id,result);
    return result;
}

char * EGTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is possibly preserved.");
    case TERNARY_FALSE: return deconst( "The predicate is not possibly preserved.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be possibly preserved.");
	default: assert(false);
    }
	return NULL;
}
 
Path EGTask::getWitnessPath()
{
        // simple property
        return exploration->ppp;
}

capacity_t *EGTask::getMarking()
{
        return NULL;
}

statusrecord * EGTask::getStatistics()
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

void EGTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	if(!RT::args.quickchecks_given)
	{
		EGTask * E = new EGTask(n,par,f,fid);
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
        //   Quickcheck(suff)         EGTask        
        //       |ag1                               
        //   Invariant                            
        //       |ag2
        //   Stateequation
        //
        int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_EG,AGGREGATE_TASK);

        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EGTask * E = new EGTask(n,agg,liveformula,live_id);
	E -> preprocessingfinished = true;

        kc::tFormula agformula = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
        agformula = agformula->rewrite(kc::eg_to_ag);
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

}


statusrecord * EGTask::getStatus()
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

void EGTask::derail()
{
	memory->stop = true;
}

Task * EGTask::copy()
{
	return new EGTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
