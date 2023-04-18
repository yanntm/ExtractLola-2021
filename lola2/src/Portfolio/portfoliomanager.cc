#include <Portfolio/portfoliomanager.h>
#include <Symmetry/Constraints.h>
#include <Core/Dimensions.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/ParserPnml.h>
#include<iomanip>
#include<ctime>

ProtoNet * portfoliomanager::proto[MAX_TASKS] = {NULL};
//ParserPTNet * portfoliomanager::symboltable[MAX_TASKS] = {NULL};
tFormula portfoliomanager::formula[MAX_TASKS] = {NULL};
bool portfoliomanager::top[MAX_TASKS] = {false};
bool portfoliomanager::topskeleton[MAX_TASKS] = {false};
int portfoliomanager::consumedtime[MAX_TASKS] = {0};
int portfoliomanager::consumedmemory[MAX_TASKS] = {0};
Petrinet * portfoliomanager::pn[MAX_TASKS] = {NULL};
time_t portfoliomanager::suspendedtime[MAX_TASKS];
int portfoliomanager::cardvisible = 0;
int portfoliomanager::cardtop = 0;
formula_t portfoliomanager::category[MAX_TASKS];
time_t portfoliomanager::starttime[MAX_TASKS];
time_t portfoliomanager::endtime[MAX_TASKS];
statusrecord * portfoliomanager::executionstatus[MAX_TASKS] = {NULL};
statusrecord * portfoliomanager::statistics[MAX_TASKS] = {NULL};
Mara * portfoliomanager::memory[MAX_TASKS] = {NULL};
int portfoliomanager::assignedtime[MAX_TASKS] = {0};
int portfoliomanager::assignedmemory[MAX_TASKS] = {0};
bool portfoliomanager::isnumerical[MAX_TASKS] = {false};
pid_t portfoliomanager::childpid[MAX_TASKS];
pthread_t portfoliomanager::threadid[MAX_TASKS];
int portfoliomanager::memorylimit;
int portfoliomanager::quickcheckmemorylimit;
int portfoliomanager::parent[MAX_TASKS] ;
int portfoliomanager::roottask[MAX_TASKS] ;
taskstatus_t portfoliomanager::status[MAX_TASKS] = {IDLE};
taskattribute_t portfoliomanager::attributes[MAX_TASKS];
Task * portfoliomanager::tasklist[MAX_TASKS] = {NULL};
ternary_t portfoliomanager::result[MAX_TASKS] ;
ternary_t portfoliomanager::preliminary_result[MAX_TASKS] ;
int portfoliomanager::preliminary_origin[MAX_TASKS];
int portfoliomanager::numericalresult[MAX_TASKS];
int portfoliomanager::delivering[MAX_TASKS] ;
float portfoliomanager::difficulty[MAX_TASKS];
pthread_mutex_t portfoliomanager::portfolio_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t portfoliomanager::portfolio_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t portfoliomanager::suspended_cond[MAX_TASKS];
int portfoliomanager::nr_of_formulas = 0;
int portfoliomanager::nr_of_tasks = 0;
int portfoliomanager::nr_of_threads = 0;
char * portfoliomanager::name[MAX_TASKS];
bool portfoliomanager::visible[MAX_TASKS];
time_t portfoliomanager::currenttime;
time_t portfoliomanager::globalstarttime;
int portfoliomanager::idlength = 20;
int * portfoliomanager::available_tasks[MAX_TASKS];
JSON * portfoliomanager::formulajson[MAX_TASKS];
JSON portfoliomanager::taskjson[MAX_TASKS];
int * portfoliomanager::symmetrymap[2] = {NULL,NULL};
int * portfoliomanager::hltrue = NULL;
int * portfoliomanager::hlunknown = NULL;
ternary_t * portfoliomanager::globalresult = NULL;
globalproducer_t * portfoliomanager::globalproducer = NULL;
int portfoliomanager::cardtodo = 0;
int portfoliomanager::hlcardtodo = 0;
node_t portfoliomanager::nodetype = PL;
bool portfoliomanager::constantplace = false;

int portfoliomanager::cardrunning = 0;
int portfoliomanager::cardpreprocessing = 0;

capacity_t * portfoliomanager::marking[MAX_TASKS];
Path * portfoliomanager::path[MAX_TASKS];

int lettaskspass = 0;

const char * TECHNIQUES = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT TOPOLOGICAL STATE_COMPRESSION STUBBORN_SETS USE_NUPN UNFOLDING_TO_PT";
const char * TECHNIQUES_UPPER = "TECHNIQUES SEQUENTIAL_PROCESSING TOPOLOGICAL EXPLICIT STATE_COMPRESSION STUBBORN_SETS USE_NUPN UNFOLDING_TO_PT";

void mcc_numerical(char * f, int result)
{
	if(!f) return;
	if(!RT::args.mcc_given) return;

	std::cout << std::endl << "FORMULA " << f << " " << result << " " << TECHNIQUES_UPPER << std::endl;
}

void portfoliomanager::mcc_boolean(char * f, ternary_t result)
{
	const char * formulaname;
	if(!f) 
	{
		switch(RT::args.check_arg)
		{
		case check_arg_QuasiLiveness: f = deconst("QuasiLiveness"); break;
		case check_arg_Liveness: f = deconst("Liveness"); break;
		case check_arg_OneSafe: f = deconst("OneSafe"); break;
		case check_arg_StableMarking: f = deconst("StableMarking"); break;
		case check_arg_deadlockfreedom: f = deconst("ReachabilityDeadlock"); break;
		default: return;
		}
	}
	if(!RT::args.mcc_given) return;
	std::stringstream ss;
	switch(result)
	{
	case TERNARY_TRUE:
		ss << std::endl << "FORMULA " << f << " " << " TRUE " << " " << TECHNIQUES << std::endl;
		break;
	case TERNARY_FALSE:
		ss << std::endl << "FORMULA " << f << " " << " FALSE " << " " << TECHNIQUES << std::endl;
		break;
	default:
		ss << std::endl << "FORMULA " << f << " " <<  " CANNOT_COMPUTE " << " " << TECHNIQUES << std::endl;
	}
	std::cout << ss.str() << std::flush;

}

const char * ternary2string(ternary_t);

int stringtomem(char * s)
{
	int pgbits = 0;
	int pgsize = Mara::pageSize;
	while(! (pgsize & 2))
	{
		pgbits++;
		pgsize /= 2;
	}
	int result;
	char unit;
	sscanf(s,"%u%c",&result,&unit);
	switch(unit)
	{
	case 'K': 	if(pgbits > 10) 
			{
				result /= pgsize;
				result /= 1 << (pgbits - 10);
				if(!result) result = 1;
			}
			else
			{
				result = result << (10 - pgbits);
				result /= pgsize;
				if(!result) result = 1;
			}
			break;
	case 'M': 	if(pgbits > 20) 
			{
				result /= pgsize;
				result /= 1 << (pgbits - 20);
				if(!result) result = 1;
			}
			else
			{
				result = result << (20 - pgbits);
				result /= pgsize;
				if(!result) result = 1;
			}
			break;
	case 'G': 	if(pgbits > 30) 
			{
				result /= pgsize;
				result /= 1 << (pgbits - 30);
				if(!result) result = 1;
			}
			else
			{
				result = result << (30 - pgbits);
				result /= pgsize;
				if(!result) result = 1;
			}
			break;
	case 'T': 	if(pgbits > 40) 
			{
				result /= pgsize;
				result /= 1 << (pgbits - 40);
				if(!result) result = 1;
			}
			else
			{
				result = result << (40 - pgbits);
				result /= pgsize;
				if(!result) result = 1;
			}
			break;
		
	default: RT::rep->status("%s", RT::rep->markup(MARKUP_WARNING, "unknown unit in memory specification: using default").str());
	return 0;
	}
RT::rep->status("MEM LIMIT %d", result);
	return result;
}

const char * portfoliomanager::globalproducer2string(globalproducer_t gp)
{
	switch(gp)
	{
	case GLOBAL_VOID: return "not produced";
	case GLOBAL_ZEROONE: return "0/1 invariant";
	case GLOBAL_STATEEQUATION: return "state equation";
	case GLOBAL_FINDPATH: return "find path";
	case GLOBAL_SEARCH: return "goal oriented search";
	case GLOBAL_SYMMETRY: return "symmetry";
	case GLOBAL_PREPROCESSING: return "preprocessing";
	case GLOBAL_GENERALSEARCH: return "universal search";
	case GLOBAL_HL: return "high level";
	}
}

const char * category2string(formula_t cat)
{
	switch(cat)
	{
	case FORMULA_REACHABLE: return "EF";
	case FORMULA_INVARIANT: return "AG";
    	case FORMULA_LIVENESS:   return "AGEF";
    	case FORMULA_FAIRNESS:   return "GF";
    	case FORMULA_STABILIZATION: return "FG";
    	case FORMULA_EVENTUALLY:   return "F";
    	case FORMULA_INITIAL:  return "INITIAL";
    	case FORMULA_LTL:      return "LTL";
    	case FORMULA_CTL:       return "CTL";
    	case FORMULA_BOTH:      return "LTL/CTL";
    	case FORMULA_MODELCHECKING: return "CTL*";
    	case FORMULA_DEADLOCK:  return "EF DL";
    	case FORMULA_AGEFAG:    return "AGEFAG";
    	case FORMULA_NODEADLOCK: return "AG NODL";
    	case FORMULA_BOUND:     return "BOUND";
    	case FORMULA_CONJUNCTION: return "CONJ";
    	case FORMULA_DISJUNCTION: return "DISJ";
    	case FORMULA_COMPOUND:  return "COMPOUND";
    	case FORMULA_EFAG:     return "EFAG";
    	case FORMULA_EFAGEF:    return "EFAGEF";
    	case FORMULA_EGEF:   return "EGEF";
    	case FORMULA_EFEG:   return "EFEG";
    	case FORMULA_EG:         return "EG";
    	case FORMULA_ER:  return "ER";
    	case FORMULA_EU:   return "EU";
    	case FORMULA_EXEF:   return "EXEF";
    	case FORMULA_EXEG:  return "EXEG";
    	case FORMULA_EXER:    return "EXER";
    	case FORMULA_EXEU: return "EXEU";
    	case FORMULA_AFAG: return "AFAG";
    	case FORMULA_AGAF:  return "AGAF";
    	case FORMULA_AR:   return "AR";
    	case FORMULA_AU:   return "AU";
    	case FORMULA_AXAF:   return "AXAF";
    	case FORMULA_AXAG:   return "AXAG";
    	case FORMULA_AXAR:   return "AXAR";
    	case FORMULA_AXAU:  return "AXAU";
    	case FORMULA_SINGLEPATHEX: return "SP ECTL";
    	case FORMULA_SINGLEPATHALL: return "SP ACTL";
	default: return "UNKNOWN";
	}
}

const char * status2string(taskstatus_t st)
{
	switch(st)
	{
	case	NOEXIST: return "NOEXIST"; break;    
             case IDLE: return "IDLE"; break;       
             case ACTIVE: return "ACTIVE"; break;     
             case RUNNNING: return "RUNNING"; break;    
             case WAITING: return "WAITING"; break;    
            case  OBSOLETE: return "OBSOLETE"; break;   
            case  FINISHED: return "FINISHED"; break;   
             case CANCELED_MEM: return "CANCELED/MEM"; break;    
             case CANCELED_LOCALTIME: return "CANCELED/LTIME"; break;
             case CANCELED_GLOBALTIME: return "CAMCELED/GTIME"; break;
             case SUSPENDED: return "SUSPENDED"; break;    
	default: return "OTHER";
	}
}

void portfoliomanager::init()
{

	// init data structures
	memset(assignedtime,0,MAX_TASKS * sizeof(int));
	memset(assignedmemory,0,MAX_TASKS * sizeof(int));
	memset(endtime,0,MAX_TASKS * sizeof(time_t));
	memset(executionstatus,0,MAX_TASKS * sizeof(class statusrecord *));
	memset(statistics,0,MAX_TASKS * sizeof(class statusrecord *));
	memset(childpid,0,sizeof(pid_t) * MAX_TASKS);
	memset(marking,0,MAX_TASKS * sizeof(capacity_t *));
	memset(consumedtime,0,MAX_TASKS * sizeof(int));
	memset(consumedmemory,0,MAX_TASKS * sizeof(int));
	nr_of_threads = RT::args.threads_arg;

	for(int i = 0; i < MAX_TASKS; i++)
	{
		taskjson[i] = JSON();
		status[i] = IDLE;
		result[i] = TERNARY_VOID;
		preliminary_result[i] = TERNARY_VOID;
		parent[i] = -1;
		roottask[i] = -1;
		delivering[i] = -1;
	}


	// Next milestone is analysis and categorisation of the verification problem
	// If task is compound, this is done in separate threads

	//RT::data["portfolio"]= JSON();
	//RT::data["portfolio"]["task"] = JSON();

	//Task::buildTask(Petrinet::InitialNet,-1,NULL,-1);

	// determine limits
	memorylimit = stringtomem(RT::args.memorylimit_arg);
	quickcheckmemorylimit = stringtomem(RT::args.quickcheckmemorylimit_arg);
	if(!memorylimit) 
	{
		memorylimit = 2000; // 1 TB
	}
	if(!quickcheckmemorylimit) quickcheckmemorylimit = 5; // 5 * 512 MB
}

void portfoliomanager::run()
{
	main_control_loop();
}


