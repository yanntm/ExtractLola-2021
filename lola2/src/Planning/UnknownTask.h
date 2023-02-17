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
#include <Exploration/Firelist.h>
#include <Exploration/StatePredicateProperty.h>
#include <Net/NetState.h>
#include <Exploration/SimpleProperty.h>


/*!
\brief the verification task

UnknwonTask os a placeholder for the case that a task is required
but no useful task can be found.
*/

class UnknownTask : public Task
{
public:
    UnknownTask(Petrinet * n, int par, tFormula f,int fid)
	{
		preprocessingfinished = false;
		net = n;
		parent = par;
		taskname = deconst("unknown (placeholder)");
		formula = f;
		formula_id = fid;
		result = TERNARY_UNKNOWN;
		memory = NULL;
		portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_MODELCHECKING,UNKNOWN_TASK,NULL);
		
	}
    ~UnknownTask(){}

    /// run the actual verification algorithm
    ternary_t getResult(){ 
				portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
				return TERNARY_UNKNOWN;
			} 

    /// interprete and display the result
    char * interpreteResult(ternary_t result){return deconst(" "); }

    /// return the witness path
    Path getWitnessPath(){return *new Path(net);}
    /// return the target marking
    capacity_t *getMarking(){ return NULL;}

    /// return the number of stored markings
    statusrecord * getStatistics(){return NULL;} 
    static void buildTask(Petrinet *n, int par ,tFormula f,int i)
    { 
		UnknownTask * U = new UnknownTask(n,par,f,i);
		U -> preprocessingfinished = true;
    }
    StatePredicate * spFormula;
    Firelist * fl;
    NetState * ns;
    SimpleProperty * p;
    statusrecord * getStatus(){return NULL;}
    // for initial task, it is unlikely that we run for more than 5 secs
    // LCOV_EXCL_START
    // LCOV_EXCL_STOP
     void derail(){}
     Task * copy(){return new UnknownTask(net,parent,portfoliomanager::copyFormula(formula), formula_id);}
};
