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
#include <Planning/EUTask.h>
#include <Planning/ReachabilityTask.h>
#include <Portfolio/portfoliomanager.h>

/*!
\brief the verification task

This class delegates the AGAF problem to the EFEG problem.
It adjusts the return value and the result interpretation.

*/

class ARTask : public Task
{
  public:
    ~ARTask() {}
    ARTask(Petrinet * n, int par, tFormula  f, int fid)
    {
	preprocessingfinished = false;
	net = n;
	parent = par; 
	taskname = deconst("AR");
	formula = f;
	formula_id = fid;
	result = TERNARY_UNKNOWN;
	portfolio_id = portfoliomanager::addTask(n,this,par, fid,FORMULA_AR,DUAL_TASK);
    }

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;} // not called for meta tasks

    /// interprete and display the result
    char * interpreteResult(ternary_t result)
    {
        switch (result)
        {
        case TERNARY_TRUE: return deconst("Predicate Phi releases Predicate Psi on all paths.");
        case TERNARY_FALSE: return deconst("Predicate Phi does not release Predicate Psi on all paths.");
        case TERNARY_UNKNOWN: return deconst("Predicate Phi may or may not release Predicate Psi on all paths.");
        default: assert(false);
        }
	return NULL;
    }

	// these routines are never called for meta tasks
    Path getWitnessPath() { return *new Path(net); }
    capacity_t *getMarking() { return NULL; }
    statusrecord *getStatus() { return NULL; }
    statusrecord * getStatistics() {return NULL;}

    static void buildTask(Petrinet * n, int par, tFormula  f, int fid)
    {
        Task * t = new ARTask(n, par,f,fid);
	f -> id = NULL;
	int myid = portfoliomanager::addFormula(f);
        EUTask::buildTask(n, t->portfolio_id,f,myid);
	t -> preprocessingfinished = true;
        return ;
    }
    void derail(){}
    Task * copy(){return NULL;}
};
