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
#include <Planning/AGEFTask.h>


/*!
\brief the verification task

This class delegates the EFAG problem to the AGEF problem.
It adjusts the return value and the result interpretation.

*/

class EFAGTask : public Task
{
public:
    ~EFAGTask(){}
     EFAGTask(Petrinet * n, int par, tFormula  f, int fid)
     {
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	result = TERNARY_UNKNOWN;
	taskname = deconst("EFAG");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_EFAG,DUAL_TASK);
     }

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;}

    /// interprete and display the result
    char * interpreteResult(ternary_t result) {
	switch(result)
	{
	case TERNARY_TRUE: return deconst("The predicate is possibly invariant.");
	case TERNARY_FALSE:  return deconst("The predicate is not possibly invariant.");
	case TERNARY_UNKNOWN: return deconst( "The predicate may or may not be possibly invariant.");
	default: assert(false);
	}
	return NULL;
   }

    Path getWitnessPath() { return *new Path(net);}
    capacity_t *getMarking() { return NULL;}
    statusrecord * getStatistics() {return NULL;}
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid) {
	EFAGTask * N = new EFAGTask(n,par,f,fid);
	f->id = NULL;
	int myid = portfoliomanager::addFormula(f);
	AGEFTask::buildTask(n,N->portfolio_id,f,myid);
	N -> preprocessingfinished = true;
    }
   statusrecord * getStatus(){ return NULL;}
    void derail(){}
    Task * copy(){return NULL;}
};
