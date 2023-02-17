/***************************************************************************

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

#include <Planning/ReachabilityTask.h>
#include <Planning/InvariantTask.h>
#include <config.h>
#include <Core/Dimensions.h>
#include <Core/mysemaphore.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Frontend/Parser/ParserPnml.h>
#include <InputOutput/InputOutput.h>
#include <Net/NetState.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Planning/Task.h>
#include <Planning/SinglePathExTask.h>
#include <Planning/SinglePathAllTask.h>
#include <Planning/FullTask.h>
//#include <Planning/ConjunctionTask.h>
//#include <Planning/DisjunctionTask.h>
#include <Planning/ComputeBoundTask.h>
//#include <Planning/CompoundTask.h>
#include <Planning/LTLTask.h>
#include <Planning/UnknownTask.h>
#include <Planning/EGTask.h>
#include <Planning/EUTask.h>
#include <Planning/ARTask.h>
#include <Planning/AUTask.h>
#include <Planning/ERTask.h>
#include <Planning/AFTask.h>
#include <Planning/AGAFTask.h>
#include <Planning/EFEGTask.h>
#include <Planning/CTLTask.h>
#include <Planning/DeadlockTask.h>
#include <Planning/NoDeadlockTask.h>
#include <Planning/AGEFAGTask.h>
#include <Planning/EFAGTask.h>
#include <Planning/AXAGTask.h>
#include <Planning/EXEFTask.h>
#include <Planning/AXAFTask.h>
#include <Planning/EXEGTask.h>
#include <Planning/EXEUTask.h>
#include <Planning/AXARTask.h>
#include <Planning/EXERTask.h>
#include <Planning/AXAUTask.h>
#include <Planning/EGEFTask.h>
#include <Planning/AFAGTask.h>
#include <Planning/InitialTask.h>
#include <Planning/EmptyTask.h>
#include <Witness/Path.h>
#include <Stores/NetStateEncoder/BitEncoder.h>
#include <Stores/NetStateEncoder/CopyEncoder.h>
#include <Stores/NetStateEncoder/FullCopyEncoder.h>
#include <Stores/NetStateEncoder/SimpleCompressedEncoder.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Formula/StatePredicate/MagicNumber.h>

// the parsers
extern int ptformula_parse();
extern int ptformula_lex_destroy();
extern int ptbuechi_parse();
extern int ptbuechi_lex_destroy();

mysemaphore * Task::foldllnetsemaphore = NULL;
mysemaphore * Task::bignetsemaphore = NULL;
// input files
extern FILE *ptformula_in;
extern FILE *ptbuechi_in;

// code to parse from a string
struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE ptformula__scan_string(const char *yy_str);
extern YY_BUFFER_STATE ptbuechi__scan_string(const char *yy_str);

extern SymbolTable *buechiStateTable;
extern FILE *tl_out;

threadid_t Task::number_of_threads = 1;
int compoundformulaid[MAX_TASKS];
tFormula compoundformula[MAX_TASKS];
tFormula skeletonformula[MAX_TASKS];

int cardtobepreprocessed = 0;
int Task::cardsubformula = 0;
// Kimwitu++ objects
extern kc::tFormula TheFormula;
extern kc::tBuechiAutomata TheBuechi;

bool Task::havehlnet = false;
bool Task::havellnet = false;
bool Task::havellformula = false;
bool Task::havehlformula = false;

Petrinet * Task::GlobalLLNet;
class ParserPTNet;
ParserPTNet * Task::symboltables;
Petrinet * Task::GlobalHLNet;

ParserPTNet * LLsymbolTables = NULL; // symbol table for LL net
ParserPTNet * HLsymbolTables = NULL; // symbol table for HL net, if unique

bool noguards(Petrinet * n)
{
	return false;
}

bool nonblockingguards(Petrinet * n)
{
	if(!Task::havehlnet) 
	{
		// When a HL net is folded from LL net,
		// findlow is checked during folding
		// --> just return precomputed value
		return n -> findlow;
	}
	// For HL net input, launch findlow only if not launched before
	pthread_mutex_lock(& (HLsymbolTables->findlowmutex));	
	if(!(HLsymbolTables -> findlowstarted))
	{
		HLsymbolTables -> findlowvalue = HLsymbolTables -> checkFindlowCriterion(n);
		HLsymbolTables -> findlowstarted = true;
	}
	pthread_mutex_unlock(&HLsymbolTables->findlowmutex);
if(HLsymbolTables->findlowvalue)
{
RT::rep->status("HLFINDLOW");
}
else
{
RT::rep->status("HLNOFINDLOW");
}
	return HLsymbolTables -> findlowvalue;
}

/// * receive a formula from the formula parser
/// * restructure and simplify formula
/// * checkin formula to portfolio manager
/// * separate into Boolean subformulas and add Boolean tasks
/// * separate leafs into LL and Skeleton formula
void Task::CompoundPlanning(tFormula f)
{	
//RT::rep->status("INITIALLY ");
//Task::outputFormula(f);
	// 1. initial preprocessing of the formula

	char * formulaname = f -> id; // save formula name as it may be lost by rewriting

	//       restructure the formula:

	pthread_mutex_lock(&kimwitu_mutex);
	//       Phase 1: remove syntactic sugar
	f = f->rewrite(kc::goodbye_doublearrows);
	f = f->rewrite(kc::goodbye_xor);
//pthread_mutex_unlock(&kimwitu_mutex);
//RT::rep->status("BEFORE CHECKSTABLE");
//Task::outputFormula(f);
//pthread_mutex_lock(&kimwitu_mutex);
	if(havellformula) 
	{
		f -> unparse(myprinter,kc::checkstableatomic);
	}
//pthread_mutex_unlock(&kimwitu_mutex);
//RT::rep->status("AFTER CHECKSTABLE");
//Task::outputFormula(f);
//pthread_mutex_lock(&kimwitu_mutex);
	//       Phase 3: Apply logical tautologies.
	f = f->rewrite(kc::tautology);
	//       Phase 3a: Remove empty path quantifiers
	f->unparse(myprinter, kc::containstemp);
	f = f->rewrite(kc::emptyquantifiers);
	f->unparse(myprinter, kc::containstemp);
	f->unparse(myprinter, kc::temporal);
	f->unparse(myprinter, kc::reduction);
	pthread_mutex_unlock(&kimwitu_mutex);

	f -> id = formulaname;


	// 2. checkin to the portfolio manager

	
	int i = portfoliomanager::cardvisible++;
	if(!f->id)
	{
		char * nname = new char [20];
		sprintf(nname,"formula # %4u", i);
		f->id = nname;
	}
	int formula_id = f->portfolio_id = portfoliomanager::addFormula(f,true /* visible */);

	// 3. initial planning down to the level of top temporal formulas

	if(f -> type == FORMULA_CONJUNCTION || f -> type == FORMULA_DISJUNCTION)
	{
		BooleanPlanning(-1,f);
	}
	else
	{
		LeafPlanning(-1,f);
	}
	return;
}


