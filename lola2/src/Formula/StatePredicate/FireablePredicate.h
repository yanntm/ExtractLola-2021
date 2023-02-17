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

\brief class definition for fireability as atomic state predicates
*/

#pragma once

#include <Formula/StatePredicate/StatePredicate.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Net/Petrinet.h>

/*!
A state predicate is a formula that assigns a Boolean value to Marking::Current
*/
class FireablePredicate : public StatePredicate
{
public:
    explicit FireablePredicate(Petrinet*,arrayindex_t,bool); // arg: true = fireable, false = !fireable
    explicit FireablePredicate(Petrinet*,TransitionSymbol *,bool); // arg: true = fireable, false = !fireable

    /// updates the value of this sub formula
    void update(NetState &ns);

    /// evaluates a formula, e.g. upon initialization
    virtual void evaluate(NetState &ns);

    /// evaluates a formula including omega values
    virtual void evaluateOmega(NetState &ns);

    /// participate in finding an up-set:
    virtual arrayindex_t getUpSet(arrayindex_t *, bool *, bool *) ;

    /// returns the negated version of this property, but leaves the current one untouched
    virtual StatePredicate *negate();

    /// returns info on a particular node in the formula tree
    FormulaInfo *getInfo() ;
    /// returns the number of subformulas
    int countSubFormulas() ;

private:
    /// updates the value of the predicate from true to false
    void updateTF(arrayindex_t) {}   // LCOV_EXCL_LINE

    /// updates the value of the predicate from false to true
    void updateFT(arrayindex_t) {}   // LCOV_EXCL_LINE

    bool DEBUG__consistency(NetState &ns);

    /// direct read access for the deletion algorithm
    arrayindex_t getSubs( StatePredicate * **subs) ;

public:
    bool sign; // true = property is "fireable", false = property is "not fireable"
    arrayindex_t t;

    /// counts atomic subformulas
    arrayindex_t countAtomic() ;

    /// collects atomic subformulas; array must be malloced beforehand
    /// result is number of inserted elements
    arrayindex_t collectAtomic(AtomicStatePredicate **);

    /// counts deadlock subformulas
    arrayindex_t countDeadlock() ;

    /// collects deadlock subformulas; array must be malloced beforehand
    /// result is number of inserted elements
    arrayindex_t collectDeadlock(DeadlockPredicate **);

    /// counts fireable subformulas
    arrayindex_t countFireable() ;

    /// collects fireable subformulas; array must be malloced beforehand
    /// result is number of inserted elements
    arrayindex_t collectFireable(FireablePredicate **);

    // copy function
    virtual StatePredicate *copy(StatePredicate *);
    virtual char * toString();
    virtual void adjust(arrayindex_t,arrayindex_t){}
	void setVisible(bool *);
	void setDown(bool *);
	void setNondestroying(bool *);
	virtual AtomicBooleanPredicate * DNF();
   virtual FormulaStatistics * count(FormulaStatistics *);
    virtual bool monotonous(){return sign;}	
    virtual bool shiftable(){return false;}	
    virtual bool exec_shift(int*, capacity_t*){return false;}	
    void markvisible();
    void markslicing(bool);
    void relabel();
    StatePredicate * unfold(Petrinet *);
    bool containsFireable();
    bool containsUnfireable();
    StatePredicate * hlunfold();
    virtual bool createskeletonatomic(Create_HL_Net *);
    virtual bool createskeletonfireable(Create_HL_Net *, bool,bool);
    virtual bool createskeletonunfireable(Create_HL_Net *, bool,bool);
    virtual StatePredicate * foldformula(Create_HL_Net *);
    int checkstable(bool sign, lprec * lp);
    int getLargestConstant();
    virtual void checkCons();
    Symbol * trsymbol;
};
