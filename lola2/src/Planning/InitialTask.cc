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
#include <Frontend/SymbolTable/SymbolTable.h>

#include <Core/Dimensions.h>
#include <Core/Handlers.h>
#include <Frontend/Parser/formula_abstract.h>
#include <Core/Runtime.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/InitialTask.h>
#include <Exploration/DeadlockExploration.h>



/*!
\brief the verification task

This class handles initial satisfiability. 
*/

InitialTask::InitialTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
	net = n;
	parent = par;
	formula = f;
	formula_id = fid;
	result = TERNARY_UNKNOWN;
	taskname = deconst("preprocessing");
	portfolio_id = portfoliomanager::addTask(n,this,par,fid,FORMULA_INITIAL,CONST_TASK);
	goStatus = false;
	extern int ptformula_lex_destroy();
    if (RT::args.formula_given)
    {
	// copy restructured formula into internal data structures
	// extract state predicate from formula
	pthread_mutex_lock(&kimwitu_mutex);
	char * fname = formula -> id;
        formula = formula->rewrite(kc::singletemporal);
        formula = formula->rewrite(kc::simpleneg);
        formula = formula->rewrite(kc::booleanlists);
Petrinet::InitialNet = net;

        formula->unparse(myprinter, kc::internal);
	formula -> id = fname;
	pthread_mutex_unlock(&kimwitu_mutex);
        spFormula = formula->formula;

    }
    net->preprocessingfinished = true;


}

ternary_t InitialTask::getResult()
{
    ns = NetState::createNetStateFromInitial(net);
    ternary_t result(TERNARY_FALSE);

    spFormula->evaluate(*ns);

    // temporary result transfer, as long as the variable bool_result is needed
    if (spFormula -> value)
    {
        result = TERNARY_TRUE;
    }

    portfoliomanager::report(portfolio_id,result);
    return result;
}
 
char * InitialTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
    case TERNARY_TRUE: return deconst("The net satisfies the property already in its initial state.");
    case TERNARY_FALSE: return deconst("The net violates the given property already in its initial state.");
    default: break; // cannot happen - only to silence compiler  LCOV_EXCL_LINE
        }
return NULL;
}
 
Path InitialTask::getWitnessPath()
{
	return p -> path();
}

capacity_t *InitialTask::getMarking()
{
    // we only provide witness states for simple properties where we found
    // a result
    if (p and p->value)
    {
        return ns->Current;
    }
    else
    {
        return NULL;
    }
}

statusrecord * InitialTask::getStatistics()
{
	return NULL;
}

void InitialTask::buildTask(Petrinet * n, int par,tFormula  f, int fid)
{
	InitialTask * I = new InitialTask(n,par,f,fid);
	I -> preprocessingfinished = true;
}

// it is unlikely that we run for +5secs
// LCOV_EXCL_START
statusrecord * InitialTask::getStatus()
{
	return NULL;
}

void InitialTask::derail()
{
}

Task * InitialTask::copy()
{
	return new InitialTask(net,parent,portfoliomanager::copyFormula(formula), formula_id);
}

// LCOV_EXCL_STOP
