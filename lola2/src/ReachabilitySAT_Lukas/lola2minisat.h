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

/*!
\file
\author Lukas
\status new

\brief We translate the net into a boolean formula that represents reachability
of a state satisfying a given constraint
*/

#pragma once

#include <ThirdParty/minisat/ThirdParty/zlib/zlib.h>
#include <ThirdParty/minisat/utils/System.h>
#include <ThirdParty/minisat/utils/ParseUtils.h>
#include <ThirdParty/minisat/utils/Options.h>
#include <ThirdParty/minisat/core/Dimacs.h>
#include <ThirdParty/minisat/simp/SimpSolver.h>

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/StatePredicate/MagicNumber.h>

#include <Portfolio/portfoliomanager.h>

#include <vector>
#include <iomanip>

using namespace std;
using namespace Minisat;

// if you change the values, adjust them in minisat2(..)!
typedef enum
{
	REACHABILITY_PROPERTY_TRUE = 0,
	REACHABILITY_PROPERTY_INDETERMINATE = 1,
	REACHABILITY_PROPERTY_FALSE = 2,
	REACHABILITY_PROPERTY_BOUND_BREACHED = 3
} reachability_result_t;

// 0 = there exists a reachable state satisfying the contraint
// 1 = with the given max length, there is no path found, maybe raising the max length would yield a result

class Reachability2Minisat
{
public:
	Reachability2Minisat(Petrinet *n, StatePredicate *f, int p);
	~Reachability2Minisat();
	reachability_result_t lola2minisat(int k, double varbound);
	arrayindex_t GetCurrentMinRounds() { return minRounds; };
	arrayindex_t GetStartRound();

private:
	/// For each transition and atomic predicate we save the change of the formal sum caused by firing said transition
	/// This is also calculated in StatePredicateProperty, but we do this here aswell to avoid unneccessary calculations
	/// from the Property
	int **AtomicChange;

	/// For each place and step we save the amount of variables needed to represent the maximum possible marking
	/// for the place in the step
	/// PlaceVarCount[p][s] therefore equals the amount of variables for place p in step s
	/// saving the 'history' through all steps is only used for adressing variables in minisat and only really
	/// useful if we need to reconstruct the path from the satisfying interpretation
	arrayindex_t **PlaceVarCount;

	int **PlaceNegCount;

	arrayindex_t *PlaceAllowedMax;

	/// For each place we save an overestimate for the minimum Value at two points, first after the first round,
	/// second after the last round
	/// the latter allows us to check for contradictions in fireable/notfireable/deadlock/nodeadlock, while
	/// the former allows us to calculate the minRounds needed to remove said contradictions
	/// similiar to AtomicMinValue, PlaceMinValue[p][0] is the min Value after the first round,
	/// while PlaceMinValue[p][1] is the min Value after the last round;
	/// one has to keep in mind that this value is probably always a big overestimate, since correctly tracking the min
	/// value of a place cannot be expressed exactly without talking about the transition sequence, therefore
	/// this value is always lowered in case a transition would lower the tokencount, but never raises, even
	/// though the value would for example have to raise before another transition lowers it again, based on
	/// needing enough tokens on predecessors
	int **PlaceValues;

	/// For each atomic predicate and step we save the amount of variables needed to represent the maximum possible value
	/// of the formal sum of said predicate in said step
	/// AtomicVarCount[p][s] therefore equals the amount of variables for atomic p in step s
	/// saving the 'history' through all steps is only used for adressing variables in minisat and only really
	/// useful if we need to reconstruct the path from the satisfying interpretation
	arrayindex_t **AtomicVarCount;

