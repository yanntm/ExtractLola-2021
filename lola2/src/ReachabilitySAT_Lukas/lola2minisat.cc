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

/* We create a CNF formula that is satisfiable if there exists a path
to a marking which satisfies a given set of contraints of the form k^1*p^1+...+k^n*p^n<=k;
these constraints are linked with boolean operators.
According to the theory, for each place p^i there exist variables p^i,j,s, such that
p^i,j,s = 1 and p^i,j+1,s = 0 at step s <==> m^s(p^i) = j, therefore encoding the
marking of places for different steps.
We then introduce a function phi^t(m^s, m^s+1) describing the behaviour of
either stepping from m^s to m^s+1 by firing t or copying m^s to m^s+1.
By encoding the initial marking aswell as the constraint according to theory,
we can then add conjunctions phi^t for every transition n-times, with n
describing the maximum length.
If the resulting formula is satisfiable, there exists a path from the
initial marking to a marking satisfying the constraint.
 */

#include <ReachabilitySAT_Lukas/lola2minisat.h>
#include <sys/time.h>
//#include <chrono>

//using namespace std::chrono;

/// Macros for adressing variables in Minisat
/// Using the macros to call helper functions is only done for consistency, since we
/// don't need functions for everything
#define VAR_PLACE(P, I, S, B) (PlaceAddress(P, I, S, B))
#define VAR_CONS(C, W, S, B) (AtomicAddress(C, W, S, B))
#define VAR_FIRE(S) (S)
#define VAR_EXTRA(E) (OverallVarCount + LastStepVariableCount + E - 1)
// two macros to make VAR_* more readable
#define NEXT true
#define CURR false

// Minisat Options:
//
static IntOption verb = IntOption("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 0, IntRange(0, 2));
static BoolOption pre = BoolOption("MAIN", "pre", "Completely turn on/off any preprocessing.", true);
static StringOption dimacs = StringOption("MAIN", "dimacs", "If given, stop after preprocessing and write the result to this file.");
static IntOption cpu_lim = IntOption("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", std::numeric_limits<int>::max(), IntRange(0, std::numeric_limits<int>::max()));
static IntOption mem_lim = IntOption("MAIN", "mem-lim", "Limit on memory usage in megabytes.\n", std::numeric_limits<int>::max(), IntRange(0, std::numeric_limits<int>::max()));

Reachability2Minisat::Reachability2Minisat(Petrinet *n, StatePredicate *f, int p) : net(n), spFormula(f), portfolio_id(p), parsedArgs(false)
{
	verboseVariables = (bool)RT::args.satverbose_given;
	isNeverSat = false;
	minRounds = 1;
	parsedArgs = false;
	skippedRounds = 0;
	// AtomicChange calculation borrowed from StatePredicateProperty
	AtomicCount = spFormula->countAtomic();
	AtomicChange = new int *[net->Card[TR]];
	Atomic = new AtomicStatePredicate *[AtomicCount];
	spFormula->collectAtomic(Atomic);
	for (arrayindex_t t = 0; t < net->Card[TR]; t++)
	{
		AtomicChange[t] = new int[AtomicCount];
		memset(AtomicChange[t], 0, sizeof(int) * AtomicCount);
		for (arrayindex_t i = 0; i < AtomicCount; i++)
		{
			for (arrayindex_t j = 0; j < net->CardDeltaT[PRE][t]; j++)
			{
				const arrayindex_t p = net->DeltaT[PRE][t][j];
				for (arrayindex_t k = 0; k < Atomic[i]->cardPos; k++)
				{
					if (Atomic[i]->posPlaces[k] == p)
					{
						AtomicChange[t][i] -= net->MultDeltaT[PRE][t][j] * Atomic[i]->posMult[k];
						break;
					}
				}
				for (arrayindex_t k = 0; k < Atomic[i]->cardNeg; k++)
				{
					if (Atomic[i]->negPlaces[k] == p)
					{
						AtomicChange[t][i] += net->MultDeltaT[PRE][t][j] * Atomic[i]->negMult[k];
						break;
					}
				}
			}
			for (arrayindex_t j = 0; j < net->CardDeltaT[POST][t]; j++)
			{
				const arrayindex_t p = net->DeltaT[POST][t][j];
				for (arrayindex_t k = 0; k < Atomic[i]->cardPos; k++)
				{
					if (Atomic[i]->posPlaces[k] == p)
					{
						AtomicChange[t][i] += net->MultDeltaT[POST][t][j] * Atomic[i]->posMult[k];
						break;
					}
				}
				for (arrayindex_t k = 0; k < Atomic[i]->cardNeg; k++)
				{
					if (Atomic[i]->negPlaces[k] == p)
					{
						AtomicChange[t][i] -= net->MultDeltaT[POST][t][j] * Atomic[i]->negMult[k];
						break;
					}
				}
			}
		}
	}
	PlaceAllowedMax = new arrayindex_t[net->Card[PL]];
	AtomicAllowedMax = new arrayindex_t[AtomicCount];
	Computed = new bool[net->Card[PL]];
	memset(Computed, 0, net->Card[PL] * sizeof(bool));

	PlaceVarCount = new arrayindex_t *[net->Card[PL]];
	PlaceNegCount = new int *[net->Card[PL]];
	PlaceValues = new arrayindex_t *[net->Card[PL]];

	AtomicVarCount = new arrayindex_t *[AtomicCount];
	AtomicNegCount = new int *[AtomicCount];
	AtomicMinValue = new int *[AtomicCount];

	for (arrayindex_t p = 0; p < net->Card[PL]; p++)
	{
		PlaceValues[p] = new int[4];
		PlaceValues[p][1] = net->Initial[p];
		PlaceValues[p][3] = net->Initial[p];
		PlaceValues[p][2] = -1;
		PlaceValues[p][0] = -1;
		if (!verboseVariables)
		{
			PlaceVarCount[p] = new arrayindex_t[2];
			PlaceNegCount[p] = new int[2];
		}
	}
	NetState *initial = NetState::createNetStateFromInitial(net);
	for (arrayindex_t a = 0; a < AtomicCount; a++)
	{
		Atomic[a]->evaluate(*initial);
		AtomicMinValue[a] = new int[2];
		AtomicMinValue[a][1] = Atomic[a]->sum;
		AtomicMinValue[a][0] = -1;
		if (!verboseVariables)
		{
			AtomicVarCount[a] = new arrayindex_t[3];
			AtomicNegCount[a] = new int[3];
		}
	}

	IsOrdered = new bool[net->Card[TR]];
	memset(IsOrdered, 0, net->Card[TR] * sizeof(bool));
	TOrder = new arrayindex_t[net->Card[TR]];
	arrayindex_t *NotYetOrdered = new arrayindex_t[net->Card[TR]];
	arrayindex_t currActivated = 0;
	arrayindex_t UnorderedCount = 0;
	arrayindex_t currNonActivated = initial->CardEnabled;
	for (arrayindex_t t = 0; t < net->Card[TR]; t++)
	{
		if (initial->Enabled[t])
		{
			TOrder[currActivated] = t;
			IsOrdered[t] = true;
			currActivated++;
			for (arrayindex_t p = 0; p < net->CardArcs[TR][POST][t]; p++)
			{
				Computed[net->Arc[TR][POST][t][p]] = true;
			}
		}
		else
		{
			bool allPre = true;
			for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][t]; p++)
			{
				if (net->Mult[TR][PRE][t][p] > net->Initial[net->Arc[TR][PRE][t][p]] && !Computed[net->Arc[TR][PRE][t][p]])
				{
					allPre = false;
				}
			}
			if (allPre)
			{
				TOrder[currNonActivated] = t;
				IsOrdered[t] = true;
				currNonActivated++;
				for (arrayindex_t p = 0; p < net->CardArcs[TR][POST][t]; p++)
				{
					Computed[net->Arc[TR][POST][t][p]] = true;
				}
			}
			else
			{
				NotYetOrdered[UnorderedCount] = t;
				UnorderedCount++;
			}
		}
	}

	bool changed = true;
	arrayindex_t start = 0;
	while (changed)
	{
		changed = false;
		for (arrayindex_t i = start; i < UnorderedCount; i++)
		{
			arrayindex_t t = NotYetOrdered[i];
			bool allPre = true;
			for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][t]; p++)
			{
				if (net->Mult[TR][PRE][t][p] > net->Initial[net->Arc[TR][PRE][t][p]] && !Computed[net->Arc[TR][PRE][t][p]])
				{
					allPre = false;
				}
			}
			if (allPre)
			{
				changed = true;
				TOrder[currNonActivated] = t;
				IsOrdered[t] = true;
				currNonActivated++;
				for (arrayindex_t p = 0; p < net->CardArcs[TR][POST][t]; p++)
				{
					Computed[net->Arc[TR][POST][t][p]] = true;
				}
				if (i != start)
				{
					arrayindex_t temp = NotYetOrdered[i];
					NotYetOrdered[i] = NotYetOrdered[start];
					NotYetOrdered[start] = temp;
					i--;
				}
				start++;
			}
		}
	}

	TransitionCount = currNonActivated;
	delete[] NotYetOrdered;
	delete initial;
}

Reachability2Minisat::~Reachability2Minisat()
{
	for (arrayindex_t t = 0; t < net->Card[TR]; t++)
		delete[] AtomicChange[t];
	for (arrayindex_t p = 0; p < net->Card[PL]; p++)
	{
		if (!verboseVariables)
		{
			delete[] PlaceVarCount[p];
			delete[] PlaceNegCount[p];
		}
		delete[] PlaceValues[p];
	}
	for (arrayindex_t c = 0; c < AtomicCount; c++)
	{
		if (!verboseVariables)
		{
			delete[] AtomicVarCount[c];
			delete[] AtomicNegCount[c];
		}
		delete[] AtomicMinValue[c];
	}
	delete[] AtomicChange;
	delete[] PlaceAllowedMax;
	delete[] AtomicAllowedMax;
	delete[] Computed;
	delete[] PlaceVarCount;
	delete[] PlaceNegCount;
	delete[] PlaceValues;
	delete[] AtomicVarCount;
	delete[] AtomicNegCount;
	delete[] AtomicMinValue;
	delete[] Atomic;
	delete[] TOrder;
	delete[] IsOrdered;
}

/// Helper functions to calculate addresses for minisat