int portfoliomanager::addTask(Petrinet * net, Task * newtask, int par,int formulaid, formula_t cat,taskattribute_t attr, Mara * mem )
{
//RT::rep->status("ADDING TASK %s",tasktype2string(attr));
	if(!lettaskspass) pthread_mutex_lock(&portfolio_mutex);
	if(net && net -> isSkeleton && attr == EXCL_MEM_TASK)	attr = SEARCH_TASK;

	// check for task overflow

	int id = formulaid;

	// enter new task
	tasklist[id] = newtask;
	status[id] = IDLE;
	attributes[id] = attr;
	pn[id] = net;
	parent[id] = par;
	memory[id] = mem;
	if(par >= 0) roottask[id] = roottask[par]; else roottask[id] = id;
	if(mem) mem->taskid = id;
	category[id] = cat;
	pthread_cond_init(&suspended_cond[id],NULL);
	//taskjson[id]["id"] = id;
	//taskjson[id]["formula"] = name[roottask[id]] ? name[roottask[id]] : deconst("none");
	//taskjson[id]["name"] = tasklist[id] ? (tasklist[id]->taskname ? tasklist[id]->taskname  : "unnamed") : "unnamed";
	//taskjson[id]["parent"] = parent[id];
	//switch(attributes[id])
	//{
	//case EXCL_MEM_TASK: taskjson[id]["type"] = "exclusive memory task"; break;
	//case SEARCH_TASK: taskjson[id]["type"] = "search task"; break;
	//case SIPHON_TASK: taskjson[id]["type"] = "siphon/trap task"; break;
	//case EQUATION_TASK: taskjson[id]["type"] = "state equation task"; break;
	//case FINDPATH_TASK: taskjson[id]["type"] = "findpath task"; break;
	//case CONST_TASK: taskjson[id]["type"] = "constant task"; break;
	//case SKELETON_TASK: taskjson[id]["type"] = "skeleton task"; break;
	//case AGGREGATE_TASK: taskjson[id]["type"] = "aggregation task"; break;
	//case WRAP_TASK: taskjson[id]["type"] = "wrapper task"; break;
	//case DUAL_TASK: taskjson[id]["type"] = "dual task"; break;
	//case CONJUNCTION_TASK: taskjson[id]["type"] = "conjunction task"; break;
	//case DISJUNCTION_TASK: taskjson[id]["type"] = "disjunction task"; break;
	//case NECESSARY_QUICKCHECK_TASK: taskjson[id]["type"] = "necessary quickcheck task"; break;
	//case SUFFICIENT_QUICKCHECK_TASK: taskjson[id]["type"] = "sufficient quickcheck task"; break;
	//case UNKNOWN_TASK: taskjson[id]["type"] = "placeholder task"; break;
	//default: taskjson[id]["type"] = "other task"; break;
	//}
	if(newtask && pn[id] && pn[id]->isSkeleton) // real task related to skeleton
	{
		char * text = new char[strlen(newtask->taskname) + 200];
		strcpy(text,"skeleton: ");
		strcpy(text + strlen(text),newtask->taskname);
		newtask -> taskname = text;
	}	
	if(attr == UNKNOWN_TASK)
	{
		status[id] = FINISHED;
		starttime[id] = time(NULL);
		endtime[id] = time(NULL);
		result[id] = TERNARY_UNKNOWN;
		delivering[id] = id;
		path[id] = NULL;
		statistics[id] = NULL;
	}
	
	if(!lettaskspass)
	{
		pthread_cond_signal(&portfoliomanager::portfolio_cond);
		pthread_mutex_unlock(&portfolio_mutex);
	}
	return id;
}

float formulapriority(kc::tFormula F, Petrinet * N);
int portfoliomanager::addFormula (kc::tFormula  f, bool vis)
{
	pthread_mutex_lock(&portfolio_mutex);
	if(nr_of_formulas > MAX_TASKS)
	{
		RT::rep->status("maximum number of formulas exceeded: increase MAX_TASKS in Core/Dimensions.h!");
		_exit(1);
	}	
	int id = nr_of_formulas++;
	nr_of_tasks = nr_of_formulas;
	formula[id] = f;
	name[id] = f ? f->id : deconst("(no formula)");
	numericalresult[id] = 0;
	visible[id] = vis;
	if(RT::args.check_arg == check_arg_full || RT::args.check_arg == check_arg_none)
	{
		visible[id] = true;
	}
	if(visible[id] && name[id] && idlength < strlen(name[id]))
	{
		idlength = strlen(name[id]);
	}
	if(name[id]) 
	{
		RT::data["formula"][(const char *)name[id]]["id"] = id;
		formulajson[id] = &RT::data["formula"][(const char *)name[id]];
	}
	else 
	{
		char * text =  new char [100];
		sprintf(text,"---internal subformula--- %d", id);
		name[id] = text;
		//RT::data["formula"][(const char *)text]["id"] = id;
		//formulajson[id] = &RT::data["formula"][(const char *)text];
	}
	available_tasks[id] = new int[CARD_STATUS];
	pthread_cond_signal(&portfoliomanager::portfolio_cond);
	pthread_mutex_unlock(&portfolio_mutex);
	return id;
}

#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>

kc::tFormula portfoliomanager::copyFormula(kc::tFormula  f)
{
        pthread_mutex_lock(&kimwitu_mutex);
	kc::tFormula  newformula = reinterpret_cast<kc::tFormula> (f->copy(true));
	unparsed.clear();
	newformula -> unparse(myprinter,kc::copy);
        pthread_mutex_unlock(&kimwitu_mutex);
	return newformula;
}

float formulapriority(kc::tFormula F, Petrinet * N)
{
	if(!F) return 0.0; // can happen for empty task and full task
	//pthread_mutex_lock(&kimwitu_mutex);
	//F->unparse(myprinter,kc::temporal);
	//pthread_mutex_unlock(&kimwitu_mutex);
	float visiblerank;
	FormulaStatistics * fs = new FormulaStatistics();
        F->fs = fs;
	pthread_mutex_lock(&kimwitu_mutex);
        F->unparse(myprinter,kc::count);
	pthread_mutex_unlock(&kimwitu_mutex);
        fs = F->fs;
	if(N->Card[TR] == 0)
	{
		visiblerank = 0.0;
	}
	else
	{
		visiblerank = (1.0 * fs -> visible_transitions) / N->Card[TR];
	}
	if(N->Card[PL] != 0)
	{
		visiblerank += (10.0 * fs -> visible_places) / N->Card[PL];
	}
	int operators = 5 * fs -> X + fs -> U + fs -> G + fs -> R + fs -> F;

	float result;
	switch(F->type)
	{
	case FORMULA_INITIAL: result =  0.0; break;
	case FORMULA_DEADLOCK: result =  20.0; break;
	case FORMULA_NODEADLOCK: result =  20.0; break;
	case FORMULA_REACHABLE: result =  20.0 + visiblerank; break;
	case FORMULA_INVARIANT: result =  20.0 + visiblerank; break;
	case FORMULA_BOUND: result =  30.0 + visiblerank; break;
	case FORMULA_EFAG: result =  40.0 + visiblerank; break;
	case FORMULA_EFEG: result =  40.0 + visiblerank; break;
	case FORMULA_AGAF: result =  40.0 + visiblerank; break;
	case FORMULA_EGEF: result =  40.0 + visiblerank; break;
	case FORMULA_AFAG: result =  40.0 + visiblerank; break;
	case FORMULA_EG: result =  40.0 + visiblerank; break;
	case FORMULA_EXEG: result =  45.0 + visiblerank; break;
	case FORMULA_AXAF: result =  45.0 + visiblerank; break;
	case FORMULA_LIVENESS: result =  40.0 + visiblerank; break;
	case FORMULA_EFAGEF: result =  40.0 + visiblerank; break;
	case FORMULA_AGEFAG: result =  40.0 + visiblerank; break;
	case FORMULA_EXEF: result =  22.0 + visiblerank; break;
	case FORMULA_AXAG: result =  22.0 + visiblerank; break;
	case FORMULA_EVENTUALLY: result =  40.0 + visiblerank; break;
	case FORMULA_FAIRNESS: result =  50.0 + visiblerank; break;
	case FORMULA_ER: result =  50.0 + visiblerank; break;
	case FORMULA_AU: result =  50.0 + visiblerank; break;
	case FORMULA_AR: result =  50.0 + visiblerank; break;
	case FORMULA_EU: result =  50.0 + visiblerank; break;
	case FORMULA_EXEU: result =  55.0 + visiblerank; break;
	case FORMULA_EXER: result =  55.0 + visiblerank; break;
	case FORMULA_AXAR: result =  55.0 + visiblerank; break;
	case FORMULA_AXAU: result =  55.0 + visiblerank; break;
	case FORMULA_STABILIZATION: result =  50.0 + visiblerank; break;
	case FORMULA_SINGLEPATHEX: result =  60.0 + visiblerank; break;
	case FORMULA_SINGLEPATHALL: result =  60.0 + visiblerank; break;
	case FORMULA_LTL: result =  70.0 + visiblerank; break;
	case FORMULA_CTL: result =  80.0 + visiblerank + operators; break;
	case FORMULA_BOTH: result =  70.0 + visiblerank + operators; break;
	default: result =  200.0 + visiblerank + operators;
	}
	if(F -> containsNext && F -> onlyNext) result = 10.0;
	return result;
}


const char * portfoliomanager::tasktype2string(taskattribute_t ty)
{
	switch(ty)
	{
	case EXCL_MEM_TASK: return "EXCL";
	case CONJUNCTION_TASK: return "CONJ";
	case DISJUNCTION_TASK: return "DISJ";
	case SUFFICIENT_QUICKCHECK_TASK: return "QC/S";
	case NECESSARY_QUICKCHECK_TASK: return "QC/N";
	case WRAP_TASK: return "WRAP";
	case DUAL_TASK: return "DUAL";
	case AGGREGATE_TASK: return "AGGR";
	case CONST_TASK: return "CNST";
	case EQUATION_TASK: return "EQUN";
	case SKELETON_TASK: return "SKEL";
	case SIPHON_TASK: return "SIPH";
	case SEARCH_TASK: return "SRCH";
	case SAT_TASK: return "SAT";
	case FINDPATH_TASK: return "FNDP";
	case UNKNOWN_TASK: return "UNKN";
	default: return "MIXD";
	}
}


void portfoliomanager::printTask(int indent, int id)
{
	for(int i = 0; i < indent; i++) std::cout << " ";
	std::cout << tasktype2string(attributes[id]);
	if(tasklist[id]) std::cout << " " << tasklist[id] -> taskname;
	std::cout << " diff: " << difficulty[id];
	std::cout << std::endl;
	for(int i = 0; i < nr_of_tasks; i++)
	{
		if(parent[i] == id) printTask(indent+4,i);

	}
}

void * execute_task(void * a)
{
	taskthreadargument * args = (taskthreadargument *) a;
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	args->task->getResult();
	return NULL;
}

/// execute task t with memorymanager mem and certain memory and time limit (0 = unlimited)

void portfoliomanager::launch(Task * t, int memlimit, int timelimit)
{
	int taskid = t->portfolio_id;
	starttime[taskid] = time(NULL);
	status[taskid] = RUNNNING;
	assignedtime[taskid] = timelimit;
	assignedmemory[taskid] = memlimit;
		taskjson[t->portfolio_id]["taskname"] = t->taskname;

	if(memory[taskid])
	{
		memory[taskid] -> taskid = taskid;
		memory[taskid] -> pagelimit = memlimit;
	}
	taskthreadargument *args = new taskthreadargument;
	args->task = t;
	args -> timelimit = timelimit;
	int ret = pthread_create(&(portfoliomanager::threadid[taskid]),NULL,execute_task,(void*) args);
	if(ret) 
	{
		RT::rep->status("Error: Could not create thread");
		status[taskid] = CANCELED_MEM;
	}
	RT::rep-> status("LAUNCH task # %d (type %s%s) for %d %s", taskid, pn[taskid]->isSkeleton ? "SKEL/" : "",tasktype2string(attributes[taskid]),roottask[taskid],name[roottask[taskid]]);


	RT::rep->indent(2);
	RT::rep->status("time limit  : %u sec", timelimit);
	RT::rep->status("memory limit: %u pages", memlimit);
	RT::rep->indent(-2);
}

void portfoliomanager::derail(int taskindex)
{
	if(status[taskindex] != SUSPENDED)
	{
		consumedtime[taskindex] += difftime(time(NULL),starttime[taskindex]);
		if(memory[taskindex] && memory[taskindex]-> cardPages > consumedmemory[taskindex])
		{
			consumedmemory[taskindex] = memory[taskindex]->cardPages;
		}
	}
	if(attributes[taskindex] != CONST_TASK)
	{
		if(status[taskindex] != FINISHED) 
		{
			result[taskindex] = TERNARY_UNKNOWN;
		}
		tasklist[taskindex]->derail();
	}
}

void * portfoliomanager::timer_thread(void*)
{
	sleep(5);
	while(true)
	{
		pthread_mutex_lock(&portfolio_mutex);

		// evaluate global time limit
		if(RT::args.timelimit_given)
		{
			if(difftime(time(NULL),globalstarttime) > RT::args.timelimit_arg)
			{
				RT::rep->status("Global time limit exceeded: stopping LoLA");
				for(int taskid = 0; taskid < nr_of_tasks; taskid++ )
				{
					if(status[taskid] == RUNNNING || status[taskid] == SUSPENDED)
					{
						status[taskid] = CANCELED_GLOBALTIME;
						RT::rep->status("CANCELED task # %d (type %s) for %s (global timeout)",taskid,tasktype2string(attributes[taskid]), name[roottask[taskid]]);
						derail(taskid);
					}
				}
				finalreport();
			}
		}

		// evaluate  time limit
		time(&currenttime);
		for(int taskid = 0;taskid < nr_of_tasks; taskid++)
		{
			if(status[taskid] != RUNNNING && status[taskid] != SUSPENDED) continue;
			if(difftime(currenttime,starttime[taskid]) > assignedtime[taskid])

			{
				status[taskid] = CANCELED_LOCALTIME;
				if(childpid[taskid]) kill(childpid[taskid],SIGKILL);
				RT::rep->status("CANCELED task # %d (type %s) for %s (local timeout)",taskid,tasktype2string(attributes[taskid]), name[roottask[taskid]]);
				derail(taskid);
			}
		}

		// create intermediate report
	
		intermediatereport();

		pthread_cond_signal(&portfolio_cond);
		pthread_mutex_unlock(&portfolio_mutex);
		sleep(5);
	}
}

