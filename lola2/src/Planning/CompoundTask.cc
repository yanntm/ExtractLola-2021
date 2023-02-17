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


#include <pthread.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <CoverGraph/CoverPayload.h>
#include <Symmetry/Constraints.h>
#include <SweepLine/Sweep.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Core/Handlers.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/StoreCreator.h>
#include <Planning/CompoundTask.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ParallelExploration.h>

kc::tFormula * CompoundTask::subformula = NULL;
int * CompoundTask::cparent = NULL;
bool * CompoundTask::cvisible = NULL;
bool * CompoundTask::cboolean = NULL;
int CompoundTask::cardsubformula = 0;
int CompoundTask::cardvisible = 0;
char ** CompoundTask::name = NULL;

/*!
\brief the verification task

This class handles a compound task

*/

CompoundTask::~CompoundTask()
{
    //delete [] subTask;
}

ternary_t CompoundTask::getResult()
{
	return TERNARY_VOID; // has no particular meaning
}

char *CompoundTask::interpreteResult(ternary_t result)
{
	return NULL;
}

typedef struct 
{
	tFormula f;
	int i;
} compound_arg;

pthread_mutex_t compound_mutex = PTHREAD_MUTEX_INITIALIZER;

extern ParserPTNet * symbolTables;

void * process_subformula(void * a)
{

	compound_arg * arg = (compound_arg *) a;
	// 1. add formula

	if(cvisible[arg->i])
	{
		arg -> f -> id = CompoundTask::name[arg->i];
		if(!arg->f->id )
		{
			char * nname = new char [20];
			sprintf(nname,"formula # %4u", arg->i);
			arg->f->id = nname;
		}
	}
	else
	{
		char * nname = new char [30];
		sprintf(nname,"internal top level formula # %4u", arg->i);
		arg->f->id = nname;
		
	}
	int formula_id = portfoliomanager::addFormula(arg -> f,cvisible[arg->i]);
	arg -> f -> portfolio_id = formula_id;

	// 2. mark visible

	pthread_mutex_lock(&compound_mutex);

	symbolTables -> markVisibleNodes(arg -> f);

	// 3. copy net

	ProtoNet * pnet = new ProtoNet(*ProtoNet::currentnet);
	pnet -> recordVisibleNodes();
	pnet -> data = &(RT::data["localreduction"][arg->i]);


	pthread_mutex_unlock(&compound_mutex);

	// 4. net reduction

	if(arg -> f -> containsTemporal)
	{
		pnet -> reduce(arg -> f -> containsNext, arg -> f -> containsDeadlock, arg -> f -> type);
	}

	// 5. protonet -> petrinet

	// 5a. relabel formula

	// 5b. generate net
	Petrinet * petri = ParserPTNet::protonet2net(pnet);
	petri -> name = arg -> f -> id;


	// 7. preprocess

	petri -> preprocess1(portfoliomanager::formulajson[formula_id]);
	ParserPTNet::relabelFormula(petri,arg -> f);
	if(arg -> f -> containsTemporal) petri -> preprocess2(portfoliomanager::formulajson[formula_id]);
	if(arg -> f -> containsTemporal) petri -> preprocess3(portfoliomanager::formulajson[formula_id]);
	// 6. add task

	Task::buildTask(petri,cparent[arg->i],arg -> f, formula_id);


	// 8. get ready to launch threads
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::preprocessingfinished[formula_id] = true;
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return NULL;
}

void CompoundTask::buildTask(Petrinet * n, int, tFormula f, int)
{
	pthread_mutex_lock(&kimwitu_mutex);
	unparsed.clear();
        f->unparse(myprinter,compound);
	pthread_mutex_unlock(&kimwitu_mutex);
	compound_arg * args[cardsubformula];
	portfoliomanager::cardvisible = cardvisible;
	pthread_t compoundthread[cardsubformula];
	RT::data["localreduction"] = JSON();
	for(int i = 0; i < cardsubformula;i++)
	{
		if(cboolean[i]) continue;
		subformula[i] = portfoliomanager::copyFormula(subformula[i]);
		RT::data["localreduction"] += JSON();
	}
	for(int i = 0; i < cardsubformula;i++)
	{
		if(cboolean[i]) continue;
		args[i] = new compound_arg;
		args[i]->f = subformula[i];
		args[i]->i = i;
		pthread_create(&(compoundthread[i]),NULL, process_subformula, args[i]);
	}

}

void CompoundTask::sort()
{
}