int Reachability2Minisat::PlaceAddress(arrayindex_t p, arrayindex_t i, arrayindex_t step, bool isNext)
{
	int address = 0;
	if (verboseVariables)
	{
		// first #PathLength variables are reserved for 'fire' or 'copy' variables
		address = PathLength * TransitionCount - 1;
		// then we need to 'step over' all addresses from steps before
		for (arrayindex_t s = 0; s < step; s++)
		{
			for (arrayindex_t p_o = 0; p_o < net->Card[PL]; p_o++)
			{
				address += PlaceVarCount[p_o][s];
			}
			for (arrayindex_t c = 0; c < AtomicCount; c++)
			{
				address += AtomicVarCount[c][s];
			}
		}
	}
	else
	{
		address = OverallVarCount - 1 + (isNext ? LastStepVariableCount : 0);
	}
	// then we need to 'step over' all addresses for places before p in step
	for (arrayindex_t p_o = 0; p_o < p; p_o++)
	{
		address += PlaceVarCount[p_o][step];
	}
	// after that, we are in the address domain for p in step, so we only need to add i
	// in this case i represents an amount of marks and we skip 0, since there are
	// always more or equal to zero marks on a place
	// this could then also be though of indexing starting at 1 instead of 0
	return address + i;
}

int Reachability2Minisat::AtomicAddress(arrayindex_t c, arrayindex_t w, arrayindex_t step, bool isNext)
{
	int address = 0;
	if (verboseVariables)
	{
		// first #PathLength variables are reserved for 'fire' or 'copy' variables
		address = PathLength * TransitionCount - 1;
		// then we need to 'step over' all addresses from steps before
		for (arrayindex_t s = 0; s < step; s++)
		{
			for (arrayindex_t p_o = 0; p_o < net->Card[PL]; p_o++)
			{
				address += PlaceVarCount[p_o][s];
			}
			for (arrayindex_t c = 0; c < AtomicCount; c++)
			{
				address += AtomicVarCount[c][s];
			}
		}
	}
	else
	{
		address = OverallVarCount - 1 + (isNext ? LastStepVariableCount : 0);
	}
	// then we need to 'step over' all addresses for places in step
	for (arrayindex_t p_o = 0; p_o < net->Card[PL]; p_o++)
	{
		address += PlaceVarCount[p_o][step];
	}
	// then we need to 'step over' all addresses for predicates before c
	for (arrayindex_t c_o = 0; c_o < c; c_o++)
	{
		address += AtomicVarCount[c_o][step];
	}
	// after that, we are in the address domain for c in step, so we only need to add w
	// for consistency (and me forgetting that atomics can have negative sums in the beginning)
	// we also assume indexing starting with 1 instead of 0
	return address + w;
}

// CREATE INITIAL FORMULA
// ====================
// = AND(i=0..Card[PL]) AND(j..m^0(p^i)+1) (not) p^i,j AND(c=0..AtomicCount) AND(i=0..c->sum+1) (not) w^i

void Reachability2Minisat::create_initial()
{
	LastStepVariableCount = 0;
	for (arrayindex_t p = 0; p < net->Card[PL]; p++)
	{
		// having the maximum step count + 1 allows us to address array entries for an additional step
		// this is not needed in practice, since that last step never happens, but we can use these
		// assignments to describe the final marking
		// if we don't want verbose variables, we only need to save the PlaceVarCount
		// at three steps:
		//		-current step
		//		-next step
		//		-step after the first round, for calculating minRounds
		if (verboseVariables)
		{
			PlaceVarCount[p] = new arrayindex_t[TransitionCount * PathLength + 1];
			PlaceNegCount[p] = new arrayindex_t[TransitionCount * PathLength + 1];
		}
		// PlaceVarCount[p] = new arrayindex_t[verboseVariables ? net->Card[TR] * PathLength + 1 : 3];
		// PlaceMinValue[p] = new arrayindex_t[2];
		// memset(PlaceVarCount[p], 0, sizeof(arrayindex_t) * (verboseVariables ? net->Card[TR] * PathLength + 1 : 3));
		// memset for PlaceMinValue prob also not needed, see AtomicMinValue

		// PlaceVarCount[p][0] = net->Initial[p] + 1;
		// // PlaceMinValue[p][1] = net->Initial[p];
		// // first we push the first m^0(p) variables as is
		// for (arrayindex_t i = 1; i < PlaceVarCount[p][0]; i++)
		// {
		// 	S->newVar();
		// 	S->addClause(mkLit(VAR_PLACE(p, i, 0, CURR)));
		// }
		// // then an additional one negated to encode the value fully
		// S->newVar();
		// S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][0], 0, CURR)));

		PlaceVarCount[p][0] = 2;
		PlaceNegCount[p][0] = -1 * net->Initial[p] + 1;
		S->newVar();
		S->newVar();
		S->addClause(mkLit(VAR_PLACE(p, 1, 0, CURR)));
		S->addClause(~mkLit(VAR_PLACE(p, 2, 0, CURR)));

		LastStepVariableCount += PlaceVarCount[p][0];
	}
	// NetState *initial = NetState::createNetStateFromInitial(net);
	for (arrayindex_t a = 0; a < AtomicCount; a++)
	{
		// having the maximum step count + 1 allows us to address array entries for an additional step
		// this is not needed in practice, since that last step never happens, but we can use these
		// assignments to describe the final marking
		// similiar to places, if we don't want verbose variables, we only need to save for 2 steps
		// one step less than places, since atomics are always <=, therefore don't need the max
		// value for minRounds
		if (verboseVariables)
		{
			AtomicVarCount[a] = new arrayindex_t[TransitionCount * PathLength + 1];
			AtomicNegCount[a] = new int[TransitionCount * PathLength + 1];
		}
		// AtomicVarCount[c] = new arrayindex_t[verboseVariables ? net->Card[TR] * PathLength + 1 : 3];
		// AtomicNegCount[c] = new arrayindex_t[verboseVariables ? net->Card[TR] * PathLength + 1 : 3];
		// AtomicMinValue[c] = new int[2];
		// memset(AtomicVarCount[c], 0, sizeof(arrayindex_t) * (verboseVariables ? net->Card[TR] * PathLength + 1 : 3));
		// memset(AtomicNegCount[c], 0, sizeof(arrayindex_t) * (verboseVariables ? net->Card[TR] * PathLength + 1 : 3));
		// memset(AtomicMinValue[c], 0, sizeof(arrayindex_t) * 2); both entries should be overwritten when working,
		// so maybe no need to memset
		// Atomic[c]->evaluate(*initial);
		// special case if the sum is 0, than we need 2 variables, one for >= 0 and one for >= 1

		// AtomicVarCount[a][0] = Atomic[a]->sum == 0 ? 2 : abs(Atomic[a]->sum) + 1;
		// // AtomicMinValue[c][1] = Atomic[c]->sum;
		// // if the formal sum is non positive, we need to say the amount of variables which represent non positive values
		// // this would again be abs(sum), but just accessing the VarCount array and subtracting 1 could be faster than abs
		// // also when negative: only the first variable is not negated, while when positive: only the last is negated
		// if (Atomic[a]->sum <= 0)
		// {
		// 	// if the sum is 0, then we have variables for 0 and 1, there one non positive
		// 	// otherwise we have values vom sum...0, which are all non positive
		// 	AtomicNegCount[a][0] = Atomic[a]->sum == 0 ? 1 : AtomicVarCount[a][0];
		// 	S->newVar();
		// 	S->addClause(mkLit(VAR_CONS(a, 1, 0, CURR)));
		// 	for (arrayindex_t w = 2; w <= AtomicVarCount[a][0]; w++)
		// 	{
		// 		S->newVar();
		// 		S->addClause(~mkLit(VAR_CONS(a, w, 0, CURR)));
		// 	}
		// }
		// else
		// {
		// 	AtomicNegCount[a][0] = 0;
		// 	for (arrayindex_t w = 1; w < AtomicVarCount[a][0]; w++)
		// 	{
		// 		S->newVar();
		// 		S->addClause(mkLit(VAR_CONS(a, w, 0, CURR)));
		// 	}
		// 	S->newVar();
		// 	S->addClause(~mkLit(VAR_CONS(a, AtomicVarCount[a][0], 0, CURR)));
		// }

		AtomicVarCount[a][0] = 2;
		AtomicNegCount[a][0] = -1 * Atomic[a]->sum + 1;
		S->newVar();
		S->newVar();
		S->addClause(mkLit(VAR_CONS(a, 1, 0, CURR)));
		S->addClause(~mkLit(VAR_CONS(a, 2, 0, CURR)));

		LastStepVariableCount += AtomicVarCount[a][0];
	}
	//delete initial;
}

int Reachability2Minisat::GetAtomicMinRounds(arrayindex_t c)
{
	arrayindex_t round = 0;
	int minChange = Atomic[c]->sum - AtomicMinValue[c][0];
	if (minChange <= 0)
	{
		// if minChange is 0, we will never satisfy this Atomic, should handle extra
		// minChange should never be smaller than 0, but we can handle that, since
		// it's basically an exception aswell
		return -2;
	}
	return (Atomic[c]->sum - Atomic[c]->threshold) / minChange + 1;
	// while (++round)
	// {
	// 	// check if the sum^init - round * minChange <= treshold
	// 	// that would mean that after #round rounds the minimum value is smaller than the treshold,
	// 	// therefore we wouldn't add the above contradiction
	// 	if (int(Atomic[c]->sum - round * minChange) <= Atomic[c]->threshold)
	// 		break;
	// }
	// return round;
}

