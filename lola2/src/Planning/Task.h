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

class Mara;
class statusrecord;

#pragma once

#include <Core/Dimensions.h>
#include <Core/mysemaphore.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Net/Petrinet.h>
#include <Memory/Mara.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Portfolio/portfoliomanager.h>

class ParserPTNet;
/*!
\brief the verification task

This class hierarchy encapsulates all information on how to approach the given
problem. An object is built after analysis of the given formula. 
Construction is controlled by several command line parameters.
The idea is that, in the end, there is one task to be executed. However,
some subclasses can wrap several subtasks to executed in parallel or
sequentielly. This way, a property can be explored by combining incomplete
(e.g. structural or findpath) with complete methods. 
The task object is configured and evaluated in the main() method.

In the lifecycle of a task object, the methods will be called
in the order of their specification below.

*/

class Task
{
public:
    Petrinet * net;
    int parent; // task where this task is subtask of. NULL if it has no parent
    int portfolio_id; /// id of this task in portfolio manager
    int formula_id; /// id of formula in portfolio manager
    virtual ~Task(){}
    char * taskname;
    kc::tFormula formula;
    ternary_t result;
    Mara * memory;
    bool goStatus;
    static mysemaphore * foldllnetsemaphore;
    static mysemaphore * bignetsemaphore;
    bool preprocessingfinished;

    /// run the actual verification algorithm
    virtual ternary_t getResult() = 0;    // nontrivial implementation for executable tasks
    /// get result from subtask
    /// interprete and display the result
    virtual char * interpreteResult(ternary_t result) = 0; /// result = value to be verbalised
                                                                    /// dual = dual value needs to be expressed (A->E, E->A)

    /// return the witness path
    virtual Path getWitnessPath() = 0;
    /// return the target marking
    virtual capacity_t *getMarking() = 0;

    /// return the number of stored markings
    virtual statusrecord * getStatistics() = 0;
    static void buildTask(Petrinet *, int, tFormula , int); /// net / parent / formula / formula id 
    static void buildSkeletonTask(Petrinet *, int, tFormula , int); /// parent / formula / formula id
    static threadid_t number_of_threads;
    virtual statusrecord * getStatus() = 0;

 	static void parseFormula();
        static void outputFormulaAsProcessed(tFormula , int); 
        static void outputFormula(void *);
	static char * formula2string(tFormula);
//    static void TopLevelPlanning();
    static void CompoundPlanning(tFormula);
    static void BooleanPlanning(int, tFormula);
     static void LeafPlanning(int, tFormula);
    static void SkeletonCompoundPlanning(tFormula, bool havellnet = false);
    static void SkeletonBooleanPlanning(int, tFormula, bool havellnet = false);
     static void SkeletonLeafPlanning(int, tFormula, bool havellnet = false);
	static int cardsubformula;
	static void* process_skeleton_subformula_havellnet(void*);
	static void* process_skeleton_subformula_havehlnet(void*);
	static void* process_subformula(void*);
	static void* process_initial(void*);
     static bool havellnet;
     static bool havehlnet;
     static bool havehlformula;
     static bool havellformula;
	static Petrinet * GlobalLLNet;
	static ParserPTNet * symboltables;
	static Petrinet * GlobalHLNet;
	static pthread_mutex_t compound_mutex;
     virtual void derail() = 0;
     virtual Task * copy() = 0;
};
