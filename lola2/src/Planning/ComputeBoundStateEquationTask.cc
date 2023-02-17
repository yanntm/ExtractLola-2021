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


#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <CoverGraph/CoverPayload.h>
#include <Symmetry/Constraints.h>
#include <SweepLine/Sweep.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/StoreCreator.h>
#include <Planning/ComputeBoundStateEquationTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornComputeBound.h>
#include <Exploration/FirelistStubbornComputeBoundCombined.h>
#include <Exploration/ComputeBoundExploration.h>
#include <Exploration/ComputeBoundExplorationRelaxed.h>
#include <unistd.h>
#include <sys/wait.h>


#ifndef __cplusplus11
inline std::string int_to_string(int i) {
    std::stringstream * s = new std::stringstream;
    (*s) << i;
    return (*s).str();
}
#endif


/*!
\brief the verification task

This class wraps the reachability check by statespace exploration

*/


ComputeBoundStateEquationTask::ComputeBoundStateEquationTask(Petrinet * n,int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula_id = fid;
	formula = f;
	memory = new Mara();
	taskname = deconst("state equation");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid, FORMULA_BOUND,EQUATION_TASK);
	portfoliomanager::isnumerical[portfolio_id] = true;
	portfoliomanager::isnumerical[parent] = true;
	goStatus = false;
        // extract state predicate from formula
	assert(formula);

	pthread_mutex_lock(&kimwitu_mutex);
        formula->unparse(myprinter, kc::internal);
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;
	// compute structural bound to make it available to all tasks
	

}

ComputeBoundStateEquationTask::~ComputeBoundStateEquationTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete store;
    delete rstore;
    delete covStore;
    delete p;
    delete spFormula;
    delete fl;
    delete exploration;
    delete rexploration;
#endif
}

ternary_t ComputeBoundStateEquationTask::getResult()
{
	AtomicStatePredicate * atomic = (AtomicStatePredicate *) (spFormula);
    if(atomic -> upper_bound == MAX_CAPACITY) 
    {
        portfoliomanager::report(portfolio_id,TERNARY_UNKNOWN);
	return TERNARY_UNKNOWN;
    }
    // reset
    saraIsRunning = 0; // not yet started
    goStatus = true;
    // extract state predicate from formula
    assert(formula);
    // copy formula for additional dnf rewrite
    kc::tFormula TheFormulaDNF = formula;

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
        saraProblemFileName = "stateEquationComputeBoundProblem" + formulaNumber + ".sara";
        saraResultFileName = "stateEquationComputeBoundProblem" + formulaNumber + ".sararesult";
        saraNetFileName = "stateEquationComputeBoundProblem" + formulaNumber + ".saralola";
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
    if (lolafile)
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
	lolafile << " > " << atomic->upper_bound;
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
		resultvalue = atomic -> upper_bound - atomic -> threshold ;
		portfoliomanager::reportNumerical(portfolio_id, resultvalue);
            }
            else if (system(("grep -q 'INFEASIBLE' " + saraResultFileName).c_str()) == 0)
            {
                // solution is infeasible - result must be smaller
                result = TERNARY_UNKNOWN;
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


char * ComputeBoundStateEquationTask::interpreteResult(ternary_t r)
{
	char * value = new char[1000];
	if(r == TERNARY_TRUE)
	{
		sprintf(value,"The maximum value of the given expression is %d.",resultvalue);
		return value;
	}
	sprintf(value, "The maximum value of the given expression is unknown");
	return value;
}
 
statusrecord * ComputeBoundStateEquationTask::getStatistics()
{
	statusrecord * result = new statusrecord();
        return result;
} 

void ComputeBoundStateEquationTask::buildTask(Petrinet * n, int par, tFormula  f, int fid)
{
	ComputeBoundStateEquationTask * C = new ComputeBoundStateEquationTask(n,par,f,fid);
	C -> preprocessingfinished = true;
}


statusrecord  * ComputeBoundStateEquationTask::getStatus()
{
		return NULL;
}

void ComputeBoundStateEquationTask::derail()
{

}

Task * ComputeBoundStateEquationTask::copy()
{
	return new ComputeBoundStateEquationTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}