void portfoliomanager::compareresult(bool value)
{
	if(!RT::args.compare_given) return;
	
	char * id;
	if(RT::args.check_arg == check_arg_modelchecking)
	{
		id = ReadPnmlFormulaId();
	}
	else
	{
		id = (char *) malloc(500);
		strcpy(id, RT::args.inputs[0] + 7);
		strcpy(id+ strlen(id) - 11, "-");
	}
	id = (char *) realloc(id, (std::strlen(id) + 1000) * sizeof(char));
	int dash = 0;
	for(int i = 0; i < std::strlen(id); i++)
	{
		if(id[i] == '-') dash++;
		if(dash == 3) 
		{
			id[i+1] = '\0';
			break;
		}
	}
	switch(RT::args.check_arg)
	{
	case check_arg_Liveness:
		strcpy(id + strlen(id), "Liveness");
		break;
	case check_arg_QuasiLiveness:
		strcpy(id + strlen(id), "QuasiLiveness");
		break;
	case check_arg_OneSafe:
		strcpy(id + strlen(id), "OneSafe");
		break;
	case check_arg_deadlockfreedom:
		strcpy(id + strlen(id), "ReachabilityDeadlock");
		break;
	case check_arg_StableMarking:
		strcpy(id + strlen(id), "StableMarking");
		break;
	default: break;
	}
	
	std::ifstream est("globalresults.csv");
	std::string candidate;
	char * comparedstring = NULL;
	while(std::getline(est, candidate))
	{
		const char * ccc = candidate.c_str();
		if(strlen(ccc) < strlen(id)) continue;
		if(!memcmp(ccc,id,strlen(id))) comparedstring = deconst(ccc);
	}
	if(!comparedstring) 
	{
RT::rep->status("NO COMPARISON FOUND %s", id);
		return;
	}
	char compvalue;
	float compconfidence;
	sscanf(comparedstring + strlen(id) + 1,"%c %f", &compvalue, &compconfidence);
	bool difference = false;
	if(value && compvalue == 'F') difference = true;
	if(!value && compvalue == 'T') difference = true;

	RT::rep->status("%s: result %s, estimated %c, confidence %f %s", id, (value ? "true" : "false"), compvalue, compconfidence, (difference ? "###" : ""));
	
}

int portfoliomanager::symmfind(node_t nodetype,int xxx)
{
	int result = xxx;
	while(symmetrymap[nodetype][result] >= 0)
	{
		result = symmetrymap[nodetype][result];
	}
	while(symmetrymap[nodetype][xxx] >= 0)
	{
		arrayindex_t yyy = xxx;
		xxx = symmetrymap[nodetype][xxx];
		symmetrymap[nodetype][yyy] = result;
	}
	return result;
}

void portfoliomanager::symmunion(node_t nodetype,int lll, int rrr)
{
	pthread_mutex_lock(&portfolio_mutex);
	lll = symmfind(nodetype,lll);
	rrr = symmfind(nodetype,rrr);
	if(lll == rrr) 
	{
		pthread_mutex_unlock(&portfolio_mutex);
		return;
	}
	if(symmetrymap[nodetype][lll] < symmetrymap[nodetype][rrr])
	{
		// lll has more elements
		symmetrymap[nodetype][lll] += symmetrymap[nodetype][rrr];
		symmetrymap[nodetype][rrr] = lll;
	}
	else
	{	
		// rrr has more elements
		symmetrymap[nodetype][rrr]+=symmetrymap[nodetype][lll];
		symmetrymap[nodetype][lll] = rrr;
	}
	pthread_mutex_unlock(&portfolio_mutex);
}

void portfoliomanager::globalplacereport()
{
/*
	Petrinet * net = Petrinet::InitialNet;
	RT::rep->status("INDIVIDUAL RESULTS");
	for(int i = 0; i < net -> Card[PL];i++)
	{
		RT::rep->status("    %30s: %15s (by %20s)", net -> Name[PL][i], ternary2string(globalresult[i]), globalproducer2string(globalproducer[i]));
	}
*/
}

void portfoliomanager::globaltransitionreport()
{
	Petrinet * net = Petrinet::InitialNet;
	RT::rep->status("INDIVIDUAL RESULTS");
	for(int i = 0; i < net -> Card[TR];i++)
	{
		RT::rep->status("    %30s: %15s (by %20s) ", net -> Name[TR][i], ternary2string(globalresult[i]), globalproducer2string(globalproducer[i]));
	}
}

void * portfoliomanager::globalreporter(void *)
{
	int sum[10];
	while(true)
	{
		sleep(5);
RT::rep->status("reporting");
		memset(sum,0,sizeof(int)*10);
		for(int i = 0; i < Petrinet::InitialNet->Card[nodetype]; i++)
		{
			sum[globalproducer[i]]++;
		}
		for(int i = 0; i < 10; i++)
		{
			if(sum[i])
			{
				RT::rep->status("%s: %d", globalproducer2string((globalproducer_t)i), sum[i]);
			}
		}
	}
	
}

void portfoliomanager::synchroniseplacevector(ternary_t * placevector, globalproducer_t gp)
{
	pthread_mutex_lock(&portfolio_mutex);

	// 1. fill reported places and their root

	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[PL];i++)
	{
		if(placevector[i] == TERNARY_VOID) continue;
		if(globalresult[i] == TERNARY_VOID) globalproducer[i] = gp;
		globalresult[i] = placevector[i];
		int root = symmfind(PL,i);
		if(globalresult[root] == TERNARY_VOID) globalproducer[root] = GLOBAL_SYMMETRY;
		globalresult[root] = globalresult[i];
	}

	// 2. propagate to whole class


	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[PL];i++)
	{
		if(globalresult[i] == TERNARY_FALSE || globalresult[i] == TERNARY_TRUE) continue;
		int root = symmfind(PL,i);
		if(globalresult[root] != TERNARY_FALSE && globalresult[root] != TERNARY_TRUE) continue;
		globalproducer[i] = GLOBAL_SYMMETRY;
		globalresult[i] = globalresult[root];
	}
	
	if(Task::havehlnet)
	{
		cardtodo = 0;
		for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[PL];i++)
		{
			if(globalresult[i] == TERNARY_VOID) cardtodo++;
			if(globalresult[i] == TERNARY_FALSE)
			{
				globalplacereport();
				if(constantplace)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net has constant places").str());
					RT::rep->status("Example for constant place: %s", Petrinet::InitialNet-> Name[PL][i]);
					compareresult(true);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str());
					RT::rep->status("Example for none-safe place: %s", Petrinet::InitialNet-> Name[PL][i]);
					compareresult(false);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
				}
				_exit(1);
			
			}
		}
		if(!cardtodo)
		{	
			globalplacereport();
			if(constantplace)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net does not have constant places").str());
				compareresult(false);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
			}
			else
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"4 The net is safe").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
			}
			_exit(0);
		}
		memcpy(placevector,globalresult,sizeof(ternary_t) * Petrinet::InitialNet->Card[PL]);
		pthread_mutex_unlock(&portfolio_mutex);
		return ;
	}
	else
	{
		cardtodo = 0;
		for(int i = 0; i < Petrinet::InitialNet -> Card[PL];i++)
		{
			if(globalresult[i] == TERNARY_VOID)
			{
				cardtodo++;
			}
			if(globalresult[i] == TERNARY_FALSE)
			{
				if(constantplace)
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net has a constant place").str());
					compareresult(true);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str());
					compareresult(false);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
				}
				_exit(0);
			}
		}
		if(!cardtodo)
		{
				if(constantplace)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have constant places").str());
					compareresult(false);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"5 The net is safe").str());
					compareresult(true);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
				}
				_exit(0);
		}
		pthread_mutex_unlock(&portfolio_mutex);
		return;
	}
	
}

void portfoliomanager::synchronisetransitionvector(ternary_t * transitionvector, globalproducer_t gp)
{

	pthread_mutex_lock(&portfolio_mutex);
	Petrinet * net = Petrinet::InitialNet;
//RT::rep->status("%s REPORTS", globalproducer2string(gp));
//for(int i = 0; i < Petrinet::InitialNet->Card[TR];i++)
//{
	//if(transitionvector[i] != TERNARY_VOID && transitionvector[i] != TERNARY_UNKNOWN) RT::rep->status("... %s: %s", Petrinet::InitialNet->Name[TR][i], ternary2string(transitionvector[i]));
//}

	// step 1: set value at given transition and at root of its symmetry class
	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[TR];i++)
	{
		if(transitionvector[i] == TERNARY_VOID) continue; // no value reported for this transition
		if(globalresult[i] == TERNARY_VOID) 
		{
			globalproducer[i] = gp;
			globalresult[i] = transitionvector[i];
		}
		int root = symmfind(TR,i);
		if(globalresult[root] == TERNARY_VOID) 
		{
			globalproducer[root] = GLOBAL_SYMMETRY;
			globalresult[root] = transitionvector[i];
		}
		
	}
	// step 2: propagate root value to whole symmetry class
	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[TR];i++)
	{
		int root = symmfind(TR,i);
		if((globalresult[root] == TERNARY_TRUE || globalresult[root] == TERNARY_FALSE) && globalresult[i] == TERNARY_VOID) 
		{
				globalproducer[i] = GLOBAL_SYMMETRY;
				globalresult[i] = globalresult[root];
		}
	}
	// at this stage, we know everything about the values
	// --> check whether task is completed
	if(Task::havehlnet)
	{
		// count status of all HL transitions
		for(int i = 0; i < hltransition::card;i++)
		{
			hltrue[i] = 0;
			hlunknown[i] = 0;
		}
		for(int i = 0; i < Petrinet::InitialNet->Card[TR];i++)
		{
			switch(globalresult[i])
			{
			case TERNARY_TRUE: 
				hltrue[((TransitionSymbol *) (net -> thesymbol[TR][i]))->origin->index]++; 
				break;
			case TERNARY_FALSE: break;
			default: hlunknown[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index]++; 

				break;
			}
		}
		switch(RT::args.check_arg)
		{
		default: break;
		case check_arg_Liveness:
			{
			bool alllive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{
					alllive = false;
					if(hlunknown[i] == 0)
					{
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
						
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(alllive)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
		case check_arg_QuasiLiveness:
			{
			bool allquasilive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{	
					allquasilive = false;
					if(hlunknown[i] == 0)
					{
						globaltransitionreport();
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"A The net is not quasilive").str());
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(allquasilive)
			{
				//globaltransitionreport();
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
		case check_arg_StableMarking:
			// copy quasiliveness into hl transition
			for(int i = 0; i< Petrinet::InitialNet->Card[TR]; i++)
			{
				hltransition * t = ((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin; 
				if(hltrue[t->index] > 0)
				{
					t -> open = false;
					t -> quasilive = true;
				}
				else
				{
					if(hlunknown[t->index] == 0)
					{
						t -> open = false;
						t -> quasilive = false;
					}
					else
					{
						t -> open = true;
					}
				}
			}
			// check place by place
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				p -> open = false;
				p -> stable = true;
			}
			for(int i = 0; i < HLHASHTABLESIZE;i++)
			{
				if(!hltransition::hash_table[i]) continue;
				hltransition * t = hltransition::hash_table[i];
				if((!(t -> open)) && !(t -> quasilive)) continue; // skip dead transitions
				hlarc * a;
				for(a = t -> pre; a ; a = a -> next)
				{
					hlarc * b;
					for(b = t -> post; b; b = b -> next)
					{
						if(a -> place == b -> place) break;
					}
					if(!b || a -> inscription -> nroftokens != b -> inscription -> nroftokens) // t changes p
					{
						hlplace * p = a -> place;
						// t relevant for p
						if(t -> open) // t not yet decided
						{
							if(p -> stable) 
							{
								p -> open = true; // p is open unless already proven to be unstable	

							}
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
					
				}
				for(a = t -> post; a ; a = a -> next)
				{
					hlarc * b;
					for(b = t -> pre; b; b = b -> next)
					{
						if(a -> place == b -> place) break;
					}
					if(!b || a -> inscription -> nroftokens != b -> inscription -> nroftokens) // t changes p
					{
						hlplace * p = a -> place;
						// t relevant for p
						if(t -> open) // t not yet decided
						{
							if(p -> stable) p -> open = true; // p is open unless already proven to be unstable
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
					
				}
			}
			bool haveopenplace = false;
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				if(p -> open) 
				{
					haveopenplace = true;
					continue;
				}
				if(p -> stable)
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"(1) The net has a stable place.").str());
					RT::rep->status("Example: %s", p -> name);
					compareresult(true);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
					_exit(0);
				}
			}
			if(!haveopenplace)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have a stable place.").str());
				compareresult(false);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				_exit(0);
			}
			break;
		}
		memcpy(transitionvector,globalresult,sizeof(ternary_t) * Petrinet::InitialNet->Card[TR]);
		cardtodo = 1;
	}
	else
	{
		cardtodo = 0;
		for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[TR];i++)
		{
			if(globalresult[i] == TERNARY_VOID) 
			{
				cardtodo++;
				continue;
			}
			if(globalresult[i] == TERNARY_FALSE)
			{
				globaltransitionreport();
				if(RT::args.check_arg == check_arg_Liveness)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
					compareresult(false);
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"B The net is not quasilive").str());
					compareresult(false);
				}
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				RT::rep->status("Example for violating transition: %s", Petrinet::InitialNet-> Name[TR][i]);
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
			} 
		}
		if(!cardtodo)
		{	
			//globaltransitionreport();
			if(RT::args.check_arg == check_arg_Liveness)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
			}
			else
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
			}
			
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
			}
			_exit(0);
			pthread_mutex_unlock(&portfolio_mutex);
		}
		memcpy(transitionvector,globalresult,sizeof(ternary_t) * Petrinet::InitialNet->Card[TR]);
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return ;
	
}