const char * category2string(formula_t);
void Task::BooleanPlanning(int parent, tFormula f)
{	
	int taskid;
	switch (f->type)
	{
	case FORMULA_CONJUNCTION:
		taskid = portfoliomanager::addTask(NULL,NULL,parent,f->portfolio_id,FORMULA_CONJUNCTION,CONJUNCTION_TASK,NULL);
		break;
	case FORMULA_DISJUNCTION:
		taskid = portfoliomanager::addTask(NULL,NULL,parent,f->portfolio_id,FORMULA_DISJUNCTION,DISJUNCTION_TASK,NULL);
		break;
	default:
		LeafPlanning(parent,f);
		return;
	}
	// we arrive here in case of CONJ or DISJ formula
	// --> recursive descent into subformulas
	tFormula formula1 = portfoliomanager::copyFormula(f);
	tFormula formula2 = portfoliomanager::copyFormula(f);
	tFormula leftformula = kc::StatePredicateFormula((tStatePredicate) formula1->subphylum(0)->subphylum(0));
	leftformula->unparse(myprinter, kc::containstemp);
	leftformula->unparse(myprinter, kc::temporal);
	tFormula rightformula = kc::StatePredicateFormula((tStatePredicate) formula2->subphylum(0)->subphylum(1));
	rightformula->unparse(myprinter, kc::containstemp);
	rightformula->unparse(myprinter, kc::temporal);
	int left = portfoliomanager::addFormula(leftformula,false);
	leftformula->portfolio_id = left;
	int right = portfoliomanager::addFormula(rightformula,false);
	rightformula->portfolio_id = right;
	portfoliomanager::parent[left] = portfoliomanager::parent[right] = f -> portfolio_id;
	BooleanPlanning(taskid, leftformula);
	BooleanPlanning(taskid, rightformula);
}

void stop_planning(int id)
{
	// interrupt the planning process for this id and set the task to "obsolete"
	RT::rep->status("planning for %s stopped (result already fixed).", portfoliomanager::formula[id]->id);
	UnknownTask::buildTask(portfoliomanager::pn[id], portfoliomanager::parent[id],portfoliomanager::formula[id],id);
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing--;
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return;
}

pthread_mutex_t Task::compound_mutex = PTHREAD_MUTEX_INITIALIZER;


