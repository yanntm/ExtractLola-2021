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

#include <Core/Runtime.h>
#include <Planning/Task.h>
#include <Planning/StateEquationTask.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include<Formula/StatePredicate/AtomicBooleanPredicate.h>
#include<Formula/StatePredicate/AtomicStatePredicate.h>
#include<Formula/StatePredicate/MagicNumber.h>
#include <Exploration/DFSExploration.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/wait.h>

/////////////////////
// HELPER FUNCTION //
/////////////////////

#ifndef __cplusplus11
inline std::string int_to_string(int i) {
    std::stringstream * s = new std::stringstream;
    (*s) << i;
    return (*s).str();
}
#endif


//pthread_mutex_t StateEquationTask::stateequation_mutex = PTHREAD_MUTEX_INITIALIZER;

StateEquationTask::StateEquationTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
        net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	result = TERNARY_UNKNOWN;
	taskname = deconst("state equation");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_REACHABLE,EQUATION_TASK);
	memory = new Mara();
	goStatus = false;
}

StateEquationTask::~StateEquationTask()
{
}

ternary_t StateEquationTask::getResult()
{

    // reset 
    saraIsRunning = 0; // not yet started
    goStatus = true;
    // extract state predicate from formula
    assert(formula);
    // copy formula for additional dnf rewrite
    kc::tFormula TheFormulaDNF = formula;

	pthread_mutex_lock(&kimwitu_mutex);
    TheFormulaDNF = TheFormulaDNF->rewrite(kc::singletemporal);
    TheFormulaDNF = TheFormulaDNF->rewrite(kc::simpleneg);

    // get the assumed length of DNF formula and the assumed number of ORs
    unparsed.clear();
    TheFormulaDNF->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
    StatePredicate * TheStatePredicate = TheFormulaDNF->formula;
//RT::rep->status("STATE EQUATION: %d %s", portfolio_id, TheStatePredicate -> toString());

    // unparse the content of the problem file for sara
	AtomicBooleanPredicate * dnf = TheStatePredicate->DNF();

	// check pathological cases in dnf
pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	if(dnf)
	{
		portfoliomanager::taskjson[portfolio_id]["stateequation literals"] = static_cast<int>(dnf->literals);
		portfoliomanager::taskjson[portfolio_id]["stateequation problems"] = static_cast<int>(dnf->cardSub);
	}
	else
	{
		portfoliomanager::taskjson[portfolio_id]["stateequation literals"] = JSON::null;
		portfoliomanager::taskjson[portfolio_id]["stateequation problems"] = JSON::null;
	}
pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);

	if(!dnf)
	{
             portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
		return TERNARY_UNKNOWN;
	}
	if(dnf -> magicnumber == MAGIC_NUMBER_TRUE)
	{
	portfoliomanager::report(portfolio_id,TERNARY_TRUE);
		return TERNARY_TRUE;
	}
	if(dnf -> magicnumber == MAGIC_NUMBER_FALSE)
	{
		portfoliomanager::report(portfolio_id,TERNARY_FALSE);
		return TERNARY_FALSE;
	}
	if((!(dnf -> isAnd)) && (dnf->cardSub == 0))
	{
		portfoliomanager::report(portfolio_id,TERNARY_FALSE);
		return TERNARY_FALSE;
	}
	if((dnf -> isAnd) && (dnf->cardSub == 0))
	{
	portfoliomanager::report(portfolio_id,TERNARY_TRUE);
		return TERNARY_TRUE;
	}

	if(dnf->isAnd)
	{
		AtomicBooleanPredicate * temp = new AtomicBooleanPredicate(net,false);
		temp -> addSub(dnf);
		dnf = temp;
	}

	//RT::rep->status("dnf: %s",dnf->toString());
	//RT::rep->status("%s",dnf->toString());

	// write problem file for sara

	// preprare files
    std::string baseFileName = "";
    std::string saraProblemFileName = "";
    std::string saraNetFileName = "";
    std::string saraResultFileName = "";

    std::string formulaNumber = "";
        // individualise formula, add portfolio id to the file name
#ifdef __cplusplus11
        formulaNumber = "-" + std::to_string(portfolio_id);
#else
        formulaNumber = "-" + int_to_string(portfolio_id);
