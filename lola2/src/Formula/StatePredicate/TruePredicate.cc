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
\file TruePredicate.cc
\author Karsten
\status new

\brief derives constant predicate TRUE
*/

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Formula/StatePredicate/TruePredicate.h>
#include <Formula/StatePredicate/FalsePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/FormulaInfo.h>
#include <Formula/StatePredicate/MagicNumber.h>


TruePredicate::TruePredicate(Petrinet * n)
{
	isfireable = isatomic = false;
     net = n;
    literals = 0;
    value = true;
    unknown = false;
    magicnumber = MAGIC_NUMBER_TRUE;
}

/*!
Fills stack with an up set of the predicate. Added elements are marked true in
onstack.

\return the size of the up set

\note getUpSet can be called only if the predicate is not satisfied. The
returned set of transitions has the property that it is impossible to turn the
predicate true without firing one of the transitions.
*/
//  true predicate occurs only in initial sat problems since it is eleiminated by rewriting ffrom all other formulas. Hence, certain methods are never called

// LCOV_EXCL_START
arrayindex_t TruePredicate::getUpSet(arrayindex_t *, bool *, bool * needEnabled) 
{
    * needEnabled = false;
    return 0;
}

// LCOV_EXCL_STOP
/*!
\return number of atomic subformulas
*/
arrayindex_t TruePredicate::countAtomic() 
{
    return 0;
}

/*!
\note array must be malloced beforehandresult is number of inserted elements
*/
arrayindex_t TruePredicate::collectAtomic(AtomicStatePredicate **)
{
    return 0;
}

/*!
\return number of deadlock subformulas
*/
arrayindex_t TruePredicate::countDeadlock() 
{
    return 0;
}

/*!
\note array must be malloced beforehandresult is number of inserted elements
*/
arrayindex_t TruePredicate::collectDeadlock(DeadlockPredicate **)
{
    return 0;
}

// LCOV_EXCL_START
/*!
\param parent  the parent predicate for the new, copied, object
*/
 StatePredicate *TruePredicate::copy(StatePredicate *parent)
{
    TruePredicate *p = new TruePredicate(net);
    p->parent = parent;
    p->position = position;
    p->magicnumber = magicnumber;
    return p;
}

arrayindex_t TruePredicate::getSubs( StatePredicate * **) 
{
    return 0;
}

StatePredicate *TruePredicate::negate()
{
    return new FalsePredicate(net);
}

// LCOV_EXCL_STOP

bool TruePredicate::DEBUG__consistency(NetState &)
{
    return true;
}

FormulaInfo *TruePredicate::getInfo() 
{
    FormulaInfo *Info = new FormulaInfo();

    Info->tag = formula_true;
    Info->cardChildren = 0;
    return Info;
}

int TruePredicate::countSubFormulas() 
{
    return 1;
}



char * TruePredicate::toString()
{
	char * result = (char *) malloc(20 * sizeof(char));
	sprintf(result,"TRUE");
	return result;
}

class BooleanAtomicPredicate;

AtomicBooleanPredicate * TruePredicate::DNF()
{
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(net,true); // empty conjunction
	result -> magicnumber = magicnumber;
	return result;
}

FormulaStatistics * TruePredicate::count(FormulaStatistics * fs)
{
	fs -> taut++;
	return fs;
}

StatePredicate *TruePredicate::unfold(Petrinet * llnet)
{
    TruePredicate *p = new TruePredicate(llnet);
    p->parent = NULL;
    p->position = position;
    p->magicnumber = magicnumber;
    return p;
}

bool TruePredicate::containsFireable()
{
	return false;
}

bool TruePredicate::containsUnfireable()
{
	return false;
}

StatePredicate * TruePredicate::hlunfold()
{
	net = Petrinet::InitialNet;
	return this;
}


StatePredicate * TruePredicate::foldformula(Create_HL_Net *)
{
	return this;
}
