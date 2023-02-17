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
#include <Planning/DisjunctionTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>



/*!
\brief the verification task

This class handles a task with a top level disjunction
*/


DisjunctionTask::~DisjunctionTask()
{
#ifndef USE_PERFORMANCE
    //delete subTask1;
    //delete subTask2;
#endif
}

//void DisjunctionTask::interpreteResult(ternary_t result)
//{
//    switch(result)
//    {
//    case TERNARY_TRUE:
//        RT::rep->status("result: %s", RT::rep->markup(MARKUP_GOOD, "yes").str());
//        RT::data["result"]["value"] = true;
//        RT::data["result"]["produced_by"] = "boolean";
//        RT::rep->status("%s", RT::rep->markup(MARKUP_GOOD, "The Boolean predicate is true.").str());
//        break;
//
//    case TERNARY_FALSE:
//        RT::rep->status("result: %s", RT::rep->markup(MARKUP_BAD, "no").str());
//        RT::data["result"]["value"] = false;
//        RT::data["result"]["produced_by"] = "boolean";
//        RT::rep->status("%s", RT::rep->markup(MARKUP_BAD, "The Boolean predicate is false.").str());
//        break;
//
//    case TERNARY_UNKNOWN:
//        RT::rep->status("result: %s", RT::rep->markup(MARKUP_WARNING, "unknown").str());
//        RT::data["result"]["value"] = JSON::null;
//        RT::data["result"]["produced_by"] = "boolean";
//        RT::rep->status("%s", RT::rep->markup(MARKUP_WARNING, "The Boolean predicate may be true or false.").str());
//        break;
//	default: assert(false);
//    }
//}
// 
void DisjunctionTask::buildTask(Petrinet * n, int par, tFormula f, int fid)
{
        int disj_id = portfoliomanager::addTask(n,NULL,par,fid,FORMULA_DISJUNCTION,DISJUNCTION_TASK,NULL);
        tFormula  left = portfoliomanager::copyFormula(f);
        tFormula  right = portfoliomanager::copyFormula(f);
        left = StatePredicateFormula((kc::tStatePredicate ) left -> subphylum(0) -> subphylum(0));
        right = StatePredicateFormula((kc::tStatePredicate ) right->subphylum(0) -> subphylum(1));
	left -> portfolio_id = portfoliomanager::addFormula(left);
	right -> portfolio_id = portfoliomanager::addFormula(right);
        Task::buildTask(n,disj_id,left,-1);
        Task::buildTask(n,disj_id,right,-1);
}

