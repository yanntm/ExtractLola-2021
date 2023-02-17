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
#include <Net/Petrinet.h>
#include <Net/NetState.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/Firelist.h>
#include <Exploration/SimpleProperty.h>


class DFSExploration;
class StatePredicate;
struct CoverPayload;
/*!
\brief the verification task

This class schedules subtasks of a top level conjunction

*/

class ConjunctionTask : public Task
{
public:
    ~ConjunctionTask();

    /// run the actual verification algorithm
    virtual ternary_t getResult(){return TERNARY_UNKNOWN;}
	
    /// interprete and display the result
    virtual char * interpreteResult(ternary_t result){return NULL;}

    // witnesses are left to subproblems
    // LCOV_EXCL_START
    /// return the witness path
    virtual Path getWitnessPath(){return *(new Path(net));}
    /// return the target marking

    virtual capacity_t *getMarking(){return NULL;} 
    // left to child process
    virtual statusrecord * getStatus(){return NULL;} // do not yet have a good idea... 
    // LCOV_EXCL_STOP

    /// return the number of stored markings
    virtual statusrecord * getStatistics(){return NULL;} // do not yet have a good idea... 
    static void buildTask(Petrinet *, int, tFormula , int);
    void derail(){}
    Task * copy(){return NULL;}

};
