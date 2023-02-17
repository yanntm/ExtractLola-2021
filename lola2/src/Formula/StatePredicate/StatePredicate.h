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

\brief derives deadlock checking from SimpleProperty
*/

#pragma once

#include <Core/Dimensions.h>
#include <Formula/FormulaStatistics.h>
#include <Net/Petrinet.h>
#include "../../../libs/lp_solve_5.5/lp_lib.h"

class Create_HL_Net;
class ProtoNet;

class DeadlockPredicate;
class FormulaInfo;
class NetState;
class AtomicStatePredicate;
class AtomicBooleanPredicate;
class FireablePredicate;

/*!
A state predicate is a formula that assigns a Boolean value to Marking::Current.
*/
class StatePredicate
{
public:
	bool isatomic;
	bool isfireable;

	Petrinet * net;
    /// the value of the predicate in Marking::Current;
    bool value;

    /// flag for unknown value in case of coverability graph checks
    bool unknown;

    /// the parent formula in the syntax tree of the formula
    StatePredicate *parent;

    /// the position of this in parent's subformula list
    arrayindex_t position;

    virtual ~StatePredicate() {}

    /// fills stack with an up set of the predicate and return size of upset
    virtual arrayindex_t getUpSet(arrayindex_t *, bool *, bool *) = 0;

    /// updates the value of the predicate from true to false
    virtual void updateTF(arrayindex_t) = 0;

    /// updates the value of the predicate from false to true
    virtual void updateFT(arrayindex_t) = 0;

    /// evaluates a formula, e.g. upon initialization
    virtual void evaluate(NetState &) = 0;

    /// evaluates a formula including omega values
    virtual void evaluateOmega(NetState &) = 0;

    /// counts atomic subformulas
    virtual arrayindex_t countAtomic() = 0;

    /// collects atomic subformulas
    virtual arrayindex_t collectAtomic(AtomicStatePredicate **) = 0;

    /// counts deadlock subformulas
    virtual arrayindex_t countDeadlock()  = 0;

    /// collects deadlock subformulas
    virtual arrayindex_t collectDeadlock(DeadlockPredicate **) = 0;

    /// counts FIREABLE subformulas
    virtual arrayindex_t countFireable() = 0;

    /// collects FIREABLE subformulas
    virtual arrayindex_t collectFireable(FireablePredicate **) = 0;

    /// debug function to check consistency
    virtual bool DEBUG__consistency(NetState &) = 0;

    /// create a new state predicate by copy this object.
    virtual StatePredicate *copy(StatePredicate *) = 0;

    /// create a new state predicate by copy this object.
    StatePredicate *copy(void);

    /// for direct read access for the deletion algorithm
    virtual arrayindex_t getSubs(StatePredicate ***) = 0;

    /// the following functions are added for the deletion algorithm
    /// the deletion algorithm will only read the state predicates - no changes
    virtual bool isOrNode() ;
    virtual arrayindex_t countUnsatisfied() ; // for and-nodes only!

    /// returns info on a particular node in the formula tree
    virtual FormulaInfo *getInfo()  = 0;
    /// returns the number of subformulas
    virtual int countSubFormulas() = 0;

    virtual char * toString() = 0;
   virtual StatePredicate * negate() = 0;
   virtual void adjust(arrayindex_t,arrayindex_t) = 0;
   virtual void setVisible(bool *) = 0;
   virtual void setNondestroying(bool *) = 0;
   virtual void setDown(bool *) = 0; // mark global down set ;

	/// DNF computation
   /// every atom gets a unique id for duplicate detection in elementary conj 
   /// will be set in recursive descent
   virtual AtomicBooleanPredicate * DNF()=0;
	int magicnumber;
	arrayindex_t literals;
   virtual FormulaStatistics * count(FormulaStatistics *) = 0;
   virtual bool monotonous() = 0; // check whether formula is monotonous in the
			     // sense that additional tokens imply
			     // that true formulas remain true
   virtual bool shiftable() = 0;
   virtual bool exec_shift(int *cutofff, capacity_t*m) = 0; // check whether formula can be adapted such that
			    // frozen tokens can be removed. This adaptation
			    // executed as long as result may be true.
			    // If result is false, shifts are only partially
	      // applied and not reversed --> formula is useless
	      // execute this on  copy!!
	      // cutoff = new initial marking
              // m: old initial marking: necessary for adjustment

   virtual void markvisible() = 0; // mark place and transition symbols visible for this state predicate
   virtual void markslicing(bool) = 0; // mark place and transition symbols visible for this state predicate (boolean par counts negations on top)
   virtual void relabel() = 0;
   virtual StatePredicate * unfold(Petrinet *) = 0; // returns the LL net version of the HL predicate "this"
    virtual bool containsFireable() = 0;
    virtual bool containsUnfireable() = 0;
    virtual StatePredicate * hlunfold() = 0;
    virtual bool createskeletonatomic(Create_HL_Net *) = 0;
    virtual bool createskeletonfireable(Create_HL_Net *, bool,bool) = 0;
    virtual bool createskeletonunfireable(Create_HL_Net *, bool,bool) = 0;
    virtual StatePredicate * foldformula(Create_HL_Net *) = 0;
     virtual int checkstable(bool sign, lprec * lp) = 0;  /// sign true -> check for STABLE FALSE, false -> check for stable true
							  /// lp: lp instance to contribute equations to, if possible
							  /// return: number of equations added
    virtual int getLargestConstant() = 0;
    virtual void checkCons() = 0;
    static bool solve_lp(ProtoNet *, lprec *);
};
