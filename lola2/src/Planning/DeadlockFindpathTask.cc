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

#include <Exploration/ChooseTransition.h>
#include <Exploration/ChooseTransitionRandomly.h>
#include <Exploration/ChooseTransitionHashDriven.h>
#include <Exploration/DeadlockExploration.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornDeadlock.h>
#include <Core/Dimensions.h>
#include <Core/Handlers.h>
#include <Stores/Store.h>
#include <Core/Runtime.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/DeadlockFindpathTask.h>
#include <Planning/StoreCreator.h>
#include <Net/NetState.h>
#include <Memory/Mara.h>

class SimpleProperty;

/*!
\brief the verification task

This class wraps the stateless search for a deadlock

*/

DeadlockFindpathTask::DeadlockFindpathTask(Petrinet * n,int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f; 
	formula_id = fid;
	memory = new Mara();
	taskname = deconst("findpath");
	portfolio_id = portfoliomanager::addTask(n,this, par, fid, FORMULA_DEADLOCK,FINDPATH_TASK,memory);
	result = TERNARY_UNKNOWN;
    goStatus = false;

}

DeadlockFindpathTask::~DeadlockFindpathTask()
{
	delete ns;
	delete p;
	delete fl;
	delete store;
	delete exploration;
}

ternary_t DeadlockFindpathTask::getResult()
{

	store = new EmptyStore<void>(net,memory,number_of_threads);
 	p = new DeadlockExploration(net);

	switch(RT::args.stubborn_arg)
	{
	case stubborn_arg_deletion:
	case stubborn_arg_combined:
	case stubborn_arg_tarjan:
	case stubborn_arg_both:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "deadlock preserving/insertion";
            fl = new FirelistStubbornDeadlock(net);
	    break;
	case stubborn_arg_off:
	    fl = new Firelist(net);
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
	    break;
	default:
	    assert(false); // case consideration exhaustive
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
    if (RT::args.retrylimit_arg == 0)
    {
	portfoliomanager::taskjson[portfolio_id]["configuration"]["retry_limit"] = JSON();
    }
    else
    {
	portfoliomanager::taskjson[portfolio_id]["configuration"]["retry_limit"] = RT::args.retrylimit_arg;
    }
	portfoliomanager::taskjson[portfolio_id]["configuration"]["depthlimit"] = RT::args.depthlimit_arg;

    // added a scope to allow a local definition of choose
    {
	ChooseTransition *choose = NULL;
        choose = new ChooseTransitionRandomly(net);
	goStatus = true;
	exploration -> attempts =  RT::args.retrylimit_arg;
	exploration -> maxdepth =  RT::args.depthlimit_arg;
	bool_result = exploration->find_path(*p, *ns, *fl, *((EmptyStore<void> *)store), *choose);
	delete choose;
    }
// temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    else
    {
	result = TERNARY_UNKNOWN;
    }

    portfoliomanager::report(portfolio_id,result);
    return result;
}
char * DeadlockFindpathTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The net has deadlock(s).");
    case TERNARY_FALSE: assert(false);
    case TERNARY_UNKNOWN: return deconst("The net may or may not have deadlocks.");
	default: assert(false);
    }
	return NULL;
} 

Path DeadlockFindpathTask::getWitnessPath()
{
	return exploration->path();
}

capacity_t *DeadlockFindpathTask::getMarking()
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

statusrecord * DeadlockFindpathTask::getStatistics()
{
	statusrecord * result = new statusrecord;
	result -> markings = store->get_number_of_markings();
        result -> calls = store->get_number_of_calls();
	result -> tries = store -> tries;
	return result;
}



void DeadlockFindpathTask::buildTask(Petrinet * n,int par, tFormula  f, int fid)
{
	DeadlockFindpathTask * D = new DeadlockFindpathTask(n,par,f,fid);
	D -> preprocessingfinished = true;
}

statusrecord * DeadlockFindpathTask::getStatus()
{
	statusrecord * result = new statusrecord;
	result -> markings = store->get_number_of_markings();
        result -> calls = store->get_number_of_calls();
	result -> tries = store -> tries;
	return result;
}

void DeadlockFindpathTask::derail()
{
	if(exploration)
	{
		exploration -> attempts = 1;
  		exploration -> maxdepth = 1;
		
	}
}

Task * DeadlockFindpathTask::copy()
{
	return new DeadlockFindpathTask(net,parent,portfoliomanager::copyFormula(formula), formula_id);
}