#endif
    // check if the task was given by a file
    if (RT::inputFormulaFileName != "")
    {
        // formula file is given
        baseFileName = RT::inputFormulaFileName;
        // remove file extension if present
        const size_t period_idx = baseFileName.rfind('.');
        if (std::string::npos != period_idx)
        {
            baseFileName.erase(period_idx);
        }
        saraProblemFileName = baseFileName + formulaNumber + ".sara";
        saraResultFileName = baseFileName + formulaNumber + ".sararesult";
	saraNetFileName = baseFileName + formulaNumber + ".saralola";
    }
    else
    {
        // No formula file is given
        saraProblemFileName = "stateEquationProblem" + formulaNumber + ".sara";
        saraResultFileName = "stateEquationProblem" + formulaNumber + ".sararesult";
	saraNetFileName = "stateEquationProblem" + formulaNumber + ".saralola";
    }

    // delete old problem and result file
    std::remove(saraProblemFileName.c_str());
    std::remove(saraResultFileName.c_str());
    // Create new problem file
    std::ofstream lolanetfile(saraNetFileName.c_str(),std::ios::out);
    if(lolanetfile)
    {
	lolanetfile << "PLACE" << std::endl;
	for(int i = 0; i < net->Card[PL];i++)
	{
		lolanetfile << net->Name[PL][i];
		lolanetfile << ((i == net->Card[PL] - 1) ? ";" : ",") << std::endl;
	}
	lolanetfile << "MARKING" << std::endl;
	bool needcomma = false;
	for(int i = 0; i < net->Card[PL];i++)
	{
		if(net->Initial[i])
		{
			if(needcomma)
			{
				lolanetfile << "," << std::endl;;
			}
			else
			{
				needcomma = true;
			}
			lolanetfile << net->Name[PL][i] << " : " << net->Initial[i];
		}
	}
	lolanetfile << ";" << std::endl;
	for(int i = 0; i < net->Card[TR]; i++)
	{
		lolanetfile << "TRANSITION " << net->Name[TR][i] << std::endl << "CONSUME" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][PRE][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][PRE][i][j]] << " : " << net->Mult[TR][PRE][i][j] << ((j == net->CardArcs[TR][PRE][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][PRE][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		lolanetfile << "PRODUCE" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][POST][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][POST][i][j]] << " : " << net->Mult[TR][POST][i][j] << ((j == net->CardArcs[TR][POST][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][POST][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		
	}
	
    }
    
    std::ofstream lolafile(saraProblemFileName.c_str(), std::ios::out);

    // check if file could be created
    if (!lolafile)
    {
        // Problem file could not be constructed
    }
    else
    {
        // Problem file was constructed
        // Write problem in file
	
	for(arrayindex_t i = 0; i < dnf->cardSub; i++)
	{
		// start single problem
		
		lolafile << "PROBLEM saraProblem:" << std::endl;
		lolafile << "GOAL REACHABILITY;" << std::endl;
		lolafile << "FILE " << saraNetFileName << " TYPE LOLA;" << std::endl;
		lolafile << "INITIAL ";
		bool needscomma = false;
		for(arrayindex_t j = 0; j < net->Card[PL]; j++)
		{
			if(net->Initial[j] == 0) continue;
			if(needscomma) lolafile << "," << std::endl;
			lolafile << net->Name[PL][j] << " : " << net->Initial[j];
			needscomma = true;
		}
		lolafile << ";" << std::endl;
		lolafile << "FINAL COVER;" << std::endl;
		lolafile << "CONSTRAINTS ";

		AtomicBooleanPredicate * conj = (AtomicBooleanPredicate *) (dnf->sub[i]);
		
		needscomma = false;
		for(arrayindex_t j = 0; j < conj -> cardSub;j++)
		{
			AtomicStatePredicate * atomic = (AtomicStatePredicate *) (conj->sub[j]);

			if(needscomma) lolafile << ", ";
			needscomma = true;
			bool needsplus = false;
			for(arrayindex_t k = 0; k < atomic->cardPos;k++)
			{
				if(needsplus) lolafile << " + ";
				if(atomic->posMult[k] != 1) lolafile << atomic->posMult[k];
				lolafile << net->Name[PL][atomic->posPlaces[k]];
				needsplus = true;
			}
			for(arrayindex_t k = 0; k < atomic->cardNeg;k++)
			{
				if(needsplus) lolafile << " + ";
				lolafile << "-" << atomic->negMult[k];
				lolafile << net->Name[PL][atomic->negPlaces[k]];
				needsplus = true;
			}
			lolafile << " < " << atomic->threshold;
		}
		lolafile << ";" << std::endl;
	}
        lolafile.close();
    }
    
    // result is as default unknown
    ternary_t result(TERNARY_UNKNOWN);
    
    // fork to get a PID for sara to kill her, if necessary
    pid_t pid_sara = fork();
    if (pid_sara < 0)
    {
        // fork failed
	portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
        return TERNARY_UNKNOWN;
    }
    else if (pid_sara == 0)
    {
        // fork worked - child process
        // call sara
        // set flag, that sara is running for the status message
        saraIsRunning = 1;
        execlp("sara", "sara", "-i", saraProblemFileName.c_str(), "-v", "-o",
                saraResultFileName.c_str(), (char*) 0);
        // call sara failed
        saraIsRunning = 2;
        // exit this process, otherwise we would have two processes
        exit(1);
    }
    else
    {
        saraIsRunning = 1;
        // parent process after fork succeeded
        // set saraPID to kill sara if necessary
pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::childpid[portfolio_id] = pid_sara;
	
pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
        // save sara's process status
        int status = 0;
        // wait until sara is finished
        waitpid(pid_sara, &status, 0);
pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::childpid[portfolio_id] = 0;
pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
        if (WIFEXITED(status) == true)
        {
            // sara finished - check sara's result
            if (system(("grep -q 'SOLUTION' " + saraResultFileName).c_str()) == 0)
            {
                // solution found - result is true
                result = TERNARY_TRUE;
            }
            else if (system(("grep -q 'could not be extended' " + saraResultFileName).c_str()) == 0)
            {
                // sara produced a nontrivial "infeasible" -> cannot trust sara
                result = TERNARY_UNKNOWN;
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) == 0)
            {
                // solution is infeasible - result is false
                result = TERNARY_FALSE;
            }
            else
            {
                // sara can't decide the problem - result is unknonw
                result = TERNARY_UNKNOWN;
            }
        }
    }
	portfoliomanager::report(portfolio_id,result);
    return result;
}


