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

#include <config.h>
#include <Core/Dimensions.h>
#include <Exploration/CTLExploration.h>
#include <Exploration/FirelistStubbornCTL.h>
#include <Symmetry/Symmetry.h>
#include <Symmetry/Constraints.h>
#include <Exploration/Firelist.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <InputOutput/InputOutput.h>
#include <Planning/CTLTask.h>
#include <Planning/StoreCreator.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Net/NetState.h>

// input files
extern FILE *ptformula_in;

// Kimwitu++ objects

CTLTask::CTLTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	result = TERNARY_UNKNOWN;
	memory = new Mara();
	taskname = deconst("CTL model checker");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_CTL,EXCL_MEM_TASK,memory);
    goStatus = false;
    // prepare counting of place in the formula

        // replace path quantor+temporal operator by dedicated CTL operator
	pthread_mutex_lock(&kimwitu_mutex);
	ParserPTNet::relabelnet = net;
	Petrinet::InitialNet = net;
        formula = formula->rewrite(kc::tautology);
        formula = formula->rewrite(kc::ctloperators);
        formula = formula->rewrite(kc::tautology);
        formula->unparse(myprinter, kc::ctl);
	pthread_mutex_unlock(&kimwitu_mutex);
	assert(formula);
	assert(formula->ctl_formula);
        ctlFormula = formula->ctl_formula;

        assert(ctlFormula);
}


/*!
\post memory for all members is deallocated
*/
CTLTask::~CTLTask()
{
#ifndef USE_PERFORMANCE
    delete ns;
    delete ctlStore;
    delete fl;
    delete sfl;
    delete ctlExploration;
#endif
}


ternary_t CTLTask::getResult()
{

    // prepare task
    ctlStore = StoreCreator<void *>::createStore(net,memory,number_of_threads);
     if(RT::args.stubborn_arg != stubborn_arg_off)
     {
	// use stubborn sets
	
	// mark visible transitions
	ctlFormula->setVisible();

		// use stubborn set firelist generator
		sfl = new FirelistStubbornCTL(net);
                portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "ctl preserving";
     }
     else
     {
            portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
	    sfl = new Firelist(net);
     }
     fl = new Firelist(net); // this is the list to be used for subformulas that contain X operators
    ctlExploration = new CTLExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    // compute symmetries
    if (RT::args.symmetry_given)
    {
        SymmetryCalculator *SC = new SymmetryCalculator(net,ctlFormula);
        assert(SC);
        SC->ComputeSymmetries(portfolio_id);
        delete SC;
	ctlStore = reinterpret_cast<SymmetryStore<void*>*>(ctlStore)->setGeneratingSet(SymmetryCalculator::G);
    }

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);
    goStatus = true;
    bool_result = ctlExploration->checkProperty(ctlFormula, *ctlStore, *fl, * sfl, *ns);
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }

    portfoliomanager::report(portfolio_id,result);
    return result;
}


char * CTLTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The net satisfies the given formula.");
    case TERNARY_FALSE: return deconst("The net does not satisfy the given formula.");
    case TERNARY_UNKNOWN: return deconst( "The net may or may not satisfy the given formula.");
	default: assert(false);
	    }
	return NULL;
}


Path CTLTask::getWitnessPath() 
{
        return ctlExploration->path();
}


capacity_t *CTLTask::getMarking() 
{
	return NULL;
}

statusrecord * CTLTask::getStatistics()
{
	statusrecord * result = new statusrecord;
        result -> calls = ctlStore->get_number_of_calls();
        result -> markings = ctlStore->get_number_of_markings();
	result -> text = NULL;
	return result;
}

statusrecord * CTLTask::getStatus()
{
	if(!goStatus)
	{
		return NULL;
	}
	statusrecord * result = new statusrecord;
	result -> text = NULL;
	result -> markings = ctlStore -> get_number_of_markings();
	result ->  calls = ctlStore -> get_number_of_calls();
	return result;

}

void CTLTask::derail()
{
	memory->stop = true;
}

Task * CTLTask::copy()
{
	return new CTLTask(net,parent,portfoliomanager::copyFormula(formula),formula_id);
}