	/// For each atomic predicate and step we save the amount of variables that represent possible non positive values
	/// of the formal sum (in other terms, are there possible values < 1, so saving 0 extra counts here)
	/// this is needed for the last step to determine the index of the treshold value
	/// We also save for each step individually to reconstruct the path that minisat finds
	/// AtomicNegCount[c][s] = 6 would mean, that the atomic predicate c has a minimum possible value of -5 in s
	/// while AtomicVarCount[c][s] - AtomicNetCount[c][s] = 5 would additionally mean that the maximum possible
	/// value of c in s would be 4, the remaining 5 variables representing values 1, 2, 3, 4 and 5 but the last needs to be negated,
	/// therefore the max value is 4
	int **AtomicNegCount;

	arrayindex_t *AtomicAllowedMax;

	/// For each atomic we save the MinValue at two points, first after the first round, for easier minChange calc,
	/// second for the last step, to check if the treshold is smaller while creating the constraint
	/// here, AtomicMinValue[c][0] is the minValue of c after the first round, while
	/// AtomicMinValue[c][1] is the minValue of the current (last) step
	int **AtomicMinValue;

	/// We save information if we computed each place for a step
	/// This is useful because the DeltaT arrays of the net allow easy computing of places with any arc to a transition,
	/// but computing the remaining places gets harder, with this array we can simply iterate through the remaining ones.
	/// While computing the remaining places we can reset the already computed places for the next step
	bool *Computed;

	/// Counter for the amount of steps done. If the max path length given is k, then step=0..Card[TR]*k-1
	arrayindex_t step;

	/// We save all atomic predicates, makes working with them easier
	AtomicStatePredicate **Atomic;

	/// Amount of atomic predicates, only saved to reduce redundant calculations
	arrayindex_t AtomicCount;

	/// Maximum path length, given as input
	arrayindex_t PathLength;

	/// Amount of extra variables needed for structure-preserving translation
	arrayindex_t Extras;

	/// Amount of variables needed for everything, makes calculating addresses for extra variables while translating easier
	arrayindex_t OverallVarCount;

	/// Need to save the variables in the last step extra, since overallvarcount + that is needed for extras,
	/// but only overallvarcount is needed to address non extras in predicate calculation
	arrayindex_t LastStepVariableCount;

	/// we just count the amount of fire variables that need to be generated for addressing, since step
	/// with non verbose variables is not indicating that
	arrayindex_t currentfire;

	arrayindex_t *TOrder;

	bool *IsOrdered;

	arrayindex_t TransitionCount;

	vector<StatePredicate *> RenamedSubs;

	double VariableBoundPercentage;

	bool verboseVariables;

	bool isNeverSat;

	Petrinet *net;

	StatePredicate *spFormula;

	int portfolio_id;

	SimpSolver *S;

	arrayindex_t minRounds;

	int argcc;

	char **argvv;

	bool parsedArgs;

	int skippedRounds;

	int PlaceAddress(arrayindex_t p, arrayindex_t i, arrayindex_t step, bool isNext);
	int AtomicAddress(arrayindex_t c, arrayindex_t w, arrayindex_t step, bool isNext);
	int GetAtomicMinRounds(arrayindex_t c);
	int GetPlaceMinRounds(arrayindex_t p, mult_t m, bool needsGreater);
	void create_initial();
	int create_atomic(StatePredicate *f, vec<Lit> &lits);
	int create_fireable(FireablePredicate *formula, vec<Lit> &lits, arrayindex_t extra);
	int create_fireable_and(FireablePredicate *formula, arrayindex_t extra);
	int create_fireable_or(FireablePredicate *formula, vec<Lit> &lits);
	int create_deadlock(arrayindex_t extra);
	int create_nodeadlock(vec<Lit> &lits);
	int create_and(AtomicBooleanPredicate *formula, arrayindex_t extra);
	int create_or(AtomicBooleanPredicate *formula, vec<Lit> &lits);
	void create_phi(arrayindex_t t, bool isEndofFirstRound);
	void create_whole_formula();
	void printAssignments(ostringstream &result);
	void setRoundSkip(long unsigned int t, long unsigned int s);
	void clearMem();
	reachability_result_t run_minisat();
};
