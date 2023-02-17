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
#include <Exploration/DFSExploration.h>
#include <Exploration/Firelist.h>
#include <Stores/Store.h>
#include <Net/NetState.h>
#include <Exploration/SimpleProperty.h>
#include <CoverGraph/CoverGraph.h>
#include <Core/Runtime.h>


/*!
\brief the verification task

This class collects all information for executing a state space search for 
deadlocks.

*/

class DeadlockSearchTask: public Task
{
public:
    bool tandem;
    bool forceinsertion;
    SimpleProperty *p;
    Firelist * fl;
    DFSExploration *exploration;
    NetState *ns;
    Store<void> *store;
    Store<CoverPayload> *covStore;
    DeadlockSearchTask(Petrinet *, int, tFormula , int);
    DeadlockSearchTask(Petrinet *, int, tFormula , int, bool); // for tandem search: true -> insertion, false -> deletion
    ~DeadlockSearchTask();

    /// run the actual verification algorithm
    ternary_t getResult();
    /// interprete and display the result
    char * interpreteResult(ternary_t result);

    /// return the witness path
    Path getWitnessPath();
    /// return the target marking
    capacity_t *getMarking();

    /// return the number of stored markings
    statusrecord * getStatistics();
    int previousNrOfMarkings;
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid)
    { 
	if(RT::args.stubborn_arg == stubborn_arg_both)
	{
		tFormula  ff = portfoliomanager::copyFormula(f);
		ff -> id = NULL;
		int agg = portfoliomanager::addTask(n,NULL, par, fid, FORMULA_DEADLOCK,AGGREGATE_TASK);
		f -> id = NULL;
		int f1 = portfoliomanager::addFormula(f);
		int f2 = portfoliomanager::addFormula(ff);
		DeadlockSearchTask * D1 = new DeadlockSearchTask(n,agg,f,f1,true);
		DeadlockSearchTask * D2 = new DeadlockSearchTask(n,agg,ff,f2,false);
		D1 -> preprocessingfinished = true;
		D2 -> preprocessingfinished = true;
	}
	else
	{
		DeadlockSearchTask * D = new DeadlockSearchTask(n,par,f,fid);
		D -> preprocessingfinished = true;
	}
    }
    statusrecord * getStatus();
    void derail();
    Task * copy();
};
