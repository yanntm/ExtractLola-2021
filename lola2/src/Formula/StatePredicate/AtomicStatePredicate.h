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
\author Karsten
\status new

\brief class definition for atomic state predicates
*/

#pragma once

#include <Formula/StatePredicate/StatePredicate.h>
#include <Formula/StatePredicate/Term.h>
#include <Net/Petrinet.h>

/*!
A state predicate is a formula that assigns a Boolean value to Marking::Current
An atomic predicate compares a formal sum of places with a constant The general
shape is: \f$ k_1 \cdot p_1 + \cdots + k_n \cdot p_n \leq k \f$
*/
class AtomicStatePredicate : public StatePredicate
{
public:
    int lower_bound;
    int upper_bound;
    AtomicStatePredicate(Petrinet *,arrayindex_t, arrayindex_t, int);
    AtomicStatePredicate(Petrinet *, Term *,bool);
    AtomicStatePredicate(Petrinet *, Term *);
    AtomicStatePredicate(Petrinet *);

    virtual ~AtomicStatePredicate();

    /// fills stack with an up set of the predicate and return size of upset
    virtual arrayindex_t getUpSet(arrayindex_t *stack, bool *onstack, bool *needEnabled) ;
    virtual arrayindex_t getDownSet(arrayindex_t *stack, bool *onstack, bool *needEnabled) ;

    /// updates the value of this sub formula
    void update(NetState &ns, int);

    /// evaluates a formula, e.g. upon initialization
    virtual void evaluate(NetState &ns);

    /// evaluates a formula including omega values
    virtual void evaluateOmega(NetState &ns);

    void initUpSet(); // initializies static generation of up set
    void finitUpSet(); // finalizes static generation of up set
    void addToUpSet(arrayindex_t); // adds a transition to static up set

    // down set is needed for bound computations
    void initDownSet(); // initializies static generation of up set
    void finitDownSet(); // finalizes static generation of up set
    void addToDownSet(arrayindex_t); // adds a transition to static up set

    /// returns the negated version of this property, but leaves the current one untouched
    virtual StatePredicate *negate();

    /// returns info on a particular node in the formula tree
    FormulaInfo *getInfo() ;
    /// returns the number of subformulas
    int countSubFormulas() ;

    /// apply the gcd to the factors and the sum
    void reduceFactors();

    void checkTrap();
    void checkContainedInvariant();

private:
    /// updates the value of the predicate from true to false
    virtual void updateTF(arrayindex_t) {}   // LCOV_EXCL_LINE

    /// updates the value of the predicate from false to true
    virtual void updateFT(arrayindex_t) {}   // LCOV_EXCL_LINE

    virtual bool DEBUG__consistency(NetState &ns);

    /// direct read access for the deletion algorithm
    virtual arrayindex_t getSubs( StatePredicate * **subs) ;

public:
    /// lists place p_i indices that occur with positive multiplicity k_i
    arrayindex_t *posPlaces;

    /// lists place p_i indices that occur with negative multiplicity k_i
    arrayindex_t *negPlaces;

    /// lists multiplicities  k_i of places in posPlaces
    capacity_t *posMult;

    /// lists multiplicities  k_i of places in negPlaces
    capacity_t *negMult;

    /// the number of pos entries
    arrayindex_t cardPos;

    /// the number of negative entries
    arrayindex_t cardNeg;

    /// the up set of this formula
    arrayindex_t *up;

    /// the size of the up set
    arrayindex_t cardUp;

    /// the down set of this formula (needed for bound calculation)
    arrayindex_t *down;

    /// the size of the down set
    arrayindex_t cardDown;

    /// the threshold k
    int threshold;

    /// The current value of the formal sum k_1 p_1 + ... + k_n p_n
    int sum;

    /// if true, this is an original property an not a copy, thus it has to free the arrays on the free command
    bool original;

    /// counts atomic subformulas
    virtual arrayindex_t countAtomic() ;
    /// counts deadlock subformulas
    virtual arrayindex_t countDeadlock() ;
    virtual arrayindex_t countFireable() ;

    /// collects atomic subformulas; array must be malloced beforehand
    /// result is number of inserted elements
    virtual arrayindex_t collectAtomic(AtomicStatePredicate **);

    /// collects deadlock subformulas; array must be malloced beforehand
    /// result is number of inserted elements
    virtual arrayindex_t collectDeadlock(DeadlockPredicate **);
    virtual arrayindex_t collectFireable(FireablePredicate **);

    // copy function
    virtual StatePredicate *copy(StatePredicate *parent);
    virtual char * toString();
    char * toCompString(); // string when used as compute bound problem
    virtual void adjust(arrayindex_t,arrayindex_t);
   void setVisible(bool *);
   void setDown(bool *);
   void setNondestroying(bool *);
    virtual AtomicBooleanPredicate * DNF();
   virtual FormulaStatistics * count(FormulaStatistics *);
   bool forBound; // true if this object is just a placeholder for a bound computation
    virtual bool monotonous();
    virtual bool exec_shift(int*, capacity_t*);
    virtual bool shiftable(){return true;}
   char * addSummand(char * text, arrayindex_t p, arrayindex_t mult);
   char * addSummand(char * text, Symbol * p, arrayindex_t mult);
    void markvisible();
    void markslicing(bool);
	void relabel();
    StatePredicate * unfold(Petrinet *);
    bool containsFireable();
    bool containsUnfireable();
    StatePredicate * hlunfold();
    virtual bool createskeletonatomic(Create_HL_Net *);
    virtual bool createskeletonfireable(Create_HL_Net *, bool, bool);
    virtual bool createskeletonunfireable(Create_HL_Net *, bool, bool);
    virtual StatePredicate * foldformula(Create_HL_Net *);
    int checkstable(bool sign, lprec * lp);
    void checkonlyimproving();
    virtual int getLargestConstant();
    virtual void checkCons();
    Symbol ** posSymbol;
    Symbol ** negSymbol;
};

