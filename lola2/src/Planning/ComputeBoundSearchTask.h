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
#include <Portfolio/portfoliomanager.h>


class ComputeBoundExploration;
class ComputeBoundExplorationRelaxed;
class StatePredicate;
struct CoverPayload;
/*!
\brief the verification task

This class wraps the calculation of place bounds

*/

class ComputeBoundSearchTask : public Task
{
public:
    ternary_t result;
    int resultvalue;
    ComputeBoundSearchTask(Petrinet * n, int, tFormula , int);
    ~ComputeBoundSearchTask();

    /// run the actual verification algorithm
    virtual ternary_t getResult();
	
    /// interprete and display the result
    virtual char * interpreteResult(ternary_t); 

   // bound computation is not able to produce meaningful witnesses
   // LCOV_EXCL_START
    /// return the witness path
    virtual Path getWitnessPath(){Path * p = new Path(net); return *p;}
    /// return the target marking
    virtual capacity_t *getMarking(){return NULL;}
    // LCOV_EXCL_STOP

    /// return the number of stored markings
    virtual statusrecord * getStatistics(); 
    static void buildTask(Petrinet *, int, tFormula , int);

    NetState * ns;
    Firelist * fl;
    ComputeBoundExploration * exploration;
    ComputeBoundExplorationRelaxed * rexploration;
    SimpleProperty * p;
    Store<void> * store;
    Store<statenumber_t> * rstore;
    Store<CoverPayload> * covStore;
    StatePredicate * spFormula;
    statusrecord * getStatus();
    int previousNrOfMarkings;
    void derail();
    Task * copy();
};