void * Task::process_subformula(void * a)
{
	tFormula f = (tFormula) a;
	int id = f -> portfolio_id;
	

	int rootindex = id;
	while(portfoliomanager::parent[rootindex] >= 0) rootindex = portfoliomanager::parent[rootindex];

	if(portfoliomanager::result[id] != TERNARY_VOID)
	{
		stop_planning(id);
		return NULL;
	}

	// 1. mark visible

	pthread_mutex_lock(&compound_mutex);
	ProtoNet::currentvisibility++;

	// 2. copy net

	ProtoNet * pnet = portfoliomanager::proto[id];

	pnet -> markedvisibility = ProtoNet::currentvisibility;
	//pnet -> resymbol(portfoliomanager::symboltable[id]->PlaceTable,portfoliomanager::symboltable[id]->TransitionTable );

	LLsymbolTables -> markVisibleNodes(f);

	pnet -> data = new JSON();
	pnet -> recordVisibleNodes();
	pthread_mutex_unlock(&compound_mutex);

	// 3. local net reduction

	
	if((RT::args.netreduction_arg == netreduction_arg_local ||
	   RT::args.netreduction_arg == netreduction_arg_both) &&
	f -> containsTemporal)
	{
		pnet -> reduce(f -> containsNext, f -> containsDeadlock, f -> type);
		pnet -> applySlicing(f->containsNext, f->containsDeadlock, f -> type, f);
		pnet -> reduce(f -> containsNext, f -> containsDeadlock, f -> type);

	}
	if(portfoliomanager::result[id] != TERNARY_VOID)
	{
		stop_planning(id);
		return NULL;
	}

	// 4. generate net
	Petrinet * petri = ParserPTNet::protonet2net(pnet);

	petri -> name = f -> id;
	portfoliomanager::pn[id] = petri;


	// 5. preprocess

	petri -> preprocess0();
	petri -> preprocess1();
	if(portfoliomanager::result[id] != TERNARY_VOID)
	{
		stop_planning(id);
		return NULL;
	}
	if(f -> type != FORMULA_INITIAL) 
	{
		petri -> preprocess2();
		petri -> preprocess3();
	}

	pthread_mutex_lock(&compound_mutex);
	LLsymbolTables -> relabelFormula(petri,f);
	pthread_mutex_unlock(&compound_mutex);



	// 6. add task
	Task::buildTask(petri,portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
	petri -> preprocessingfinished = true;
	delete pnet;
	bignetsemaphore->put();
	
	
	// 8. get ready to launch threads
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing--;
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return NULL;
}

void * Task::process_initial(void * a)
{
	tFormula f = (tFormula) a;


	// 4. generate net

	Petrinet * petri = portfoliomanager::pn[f->portfolio_id];
	petri -> name = f -> id;


	// 5. preprocess

	// done in main.cc: petri -> preprocess0();
	// done in main.cc: petri -> preprocess1();
	LLsymbolTables -> relabelFormula(petri,f);

	// 6. add task

	Task::buildTask(petri,portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);


	// 8. get ready to launch threads
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing--;
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return NULL;
}

/// thread body for executing the planning process 
/// for the following case:
/// * we have a single HL net skeleton
/// * we have a HL formula

void * Task::process_skeleton_subformula_havehlnet(void * a)
{
	tFormula f = (tFormula) a;
	int myid = f -> portfolio_id;

	if(!RT::args.skeleton_flag)
	{
		UnknownTask::buildTask(NULL,portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
		pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
		portfoliomanager::cardpreprocessing--;
		pthread_cond_signal(&portfoliomanager::portfolio_cond);
		pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
		return NULL;
	}

	// net: SymbolTable+ProtoNet+Net Local HighLevel Unreduced formula: HighLevel Local but all attached to global HL net symboltable

	// 1. copy net and mark visible

	//portfoliomanager::proto[myid]->resymbol(portfoliomanager::symboltable[myid]->PlaceTable, portfoliomanager::symboltable[myid]->TransitionTable);

	// net: SymbolTable+ProtoNet+Net Local HighLevel Unreduced formula: HighLevel Local attached to individual HL net symboltable

	pthread_mutex_lock(&compound_mutex);
	ProtoNet::currentvisibility++;
	HLsymbolTables -> markVisibleNodes(f);
	portfoliomanager::proto[myid] -> data = new JSON();
	portfoliomanager::proto[myid] -> markedvisibility = ProtoNet::currentvisibility;
	portfoliomanager::proto[myid] -> recordVisibleNodes();
	pthread_mutex_unlock(&compound_mutex);

	// 2. local net reduction
	if(portfoliomanager::result[myid] != TERNARY_VOID)
	{
		stop_planning(myid);
		return NULL;
	}

	if((RT::args.netreduction_arg == netreduction_arg_local ||
	   RT::args.netreduction_arg == netreduction_arg_both) &&
	f -> containsTemporal)
	{

		portfoliomanager::proto[myid] -> reduce(f -> containsNext, f -> containsDeadlock, f -> type);
		portfoliomanager::proto[myid] -> applySlicing(f->containsNext, f->containsDeadlock, f -> type, f);
		portfoliomanager::proto[myid] -> reduce(f -> containsNext, f -> containsDeadlock, f -> type);

	}

	// net: SymbolTable+ProtoNet+Net Local HighLevel Reduced formula: HighLevel Local attached to individual HL net symboltable

	// 3. generate net

	portfoliomanager::pn[myid] = ParserPTNet::protonet2net(portfoliomanager::proto[myid]);
	portfoliomanager::pn[myid] -> name = f -> id;
	portfoliomanager::pn[myid] -> isSkeleton = true;
	portfoliomanager::pn[myid] -> preprocess0();


	// 4. preprocess

	portfoliomanager::pn[myid] -> preprocess1();
	if(f -> type != FORMULA_INITIAL) 
	{
		portfoliomanager::pn[myid] -> preprocess2();
		portfoliomanager::pn[myid] -> preprocess3();
	}
	pthread_mutex_lock(&compound_mutex);
	HLsymbolTables->relabelFormula(portfoliomanager::pn[myid],f);
	pthread_mutex_unlock(&compound_mutex);

	// 5. add task

	Task::buildSkeletonTask(portfoliomanager::pn[myid],portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
	portfoliomanager::pn[myid] -> preprocessingfinished = true;
	delete portfoliomanager::proto[myid];

	// 6. get ready to launch threads

	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing--;
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return NULL;
}

void * Task::process_skeleton_subformula_havellnet(void * a)
{

	foldllnetsemaphore -> get();
	tFormula f = (tFormula) a;
	int index = f -> portfolio_id;

	if(!RT::args.skeleton_flag)
	{
		UnknownTask::buildTask(NULL,portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
		pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
		portfoliomanager::cardpreprocessing--;
		pthread_cond_signal(&portfoliomanager::portfolio_cond);
		pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
		return NULL;
	}
	if(f -> type == FORMULA_INITIAL)
	{
		portfoliomanager::pn[index] = NULL;
		UnknownTask::buildTask(portfoliomanager::pn[index],portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
		pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
		portfoliomanager::cardpreprocessing--;
		pthread_cond_signal(&portfoliomanager::portfolio_cond);
		pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
		foldllnetsemaphore -> put();
		return NULL;
	}

	// 1. fold net

	pthread_mutex_lock(&compound_mutex);
	ProtoNet::currentnet = portfoliomanager::proto[index];
	Create_HL_Net * CHLN = new Create_HL_Net(ProtoNet::currentnet, f);
	Create_HL_Net::current = CHLN;
	ParserPTNet * myskeleton = CHLN -> create_HL_Net();

	if(myskeleton -> PlaceTable -> getCard() >= ProtoNet::currentnet -> cardPL / 3
        || myskeleton -> TransitionTable -> getCard() >= ProtoNet::currentnet -> cardTR / 3)
	{
		portfoliomanager::pn[index] = NULL;
		UnknownTask::buildTask(portfoliomanager::pn[index],portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
		pthread_mutex_unlock(&compound_mutex);
		pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
		portfoliomanager::cardpreprocessing--;
		pthread_cond_signal(&portfoliomanager::portfolio_cond);
		pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
		foldllnetsemaphore -> put();
		return NULL;
	}
	myskeleton -> symboltables2protonet();
	ProtoNet::currentnet -> isSkeleton = true;
	portfoliomanager::proto[index] = ProtoNet::currentnet;
	ProtoNet * myprotoskeleton = ProtoNet::currentnet;
	myprotoskeleton->data = new JSON();

	// 2. fold formula

	ParserPTNet::currentsymbols = myskeleton;
	f = portfoliomanager::formula[index] = CHLN->create_HL_Formula(f);

	pthread_mutex_unlock(&compound_mutex);
	
	pthread_mutex_lock(&kimwitu_mutex);
	//f->unparse(myprinter,elem);
	//       Phase 1: remove syntactic sugar
	f = f->rewrite(kc::goodbye_doublearrows);
	f = f->rewrite(kc::goodbye_xor);
	//       Phase 3: Apply logical tautologies.
	f = f->rewrite(kc::tautology);
	//       Phase 3a: Remove empty path quantifiers
	f->unparse(myprinter, kc::containstemp);
	f = f->rewrite(kc::emptyquantifiers);
	f->unparse(myprinter, kc::containstemp);
	f->unparse(myprinter, kc::temporal);
	pthread_mutex_unlock(&kimwitu_mutex);


	// 1. copy net and mark visible

	pthread_mutex_lock(&compound_mutex);
	ProtoNet::currentvisibility++;
	myprotoskeleton -> markedvisibility = ProtoNet::currentvisibility;
	myskeleton -> markVisibleNodes(f);
	myprotoskeleton -> recordVisibleNodes();
	pthread_mutex_unlock(&compound_mutex);

	// 3. generate net

	Petrinet * petri = ParserPTNet::protonet2net(myprotoskeleton);
	petri -> findlow = myskeleton -> findlowvalue;
	portfoliomanager::pn[f->portfolio_id] = petri;
	petri -> name = f -> id;
	petri -> isSkeleton = true;
	petri -> preprocess0();


	// 4. preprocess

	petri -> preprocess1();
	if(f -> type != FORMULA_INITIAL) 
	{
		petri -> preprocess2();
		petri -> preprocess3();
	}

	myskeleton->relabelFormula(petri,f);

	// 5. add task

	Task::buildSkeletonTask(petri,portfoliomanager::parent[f->portfolio_id],f, f->portfolio_id);
	petri -> preprocessingfinished = true;
	delete myprotoskeleton;
	delete CHLN;

	// 6. get ready to launch threads

	foldllnetsemaphore -> put();
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing--;
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return NULL;
}


void Task::LeafPlanning(int par,tFormula f)
{
	// split task tree into branch for ll net verification and
	// branch for skeleton verfication

	portfoliomanager::parent[f->portfolio_id] = par;
	int myid = f -> portfolio_id;

	tFormula llformula = portfoliomanager::copyFormula(f);
	tFormula otherformula = portfoliomanager::copyFormula(f);
	int tid = portfoliomanager::addTask(NULL,NULL, par, myid,f->type,AGGREGATE_TASK,NULL);
	int llindex = portfoliomanager::addFormula(llformula);
	int skeletonindex = portfoliomanager::addFormula(otherformula);
	portfoliomanager::parent[llindex] = portfoliomanager::parent[skeletonindex] = myid;
	portfoliomanager::top[llindex] = true;
	portfoliomanager::topskeleton[skeletonindex] = true;
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	portfoliomanager::cardpreprocessing += 2; // book slots for skeleton planning and p/t planning
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
}

void Task::buildTask(Petrinet * n, int par,tFormula f, int myid)
{
	switch (f->type)
	{
	case FORMULA_BOUND:
		ComputeBoundTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_DEADLOCK:
		DeadlockTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_NODEADLOCK:
		NoDeadlockTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_REACHABLE:
		ReachabilityTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_INVARIANT:
		InvariantTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_LIVENESS:
		AGEFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EFAGEF:
		EFAGEFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EFAG:
		EFAGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EFEG:
		EFEGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AGAF:
		AGAFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EGEF:
		// Intermediate solution: need to reshape the EGEF procedure
		CTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AFAG:
		// Intermediate solution: need to reshape the EGEF procedure
		CTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EXEF:
		EXEFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AXAG:
		AXAGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EXEG:
		EXEGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AXAF:
		AXAFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EXEU:
		EXEUTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AXAR:
		AXARTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EXER:
		EXERTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AXAU:
		AXAUTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AGEFAG:
		AGEFAGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_FAIRNESS:
		LTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_STABILIZATION:
		LTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EVENTUALLY:
		AFTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EG:
		EGTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_EU:
		EUTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AR:
		ARTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_AU:
		AUTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_ER:
		ERTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_SINGLEPATHEX:
		SinglePathExTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_SINGLEPATHALL:
		SinglePathAllTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_INITIAL:
		InitialTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_BOTH:
		if (RT::args.preference_arg == preference_arg_ctl || RT::args.preference_arg == preference_arg_force_ctl)
		{
			CTLTask::buildTask(n,par,f,myid);
		}
		else
		{
			LTLTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_LTL:
		if (RT::args.preference_arg == preference_arg_force_ctl)
		{
			return;
		}
		LTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_CTL:
		if (RT::args.preference_arg == preference_arg_force_ltl)
		{
			return;
		}
		CTLTask::buildTask(n,par,f,myid);
		break;
	case FORMULA_MODELCHECKING:
		RT::rep->status("checking CTL*");
		RT::rep->message("check not yet implemented");
		UnknownTask::buildTask(n,par,f,myid);
		break;
	default:
		assert(false); // complete case consideration
	}

	return;
}

bool assumedeadlockfree()
{
	if(RT::args.deadlockfree_given) 
	{
RT::rep->status("ASSUMEDDEADLOCKFREE");
		return true;
	}
RT::rep->status("NOTDEADLOCKFREE");
	return false;
}

void Task::buildSkeletonTask(Petrinet * n, int par,tFormula f, int myid)
{
	if(!RT::args.skeleton_given) // skeleton switched off
	{
		UnknownTask::buildTask(n,par,f,myid);
		return;
	}
	// deadlocks are not preserved by skeleton approach
	if(f -> containsDeadlock) 
	{
		UnknownTask::buildTask(n,par,f,myid);
		return;
	}
	// fireability permits only negative  answers, universal formula only positive
	if(f -> containsFireable && f -> containsUniversal)
	{
		UnknownTask::buildTask(n,par,f,myid);
		return;
	}
	// unfireability permits only positive  answers, existential  formula only negative
	if(f -> containsUnfireable && f -> containsExistential)
	{
		UnknownTask::buildTask(n,par,f,myid);
		return;
	}
	int fid;
	int tid;
	switch (f->type)
	{
	case FORMULA_REACHABLE:
		{
			// Reachability in the skeleton is necessary for
			// reachability in the net
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_REACHABLE,NECESSARY_QUICKCHECK_TASK,NULL);
			ReachabilityTask::buildTask(n,myid,f,fid);
		}
		break;
	case FORMULA_INVARIANT:
		{
			// Invariance in the skeleton is sufficient for
			// invariance in the net
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_INVARIANT,SUFFICIENT_QUICKCHECK_TASK,NULL);
			InvariantTask::buildTask(n,myid,f,fid);
		}
		break;
	case FORMULA_EFEG:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EFEG,NECESSARY_QUICKCHECK_TASK,NULL);
			EFEGTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_AGAF:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AGAF,SUFFICIENT_QUICKCHECK_TASK,NULL);
			AGAFTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EGEF:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EFEG,NECESSARY_QUICKCHECK_TASK,NULL);
			//intermediate solution, need to reshape EGEF
			CTLTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_AFAG:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AFAG,SUFFICIENT_QUICKCHECK_TASK,NULL);
			//intermediate solution, need to reshape EGEF
			CTLTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EXEF:
		// Satisfaction in the skeleton is necessary for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EXEF,NECESSARY_QUICKCHECK_TASK,NULL);
		EXEFTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_AXAG:
		// Satisfaction in the skeleton is sufficient for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AXAG,SUFFICIENT_QUICKCHECK_TASK,NULL);
		AXAGTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_EXEG:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EXEG,NECESSARY_QUICKCHECK_TASK,NULL);
			EXEGTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_AXAF:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AXAF,SUFFICIENT_QUICKCHECK_TASK,NULL);
			AXAFTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EXEU:
		// Satisfaction in the skeleton is necessary for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EXEU,NECESSARY_QUICKCHECK_TASK,NULL);
		EXEUTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_AXAR:
		// Satisfaction in the skeleton is sufficient for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AXAR,SUFFICIENT_QUICKCHECK_TASK,NULL);
		AXARTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_EXER:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EXER,NECESSARY_QUICKCHECK_TASK,NULL);
			EXERTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_AXAU:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AXAU,SUFFICIENT_QUICKCHECK_TASK,NULL);
			AXAUTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_FAIRNESS:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_FAIRNESS,SUFFICIENT_QUICKCHECK_TASK,NULL);
			LTLTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_STABILIZATION:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_STABILIZATION, SUFFICIENT_QUICKCHECK_TASK,NULL);
			LTLTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EVENTUALLY:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EVENTUALLY,SUFFICIENT_QUICKCHECK_TASK,NULL);
			AFTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EG:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EG,NECESSARY_QUICKCHECK_TASK,NULL);
			EGTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_EU:
		// Satisfaction in the skeleton is necessary for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_EU,NECESSARY_QUICKCHECK_TASK,NULL);
		EUTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_AR:
		// Satisfaction in the skeleton is sufficient for
		// satisfaction in the net as long as all deadlocks in the
		// net are deadlocks in the skeleton
		fid = portfoliomanager::addFormula(f,false);
		tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AR,SUFFICIENT_QUICKCHECK_TASK,NULL);
		ARTask::buildTask(n,myid,f,fid);
		break;
	case FORMULA_AU:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is sufficient for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_AU,SUFFICIENT_QUICKCHECK_TASK,NULL);
			AUTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_ER:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n))
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_ER,NECESSARY_QUICKCHECK_TASK,NULL);
			ERTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_SINGLEPATHEX:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n) || f->finitepath)
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_SINGLEPATHEX,NECESSARY_QUICKCHECK_TASK,NULL);
			SinglePathExTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_SINGLEPATHALL:
		if(assumedeadlockfree() || noguards(n) || nonblockingguards(n) || f->finitepath)
		{
			// Satisfaction in the skeleton is necessary for
			// satisfaction in the net as long as all deadlocks in the
			// net are deadlocks in the skeleton
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_SINGLEPATHALL,SUFFICIENT_QUICKCHECK_TASK,NULL);
			SinglePathAllTask::buildTask(n,myid,f,fid);
		}
		else
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		break;
	case FORMULA_INITIAL:
		if(f -> containsFireable)
		{
			// for FIREABLE, only a negative answer in the skeleton is
			// also valid in the net.
			if(f -> containsUnfireable)
			{
				UnknownTask::buildTask(n,par,f,myid);
			}
			else
			{
				fid = portfoliomanager::addFormula(f,false);
				tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_INITIAL,NECESSARY_QUICKCHECK_TASK,NULL);
				InitialTask::buildTask(n,myid,f,fid);
				
			}
		}
		else
		{
			if(f -> containsUnfireable)
			{
				fid = portfoliomanager::addFormula(f,false);
				tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_INITIAL,SUFFICIENT_QUICKCHECK_TASK,NULL);
				InitialTask::buildTask(n,myid,f,fid);
			}
			else
			{
				InitialTask::buildTask(n,par,f,myid);
			}
		}
		break;
	case FORMULA_LTL:
		if (RT::args.preference_arg == preference_arg_force_ctl)
		{
			return;
		}
		// LTL is universal -> only pos answer, fir -> only neg answer
		if(f -> containsFireable || (!assumedeadlockfree() && ! noguards(n) && ! nonblockingguards(n) && ! f->finitepath))
		{
			UnknownTask::buildTask(n,par,f,myid);
		}
		else
		{
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_LTL,SUFFICIENT_QUICKCHECK_TASK,NULL);
			LTLTask::buildTask(n,myid,f,fid);
		}
		break;
	case FORMULA_CTL:
		if (RT::args.preference_arg == preference_arg_force_ltl)
		{
			return;
		}
		if(f -> containsExistential && f -> containsUniversal)
		{
			// formula is neither ECTL nor ACTL
			UnknownTask::buildTask(n,par,f,myid);
			return;
		}
		if(!assumedeadlockfree() && ! noguards(n) && ! nonblockingguards(n) && ! f->finitepath)
		{
			// formula not preserved due to possibly hidden deadlocks
			UnknownTask::buildTask(n,par,f,myid);
			return;
		}
		if(f -> containsExistential)
		{

			// formula is an ECTL formula
			// ECTL is existential -> only neg answer, unfir -> only pos answer
			if(f -> containsUnfireable)
			{
				UnknownTask::buildTask(n,par,f,myid);
				return;
			}
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_CTL,NECESSARY_QUICKCHECK_TASK,NULL);
			CTLTask::buildTask(n,myid,f,fid);
			return;
		}
		if(f -> containsUniversal)
		{

			// formula is an ACTL formula
			// ACTL is universal -> only pos answer, fir -> only neg answer
			if(f -> containsFireable)
			{
				UnknownTask::buildTask(n,par,f,myid);
				return;
			}
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_CTL,SUFFICIENT_QUICKCHECK_TASK,NULL);
			CTLTask::buildTask(n,myid,f,fid);
			return;
		}
		break;
	case FORMULA_BOTH:
		if(!assumedeadlockfree() && ! noguards(n) && ! nonblockingguards(n) && ! f->finitepath)
		{
			// formula not preserved due to possibly hidden deadlocks
			UnknownTask::buildTask(n,par,f,myid);
			return;
		}
		// FORMULA_BOTH = LTL or ACTL
		// LTL/ACTL is universal -> only pos answer, fir -> only neg answer
		if(f -> containsFireable)
		{
			UnknownTask::buildTask(n,par,f,myid);
			return;
		}
		if (RT::args.preference_arg == preference_arg_ctl || RT::args.preference_arg == preference_arg_force_ctl)
		{
			// formula is an ACTL formula
			fid = portfoliomanager::addFormula(f,false);
			tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_CTL,SUFFICIENT_QUICKCHECK_TASK,NULL);
			CTLTask::buildTask(n,myid,f,fid);
			return;
		}
		else
		{
			// treat as LTL
			if(f -> containsFireable)
			{
				// for FIREABLE, only a negative answer in the skeleton is
				// also valid in the net.
				if(f -> containsUnfireable)
				{
					UnknownTask::buildTask(n,par,f,myid);
				}
				else
				{
					fid = portfoliomanager::addFormula(f,false);
					tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_LTL,NECESSARY_QUICKCHECK_TASK,NULL);
					LTLTask::buildTask(n,myid,f,fid);
					
				}
			}
			else
			{
                                        fid = portfoliomanager::addFormula(f,false);
                                        tid = portfoliomanager::addTask(n,NULL, par, myid,FORMULA_LTL,NECESSARY_QUICKCHECK_TASK,NULL);
                                        LTLTask::buildTask(n,myid,f,fid);

			}
		}
		break;
	case FORMULA_MODELCHECKING:
		RT::rep->status("checking CTL*");
		RT::rep->message("check not yet implemented");
		UnknownTask::buildTask(n,par,f,myid);
		break;
	default:
		// if not mentioned in case list, property is not
		// preserved, or skeleton approach is not yet supported
		UnknownTask::buildTask(n,par,f,myid);
	}

	return;
}

