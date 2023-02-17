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
#include <Symmetry/Constraints.h>


/*!
\brief the verification task

An empty Task is a task that does nothing. It implements the --check=none
option.

*/

class EmptyTask : public Task
{
public:
     EmptyTask(Petrinet * n, int par,tFormula  f, int fid)
     {
	preprocessingfinished = false;
	net = n;
	formula_id = fid;
	taskname = deconst("empty task");
        portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_INITIAL,CONST_TASK);
     }
    ~EmptyTask(){};

    ternary_t getResult() { 

		number_of_threads = 1;
		// compute symmetries, just for fun...
		if (RT::args.symmetry_given)
		    {
			SymmetryCalculator *SC = NULL;
			SC = new SymmetryCalculator(net);
			assert(SC);
			SC->ComputeSymmetries(portfolio_id);
			delete SC;
		    }

		
		portfoliomanager::report(portfolio_id,result);
		return TERNARY_UNKNOWN;}
    /// interprete and display the result
    char * interpreteResult(ternary_t result) {return deconst("no verification problem provided.");}
    // the following will never be called
    // LCOV_EXCL_START
    /// return the witness path
    Path getWitnessPath() {assert(false); return * (new Path(net));}
    // LCOV_EXCL_STOP

    /// return the target marking
    capacity_t *getMarking() {assert(false); return NULL;}

    /// return the number of stored markings
    statusrecord * getStatistics() { return NULL;}
    static void buildTask(Petrinet * n, int par, tFormula  f, int fid) 
	{ 
		EmptyTask * E = new EmptyTask(n,par,f,fid);
		E -> preprocessingfinished = true;
	}
    statusrecord * getStatus(){return NULL;}
     void derail(){}
    Task * copy(){return new EmptyTask(net,parent,NULL,formula_id);}
};
