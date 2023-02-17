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

#pragma once

#include <Core/Dimensions.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/EGEFTask.h>
#include <Planning/ReachabilityTask.h>
#include <Portfolio/portfoliomanager.h>


/*!
\brief the verification task

This class delegates the AFAG problem to the EGEF problem.
It adjusts the return value and the result interpretation.

*/

class AFAGTask : public Task
{
public:
    ~AFAGTask(){}
     AFAGTask(Petrinet * n, int par, tFormula f, int fid)
     {
	preprocessingfinished = false;
        net = n;
	parent = par;
	taskname = deconst("AFAG");
	formula = f;
	result = TERNARY_UNKNOWN;
	formula_id = fid;
	portfolio_id = portfoliomanager::addTask(n,this, par, fid, FORMULA_AFAG, DUAL_TASK);
	return;
     }

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;} // is non-executable

    /// interprete and display the result
    char *  interpreteResult(ternary_t result) {
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is eventually invariant.");
    case TERNARY_FALSE: return deconst("The predicate is not eventually invariant.");
    case TERNARY_UNKNOWN: return deconst("The predicate may or may not be eventually invariant.");
   default: assert(false);
    }
return NULL;
   }

    // these routines are never called for meta tasks
    Path getWitnessPath() { return *new Path(net);}
    capacity_t *getMarking() { return NULL;}
    statusrecord * getStatus(){ return NULL;}
    statusrecord * getStatistics() { return NULL;}

    static void buildTask(Petrinet * n, int par, tFormula f, int fid) {
	Task * t = new AFAGTask(n,par,f,fid);
	f -> id = NULL;
	int my_id = portfoliomanager::addFormula(f);
	EGEFTask::buildTask(n,t->portfolio_id, f,my_id);
	t -> preprocessingfinished = true;
	return;
    }
   void derail(){}
   Task * copy()
   {
	return NULL;
   }
   
};