void portfoliomanager::synchronisetransition(arrayindex_t tr, ternary_t value, globalproducer_t gp)
{

//RT::rep->status("%s REPORTS", globalproducer2string(gp));
	RT::rep->status("..... %s: %s", Petrinet::InitialNet->Name[TR][tr], ternary2string(value));
	Petrinet * net = Petrinet::InitialNet;
	if(value == TERNARY_VOID || value == TERNARY_UNKNOWN) return;
	pthread_mutex_lock(&portfolio_mutex);

	// step 1: set value at given transition and at root of its symmetry class
	if(globalresult[tr] == TERNARY_VOID) globalproducer[tr] = gp;
	globalresult[tr] = value;
	int root = symmfind(TR,tr);
	if(globalresult[root] == TERNARY_VOID) globalproducer[root] = GLOBAL_SYMMETRY;
	globalresult[root] = value;
	// step 2: propagate root value to whole symmetry class
	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[TR];i++)
	{
		root = symmfind(TR,i);
		if(globalresult[root] != TERNARY_VOID) 
		{
			if(globalresult[i] == TERNARY_VOID) globalproducer[i] = GLOBAL_SYMMETRY;
			globalresult[i] = globalresult[root];
		}
	}
	// at this stage, we know everything about the values
	// --> check whether task is completed
	if(Task::havehlnet)
	{
		// count status of all HL transitions
		for(int i = 0; i < hltransition::card;i++)
		{
			hltrue[i] = 0;
			hlunknown[i] = 0;
		}
		for(int i = 0; i < Petrinet::InitialNet->Card[TR];i++)
		{
			switch(globalresult[i])
			{
			case TERNARY_TRUE: hltrue[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index]++; break;
			case TERNARY_FALSE: break;
			default: hlunknown[((TransitionSymbol *) (net -> thesymbol[TR][i]))->origin->index]++; break;
			}
		}
		switch(RT::args.check_arg)
		{
		default: break;
		case check_arg_Liveness:
			{
			bool alllive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{
					alllive = false;
					if(hlunknown[i] == 0)
					{
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
						
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(alllive)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
		case check_arg_QuasiLiveness:
			{
			bool allquasilive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{	
					allquasilive = false;
					if(hlunknown[i] == 0)
					{
						globaltransitionreport();
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"C The net is not quasilive").str());
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(allquasilive)
			{
				//globaltransitionreport();
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
		case check_arg_StableMarking:
			// copy quasiliveness into hl transition
			for(int i = 0; i< Petrinet::InitialNet->Card[TR]; i++)
			{
				hltransition * t = ((TransitionSymbol *) (net -> thesymbol[TR][i]))->origin;
				if(t->index > 0)
				{
					t -> open = false;
					t -> quasilive = true;
				}
				else
				{
					if(hlunknown[t->index] == 0)
					{
						t -> open = false;
						t -> quasilive = false;
					}
					else
					{
						t -> open = true;
					}
				}
			}
			// check place by place
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				p -> open = false;
				p -> stable = true;
			}
			for(int i = 0; i < HLHASHTABLESIZE;i++)
			{
				if(!hltransition::hash_table[i]) continue;
				hltransition * t = hltransition::hash_table[i];
				if((!(t -> open)) && !(t -> quasilive)) continue;
				hlarc * a;
				for(a = t -> pre; a ; a = a -> next)
				{
					hlarc * b;
					for(b = t -> post; b; b = b -> next)
					{
						if(a -> place == b -> place) break;
					}
					if(!b || a -> inscription -> nroftokens != b -> inscription -> nroftokens)
					{
						hlplace * p = a -> place;
						// t relevant for p
						if(t -> open) // t not yet decided
						{
							if(p -> stable) p -> open = true; // p is open unless already proven to be unstable
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
				}
				for(a = t -> post; a ; a = a -> next)
				{
					hlarc * b;
					for(b = t -> pre; b; b = b -> next)
					{
						if(a -> place == b -> place) break;
					}
					if(!b || a -> inscription -> nroftokens != b -> inscription -> nroftokens)
					{
						hlplace * p = a -> place;
						// t relevant for p
						if(t -> open) // t not yet decided
						{
							if(p -> stable) p -> open = true; // p is open unless already proven to be unstable
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
				}
			}
			bool haveopenplace = false;
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				if(p -> open) 
				{
					haveopenplace = true;
					continue;
				}
				if(p -> stable)
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"(2) The net has a stable place.").str());
					compareresult(true);
					RT::rep->status("Example: %s", p -> name);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
					_exit(0);
				}
			}
			if(!haveopenplace)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have a stable place.").str());
				compareresult(false);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				_exit(0);
			}
			break;
		}
		cardtodo = 1;
	}
	else
	{
		cardtodo = 0;
		for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[TR];i++)
		{
			if(globalresult[i] == TERNARY_VOID) 
			{
				cardtodo++;
				continue;
			}
			if(globalresult[i] == TERNARY_FALSE)
			{
				globaltransitionreport();
				if(RT::args.check_arg == check_arg_Liveness)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
					compareresult(false);
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"D The net is not quasilive").str());
					compareresult(false);
				}
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				RT::rep->status("Example for violating transition: %s", Petrinet::InitialNet-> Name[TR][i]);
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
			} 
		}
		if(!cardtodo)
		{	
			//globaltransitionreport();
			if(RT::args.check_arg == check_arg_Liveness)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
			}
			else
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
			}
			
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
			}
			_exit(0);
			pthread_mutex_unlock(&portfolio_mutex);
		}
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return ;
	
}

void portfoliomanager::synchroniseplace(arrayindex_t pl, ternary_t value, globalproducer_t gp)
{
	if(value == TERNARY_VOID || value == TERNARY_UNKNOWN) return;
	pthread_mutex_lock(&portfolio_mutex);
	if(value == TERNARY_FALSE)
	{
		globalresult[pl] = TERNARY_FALSE;
		globalproducer[pl] = gp;
		globalplacereport();
		if(constantplace)
		{
			RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net has constant places").str());
			compareresult(true);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
			}
			RT::rep->status("Example for constant place: %s", Petrinet::InitialNet -> Name[PL][pl]);
		}
		else
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str());
			compareresult(false);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			RT::rep->status("Example for none-safe place: %s", Petrinet::InitialNet -> Name[PL][pl]);
		}
		_exit(0);
		pthread_mutex_unlock(&portfolio_mutex);
	}
	globalresult[pl] = TERNARY_TRUE;
	globalproducer[pl] = gp;
	arrayindex_t ppp = symmfind(PL,pl);
	if(globalresult[ppp] == TERNARY_VOID)
	{
		globalresult[ppp] = value;
		globalproducer[ppp] = GLOBAL_SYMMETRY;
	}
	for(int i = 0; i < Petrinet::InitialNet->Card[PL];i++)
	{
		if(symmfind(PL,i) == ppp)
		{
			if(globalresult[i] == TERNARY_VOID)
			{
				globalresult[i] = value;
				globalproducer[i] = GLOBAL_SYMMETRY;
			}
		}
	}
	cardtodo = 0; 
	for(int i = 0; i < Petrinet::InitialNet->Card[PL];i++)
	{
		if(globalresult[i] == TERNARY_VOID)
		{
			cardtodo++;
		 }
	}
	if(!cardtodo)
	{
		globalplacereport();
		if(constantplace)
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have constant places").str());
			compareresult(false);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
		}
		else
		{
			RT::rep->status(RT::rep->markup(MARKUP_GOOD,"6 The net is safe").str());
			compareresult(true);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
			}
		}
		_exit(0);
		pthread_mutex_unlock(&portfolio_mutex);
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return;
}

/*
void portfoliomanager::synchronisetransition(arrayindex_t tr, ternary_t value, globalproducer_t gp)
{
	Petrinet * net = Petrinet::InitialNet;
	pthread_mutex_lock(&portfolio_mutex);
	if(Task::havehlnet)
	{
		arrayindex_t ttt = symmfind(TR,tr);
		if(value == TERNARY_FALSE)
		{
			if(globalresult[ttt] == TERNARY_VOID)
			{
				cardtodo--;
				if(!--hlfalse[((TransitionSymbol *) (net -> thesymbol[TR][ttt]))->origin->index])
				{
					globalresult[ttt] = TERNARY_FALSE;
					switch(RT::args.check_arg)
					{
					case check_arg_Liveness:
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					case check_arg_QuasiLiveness:
						globaltransitionreport();
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"E The net is not quasi-live").str());
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					case check_arg_StableMarking:
						// situation: a HL transition just turned out to be dead in all firing modes
						// --> check if that renders some HL place constant

						hltransition * t = ((TransitionSymbol *)(net -> thesymbol[TR][root]))->origin -> index];
						t -> open = false;
						// run through all places, looking if one of them is proven constant now
						for(int i = 0; i < HLHASHTABLESIZE;i++)
						{
							if(!hlplace::hash_table[i]) continue;
							hlplace * p = hlplace::hash_table[i];
							if(!p->open) continue; // place already has quasilive witness transition
							bool placehasopentransition = false;
							for(int j = 0; j < HLHASHTABLESIZE;j++)
							{
								if(!hltransition::hash_table[j]) continue;
								hltransition * tt = hltransition:: hash_table[j];
								if(!tt -> open) continue; 
								hlarc * a;
								hlarc * b;
								for(a = tt -> pre; a ; a = a -> next_pre)
								{
									if(a -> place == p) break;
								}
								for(b = tt -> post; b ; b = b -> next_post)
								{
									if(b -> place == p) break;
								}
								if(!a && !b) continue;
								if(a && b && a -> inscription -> nroftokens == b -> inscription -> nroftokens) continue;
								placehasopentransition = true;
								break;
							
							}
							if(!placehasopentransition)
							{
								RT::rep->status(RT::rep->markup(MARKUP_GOOD,"(3) The net has a stable place.).str());
								compareresult(true);
								RT::rep->status("Example: %s", p -> name);
								if(RT::args.mcc_given)
								{
									mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
								}
								_exit(0);
								pthread_mutex_unlock(&portfolio_mutex);
							}
						}
						break;
i							

					default: 
						RT::rep->status("Unexpected flow of control");
						_exit(1);
					}
				}
			}
			//for(int i = 0; i < net -> Card[TR];i++)
			//{
				//if(globalresult[i] != TERNARY_VOID) continue;
				//if(symmfind(TR,i) != ttt) continue;
				//if(!--hlfalse[transitionmao[i]->origin->index])
				//{
					//globalresult[i] = TERNARY_FALSE;
					//RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live resp. quasilive").str());
					//if(RT::args.mcc_given)
					//{
						//mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					//}
					//_exit(0);
					//pthread_mutex_unlock(&portfolio_mutex);
				//}
			//}
		}
		else if(value == TERNARY_TRUE)
		{
			if(globalresult[ttt] == TERNARY_VOID)
			{
					globalresult[ttt] = TERNARY_TRUE;
					switch(RT::args.check_arg)
					{
					case check_arg_Liveness:
						cardtodo--;
						if(hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][ttt]))->origin->index]> 0)
						{
							hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][ttt]))->origin->index] = -1;
							if(!--hlcardtodo)
							{
								RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
								compareresult(true);
								if(RT::args.mcc_given)
								{
									mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
								}
								_exit(0);
								pthread_mutex_unlock(&portfolio_mutex);
							}
						}
						break;
					case check_arg_QuasiLiveness:
						cardtodo--;
						if(hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][ttt]))->origin->index]> 0)
						{
							hlfalse[net -> thesymbol[TR][ttt]->orign->index] = -1;
							if(!--hlcardtodo)
							{
				//globaltransitionreport();
								RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
								compareresult(true);
								if(RT::args.mcc_given)
								{
									mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
								}
								_exit(0);
								pthread_mutex_unlock(&portfolio_mutex);
							}
						}
						break;
					case check_arg_StableMarking:
						// a transition is quasilive (so is whole HL transition t)
						// all places where t changes token count cease to be candidates for stable place
						hltransition * t = ((TransitionSymbol *)(net -> thesymbol[TR][ttt]))->origin;
						bool haveopenplace = false;
						for(int i = 0; i < HLHASHTABLESIZE;i++)
						{
							if(!hlplace::hash_table[i]) continue;
							hlplace * p = hlplace::hash_table[i];
							if(!(p->open)) continue;
							for(hlarc * a = t -> pre; a; a = a -> next_pre)
							{
								if(a -> place == p) break;
							}
							for(hlarc * b = t -> post; b; b = b -> next_post)
							{
								if(b -> place == p) break;
							}
							if(!a && !b) 
							{
								haveopenplace = true;
								continue;
							}
							if(a && b && a -> inscription -> nroftokens == b -> inscription -> nroftokens) 
							{
								haveopenplace = true;
								continue;
							}
							p -> open = false;
							
						}
						if(!haveopenplace)
						{
								RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net nas no stable place").str());
								compareresult(false);
								if(RT::args.mcc_given)
								{
									mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
								}
								_exit(0);
						}
						break;
					default: RT::rep->status("Unexpected control flow.");
						_exit(1);
					}
			}
			for(int i = 0; i < net->Card[TR];i++)
			{
				if(globalresult[i] != TERNARY_VOID) continue;
				if(symmfind(TR,i) != ttt) continue;
				if(hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index] >= 0)
                                {
					hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index] = -1;
					if(!--hlcardtodo)
					{
						RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live resp. quasilive").str());
								compareresult(true);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
					}
					globalresult[i] = TERNARY_TRUE;
				}
			}
			for(int i = 0; i < net->Card[TR];i++)
			{
				if(globalresult[i] == TERNARY_VOID && hlfalse[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index] < 0)
				{
					globalresult[i] = TERNARY_UNKNOWN;
				}
			}
		}
	}
	else
	{
		arrayindex_t ttt = symmfind(TR,tr);
		if(value == TERNARY_FALSE)
		{
			globalresult[ttt] = TERNARY_FALSE;
			globalproducer[ttt] = gp;
			//globaltransitionreport();
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live resp. quasilive").str());
globaltransitionreport();
			compareresult(false);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			RT::rep->status("Example for none-(quasi-)live transition: %s", Petrinet::InitialNet -> Name[TR][tr]);
			_exit(0);
			pthread_mutex_unlock(&portfolio_mutex);
		}
		if(globalresult[ttt] == TERNARY_VOID && value != TERNARY_VOID)
		{
			globalresult[ttt] = value;
			globalproducer[ttt] = gp;
			cardtodo--;
		}
		if(!cardtodo)
		{
			//globaltransitionreport();
			RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live resp. quasilive").str());
			compareresult(true);
			if(RT::args.mcc_given)
			{
				mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
			}
			_exit(0);
			pthread_mutex_unlock(&portfolio_mutex);
		}
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return;
}
*/

