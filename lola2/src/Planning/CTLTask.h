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
#include <Exploration/CTLExploration.h>
#include <Portfolio/portfoliomanager.h>

struct CTLFormula;

/*!
\brief the verification task

This class collects all information for executing a CTL model checking
procedure.

*/

class CTLTask: public Task
{
public:
    CTLFormula * ctlFormula;
    Store<void *> *ctlStore;
    Firelist * fl; // the firelist generator for the full mode --> used if subformula contains X
    Firelist * sfl; // the firelist generator for the stubborn mode --> used if subformula does not contain X
    NetState * ns;
    CTLExploration * ctlExploration;
    CTLTask(Petrinet *, int, tFormula , int);
    ~CTLTask();

    /// run the actual verification algorithm
    ternary_t getResult();
    /// interprete and display the result
    char * interpreteResult(ternary_t result);

    /// return the witness path
    Path getWitnessPath() ;
    /// return the target marking
    capacity_t *getMarking() ;

    /// return the number of stored markings
    statusrecord * getStatistics();
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid)
    { 
	CTLTask * C = new CTLTask(n, par,f,fid);

	C -> preprocessingfinished = true;
	return;
    }

    statusrecord * getStatus();
    int previousNrOfMarkings;
    void derail();
    Task * copy();
};