void Task::parseFormula()
{

	RT::currentInputFile = NULL;

	// Check if the paramter of --formula is a file that we can open: if that
	// works, parse the file. If not, parse the string.
	FILE *file = fopen(RT::args.formula_arg, "r");
	if (file == NULL and (errno == ENOENT or errno == ENAMETOOLONG))
	{
		// reset error
		errno = 0;
		ptformula__scan_string(RT::args.formula_arg);
	}
	else
	{
		fclose(file);
		RT::currentInputFile = new Input("formula", RT::args.formula_arg);
		ptformula_in = *RT::currentInputFile;
		// Save formula input file name for sara problem file name
		RT::inputFormulaFileName = RT::currentInputFile->getFilename();
	}

	// parse the formula
	ptformula_parse();
}

char * Task::formula2string(tFormula f)
{
	pthread_mutex_lock(&kimwitu_mutex);
	unparsed.clear();
	f->unparse(stringprinter, kc::out);
	char * result = new char[strlen(unparsed.c_str())+1];
	strcpy(result, unparsed.c_str());
	pthread_mutex_unlock(&kimwitu_mutex);
	return result;
}

void Task::outputFormulaAsProcessed(tFormula f, int fid)
{
	pthread_mutex_lock(&kimwitu_mutex);
	unparsed.clear();
	f->unparse(stringprinter, kc::out);
	pthread_mutex_unlock(&kimwitu_mutex);
	RT::data["formula"][(const char *) f->id]["text"] = unparsed.c_str();
	RT::data["formula"][(const char *) f->id]["length"] = static_cast<int>(unparsed.size());

			FormulaStatistics *fs = new FormulaStatistics();
			f->fs = fs;
	pthread_mutex_lock(&kimwitu_mutex);
			f->unparse(myprinter, kc::count);
	pthread_mutex_unlock(&kimwitu_mutex);
			fs = f->fs;
			RT::data["formula"][(const char *) f->id]["count"]["A"] = fs->A;
			RT::data["formula"][(const char *) f->id]["count"]["E"] = fs->E;
			RT::data["formula"][(const char *) f->id]["count"]["F"] = fs->F;
			RT::data["formula"][(const char *) f->id]["count"]["G"] = fs->G;
			RT::data["formula"][(const char *) f->id]["count"]["X"] = fs->X;
			RT::data["formula"][(const char *) f->id]["count"]["U"] = fs->U;
			RT::data["formula"][(const char *) f->id]["count"]["tconj"] = fs->tconj;
			RT::data["formula"][(const char *) f->id]["count"]["tdisj"] = fs->tdisj;
			RT::data["formula"][(const char *) f->id]["count"]["tneg"] = fs->tneg;
			RT::data["formula"][(const char *) f->id]["count"]["taut"] = fs->taut;
			RT::data["formula"][(const char *) f->id]["count"]["cont"] = fs->cont;
			RT::data["formula"][(const char *) f->id]["count"]["dl"] = fs->dl;
			RT::data["formula"][(const char *) f->id]["count"]["nodl"] = fs->nodl;
			RT::data["formula"][(const char *) f->id]["count"]["fir"] = fs->fir;
			RT::data["formula"][(const char *) f->id]["count"]["unfir"] = fs->unfir;
			RT::data["formula"][(const char *) f->id]["count"]["comp"] = fs->comp;
			RT::data["formula"][(const char *) f->id]["count"]["aconj"] = fs->aconj;
			RT::data["formula"][(const char *) f->id]["count"]["adisj"] = fs->adisj;
			RT::data["formula"][(const char *) f->id]["count"]["visible_places"] = fs->visible_places;
			RT::data["formula"][(const char *) f->id]["count"]["visible_transitions"] = fs->visible_transitions;
			RT::data["formula"][(const char *) f->id]["count"]["place_references"] = fs->place_references;
			RT::data["formula"][(const char *) f->id]["count"]["transition_references"] = fs->transition_references;
			RT::data["formula"][(const char *) f->id]["count"]["aneg"] = fs->aneg;
}

void Task::outputFormula(void *V)
{
	tFormula F = (tFormula)V;
	pthread_mutex_lock(&kimwitu_mutex);
	unparsed.clear();
	F->unparse(stringprinter, kc::out);
	RT::rep->status("%s", unparsed.c_str());
	pthread_mutex_unlock(&kimwitu_mutex);
	// save already computed atomic formulas in leaf node of parse tree
}

