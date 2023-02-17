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

#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Core/Runtime.h>
#include <Core/Dimensions.h>
#include <Planning/Task.h>
#include <Net/Net.h>
#include <Net/Marking.h>
#include <Net/Place.h>
#include <Net/Transition.h>
#include <SAT/ReadableParser.hh>
#include <SAT/SATSymbolTable.h>
#include <Witness/Path.h>
#include <cmath>
#include <ctime>
#include <set>

/*!
Bit naming: 
    Places: sSTEPpPLACEbBIT

    Trans.: sSTEPtbBIT
*/

#define SAT_VERBOSE

#ifdef SAT_VERBOSE
#define SAT_COMMENT(text, ...) fprintf(satfile, text, ##__VA_ARGS__)
#else
#define SAT_COMMENT(text, ...) do { } while(0)
#endif

#define SAT_PRINT(text, ...) fprintf(satfile, text, ##__VA_ARGS__)

#define NTH_BIT(number, bit) ((number >> bit) % 2 == 0) ? 'F' : 'T'
#define FORMAT_OR_FALSE(format, bit,  card) { if (bit < card) { fprintf(satfile, format.c_str(), bit); } else { fprintf(satfile, "F"); } }

#define SATFILE_PATH "sat.out"
#define SATFILE_PREFIX_PATH "satprefix.out"
#define MINISAT_LOG "minisat.log"

// naming of transition bits is required in SAT/SATSymbolTable to calculate the witness path from a satisfying interpretation
// declaring a format string for places here would require a whole lot of snprintfs, so it is omitted
#define FORMAT_TRANSITION_BIT "s%utb%u"

class SATTask : public Task {
public:
    static ternary_t satisfiable;
    static arrayindex_t * pathArray;

    SATTask();

    static Task * buildTask() {return new SATTask();}

    ternary_t getResult();

    void interpreteResult(ternary_t result);

    Path getWitnessPath();

    capacity_t *getMarking();

    void getStatistics();

    char * getStatus(uint64_t);

    char * early_abortion(){return NULL;}
private:
    unsigned int step = 0;
    unsigned int steps = 1;
    unsigned int bitsTransitions = 0;

    bool WitnessPathRequired = true;

    time_t minisat_start;

    FILE * satfile;

    arrayindex_t **UnchangedPlaces;
    arrayindex_t *CardUnchangedPlaces;

    arrayindex_t **PrePostIntersect;
    arrayindex_t *CardPrePostIntersect;

    std::set<arrayindex_t> relevantPlaces;

    // collects places that are relevant to the goal
    void collectRelevantPlaces();

    /// collects UnchangedPlaces and PrePostIntersect for later lookups
    void initDataStructures();

    /// encodes initial marking. enclosed in parenthesis
    void encodeInitialMarking();

    /// encodes the addition at a given postplace with a given multiplicity. (starts with &&)
    void encodeAddition(arrayindex_t place, mult_t multipl);

    /// encodes the subtraction at a given postplace with a given multiplicity. (starts with &&)
    void encodeSubtraction(arrayindex_t place, mult_t multipl);

    /// encodes the ">" operator. clause will be enclosed in parenthesis and be satisfiable iff the binary number encoded in formatFirst > the constant
    void encodeGreaterThanConst(std::string formatFirst, int bitsFirst, int constant);

    /// encodes the activation of a given transition. enclosed in parenthesis. Step prefix might be something like "s_goal" -> "s_goal_p_p2_b_4"
    void encodeActivation(std::string stepPrefix, arrayindex_t transition);

    /// encodes index of the transition in variables "sSTEPtbBIT", enclosed in parenthesis.
    void encodeFireTransition(arrayindex_t transition);

    /// all bits of the given place are propagated to the next step
    void encodeKeep(arrayindex_t place);

    // renames variables of the last step to sGOALpPLACEbBIT
    void renameGoalVariables();
    
    /// encodes the product of place and weight as a part of the given goal, result is prefixResult + "layer0b" + BIT. returns amount of bits that encode the result
    int encodeMultiplication(arrayindex_t place, capacity_t weight, std::string prefixResult);

    /// encodes addition of the previous layer and a given variable which is shifted to the left by the specified bits. result in prefixResult + "layer" + layer + "b" + BIT
    void encodeAddShift(int layer, std::string formatVariable, capacity_t bitsVariable, capacity_t bitsPrevious, unsigned int shiftBy, std::string prefixResult);

    /// adds two variables and encodes the result in prefixResult + "b" + BIT
    void encodeVariableAddition(std::string formatFirst, capacity_t bitsFirst, std::string formatSecond, capacity_t bitsSecond, std::string prefixResult);

    /// subtracts variables encoded in formatSecond from formatFirst, result in prefixResult + "b" + BIT
    void encodeVariableSubtraction(std::string formatFirst, capacity_t bitsFirst, std::string formatSecond, capacity_t bitsSecond, std::string prefixResult);

    /// encodes reachability based on TheFormula
    void encodeReachability();

    /// encodes a deadlock
    void encodeDeadlock();

    /// encode the goal. enclosed in parenthesis.
    void encodeGoal();

    /// transforms the formula into CNF and passes it to minisat for checking. 
    void checkSatisfiability();
};

