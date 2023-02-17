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

\brief derives constant predicate TRUE
*/

#pragma once

#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Net/Petrinet.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Frontend/Parser/ParserPTNet.h>

class TruePredicate: public StatePredicate
{
public:
    TruePredicate(Petrinet *);

    /// evaluates a formula, e.g. upon initialization
    virtual void evaluate(NetState &) {}

    /// evaluates a formula using omega values
    virtual void evaluateOmega(NetState &) {} // LCOV_EXCL_LINE

    /// for direct read access for the deletion algorithm
    virtual arrayindex_t getSubs( StatePredicate * **subs) ;

    /// negates the formula
    virtual StatePredicate *negate();

    /// returns info on a particular node in the formula tree
    FormulaInfo *getInfo() ;
    /// returns the number of subformulas
    int countSubFormulas() ;

    /// fills stack with an up set of the predicate and return size of upset
    virtual arrayindex_t getUpSet(arrayindex_t *stack, bool *onstack, bool *needEnabled) ;

    /// updates the value of the predicate from true to false
    virtual void updateTF(arrayindex_t) {} // LCOV_EXCL_LINE

    /// updates the value of the predicate from false to true
    virtual void updateFT(arrayindex_t) {} // LCOV_EXCL_LINE

    /// debug function to check consistency
    virtual bool DEBUG__consistency(NetState &ns);

    /// counts atomic subformulas
    virtual arrayindex_t countAtomic() ;

    /// collects atomic subformulas
    virtual arrayindex_t collectAtomic(AtomicStatePredicate **);

    /// counts deadlock subformulas
    virtual arrayindex_t countDeadlock() ;

    /// collects deadlock subformulas
    virtual arrayindex_t collectDeadlock(DeadlockPredicate **);

    /// counts fireable subformulas
    arrayindex_t countFireable() {return 0;}

    /// collects fireable subformulas
    virtual arrayindex_t collectFireable(FireablePredicate **){return 0;}

    /// create a new state predicate by copy this object.
    virtual StatePredicate *copy(StatePredicate *parent);
    virtual char * toString();
    virtual void adjust(arrayindex_t,arrayindex_t){}
   void setVisible(bool *){}
   void setDown(bool *){}
   void setNondestroying(bool *){}
    virtual AtomicBooleanPredicate * DNF();
    virtual FormulaStatistics * count(FormulaStatistics *);
    virtual bool monotonous(){return true;}
    virtual bool shiftable(){return true;}
    virtual bool exec_shift(int*, capacity_t*){return true;}
     virtual void markvisible(){}
     virtual void markslicing(bool){}
     void relabel(){net = ParserPTNet::relabelnet;}
     StatePredicate * unfold(Petrinet *);
    bool containsFireable();
    bool containsUnfireable();
    StatePredicate * hlunfold();
    virtual bool createskeletonatomic(Create_HL_Net *) {return true;}
    virtual bool createskeletonfireable(Create_HL_Net *, bool,bool) {return true;}
    virtual bool createskeletonunfireable(Create_HL_Net *, bool,bool) {return true;}
    virtual StatePredicate * foldformula(Create_HL_Net *);
    int checkstable(bool sign, lprec * lp){return 0;}
 int getLargestConstant(){return 0;}
    virtual void checkCons(){} 
};
