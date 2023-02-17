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
#include <Exploration/Firelist.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/EGExploration.h>


class DFSExploration;
class StatePredicate;
struct CoverPayload;
/*!
\brief the verification task

This class implements the dedicated search routine for EG formulas

*/

class EGTask : public Task
{
public:
	ternary_t result;
    EGTask(Petrinet *, int,tFormula,int);
    ~EGTask();

    /// run the actual verification algorithm
    virtual ternary_t getResult();
	
    /// interprete and display the result
    virtual char * interpreteResult(ternary_t result); 

    /// return the witness path
    virtual Path getWitnessPath();
    /// return the target marking
    virtual capacity_t *getMarking();

    /// return the number of stored markings
    virtual statusrecord * getStatistics(); 
    static void buildTask(Petrinet *, int,tFormula,int);

    NetState * ns;
    Firelist * fl;
    EGExploration * exploration;
    StatePredicateProperty * p;
    Store<bool> * store;
    Store<CoverPayload> * covStore;
    StatePredicate * spFormula;
    statusrecord * getStatus();
    int previousNrOfMarkings;
    void derail();
    Task * copy();
};
