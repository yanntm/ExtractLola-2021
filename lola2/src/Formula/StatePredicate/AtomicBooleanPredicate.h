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

\brief class definition for Boolean state predicates in non-temporal
subformulas
*/

#pragma once

#include "Formula/FormulaStatistics.h"
#include "Net/Petrinet.h"
class StatePredicate;
class AtomicStatePredicate;

/// A disjunction state predicate is an OR formula with multiple children that
/// assigns a Boolean value to Marking::Current
class AtomicBooleanPredicate : public StatePredicate
{
public:
    bool isAnd;
    /// arg is number of subformulas
    explicit AtomicBooleanPredicate(Petrinet *, bool); /// true = conj, false = disj
    void merge(AtomicBooleanPredicate *); // turn two into one
    void mergeSorted(AtomicBooleanPredicate *); // turn two into one

    virtual ~AtomicBooleanPredicate();

    /// adds subformula
    void addSub(StatePredicate *f);

    /// evaluates a formula, e.g. upon initialization
    virtual void evaluate(NetState &ns);

    /// evaluates a formula including omega values
    virtual void evaluateOmega(NetState &ns);

    // direct read access for the deletion algorithm
    virtual arrayindex_t getSubs( StatePredicate * **subs) ;

    bool isOrNode() ; // for stubborn set deletion algorithm

    /// returns info on a particular node in the formula tree
    FormulaInfo *getInfo() ;
    /// returns the number of subformulas
    int countSubFormulas() ;

    /// fills stack with an up set of the predicate and return size of upset
    virtual arrayindex_t getUpSet(arrayindex_t *stack, bool *onstack, bool *needEnabled) ;

    /// updates the value of the predicate from true to false
    virtual void updateTF(arrayindex_t);

    /// updates the value of the predicate from false to true
    virtual void updateFT(arrayindex_t);

    /// debug function to check consistency
    virtual bool DEBUG__consistency(NetState &ns);

    /// the list of subformulas
    StatePredicate **sub;

    /// The number of subformulas;
    arrayindex_t cardSub;

    /// The number of satisfied subformulas
    arrayindex_t cardSat;

    /// counts atomic subformulas
    virtual arrayindex_t countAtomic() ;

    /// collects atomic subformulas
    virtual arrayindex_t collectAtomic(AtomicStatePredicate **);

    /// counts fireable subformulas
    virtual arrayindex_t countFireable() ;

    /// collects fireable subformulas
    virtual arrayindex_t collectFireable(FireablePredicate **);

    /// counts deadlock subformulas
    virtual arrayindex_t countDeadlock() ;

    /// collects deadlock subformulas
    virtual arrayindex_t collectDeadlock(DeadlockPredicate **);

    /// create a new state predicate by copy this object.
    virtual StatePredicate *copy(StatePredicate *);
    virtual char * toString();
    virtual StatePredicate * negate();
     virtual void adjust(arrayindex_t,arrayindex_t);
	virtual void setVisible(bool *);
	virtual void setDown(bool *);
	virtual void setNondestroying(bool *);
    virtual AtomicBooleanPredicate * DNF();
   void addSubSorted(StatePredicate * );
   virtual FormulaStatistics * count(FormulaStatistics *);
   virtual bool monotonous();
	
    virtual bool exec_shift(int *,capacity_t*);
     virtual bool shiftable();
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
    virtual int getLargestConstant();
    virtual void checkCons();
};

