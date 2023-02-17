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
#include <Net/NetState.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/Firelist.h>
#include <Exploration/ComputeBoundExploration.h>
#include <Portfolio/portfoliomanager.h>
#include <Planning/ComputeBoundSearchTask.h>
#include <Planning/ComputeBoundFindpathTask.h>
#include <Planning/ComputeBoundStateEquationTask.h>


class ComputeBoundExploration;
class ComputeBoundExplorationRelaxed;
class StatePredicate;
struct CoverPayload;
/*!
\brief the verification task

This class wraps the calculation of place bounds

*/

class ComputeBoundTask : public Task
{
public:
    ternary_t result;
    int resultvalue;

    /// run the actual verification algorithm
    virtual ternary_t getResult(){return TERNARY_UNKNOWN;}
	
    /// interprete and display the result
    virtual char * interpreteResult(ternary_t){return NULL;} 

   // bound computation is not able to produce meaningful witnesses
   // LCOV_EXCL_START
    /// return the witness path
    virtual Path getWitnessPath(){Path * p = new Path(net); return *p;}
    /// return the target marking
    virtual capacity_t *getMarking(){return NULL;}
    // LCOV_EXCL_STOP

    /// return the number of stored markings
    virtual statusrecord * getStatistics(){return NULL;} 
    static void buildTask(Petrinet *n , int par, tFormula f, int fid)
    {
        pthread_mutex_lock(&kimwitu_mutex);
        f->unparse(myprinter, kc::internal);
        pthread_mutex_unlock(&kimwitu_mutex);
        StatePredicate * spFormula = f->formula;
        // compute structural bound to make it available to all tasks
        ComputeBoundExploration::lp(n, (AtomicStatePredicate *) spFormula);

	int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_REACHABLE,AGGREGATE_TASK,NULL);
        if(RT::args.findpath_arg != findpath_arg_off)
        {
                tFormula  ff = portfoliomanager::copyFormula(f);
                ff -> id = NULL;
                int myid = portfoliomanager::addFormula(ff);
                ComputeBoundFindpathTask::buildTask(n,agg,ff,myid);
        }
        if(RT::args.stateequation_arg != stateequation_arg_off)
        {
                tFormula  ff = portfoliomanager::copyFormula(f);
                ff -> id = NULL;
                int myid = portfoliomanager::addFormula(ff);
                ComputeBoundStateEquationTask::buildTask(n,agg,ff,myid);
        }
        if(RT::args.search_arg != search_arg_off)
        {
                f -> id = NULL;
                int myid = portfoliomanager::addFormula(f);
                ComputeBoundSearchTask::buildTask(n,agg,f,myid);
        }
	portfoliomanager::isnumerical[agg] = true;
	portfoliomanager::isnumerical[par] = true;

    }

    NetState * ns;
    Firelist * fl;
    ComputeBoundExploration * exploration;
    ComputeBoundExplorationRelaxed * rexploration;
    SimpleProperty * p;
    Store<void> * store;
    Store<statenumber_t> * rstore;
    Store<CoverPayload> * covStore;
    StatePredicate * spFormula;
    statusrecord * getStatus(){return NULL;}
    int previousNrOfMarkings;
    void derail(){}
    Task * copy(){return NULL;}
};
