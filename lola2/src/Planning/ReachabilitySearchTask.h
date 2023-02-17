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
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/TSCCExploration.h>
#include <Exploration/Firelist.h>
#include <Exploration/SimpleProperty.h>
#include <Exploration/ReachabilityExplorationRelaxed.h>


class DFSExploration;
class StatePredicate;
struct CoverPayload;
/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/

class ReachabilitySearchTask : public Task
{
public:
	bool tandem;
	bool forceinsertion;
	int tokenthreshold;
	ternary_t result;
    ReachabilitySearchTask(Petrinet *, int,tFormula , int);
    ReachabilitySearchTask(Petrinet *, int,tFormula , int,bool); // to be used for stubborn = both: true -> insertion, false -> deletion
    ~ReachabilitySearchTask();

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
    DFSExploration * exploration;
    ReachabilityExplorationRelaxed * exploration1;
    StatePredicateProperty * p;
    Store<void> * store;
    Store<statenumber_t> * store1;
    Store<CoverPayload> * covStore;
    StatePredicate * spFormula;
    statusrecord * getStatus();
    bool relaxed;
    int previousNrOfMarkings;
    void derail();
    Task * copy();
};
