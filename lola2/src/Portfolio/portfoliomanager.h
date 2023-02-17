
#pragma once
#include<Planning/Task.h>
#include<Net/Protonet.h>
#include<Core/Dimensions.h>
#include<Memory/Mara.h>
#include<pthread.h>
#include<ctime>
#include<unistd.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Frontend/Parser/ParserPnml.h>

class hltransition;
class ProtoNet;

class Task;

typedef enum {NOEXIST,    // this entry does not represent a task (for init only)
              IDLE,       // some prerequisite for execution is not yet set
              ACTIVE,     // ready to run
              RUNNNING,    // launched but not yet finished
              WAITING,    // meta task suspended, waiting for subtasks to finish
              OBSOLETE,   // no longer needed for getting a result
              FINISHED,    // already delivered its result
              CANCELED_MEM,    // stopped due to lack of memory
              CANCELED_LOCALTIME,    // stopped due to lack of global time
              CANCELED_GLOBALTIME,    // stopped due to lack of local time
              SUSPENDED,    // stopped due to lack of local memory, but ready to resume
	      CARD_TYPE   // must be last entry, used as number of elements in this enum type
             } taskstatus_t;

typedef enum {EXCL_MEM_TASK,                         // memory intensive -> we should not have two of them running
              CONJUNCTION_TASK,                      // need to && the results of subtasks
              DISJUNCTION_TASK,                      // need to || the results of the subtasks
              SUFFICIENT_QUICKCHECK_TASK,            // subtask = true -> result true, subtask = false -> result unknown
              NECESSARY_QUICKCHECK_TASK,            // subtask = true -> result unknown, subtask = false -> result false
              WRAP_TASK,                            // result = subresult (but interprete result may be different)
              DUAL_TASK,                            // subtask = true -> result = false, subtask = false -> result = true
	      AGGREGATE_TASK,                       // subtask != unknown -> result = subtask

              CONST_TASK, 			 // executable task evaluates at most initial marking
	      EQUATION_TASK,                       // executable task based on linear programming
              SKELETON_TASK,                       // executable task based on skeleton abstraction
	      SIPHON_TASK,                        // executable task based on siphon/trap property
	      SEARCH_TASK,                        // executable task based on state space exploration
	      SAT_TASK,                        // executable task based on SAT checking
	      FINDPATH_TASK,                        // executable task based on memoryless search
	      UNKNOWN_TASK,                        // placeholder task, always return UNKNOWN
	      CARD_STATUS                  // must be the last entry; used as number of elements in this enum type
             } taskattribute_t;

typedef enum
{
		GLOBAL_VOID,
		GLOBAL_ZEROONE,
		GLOBAL_STATEEQUATION,
		GLOBAL_FINDPATH,
		GLOBAL_SEARCH,
		GLOBAL_SYMMETRY,
		GLOBAL_PREPROCESSING,
		GLOBAL_GENERALSEARCH,
		GLOBAL_HL
} globalproducer_t;
// Remark:
// For quickcheck, conjunction, disjunction, and aggregation, we do not need a material task. They will be checked in
// with NULL instead of an actual task object. In contrast, dual tasks are real objects since their
// interprete_result method is needed for reporting.

class statusrecord
{
public:
	int markings;
	int calls;
	char * text;
	int lowerbound;
	int upperbound;
	int tries;
	int persistent;
	int transient;

	statusrecord() 
	{
		markings = 0;
		calls = 0;
		text = NULL;
		lowerbound = 1;
		upperbound = 0;
		tries = 0;
		persistent = 0;
		transient = 0;
	}
};


class portfoliomanager
{
public:

	/// get portfolio manager started
	/// =============================

	static void run();      /// run portfolio manager
	static void runglobal(node_t nodetype); /// run portfolio manager for a global property
	static void init();      /// initialise portfolio manager
	static bool skeletonfinished; ///pre-requisite for launching skeleton based tasks
	static void main_control_loop();   /// the actual portfolio management
	
	/// launch tasks and get reply from tasks
	/// =====================================

	static void report(int,ternary_t); /// check in a Boolean value
	static void reportNumerical(int,int); /// check in a numerical value (compute bound)
	static void launch(Task *, int, int); /// launch a task
	static void derail(int); /// interrupt running task

	/// static description of tasks
	/// ===========================

	// a task without parent is called a "top" task for its formula.
	// such tasks only exist for the original formulas from the
	// user input.
	// tasks with parents concern subformulas (or modified formulas, in case
	// of replacing AG by EF and the like). 
	// Witness paths and witness markings are directly taken from the "delivering" task.
	// Meta tasks do not need to implement these methods.
	
	static formula_t category[MAX_TASKS]; /// the shape of formula handled by this task
	static taskattribute_t attributes[MAX_TASKS]; /// the shape of the task
	static int parent[MAX_TASKS]; /// parent task in task tree
	static Task * tasklist[MAX_TASKS]; /// the actual procedure for the task

	static int addTask(Petrinet *, Task*, int, int, formula_t,taskattribute_t, Mara * mem = NULL) ; /// return is task id, args are task / parent / formula id / attributes

	/// data for running tasks
	/// ======================

	static class statusrecord * executionstatus[MAX_TASKS]; /// data from running task to be displayed
	static taskstatus_t  status[MAX_TASKS]; /// scheduling status
	static pthread_t threadid[MAX_TASKS]; /// thread id for running task;
	static int assignedtime[MAX_TASKS]; /// time assigned for this task
	static int assignedmemory[MAX_TASKS];
	static Mara * memory[MAX_TASKS]; // the memory manager for that task
	static time_t starttime[MAX_TASKS];
	static JSON taskjson[MAX_TASKS];
 	static pid_t childpid[MAX_TASKS]; /// pid of sara process started by this task
	static time_t suspendedtime[MAX_TASKS];

