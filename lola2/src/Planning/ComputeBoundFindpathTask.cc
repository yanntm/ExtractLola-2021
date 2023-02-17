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
#include <Planning/ComputeBoundFindpathTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornComputeBound.h>
#include <Exploration/FirelistStubbornComputeBoundCombined.h>
#include <Exploration/ComputeBoundExploration.h>
#include <Exploration/ComputeBoundExplorationRelaxed.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/ChooseTransitionHashDriven.h>
#include <Exploration/ChooseTransitionRandomly.h>



/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/


ComputeBoundFindpathTask::ComputeBoundFindpathTask(Petrinet * n,int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula_id = fid;
	formula = f;
	memory = new Mara();
	taskname = deconst("findpath");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid, FORMULA_BOUND,FINDPATH_TASK);
	portfoliomanager::isnumerical[portfolio_id] = true;
	portfoliomanager::isnumerical[parent] = true;
	goStatus = false;
        // extract state predicate from formula
	assert(formula);

	pthread_mutex_lock(&kimwitu_mutex);
        formula->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
	exploration = NULL;
	// compute structural bound to make it available to all tasks
	

}

ComputeBoundFindpathTask::~ComputeBoundFindpathTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete p;
    delete spFormula;
    delete fl;
    delete exploration;
    delete rexploration;
#endif
}

ternary_t ComputeBoundFindpathTask::getResult()
{
        // choose a store
	store = new EmptyStore<void>(net,memory,number_of_threads);
        // choose a simple property
        p = new StatePredicateProperty(net,spFormula);
            switch(RT::args.stubborn_arg)
            {
	    case stubborn_arg_off:
		fl = new Firelist(net);
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
		break;
	   case stubborn_arg_tarjan:
			fl = new FirelistStubbornComputeBound(net,spFormula);
			break;
	    default: 
		fl = new FirelistStubbornComputeBoundCombined(net,spFormula);
            ; 
            }

		exploration = new ComputeBoundExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    goStatus = true;

    bool value = reinterpret_cast<ComputeBoundExploration*>(exploration)->find_path(*p, *ns,  *fl, *store, *(new ChooseTransitionHashDriven(net))); 
    if(value) 
    {
	AtomicStatePredicate * a = (AtomicStatePredicate *) spFormula;
	resultvalue = a -> upper_bound - a -> threshold;
	portfoliomanager::reportNumerical(portfolio_id, resultvalue);
	return TERNARY_TRUE;
    } 
    return TERNARY_UNKNOWN;
}

char * ComputeBoundFindpathTask::interpreteResult(ternary_t r)
{
	char * value = new char[1000];
	if(r == TERNARY_TRUE)
	{
		sprintf(value,"The maximum value of the given expression is %d.",resultvalue);
		return value;
	}
	sprintf(value, "The maximum value of the given expression is unknown");
	return value;
}
 
statusrecord * ComputeBoundFindpathTask::getStatistics()
{
	statusrecord * result = new statusrecord();
result -> markings = store->get_number_of_markings();
        result -> calls = store->get_number_of_calls();
        result -> tries = store -> tries;
        return result;
} 

void ComputeBoundFindpathTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	ComputeBoundFindpathTask * C = new ComputeBoundFindpathTask(n,par,f,fid);
	C -> preprocessingfinished = true;
}


statusrecord  * ComputeBoundFindpathTask::getStatus()
{
	if(!goStatus)
	{
		return NULL;
	}
	statusrecord * result = new statusrecord();
        result -> tries = store -> tries;
        result -> markings = store -> get_number_of_markings();
        result -> calls = store->get_number_of_calls();
        return result;
}

void ComputeBoundFindpathTask::derail()
{
	if(exploration)
        {
                exploration -> attempts = 1;
                exploration -> maxdepth = 1;
        }

}

Task * ComputeBoundFindpathTask::copy()
{
	return new ComputeBoundFindpathTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
