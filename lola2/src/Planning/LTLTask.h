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
#include <Net/NetState.h>
#include <Planning/Task.h>
#include <Exploration/LTLExploration.h>
#include <Formula/LTL/BuechiFromLTL.h>
#include <Portfolio/portfoliomanager.h>


class AutomataTree;


/*!
\brief the verification task

This class collects all information for executing an LTL model checking
procedure.

*/

class LTLTask: public Task
{
public:
    NetState * ns;
    Store<LTLPayload> * ltlStore;
    Firelist * fl;
    LTLExploration * ltlExploration;
    BuechiAutomata * bauto;
    LTLTask(Petrinet *, int,tFormula,int);
    ~LTLTask();

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
    static void buildTask(Petrinet * n, int par, tFormula f, int fid)
	{
		LTLTask * L = new LTLTask(n,par,f,fid);
		L -> preprocessingfinished = true;
	}
    statusrecord * getStatus();
    int previousNrOfMarkings;
    StatePredicate *buildPropertyFromList(int *pos, int *neg);
    static void ParseBuechi();
    void derail();
    Task * copy();
};
