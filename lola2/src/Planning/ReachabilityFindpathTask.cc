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
#include <Core/Dimensions.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/ReachabilityFindpathTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/ChooseTransitionHashDriven.h>
#include <Exploration/ChooseTransitionRandomly.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>


/*!
\brief the verification task

This class wraps the stateless reachability check

*/

ReachabilityFindpathTask::ReachabilityFindpathTask(Petrinet * n,int par, tFormula  f, int fid)
{
	exploration = NULL;
	preprocessingfinished = false;
	net = n;
	parent = par;
        formula = f;
	formula_id = fid;
        memory = new Mara();
	result = TERNARY_UNKNOWN;
	taskname = deconst("findpath");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,FINDPATH_TASK);
        // extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
        formula = formula->rewrite(kc::singletemporal);
        formula = formula->rewrite(kc::simpleneg);

Petrinet::InitialNet = net;
        formula->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
	goStatus = false;
}

ReachabilityFindpathTask::~ReachabilityFindpathTask()
{
	 delete ns;
    delete store;
    delete p;
    //delete spFormula;
    delete fl;
    delete exploration;
}

ternary_t ReachabilityFindpathTask::getResult()
{

    store = new EmptyStore<void>(net,memory,number_of_threads);
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
	    default:
		assert(false); // exhaustive enumeration
            }

	portfoliomanager::taskjson[portfolio_id]["configuration"]["threads"] = number_of_threads;
     if (number_of_threads == 1)
            {
                exploration = new DFSExploration(net,memory);
            }
            else
            {
                exploration = new ParallelExploration(net,memory);
            }
    ns = NetState::createNetStateFromInitial(net);

	
    bool bool_result;

	//T::data["task"]["findpath"]["depthlimit"]=RT::args.depthlimit_arg;

    // added a scope to allow a local definition of choose
    {
	ChooseTransition *choose = NULL;
	choose = new ChooseTransitionHashDriven(net);
	goStatus = true;
	exploration->attempts = RT::args.retrylimit_arg;
	exploration->maxdepth = RT::args.depthlimit_arg;
	bool_result = exploration->find_path(*p, *ns, *fl, *((EmptyStore<void> *)store), *choose);
	delete choose;
    }
    if(bool_result)
    {
	portfoliomanager::report(portfolio_id,TERNARY_TRUE);
	return TERNARY_TRUE;
    }
    else
    {
	portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
	return TERNARY_UNKNOWN;
    }
}

char * ReachabilityFindpathTask::interpreteResult(ternary_t result)
{
	switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is reachable.");
case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be reachable.");
    default: assert(false);
    }
	return NULL;
}
 
Path ReachabilityFindpathTask::getWitnessPath()
{
	return exploration->path();
}


capacity_t *ReachabilityFindpathTask::getMarking()
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

statusrecord * ReachabilityFindpathTask::getStatistics()
{
	statusrecord * result = new statusrecord();
	result -> markings = store->get_number_of_markings();
	result -> calls = store->get_number_of_calls();
	result -> tries = store -> tries;
	return result;
}
 
void ReachabilityFindpathTask::buildTask(Petrinet * n,int par, tFormula  f, int fid)
{
	ReachabilityFindpathTask * R = new ReachabilityFindpathTask(n,par,f,fid);
	R -> preprocessingfinished = true;
}


statusrecord * ReachabilityFindpathTask::getStatus()
{
	statusrecord * result = new statusrecord();
	result -> tries = store -> tries;
	result -> markings = store -> get_number_of_markings();
	result -> calls = store->get_number_of_calls();
        return result;
}

void ReachabilityFindpathTask::derail()
{
	if(exploration)
	{
		exploration -> attempts = 1;
		exploration -> maxdepth = 1;
	}
}

Task * ReachabilityFindpathTask::copy()
{
	return new ReachabilityFindpathTask(net,parent, portfoliomanager::copyFormula(formula), formula_id);
}