	/// data for finished tasks
	/// =======================
	
	static statusrecord * statistics[MAX_TASKS]; /// data from task to be displayed
	static time_t endtime[MAX_TASKS];
	static int consumedtime[MAX_TASKS];
	static int consumedmemory[MAX_TASKS];

	/// static data for formulas
	/// ========================

	static tFormula formula[MAX_TASKS];
	static Petrinet * pn[MAX_TASKS];  // PN attached to formula
	static ProtoNet * proto[MAX_TASKS]; // Protonet attached to formula
	//static ParserPTNet * symboltable[MAX_TASKS]; // Symboltable (link symbols to indices in pn)
	static char * name[MAX_TASKS]; /// identifying string of formula
	static bool isnumerical[MAX_TASKS]; /// i-th formula is numerical
	static float difficulty[MAX_TASKS]; /// the difficulty of the formula, guessed from the temporal structure
	static int roottask[MAX_TASKS]; /// Task id that is root of task tree for this formula
	static bool visible[MAX_TASKS]; /// whether formula is user given (needs to be reported)

	// a visible formula is a boolean combination of "top" formulas.
	// every top formula gets its own PN and its own skeleton PN, 
	// obtained by its individual local net reduction resp. individual
	// folding. Hence the task tree of a top formula forks into the
	// subtrees for LL verification and skeleton verification, resp.

	static int cardtop;
	static bool top[MAX_TASKS]; /// whether formula is the top element of
				    /// a ll portfolio tree
	static bool topskeleton[MAX_TASKS]; /// whether formula is the 
			// top element of skeleton portfolio tree

	static int addFormula(tFormula,bool vis = false);

	/// data for running formulas
	/// =========================

	
	static int preliminary_origin[MAX_TASKS]; /// the child task id that defined preliminary 	
	static ternary_t preliminary_result[MAX_TASKS]; /// relevant for booleans: result of first subformula 
	static JSON * formulajson[MAX_TASKS];
	static int * available_tasks[MAX_TASKS];

	static void compute_available();

	/// data for finished formulas
	/// ==========================

	static ternary_t result[MAX_TASKS]; /// result[i] is value of i-th formula

	static int numericalresult[MAX_TASKS]; /// result[i] is numerical value of i-th formula (if compute bound)
	static int delivering[MAX_TASKS]; /// Task id that delivered result
	static capacity_t * marking[MAX_TASKS];

	/// data for witnesses
	/// ==================

	static Path  * path[MAX_TASKS];
	/// add a formula

	/// data for available ressources
	/// =============================

	static int nr_of_threads;
	static int running_task[MAX_TASKS];
	static time_t globalstarttime;
        static int memorylimit;
        static int quickcheckmemorylimit;

	/// data for current scheduling situation
	/// =====================================

	static time_t currenttime;
	static int cardrunning;
	static int nr_of_formulas;
	static int nr_of_tasks;
	static int cardvisible; // nr of formulas as announced by task builder
	static int cardpreprocessing; // nr of (top level) tasks for which preprocessing is still running

	/// thread coordination
	/// ===================

	static pthread_mutex_t portfolio_mutex;
	static pthread_cond_t portfolio_cond;
	static pthread_cond_t suspended_cond[MAX_TASKS];
	static bool something_changed;


	/// reporting
	/// =========

	static void * timer_thread(void *);
	static void finalreport();
	static void intermediatereport();
	static const char * tasktype2string(taskattribute_t);
	static int idlength;
	
	/// auxiliary methods
	/// =================

	static tFormula copyFormula(const tFormula ); // make a full copy of formula, including atomic propositions
	static void printTask(int indent, int id);

	/// Global Properties management
 	/// ============================

	static int * symmetrymap[2]; // union/find for found symmetries
	static int * hltrue; // count live (quasilive) firing modes of hl transition
	static int * hlunknown; // count undetermined firing modes of hl transition
	static int hlcardtodo; // number of open hltransitions
	static ternary_t * globalresult; // result vector for the nodes
	static globalproducer_t * globalproducer; // result vector for the nodes
	static taskstatus_t * stateequationstatus;
	static taskstatus_t * searchstatus;
	static taskstatus_t * findpathstatus;
	static int cardtodo;
	static node_t nodetype;
	static bool symmetriesfinished;
	static bool constantplace;
	static ternary_t finalresult;
	static void * launchsymmetrythread(void*);
	static void synchroniseplacevector(ternary_t *,globalproducer_t);
	static void synchronisetransitionvector(ternary_t *,globalproducer_t);
	static void synchroniseplace(arrayindex_t,ternary_t,globalproducer_t);
	static void synchronisetransition(arrayindex_t,ternary_t,globalproducer_t);
	static int symmfind(node_t,int);
	static void symmunion(node_t,int,int);
	static void reportsymmetry();
	static void globalplacereport();
	static void globaltransitionreport();
	static arrayindex_t gettargettransition();
	static arrayindex_t gettargetplace();
	static const char * globalproducer2string(globalproducer_t);
	static void * globalreporter(void *);
	static void mcc_boolean(char * f, ternary_t result);
	static void compareresult(bool);



};

typedef struct 
{
	Mara * memory;
	Task * task;
	int timelimit;
} taskthreadargument;
