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

#include <ReachabilitySAT_Lukas/lola2minisat.h>
#include <Core/Dimensions.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Formula/StatePredicate/StatePredicate.h>


/*!
\brief the verification task

This class wraps the SAT based check for reachability
*/

class ReachabilitySATTask : public Task
{
public:
     reachability_result_t result;
     bool finished;
     bool canceled;
    ReachabilitySATTask(Petrinet * n, int par, tFormula  f, int fid){ 
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	taskname = deconst("sat");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,SAT_TASK);
    }
    ~ReachabilitySATTask(){ 
    }

    /// run the actual verification algorithm
    ternary_t getResult()
    {
	
	canceled = false;
	// Prepare formula
RT::rep->status("SAT STARTED %d", portfolio_id);
RT::rep->status("SAT REALLY STARTED %d", portfolio_id);
        pthread_mutex_lock(&kimwitu_mutex);
	StatePredicate * spFormula;
        formula = formula->rewrite(kc::singletemporal);
        formula = formula->rewrite(kc::simpleneg);
        formula = formula->rewrite(kc::booleanlists);
        formula->unparse(myprinter, kc::internal);
        spFormula = formula->formula->copy(NULL);
        pthread_mutex_unlock(&kimwitu_mutex);
	

	// launch SAT based model checker round by round
        finished = false;
	ternary_t lolaresult = TERNARY_UNKNOWN;
    double varbound = 1;
    if(RT::args.satvariablebound_given){
        if(RT::args.satvariablebound_arg < 0){
            varbound = 0;
        } else if (RT::args.satvariablebound_arg > 1){
            varbound = 1;
        } else {
            varbound = RT::args.satvariablebound_arg;
        }
    }
	result = REACHABILITY_PROPERTY_INDETERMINATE;
	Reachability2Minisat solver(net, spFormula, portfolio_id);
    int round = solver.GetStartRound();
	do
	{
        if(round < solver.GetCurrentMinRounds() && (solver.GetCurrentMinRounds() <= RT::args.satrounds_arg || RT::args.satrounds_arg == 0)) round = solver.GetCurrentMinRounds();
RT::rep->status("TASK %d ROUND %d", portfolio_id,round);
		result = solver.lola2minisat(round, varbound);
		RT::rep->status("Looks like SAT %d is running",portfolio_id);
		if(result == REACHABILITY_PROPERTY_TRUE || result == REACHABILITY_PROPERTY_FALSE) break;
        if(result == REACHABILITY_PROPERTY_BOUND_BREACHED){
            // if we breach the variable bound, we want to raise and restart the round
            if(RT::args.satvariableraise_arg < 0 || RT::args.satvariableraise_arg > 1){
                varbound += 0.1;
            } else {
                varbound += RT::args.satvariableraise_arg;
            }
            if(varbound > 1) varbound = 1;
            round--;
        }
        if(RT::args.satrounds_arg != 0 && RT::args.satrounds_arg < solver.GetCurrentMinRounds()) break;
		if(canceled) break;
	}
	while(round++ != RT::args.satrounds_arg);
RT::rep->status("SAT LOOP LEFT %d", portfolio_id);

        finished = true;
        // display information about the siphon/trap property and set the returned result value
        switch (result)
        {
            case REACHABILITY_PROPERTY_TRUE: 
		portfoliomanager::taskjson[portfolio_id]["sat result"] = "REACHABILITY_PROPERTY_TRUE";
                lolaresult =  TERNARY_TRUE;
		break;
            case REACHABILITY_PROPERTY_FALSE: 
		portfoliomanager::taskjson[portfolio_id]["sat result"] = "REACHABILITY_PROPERTY_FALSE";
                lolaresult =  TERNARY_FALSE;
		break;
            case REACHABILITY_PROPERTY_INDETERMINATE: 
		portfoliomanager::taskjson[portfolio_id]["sat result"] = "REACHABILITY_PROPERTY_INDETERMINATE";
                lolaresult =  TERNARY_UNKNOWN;
		break;
        }
RT::rep->status("SAT REPORTING %d", portfolio_id);
	portfoliomanager::report(portfolio_id,lolaresult);
RT::rep->status("SAT ALMOST FINISHED %d, portfolio_id");
RT::rep->status("SAT FINISHED %d", portfolio_id);
	return lolaresult;
    }

    /// interprete and display the result
    char * interpreteResult(ternary_t r)
    {
        if (result == REACHABILITY_PROPERTY_TRUE)
        {
                    return deconst("The predicate is reachable.");
        }
        // else if (result == REACHABILITY_PROPERTY_FALSE)
        // {
        //             return deconst("The predicate is not reachable.");
        // }
	else
        {
                    return deconst("The predicate may or may not be reachable.");
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
		ReachabilitySATTask * S = new ReachabilitySATTask(n,par,f,fid);
		S -> preprocessingfinished = true;
	}
    statusrecord  *  getStatus()
    { 
	    statusrecord * result = new statusrecord();
	    result ->text= new char[STATUSLENGTH]; 
	    if(finished)
	    {
		sprintf(result->text,"SAT completed");
	    }
	    else
	    {
		sprintf(result->text,"SAT running");
	    }
	    result -> markings = 0;
	    result -> calls = 0;
	    return result;
    }
    void derail(){
RT::rep->status("DERAIL");
	canceled = true;}
    Task * copy(){return new ReachabilitySATTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);}
};
