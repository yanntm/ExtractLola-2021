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
#include <Siphon/lola2minisat.h>


/*!
\brief the verification task

This class wraps the investigation of the siphon/trap property
*/

class SiphonTrapTask : public Task
{
public:
     siphon_result_t result;
     bool finished;
     bool printFinalResult;
    SiphonTrapTask(Petrinet * n, int par, tFormula  f, int fid){ 
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	taskname = deconst("stp");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_DEADLOCK,SIPHON_TASK);
    }
    ~SiphonTrapTask(){ 
    }

    /// run the actual verification algorithm
    ternary_t getResult()
    {
        finished = false;
        result = lola2minisat(net,portfolio_id);
	ternary_t lolaresult = TERNARY_UNKNOWN;
        finished = true;
        // display information about the siphon/trap property and set the returned result value
        switch (result)
        {
            case SIPHON_PROPERTY_TRUE: 
		portfoliomanager::taskjson[portfolio_id]["siphon result"] = "SIPHON_PROPERTY_TRUE";
                lolaresult =  TERNARY_FALSE;
		break;
            case SIPHON_PROPERTY_FALSE: 
		portfoliomanager::taskjson[portfolio_id]["siphon result"] = "SIPHON_PROPERTY_FALSE";
		break;
            case SIPHON_INHOMOGENIOUS: 
		portfoliomanager::taskjson[portfolio_id]["siphon result"] = "SIPHON_INHOMOGENIOUS";
            case SIPHON_INCONCLUSIVE: 
		portfoliomanager::taskjson[portfolio_id]["siphon result"] = "SIPHON_INCONCLUSIVE";
            case SIPHON_INDETERMINATE: 
		portfoliomanager::taskjson[portfolio_id]["siphon result"] = "SIPHON_INDETERMINATE";
		break;
        }
	portfoliomanager::report(portfolio_id,lolaresult);
	return lolaresult;
    }

    /// interprete and display the result
    char * interpreteResult(ternary_t r)
    {
        if (result == SIPHON_PROPERTY_TRUE)
        {
                    return deconst("The net does not have deadlocks.");
        }
        else
        {
                    return deconst("The net may or may not have deadlocks.");
        }
    }

    /// return the witness path
    Path getWitnessPath() {Path * p = new Path(net); return *p;}
    /// return the target marking
    capacity_t *getMarking() {return NULL;}

    /// return the number of stored markings
    statusrecord * getStatistics() {return NULL;} // \TODO: add stat on minisat
    static void buildTask(Petrinet * n, int par,tFormula  f, int fid) 
	{
		SiphonTrapTask * S = new SiphonTrapTask(n,par,f,fid);
		S -> preprocessingfinished = true;
	}
    statusrecord  *  getStatus(){ 
		statusrecord * result = new statusrecord();
	result ->text= new char[STATUSLENGTH]; 
    if(finished)
    {
	sprintf(result->text,"STP completed");
    }
    else
    {
	sprintf(result->text,"STP running");
    }
    result -> markings = 0;
    result -> calls = 0;
    return result;}
    void derail(){}
    Task * copy(){return new SiphonTrapTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);}
    static void * siphontrapthread(void *)
    {
	while(true)
	{
		siphon_result_t res = lola2minisat(Petrinet::InitialNet, 0);
RT::rep->status("SIPHON RESULT %d", res);
		if(!RT::args.siphondepth_given) return NULL;
		if(res != SIPHON_INCONCLUSIVE) return NULL;
		RT::args.siphondepth_arg += 10;
	}
	return NULL;
    }
};
