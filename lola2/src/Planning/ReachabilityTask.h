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

#include <Planning/ReachabilitySATTask.h>
#include <Core/Dimensions.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/ReachabilityFindpathTask.h>
#include <Planning/ReachabilitySearchTask.h>
#include <Planning/StateEquationTask.h>
#include <Net/Petrinet.h>


/*!
\brief the verification task

This is a dummy class, i.e. no objects will be generated. The only purpose
of this class is to have a buildTask method that evaluates cmdline options
and creates a a task for reachability checking.

*/

class ReachabilityTask : public Task
{
public:
    ~ReachabilityTask(){}

    /// run the actual verification algorithm
    ternary_t getResult() {return TERNARY_UNKNOWN;}

    /// interprete and display the result
    char * interpreteResult(ternary_t result) {return NULL;}

    /// return the witness path
    Path getWitnessPath() { Path * p = new Path(net); return *p;}
    /// return the target marking
    capacity_t *getMarking() { return NULL;}

    /// return the number of stored markings
    statusrecord * getStatistics() {return NULL;}
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid)
    {
   	int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_REACHABLE,AGGREGATE_TASK,NULL);
	if(RT::args.findpath_arg != findpath_arg_off)
        {
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int myid = portfoliomanager::addFormula(ff);
		ReachabilityFindpathTask::buildTask(n,agg,ff,myid);
	}
	if(RT::args.stateequation_arg != stateequation_arg_off)
        {
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int myid = portfoliomanager::addFormula(ff);
		StateEquationTask::buildTask(n,agg,ff,myid);
	}
	if(RT::args.search_arg != search_arg_off)
	{
		f -> id = NULL;
		int myid = portfoliomanager::addFormula(f);
		ReachabilitySearchTask::buildTask(n,agg,f,myid);
	}
	if(RT::args.satbased_given)
	{
		tFormula  ff = portfoliomanager::copyFormula(f);
		f -> id = NULL;
		int myid = portfoliomanager::addFormula(ff);
		ReachabilitySATTask::buildTask(n,agg,ff,myid);
	}
    }
    statusrecord * getStatus(){return NULL;}
    void derail(){}
    Task * copy(){return NULL;}
};
