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
#include <Planning/StoreCreator.h>
#include <Planning/EXEGTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/EXEGExploration.h>
#include <Formula/StatePredicate/MagicNumber.h>



/*!
\brief the verification task

This class wraps the EX EG check by statespace exploration

*/


EXEGTask::EXEGTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
    net = n;
    parent = par;
    formula = f;
    formula_id = fid;
     result = TERNARY_UNKNOWN;
    memory = new Mara();
    taskname = deconst("state space /EXEG");
    portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EXEG,EXCL_MEM_TASK,memory);
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

EXEGTask::~EXEGTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete p;
    //delete spFormula;
#endif
}

ternary_t EXEGTask::getResult()
{

        // choose a store
	store = StoreCreator<bool>::createStore(net,memory,1);


    // choose a simple property
            p = new StatePredicateProperty(net,spFormula);
            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_deletion:
	    case stubborn_arg_tarjan:
	    case stubborn_arg_combined:
	    case stubborn_arg_both:
		{
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "invisibility based";
                fl = new FirelistStubbornLTLTarjan(net);
	        fl-> visible = new bool[net->Card[TR]];
		memset(fl->visible,0,net->Card[TR]*sizeof(bool));
		p -> predicate -> setNondestroying(fl->visible);
		int vis = 0;
		for(arrayindex_t i = 0; i < net->Card[TR];i++)
		{
			if(fl->visible[i]) vis++;
		}
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn visible"] = vis;
		}
		break;
	    case stubborn_arg_off:
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		fl = new Firelist(net);
		break;
		default: ;
            }

    // set the correct exploration algorithm
	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = 1;
                exploration = new EXEGExploration(net);
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
	if(store) store = reinterpret_cast<SymmetryStore<bool>*>(store)->setGeneratingSet(SymmetryCalculator::G);
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

char * EXEGTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is possibly preserved from some successor.");
    case TERNARY_FALSE: return deconst("The predicate is not possibly preserved from any successor.");
    case TERNARY_UNKNOWN: return deconst("The predicate may or may not be possibly preserved from successors.");
	default: assert(false);
    }
return NULL;
}
 
Path EXEGTask::getWitnessPath()
{
        return *(new Path(net));
}

capacity_t *EXEGTask::getMarking()
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

statusrecord * EXEGTask::getStatistics()
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
        result -> calls = store->get_number_of_calls();
    }
    else
    {
	assert(false);
    }
	return result;
} 

void  EXEGTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	if(!RT::args.quickchecks_given)
        {
	
                EXEGTask * E = new EXEGTask(n,par,f,fid);
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
        //   Quickcheck(suff)         EXEGTask
        //       |ag1
        //   Invariant
        //       |ag2
        //   Stateequation
        //
        int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_EXEG,AGGREGATE_TASK);

        kc::tFormula liveformula = f;
        int live_id = portfoliomanager::addFormula(liveformula);
        EXEGTask * E = new EXEGTask(n,agg,liveformula,live_id);
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
	inv ->preprocessingfinished = true;

        int ag2id = portfoliomanager::addFormula(agformula);
        Task * stateeqsuff = new StateEquationTask(n,inv->portfolio_id, agformula,ag2id);
	stateeqsuff -> preprocessingfinished = true;

}


statusrecord * EXEGTask::getStatus()
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

void EXEGTask::derail()
{
	memory -> stop = true;
}

Task * EXEGTask::copy()
{
	return new EXEGTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