ternary_t StateEquationTask::getSafetyResult(arrayindex_t ppp)
{
	Petrinet * net = Petrinet::InitialNet;

	// preprare files
    std::string baseFileName = "";
    std::string saraProblemFileName = "";
    std::string saraNetFileName = "";
    std::string saraResultFileName = "";

    std::string formulaNumber = "";
        // individualise formula, add portfolio id to the file name
#ifdef __cplusplus11
        formulaNumber = "-" + std::to_string(ppp);
#else
        formulaNumber = "-" + int_to_string(ppp);
#endif
        // No formula file is given
        saraProblemFileName = "stateEquationProblem-Safety" + formulaNumber + ".sara";
        saraResultFileName = "stateEquationProblem-Safety" + formulaNumber + ".sararesult";
	saraNetFileName = "stateEquationProblem-Safety" + formulaNumber + ".saralola";

    // delete old problem and result file
    std::remove(saraProblemFileName.c_str());
    std::remove(saraResultFileName.c_str());
    // Create new problem file
    std::ofstream lolanetfile(saraNetFileName.c_str(),std::ios::out);
    if(lolanetfile)
    {
	lolanetfile << "PLACE" << std::endl;
	for(int i = 0; i < net->Card[PL];i++)
	{
		lolanetfile << net->Name[PL][i];
		lolanetfile << ((i == net->Card[PL] - 1) ? ";" : ",") << std::endl;
	}
	lolanetfile << "MARKING" << std::endl;
	bool needcomma = false;
	for(int i = 0; i < net->Card[PL];i++)
	{
		if(net->Initial[i])
		{
			if(needcomma)
			{
				lolanetfile << "," << std::endl;;
			}
			else
			{
				needcomma = true;
			}
			lolanetfile << net->Name[PL][i] << " : " << net->Initial[i];
		}
	}
	lolanetfile << ";" << std::endl;
	for(int i = 0; i < net->Card[TR]; i++)
	{
		lolanetfile << "TRANSITION " << net->Name[TR][i] << std::endl << "CONSUME" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][PRE][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][PRE][i][j]] << " : " << net->Mult[TR][PRE][i][j] << ((j == net->CardArcs[TR][PRE][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][PRE][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		lolanetfile << "PRODUCE" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][POST][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][POST][i][j]] << " : " << net->Mult[TR][POST][i][j] << ((j == net->CardArcs[TR][POST][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][POST][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		
	}
	
    }
    
    std::ofstream lolafile(saraProblemFileName.c_str(), std::ios::out);

    // check if file could be created
    if (!lolafile)
    {
        // Problem file could not be constructed
    }
    else
    {
        // Problem file was constructed
        // Write problem in file
	
	// start single problem
	
	lolafile << "PROBLEM saraProblem:" << std::endl;
	lolafile << "GOAL REACHABILITY;" << std::endl;
	lolafile << "FILE " << saraNetFileName << " TYPE LOLA;" << std::endl;
	lolafile << "INITIAL ";
	bool needscomma = false;
	for(arrayindex_t j = 0; j < net->Card[PL]; j++)
	{
		if(net->Initial[j] == 0) continue;
		if(needscomma) lolafile << "," << std::endl;
		lolafile << net->Name[PL][j] << " : " << net->Initial[j];
		needscomma = true;
	}
	lolafile << ";" << std::endl;
	lolafile << "FINAL COVER;" << std::endl;
	lolafile << "CONSTRAINTS ";

	lolafile << "-1" ;
	lolafile << net->Name[PL][ppp];
	lolafile << " < " << -2;
	lolafile << ";" << std::endl;
        lolafile.close();
    }
    
    // result is as default unknown
    ternary_t result(TERNARY_UNKNOWN);
    
    // fork to get a PID for sara to kill her, if necessary
    pid_t pid_sara = fork();
    if (pid_sara < 0)
    {
        // fork failed
        return TERNARY_UNKNOWN;
    }
    else if (pid_sara == 0)
    {
        // fork worked - child process
        // call sara
        // set flag, that sara is running for the status message
        execlp("sara", "sara", "-i", saraProblemFileName.c_str(), "-v", "-o",
                saraResultFileName.c_str(), (char*) 0);
        // call sara failed
        // exit this process, otherwise we would have two processes
        exit(1);
    }
    else
    {
        // parent process after fork succeeded
        // set saraPID to kill sara if necessary
        // save sara's process status
        int status = 0;
        // wait until sara is finished
        waitpid(pid_sara, &status, 0);
        if (WIFEXITED(status) == true)
        {
            // sara finished - check sara's result
            if (system(("grep -q 'SOLUTION' " + saraResultFileName).c_str()) == 0)
            {
                // solution found - result is true
                result = TERNARY_FALSE;
            }
            else if (system(("grep -q 'could not be extended' " + saraResultFileName).c_str()) == 0)
            {
                // sara produced a nontrivial "infeasible" -> cannot trust sara
                result = TERNARY_UNKNOWN;
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) == 0)
            {
                // solution is infeasible - result is false
                result = TERNARY_TRUE;
            }
            else
            {
                // sara can't decide the problem - result is unknonw
                result = TERNARY_UNKNOWN;
            }
        }
    }
    return result;
}

void StateEquationTask::getQuasilivenessResult()
{
	Petrinet * net = Petrinet::InitialNet;
	arrayindex_t targettransition = portfoliomanager::gettargettransition();
RT::rep->status("STATE EQUATION TRIES TRANSITION %s", net -> Name[TR][targettransition]);

	// preprare files
    std::string baseFileName = "";
    std::string saraProblemFileName = "";
    std::string saraNetFileName = "";
    std::string saraResultFileName = "";

    std::string formulaNumber = "";
        // individualise formula, add portfolio id to the file name
#ifdef __cplusplus11
        formulaNumber = "-" + std::to_string(targettransition);
#else
        formulaNumber = "-" + int_to_string(targettransition);
#endif
        // No formula file is given
        saraProblemFileName = "stateEquationProblem-QuasiLiveness" + formulaNumber + ".sara";
        saraResultFileName = "stateEquationProblem-QuasiLiveness" + formulaNumber + ".sararesult";
	saraNetFileName = "stateEquationProblem-QuasiLiveness" + formulaNumber + ".saralola";

    // delete old problem and result file
    std::remove(saraProblemFileName.c_str());
    std::remove(saraResultFileName.c_str());
    // Create new problem file
    std::ofstream lolanetfile(saraNetFileName.c_str(),std::ios::out);
    if(lolanetfile)
    {
	lolanetfile << "PLACE" << std::endl;
	for(int i = 0; i < net->Card[PL];i++)
	{
		lolanetfile << net->Name[PL][i];
		lolanetfile << ((i == net->Card[PL] - 1) ? ";" : ",") << std::endl;
	}
	lolanetfile << "MARKING" << std::endl;
	bool needcomma = false;
	for(int i = 0; i < net->Card[PL];i++)
	{
		if(net->Initial[i])
		{
			if(needcomma)
			{
				lolanetfile << "," << std::endl;;
			}
			else
			{
				needcomma = true;
			}
			lolanetfile << net->Name[PL][i] << " : " << net->Initial[i];
		}
	}
	lolanetfile << ";" << std::endl;
	for(int i = 0; i < net->Card[TR]; i++)
	{
		lolanetfile << "TRANSITION " << net->Name[TR][i] << std::endl << "CONSUME" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][PRE][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][PRE][i][j]] << " : " << net->Mult[TR][PRE][i][j] << ((j == net->CardArcs[TR][PRE][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][PRE][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		lolanetfile << "PRODUCE" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][POST][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][POST][i][j]] << " : " << net->Mult[TR][POST][i][j] << ((j == net->CardArcs[TR][POST][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][POST][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		
	}
	
    }
    
    std::ofstream lolafile(saraProblemFileName.c_str(), std::ios::out);

    // check if file could be created
    if (!lolafile)
    {
        // Problem file could not be constructed
    }
    else
    {
        // Problem file was constructed
        // Write problem in file
	
	// start single problem
	
	lolafile << "PROBLEM saraProblem:" << std::endl;
	lolafile << "GOAL REACHABILITY;" << std::endl;
	lolafile << "FILE " << saraNetFileName << " TYPE LOLA;" << std::endl;
	lolafile << "INITIAL ";
	bool needscomma = false;
	for(arrayindex_t j = 0; j < net->Card[PL]; j++)
	{
		if(net->Initial[j] == 0) continue;
		if(needscomma) lolafile << "," << std::endl;
		lolafile << net->Name[PL][j] << " : " << net->Initial[j];
		needscomma = true;
	}
	lolafile << ";" << std::endl;
	lolafile << "FINAL COVER;";
	lolafile << "CONSTRAINTS ";
	needscomma = false;
	for(int i = 0; i < net->CardArcs[TR][PRE][targettransition];i++)
	{
		if(needscomma) lolafile << ", ";
		needscomma = true;
		lolafile << net->Name[PL][net->Arc[TR][PRE][targettransition][i]];
		lolafile << " > " ;
		lolafile << net -> Mult[TR][PRE][targettransition][i];
	}
	lolafile << ";" << std::endl;
        lolafile.close();
    }
    
    // result is as default unknown
    ternary_t result(TERNARY_UNKNOWN);
    
    // fork to get a PID for sara to kill her, if necessary
    pid_t pid_sara = fork();
    if (pid_sara < 0)
    {
        // fork failed
        return;
    }
    else if (pid_sara == 0)
    {
        // fork worked - child process
        // call sara
        // set flag, that sara is running for the status message
        execlp("sara", "sara", "-i", saraProblemFileName.c_str(), "-v", "-o",
                saraResultFileName.c_str(), (char*) 0);
        // call sara failed
        // exit this process, otherwise we would have two processes
        exit(1);
    }
    else
    {
        // parent process after fork succeeded
        // set saraPID to kill sara if necessary
        // save sara's process status
        int status = 0;
        // wait until sara is finished
        waitpid(pid_sara, &status, 0);
        if (WIFEXITED(status) == true)
        {
RT::rep->status("SARA FINISHED");
            // sara finished - check sara's result
            if (system(("grep -q 'SOLUTION' " + saraResultFileName).c_str()) == 0)
            {
                // solution found - result is true
		ternary_t * resultvector = new ternary_t [net -> Card[TR]];
		std::fill(resultvector, resultvector + net -> Card[TR], TERNARY_VOID);
		resultvector[targettransition] = TERNARY_TRUE;
		portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_STATEEQUATION);
                //system(("grep 'SOLUTION' " + saraResultFileName + " > " + saraResultFileName + "copy").c_str());
		
		//for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
		//{
		//	if(resultvector[i] != TERNARY_VOID) continue;
		//	if(system(("grep -q ' "+ std::string(net->Name[TR][i]) + " ' " + saraResultFileName +"copy") .c_str())  == 0)
		//	{
		//		resultvector[i] = TERNARY_TRUE;
		//RT::rep->status("BEIFANG %s", net ->Name[TR][i]);
		//	}
		//}
		//std::remove((saraResultFileName +"copy") .c_str());
		//portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_STATEEQUATION);
		delete [] resultvector;
            }
            else if (system(("grep -q 'could not be extended' " + saraResultFileName).c_str()) == 0)
            {
                // sara produced a nontrivial "infeasible" -> cannot trust sara
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) == 0)
            {
                // solution is infeasible - result is false
		portfoliomanager::synchronisetransition(targettransition, TERNARY_FALSE, GLOBAL_STATEEQUATION);
            }
        }
    }
    return;
}

void StateEquationTask::getConstantPlaceResult()
{
	Petrinet * net = Petrinet::InitialNet;
	arrayindex_t targetplace = portfoliomanager::gettargetplace();
RT::rep->status("STATE EQUATION TRIES PLACE %s", net -> Name[PL][targetplace]);

	// preprare files
    std::string baseFileName = "";
    std::string saraProblemFileName = "";
    std::string saraNetFileName = "";
    std::string saraResultFileName = "";

    std::string formulaNumber = "";
        // individualise formula, add portfolio id to the file name
#ifdef __cplusplus11
        formulaNumber = "-" + std::to_string(targetplace);
#else
        formulaNumber = "-" + int_to_string(targetplace);
#endif
        // No formula file is given
        saraProblemFileName = "stateEquationProblem-ConstantPlace" + formulaNumber + ".sara";
        saraResultFileName = "stateEquationProblem-ConstantPlace" + formulaNumber + ".sararesult";
	saraNetFileName = "stateEquationProblem-ConstantPlace" + formulaNumber + ".saralola";

    // delete old problem and result file
    std::remove(saraProblemFileName.c_str());
    std::remove(saraResultFileName.c_str());
    // Create new problem file
    std::ofstream lolanetfile(saraNetFileName.c_str(),std::ios::out);
    if(lolanetfile)
    {
	lolanetfile << "PLACE" << std::endl;
	for(int i = 0; i < net->Card[PL];i++)
	{
		lolanetfile << net->Name[PL][i];
		lolanetfile << ((i == net->Card[PL] - 1) ? ";" : ",") << std::endl;
	}
	lolanetfile << "MARKING" << std::endl;
	bool needcomma = false;
	for(int i = 0; i < net->Card[PL];i++)
	{
		if(net->Initial[i])
		{
			if(needcomma)
			{
				lolanetfile << "," << std::endl;;
			}
			else
			{
				needcomma = true;
			}
			lolanetfile << net->Name[PL][i] << " : " << net->Initial[i];
		}
	}
	lolanetfile << ";" << std::endl;
	for(int i = 0; i < net->Card[TR]; i++)
	{
		lolanetfile << "TRANSITION " << net->Name[TR][i] << std::endl << "CONSUME" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][PRE][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][PRE][i][j]] << " : " << net->Mult[TR][PRE][i][j] << ((j == net->CardArcs[TR][PRE][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][PRE][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		lolanetfile << "PRODUCE" << std::endl;
		for(int j = 0; j < net->CardArcs[TR][POST][i]; j++)
		{
			lolanetfile << net->Name[PL][net->Arc[TR][POST][i][j]] << " : " << net->Mult[TR][POST][i][j] << ((j == net->CardArcs[TR][POST][i] -1) ? ";" : ",") << std::endl;
		}
		if(net->CardArcs[TR][POST][i] == 0) lolanetfile << std::endl << ";" << std::endl;
		
	}
	
    }
    
    std::ofstream lolafile(saraProblemFileName.c_str(), std::ios::out);

    // check if file could be created
    if (!lolafile)
    {
        // Problem file could not be constructed
    }
    else
    {
        // Problem file was constructed
        // Write problem in file
	
	// start single problem
	
	lolafile << "PROBLEM saraProblem:" << std::endl;
	lolafile << "GOAL REACHABILITY;" << std::endl;
	lolafile << "FILE " << saraNetFileName << " TYPE LOLA;" << std::endl;
	lolafile << "INITIAL ";
	bool needscomma = false;
	for(arrayindex_t j = 0; j < net->Card[PL]; j++)
	{
		if(net->Initial[j] == 0) continue;
		if(needscomma) lolafile << "," << std::endl;
		lolafile << net->Name[PL][j] << " : " << net->Initial[j];
		needscomma = true;
	}
	lolafile << ";" << std::endl;
	lolafile << "FINAL COVER;";
	lolafile << "CONSTRAINTS ";
		lolafile << net->Name[PL][targetplace];
		lolafile << " > " ;
		lolafile << net -> Initial[targetplace]+1;
	lolafile << ";" << std::endl;
        lolafile.close();
    }
    
    // result is as default unknown
    ternary_t result(TERNARY_UNKNOWN);
    
    // fork to get a PID for sara to kill her, if necessary
    pid_t pid_sara = fork();
    if (pid_sara < 0)
    {
        // fork failed
        return;
    }
    else if (pid_sara == 0)
    {
        // fork worked - child process
        // call sara
        // set flag, that sara is running for the status message
        execlp("sara", "sara", "-i", saraProblemFileName.c_str(), "-v", "-o",
                saraResultFileName.c_str(), (char*) 0);
        // call sara failed
        // exit this process, otherwise we would have two processes
        exit(1);
    }
    else
    {
        // parent process after fork succeeded
        // set saraPID to kill sara if necessary
        // save sara's process status
        int status = 0;
        // wait until sara is finished
        waitpid(pid_sara, &status, 0);
        if (WIFEXITED(status) == true)
        {
            // sara finished - check sara's result
            if (system(("grep -q 'SOLUTION' " + saraResultFileName).c_str()) == 0)
            {
                // solution found - result is true
		portfoliomanager::synchroniseplace(targetplace,TERNARY_TRUE,GLOBAL_STATEEQUATION);
		return;
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) != 0)
	    {
		return;
            }
        }
	else
	{
		return;
	}
    }
    // arriving here: nobody can increase place. Now check for decreasing
    if(net -> Initial[targetplace] == 0) 
    {
		portfoliomanager::synchroniseplace(targetplace, TERNARY_FALSE, GLOBAL_STATEEQUATION);
		return;
    }
    std::remove(saraProblemFileName.c_str());
    std::ofstream llolafile(saraProblemFileName.c_str(), std::ios::out);

    // check if file could be created
    if (!llolafile)
    {
        // Problem file could not be constructed
    }
    else
    {
        // Problem file was constructed
        // Write problem in file
	
	// start single problem
	
	llolafile << "PROBLEM saraProblem:" << std::endl;
	llolafile << "GOAL REACHABILITY;" << std::endl;
	llolafile << "FILE " << saraNetFileName << " TYPE LOLA;" << std::endl;
	llolafile << "INITIAL ";
	bool needscomma = false;
	for(arrayindex_t j = 0; j < net->Card[PL]; j++)
	{
		if(net->Initial[j] == 0) continue;
		if(needscomma) llolafile << "," << std::endl;
		llolafile << net->Name[PL][j] << " : " << net->Initial[j];
		needscomma = true;
	}
	llolafile << ";" << std::endl;
	llolafile << "FINAL COVER;";
	llolafile << "CONSTRAINTS ";
		llolafile << net->Name[PL][targetplace];
		llolafile << " < " ;
		llolafile << net -> Initial[targetplace]-1;
	llolafile << ";" << std::endl;
        llolafile.close();
    }
    
    // result is as default unknown
    result = TERNARY_UNKNOWN;
    
    // fork to get a PID for sara to kill her, if necessary
    pid_sara = fork();
    if (pid_sara < 0)
    {
        // fork failed
        return;
    }
    else if (pid_sara == 0)
    {
        // fork worked - child process
        // call sara
        // set flag, that sara is running for the status message
        execlp("sara", "sara", "-i", saraProblemFileName.c_str(), "-v", "-o",
                saraResultFileName.c_str(), (char*) 0);
        // call sara failed
        // exit this process, otherwise we would have two processes
        exit(1);
    }
    else
    {
        // parent process after fork succeeded
        // set saraPID to kill sara if necessary
        // save sara's process status
        int status = 0;
        // wait until sara is finished
        waitpid(pid_sara, &status, 0);
        if (WIFEXITED(status) == true)
        {
            // sara finished - check sara's result
            if (system(("grep -q 'SOLUTION' " + saraResultFileName).c_str()) == 0)
            {
                // solution found - result is true
		portfoliomanager::synchroniseplace(targetplace,TERNARY_TRUE,GLOBAL_STATEEQUATION);
		return;
            }
            else if (system(("grep -q 'could not be extended' " + saraResultFileName).c_str()) == 0)
            {
                // sara produced a nontrivial "infeasible" -> cannot trust sara
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) == 0)
            {
                // solution is infeasible - result is false
		portfoliomanager::synchroniseplace(targetplace, TERNARY_FALSE, GLOBAL_STATEEQUATION);
            }
	    else
	    {
		return;
            }
        }
	else
	{
		return;
	}
    }

    return;
}

char * StateEquationTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
        case TERNARY_TRUE: return deconst("The predicate is reachable.");
        case TERNARY_FALSE: return deconst("The predicate is unreachable.");
        case TERNARY_UNKNOWN: return deconst("The predicate may or may not be reachable.");
	default: assert(false);
    }
	return NULL;
}

Path StateEquationTask::getWitnessPath()
{
    // \todo add witness path
    Path * p = new Path(net);

    return *p;
}

capacity_t * StateEquationTask::getMarking()
{
    return NULL;
}

statusrecord *  StateEquationTask::getStatistics()
{
	return NULL;
}

statusrecord * StateEquationTask::getStatus()
{
	statusrecord * result = new statusrecord();
    switch (saraIsRunning)
    {
    case 0: result -> text =  deconst("sara not yet started (preprocessing)");
	    break;
    case 1: result -> text = deconst( "sara is running");
	    break;
    case 2: result -> text = deconst( "could not launch sara");
	    break;
    default:result-> text = deconst( "unknwon sara status");
    }
    return result;
}


void StateEquationTask::buildTask(Petrinet * n, int par,tFormula  f, int fid)
{
    StateEquationTask * S = new StateEquationTask(n,par,f,fid);
    S -> preprocessingfinished = true;
}

void * StateEquationTask::safetystateequationthread(void *)
{
	Petrinet * net = Petrinet::InitialNet;
	arrayindex_t targetplace = portfoliomanager::gettargetplace();
	bool * tried = new bool [net -> Card[PL]];
	memset(tried, 0, net -> Card[PL]*sizeof(bool));
	while(true)
	{
		ternary_t result = getSafetyResult(targetplace);
		tried[targetplace] = true;
		switch(result)
		{
			case TERNARY_TRUE:
			case TERNARY_FALSE:
		
				portfoliomanager::synchroniseplace(targetplace,result,GLOBAL_STATEEQUATION);
				break;
			
			default:  ;
		}
		for(targetplace = 0; targetplace < net -> Card[PL];targetplace++)
		{
			if(portfoliomanager::globalresult[targetplace] == TERNARY_VOID && !tried[targetplace]) break;
		}
		if(targetplace == net -> Card[PL]) return NULL;
	}
	return NULL;
}

void * StateEquationTask::constantplacestateequationthread(void *)
{
	while(true)
	{
		getConstantPlaceResult();
	}
	return NULL;
}

void * StateEquationTask::quasilivenessstateequationthread(void *)
{
	while(true)
	{
		getQuasilivenessResult();
	}
	return NULL;
}

void StateEquationTask::derail()
{
	if(portfoliomanager::childpid[portfolio_id] > 0) kill(portfoliomanager::childpid[portfolio_id],SIGKILL);
}

Task * StateEquationTask::copy()
{
	return new StateEquationTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
