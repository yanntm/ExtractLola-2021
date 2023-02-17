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
#include <Planning/DeadlockSearchTask.h>
#include <Planning/SiphonTrapTask.h>
#include <Planning/DeadlockFindpathTask.h>



/*!
\brief the verification task

This is a dummy class, i.e. no objects will be generated. The only purpose
of this class is to have a buildTask method that evaluates cmdline options
and assembles a--possibly complex--task for deadlock checking.

*/

class DeadlockTask : public Task
{
public:
     // there will be no members of this class
     // LCOV_EXCL_START
    ~DeadlockTask(){}

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;} // dummy

    /// interprete and display the result
    char * interpreteResult(ternary_t result) {return NULL;} //dummy

    /// return the witness path
    Path getWitnessPath() { assert(false); return *(new Path(net));} // dumy
    /// return the target marking
    capacity_t *getMarking() { return NULL;} // dummy

    /// return the number of stored markings
    statusrecord * getStatistics() {return NULL;} // dummy
    statusrecord * getStatus(int){ return NULL;}
    // LCOV_EXCL_STOP
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid) // NOT dummy
    {  
	// create aggregation task since portfolio may be nontrivial
	int agg = portfoliomanager::addTask(n,NULL,par,fid,FORMULA_DEADLOCK,AGGREGATE_TASK);

	// deal with search task
	if(RT::args.search_arg != search_arg_off)
	{
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int ffid = portfoliomanager::addFormula(ff);
		DeadlockSearchTask::buildTask(n,agg,ff,ffid);
	}

	// deal with findpath task
	if(RT::args.findpath_arg != findpath_arg_off)
	{
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int ffid = portfoliomanager::addFormula(ff);
		DeadlockFindpathTask::buildTask(n,agg,ff,ffid);
	}

	// deal with siphon/trap task
	if(RT::args.siphontrap_arg != siphontrap_arg_off)
	{
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int ffid = portfoliomanager::addFormula(ff);
		SiphonTrapTask::buildTask(n,agg,ff,ffid);
	}
    }
    void derail(){}
    Task * copy(){return NULL;}
    
};