int Reachability2Minisat::GetPlaceMinRounds(arrayindex_t p, mult_t m, bool needsGreater)
{
	// in this we determine the minimum rounds needed so that the tokencount on p is greater or lesser than m
	// with needsGreater deciding if greater or lesser
	arrayindex_t round = 0;
	if (needsGreater)
	{
		// this means that the max needs to be greater than some mult, to remove the contradiction that a
		// transition is never fireable
		// for that we calculate the difference in max between initial and the state after the first round
		// and check if p^init + round * maxChange >= m
		// to find out the max, we need to differentiate verbose variables
		// if we have verbose variables, then its after every transition done once -> first round
		// if we don't have them, then its saved in the extra entry of the array and set after
		// the first round
		int maxChange = ceil(VariableBoundPercentage * (PlaceValues[p][2] - net->Initial[p]));
		if (maxChange <= 0)
		{
			// if the max never rises then we can't solve the contradiction
			return -2;
		}
		round = int(m - net->Initial[p]) / maxChange + 1;
		// while (++round)
		// {
		// 	// 'max after first round' + k*maxChange means that after k+1 round (+1 since the first summand includes
		// 	// the first round) the max is greater than m
		// 	if (int(PlaceValues[p][2] + (round - 1) * maxChange) >= m)
		// 		break;
		// }
	}
	else
	{
		// check if the sum^init - round * minChange <= treshold
		// that would mean that after #round rounds the minimum value is smaller than the treshold,
		// therefore we wouldn't add the above contradiction
		int minChange = net->Initial[p] - PlaceValues[p][0];
		if (minChange <= 0)
		{
			// if minChange is 0, we will never satisfy this Atomic, should handle extra
			// minChange should never be smaller than 0, but we can handle that, since
			// it's basically an exception aswell
			return -2;
		}
		round = int(net->Initial[p] - m) / minChange + 1;
		// while (++round)
		// {
		// 	if (int(net->Initial[p] - round * minChange) <= m)
		// 		break;
		// }
	}

	return round;
}

int Reachability2Minisat::create_atomic(StatePredicate *f, vec<Lit> &lits)
{
	// since we're adding the atomic to a clause, we can find that the clause is satisfied if we
	// find the atomic to be satisfied, for that we use an int instead of bool, because
	// we can then use the same int for minRounds incase this atomic is unsatisfiable
	// for that we still define false=0, but true=-1, while some value > 0 would be minRounds
	int isSat = 0;
	arrayindex_t c = -1;
	// need to find the atomic that is the current child so we can use its variable
	while (f != Atomic[++c])
		;
	// the variable for the atomic is negated value for threshhold + 1 at the last step
	// since this is done after generating all phi_t, we know that step is the last step
	// we need to find out if the threshold is even in the possible values
	// if the threshold is smaller than the minimum possible value, than the atomic is unsatisfied, so no need to do anything
	// since we would only add a false to a clause, we then only need to add to the clause if
	// the threshold is also smaller than the maximum value,
	// if the the threshold is even greater than that, then the atomic is satisfied, so
	// we can add a true to the clause, which is aquivilant to just not adding the clause
	if (Atomic[c]->threshold + AtomicNegCount[c][step] > 0)
	{
		if (Atomic[c]->threshold < AtomicVarCount[c][step] - AtomicNegCount[c][step])
		{
			lits.push(~mkLit(VAR_CONS(c, Atomic[c]->threshold + AtomicNegCount[c][step] + 1, step, CURR)));
		}
		else
		{
			isSat = -1;
		}
	}
	else
	{
		isSat = GetAtomicMinRounds(c);
	}
	return isSat;
}

int Reachability2Minisat::create_fireable(FireablePredicate *f, vec<Lit> &lits, arrayindex_t extra)
{
	int isSat = -1;
	bool always = true;
	bool already = false;
	int rounds = 0;
	for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][f->t]; p++)
	{
		mult_t mult = net->Mult[TR][PRE][f->t][p];
		arrayindex_t place = net->Arc[TR][PRE][f->t][p];
		if (mult + PlaceNegCount[place][step] > 0)
		{
			always = false;
			if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
			{
				if (f->sign)
				{
					if (net->CardArcs[TR][PRE][f->t] == 1)
					{
						lits.push(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
					}
					else
					{
						if (extra)
						{
							S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)), ~mkLit(VAR_EXTRA(extra)));
						}
						else
						{
							S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
						}
					}
				}
				else
				{
					lits.push(~mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
				}
			}
			else
			{
				already = true;
				if (f->sign)
				{
					isSat = GetPlaceMinRounds(place, mult, true);
				}
				break;
			}
		}
		else
		{
			if (!(f->sign))
			{
				int round = GetPlaceMinRounds(place, mult, false);
				if (round != -2)
				{
					if (rounds == 0)
					{
						rounds = round;
					}
					else if (rounds > 0 && round < rounds)
					{
						rounds = round;
					}
				}
			}
		}
	}
	if (always)
	{
		if (f->sign)
		{
			isSat = -1;
		}
		else
		{
			isSat = rounds == 0 ? -2 : rounds;
		}
	}
	else if (!already)
	{
		isSat = 0;
	}
	return isSat;
}

int Reachability2Minisat::create_fireable_and(FireablePredicate *f, arrayindex_t extra)
{
	int isSat = -1;
	// fireable:
	//	AND(p^i in pre(t)) p^i,W(p,t),step
	//	since we're at and level, can simply add clauses
	// not-fireable:
	// 	OR(p^i in pre(t)) not p^i,W(p,t),step
	//	atleast one place needs not enough tokens
	//	since we're at and level, can simply add one clause
	if (f->sign)
	{
		for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][f->t]; p++)
		{

			mult_t mult = net->Mult[TR][PRE][f->t][p];
			arrayindex_t place = net->Arc[TR][PRE][f->t][p];
			// first, we can use our estimate for PlaceMinValue to determine if
			// the clause for this place is already satisfied
			// if even our estimate is larger than mult, than we safely know
			// that there are enough tokens, no need to do anything
			if (mult + PlaceNegCount[place][step] > 0)
			{
				// if thats not the case, we need to check if the clause is unsatisfiable
				// meaning that there can't be enough tokens
				if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
				{
					isSat = 0;
					// because only if mult is in the possible values we need to say which value is needed
					if (extra)
					{
						S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)), ~mkLit(VAR_EXTRA(extra)));
					}
					else
					{
						S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
					}
				}
				else
				{
					// if mult is even greater than the max, then the fireable becomes unsatisfiable
					// or in case of renaming we know that the parent and is false, since this sub is false
					if (!extra)
					{
						S->addEmptyClause();
					}
					else
					{
						S->addClause(~mkLit(VAR_EXTRA(extra)));
					}
					isSat = GetPlaceMinRounds(place, mult, true);
					break;
				}
			}
			// else
			// {
			// 	isSat = -1;
			// }
		}
	}
	else
	{
		vec<Lit> lits;
		bool always = true;
		bool foundSat = false;
		int rounds = 0;
		for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][f->t]; p++)
		{
			// here, we know that, when the min value is greater than mult,
			// that t is not not fireable because of this place
			// which simply means not adding a literal
			// but if that happens for every place, then we have a contradiction
			// and want to raise the minRounds value by however much, such that
			// atleast one could have 'not enough tokens'
			mult_t mult = net->Mult[TR][PRE][f->t][p];
			arrayindex_t place = net->Arc[TR][PRE][f->t][p];
			if (mult + PlaceNegCount[place][step] > 0)
			{
				always = false;
				if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
				{
					lits.push(~mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
				}
				else
				{
					foundSat = true;
					break;
				}
			}
			else
			{
				// since we know that the min is always greater, we can calculate the amount of
				// rounds until it's no longer greater in case of contradiction
				int round = GetPlaceMinRounds(place, mult, false);
				if (round != -2)
				{
					if (rounds == 0)
					{
						rounds = round;
					}
					else if (rounds > 0 && round < rounds)
					{
						rounds = round;
					}
				}
			}
		}
		if (always)
		{
			// here we know that there are enough tokens on every predecessor, meaning t can fire
			isSat = rounds == 0 ? -2 : rounds;
			if (!extra)
			{
				S->addEmptyClause();
			}
			else
			{
				S->addClause(~mkLit(VAR_EXTRA(extra)));
			}
		}
		// here, extra is either the variable for the parent and or, if the deadlock is part of an or,
		// the variable for the deadlock itself, since we need to rename then
		// finally, if we found out we're neither unsat nor already sat, we want to add the clause
		else if (!foundSat)
		{
			if (extra)
			{
				lits.push(~mkLit(VAR_EXTRA(extra)));
			}
			S->addClause(lits);
			isSat = 0;
		}
	}
	return isSat;
}

int Reachability2Minisat::create_fireable_or(FireablePredicate *f, vec<Lit> &lits)
{
	// similiar to create_atomic_or, we're adding literals to a parent or
	// we also have to differentiate between already satisfied, needs work and unsatisfiable
	// with the letter meaning that we add no literals, but since the parent could then be empty,
	// we want to manage how many rounds are needed until we would add literals here
	// therefore we return int, with isSat = -1 meaning 'already satisfied', isSat = 0 being 'more work'
	// and isSat > 0 meaning 'not adding things, but if leading to empty clause how many rounds'
	int isSat = -1;
	// here we are at or level and want to add a clause
	// for fireable we would usually add a set of clauses -> rename now
	// for not fireable we would usually add a clause -> add literals to parent clause
	if (f->sign)
	{
		// need to rename, first introduce a new variable and add to parent clause
		// but only need to rename if t has more than one predecessor
		if (net->CardArcs[TR][PRE][f->t] > 1)
		{
			S->newVar();
			arrayindex_t subextra = ++Extras;
			if (verboseVariables)
				RenamedSubs.push_back(f->copy(NULL));
			// compared to parent and, if we find that the fireable is always satisfied,
			// then we can make the parent clause always satisfied
			bool always = true;
			bool couldContradict = false;
			// then add the clauses for extra -> fireable
			for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][f->t]; p++)
			{
				// similiar to phi^t, we only need to add the literal if
				// there could have been enough tokens
				// if there possibly aren't enough, we need to add the unit clause not extra
				// and can break
				mult_t mult = net->Mult[TR][PRE][f->t][p];
				arrayindex_t place = net->Arc[TR][PRE][f->t][p];
				if (mult + PlaceNegCount[place][step] > 0)
				{
					// if for any place our estimate min value is smaller than the mult,
					// then we need to add some literal
					always = false;
					if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
					{
						S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)), ~mkLit(VAR_EXTRA(subextra)));
					}
					else
					{
						couldContradict = true;
						// if we're here, we want to calculate minRounds
						// because, if subextra is the only literal of the parent or, then by finding out
						// here that that is false, we would have an empty clause
						S->addClause(~mkLit(VAR_EXTRA(subextra)));
						isSat = GetPlaceMinRounds(place, mult, true);
						break;
					}
				}
			}
			// if after handling the loop we find that this is always not fireable (=couldContract = true),
			// we don't need to add the variable to the parent clause
			if (!couldContradict)
			{
				lits.push(mkLit(VAR_EXTRA(subextra)));
				isSat = 0;
			}
			// if after handling the for loop 'always' is still true, then we know that for every predecessor,
			// our estimate for min val is greater than mult, therefore we know that this is satisfied
			if (always)
				isSat = -1;
		}
		else if (net->CardArcs[TR][PRE][f->t] == 1)
		{
			// since now fireable can be represented with one literal, we can add that instead
			mult_t mult = net->Mult[TR][PRE][f->t][0];
			arrayindex_t place = net->Arc[TR][PRE][f->t][0];
			if (mult + PlaceNegCount[place][step] > 0)
			{
				if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
				{
					lits.push(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
					isSat = 0;
				}
				else
				{
					// similiar else case as above, in case of empty parent or we can calculate
					// the rounds needed to avoid this contradiction
					isSat = GetPlaceMinRounds(place, mult, true);
				}
			}
			else
			{
				// again, if we find this to already be satisfied, we can tell that to the parent or
				isSat = -1;
			}
		}
	}
	else
	{
		// this is not fireable, if we don't add any literal, then we know that this is fireable
		// therefore we want to do contradiction handling
		int rounds = 0;
		bool always = true;
		bool already = false;
		for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][f->t]; p++)
		{
			// only need to add literal if there are enough variables
			// if there is one where there are not enough variables
			// we know that t can't fire, so don't need to add the clause, but add
			// contradiction handling
			mult_t mult = net->Mult[TR][PRE][f->t][p];
			arrayindex_t place = net->Arc[TR][PRE][f->t][p];
			if (mult + PlaceNegCount[place][step] > 0)
			{
				always = false;
				if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
				{
					lits.push(~mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)));
				}
				else
				{
					already = true;
					break;
				}
			}
			else
			{
				// since we know that the min is always greater, we can calculate the amount of
				// rounds until it's no longer greater in case of contradiction
				int round = GetPlaceMinRounds(place, mult, false);
				if (round != -2)
				{
					if (rounds == 0)
					{
						rounds = round;
					}
					else if (rounds > 0 && round < rounds)
					{
						rounds = round;
					}
				}
			}
		}
		// now need to set isSat, if we found it to be true, keep that, otherwise we can equal to rounds
		// which is equal to 0 (false) in case of no contradictions and minRounds otherwise
		if (always)
		{
			isSat = rounds == 0 ? -2 : rounds;
		}
		else
		{
			isSat = 0;
		}
	}
	return isSat;
}