arrayindex_t portfoliomanager::gettargettransition()
{
	pthread_mutex_lock(&portfolio_mutex);
	int rrr = rand() % Petrinet::InitialNet -> Card[TR];
	for(arrayindex_t i = rrr; i < Petrinet::InitialNet->Card[TR];i++)
	{
		if(globalresult[i] == TERNARY_VOID)
		{
				pthread_mutex_unlock(&portfolio_mutex);
				return i;
		}
	}
	for(arrayindex_t i = 0; i < rrr;i++)
	{
		if(globalresult[i] == TERNARY_VOID)
		{
				pthread_mutex_unlock(&portfolio_mutex);
				return i;
		}
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return 0;
}

arrayindex_t portfoliomanager::gettargetplace()
{
	pthread_mutex_lock(&portfolio_mutex);
	if(!cardtodo)
	{
		pthread_mutex_lock(&portfolio_mutex);
		return -1;
	}
	int rrr = rand() % cardtodo;
	for(arrayindex_t i = 0; i < Petrinet::InitialNet->Card[PL];i++)
	{
		if(globalresult[i] == TERNARY_VOID)
		{
			if(!(rrr--)) 
			{
				pthread_mutex_unlock(&portfolio_mutex);
				return i;
			}
		}
	}
	pthread_mutex_unlock(&portfolio_mutex);
	return 0;
}

void portfoliomanager::reportsymmetry()
{
	pthread_mutex_lock(&portfolio_mutex);
	int cardroot = 0;
	for(int i = 0; i < Petrinet::InitialNet -> Card[nodetype]; i++)
	{
	if(symmetrymap[nodetype][i] < 0) cardroot++;
	}

	Petrinet * net = Petrinet::InitialNet;
	
	// update result record

	// phase 1: transfer all firm results to the new canonical representative
	for(int i = 0; i < Petrinet::InitialNet -> Card[nodetype]; i++)
	{
		switch(globalresult[i])
		{
		case TERNARY_VOID: // nothing known so far for this node
		case TERNARY_UNKNOWN:
			break; // nothing to do
		case TERNARY_FALSE:
		case TERNARY_TRUE:
			if(symmetrymap[nodetype][i] >= 0) // not canonical representative
			{
				int root = symmfind(nodetype,i);
				if(globalresult[root] == TERNARY_VOID) globalproducer[root] = GLOBAL_SYMMETRY;
				globalresult[root] = globalresult[i];
			}
			break;
		default: break;
		}
	}

	// phase 2: transfer firm results from root to whole symmetry class
	cardtodo = 0;
	for(int i = 0; i < net -> Card[nodetype];i++)
	{
		int root = symmfind(nodetype,i);
		if(globalresult[root] != TERNARY_TRUE && globalresult[root] != TERNARY_FALSE) 
		{
			continue;
		}
		if(globalresult[i] == TERNARY_VOID) globalproducer[i] = GLOBAL_SYMMETRY;
		globalresult[i] = globalresult[root];
	}

	// evaluate results

	if(Task::havehlnet && RT::args.check_arg != check_arg_OneSafe)
	{
		// count status of all HL transitions
		for(int i = 0; i < hltransition::card;i++)
		{
			hltrue[i] = 0;
			hlunknown[i] = 0;
		}
		for(int i = 0; i < Petrinet::InitialNet->Card[TR];i++)
		{
			switch(globalresult[i])
			{
			case TERNARY_TRUE: hltrue[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index]++; break;
			case TERNARY_FALSE: break;
			default: hlunknown[((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->index]++; break;
			}
		}
		switch(RT::args.check_arg)
		{
		default: break;
		case check_arg_Liveness:
			{
			bool allquasilive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{	
					allquasilive = false;
					if(hlunknown[i] == 0)
					{
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(allquasilive)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
/*
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0 && hlunknown[i] != 0)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
					compareresult(false);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
					_exit(0);
					pthread_mutex_unlock(&portfolio_mutex);
					break;
					
				}
			}
*/
			break;
		case check_arg_QuasiLiveness:
			{
			bool allquasilive = true;
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0)
				{	
					allquasilive = false;
					if(hlunknown[i] == 0)
					{
				globaltransitionreport();
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"F The net is not quasilive").str());
						compareresult(false);
						if(RT::args.mcc_given)
						{
							mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(0);
						pthread_mutex_unlock(&portfolio_mutex);
						break;
					}
				}
				else
				{
					for(int j = 0; j < net -> Card[TR];j++)
					{
						if(((TransitionSymbol *)(net -> thesymbol[TR][j]))->origin->index == i && globalresult[j] == TERNARY_VOID)
						{
							globalresult[j] = TERNARY_UNKNOWN;
							globalproducer[j] = GLOBAL_HL;
						}
					}
				}
			}
			if(allquasilive)
			{
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
				pthread_mutex_unlock(&portfolio_mutex);
				break;
			}
			break;
			}
/*
			for(int i = 0; i < hltransition::card;i++)
			{
				if(hltrue[i] == 0 && hlunknown[i] != 0)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
globaltransitionreport();
					compareresult(false);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
					_exit(0);
					pthread_mutex_unlock(&portfolio_mutex);
					break;
					
				}
			}
*/
			break;
		case check_arg_StableMarking:
			// copy quasiliveness into hl transition
			for(int i = 0; i< hltransition::card; i++)
			{
				hltransition * t = ((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin;
				if(hltrue[t->index] > 0)
				{
					t -> open = false;
					t -> quasilive = true;
				}
				else
				{
					if(hlunknown[t->index] == 0)
					{
						t -> open = false;
						t -> quasilive = false;
					}
					else
					{
						t -> open = true;
					}
				}
			}
			// check place by place
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				p -> open = false;
				p -> stable = true;
			}
			for(int i = 0; i < HLHASHTABLESIZE;i++)
			{
				hltransition * t = ((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin;
				if((!(t -> open)) && !(t -> quasilive)) continue;
				hlarc * a;
				for(a = t -> pre; a ; a = a -> next)
				{
					
				}
				if(a)
				{
					hlarc * b;
					for(b = t -> post; b; b = b -> next)
					{
						if(a -> place == b -> place) break;
					}
					if(!b || a -> inscription -> nroftokens != b -> inscription -> nroftokens)
					{
						hlplace * p = a -> place;
						// t relevant for p
						if(t -> open) // t not yet decided
						{
							if(p -> stable) p -> open = true; // p is open unless already proven to be unstable
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
					
				}
				else
				{
					for(hlarc * b = t -> post; b; b = b -> next)
					{
						// t is relevant for b->place (unidir. arc)
						hlplace * p = b -> place;
						if(t -> open) // t not yet decided
						{
							if(p -> stable) p -> open = true; // p is open unless already proven to be unstable
						}
						else
						{
							if(t -> quasilive)
							{
								p -> open = false;
								p -> stable = false; // p ultimately unstable
							}
						}
					}
				}
			}
			bool haveopenplace = false;
			for(int i = 0; i < HLHASHTABLESIZE; i++)
			{
				if(!hlplace::hash_table[i]) continue;
				hlplace * p = hlplace::hash_table[i];
				if(p -> open) 
				{
					haveopenplace = true;
					continue;
				}
				if(p -> stable)
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"(4) The net has a stable place.").str());
					compareresult(true);
					RT::rep->status("Example: %s", p -> name);
					if(RT::args.mcc_given)
					{
						mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
					_exit(0);
				}
			}
			if(!haveopenplace)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have a stable place.").str());
				compareresult(false);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				_exit(0);
			}
			break;
		}

	}
	else
	{
		// a symmetry cannot bring in a new witness. It can, however,
		// reduce the todo list to 0 and thus prove that there is no witness.
		
		cardtodo = 0;
		for(int i = 0; i < Petrinet::InitialNet->Card[nodetype];i++)
		{
			if(globalresult[i] == TERNARY_VOID)
			{
				cardtodo++;
			}
		}
		if(!cardtodo)
		{
			switch(RT::args.check_arg)
			{
			default: break;
			case check_arg_Liveness:
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			case check_arg_QuasiLiveness:
				//globaltransitionreport();
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			case check_arg_OneSafe:
				RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is one-safe").str());
				compareresult(true);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			case check_arg_StableMarking:
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have a stable place").str());
				compareresult(false);
				if(RT::args.mcc_given)
				{
					mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				_exit(0);
			}
		}
	}
	pthread_mutex_unlock(&portfolio_mutex);
}


void * portfoliomanager::launchsymmetrythread(void *)
{
RT::rep->status("LAUNCH SYMM");
	SymmetryCalculator * SC = new SymmetryCalculator(Petrinet::InitialNet);
	SC -> wantequivalenceclasses = true;
	SC -> ComputeSymmetries(0);

	// TODO: start another stateequation task
	pthread_exit(NULL);
}

void portfoliomanager::main_control_loop()
{
	// prologue: init thread data
	
	if(nr_of_threads <= 1) nr_of_threads = 2;
	int * alreadypassedtime = new int[nr_of_threads];
	int * runningid = new int[nr_of_threads];
	cardrunning = 0;

	// prologue: start timer process
	pthread_t timer_thread_id;
	pthread_create(&timer_thread_id, NULL, timer_thread,NULL);

	pthread_mutex_lock(&portfolio_mutex);
	while(true)
	{
		// launch all constant and unknown tasks, not allocating time & memory (as they shall return soon)
		for(int taskid = 0;  taskid < nr_of_tasks;taskid++)
		{
			if((attributes[taskid] == CONST_TASK && status[taskid] == ACTIVE) || (attributes[taskid] == UNKNOWN_TASK && result[taskid] == TERNARY_VOID))
			{
RT::rep->status("LAUNCH INITIAL");
				launch(tasklist[taskid],0,0);
			}
		}

		// evaluate meta tasks

		bool somethingchanged;
		do
		{
			somethingchanged = false;
			for(int taskid = 0; taskid < nr_of_tasks; taskid++)
			{
				int parentid = parent[taskid];
				if(parentid < 0) continue; // we are at top task

				// proceed if we do not have a value for propagation
				if(result[taskid] == TERNARY_VOID) continue;
				
				// proceed if result of parent already fixed
				if(result[parentid] != TERNARY_VOID) continue;

				// proceed if preliminary result of parent stems from this task
				if(preliminary_origin[parentid] == taskid) continue;
				switch(attributes[parentid])
				{
				case CONJUNCTION_TASK:
					switch(result[taskid])
					{
					case TERNARY_TRUE: 
						switch(preliminary_result[parentid])
						{
						case TERNARY_VOID:
							preliminary_result[parentid] = TERNARY_TRUE;
							preliminary_origin[parentid] = taskid;
							break;
						case TERNARY_UNKNOWN:
						case TERNARY_TRUE:
							result[parentid] = preliminary_result[parentid];
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							somethingchanged = true;
							break;
						case TERNARY_FALSE: 
							result[parentid] = TERNARY_FALSE;
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							break;
						}
						break;
					case TERNARY_FALSE: 
						preliminary_result[parentid] = 
						result[parentid] = TERNARY_FALSE;
						preliminary_origin[parentid] = taskid;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						delivering[parentid] = delivering[taskid];
						somethingchanged = true;
						break;
					case TERNARY_UNKNOWN: 
						switch(preliminary_result[parentid])
						{
						case TERNARY_VOID:
							preliminary_result[parentid] = TERNARY_UNKNOWN;
							preliminary_origin[parentid] = taskid;
							break;
						case TERNARY_UNKNOWN:
						case TERNARY_TRUE:
							result[parentid] = TERNARY_UNKNOWN;
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							somethingchanged = true;
							break;
						case TERNARY_FALSE: 
							result[parentid] = TERNARY_FALSE;
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							break;// cannot happen
						}
					default: break;
					}
					break;
				case DISJUNCTION_TASK:
					switch(result[taskid])
					{
					case TERNARY_FALSE: 

						switch(preliminary_result[parentid])
						{
						case TERNARY_VOID:
							preliminary_result[parentid] = TERNARY_FALSE;
							preliminary_origin[parentid] = taskid;
							break;
						case TERNARY_UNKNOWN:
						case TERNARY_FALSE:
							result[parentid] = preliminary_result[parentid];
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							somethingchanged = true;
							break;
						case TERNARY_TRUE: break;// cannot happen
						}
						break;
					case TERNARY_TRUE: 
						result[parentid] = TERNARY_TRUE;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						delivering[parentid] = delivering[taskid];
						somethingchanged = true;
						break;
					case TERNARY_UNKNOWN: 
						switch(preliminary_result[parentid])
						{
						case TERNARY_VOID:
							preliminary_result[parentid] = TERNARY_UNKNOWN;
							preliminary_origin[parentid] = taskid;
							break;
						case TERNARY_UNKNOWN:
						case TERNARY_FALSE:
							result[parentid] = TERNARY_UNKNOWN;
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
							somethingchanged = true;
							break;
						case TERNARY_TRUE: break;// cannot happen
						}
					default: break;
					}
					break;
				case SUFFICIENT_QUICKCHECK_TASK:
					if(result[taskid] == TERNARY_TRUE)
					{
						result[parentid] = TERNARY_TRUE;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						delivering[parentid] = delivering[taskid];
					}
					else
					{
						result[parentid] = TERNARY_UNKNOWN;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
					}
					somethingchanged = true;
					break;
				case NECESSARY_QUICKCHECK_TASK:
					if(result[taskid] == TERNARY_FALSE)
					{
						result[parentid] = TERNARY_FALSE;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						delivering[parentid] = delivering[taskid];
					}
					else
					{
						result[parentid] = TERNARY_UNKNOWN;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
					}
					somethingchanged = true;
					break;
				case WRAP_TASK:
					result[parentid] = result[taskid];
					if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
					delivering[parentid] = delivering[taskid];
					somethingchanged = true;
					break;
				case AGGREGATE_TASK:
					if(result[taskid] != TERNARY_UNKNOWN)
					{
						result[parentid] = result[taskid];
						delivering[parentid] = delivering[taskid];
						if(isnumerical[taskid])
						{
							numericalresult[parentid] = numericalresult[taskid];
							if(visible[parentid]) mcc_numerical(name[parentid],numericalresult[parentid]);
		
						}
						else
						{
							if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						}
						somethingchanged = true;
					}
					break;
				case DUAL_TASK:
					switch(result[taskid])
					{
					case TERNARY_TRUE: 
						result[parentid] = TERNARY_FALSE;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						break;
					case TERNARY_FALSE: 
						result[parentid] = TERNARY_TRUE;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						break;
					case TERNARY_UNKNOWN: 
						result[parentid] = TERNARY_UNKNOWN;
						if(visible[parentid]) mcc_boolean(name[parentid],result[parentid]);
						break;
					default: assert(false);
					}
					delivering[parentid] = delivering[taskid];
					somethingchanged = true;
					break;
				default: assert(false); // parent cannot be executable
				}
			}
		}
		while(somethingchanged);


		// check for remaining formula
		bool remainingtask = false;
		bool allresults = true;
		for(int i = 0; i < nr_of_formulas;i++)
		{
			if(status[i] == WAITING
                        || status[i] == IDLE
                        || status[i] == RUNNNING
                        || status[i] == SUSPENDED
  	                || status[i] == ACTIVE)
			{
				remainingtask = true;
			}
			if(result[i] == TERNARY_VOID)
			{
				allresults = false;
				break;
			}
		}
		if(allresults && !remainingtask)
		{
			finalreport();

		}

		if(remainingtask)
		{

			// promote task status

			for(int taskid = 0; taskid < nr_of_tasks; taskid++)
			{
				// promote idle task
				if(status[taskid] == IDLE &&
					(attributes[taskid]  == EXCL_MEM_TASK || 
					 attributes[taskid] == CONST_TASK || 
					 attributes[taskid] == SEARCH_TASK || 
					 attributes[taskid] == SAT_TASK || 
					 attributes[taskid] == FINDPATH_TASK ||
					attributes[taskid] == EQUATION_TASK ||
					 attributes[taskid] == SIPHON_TASK)
				&& tasklist[taskid] && tasklist[taskid]->preprocessingfinished
 				&& 	tasklist[taskid]->net->preprocessingfinished)
				{
					status[taskid] = ACTIVE;

					difficulty[taskid] = formulapriority(formula[taskid],pn[taskid]);
				}
				if(status[taskid] == IDLE &&
				   (attributes[taskid]  == CONJUNCTION_TASK || 
				    attributes[taskid] == DISJUNCTION_TASK || 
				    attributes[taskid] == SUFFICIENT_QUICKCHECK_TASK ||
				    attributes[taskid] ==  NECESSARY_QUICKCHECK_TASK ||
				    attributes[taskid] ==  WRAP_TASK || 
				  attributes[taskid] == DUAL_TASK || 
				  attributes[taskid] == AGGREGATE_TASK))
				{
					status[taskid] = WAITING;
				}

				// promote obsolete tasks
				int parentid = parent[taskid];
				if(status[taskid] != OBSOLETE && parentid >= 0 && result[parentid] != TERNARY_VOID)
				{
					
					switch(status[taskid])
					{
					case IDLE:
					case WAITING:
					case ACTIVE: 
						status[taskid] = OBSOLETE;
						result[taskid] = TERNARY_UNKNOWN;
						break;
					case RUNNNING:
					case SUSPENDED:
						derail(taskid);
						status[taskid] = OBSOLETE;
						RT::rep->status("CANCELED task # %d (type %s) for %s (obsolete)",taskid,tasktype2string(attributes[taskid]), name[roottask[taskid]]);
						break;
					default: ;
					}
				}
			}


			// select tasks to be scheduled

			// check whether exclusive task is running
			int haveexclusive = 0;
			bool haverunningexclusive = false;
			int theExclusive;

			cardrunning = 0;
			int cardcandidate = 0;
			int i;
			for(i = 0; i < nr_of_tasks;i++)
			{
				if(status[i] == RUNNNING || status[i] == SUSPENDED)
				{
					cardrunning++;
					cardcandidate++;
					if(attributes[i] == EXCL_MEM_TASK)
					{
						haverunningexclusive = true;
						haveexclusive++;
						theExclusive = i;
					}
				}
				else if(status[i] == IDLE || status[i] == ACTIVE)
				{
					cardcandidate ++;
					if(attributes[i] == EXCL_MEM_TASK)
					{
						haveexclusive++;
					}
				}
			}

			// wake up suspended task && recalculate time limit

			int alreadyrun = 0;
			if(haverunningexclusive)
			{
				if(status[theExclusive] ==  RUNNNING && RT::args.timelimit_given)
				{
					alreadyrun = difftime(time(NULL), starttime[theExclusive]);
					// basis for reschedule is interval from start of this to global end 
					//                         + time already spent in running task
					int remainingtime = RT::args.timelimit_arg + alreadyrun - difftime(time(NULL),globalstarttime);
					remainingtime /= haveexclusive;
					assignedtime[theExclusive] = remainingtime;
				}
				else
				{
					if(status[theExclusive] ==  SUSPENDED &&Mara::freePages) 
					{
						memory[theExclusive]->assignPage();
						status[theExclusive] = RUNNNING;
						if(RT::args.timelimit_given)
						{
							alreadyrun = consumedtime[theExclusive];
							// basis for reschedule is interval from start of this to global end 
							//                         + time already spent in running task
							// we reset start time in order to be able to measure net time consumed
							int remainingtime = RT::args.timelimit_arg + alreadyrun - difftime(time(NULL),globalstarttime);
							remainingtime /= haveexclusive;
							assignedtime[theExclusive] = remainingtime - alreadyrun;
							starttime[theExclusive] = time(NULL);
						}
						pthread_cond_signal(&suspended_cond[theExclusive]);
					}
				}
			}
			
			// select new exclusive task

			compute_available();
			if(!haverunningexclusive && cardrunning < nr_of_threads )
			{
				// no exclusive task running -> check for pending exclusive tasks
				int candidate = nr_of_tasks; // no candidate yet
				int cardcandidates = 0;
				for(i = 0; i < nr_of_tasks; i++)
				{
					if(attributes[i] != EXCL_MEM_TASK) continue;
					if(status[i] == IDLE)
					{
						cardcandidates++;
						continue;
					}
					if(status[i] != ACTIVE) continue;
					cardcandidates++;

					// priority 0: candidate is better than no candidate
					if(candidate == nr_of_tasks) 
					{
						candidate = i;
						continue;
					}

					// priority 1: smallest (remaining) portfolio first
					int available_c = available_tasks[candidate][IDLE] + available_tasks[candidate][ACTIVE] 
						    + available_tasks[candidate][RUNNNING] + available_tasks[candidate][SUSPENDED];
					int available_i = available_tasks[i][IDLE] + available_tasks[i][ACTIVE] + available_tasks[i][RUNNNING] 
						    + available_tasks[i][SUSPENDED];
					if(available_c > available_i)
					{
						// change candidate
						candidate = i;
						continue;
					}
					if(available_c < available_i)
					{
						// keep candidate
						continue;
					}

					// priority 2: smallest difficulty first
					if(difficulty[candidate] < difficulty[i])
					{
						// keep candidate
						continue;
					}
					else
					{
						// change candidate
						candidate = i;
						continue;
					}

				}
				
				// determine time limit for exclusive task
				if(candidate < nr_of_tasks)
				{
					// have candidate
					haverunningexclusive = true;

					int my_timelimit;

					if(RT::args.localtimelimit_given)
					{
						// first choice: given local time limit
						my_timelimit = RT::args.localtimelimit_arg;
					}
					else if(RT::args.timelimit_given)
					{
						// second choice: proportional portion of remaining global time
						int timepassed = difftime(time(NULL),globalstarttime);
						my_timelimit = RT::args.timelimit_arg - timepassed;
						my_timelimit /= cardcandidates;
						if(my_timelimit < 10) my_timelimit = 10;
					}
					else
					{
						my_timelimit = 32000000; // >one year
					}
					// launch exclusive task
					memory[candidate]->reset();
					launch(tasklist[candidate], memorylimit, my_timelimit);
					cardrunning++;
					haverunningexclusive = true;
				}
			}


			// launch nonexclusive tasks
			// step 1: launch tasks with big time limit (step 2: schedule actual time limits)

			while(cardrunning < nr_of_threads)
			{
				// elect one task
				int candidate = nr_of_tasks;
				for(int i = 0; i < nr_of_tasks;i++)
				{
					// rule out all but the quickcheck tasks
					switch(attributes[i])
					{
					case EXCL_MEM_TASK:
					case CONJUNCTION_TASK:
					case DISJUNCTION_TASK:
					case WRAP_TASK:
					case AGGREGATE_TASK:
					case DUAL_TASK:
					case NECESSARY_QUICKCHECK_TASK:
					case SUFFICIENT_QUICKCHECK_TASK:
					case CONST_TASK:  continue;
					default: ;
					}
					// rule out nonactive tasks
					if(status[i] != ACTIVE) continue;
					
					// priority 0: any candidate is better than no candidate
					if(candidate == nr_of_tasks) 
					{
						candidate = i;
						continue;
					}

					// priority 1: support unsupported formula
					if(available_tasks[roottask[i]][RUNNNING] < available_tasks[roottask[candidate]][RUNNNING])
					{
						candidate = i;
						continue;
					}
					if(available_tasks[roottask[i]][RUNNNING] > available_tasks[roottask[candidate]][RUNNNING])
					{
						continue;
					}
		
					// priority 2: large portfolio first
					if(available_tasks[roottask[i]][IDLE] + available_tasks[roottask[i]][ACTIVE] > 
					   available_tasks[roottask[candidate]][IDLE] + available_tasks[roottask[candidate]][ACTIVE])
					{
						candidate = i;
						continue;
					}
					if(available_tasks[roottask[i]][IDLE] + available_tasks[roottask[i]][ACTIVE] < 
					   available_tasks[roottask[candidate]][IDLE] + available_tasks[roottask[candidate]][ACTIVE])
					{
						continue;
					}

					// priority 3: large difficulty first
					if(difficulty[candidate] < difficulty[i])
					{
						candidate = i;
						continue;
					}
					if(difficulty[candidate] > difficulty[i])
					{
						continue;
					}
				}
				if(candidate < nr_of_tasks)
				{
					int temptimelimit;
					if(RT::args.localtimelimit_given)
					{
						temptimelimit = RT::args.localtimelimit_arg;
					}
					else 
					{
						temptimelimit = 32000000; // one year
					}
					
					launch(tasklist[candidate],quickcheckmemorylimit  ,temptimelimit); // we be reduced soon if timelimit:arg is set
					cardrunning++;
				}
				else
				{
					break;
				}
			}


			
			// step 2: compute time limits for running tasks

			if(RT::args.timelimit_given && ! RT::args.localtimelimit_given)
			{
				int currentrunning = 0 ;
				int cardcandidates = 0;
				for(int i = 0; i < nr_of_tasks;i++)
				{
					// rule out all but the quickcheck tasks
					switch(attributes[i])
					{
					case EXCL_MEM_TASK:
					case CONJUNCTION_TASK:
					case DISJUNCTION_TASK:
					case WRAP_TASK:
					case AGGREGATE_TASK:
					case DUAL_TASK:
					case NECESSARY_QUICKCHECK_TASK:
					case SUFFICIENT_QUICKCHECK_TASK:
					case CONST_TASK:  continue;
					default: ;
					}
					// count remaining time of running tasks
					if(status[i] == RUNNNING)
					{
						runningid[currentrunning] = i;
						alreadypassedtime[currentrunning] = difftime(time(NULL), starttime[i]);
						currentrunning++;

					}
					// rule out nonactive tasks
					if(status[i] == ACTIVE || status[i] == IDLE || status[i] == RUNNNING)
					{
						cardcandidates ++;
					}
				}
				int cores = nr_of_threads - (haverunningexclusive ? 1 : 0);
				//simple case: cores for everybody available

				int remainingtime = RT::args.timelimit_arg - difftime(time(NULL), globalstarttime);
				if(cardcandidates <= cores)
				{
					for(int i = 0; i < currentrunning; i++)
					{
						assignedtime[runningid[i]] = remainingtime + alreadypassedtime[i];
					}
				}
				else
				{
					int tasksperthread = cardcandidates / cores; // this many tasks must be scheduled on each core;
					int additionaltasks = cardcandidates - cores * tasksperthread; // number of cores where 
												       // taskprethread + 1 tasks must be run
					// idea: schedule additional tasks on cores with oldest tasks running -> maximise scheduled time

					// --> bubble sort list of running tasks

					for(int i = 0; i < currentrunning - 1; i++)
					{
						int m = i;
						for(int j = i+1;  j < currentrunning; j++)
						{
							if(alreadypassedtime[j] < alreadypassedtime[m])
							{
								m = j;
							}
							if(m != i)
							{
								int temp1 = alreadypassedtime[m];
								alreadypassedtime[m] = alreadypassedtime[i];
								alreadypassedtime[i] = temp1;
								int temp = runningid[m];
								runningid[m] = runningid[i];
								runningid[i] = temp;
							}
						}
					}
				
					// now: running tasks with longest previous run time appear first in list.
					for(int i = 0; i < currentrunning; i++)
					{
						int localtasks = tasksperthread;
						if(i < additionaltasks) localtasks++;
						int localportion = (remainingtime + alreadypassedtime[i]) / localtasks;
						assignedtime[runningid[i]] = localportion - alreadypassedtime[i];
					}
				}
			}
			

			// rewake task if threads are available

			while(cardrunning < nr_of_threads)
			{
				int candidate = nr_of_tasks;
				for(int i = 0; i < nr_of_tasks;i++)
				{
					if(status[i] != CANCELED_LOCALTIME) continue;
					switch(attributes[i])
					{
					case CONJUNCTION_TASK:
					case DISJUNCTION_TASK:
					case WRAP_TASK:
					case AGGREGATE_TASK:
					case DUAL_TASK:
					case NECESSARY_QUICKCHECK_TASK:
					case SUFFICIENT_QUICKCHECK_TASK:
					case CONST_TASK:  continue;
					default: ;
					}
					bool makes_sense = true;
					int ppp = i;
					do
					{
						if(result[ppp] == TERNARY_TRUE || result[ppp] == TERNARY_FALSE)
						{
							makes_sense = false;
							break;
						}
						ppp = parent[ppp];
					}
					while(ppp >=0);
					if(!makes_sense) continue;
					if(candidate == nr_of_tasks) 
					{
						candidate = i;
						continue;
					}
					if(assignedtime[candidate] > assignedtime[i])
					{
						candidate = i;
						continue;
					}
				}
				if(candidate == nr_of_tasks)
				{
					break;
				}
				int mytimelimit;
				if(RT::args.timelimit_given)
				{
					mytimelimit =  RT::args.timelimit_arg - difftime(time(NULL),globalstarttime);
				}
				else
				{
					mytimelimit = 32000000;
				}
				if(attributes[candidate] == EXCL_MEM_TASK) attributes[candidate] = SEARCH_TASK;
				for(int ppp = candidate; ppp >= 0; ppp = parent[ppp])
				{
					result[ppp] = TERNARY_VOID;
				}
				if(memory[candidate]) 
				{
					memory[candidate] -> reset();
					memory[candidate]->stop = false;
				}
				tasklist[candidate]->goStatus = false;
				lettaskspass = 1;
				launch(tasklist[candidate]->copy(),haverunningexclusive ? quickcheckmemorylimit : memorylimit,mytimelimit);
				lettaskspass = 0;
				cardrunning++;
			}



			// check termination

			// termination 1: no open formulas
			int openformulas = 0;
			int opentasks = 0;
			for(int i = 0; i < nr_of_formulas;i++)
			{
				if(visible[i] && result[i] == TERNARY_VOID) 
				{
					openformulas++;
				}
				if(status[i] == IDLE || status[i] == ACTIVE || 
					status[i] == RUNNNING || status[i] == SUSPENDED)
				{
					opentasks++;
				}
			}
			for(int i = 0; i < nr_of_formulas;i++)
			{
				if(!visible[i]) continue;
			}
			if(!openformulas)
			{
	RT::rep->status("Portfolio finished: no open formulas");
				finalreport();
			}

			// termination 2: no open tasks
			if(!cardrunning && !opentasks &&!cardpreprocessing)
			{
	RT::rep->status("Portfolio finished: no open tasks %d ",cardvisible);
				finalreport();

			}
		}

		//  now it is up to the others to do their work
		pthread_cond_wait(&portfolio_cond,&portfolio_mutex);
	}
}

const char * ternary2string(ternary_t res)
{
	switch(res)
	{
	case TERNARY_TRUE: return "true";
	case TERNARY_FALSE: return "false";
	case TERNARY_UNKNOWN: return "unknown";
	case TERNARY_VOID: return "void";
	}
}
void portfoliomanager::report(int taskid, ternary_t res)
{
	pthread_mutex_lock(&portfolio_mutex);
	endtime[taskid] = time(NULL);
	result[taskid] = res;
	status[taskid] = FINISHED;
	delivering[taskid] = taskid;
	if(RT::args.state_given)
	{
		marking[taskid] = tasklist[taskid]->getMarking();
	}
	if(RT::args.path_given)
	{
		path[taskid] = new Path(pn[taskid]);
		* path[taskid] = tasklist[taskid]->getWitnessPath();
	}
	else
	{
		path[taskid] = NULL;
	}
	statistics[taskid] = tasklist[taskid]->getStatistics();
	RT::rep->status("FINISHED task # %d (type %s%s) for %s", taskid, pn[taskid]->isSkeleton ? "SKEL/" : "", tasktype2string(attributes[taskid]),name[roottask[taskid]]);
	if(visible[taskid]) mcc_boolean(name[taskid],res);
	RT::rep->indent(2);
	RT::rep->status("result             : %s", ternary2string(res));
	if(statistics[taskid])
	{
		if(statistics[taskid]->markings)
		{
			RT::rep->status("markings          : %15d", statistics[taskid]->markings);
		}
		if(statistics[taskid]->calls)
		{
			RT::rep->status("fired transitions : %15d", statistics[taskid]->calls);
		}
		if(statistics[taskid]->tries)
		{
			RT::rep->status("tried executions  : %15d", statistics[taskid]->tries);
		}
		if(statistics[taskid]->persistent)
		{
			RT::rep->status("persistent        : %15d", statistics[taskid]->persistent);
		}
		if(statistics[taskid]->transient)
		{
			RT::rep->status("transient         : %15d", statistics[taskid]->transient);
		}
		if(statistics[taskid] -> markings && RT::args.store_arg == store_arg_bloom)
		{
			const double n = static_cast<double>(statistics[taskid]->markings);
			const double k = RT::args.hashfunctions_arg;
			const double m = static_cast<double>(BLOOM_FILTER_SIZE);
			const double probability_false_positive = pow((1.0 - exp((-k * n) / m)), k);
			const double optimal_hash_functions =  log(m / n) / log(2.0);
			RT::rep->status("prob. fals pos: %15f",probability_false_positive);
			RT::rep->status("nr. opt. hash fct.: %15f",optimal_hash_functions);
		}
		RT::rep->status("time used         : %15f", difftime(endtime[taskid],starttime[taskid]));
		RT::rep->status("memory pages used : %15d", memory[taskid] ? memory[taskid]->cardPages : 0);
	}
	RT::rep->indent(-2);
	if(memory[taskid])
	{	
		while(memory[taskid]->mypages)
		{
			pagelist * tmp = memory[taskid]->mypages;
			memory[taskid]->mypages = memory[taskid]->mypages -> next;
			tmp -> next = Mara::freePages;
			Mara::freePages = tmp;
		}
	}
	derail(taskid);

 	pthread_cond_signal(&portfolio_cond);
	pthread_mutex_unlock(&portfolio_mutex);
	pthread_exit(NULL);
	
	
}

void portfoliomanager::reportNumerical(int taskid, int res)
{
RT::rep->status("REPORT NUM");
	pthread_mutex_lock(&portfolio_mutex);
	endtime[taskid] = time(NULL);
	numericalresult[taskid] = res;
	numericalresult[parent[taskid]] = res;
	isnumerical[taskid] = true;
	result[taskid] = TERNARY_TRUE;
	status[taskid] = FINISHED;
	delivering[taskid] = taskid;
	if(RT::args.state_given)
	{
		marking[taskid] = tasklist[taskid]->getMarking();
	}
	if(RT::args.path_given)
	{
		path[taskid] = new Path(pn[taskid]);
		*path[taskid] = tasklist[taskid]->getWitnessPath();
	}
	statistics[taskid] = tasklist[taskid]->getStatistics();
	RT::rep->status("FINISHED task # %d (type %s) for %s", taskid, tasktype2string(attributes[taskid]),name[roottask[taskid]]);
	if(visible[taskid]) mcc_numerical(name[taskid],res);
	RT::rep->indent(2);
	if(statistics[taskid])
	{
		if(statistics[taskid]->markings)
		{
			RT::rep->status("markings          : %15d", statistics[taskid]->markings);
		}
		if(statistics[taskid]->calls)
		{
			RT::rep->status("fired transitions : %15d", statistics[taskid]->calls);
		}
		if(statistics[taskid]->tries)
		{
			RT::rep->status("tried executions  : %15d", statistics[taskid]->tries);
		}
		if(statistics[taskid]->persistent)
		{
			RT::rep->status("persistent        : %15d", statistics[taskid]->persistent);
		}
		if(statistics[taskid]->transient)
		{
			RT::rep->status("transient         : %15d", statistics[taskid]->transient);
		}
		if(statistics[taskid] -> markings && RT::args.store_arg == store_arg_bloom)
		{
			const double n = static_cast<double>(statistics[taskid]->markings);
			const double k = RT::args.hashfunctions_arg;
			const double m = static_cast<double>(BLOOM_FILTER_SIZE);
			const double probability_false_positive = pow((1.0 - exp((-k * n) / m)), k);
			const double optimal_hash_functions =  log(m / n) / log(2.0);
			RT::rep->status("prob. fals pos: %15f",probability_false_positive);
			RT::rep->status("nr. opt. hash fct.: %15f",optimal_hash_functions);
		}
			RT::rep->status("time used         : %15f", difftime(endtime[taskid],starttime[taskid]));
			if(memory[taskid]) RT::rep->status("memory pages used : %15d", memory[taskid]->cardPages);
	}
	RT::rep->indent(-2);
	if(memory[taskid])
	{		
		while(memory[taskid]->mypages)
		{
			pagelist * tmp = memory[taskid]->mypages;
			memory[taskid]->mypages = memory[taskid]->mypages -> next;
			tmp -> next = Mara::freePages;
			Mara::freePages = tmp;
		}
	}
	derail(taskid);

 	pthread_cond_signal(&portfolio_cond);
	pthread_mutex_unlock(&portfolio_mutex);
	pthread_exit(NULL);
	
	
}

void portfoliomanager::intermediatereport()
{
	pthread_mutex_lock(&Reporter::statusmutex);
	compute_available();
	std::ostringstream txt;
	txt  << std::setw(idlength) << "FINISHED FORMULA" << ":" << std::setw(9) << "CATEGORY" << std::setw(9) << "VALUE" << "  " << "PRODUCED BY" ;
	std::string ctxt = txt.str();
	std::cerr << RT::rep->markup(MARKUP_IMPORTANT,ctxt.c_str()).str() << std::endl;
	
	for(int i = 0; i < nr_of_formulas; i++)
	{

		if(!visible[i]) continue;
		if(result[i] == TERNARY_VOID) continue;
		const char * producedby = NULL;
		if(delivering[i]>= 0) producedby = tasklist[delivering[i]]->taskname;
		if(!producedby) producedby = tasktype2string(attributes[i]);
		if(isnumerical[i])
		{
		
			if(result[i] == TERNARY_UNKNOWN)
			{
				std::ostringstream text;
				text  << std::setw(idlength) << name[i] << ":" << std::setw(9) << category2string(category[i]) << std::setw(9) 
				<< "???" << "  " << producedby;
				std::string ctext = text.str();
				std::cerr << RT::rep->markup(MARKUP_WARNING,ctext.c_str()).str();
			}
			else
			{
				std::ostringstream text;
				text << std::setw(idlength) << name[i] << ":" << std::setw(9) << category2string(category[i]) << std::setw(9) 
				<< numericalresult[i] << "  " << producedby;
				std::string ctext = text.str();
				std::cerr << RT::rep->markup(MARKUP_GOOD,ctext.c_str()).str();
			}
		}
		else
		{
			std::ostringstream text;
			text << std::setw(idlength) << name[i] << ":" << std::setw(9) << category2string(category[i]) << 
			std::setw(9) << ternary2string(result[i])<< "  " << producedby;
			std::string ctext = text.str();
			std::cerr << RT::rep->markup(result[i] == TERNARY_TRUE ? MARKUP_GOOD : result[i] == TERNARY_FALSE ? MARKUP_BAD : MARKUP_WARNING,ctext.c_str()).str() ;

		}
		std::cerr << std::endl;
	}
	std::cerr << std::endl;

	std::stringstream txxt;
	txxt << std::setw(idlength) << "PENDING FORMULAS" << ":" << std::setw(9) << "CATEGORY" << std::setw(5) << "IDL" << std::setw(5) << "ACT" << std::setw(5) << "RUN" << std::setw(5) << "SUS" << std::setw(5) << "FIN" << std::setw(5) << "C/T" << std::setw(5) << "C/M" << std::setw(5) << "OBS" ;
	std::string ctxxt = txxt.str();
	std::cerr << RT::rep->markup(MARKUP_IMPORTANT,ctxxt.c_str()).str() << std::endl;
	for(int i = 0; i < nr_of_formulas; i++)
	{
		if(!visible[i]) continue;
		if(result[i] != TERNARY_VOID) continue;
		std::cerr << std::setw(idlength) << name[i] << ":" << std::setw(9) << category2string(category[i]) 
			<< std::setw(5) << available_tasks[i][IDLE] 
			<< std::setw(5) << available_tasks[i][ACTIVE] 
			<< std::setw(5) << available_tasks[i][RUNNNING] 
			<< std::setw(5) << available_tasks[i][SUSPENDED] 
			<< std::setw(5) << available_tasks[i][FINISHED] 
			<< std::setw(5) << available_tasks[i][CANCELED_LOCALTIME] // CANCELED_GLOBALTIME not relevant for intermediate
			<< std::setw(5) << available_tasks[i][CANCELED_MEM] 
			<< std::setw(5) << available_tasks[i][OBSOLETE] 
			<< std::endl;
	}
	std::cerr << std::endl;

	std::stringstream txxxt;
	txxxt << std::right << std::setw(5) << "TASK" << std::setw(9) << "CATEGORY" << std::setw(6) << "TYPE" << std::setw(7) << "TIME" << "/" << std::left << std::setw(7) << "TLIMIT" << std::right << std::setw(7) << "MEM PG" << "/" << std::left << std::setw(7) << "PGLIMIT " << std::setw(idlength+1) << "FORMULA" << "STATUS" ;
	std::string ctxxxt = txxxt.str();
	std::cerr << RT::rep->markup(MARKUP_IMPORTANT,ctxxxt.c_str()).str() << std::endl;
	for(int i = 0; i < nr_of_tasks;i++)
	{
		if(status[i] != RUNNNING && status[i] != SUSPENDED) continue;
		std::cerr << std::right << std::setw(5) << i << std::setw(9) << category2string(category[i]) << std::setw(6) << 
		(attributes[i] == EXCL_MEM_TASK ? "EXCL" :
		attributes[i] == SEARCH_TASK ? "SRCH" :
		attributes[i] == SAT_TASK ? "SAT " :
		attributes[i] == SIPHON_TASK ? "SIPH" :
		attributes[i] == SKELETON_TASK ? "SKEL" :
		attributes[i] == EQUATION_TASK ? "STEQ" :
		attributes[i] == FINDPATH_TASK ? "FNDP" :
		"UNKN")
		<< std::setw(7) << difftime(time(NULL),starttime[i]) << "/" << std::left << std::setw(7) << assignedtime[i] << std::right << std::setw(7) << (memory[i] ? memory[i]-> cardPages : 0) << "/" << std::left << std::setw(7) << assignedmemory[i] <<
		std::right << std::setw(idlength+1) << name[roottask[i]];
		class statusrecord * s = (status[i] == RUNNNING && tasklist[i] -> goStatus) ? tasklist[i]->getStatus() : NULL;
		if(s)
		{
			std::cerr << " ";
			if(s -> markings)
			{
				std::cerr << s -> markings << " m, ";
				int olds = executionstatus[i] ? executionstatus[i] -> markings : 0;
				int mpersec = (s -> markings - olds) / 5;
				std::cerr << mpersec << " m/sec, ";
			}
			if(s-> calls)
			{
				std::cerr << s -> calls << " t fired, ";
			}
			if(s -> lowerbound <= s -> upperbound)
			{
				std::cerr << " result in [" << s->lowerbound << "," << s->upperbound <<"] ";
			}
			if(s -> tries)
			{
				std::cerr << s -> tries << " attempts, ";
			}
			if(s -> persistent)
			{
				std::cerr << s->persistent << " persistent, ";
			}
			if(s -> transient)
			{
				std::cerr << s -> transient << " transient, ";
			}
			if(s -> text)
			{
				std::cerr << " " << s->text;
			}
			if(s -> markings && RT::args.store_arg == store_arg_bloom)
			{
				const double n = static_cast<double>(s->markings);
				const double k = RT::args.hashfunctions_arg;
				const double m = static_cast<double>(BLOOM_FILTER_SIZE);
				const double probability_false_positive = pow((1.0 - exp((-k * n) / m)), k);
				const double optimal_hash_functions =  log(m / n) / log(2.0);
				std::cerr << probability_false_positive << " prob false pos, " << optimal_hash_functions << " opt # hash fct, ";
			}
			std::cerr << "." << std::endl;
			if(executionstatus[i]) delete executionstatus[i];
			executionstatus[i] = s;
		}
		else std::cerr << " --" << std::endl;
	
		
	}
	std::cerr << std::endl << "Time elapsed: " << difftime(time(NULL),globalstarttime) << " secs." << "     Pages in use: " << Mara::PagesInUse << std::endl;
std::cerr << "# running tasks: " << cardrunning << " of " << nr_of_threads << " Visible: " << cardvisible << std::endl;
	pthread_mutex_unlock(&Reporter::statusmutex) ;
}

void portfoliomanager::finalreport()
{
int * estimatedresult;
float * estimatedconfidence;
char ** estimatedstring;

if(RT::args.compare_given)
{
	estimatedresult = new int[nr_of_formulas];
	estimatedconfidence = new float[nr_of_formulas];
	estimatedstring = new char *[nr_of_formulas];
	for(int i = 0; i < nr_of_formulas;i++)
	{
		estimatedresult[i] = -1;
		estimatedconfidence[i] = -1.0;
		estimatedstring[i] = NULL;
	}
	std::ifstream est("results.csv");
	std::string candidate;
	while(std::getline(est, candidate))
	{
		const char * ccc = candidate.c_str();
		for(int i = 0; i < nr_of_formulas;i++)
		{
			if(!visible[i]) continue;
			if(!name[i]) continue;
			if(strlen(ccc) < strlen(name[i])) continue;
			if(!memcmp(ccc,name[i],strlen(name[i]))) estimatedstring[i] = deconst(ccc);
		}
	}

	
}

//for(int i = 0; i < nr_of_formulas; i++)
//{
//	std::cout << i << ": " << tasktype2string(attributes[i]) << " " << status2string(status[i]) << " res " << ternary2string(result[i]) << " par " << parent[i] <<  " " << category2string(category[i]) << std::endl;
//Task::outputFormula(formula[i]);
//}
	for(int i = 0; i < nr_of_tasks;i++)
	{
		if(childpid[i]) kill(childpid[i],SIGKILL);
	}
	// SCREEN REPORT

	pthread_mutex_lock(&Reporter::statusmutex);
	std::stringstream txxxt;
	txxxt << std::endl << "FINAL RESULTS";
	std::string ctxxxt = txxxt.str();
	std::cerr << RT::rep->markup(MARKUP_IMPORTANT,ctxxxt.c_str()).str() << std::endl;
	std::stringstream txt;
	txt << std::setw(idlength) << "FINISHED FORMULA" << ":" << std::setw(9) << "CATEGORY" << std::setw(9) << "VALUE" << std::setw(30) << "PRODUCED BY";
	if(RT::args.compare_given) txt << std::setw(9) << "ESTIMATE" << std::setw(12) << "CONFIDENCE";
	std::string ctxt = txt.str();
	std::cerr << RT::rep->markup(MARKUP_IMPORTANT,ctxt.c_str()).str() << std::endl;
	for(int i = 0; i < nr_of_formulas; i++)
	{

		if(!visible[i]) continue;
		const char * producedby = NULL;
		if(delivering[i]>= 0) producedby = tasklist[delivering[i]]->taskname;
		if(result[i] == TERNARY_VOID) 
		{
			result[i] = TERNARY_UNKNOWN;
		}
		if(!producedby) producedby = tasktype2string(attributes[i]);
		if(isnumerical[i])
		{
		
			if(result[i] == TERNARY_UNKNOWN)
			{
				std::ostringstream text;
				text  << std::setw(idlength) << name[i] << ":" << std::setw(9) << category2string(category[roottask[i]]) << std::setw(9) 
				<< "???" << std::setw(30) << producedby;
				std::string ctext = text.str();
				std::cerr << RT::rep->markup(MARKUP_WARNING,ctext.c_str()).str();
			}
			else
			{
				std::ostringstream text;
				text << std::setw(idlength) << name[i] << ":" << std::setw(9)  << category2string(category[roottask[i]]) << std::setw(9)
				<< numericalresult[i] << std::setw(30) << producedby;
				if(RT::args.compare_given && estimatedstring[i])
				{
					int compvalue;
					float compconfidence;
					sscanf(estimatedstring[i] + strlen(name[i])+1, "%d %f", &compvalue, &compconfidence);
					text << std::setw(9) << compvalue << std::setw(12) << compconfidence;
					if(compvalue >= 0 && numericalresult[i] != compvalue) text << "   ###";
				}
				std::string ctext = text.str();
				std::cerr << RT::rep->markup(MARKUP_GOOD,ctext.c_str()).str();
			}
		}
		else
		{

			std::ostringstream text;
			text << std::setw(idlength) << name[i] << ":" 
			<< std::setw(9) << category2string(category[roottask[i]]) 
			<< std::setw(9) << ternary2string(result[i])<< std::setw(30) << producedby ;
			if(RT::args.compare_given && estimatedstring[i])
			{
				char compvalue;
				float compconfidence;
				sscanf(estimatedstring[i] + strlen(name[i])+1, "%c %f", &compvalue, &compconfidence);
				text << std::setw(9) << (compvalue == 'T' ? "true" : (compvalue == 'F' ? "false" : "unknown")) << std::setw(12) << compconfidence;
				if((compvalue == 'T' && result[i] == false) || (compvalue == 'F' && result[i] == true)) text << "   ###";
			}
			
			std::string ctext = text.str();
			std::cerr << RT::rep->markup(result[i] == TERNARY_TRUE ? MARKUP_GOOD : result[i] == TERNARY_FALSE ? MARKUP_BAD : MARKUP_WARNING,ctext.c_str()).str();

		}
		std::cerr << std::endl;
	}
	std::cerr << std::endl;

	std::cerr << std::endl << "Time elapsed: " << difftime(time(NULL),globalstarttime) << " secs." << "     Pages in use: " << Mara::PagesInUse << std::endl;

	pthread_mutex_unlock(&Reporter::statusmutex);
	// JSON REPORT

	for(int i = 0; i < nr_of_tasks;i++)
	{
		RT::data["portfolio"]["task"] += taskjson[i];
		RT::data["portfolio"]["task"][i]["category"] = category2string(category[i]);
		switch(status[i])
		{
		case IDLE: RT::data["portfolio"]["task"][i]["final status"] = "idle"; break;
		case ACTIVE: RT::data["portfolio"]["task"][i]["final status"] = "active"; break;
		case RUNNNING: RT::data["portfolio"]["task"][i]["final status"] = "running"; break;
		case CANCELED_MEM: RT::data["portfolio"]["task"][i]["final status"] = "canceled (memory)"; break;
		case CANCELED_LOCALTIME: RT::data["portfolio"]["task"][i]["final status"] = "canceled (local time limit)"; break;
		case CANCELED_GLOBALTIME: RT::data["portfolio"]["task"][i]["final status"] = "canceled (global time limit)"; break;
		case OBSOLETE: RT::data["portfolio"]["task"][i]["final status"] = "obsolete"; break;
		case FINISHED: RT::data["portfolio"]["task"][i]["final status"] = "finished"; break;
		case WAITING: RT::data["portfolio"]["task"][i]["final status"] = "waiting"; break;
		case SUSPENDED: RT::data["portfolio"]["task"][i]["final status"] = "suspended"; break;
		default: RT::data["portfolio"]["task"][i]["final status"] = "other"; break;
		}
		if(assignedtime[i] > 0)
		{
			RT::data["portfolio"]["task"][i]["time"]["used"] = (int) consumedtime[i];
			RT::data["portfolio"]["task"][i]["time"]["assigned"] = (int) assignedtime[i];
		}
		else	
		{
			RT::data["portfolio"]["task"][i]["time"]["used"] = "n/a";
			RT::data["portfolio"]["task"][i]["time"]["assigned"] = "n/a";
		}
		if(assignedmemory[i] > 0)
		{
			RT::data["portfolio"]["task"][i]["memory"]["used"] = consumedmemory[i];
			RT::data["portfolio"]["task"][i]["memory"]["assigned"] = memory[i] ? memory[i]->pagelimit : 0;
		}
		else	
		{
			RT::data["portfolio"]["task"][i]["memory"]["used"] = "n/a";
			RT::data["portfolio"]["task"][i]["memory"]["assigned"] = "n/a";
		}
		if(statistics[i])
		{
			if(statistics[i]->markings)
			{
				RT::data["portfolio"]["task"][i]["statistics"]["markings"]= (int) statistics[i]->markings;
			}
			if(statistics[i]->calls)
			{
				RT::data["portfolio"]["task"][i]["statistics"]["calls"]= (int) statistics[i]->calls;
			}
			if(statistics[i]->tries)
			{
				RT::data["portfolio"]["task"][i]["statistics"]["tries"]= (int) statistics[i]->tries;
			}
			if(statistics[i]->persistent)
			{
				RT::data["portfolio"]["task"][i]["statistics"]["persistent"]= (int) statistics[i]->persistent;
			}
			if(statistics[i]->transient)
			{
				RT::data["portfolio"]["task"][i]["statistics"]["transient"]= (int) statistics[i]->transient;
			}
			if(statistics[i] -> markings && RT::args.store_arg == store_arg_bloom)
			{
				const double n = static_cast<double>(statistics[i]->markings);
				const double k = RT::args.hashfunctions_arg;
				const double m = static_cast<double>(BLOOM_FILTER_SIZE);
				const double probability_false_positive = pow((1.0 - exp((-k * n) / m)), k);
				const double optimal_hash_functions =  log(m / n) / log(2.0);
				RT::data["portfolio"]["task"][i]["statistics"]["probability false positive"] = probability_false_positive;
				RT::data["portfolio"]["task"][i]["statistics"]["nr optimal hash functions"] = optimal_hash_functions;
			}

		}
		
	}
	for(int i = 0; i < nr_of_formulas;i++)
	{
		RT::data["formula"][(const char *)name[i]]["category"] = category2string(category[i]);
		if(!visible[i]) continue;
		if(isnumerical[i])
		{
			if(result[i] == TERNARY_VOID)
			{
				RT::data["formula"][(const char *)name[i]]["value"] = "void";
			}
			else
			{
				RT::data["formula"][(const char *)name[i]]["value"] = numericalresult[i];
			}
		}
		else
		{
			switch(result[i])
			{
			case TERNARY_TRUE: RT::data["formula"][(const char *)name[i]]["value"] = "true"; break;
			case TERNARY_FALSE: RT::data["formula"][(const char *)name[i]]["value"] = "false"; break;
			case TERNARY_UNKNOWN: RT::data["formula"][(const char *)name[i]]["value"] = "unknown"; break;
			case TERNARY_VOID: RT::data["formula"][(const char *)name[i]]["value"] = "void"; break;
			}
		}
		RT::data["formula"][(const char *)name[i]]["delivering task"] = delivering[i];
                const char * producedby = NULL;
                if(delivering[i]>= 0) producedby = tasklist[delivering[i]]->taskname;
                if(!producedby) producedby = tasktype2string(attributes[i]);
		RT::data["formula"][(const char *)name[i]]["produced by"] = producedby;
		RT::data["formula"][(const char *)name[i]]["delivering task name"] = (delivering[i] >= 0 && tasklist[delivering[i]]) ? (tasklist[delivering[i]]->taskname ? tasklist[delivering[i]]->taskname : "unnamed" ): "unnamed";
	}
	for(int i = 0; i < nr_of_tasks;i++)
	{
		if(path[i])
		{
			RT::data["formula"][(const char *)name[i]]["witness path"] = path[i]->json();
		}
		else
		{
			RT::data["formula"][(const char *)name[i]]["witness path"] = deconst("no path");
		}
		if (marking[i])
            	{
			RT::data["formula"][(const char *)name[i]]["witness marking"] = JSON();
                	for (arrayindex_t p = 0; p < Petrinet::InitialNet->Card[PL]; ++p)
                	{
                    		if (marking[i][p] == OMEGA)
                    		{
                        		RT::data["formula"][(const char *)name[i]]["witness marking"][Petrinet::InitialNet->Name[PL][p]] = "oo";
                    		}
                    		else if (marking[i][p] > 0)
                    		{
                        		RT::data["formula"][(const char *)name[i]]["witness marking"][Petrinet::InitialNet->Name[PL][p]] = static_cast<int> (marking[i][p]);
                    		}
                	}
            	}
	}
	RT::data["exit"]["time used"] = difftime(time(NULL),globalstarttime);
	RT::data["exit"]["pages used"] = Mara::PagesInUse;
	RT::data["portfolio"]["nr of tasks"] = nr_of_tasks;
	RT::data["portfolio"]["nr of formulas"] = nr_of_formulas;
	RT::data["portfolio"]["nr of threads"] = nr_of_threads;
    if (RT::args.json_given)
    {
        RT::rep->status("print data as JSON (%s)",
                        RT::rep->markup(MARKUP_PARAMETER, "--json").str());

        const std::string jsonstring = RT::data.toString();


        Output o("JSON", RT::args.json_arg);
        fprintf(o, "%s\n", jsonstring.c_str());
    }

////////////////////////////////////////////////

//for(int taskid = 0; taskid < nr_of_tasks; taskid++)
//{
//	std::cout << "id " << taskid  << " " << taskid << " type " << tasktype2string(attributes[taskid]) << " par " << parent[taskid] << " root " << name[roottask[taskid]] << " res " << result[taskid] << std::endl;
//}
////////////////////////////////////////////////
	
 	_exit(0);
}

void portfoliomanager::compute_available()
{
	for(int i = 0; i < nr_of_formulas;i++)
	{
		memset(available_tasks[i],0,CARD_STATUS * sizeof(int));
	}
	for(int j = 0; j < nr_of_tasks;j++)
	{
		if(roottask[j] < 0) continue;
		switch(attributes[j])
		{
		case EXCL_MEM_TASK:
		case SEARCH_TASK:
		case SAT_TASK:
		case EQUATION_TASK:
		case SIPHON_TASK:
		case SKELETON_TASK:
		case FINDPATH_TASK:
		case UNKNOWN_TASK:
			available_tasks[roottask[j]][status[j]]++;
			break;
		default: ;
		}
	}
}


