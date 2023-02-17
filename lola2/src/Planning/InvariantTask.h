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

#include <Planning/ReachabilityTask.h>
#include <Core/Dimensions.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Portfolio/portfoliomanager.h>


/*!
\brief the verification task

This class delegates the INVARIANT problem to the REACHABILITY problem.
It adjusts the return value and the result interpretation.

*/

class InvariantTask : public Task
{
public:
     InvariantTask(Petrinet * n, int par,tFormula f,int fid)
     {
	preprocessingfinished = false;
	net = n;
	parent = par;
        formula = f;
        formula_id = fid;
        result = TERNARY_UNKNOWN;
	taskname = deconst("AG");
       portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_INVARIANT,DUAL_TASK,NULL);
     }
    ~InvariantTask(){}

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;}

    /// interprete and display the result
    char * interpreteResult(ternary_t result) {
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The predicate is invariant.");
    case TERNARY_FALSE: return deconst( "The predicate is not invariant.");
    case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be invariant.");
        break;
   default: assert(false);
    }
	return NULL;

   }

    Path getWitnessPath() { return * new Path(net);}
    capacity_t *getMarking() { return NULL;}
    statusrecord * getStatistics() {return NULL;}
    statusrecord * getStatus(){ return NULL;}

    static void buildTask(Petrinet * n, int par, tFormula  f, int fid) {
	InvariantTask * N = new InvariantTask(n,par,f,fid);
	f -> id = NULL;
	int myid = portfoliomanager::addFormula(f);
	ReachabilityTask::buildTask(n,N->portfolio_id,f,myid);
	N -> preprocessingfinished = true;
    }

    void derail(){}
    Task * copy(){return NULL;}
};