int Reachability2Minisat::create_deadlock(arrayindex_t extra)
{
	// we can initialize with -1, since not changing the value (meaning no transitions)
	// would mean that we have a deadlock
	int isSat = -1;
	// AND (t=0..Card[TR]) OR (p^i from pre(t)) not p^i,W(p^i,t),step
	// a deadlock means that every transition is not fireable
	// so we add the clause for not fireable for every transition
	for (arrayindex_t i = 0; i < TransitionCount; i++)
	{
		arrayindex_t t = TOrder[i];
		FireablePredicate *f = new FireablePredicate(net, t, false);
		vec<Lit> lits;
		int result = create_fireable(f, lits, extra);
		if (result == -2)
		{
			isSat = -2;
			delete f;
			break;
		}
		else if (result == 0)
		{
			if (extra)
			{
				lits.push(~mkLit(VAR_EXTRA(extra)));
			}
			S->addClause(lits);
		}
		else if (result > 0)
		{
			isSat = result > isSat ? result : isSat;
		}
		delete f;
	}
	return isSat;
}

int Reachability2Minisat::create_nodeadlock(vec<Lit> &lits)
{
	// OR (t=0..Card[TR]) AND (p^i from pre(t)) p^i,W(p^i,t),step
	// no deadlock means that atleast one transition can fire
	// that means introducing new variables for every t and adding them to a clause
	// then add clauses fire^t -> fireable(t)
	// we initialize isSat with -2, because not having any transition would leave us with an unsovable deadlock,
	// meaning isSat = -2
	int isSat = -2;
	// additionally, we introduce a bool to determine if there is a deadlock because all transitions
	// are always not fireable
	bool isDeadlock = true;
	for (arrayindex_t i = 0; i < TransitionCount; i++)
	{
		arrayindex_t t = TOrder[i];
		FireablePredicate *f = new FireablePredicate(net, t, true);
		arrayindex_t sub = 0;
		if (net->CardArcs[TR][PRE][f->t] > 1)
		{
			S->newVar();
			sub = ++Extras;
			if (verboseVariables)
				RenamedSubs.push_back(f->copy(NULL));
		}
		int result = create_fireable(f, lits, sub);
		if (result == -1)
		{
			isSat = -1;
			delete f;
			break;
		}
		else if (result == 0)
		{
			isDeadlock = false;
		}
		else if (result > 0)
		{
			if (isSat <= 0)
			{
				isSat = result;
			}
			else
			{
				isSat = result > isSat ? isSat : result;
			}
		}
		delete f;
	}
	// finally, if we find that there is atleast one transition which is neither always fireable
	// nor contradictory, we know that this nodeadlock is (as a whole) neither contradictory nor
	// satisfied, therefore isSat = 0
	if (!isDeadlock)
		isSat = 0;
	return isSat;
}

// CREATE CONSTRAINT FORMULA
// ====================
// = StatePredicate in CNF with every atomic replaced by 'not w^k+1' with k being the threshhold of the atomic

int Reachability2Minisat::create_and(AtomicBooleanPredicate *formula, arrayindex_t extra)
{
	// need to create structure-preserving translation of formula, implementing according to theory
	// we have the advantage, that the formula is 'cleaned up' by removing arrows/double arrows and
	// moving negations to the Atomics and applying them beforehand
	// therefore we only need to differentiate between conjunctions and disjunctions
	// first create new variable for the subformula 'formula'

	// since we're in the renaming context, the parent of 'formula' is an or
	// meaning, if we find 'formula' to be unsat, because of a child,
	// we wouldn't add anything to the parent or, but we use the minRounds to make the
	// child no longer unsat to remove the unsat parent or, in case its clause is empty
	// because of 'formula' being unsat
	// similiar to create_atomic_or, this is -1 incase of sat, 0 incase of unsure, > 0 in case of unsat, with
	// the exact number representing the rounds to undo unsat
	int isSat = 0;
	// we can also track sat of the whole and, which would make the parent or sat
	// this will be set to false as soon as one child isn't safely sat
	bool always = true;
	for (arrayindex_t child = 0; child < formula->cardSub; child++)
	{
		int result = -3;
		bool needOrAdd = false;
		vec<Lit> lits;
		// if the subformula is an atomic, we can use the variable representing the atomic
		if (formula->sub[child]->isatomic)
		{
			// add atomic formula, if by doing that we find out that the parent and is unsatisfied, we don't need to handle the remaining subs
			needOrAdd = true;
			result = create_atomic(formula->sub[child], lits);
		}
		else if (formula->sub[child]->isfireable)
		{
			FireablePredicate *f = (FireablePredicate *)(formula->sub[child]);
			if (f->sign && !IsOrdered[f->t])
			{
				if (extra)
				{
					S->addClause(~mkLit(VAR_EXTRA(extra)));
				}
				else
				{
					S->addEmptyClause();
				}
				isSat = -2;
				always = false;
				break;
			}
			else if (IsOrdered[f->t])
			{
				if (!(f->sign) || net->CardArcs[TR][PRE][f->t] == 1)
					needOrAdd = true;
				result = create_fireable(f, lits, extra);
			}
		}
		else if (formula->sub[child]->magicnumber == MAGIC_NUMBER_DEADLOCK)
		{
			result = create_deadlock(extra);
		}
		else if (formula->sub[child]->magicnumber == MAGIC_NUMBER_NODEADLOCK)
		{
			needOrAdd = true;
			result = create_nodeadlock(lits);
		}
		// now only or is left, so add clause
		else
		{
			AtomicBooleanPredicate *b = (AtomicBooleanPredicate *)formula->sub[child];
			if (b->isAnd)
			{
				result = create_and(b, extra);
			}
			else
			{
				needOrAdd = true;
				result = create_or(b, lits);
			}
		}
		if (result != -3 && result != -1)
		{
			always = false;
			if (result == -2)
			{
				isSat = -2;
				break;
			}
			else if (result == 0)
			{
				if (needOrAdd)
				{
					if (extra)
					{
						lits.push(~mkLit(VAR_EXTRA(extra)));
					}
					S->addClause(lits);
				}
			}
			else if (result > 0)
			{
				isSat = result > isSat ? result : isSat;
			}
		}
	}
	if (always)
	{
		isSat = -1;
	}
	return isSat;
}

int Reachability2Minisat::create_or(AtomicBooleanPredicate *b, vec<Lit> &lits)
{
	arrayindex_t rounds = 0;
	bool foundSat = false;
	bool addedsomething = false;
	int isSat = -1;
	// add literal for every subsub, which are either atmoic or and predicate (or fireable/deadlock)
	for (arrayindex_t subChild = 0; subChild < b->cardSub; subChild++)
	{
		int result = -3;
		bool needAndAdd = false;
		arrayindex_t subsub = 0;
		// if the subformula is an atomic, we can use the variable representing the atomic
		if (b->sub[subChild]->isatomic)
		{
			result = create_atomic(b->sub[subChild], lits);
		}
		else if (b->sub[subChild]->isfireable)
		{
			FireablePredicate *f = (FireablePredicate *)(b->sub[subChild]);
			if (!(f->sign) && !IsOrdered[f->t])
			{
				foundSat = true;
				break;
			}
			else if (IsOrdered[f->t])
			{
				if (net->CardArcs[TR][PRE][f->t] > 1)
				{
					S->newVar();
					subsub = ++Extras;
					needAndAdd = true;
					if (verboseVariables)
						RenamedSubs.push_back(f->copy(NULL));
				}
				result = create_fireable(f, lits, subsub);
			}
		}
		else if (b->sub[subChild]->magicnumber == MAGIC_NUMBER_DEADLOCK)
		{
			if (net->Card[TR] == 1)
			{
				FireablePredicate f(net, 0, false);
				if (!(f.sign) && !IsOrdered[f.t])
				{
					foundSat = true;
					break;
				}
				else if (IsOrdered[f.t])
				{
					result = create_fireable(&f, lits, 0);
				}
			}
			else
			{
				// since we're at or level and want to add a set of clauses, we need to rename
				S->newVar();
				subsub = ++Extras;
				needAndAdd = true;
				if (verboseVariables)
					RenamedSubs.push_back(b->sub[subChild]->copy(NULL));
				result = create_deadlock(subsub);
			}
		}
		else if (b->sub[subChild]->magicnumber == MAGIC_NUMBER_NODEADLOCK)
		{
			result = create_nodeadlock(lits);
		}
		// now only and is left, need to rename
		else
		{
			AtomicBooleanPredicate *bb = (AtomicBooleanPredicate *)b->sub[subChild];
			if (bb->isAnd)
			{
				S->newVar();
				subsub = ++Extras;
				needAndAdd = true;
				if (verboseVariables)
					RenamedSubs.push_back(bb->copy(NULL));
				result = create_and(bb, subsub);
			}
			else
			{
				result = create_or(bb, lits);
			}
		}
		if (result != -3)
		{
			if (result == -1)
			{
				foundSat = true;
				break;
			}
			else if (result == 0)
			{
				if (needAndAdd)
				{
					lits.push(mkLit(VAR_EXTRA(subsub)));
				}
				addedsomething = true;
			}
			else if (result > 0)
			{
				// if this literal is no longer contradictory with the least rounds so far, we
				// raise only that amount
				if (rounds == 0)
				{
					rounds = result;
				}
				else if (rounds > 0 && result < rounds)
				{
					rounds = result;
				}
			}
		}
	}
	if (!foundSat)
	{
		if (addedsomething)
		{
			isSat = 0;
		}
		else if (rounds == 0)
		{
			isSat = -2;
		}
		else
		{
			isSat = rounds;
		}
	}
	return isSat;
}

// CREATE PHI^t FORMULA
// ====================
// = (copy or fire) and (copy non relevant)

void Reachability2Minisat::create_phi(arrayindex_t t, bool isEndofFirstRound)
{
	// if we don't want verbose variables, next step is the other
	// index in PlaceVarCount etc., therefore we can calculate via mod 2,
	// which can simply be done with bitwise AND 1
	arrayindex_t nextstep = step + 1;
	if (!verboseVariables)
		nextstep &= 1;
	arrayindex_t stepvariables = 0;
	bool willCopyAnyway = false;
	// here we need to describe the change of every place by either firing t or copying
	// first we need to add the condition that t can even fire
	// for that we check the ArcMults for every predecessor of t, there 2 possibilities arise
	// 1) ArcMult < PlaceVarCount[p][step]
	//		-> We could have enough variables, so we need to add the condition that there are enough variables
	// 2) ArcMult >= PlaceVarCount[p][step]
	//		-> There is no way that there are enough tokens to allow t to fire, so we add the condition that we HAVE TO copy here
	for (arrayindex_t p = 0; p < net->CardArcs[TR][PRE][t]; p++)
	{
		mult_t mult = net->Mult[TR][PRE][t][p];
		arrayindex_t place = net->Arc[TR][PRE][t][p];
		if (mult + PlaceNegCount[place][step] > 0)
		{
			if (mult < PlaceVarCount[place][step] - PlaceNegCount[place][step])
			{
				S->addClause(mkLit(VAR_PLACE(place, mult + PlaceNegCount[place][step], step, CURR)), ~mkLit(VAR_FIRE(currentfire)));
			}
			else
			{
				S->addClause(~mkLit(VAR_FIRE(currentfire)));
				willCopyAnyway = true;
			}
		}
	}
	if (!willCopyAnyway)
	{
		// after that we update the variable count for the next step, so addressing for minisat stays consistent
		// for that we iterate through all places with positive delta and change them
		// then we iterate through the rest; differentiating between negative and no delta
		// is not needed, since for both the variable count stays the same
		for (arrayindex_t i = 0; i < net->CardDeltaT[POST][t]; i++)
		{
			const arrayindex_t p = net->DeltaT[POST][t][i];
			// const mult_t delta = net->MultDeltaT[POST][t][i];
			arrayindex_t nextmax = PlaceVarCount[p][step] + net->MultDeltaT[POST][t][i];
			Computed[p] = true;
			// the positive delta causes a higher possible maximum marking, meaning we may need new variables
			PlaceVarCount[p][nextstep] = (PlaceAllowedMax[p] > 0 && nextmax > PlaceAllowedMax[p]) ? PlaceAllowedMax[p] : nextmax;
			PlaceNegCount[p][nextstep] = PlaceNegCount[p][step];
			stepvariables += PlaceVarCount[p][nextstep];
		}
		for (arrayindex_t i = 0; i < net->CardDeltaT[PRE][t]; i++)
		{
			const arrayindex_t p = net->DeltaT[PRE][t][i];
			arrayindex_t nextmax;
			if (1 - PlaceNegCount[p][step] - net->MultDeltaT[PRE][t][i] >= 0)
			{
				nextmax = PlaceVarCount[p][step] + net->MultDeltaT[PRE][t][i];
			}
			else
			{
				nextmax = PlaceVarCount[p][step] + 1 - PlaceNegCount[p][step];
			}
			Computed[p] = true;
			PlaceVarCount[p][nextstep] = (PlaceAllowedMax[p] > 0 && nextmax > PlaceAllowedMax[p]) ? PlaceAllowedMax[p] : nextmax;
			stepvariables += PlaceVarCount[p][nextstep];
		}
	}
	for (arrayindex_t p = 0; p < net->Card[PL]; p++)
	{
		if (willCopyAnyway || !Computed[p])
		{
			PlaceVarCount[p][nextstep] = PlaceVarCount[p][step];
			PlaceNegCount[p][nextstep] = PlaceNegCount[p][step];
			stepvariables += PlaceVarCount[p][nextstep];
		}
		else
		{
			// we need to reset the computed value for the ones done in the loop above, since
			// we still need to add the clauses, but changing the value in the first place
			// allows working like this
			Computed[p] = false;
		}
		// first we tell minisat of the new variables for the next step, we do that separately so
		// there are enough variables for the adresses needed, since we can theoretically use more
		// adresses before introducing enough variables otherwise
		for (arrayindex_t j = 0; j < PlaceVarCount[p][nextstep]; j++)
		{
			S->newVar();
		}
	}
	if (!willCopyAnyway)
	{
		// then we compute all places where transition t has a positive delta
		for (arrayindex_t i = 0; i < net->CardDeltaT[POST][t]; i++)
		{
			const arrayindex_t p = net->DeltaT[POST][t][i];
			const mult_t delta = net->MultDeltaT[POST][t][i];
			// vardelta could be different from delta if UpperPlaceBound is set
			const arrayindex_t vardelta = PlaceVarCount[p][nextstep] - PlaceVarCount[p][step];
			Computed[p] = true;
			PlaceValues[p][3] += delta;
			for (arrayindex_t j = 1; j <= delta; j++)
			{
				if (VariableBoundPercentage != 1)
				{
					S->addClause(mkLit(VAR_PLACE(p, 1, step, CURR)), ~mkLit(VAR_PLACE(p, j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_PLACE(p, 1, step, CURR)), mkLit(VAR_PLACE(p, j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					if (j <= vardelta)
					{
						S->addClause(mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), ~mkLit(VAR_PLACE(p, PlaceVarCount[p][step] + j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), mkLit(VAR_PLACE(p, PlaceVarCount[p][step] + j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					}
				}
				else
				{
					S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][step] + j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					S->addClause(mkLit(VAR_PLACE(p, j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
				}
			}
			// then every variable j in step has the same value as the variable j+delta in step+1
			for (arrayindex_t j = 1; j <= PlaceVarCount[p][step]; j++)
			{
				// when an upperbound is set, only variables with j + delta <= PlaceVarCount[p][nextstep]
				// are relevant, therefore don't need to add things for the rest
				if (VariableBoundPercentage == 1 || j + delta <= PlaceVarCount[p][nextstep])
				{
					S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j + delta, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j + delta, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
				}
				// instead of firing we could also just copy
				S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
				S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
			}
		}
		// next, we compute places where t has a negative delta
		for (arrayindex_t i = 0; i < net->CardDeltaT[PRE][t]; i++)
		{
			const arrayindex_t p = net->DeltaT[PRE][t][i];
			const mult_t delta = net->MultDeltaT[PRE][t][i];
			const arrayindex_t vardelta = PlaceVarCount[p][nextstep] - PlaceVarCount[p][step];
			PlaceNegCount[p][nextstep] = PlaceNegCount[p][step] + vardelta;
			Computed[p] = true;
			// having a negative delta also changes our estimate for the MinValue
			// since places never have a negative tokencount, we can guard that
			if (PlaceValues[p][1] > delta)
			{
				PlaceValues[p][1] -= delta;
			}
			else
			{
				PlaceValues[p][1] = 0;
			}
			if (PlaceNegCount[p][step] == 1)
			{
				// then every variable j in step has the same value as the variable j-delta in step+1
				// but this is only needed for j-delta > 0, since we don't need variables for non positive values
				for (arrayindex_t j = 1; j <= PlaceVarCount[p][step]; j++)
				{
					if (j - delta > 0)
					{
						S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j - delta, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j - delta, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					}
					// instead of firing we could also just copy
					S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
				}
				// having a negative balance k causes the last k variables to not be set by the loop before
				// compared to the loop for positive balance the last k variables need to be negated
				// and we can count beginning from the maximum
				// only problem would arise when PlaceVarCount > UpperPlaceBound, because then we don't know,
				// how many variables would need to be set
				// another way to interpret this is that with negative balance, the last k
				// variables get their values from the first k variables above the maximum, which would be 0
				// when the UpperBound is not reached and therefore the real value is smaller than UpperBound.
				// If the UpperBound was reached, the maximum can be interpreted as infinite, but we still need
				// to set the variables
				// Fortunately, we can say that we can just set these variables according to the last variable in step
				// because:
				//	1)	if the last variable was 0, we know that the first k above max are 0 aswell
				//	2)	if the last was 1, we know that the UpperBound is reached and we can never safely go below
				//		so we can set these as 1
				//	meaning we can add PlaceVarCount[p][step] <=> PlaceVarCount[p][nextstep] - j
				// this also works when no Upperbound is set, its just always the 1) case above, meaning we
				// have a few more clauses but save time differentiating when an upperbound is set
				for (arrayindex_t j = 0; j < delta; j++)
				{
					if (VariableBoundPercentage != 1)
					{
						S->addClause(mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), ~mkLit(VAR_PLACE(p, PlaceVarCount[p][step] - j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), mkLit(VAR_PLACE(p, PlaceVarCount[p][step] - j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					}
					else
					{
						S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][step] - j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					}
				}
			}
			else
			{
				int indexOffsetFire = vardelta - delta;
				for (arrayindex_t j = 1; j <= PlaceVarCount[p][step]; j++)
				{
					if (j + indexOffsetFire > 0)
					{
						S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j + indexOffsetFire, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j + indexOffsetFire, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					}
					// instead of firing we could also just copy
					S->addClause(mkLit(VAR_PLACE(p, j, step, CURR)), ~mkLit(VAR_PLACE(p, j + vardelta, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_PLACE(p, j, step, CURR)), mkLit(VAR_PLACE(p, j + vardelta, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
				}

				for (arrayindex_t j = 0; j > -1 * delta; j--)
				{
					if (VariableBoundPercentage != 1)
					{
						S->addClause(mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), ~mkLit(VAR_PLACE(p, PlaceVarCount[p][nextstep] + j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)), mkLit(VAR_PLACE(p, PlaceVarCount[p][nextstep] + j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						if (j + vardelta > 0)
						{
							S->addClause(mkLit(VAR_PLACE(p, 1, step, CURR)), ~mkLit(VAR_PLACE(p, j + vardelta, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
							S->addClause(~mkLit(VAR_PLACE(p, 1, step, CURR)), mkLit(VAR_PLACE(p, j + vardelta, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
						}
					}
					else
					{
						S->addClause(~mkLit(VAR_PLACE(p, PlaceVarCount[p][nextstep] + j, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
						if (j + vardelta > 0)
						{
							S->addClause(mkLit(VAR_PLACE(p, j + vardelta, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
						}
					}
				}
			}
		}
	}
	else
	{
		for (arrayindex_t i = 0; i < net->CardDeltaT[POST][t]; i++)
		{
			PlaceValues[net->DeltaT[POST][t][i]][3] += net->MultDeltaT[POST][t][i];
		}
		for (arrayindex_t i = 0; i < net->CardDeltaT[PRE][t]; i++)
		{
			const arrayindex_t p = net->DeltaT[PRE][t][i];
			const mult_t delta = net->MultDeltaT[PRE][t][i];
			if (PlaceValues[p][1] > delta)
			{
				PlaceValues[p][1] -= delta;
			}
			else
			{
				PlaceValues[p][1] = 0;
			}
		}
	}
	// now the remaining places just need to be copied and can be identified via the Computed array
	for (arrayindex_t p = 0; p < net->Card[PL]; p++)
	{
		// also, if the next step is the state after the first round, we can update PlaceMinValue[p][0]
		if (isEndofFirstRound)
		{
			PlaceValues[p][2] = PlaceValues[p][3];
			PlaceValues[p][0] = PlaceValues[p][1];
		}
		if (willCopyAnyway || !Computed[p])
		{
			for (arrayindex_t i = 1; i <= PlaceVarCount[p][step]; i++)
			{
				S->addClause(mkLit(VAR_PLACE(p, i, step, CURR)), ~mkLit(VAR_PLACE(p, i, nextstep, NEXT)));
				S->addClause(~mkLit(VAR_PLACE(p, i, step, CURR)), mkLit(VAR_PLACE(p, i, nextstep, NEXT)));
			}
		}
		else
		{
			// we can reset the Computed value of the ones computed before for the next step
			Computed[p] = false;
		}
	}
	// finally, we need to compute the change of the atomic predicates
	// fortunately, we don't need to iterate beforehand to update AtomicVarCount,
	// since the predicates are always computed in the same, ascending order
	for (arrayindex_t c = 0; c < AtomicCount; c++)
	{
		if (!willCopyAnyway && AtomicChange[t][c] != 0)
		{
			// need to adjust variable count first
			// having a non zero change needs new variables, either on the positive end or the negative end
			// int absChange = abs(AtomicChange[t][c]);
			arrayindex_t nextmax = AtomicVarCount[c][step] + abs(AtomicChange[t][c]);
			AtomicVarCount[c][nextstep] = (AtomicAllowedMax[c] > 0 && nextmax > AtomicAllowedMax[c]) ? AtomicAllowedMax[c] : nextmax;
			arrayindex_t vardelta = AtomicVarCount[c][nextstep] - AtomicVarCount[c][step];
			// need to update neg count and minValue
			if (AtomicChange[t][c] < 0)
			{
				AtomicNegCount[c][nextstep] = AtomicNegCount[c][step] + vardelta;
				AtomicMinValue[c][1] += AtomicChange[t][c];
			}
			else
			{
				AtomicNegCount[c][nextstep] = AtomicNegCount[c][step];
			}

			stepvariables += AtomicVarCount[c][nextstep];
			// first we tell minisat of the new variables for the next step, we do that separately so
			// there are enough variables for the adresses needed, since we can theoretically use more
			// adresses before introducing enough variables otherwise
			for (arrayindex_t j = 0; j < AtomicVarCount[c][nextstep]; j++)
			{
				S->newVar();
			}
			for (arrayindex_t w = 1; w <= AtomicChange[t][c]; w++)
			{
				if (VariableBoundPercentage != 1)
				{
					S->addClause(mkLit(VAR_CONS(c, 1, step, CURR)), ~mkLit(VAR_CONS(c, w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_CONS(c, 1, step, CURR)), mkLit(VAR_CONS(c, w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					if (w <= vardelta)
					{
						S->addClause(mkLit(VAR_CONS(c, AtomicVarCount[c][step], step, CURR)), ~mkLit(VAR_CONS(c, AtomicVarCount[c][step] + w, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
						S->addClause(~mkLit(VAR_CONS(c, AtomicVarCount[c][step], step, CURR)), mkLit(VAR_CONS(c, AtomicVarCount[c][step] + w, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					}
				}
				else
				{
					S->addClause(~mkLit(VAR_CONS(c, AtomicVarCount[c][step] + w, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					S->addClause(mkLit(VAR_CONS(c, w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
				}
			}
			// we now need to introduce index offsets based on positive/negative change
			// positive change means that in step the index i has the same value as index i+change in step+1 when firing
			// and the same value as index i in step+1 when copying
			// while negative change means that in step the index i has the same value as index i in step+1 when firing
			// and the same value as index i+abs(change) in step+1 when copying, since negative change introduces new
			// variables at the bottom end but indexing still starts with 1

			// so the firing offset should be 0 in case of non positive (including 0) change
			int indexOffsetFire = AtomicChange[t][c] > 0 ? AtomicChange[t][c] : AtomicChange[t][c] + vardelta;
			// while the copy offset should be 0 in case of positive change
			int indexOffsetCopy = AtomicChange[t][c] - indexOffsetFire;
			// analogous to places, variables for a constraint cause the value of variables in the next step
			for (arrayindex_t w = 1; w <= AtomicVarCount[c][step]; w++)
			{
				// firing
				if (w + indexOffsetFire <= AtomicVarCount[c][nextstep] && w + indexOffsetFire > 0)
				{
					S->addClause(mkLit(VAR_CONS(c, w, step, CURR)), ~mkLit(VAR_CONS(c, w + indexOffsetFire, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_CONS(c, w, step, CURR)), mkLit(VAR_CONS(c, w + indexOffsetFire, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
				}

				// copying, note that we substract the offset to remove the need to take the abs of AtomicChange above
				// in calculating the offset, we know that the offset is 0 for positive change
				// and since the change is negative otherwise, we know that the offset is negative for negative change, meaning subtracting adds the abs(offset)
				// the case of 0 change is trivially included
				S->addClause(mkLit(VAR_CONS(c, w, step, CURR)), ~mkLit(VAR_CONS(c, w - indexOffsetCopy, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
				S->addClause(~mkLit(VAR_CONS(c, w, step, CURR)), mkLit(VAR_CONS(c, w - indexOffsetCopy, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
			}
			// also, with negative change the last k variables are not set when firing and the first
			// k aren't set when copying, mirrored compared to positive change
			for (arrayindex_t w = 0; w > AtomicChange[t][c]; w--)
			{
				if (VariableBoundPercentage != 1)
				{
					S->addClause(mkLit(VAR_CONS(c, AtomicVarCount[c][step], step, CURR)), ~mkLit(VAR_CONS(c, AtomicVarCount[c][nextstep] + w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(~mkLit(VAR_CONS(c, AtomicVarCount[c][step], step, CURR)), mkLit(VAR_CONS(c, AtomicVarCount[c][nextstep] + w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					if (w - indexOffsetCopy > 0)
					{
						S->addClause(~mkLit(VAR_CONS(c, 1, step, CURR)), mkLit(VAR_CONS(c, w - indexOffsetCopy, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
						S->addClause(mkLit(VAR_CONS(c, 1, step, CURR)), ~mkLit(VAR_CONS(c, w - indexOffsetCopy, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
					}
				}
				else
				{
					S->addClause(~mkLit(VAR_CONS(c, AtomicVarCount[c][nextstep] + w, nextstep, NEXT)), ~mkLit(VAR_FIRE(currentfire)));
					S->addClause(mkLit(VAR_CONS(c, w - indexOffsetCopy, nextstep, NEXT)), mkLit(VAR_FIRE(currentfire)));
				}
			}
		}
		else
		{
			// need to adjust variable count first
			// having a non zero change needs new variables, either on the positive end or the negative end
			AtomicVarCount[c][nextstep] = AtomicVarCount[c][step];
			// need to update neg count and minValue
			AtomicNegCount[c][nextstep] = AtomicNegCount[c][step];
			if (AtomicChange[t][c] < 0)
			{
				AtomicMinValue[c][1] += AtomicChange[t][c];
			}
			stepvariables += AtomicVarCount[c][nextstep];
			// first we tell minisat of the new variables for the next step, we do that separately so
			// there are enough variables for the adresses needed, since we can theoretically use more
			// adresses before introducing enough variables otherwise
			for (arrayindex_t i = 1; i <= AtomicVarCount[c][step]; i++)
			{
				S->newVar();
				S->addClause(mkLit(VAR_CONS(c, i, step, CURR)), ~mkLit(VAR_CONS(c, i, nextstep, NEXT)));
				S->addClause(~mkLit(VAR_CONS(c, i, step, CURR)), mkLit(VAR_CONS(c, i, nextstep, NEXT)));
			}
		}

		// additionally, if step + 1 is the state after the first round, we need to update AtomicMinValue[c][0]
		if (isEndofFirstRound)
		{
			AtomicMinValue[c][0] = AtomicMinValue[c][1];
			if (!verboseVariables)
			{
				AtomicVarCount[c][2] = AtomicVarCount[c][nextstep];
				AtomicNegCount[c][2] = AtomicNegCount[c][nextstep];
			}
		}
	}
	step = nextstep;
	currentfire++;
	OverallVarCount += LastStepVariableCount;
	LastStepVariableCount = stepvariables;
}

// CREATE WHOLE FORMULA
// ====================
// = Initial and AND(i=0..length-1) AND(t=0..Card[TR]) Phi^t and Constraint

void Reachability2Minisat::create_whole_formula()
{
	// Reset some helper fields
	step = 0;
	OverallVarCount = PathLength * TransitionCount;
	Extras = 0;
	currentfire = 0;
	if (verboseVariables)
		RenamedSubs.clear();

	// Initialize Array helper fields
	memset(PlaceAllowedMax, 0, sizeof(arrayindex_t) * net->Card[PL]);
	memset(Computed, false, sizeof(bool) * net->Card[PL]);
	memset(AtomicAllowedMax, 0, sizeof(arrayindex_t) * AtomicCount);

	// Declare variables for fire/copy decision for minisat
	for (arrayindex_t i = 0; i < PathLength * TransitionCount; i++)
		S->newVar();

	// Create initial formula which also initializes the remaining helper fields
	create_initial();

	// Create phi^t for every transition for every 'major step'
	for (arrayindex_t k = 0; k < PathLength; k++)
	{
		for (arrayindex_t t = 0; t < TransitionCount; t++)
		{
			create_phi(TOrder[t], k == 0 && t == TransitionCount - 1);
		}
		// update allowed max
		// only need to do that if we allow another value than 100%
		if (VariableBoundPercentage != 1)
		{
			for (arrayindex_t p = 0; p < net->Card[PL]; p++)
			{
				// need to set after first round
				if (k == 0)
				{
					PlaceAllowedMax[p] = PlaceVarCount[p][step];
				}
				// then for next round can raise
				if (PlaceNegCount[p][step] == 1)
				{
					PlaceAllowedMax[p] += ceil(VariableBoundPercentage * (PlaceValues[p][2] - net->Initial[p]));
				}
				else
				{
					PlaceAllowedMax[p] += ceil(VariableBoundPercentage * (PlaceValues[p][2] - PlaceValues[p][0]));
				}
			}
			for (arrayindex_t a = 0; a < AtomicCount; a++)
			{
				if (k == 0)
				{
					AtomicAllowedMax[a] = AtomicVarCount[a][step];
				}
				// arrayindex_t VarCountAtInitial = Atomic[a]->sum == 0 ? 2 : abs(Atomic[a]->sum) + 1;
				AtomicAllowedMax[a] += ceil(VariableBoundPercentage * (AtomicVarCount[a][verboseVariables ? TransitionCount : 2] - 2));
			}
		}
	}
	int result = -3;
	vec<Lit> lits;
	bool needOrAdd = false;
	if (spFormula->isatomic)
	{
		needOrAdd = true;
		result = create_atomic(spFormula, lits);
	}
	else if (spFormula->isfireable)
	{
		FireablePredicate *f = (FireablePredicate *)(spFormula);
		if (f->sign && !IsOrdered[f->t])
		{
			S->addEmptyClause();
			isNeverSat = true;
		}
		else if (IsOrdered[f->t])
		{
			if (!(f->sign) || net->CardArcs[TR][PRE][f->t] == 1)
				needOrAdd = true;
			result = create_fireable(f, lits, 0);
		}
	}
	else if (spFormula->magicnumber == MAGIC_NUMBER_DEADLOCK)
	{
		result = create_deadlock(0);
	}
	else if (spFormula->magicnumber == MAGIC_NUMBER_NODEADLOCK)
	{
		needOrAdd = true;
		result = create_nodeadlock(lits);
	}
	else
	{
		// now only and/or are left
		AtomicBooleanPredicate *b = (AtomicBooleanPredicate *)spFormula;
		// if the formula is and, we need to add a clause for every subformula
		if (b->isAnd)
		{
			result = create_and(b, 0);
		}
		// if the formula is or, we need to add one clause, literals being atomics or variables for subformulae
		else
		{
			needOrAdd = true;
			result = create_or(b, lits);
		}
	}
	if (result != -3)
	{
		if (result == -2)
		{
			isNeverSat = true;
			S->addEmptyClause();
		}
		else if (result == 0)
		{
			if (needOrAdd)
			{
				S->addClause(lits);
			}
		}
		else if (result > 0)
		{
			S->addEmptyClause();
			minRounds = result > minRounds ? result : minRounds;
		}
	}
}

reachability_result_t Reachability2Minisat::lola2minisat(int round, double varbound)
{
	// Get Value for PathLength and, if everything should be saved for reconstuction
	PathLength = round;
	// also get UpperPlaceBound
	VariableBoundPercentage = varbound;
	// get minisat args and turn them into argc/argv
	// we guard this so its only done once
	if (!parsedArgs)
	{
		char *original;
		if (RT::args.minisatargs_given)
		{
			original = RT::args.minisatargs_arg;
		}
		else
		{
			original = new char[1];
			original[0] = '\0';
		}
		argcc = 0;
		char *passed_string = new char[strlen(original) + 5];
		strcpy(passed_string, "x x "); // dummy file name
		strcpy(passed_string + 4, original);
		for (int i = 0; i < strlen(passed_string); i++)
		{
			if (passed_string[i] > ' ' && passed_string[i + 1] <= ' ')
			{
				// count ends of visible char sequences
				argcc++;
			}
		}
		argvv = new char *[argcc];
		int argnr = 0;
		int j = strlen(passed_string);
		for (int i = 0; i < j; i++)
		{
			if (i == 0 ||
				(passed_string[i] > ' ' && passed_string[i - 1] == '\0'))
			{
				argvv[argnr++] = passed_string + i;
			}
			if (passed_string[i] <= ' ')
			{
				passed_string[i] = '\0';
			}
		}
	}

	// run minisat
	return run_minisat();
}

// signal handler: cannot be tested
// LCOV_EXCL_START

static Solver *solver;
void printStats(Solver &solver);
static void SIGINT_interrupt(int signum) { solver->interrupt(); }
static void SIGINT_exit(int signum)
{
	printf("\n");
	printf("*** INTERRUPTED ***\n");
	if (solver->verbosity > 0)
	{
		printStats(*solver);
		printf("\n");
		printf("*** INTERRUPTED ***\n");
	}
	_exit(1);
}

// LCOV_EXCL_STOP

reachability_result_t Reachability2Minisat::run_minisat()
{
	try
	{
		setUsageHelp("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n");
#if defined(__linux__)
		fpu_control_t oldcw, newcw;
		_FPU_GETCW(oldcw);
		newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
		_FPU_SETCW(newcw);
#endif

		if (!parsedArgs)
		{
			parseOptions(argcc, argvv, true);
			parsedArgs = true;
		}

		S = new SimpSolver();
		double initial_time = cpuTime();

		if (!pre)
			S->eliminate(true);

		S->verbosity = verb;

		solver = S;
		// Use signal handlers that forcibly quit until the solver will be able to respond to
		// interrupts:
		//signal(SIGINT, SIGINT_exit);
		//signal(SIGXCPU,SIGINT_exit);

		// Set limit on CPU-time:
		if (cpu_lim != std::numeric_limits<int>::max())
		{
			rlimit rl;
			getrlimit(RLIMIT_CPU, &rl);
			if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max)
			{
				rl.rlim_cur = cpu_lim;
				if (setrlimit(RLIMIT_CPU, &rl) == -1)
					printf("WARNING! Could not set resource limit: CPU-time.\n");
			}
		}

		// Set limit on virtual memory:
		if (mem_lim != std::numeric_limits<int>::max())
		{
			rlim_t new_mem_lim = (rlim_t)mem_lim * 1024 * 1024;
			rlimit rl;
			getrlimit(RLIMIT_AS, &rl);
			if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max)
			{
				rl.rlim_cur = new_mem_lim;
				if (setrlimit(RLIMIT_AS, &rl) == -1)
					printf("WARNING! Could not set resource limit: Virtual memory.\n");
			}
		}

		//        if (argc == 1)
		//            printf("Reading from standard input... Use '--help' for help.\n");
		//
		//        gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
		//        if (in == NULL)
		//            printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]), exit(1);

		//        parse_DIMACS(in, S);
		//        gzclose(in);
		struct timeval start, translation, solving;
		double pretime = cpuTime();
		//auto starttime = high_resolution_clock::now();
		gettimeofday(&start, NULL);
		create_whole_formula();
		gettimeofday(&translation, NULL);
		//double last_trans_t = cpuTime();
		//auto transtime = high_resolution_clock::now();
		RT::rep->status("STP: formula with %u variables and %u clauses shipped to Minisat", S->nVars(), S->nClauses());
		//portfoliomanager::taskjson[portfolio_id]["reachability_sat"]["literals"] = S->nVars();
		//portfoliomanager::taskjson[portfolio_id]["reachability_sat"]["clauses"] = S->nClauses();
		ostringstream stats;
		if (S->verbosity > 1)
		{
			stats << "\n============================[ Problem Statistics ]=============================\n"
				  << "|                                                                             |\n"
				  << "|  Number of variables:  " << setw(12) << S->nVars() << "                                         |\n"
				  << "|  Number of clauses:    " << setw(12) << S->nClauses() << "                                         |\n";
		}
		FILE *res = (argcc >= 3) ? fopen(argvv[2], "wb") : NULL;
		double parsed_time = cpuTime();
		S->eliminate(true);
		double simplified_time = cpuTime();

		if (S->verbosity > 1)
		{

			stats << "|  Parse time:           " << setw(12) << setprecision(2) << parsed_time - initial_time << " s                                       |\n"
				  << "|  Simplification time:  " << setw(12) << setprecision(2) << simplified_time - parsed_time << " s                                       |\n"
				  << "|                                                                             |\n";
		}

		// Change to signal-handlers that will only notify the solver and allow it to terminate
		// voluntarily:
		//signal(SIGINT, SIGINT_interrupt);
		//signal(SIGXCPU,SIGINT_interrupt);

		if (!S->okay())
		{
			if (res != NULL)
				fprintf(res, "UNSAT\n"), fclose(res);
			if (S->verbosity > 1)
			{
				stats << "===============================================================================\n"
					  << "Solved by simplification"
					  << flush;
				const string result = stats.str();
				RT::rep->status(result.c_str());
				printStats(*S);
			}

			clearMem();
			if (isNeverSat)
			{
				return REACHABILITY_PROPERTY_FALSE;
			}
			return REACHABILITY_PROPERTY_INDETERMINATE;
		}

		if (dimacs)
		{
			S->toDimacs((const char *)dimacs);
			if (S->verbosity > 1)
			{
				stats << "==============================[ Writing DIMACS ]===============================\n"
					  << flush;
				const string result = stats.str();
				RT::rep->status(result.c_str());
				printStats(*S);
			}
			clearMem();
			return REACHABILITY_PROPERTY_INDETERMINATE;
		}

		vec<Lit> dummy;
		lbool ret = S->solveLimited(dummy);
		gettimeofday(&solving, NULL);
		//double last_solve_t = cpuTime();
		//auto solvetime = high_resolution_clock::now();

		stats << flush;
		const string prelim = stats.str();
		RT::rep->status(prelim.c_str());
		stats.str("");
		stats.clear();
		if (S->verbosity > 1)
		{
			printStats(*S);
			printf("\n");
		}
		if (res != NULL)
		{
			if (ret == l_True)
			{
				fprintf(res, "SAT\n");
				for (int i = 0; i < S->nVars(); i++)
					if (S->model[i] != l_Undef)
						fprintf(res, "%s%s%d", (i == 0) ? "" : " ", (S->model[i] == l_True) ? "" : "-", i + 1);
				fprintf(res, " 0\n");
			}
			else if (ret == l_False)
				fprintf(res, "UNSAT\n");
			else
				fprintf(res, "INDET\n");
			fclose(res);
		}

		if (ret != l_True)
		{
			if (RT::args.satheuristicskip_given)
			{
				//last_trans_t = duration_cast<milliseconds>(transtime - starttime).count();
				//last_solve_t = duration_cast<milliseconds>(solvetime - transtime).count();
				long unsigned int last_trans_t = (translation.tv_sec - start.tv_sec) * 1000000 + translation.tv_usec - start.tv_usec;
				long unsigned int last_solve_t = (solving.tv_sec - translation.tv_sec) * 1000000 + solving.tv_usec - translation.tv_usec;
				setRoundSkip(last_trans_t, last_solve_t);
			}
			clearMem();
			return REACHABILITY_PROPERTY_INDETERMINATE;
		}
		assert(ret == l_True);

		// before returning true, we have to check if the UpperBound was breached for any place
		// this can simply be checked by checking the last variable in the last step
		// since breaching at some step means breaching in all following steps
		// not breached would mean the last variable is false, since then the value
		// is smaller than the upperbound
		for (arrayindex_t p = 0; p < net->Card[PL]; p++)
		{
			if (S->model[VAR_PLACE(p, PlaceVarCount[p][step], step, CURR)] == l_True)
			{
				clearMem();
				return REACHABILITY_PROPERTY_BOUND_BREACHED;
			}
			if (S->model[VAR_PLACE(p, 1, step, CURR)] == l_False)
			{
				clearMem();
				return REACHABILITY_PROPERTY_BOUND_BREACHED;
			}
		}
		for (arrayindex_t a = 0; a < AtomicCount; a++)
		{
			if (S->model[VAR_CONS(a, AtomicVarCount[a][step], step, CURR)] == l_True)
			{
				clearMem();
				return REACHABILITY_PROPERTY_BOUND_BREACHED;
			}
			if (S->model[VAR_CONS(a, 1, step, CURR)] == l_False)
			{
				clearMem();
				return REACHABILITY_PROPERTY_BOUND_BREACHED;
			}
		}
		if (RT::args.satoutput_given)
			printAssignments(stats);
		const string result = stats.str();
		RT::rep->status(result.c_str());

		clearMem();
		return REACHABILITY_PROPERTY_TRUE;
		//#ifdef NDEBUG
		//        exit(ret == l_True ? 10 : ret == l_False ? 20 : 0);     // (faster than "return", which will invoke the destructor for 'Solver')
		//#else
		//        return (ret == l_True ? 10 : ret == l_False ? 20 : 0);
		//#endif
	}
	catch (OutOfMemoryException &)
	{
		printf("===============================================================================\n");
		printf("INDETERMINATE\n");
		exit(0);
	}
}

void Reachability2Minisat::clearMem()
{
	delete S;
	if (verboseVariables)
	{
		for (arrayindex_t p = 0; p < net->Card[PL]; p++)
		{
			delete[] PlaceVarCount[p];
			delete[] PlaceNegCount[p];
		}
		for (arrayindex_t c = 0; c < AtomicCount; c++)
		{
			delete[] AtomicVarCount[c];
			delete[] AtomicNegCount[c];
		}
	}
	for (vector<StatePredicate *>::iterator s = RenamedSubs.begin(); s != RenamedSubs.end(); ++s)
	{
		delete *s;
	}
	RenamedSubs.clear();
}

// below: only debig functions
// LCOV_EXCL_START

const string printVarValue(lbool v)
{
	if (v == l_True)
	{
		return string("T");
	}
	if (v == l_False)
	{
		return string("F");
	}
	if (v == l_Undef)
	{
		return string("U");
	}
	return string("?");
}

void Reachability2Minisat::printAssignments(ostringstream &result)
{
	result << "Assignments for " << portfolio_id << ": \n";
	if (verboseVariables)
	{
		for (arrayindex_t s = 0; s <= step; s++)
		{
			// Print place var for step
			for (arrayindex_t p = 0; p < net->Card[PL]; p++)
			{
				for (arrayindex_t i = 1; i <= PlaceVarCount[p][s]; i++)
				{
					result << VAR_PLACE(p, i, s, CURR) << " = P(" << p << ", " << i - PlaceNegCount[p][s] << ", " << s << "): " << printVarValue(S->model[VAR_PLACE(p, i, s, CURR)]) << "\n";
					// printf("%u = P(%u, %u, %u): ", VAR_PLACE(p, i, s, CURR), p, i, s);
					// printVarValue(S->model[VAR_PLACE(p, i, s, CURR)]);
					// printf("\n");
				}
			}
			for (arrayindex_t c = 0; c < AtomicCount; c++)
			{
				for (arrayindex_t w = 1; w <= AtomicVarCount[c][s]; w++)
				{
					result << VAR_CONS(c, w, s, CURR) << " = A(" << c << ", " << w - AtomicNegCount[c][s] << ", " << s << "): " << printVarValue(S->model[VAR_CONS(c, w, s, CURR)]) << "\n";
					// printf("%u = A(%u, %d, %u): ", VAR_CONS(c, w, s, CURR), c, w - AtomicNegCount[c][s], s);
					// printVarValue(S->model[VAR_CONS(c, w, s, CURR)]);
					// printf("\n");
				}
			}
			// Print fire/copy var, but only if s != final step
			if (s != step)
			{
				result << VAR_FIRE(s) << " = Fire transition " << net->Name[TR][TOrder[s % TransitionCount]] << ": " << printVarValue(S->model[VAR_FIRE(s)]) << "\n";
				// printf("%u = Fire transition #%u: ", VAR_FIRE(s), s % net->Card[TR]);
				// printVarValue(S->model[VAR_FIRE(s)]);
				// printf("\n");
			}
		}
		for (arrayindex_t e = 1; e <= Extras; e++)
		{
			result << VAR_EXTRA(e) << " = Renamed Formula \"" << RenamedSubs.at(e - 1)->toString() << "\": " << printVarValue(S->model[VAR_EXTRA(e)]) << "\n";
		}
	}
	else
	{
		// don't have verbose variables, but we can still print the transition sequence
		for (arrayindex_t i = 0; i < PathLength * TransitionCount; i++)
		{
			result << VAR_FIRE(i) << " = Fire transition " << net->Name[TR][TOrder[i % TransitionCount]] << ": " << printVarValue(S->model[VAR_FIRE(i)]) << "\n";
			// printf("%u = Fire transition #%u: ", VAR_FIRE(i), i % net->Card[TR]);
			// printVarValue(S->model[VAR_FIRE(i)]);
			// printf("\n");
		}
	}
}

// LCOV_EXCL_STOP

arrayindex_t Reachability2Minisat::GetStartRound()
{
	arrayindex_t round = RT::args.satstart_arg;
	// if (RT::args.satheuristicstart_given)
	// {
	// 	// subject to change
	// 	arrayindex_t temp = ceil((net->Card[TR] + net->Card[PL]) * 0.1);
	// 	if (RT::args.satrounds_arg > 0)
	// 	{
	// 		temp = temp > RT::args.satrounds_arg ? RT::args.satrounds_arg : temp;
	// 	}
	// 	round = temp > round ? temp : round;
	// }
	return round;
}

void Reachability2Minisat::setRoundSkip(long unsigned int t, long unsigned int s)
{
	// if the solve time was basically 0, then we don't need to worry about that and can easily solve another round with
	// probably no real difference, therefore skipping rounds could result in longer calculation
	if (t != 0 && s != 0)
	{
		double ratio = (double)t / (double)s;
		// if ratio is greater than ~1, translation takes longer than solving a non-solution round, therefore we would rather solve than
		// skip rounds to account for solution round being close to current round
		if (ratio < 1 && ratio > 0)
		{
			ratio = ratio > 0.1 ? ratio : 0.1;
			ratio = floorf(ratio * 10) / 10;
			// solving now takes longer than translating rounds, therefore we could just translate more rounds and hope to avoid
			// having to solve more non-solution rounds
			// the amount of rounds to skip depends on the ratio and the current round, if the current round is high and the ratio
			// small, we can skip more rounds than if the round is small and ratio large or close to 1
			// \todo exact addition may be adjusted with constants, needs testing
			arrayindex_t nextround = ceil(RT::args.satratio_arg * PathLength / sqrt(ratio)) - skippedRounds;
			skippedRounds += nextround - PathLength;
			if (RT::args.satrounds_arg != 0 && nextround >= RT::args.satrounds_arg)
			{
				minRounds = RT::args.satrounds_arg;
			}
			else
			{
				minRounds = nextround;
			}
			// cout << last_trans_t << " last translation time\n"
			// 	 << last_solve_t << " last solve time\nskipping from " << PathLength << " to " << minRounds
			// 	 << " for TASK " << portfolio_id << "\n\n";
			RT::rep->status("round skipping for %d:\nlast translation time: %lu\nlast solving time: %lu\nratio: %f\nskipping from round %d to round %d", portfolio_id, t, s, ratio, PathLength, minRounds);
		}
	}
}
