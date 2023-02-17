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

%{ KC_UNPARSE
// LCOV_EXCL_START
/*!
\brief printer-function for Kimiwtu's output on stdout

To use an unparser in Kimwitu++, a function with this signature needs to be
passed to the unparse() function.

\param[in] s  string to print

\note This function is not "found" by the code coverage detection and is hence
hidden from it.
*/
void myprinter(const char *s, kc::uview)
{
    printf("%s", s);
}
// LCOV_EXCL_STOP

/// result string for stringprinter
std::string unparsed;

// LCOV_EXCL_START
/*!
\brief printer-function for Kimiwtu's output to a string object

To use an unparser in Kimwitu++, a function with this signature needs to be
passed to the unparse() function.

\param[in] s  string to add

\note This function is not "found" by the code coverage detection and is hence
hidden from it.
\post Output is written to string unparsed which can be accessed wherever an
unparser can be used. You need to clear the string after use to avoid keeping
old data.
*/
void stringprinter(const char *s, kc::uview)
{
    unparsed += s;
}
// LCOV_EXCL_STOP

%}

%{ KC_UNPARSE_HEADER
#include <CoverGraph/CoverGraph.h>
#include <Formula/StatePredicate/DeadlockPredicate.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <string>
#include <Portfolio/portfoliomanager.h>
#include <Exploration/Constraints.h>

extern std::string unparsed;

void myprinter(const char *s, kc::uview v);
void stringprinter(const char *s, kc::uview v);
%}



//////////////////////////////////////////////////////////////////////////
%uview out;

StatePredicateFormula(x) -> [out: x];
c=CompBound() -> [out:  "MAX(" {kc_printer(((AtomicStatePredicate *)(c->formula))->toCompString(),kc_current_view);} ")"];

a=AtomicProposition(x=Elementary()) -> [out:   {kc_printer(x->pred->toString(),kc_current_view);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [out: {StatePredicate * t=x->pred->copy(NULL)->negate();kc_printer(t->toString(),kc_current_view);delete t;}];
AtomicProposition(True()) -> [out: " TRUE "];
AtomicProposition(False()) -> [out: " FALSE "];
AtomicProposition(Deadlock()) -> [out: " DEADLOCK "];
AtomicProposition(NoDeadlock()) -> [out: " (NOT DEADLOCK) "];
Negation(x) -> [out:  "NOT(" x ")"];
Conjunction(x, y) -> [out:  "(" x " AND " y ")"];
Disjunction(x, y) -> [out:   "(" x " OR " y ")" ];
ExclusiveDisjunction(x, y) -> [out: "(" x " XOR " y ")"];
Implication(x, y) -> [out: "(" x " -> " y ")"];
Equivalence(x, y) -> [out: "(" x " <-> " y ")"];
AllPath(x) -> [out: "A (" x ")"];
ExPath(x)  -> [out: "E (" x ")"];
Always(x)     -> [out: "G ("  x  ")"];
Eventually(x) -> [out: "F (" x ")"];
NextState(x) -> [out: "X (" x ")"];
Until(phi,psi) -> [out: "(" phi " U " psi ")"];
Release(phi,psi) -> [out: "(" phi " R " psi ")"];
EU(x,y) -> [out: "E(" x " U " y ")"];
AR(x,y) -> [out: "A(" x " R " y ")"];
ER(x,y) -> [out: "E(" x " R " y ")"];
AU(x,y) -> [out: "A(" x " U " y ")"];
AF(x)   -> [out: "AF(" x ")"];
AG(x)   -> [out: "AG("  x  ")"];
AX(x)   -> [out: "AX(" x ")"];
EX(x)   -> [out: "EX(" x ")"];
EF(x)   -> [out: "EF(" x ")"];
EG(x)   -> [out: "EG(" x ")"];

ConjunctionList(l) -> [out: "(" l ")"];
ConstConjunction_list(h, NiltConjunction_list()) -> [out: h];
ConstConjunction_list(h, l) -> [out: h " AND " l];

DisjunctionList(l) -> [out: "(" l ")"];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [out: h];
ConstDisjunction_list(h, l) -> [out: h " OR " l];


True() -> [out: "TRUE"];
False() -> [out: "FALSE"];
Deadlock() -> [out: "DEADLOCK"];
NoDeadlock() -> [out: "NOT DEADLOCK"];

BuechiAutomaton(trans,accept) -> [out: trans "\n" accept];
BuechiNull() -> [out:];

EmptyBuechiRules() -> [out:];
BuechiRule(i,t) -> [out: "STATE" i t];
ExpandedBuechiRule(from, formula, to) -> [out: from "(" formula ") -> " to "\n"];
BuechiRules(a,b) -> [out: a "&" b];

EmptyTransitionRules() -> [out:];
TransitionRule(formu,i) -> [out: formu "->" i];
TransitionRules(a,b) -> [out: a "&" b];

EmptyAcceptingSet() -> [out:];
AcceptingState(*) -> [out:];
AcceptingSet(*,*) -> [out:];


//////////////////////////////////////////////////////////////////////////

%uview relabelformula;

StatePredicateFormula(x) -> [relabelformula: x];
c=CompBound() -> [relabelformula:  {((AtomicStatePredicate *)(c->formula))->relabel();} ];

a=AtomicProposition(x=Elementary()) -> [relabelformula:  {x->pred->relabel();}];
a=AtomicProposition(NNegation(x=Elementary())) -> [relabelformula: {x->pred->relabel();}];
Negation(x) -> [relabelformula:  x ];
Conjunction(x, y) -> [relabelformula:  x y ];
Disjunction(x, y) -> [relabelformula:   x  y  ];
ExclusiveDisjunction(x, y) -> [relabelformula:  x y ];
Implication(x, y) -> [relabelformula: x  y ];
Equivalence(x, y) -> [relabelformula:  x  y ];
AllPath(x) -> [relabelformula: x ];
ExPath(x)  -> [relabelformula: x ];
Always(x)     -> [relabelformula:  x ];
Eventually(x) -> [relabelformula: x];
NextState(x) -> [relabelformula:  x ];
Until(phi,psi) -> [relabelformula:  phi psi ];
Release(phi,psi) -> [relabelformula: phi  psi ];
EU(x,y) -> [relabelformula:  x  y ];
AR(x,y) -> [relabelformula:  x  y ];
ER(x,y) -> [relabelformula:  x  y];
AU(x,y) -> [relabelformula: x  y ];
AF(x)   -> [relabelformula:  x ];
AG(x)   -> [relabelformula:  x ];
AX(x)   -> [relabelformula:  x ];
EX(x)   -> [relabelformula:  x ];
EF(x)   -> [relabelformula:  x ];
EG(x)   -> [relabelformula:  x ];

ConjunctionList(l) -> [relabelformula: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [relabelformula: h];
ConstConjunction_list(h, l) -> [relabelformula: h  l];

DisjunctionList(l) -> [relabelformula:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [relabelformula: h];
ConstDisjunction_list(h, l) -> [relabelformula: h  l];


True() -> [relabelformula: ];
False() -> [relabelformula: ];
Deadlock() -> [relabelformula: ];
NoDeadlock() -> [relabelformula:];

BuechiAutomaton(trans,accept) -> [relabelformula: trans  accept];
BuechiNull() -> [relabelformula:];

EmptyBuechiRules() -> [relabelformula:];
BuechiRule(i,t) -> [relabelformula:  i t];
ExpandedBuechiRule(from, formula, to) -> [relabelformula: from formula  to ];
BuechiRules(a,b) -> [relabelformula: a  b];

EmptyTransitionRules() -> [relabelformula:];
TransitionRule(formu,i) -> [relabelformula: formu  i];
TransitionRules(a,b) -> [relabelformula: a  b];

EmptyAcceptingSet() -> [relabelformula:];
AcceptingState(*) -> [relabelformula:];
AcceptingSet(*,*) -> [relabelformula:];

//////////////////////////////////////////////////////////////////////////

%uview checkcons;

StatePredicateFormula(x) -> [checkcons: x];
c=CompBound() -> [checkcons:  {((AtomicStatePredicate *)(c->formula))->checkCons();} ];

a=AtomicProposition(x=Elementary()) -> [checkcons:  {x->pred->checkCons();}];
a=AtomicProposition(NNegation(x=Elementary())) -> [checkcons: {x->pred->checkCons();}];
Negation(x) -> [checkcons:  x ];
Conjunction(x, y) -> [checkcons:  x y ];
Disjunction(x, y) -> [checkcons:   x  y  ];
ExclusiveDisjunction(x, y) -> [checkcons:  x y ];
Implication(x, y) -> [checkcons: x  y ];
Equivalence(x, y) -> [checkcons:  x  y ];
AllPath(x) -> [checkcons: x ];
ExPath(x)  -> [checkcons: x ];
Always(x)     -> [checkcons:  x ];
Eventually(x) -> [checkcons: x];
NextState(x) -> [checkcons:  x ];
Until(phi,psi) -> [checkcons:  phi psi ];
Release(phi,psi) -> [checkcons: phi  psi ];
EU(x,y) -> [checkcons:  x  y ];
AR(x,y) -> [checkcons:  x  y ];
ER(x,y) -> [checkcons:  x  y];
AU(x,y) -> [checkcons: x  y ];
AF(x)   -> [checkcons:  x ];
AG(x)   -> [checkcons:  x ];
AX(x)   -> [checkcons:  x ];
EX(x)   -> [checkcons:  x ];
EF(x)   -> [checkcons:  x ];
EG(x)   -> [checkcons:  x ];

ConjunctionList(l) -> [checkcons: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [checkcons: h];
ConstConjunction_list(h, l) -> [checkcons: h  l];

DisjunctionList(l) -> [checkcons:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [checkcons: h];
ConstDisjunction_list(h, l) -> [checkcons: h  l];


True() -> [checkcons: ];
False() -> [checkcons: ];
Deadlock() -> [checkcons: ];
NoDeadlock() -> [checkcons: ];

BuechiAutomaton(trans,accept) -> [checkcons: trans  accept];
BuechiNull() -> [checkcons:];

EmptyBuechiRules() -> [checkcons:];
BuechiRule(i,t) -> [checkcons:  i t];
ExpandedBuechiRule(from, formula, to) -> [checkcons: from formula  to ];
BuechiRules(a,b) -> [checkcons: a  b];

EmptyTransitionRules() -> [checkcons:];
TransitionRule(formu,i) -> [checkcons: formu  i];
TransitionRules(a,b) -> [checkcons: a  b];

EmptyAcceptingSet() -> [checkcons:];
AcceptingState(*) -> [checkcons:];
AcceptingSet(*,*) -> [checkcons:];

//////////////////////////////////////////////////////////////////////////

%uview checkstableatomic;

StatePredicateFormula(x) -> [checkstableatomic: x];
c=CompBound() -> [checkstableatomic:  ];

a=AtomicProposition(x=Elementary()) -> [checkstableatomic:  {x->pred->checkstable(true,NULL);
							     x -> pred->checkstable(false,NULL);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [checkstableatomic: {x->pred->checkstable(true,NULL);
							x->pred->checkstable(false,NULL);}];
Negation(x) -> [checkstableatomic:  x ];
Conjunction(x, y) -> [checkstableatomic:  x y ];
Disjunction(x, y) -> [checkstableatomic:   x  y  ];
ExclusiveDisjunction(x, y) -> [checkstableatomic:  x y ];
Implication(x, y) -> [checkstableatomic: x  y ];
Equivalence(x, y) -> [checkstableatomic:  x  y ];
AllPath(x) -> [checkstableatomic: x ];
ExPath(x)  -> [checkstableatomic: x ];
Always(x)     -> [checkstableatomic:  x ];
Eventually(x) -> [checkstableatomic: x];
NextState(x) -> [checkstableatomic:  x ];
Until(phi,psi) -> [checkstableatomic:  phi psi ];
Release(phi,psi) -> [checkstableatomic: phi  psi ];
EU(x,y) -> [checkstableatomic:  x  y ];
AR(x,y) -> [checkstableatomic:  x  y ];
ER(x,y) -> [checkstableatomic:  x  y];
AU(x,y) -> [checkstableatomic: x  y ];
AF(x)   -> [checkstableatomic:  x ];
AG(x)   -> [checkstableatomic:  x ];
AX(x)   -> [checkstableatomic:  x ];
EX(x)   -> [checkstableatomic:  x ];
EF(x)   -> [checkstableatomic:  x ];
EG(x)   -> [checkstableatomic:  x ];

ConjunctionList(l) -> [checkstableatomic: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [checkstableatomic: h];
ConstConjunction_list(h, l) -> [checkstableatomic: h  l];

DisjunctionList(l) -> [checkstableatomic:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [checkstableatomic: h];
ConstDisjunction_list(h, l) -> [checkstableatomic: h  l];


True() -> [checkstableatomic: ];
False() -> [checkstableatomic: ];
Deadlock() -> [checkstableatomic: ];
NoDeadlock() -> [checkstableatomic: ];

BuechiAutomaton(trans,accept) -> [checkstableatomic: trans  accept];
BuechiNull() -> [checkstableatomic:];

EmptyBuechiRules() -> [checkstableatomic:];
BuechiRule(i,t) -> [checkstableatomic:  i t];
ExpandedBuechiRule(from, formula, to) -> [checkstableatomic: from formula  to ];
BuechiRules(a,b) -> [checkstableatomic: a  b];

EmptyTransitionRules() -> [checkstableatomic:];
TransitionRule(formu,i) -> [checkstableatomic: formu  i];
TransitionRules(a,b) -> [checkstableatomic: a  b];

EmptyAcceptingSet() -> [checkstableatomic:];
AcceptingState(*) -> [checkstableatomic:];
AcceptingSet(*,*) -> [checkstableatomic:];

//////////////////////////////////////////////////////////////////////////

%uview foldformula;

StatePredicateFormula(x) -> [foldformula: x];
c=CompBound() -> [foldformula:  {c->formula = ((AtomicStatePredicate *)(c->formula))->foldformula(Create_HL_Net::current);} ];

a=AtomicProposition(x=Elementary()) -> [foldformula:  {x->pred=x->pred->foldformula(Create_HL_Net::current); }];
a=AtomicProposition(NNegation(x=Elementary())) -> [foldformula: {x->pred=x->pred->foldformula(Create_HL_Net::current);}];
AtomicProposition(*) -> [foldformula: ];
Negation(x) -> [foldformula:  x ];
Conjunction(x, y) -> [foldformula:  x y ];
Disjunction(x, y) -> [foldformula:   x  y  ];
ExclusiveDisjunction(x, y) -> [foldformula:  x y ];
Implication(x, y) -> [foldformula: x  y ];
Equivalence(x, y) -> [foldformula:  x  y ];
AllPath(x) -> [foldformula: x ];
ExPath(x)  -> [foldformula: x ];
Always(x)     -> [foldformula:  x ];
Eventually(x) -> [foldformula: x];
NextState(x) -> [foldformula:  x ];
Until(phi,psi) -> [foldformula:  phi psi ];
Release(phi,psi) -> [foldformula: phi  psi ];
EU(x,y) -> [foldformula:  x  y ];
AR(x,y) -> [foldformula:  x  y ];
ER(x,y) -> [foldformula:  x  y];
AU(x,y) -> [foldformula: x  y ];
AF(x)   -> [foldformula:  x ];
AG(x)   -> [foldformula:  x ];
AX(x)   -> [foldformula:  x ];
EX(x)   -> [foldformula:  x ];
EF(x)   -> [foldformula:  x ];
EG(x)   -> [foldformula:  x ];

ConjunctionList(l) -> [foldformula: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [foldformula: h];
ConstConjunction_list(h, l) -> [foldformula: h  l];

DisjunctionList(l) -> [foldformula:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [foldformula: h];
ConstDisjunction_list(h, l) -> [foldformula: h  l];


True() -> [foldformula: ];
False() -> [foldformula: ];
Deadlock() -> [foldformula: ];
NoDeadlock() -> [foldformula: ];

//////////////////////////////////////////////////////////////////////////
%uview createskeleton;

StatePredicateFormula(x) -> [createskeleton: x];
c=CompBound() -> [createskeleton:  {
				((AtomicStatePredicate *)(c->formula))->createskeletonatomic(Create_HL_Net::current);
				} ];

a=AtomicProposition(x=Elementary()) -> [createskeleton:  {
						x->pred->createskeletonatomic(Create_HL_Net::current);
						x->pred->createskeletonfireable(Create_HL_Net::current,true,false);
						x->pred->createskeletonunfireable(Create_HL_Net::current,true,false);
					}];
a=AtomicProposition(NNegation(x=Elementary())) -> [createskeleton: {
							x->pred->createskeletonatomic(Create_HL_Net::current);
							x->pred->createskeletonfireable(Create_HL_Net::current,true,false);
							x->pred->createskeletonunfireable(Create_HL_Net::current,true,false);
						}];
AtomicProposition(x) -> [createskeleton:  ];
Negation(x) -> [createskeleton:  x ];
Conjunction(x, y) -> [createskeleton:  x y ];
Disjunction(x, y) -> [createskeleton:   x  y  ];
ExclusiveDisjunction(x, y) -> [createskeleton:  x y ];
Implication(x, y) -> [createskeleton: x  y ];
Equivalence(x, y) -> [createskeleton:  x  y ];
AllPath(x) -> [createskeleton: x ];
ExPath(x)  -> [createskeleton: x ];
Always(x)     -> [createskeleton:  x ];
Eventually(x) -> [createskeleton: x];
NextState(x) -> [createskeleton:  x ];
Until(phi,psi) -> [createskeleton:  phi psi ];
Release(phi,psi) -> [createskeleton: phi  psi ];
EU(x,y) -> [createskeleton:  x  y ];
AR(x,y) -> [createskeleton:  x  y ];
ER(x,y) -> [createskeleton:  x  y];
AU(x,y) -> [createskeleton: x  y ];
AF(x)   -> [createskeleton:  x ];
AG(x)   -> [createskeleton:  x ];
AX(x)   -> [createskeleton:  x ];
EX(x)   -> [createskeleton:  x ];
EF(x)   -> [createskeleton:  x ];
EG(x)   -> [createskeleton:  x ];

ConjunctionList(l) -> [createskeleton: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [createskeleton: h];
ConstConjunction_list(h, l) -> [createskeleton: h  l];

DisjunctionList(l) -> [createskeleton:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [createskeleton: h];
ConstDisjunction_list(h, l) -> [createskeleton: h  l];


True() -> [createskeleton: ];
False() -> [createskeleton: ];
Deadlock() -> [createskeleton: ];
NoDeadlock() -> [createskeleton: ];

BuechiAutomaton(trans,accept) -> [createskeleton: trans  accept];
BuechiNull() -> [createskeleton:];

EmptyBuechiRules() -> [createskeleton:];
BuechiRule(i,t) -> [createskeleton:  i t];
ExpandedBuechiRule(from, formula, to) -> [createskeleton: from formula  to ];
BuechiRules(a,b) -> [createskeleton: a  b];

EmptyTransitionRules() -> [createskeleton:];
TransitionRule(formu,i) -> [createskeleton: formu  i];
TransitionRules(a,b) -> [createskeleton: a  b];

EmptyAcceptingSet() -> [createskeleton:];
AcceptingState(*) -> [createskeleton:];
AcceptingSet(*,*) -> [createskeleton:];


//////////////////////////////////////////////////////////////////////////

%uview hlunfold;

StatePredicateFormula(x) -> [hlunfold: x];
c=CompBound() -> [hlunfold:  {c -> formula = ((AtomicStatePredicate *)(c->formula))->hlunfold();} ];

a=AtomicProposition(x=Elementary()) -> [hlunfold:  {x -> pred = x->pred->hlunfold();}];
a=AtomicProposition(NNegation(x=Elementary())) -> [hlunfold: {x -> pred = x->pred->hlunfold();}];
AtomicProposition(x) -> [hlunfold: ];
Negation(x) -> [hlunfold:  x ];
Conjunction(x, y) -> [hlunfold:  x y ];
Disjunction(x, y) -> [hlunfold:   x  y  ];
ExclusiveDisjunction(x, y) -> [hlunfold:  x y ];
Implication(x, y) -> [hlunfold: x  y ];
Equivalence(x, y) -> [hlunfold:  x  y ];
AllPath(x) -> [hlunfold: x ];
ExPath(x)  -> [hlunfold: x ];
Always(x)     -> [hlunfold:  x ];
Eventually(x) -> [hlunfold: x];
NextState(x) -> [hlunfold:  x ];
Until(phi,psi) -> [hlunfold:  phi psi ];
Release(phi,psi) -> [hlunfold: phi  psi ];
EU(x,y) -> [hlunfold:  x  y ];
AR(x,y) -> [hlunfold:  x  y ];
ER(x,y) -> [hlunfold:  x  y];
AU(x,y) -> [hlunfold: x  y ];
AF(x)   -> [hlunfold:  x ];
AG(x)   -> [hlunfold:  x ];
AX(x)   -> [hlunfold:  x ];
EX(x)   -> [hlunfold:  x ];
EF(x)   -> [hlunfold:  x ];
EG(x)   -> [hlunfold:  x ];

ConjunctionList(l) -> [hlunfold: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [hlunfold: h];
ConstConjunction_list(h, l) -> [hlunfold: h  l];

DisjunctionList(l) -> [hlunfold:  l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [hlunfold: h];
ConstDisjunction_list(h, l) -> [hlunfold: h  l];


True() -> [hlunfold: ];
False() -> [hlunfold: ];
Deadlock() -> [hlunfold: ];
NoDeadlock() -> [hlunfold: ];

BuechiAutomaton(trans,accept) -> [hlunfold: trans  accept];
BuechiNull() -> [hlunfold:];

EmptyBuechiRules() -> [hlunfold:];
BuechiRule(i,t) -> [hlunfold:  i t];
ExpandedBuechiRule(from, formula, to) -> [hlunfold: from formula  to ];
BuechiRules(a,b) -> [hlunfold: a  b];

EmptyTransitionRules() -> [hlunfold:];
TransitionRule(formu,i) -> [hlunfold: formu  i];
TransitionRules(a,b) -> [hlunfold: a  b];

EmptyAcceptingSet() -> [hlunfold:];
AcceptingState(*) -> [hlunfold:];
AcceptingSet(*,*) -> [hlunfold:];


//////////////////////////////////////////////////////////////////////////
%uview reduction;
// check applicability of reduction rules

StatePredicateFormula(x) -> [reduction: x { containsDeadlock = x -> containsDeadlock; containsNext = x -> containsNext; } ];
c=CompBound() -> [reduction: {containsNext = containsDeadlock = false;} ];

a=AtomicProposition(x=Elementary()) -> [reduction:  {containsNext = false; containsDeadlock = x -> pred -> countDeadlock(); }];
a=AtomicProposition(NNegation(x)) -> [reduction: x {;containsDeadlock = x -> containsDeadlock; containsNext = false;}];
AtomicProposition(x) -> [reduction: x {containsDeadlock = x -> containsDeadlock; containsNext = false;}];
Negation(x) -> [reduction:  x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
Conjunction(x, y) -> [reduction:  x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
Disjunction(x, y) -> [reduction:  x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);} ];
ExclusiveDisjunction(x, y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
Implication(x, y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
Equivalence(x, y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock); }];
AllPath(x) -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
ExPath(x)  -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
Always(x)     -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock; }];
Eventually(x) -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock; }];
NextState(x) -> [reduction:  x {containsNext = true; containsDeadlock = x -> containsDeadlock;} ];
Until(phi,psi) -> [reduction: phi psi {containsNext=(phi->containsNext || psi->containsNext); 
                                       containsDeadlock = (phi -> containsDeadlock || psi -> containsDeadlock);}];
Release(phi,psi) -> [reduction: phi psi {containsNext=(phi->containsNext || psi->containsNext); 
                                       containsDeadlock = (phi -> containsDeadlock || psi -> containsDeadlock);}];
EU(x,y) -> [reduction: x  y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
AR(x,y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
ER(x,y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
AU(x,y) -> [reduction: x y {containsNext = (x->containsNext || y->containsNext); 
                                       containsDeadlock = (x -> containsDeadlock || y -> containsDeadlock);}];
AF(x)   -> [reduction: x  {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
AG(x)   -> [reduction: x  {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
AX(x)   -> [reduction: x {containsNext = true; containsDeadlock = x -> containsDeadlock;}];
EX(x)   -> [reduction: x {containsNext = true; containsDeadlock = x -> containsDeadlock;}];
EF(x)   -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];
EG(x)   -> [reduction: x {containsNext = x -> containsNext; containsDeadlock = x -> containsDeadlock;}];

ConjunctionList(l) -> [reduction: l {containsNext = l->containsNext; containsDeadlock=l->containsDeadlock;}];
ConstConjunction_list(h, NiltConjunction_list()) -> [reduction: h { containsNext = h -> containsNext; containsDeadlock = h -> containsDeadlock;}];
ConstConjunction_list(h, l) -> [reduction: h l {containsNext = (l -> containsNext||h -> containsNext);
                                                 containsDeadlock = (l ->containsDeadlock || h -> containsDeadlock);}];

DisjunctionList(l) -> [reduction: l {containsNext = l->containsNext; containsDeadlock=l->containsDeadlock;}];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [reduction: h { containsNext = h -> containsNext; containsDeadlock = h -> containsDeadlock;}];
ConstDisjunction_list(h, l) -> [reduction: h  l {containsNext = (l -> containsNext||h -> containsNext);
                                                 containsDeadlock = (l ->containsDeadlock || h -> containsDeadlock);}];


True() -> [reduction: {containsNext = containsDeadlock = false;}];
False() -> [reduction: {containsNext = containsDeadlock = false;}];
Deadlock() -> [reduction: {containsNext = false; containsDeadlock = true;}];
NoDeadlock() -> [reduction: {containsNext = false; containsDeadlock = true;}];

//BuechiAutomaton(trans,accept) -> [out: trans "\n" accept];
//BuechiNull() -> [out:];

//EmptyBuechiRules() -> [out:];
//BuechiRule(i,t) -> [out: "STATE" i t];
//ExpandedBuechiRule(from, formula, to) -> [out: from "(" formula ") -> " to "\n"];
//BuechiRules(a,b) -> [out: a "&" b];

//EmptyTransitionRules() -> [out:];
//TransitionRule(formu,i) -> [out: formu "->" i];
//TransitionRules(a,b) -> [out: a "&" b];

//EmptyAcceptingSet() -> [out:];
//AcceptingState(*) -> [out:];
//AcceptingSet(*,*) -> [out:];


//////////////////////////////////////////////////////////////////////////
%uview markvisible;

StatePredicateFormula(x) -> [markvisible:   x ];
c=CompBound() -> [markvisible:  { ((AtomicStatePredicate *)(c->formula))->markvisible();}];

a=AtomicProposition(x=Elementary()) -> [markvisible:  { x->pred->markvisible();}];
a=AtomicProposition(NNegation(x=Elementary())) -> [markvisible: {x->pred->markvisible();}];
AtomicProposition(x) -> [markvisible: ];
Negation(x) -> [markvisible:  x];
Conjunction(x, y) -> [markvisible:  x y];
Disjunction(x, y) -> [markvisible:   x  y  ];
ExclusiveDisjunction(x, y) -> [markvisible:  x y ];
Implication(x, y) -> [markvisible: x  y ];
Equivalence(x, y) -> [markvisible: x  y ];
AllPath(x) -> [markvisible:  x];
ExPath(x)  -> [markvisible:  x];
Always(x)     -> [markvisible: x ];
Eventually(x) -> [markvisible: x ];
NextState(x) -> [markvisible: x];
Until(phi,psi) -> [markvisible:  phi  psi];
Release(phi,psi) -> [markvisible: phi  psi ];
EU(x,y) -> [markvisible: x  y ];
AR(x,y) -> [markvisible: x  y ];
ER(x,y) -> [markvisible: x y];
AU(x,y) -> [markvisible:  x  y ];
AF(x)   -> [markvisible:  x];
AG(x)   -> [markvisible:  x ];
AX(x)   -> [markvisible:  x];
EX(x)   -> [markvisible:  x ];
EF(x)   -> [markvisible:  x ];
EG(x)   -> [markvisible:  x ];

ConjunctionList(l) -> [markvisible: l ];
ConstConjunction_list(h, NiltConjunction_list()) -> [markvisible: h];
ConstConjunction_list(h, l) -> [markvisible: h l];

DisjunctionList(l) -> [markvisible: l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [markvisible: h];
ConstDisjunction_list(h, l) -> [markvisible: h l];


True() -> [markvisible: ];
False() -> [markvisible: ];
Deadlock() -> [markvisible: ];
NoDeadlock() -> [markvisible: ];

BuechiAutomaton(trans,accept) -> [markvisible: trans  accept];
BuechiNull() -> [markvisible:];

EmptyBuechiRules() -> [markvisible:];
BuechiRule(i,t) -> [markvisible:  i t];
ExpandedBuechiRule(from, formula, to) -> [markvisible: formula ];
BuechiRules(a,b) -> [markvisible: a b];

EmptyTransitionRules() -> [markvisible:];
TransitionRule(formu,i) -> [markvisible: formu i];
TransitionRules(a,b) -> [markvisible: a b];

EmptyAcceptingSet() -> [markvisible:];
AcceptingState(*) -> [markvisible:];
AcceptingSet(*,*) -> [markvisible:];

//////////////////////////////////////////////////////////////////////////
%uview markslicingef;

StatePredicateFormula(x) -> [markslicingef:   x ];
c=CompBound() -> [markslicingef:  { ((AtomicStatePredicate *)(c->formula))->markslicing(false);}];

a=AtomicProposition(x=Elementary()) -> [markslicingef:  { x->pred->markslicing(false);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [markslicingef: {x->pred->markslicing(true);}];
AtomicProposition(x) -> [markslicingef: ];
Negation(x) -> [markslicingef:  x];
Conjunction(x, y) -> [markslicingef:  x y];
Disjunction(x, y) -> [markslicingef:   x  y  ];
ExclusiveDisjunction(x, y) -> [markslicingef:  x y ];
Implication(x, y) -> [markslicingef: x  y ];
Equivalence(x, y) -> [markslicingef: x  y ];
AllPath(x) -> [markslicingef:  x];
ExPath(x)  -> [markslicingef:  x];
Always(x)     -> [markslicingef: x ];
Eventually(x) -> [markslicingef: x ];
NextState(x) -> [markslicingef: x];
Until(phi,psi) -> [markslicingef:  phi  psi];
Release(phi,psi) -> [markslicingef: phi  psi ];
EU(x,y) -> [markslicingef: x  y ];
AR(x,y) -> [markslicingef: x  y ];
ER(x,y) -> [markslicingef: x y];
AU(x,y) -> [markslicingef:  x  y ];
AF(x)   -> [markslicingef:  x];
AG(x)   -> [markslicingef:  x ];
AX(x)   -> [markslicingef:  x];
EX(x)   -> [markslicingef:  x ];
EF(x)   -> [markslicingef:  x ];
EG(x)   -> [markslicingef:  x ];

ConjunctionList(l) -> [markslicingef: l ];
ConstConjunction_list(h, NiltConjunction_list()) -> [markslicingef: h];
ConstConjunction_list(h, l) -> [markslicingef: h l];

DisjunctionList(l) -> [markslicingef: l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [markslicingef: h];
ConstDisjunction_list(h, l) -> [markslicingef: h l];


True() -> [markslicingef: ];
False() -> [markslicingef: ];
Deadlock() -> [markslicingef: ];
NoDeadlock() -> [markslicingef: ];

BuechiAutomaton(trans,accept) -> [markslicingef: trans  accept];
BuechiNull() -> [markslicingef:];

EmptyBuechiRules() -> [markslicingef:];
BuechiRule(i,t) -> [markslicingef:  i t];
ExpandedBuechiRule(from, formula, to) -> [markslicingef: formula ];
BuechiRules(a,b) -> [markslicingef: a b];

EmptyTransitionRules() -> [markslicingef:];
TransitionRule(formu,i) -> [markslicingef: formu i];
TransitionRules(a,b) -> [markslicingef: a b];

EmptyAcceptingSet() -> [markslicingef:];
AcceptingState(*) -> [markslicingef:];
AcceptingSet(*,*) -> [markslicingef:];

//////////////////////////////////////////////////////////////////////////
%uview markslicingag;

StatePredicateFormula(x) -> [markslicingag:   x ];
c=CompBound() -> [markslicingag:  { ((AtomicStatePredicate *)(c->formula))->markslicing(true);}];

a=AtomicProposition(x=Elementary()) -> [markslicingag:  { x->pred->markslicing(true);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [markslicingag: {x->pred->markslicing(false);}];
AtomicProposition(x) -> [markslicingag: ];
Negation(x) -> [markslicingag:  x];
Conjunction(x, y) -> [markslicingag:  x y];
Disjunction(x, y) -> [markslicingag:   x  y  ];
ExclusiveDisjunction(x, y) -> [markslicingag:  x y ];
Implication(x, y) -> [markslicingag: x  y ];
Equivalence(x, y) -> [markslicingag: x  y ];
AllPath(x) -> [markslicingag:  x];
ExPath(x)  -> [markslicingag:  x];
Always(x)     -> [markslicingag: x ];
Eventually(x) -> [markslicingag: x ];
NextState(x) -> [markslicingag: x];
Until(phi,psi) -> [markslicingag:  phi  psi];
Release(phi,psi) -> [markslicingag: phi  psi ];
EU(x,y) -> [markslicingag: x  y ];
AR(x,y) -> [markslicingag: x  y ];
ER(x,y) -> [markslicingag: x y];
AU(x,y) -> [markslicingag:  x  y ];
AF(x)   -> [markslicingag:  x];
AG(x)   -> [markslicingag:  x ];
AX(x)   -> [markslicingag:  x];
EX(x)   -> [markslicingag:  x ];
EF(x)   -> [markslicingag:  x ];
EG(x)   -> [markslicingag:  x ];

ConjunctionList(l) -> [markslicingag: l ];
ConstConjunction_list(h, NiltConjunction_list()) -> [markslicingag: h];
ConstConjunction_list(h, l) -> [markslicingag: h l];

DisjunctionList(l) -> [markslicingag: l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [markslicingag: h];
ConstDisjunction_list(h, l) -> [markslicingag: h l];


True() -> [markslicingag: ];
False() -> [markslicingag: ];
Deadlock() -> [markslicingag: ];
NoDeadlock() -> [markslicingag: ];

BuechiAutomaton(trans,accept) -> [markslicingag: trans  accept];
BuechiNull() -> [markslicingag:];

EmptyBuechiRules() -> [markslicingag:];
BuechiRule(i,t) -> [markslicingag:  i t];
ExpandedBuechiRule(from, formula, to) -> [markslicingag: formula ];
BuechiRules(a,b) -> [markslicingag: a b];

EmptyTransitionRules() -> [markslicingag:];
TransitionRule(formu,i) -> [markslicingag: formu i];
TransitionRules(a,b) -> [markslicingag: a b];

EmptyAcceptingSet() -> [markslicingag:];
AcceptingState(*) -> [markslicingag:];
AcceptingSet(*,*) -> [markslicingag:];

//////////////////////////////////////////////////////////////////////////
%uview structural;

StatePredicateFormula(x) -> [structural: x];
CompBound() -> [structural:  "MAX(*)"];

AtomicProposition(Deadlock()) -> [structural: "DEADLOCK"];
AtomicProposition(NoDeadlock()) -> [structural: "NODEADLOCK"];
AtomicProposition(False()) -> [structural: "FALSE"];
AtomicProposition(True()) -> [structural: "TRUE"];
AtomicProposition(Elementary()) -> [structural: "**"];
AtomicProposition(*) -> [structural: "*"];
Negation(x) -> [structural: "NOT(" x ")"];
Conjunction(x, y) -> [structural:  "(" x " AND " y ")"];
Disjunction(x, y) -> [structural:   "(" x " OR " y ")" ];
ExclusiveDisjunction(x, y) -> [structural: "(" x " XOR " y ")"];
Implication(x, y) -> [structural: "(" x " -> " y ")"];
Equivalence(x, y) -> [structural: "(" x " <-> " y ")"];
AllPath(x) -> [structural: "A(" x ")"];
ExPath(x)  -> [structural: "E(" x ")"];
Always(x)     -> [structural: "G("  x  ")"];
Eventually(x) -> [structural: "F(" x ")"];
NextState(x) -> [structural: "X(" x ")"];
Until(phi,psi) -> [structural: "(" phi " U " psi ")"];
Release(phi,psi) -> [structural: "(" phi " R " psi ")"];
EU(x,y) -> [structural: "E(" x " U " y ")"];
AR(x,y) -> [structural: "A(" x " R " y ")"];
ER(x,y) -> [structural: "E(" x " R " y ")"];
AU(x,y) -> [structural: "A(" x " U " y ")"];
AF(x)   -> [structural: "AF(" x ")"];
AG(x)   -> [structural: "AG(" x ")"];
AX(x)   -> [structural: "AX(" x ")"];
EX(x)   -> [structural: "EX(" x ")"];
EF(x)   -> [structural: "EF(" x ")"];
EG(x)   -> [structural: "EG(" x ")"];

ConjunctionList(l) -> [structural: "(" l ")"];
ConstConjunction_list(h, NiltConjunction_list()) -> [structural: h];
ConstConjunction_list(h, l) -> [structural: h " AND " l];

DisjunctionList(l) -> [structural: "(" l ")"];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [structural: h];
ConstDisjunction_list(h, l) -> [structural: h " OR " l];


True() -> [structural: "TRUE"];
False() -> [structural: "FALSE"];
Deadlock() -> [structural: "DEADLOCK"];
NoDeadlock() -> [structural: "NODEADLOCK"];

//////////////////////////////////////////////////////////////////////////
%uview containstemp;

StatePredicateFormula(x) -> [containstemp: {x->toplevel = true;} x {containsNext = x->containsNext; containsTemporal = x -> containsTemporal;}];
Conjunction(x, y),
Disjunction(x, y) -> [containstemp:  {x -> toplevel = y -> toplevel = toplevel;} x y {containsTemporal = x -> containsTemporal || y -> containsTemporal;
                                      containsNext = x -> containsNext || y -> containsNext;
					onlyNext = x -> onlyNext || y -> onlyNext;}];

CompBound() -> [containstemp: {containsTemporal = true; containsNext = false;}];

AtomicProposition(x) -> [containstemp: {containsTemporal = false; 
						 containsNext = false;
						 onlyNext = true;}];
Negation(x) -> [containstemp: {x->toplevel = toplevel;} x { containsTemporal = x -> containsTemporal;
				   containsNext = x -> containsNext;
				   onlyNext = x -> onlyNext;}];
AllPath(x),
ExPath(x) -> [containstemp: {x -> toplevel = false;} x { containsTemporal = true;
                                    containsNext = x -> containsNext;
				onlyNext = x -> onlyNext;}];
AF(x),
AG(x),
EF(x),
EG(x),
Always(x),
Eventually(x) -> [containstemp: {x -> toplevel = false;} x { containsTemporal = true;
				    containsNext = x -> containsNext;
					onlyNext = false;}];
AX(x),
EX(x),
NextState(x) -> [containstemp:  {x->toplevel = false;} x { containsTemporal = true;
				    containsNext = true;
					onlyNext = x -> onlyNext;}];
EU(phi,psi),
AR(phi,psi),
ER(phi,psi),
AU(phi,psi),
Until(phi,psi), 
Release(phi,psi) -> [containstemp: {phi->toplevel = psi->toplevel = false;} phi  psi { containsTemporal = true;
					       containsNext = phi -> containsNext || psi -> containsNext;
						onlyNext = false;}];


//////////////////////////////////////////////////////////////////////////
%uview count;

StatePredicateFormula(x) -> [count: {x->fs = fs;} x {fs = x->fs;}];
c=CompBound() -> [count: {fs=c->formula->count(fs);fs->comp--;}];

a=AtomicProposition(x=Elementary()) -> [count: {fs=x->pred->count(fs);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [count: {fs=x->pred->count(fs);fs->aneg++;}];
AtomicProposition(x) -> [count:  ];
Negation(x) -> [count: {x->fs=fs;} x {fs=x->fs;fs->tneg++;}];
Conjunction(x, y) -> [count:  {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->tconj++;}];
Disjunction(x, y) -> [count:  {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->tdisj++;}];
AllPath(x) -> [count: {x->fs=fs;} x {fs=x->fs;fs->A++;}];
ExPath(x)  -> [count: {x->fs=fs;} x {fs=x->fs;fs->E++;}];
Always(x)     -> [count: {x->fs=fs;}  x  {fs=x->fs;fs->G++;}];
Eventually(x) -> [count: {x->fs=fs;} x {fs=x->fs;fs->F++;}];
NextState(x) -> [count: {x->fs=fs;} x {fs=x->fs;fs->X++;}];
Until(phi,psi) -> [count: {phi->fs=fs;} phi {psi->fs=phi->fs;} psi {fs=psi->fs;fs->U++;}];
Release(phi,psi) -> [count: {phi->fs=fs;} phi {psi->fs=phi->fs;} psi {fs=psi->fs;fs->R++;}];
EU(x,y) -> [count: {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->E++;fs->U++;}];
AR(x,y) -> [count: {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->A++;fs->R++;}];
ER(x,y) -> [count: {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->E++;fs->R++;}];
AU(x,y) -> [count: {x->fs=fs;} x {y->fs=x->fs;} y {fs=y->fs;fs->A++;fs->U++;}];
AF(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->A++;fs->F++;}];
AG(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->A++;fs->G++;}];
AX(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->A++;fs->X++;}];
EX(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->E++;fs->X++;}];
EF(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->E++;fs->F++;}];
EG(x)   -> [count: {x->fs=fs;} x {fs=x->fs;fs->E++;fs->G++;}];

ConjunctionList(l) -> [count: {l->fs=fs;} l {fs=l->fs;fs->tconj++;}];
ConstConjunction_list(h, NiltConjunction_list()) -> [count: {h->fs=fs;} h {fs=h->fs;}];
ConstConjunction_list(h, l) -> [count: {h->fs=fs;} h {l->fs=h->fs;} l {fs =l->fs;}];

DisjunctionList(l) -> [count: {l->fs=fs;} l {fs=l->fs;fs->tdisj++;}];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [count: {h->fs=fs;} h {fs=h->fs;}];
ConstDisjunction_list(h, l) -> [count: {h->fs=fs;} h {l->fs=h->fs;} l {fs=l->fs;}];


True() -> [count: {fs->taut++;}];
False() -> [count: {fs->cont++;}];

//////////////////////////////////////////////////////////////////////////
%uview elem;

StatePredicateFormula(x) -> [elem: x];
c=CompBound() -> [elem: ];

a=AtomicProposition(x=Elementary()) -> [elem:  {x->pred = a -> formula;}];
a=AtomicProposition(NNegation(x=Elementary())) -> [elem:  {x->pred=a->formula;}];
AtomicProposition(x) -> [elem: {x -> pred = NULL;}];
Negation(x) -> [elem:  x];
Conjunction(x, y) -> [elem: x y];
Disjunction(x, y) -> [elem: x y];
ExclusiveDisjunction(x, y) -> [elem: x y];
Implication(x, y) -> [elem: x  y];
Equivalence(x, y) -> [elem: x y];
AllPath(x) -> [elem:  x ];
ExPath(x)  -> [elem:  x];
Always(x)     -> [elem:  x ];
Eventually(x) -> [elem:  x ];
NextState(x) -> [elem: x];
Until(phi,psi) -> [elem: phi  psi];
Release(phi,psi) -> [elem: phi  psi ];
EU(x,y) -> [elem: x  y ];
AR(x,y) -> [elem:  x y ];
ER(x,y) -> [elem: x y ];
AU(x,y) -> [elem: x y ];
AF(x)   -> [elem: x ];
AG(x)   -> [elem: x];
AX(x)   -> [elem: x];
EX(x)   -> [elem: x];
EF(x)   -> [elem: x];
EG(x)   -> [elem:  x];

ConjunctionList(l) -> [elem: l];
ConstConjunction_list(h, NiltConjunction_list()) -> [elem: h];
ConstConjunction_list(h, l) -> [elem: h  l];

DisjunctionList(l) -> [elem: l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [elem: h];
ConstDisjunction_list(h, l) -> [elem: h l];

//////////////////////////////////////////////////////////////////////////
%uview temporal;

// assumption: ExclusiveDisjunction, Implication, and Equivalence are gone.


f=CompBound() -> [temporal: 
    {
	f -> type = FORMULA_BOUND; 
	f -> cannotcompute = false;
	f -> cardcompound = 1;
    }];


/////////////////////////////////////////
//// SPECIAL FORMULAS                  //
/////////////////////////////////////////

// EF DEADLOCK
StatePredicateFormula(ExPath(Eventually(AtomicProposition(Deadlock())))) -> [temporal: 
    { 
	type = FORMULA_DEADLOCK;
	cannotcompute = false;

	containsExistential = true;
	containsDeadlock = true;
 	finitepath = true;
	cardcompound = 1;
    }];

// AG NODEADLOCK
StatePredicateFormula(AllPath(Always(AtomicProposition(NoDeadlock()))))  -> [temporal: 
    { 
	type = FORMULA_NODEADLOCK;
	cannotcompute = false;

	containsUniversal = true;
	containsDeadlock = true;
	finitepath = true;
	cardcompound = 1;
    }];

// AGEFAG
StatePredicateFormula(AllPath(Always(ExPath(Eventually(AllPath(Always(y))))))) provided(!y->containsTemporal) -> [temporal: y
    { 
		type = FORMULA_AGEFAG;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// EFAGEF
StatePredicateFormula(ExPath(Eventually(AllPath(Always(ExPath(Eventually(y))))))) provided(! y->containsTemporal) -> [temporal:  y
    { 
		type = FORMULA_EFAGEF;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// EFAG 
StatePredicateFormula(ExPath(Eventually(AllPath(Always(y))))) provided(! y->containsTemporal) ->[temporal:  y
    { 
		type = FORMULA_EFAG;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// AGEF
StatePredicateFormula(AllPath(Always(ExPath(Eventually(y))))) provided(! y->containsTemporal) -> [temporal:   y
    { 
		type = FORMULA_LIVENESS;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// EF
StatePredicateFormula(ExPath(Eventually(y))) provided(!y->containsTemporal) -> [temporal:  y
    { 
		type = FORMULA_REACHABLE;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// AG
StatePredicateFormula(AllPath(Always(y))) provided(!y->containsTemporal) -> [temporal: y
    { 
		type = FORMULA_INVARIANT;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// AFAG
StatePredicateFormula(AllPath(Eventually(AllPath(Always(y))))) provided(!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_AFAG;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// AGAF
StatePredicateFormula(AllPath(Always(AllPath(Eventually(y))))) provided(!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_AGAF;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// EGEF
StatePredicateFormula(ExPath(Always(ExPath(Eventually(y))))) provided(! y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_EGEF;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// EFEG
StatePredicateFormula(ExPath(Eventually(ExPath(Always(y))))) provided (!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_EFEG;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// EXEF
StatePredicateFormula(f=ExPath(g=NextState(h=ExPath(i=Eventually(y))))) provided(!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_EXEF;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// EXEG
StatePredicateFormula(ExPath(NextState(ExPath(Always(y))))) provided (! y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_EXEG;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable;
		containsUnfireable = y -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// EXEU
StatePredicateFormula(ExPath(NextState(ExPath(Until(x,y))))) provided((!x->containsTemporal) && (!y->containsTemporal)) ->[temporal: x y
    { 
		type = FORMULA_EXEU;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// EXER
StatePredicateFormula(ExPath(NextState(ExPath(Release(x,y))))) provided((!x->containsTemporal) && (!y->containsTemporal)) ->[temporal: x y
    { 
		type = FORMULA_EXER;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// AXAG
StatePredicateFormula(AllPath(NextState(AllPath(Always(y))))) provided(!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_AXAG;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = true;
	cardcompound = 1;
    }];

// AXAF
StatePredicateFormula(AllPath(NextState(AllPath(Eventually(y))))) provided(!y->containsTemporal) ->[temporal: y
    { 
		type = FORMULA_AXAF;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// AXAR
StatePredicateFormula(AllPath(NextState(AllPath(Release(x,y))))) provided((!x->containsTemporal) && (!y->containsTemporal)) ->[temporal: x y
    { 
		type = FORMULA_AXAR;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// AXAU
StatePredicateFormula(AllPath(NextState(AllPath(Until(x,y))))) provided((!x->containsTemporal) && (!y->containsTemporal)) ->[temporal: x y
    { 
		type = FORMULA_AXAU;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// EG
StatePredicateFormula(ExPath(Always(y))) provided(!y->containsTemporal) -> [temporal:  y
    { 
		type = FORMULA_EG;
		cannotcompute = false;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// AF
StatePredicateFormula(AllPath(Eventually(y))) provided(!y->containsTemporal) -> [temporal:  y
    { 
		type = FORMULA_EVENTUALLY;
		cannotcompute = false;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// GF
StatePredicateFormula(Always(Eventually(y))) provided(!y->containsTemporal) -> [temporal: y
    { 
		type = FORMULA_FAIRNESS;
		cannotcompute = false;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// FG
StatePredicateFormula(Eventually(Always(y))) provided(!y->containsTemporal) -> [temporal: y
    { 
		type = FORMULA_STABILIZATION;
		cannotcompute = false;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// F
StatePredicateFormula(Eventually(y)) provided(!y->containsTemporal) -> [temporal:  y
    { 
		type = FORMULA_EVENTUALLY;
		cannotcompute = false;
		containsFireable = y -> containsFireable ;
		containsUnfireable = y -> containsUnfireable ;
		finitepath = false;
	cardcompound = 1;
    }];

// INITIAL
StatePredicateFormula(AtomicProposition(NNegation(y))) -> [temporal: y
    {
        type = FORMULA_INITIAL;
	cannotcompute=false;
	if(y -> pred)
	{
		containsFireable = y -> pred -> containsUnfireable() ;
		containsUnfireable = y ->pred -> containsFireable() ;
	}	
	else
	{
		containsFireable = false ;
		containsUnfireable = false ;
	}
	finitepath = true;
	cardcompound = 1;
    }];

StatePredicateFormula(AtomicProposition(y)) -> [temporal: y
    {
        type = FORMULA_INITIAL;
	cannotcompute=false;
	if(y -> pred)
	{
		containsFireable = y -> pred -> containsFireable() ;
		containsUnfireable = y ->pred -> containsUnfireable() ;
	}	
	else
	{
		containsFireable = false ;
		containsUnfireable = false ;
	}
	finitepath = true;
	cardcompound = 1;
    }];

// EU
StatePredicateFormula(ExPath(Until(x,y))) provided((!x->containsTemporal) && (!y->containsTemporal)) -> [temporal: x y 
    {
	cannotcompute = false;
	type = FORMULA_EU;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

// ER
StatePredicateFormula(ExPath(Release(x,y))) provided((!x->containsTemporal) && (!y->containsTemporal)) -> [temporal: x y
    {
	cannotcompute = false;
	type = FORMULA_ER;
		containsExistential = true;
		containsUniversal = false;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// AU
StatePredicateFormula(AllPath(Until(x,y))) provided((!x->containsTemporal) && (!y->containsTemporal)) -> [temporal: x y
    {
	cannotcompute = false;
	type = FORMULA_AU;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable || x -> containsUnfireable;
		finitepath = false;
	cardcompound = 1;
    }];

// AR
StatePredicateFormula(AllPath(Release(x,y))) provided((!x->containsTemporal) && (!y->containsTemporal)) -> [temporal: x y
    {
	cannotcompute = false;
	type = FORMULA_AR;
		containsExistential = false;
		containsUniversal = true;
		containsFireable = y -> containsFireable || x -> containsFireable;
		containsUnfireable = y -> containsUnfireable ||x -> containsUnfireable;
		finitepath = true;
	cardcompound = 1;
    }];

/////////////////////////////////////////
//// GENERAL FORMULAS                  //
/////////////////////////////////////////

StatePredicateFormula(f=AllPath(x)) -> [temporal: 
	x 
    {
        cannotcompute = x -> cannotcompute || ((! x -> validLTLFormula) && (! x -> validCTLPathFormula));
	cardcompound = 1;
	if(x->validLTLFormula)
	{
		if(x->validCTLPathFormula)
		{
			type = FORMULA_BOTH;
			f->validLTLFormula = true;
			f->validCTLStateFormula = true;
			f->singlepathex = false;
			f->singlepathall = x -> singlepathall;
		}
		else
		{
			type = FORMULA_LTL;
			f->validLTLFormula = true;
			f->validCTLStateFormula = false;
		}
	}
	else
	{
		if(x->validCTLPathFormula)
		{
			type = FORMULA_CTL;
			f->validLTLFormula = false;
			f->validCTLStateFormula = true;
			f->singlepathex = false;
			f->singlepathall = x -> singlepathall;
			if(f->singlepathall) type = FORMULA_SINGLEPATHALL;
		}
		else
		{
			type = FORMULA_MODELCHECKING;
			f->validLTLFormula = false;
			f->validCTLStateFormula = false;
		}
	}

	containsUniversal = true;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x -> finitepathall;
	finitepathall = false;
	finitepathex = false;
    }];

StatePredicateFormula(x) -> [temporal: 
	x 
    {
        cannotcompute = x -> cannotcompute;
	cardcompound = 1;
	if(x->validLTLFormula)
	{
		if(x->validCTLStateFormula)
		{
			type = FORMULA_BOTH;
		}
		else
		{
			type = FORMULA_LTL;
		}
	}
	else
	{
		if(x->validCTLStateFormula)
		{
			type = FORMULA_CTL;
			if(x->singlepathall) type = FORMULA_SINGLEPATHALL;
			else if(x->singlepathex) type = FORMULA_SINGLEPATHEX;
		}
		else
		{
			type = FORMULA_MODELCHECKING;
		}
	}
	if(x->type == FORMULA_INITIAL) type = FORMULA_INITIAL;
	if(x->type == FORMULA_CONJUNCTION) type = FORMULA_CONJUNCTION;
	if(x->type == FORMULA_DISJUNCTION && x -> validCTLStateFormula) type = FORMULA_DISJUNCTION;
	containsUniversal = x-> containsUniversal;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x -> finitepath;
	
    }];

AtomicProposition(x) -> [temporal:  x
    {
        type = FORMULA_INITIAL;
	cannotcompute=false;
	validLTLFormula=true;	
	validCTLStateFormula=true;
	validCTLPathFormula=false;
	singlepathall = true;
	singlepathex = true;
	containsUniversal = false;
	containsExistential = false;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepathex = true;
	finitepathall = true;
	finitepath = true;
    }];

Negation(x) -> [temporal: x 
    {
	if(not x->containsTemporal)
	{
        	type = FORMULA_INITIAL;
		singlepathall = true;
		singlepathex = true;
	}
	else
	{
		// in general, negation should not appear when applying this uview
        	type = FORMULA_MODELCHECKING;
		singlepathex = x -> singlepathall;
		singlepathall = x -> singlepathex;
	}
	cannotcompute=x->cannotcompute;
	if(cannotcompute)
	{
		type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = x->validCTLStateFormula;
    	validCTLPathFormula = false;
    	validLTLFormula = x->validLTLFormula;
	containsUniversal = x -> containsExistential;
	containsExistential = x -> containsUniversal;
	containsFireable = x -> containsUnfireable;	
	containsUnfireable = x -> containsFireable;	
	finitepath = x -> finitepath;
	finitepathex = x -> finitepathall;
	finitepathall = x -> finitepathex;
    }];

NNegation(x) -> [temporal: x 
    {
	containsUniversal = false;
	containsExistential = false;
	containsFireable = x -> containsUnfireable;	
	containsUnfireable = x -> containsFireable;	
    }];

True() -> [temporal:
	{
		containsUniversal = false;
		containsExistential = false;
		containsFireable = false;
		containsUnfireable = false;
	}];

False() -> [temporal:
	{
		containsUniversal = false;
		containsExistential = false;
		containsFireable = false;
		containsUnfireable = false;
	}];

Deadlock() -> [temporal:
	{
		containsUniversal = false;
		containsExistential = false;
		containsFireable = false;
		containsUnfireable = false;
	}];

NoDeadlock() -> [temporal:
	{
		containsUniversal = false;
		containsExistential = false;
		containsFireable = false;
		containsUnfireable = false;
	}];

Elementary() -> [temporal: 
    {
	containsUniversal = false;
	containsExistential = false;
	containsFireable = pred -> containsFireable();
	containsUnfireable = pred -> containsUnfireable();
    }];


Conjunction(x,y) -> [temporal: x y 
    {
	if(not (x->containsTemporal || y->containsTemporal))
	{
        	type = FORMULA_INITIAL;
	}
	else
	{
        	type = FORMULA_CONJUNCTION;
	}
	cannotcompute= x->cannotcompute || y -> cannotcompute;
	if(cannotcompute)
	{
		type = FORMULA_MODELCHECKING;
	}
   	validCTLStateFormula = (x->validCTLStateFormula && y->validCTLStateFormula);
    	validCTLPathFormula = false;
    	validLTLFormula = (x->validLTLFormula && y->validLTLFormula);
	if(!toplevel && !validCTLStateFormula && !validLTLFormula) cannotcompute = true;
	singlepathall = x -> singlepathall && y -> singlepathall;
	singlepathex = (x -> singlepathex && not (y ->containsTemporal)) || (y -> singlepathex && not (x->containsTemporal));
	containsUniversal = x -> containsUniversal || y -> containsUniversal;
	containsExistential = x -> containsExistential || y -> containsExistential;
	containsFireable = x -> containsFireable || y -> containsFireable;	
	containsUnfireable = x -> containsUnfireable || y -> containsUnfireable;	
	finitepath = x -> finitepath && y -> finitepath;
	finitepathex = x -> finitepathex && y -> finitepathex;
	finitepathall = y -> finitepathall && y -> finitepathall;
    }];

Disjunction(x,y) -> [temporal: x y 
    {
	if(not (x->containsTemporal || y->containsTemporal))
	{
        	type = FORMULA_INITIAL;
	}
	else
	{
        	type = FORMULA_DISJUNCTION;
	}
	cannotcompute= x->cannotcompute || y -> cannotcompute;
	if(cannotcompute)
	{
		type = FORMULA_MODELCHECKING;
	}
   	validCTLStateFormula = (x->validCTLStateFormula && y->validCTLStateFormula);
    	validCTLPathFormula = false;
    	validLTLFormula = (x->validLTLFormula && y->validLTLFormula);
	if(validLTLFormula && type == FORMULA_DISJUNCTION) type = FORMULA_LTL;
	if(!toplevel && !validCTLStateFormula && !validLTLFormula) cannotcompute = true;
	singlepathex = x -> singlepathex && y -> singlepathex;
	singlepathall = (x -> singlepathall && not (y ->containsTemporal)) || (y -> singlepathall && not (x->containsTemporal));
	containsUniversal = x -> containsUniversal || y -> containsUniversal;
	containsExistential = x -> containsExistential || y -> containsExistential;
	containsFireable = x -> containsFireable || y -> containsFireable;	
	containsUnfireable = x -> containsUnfireable || y -> containsUnfireable;	
	finitepath = x -> finitepath && y -> finitepath;
	finitepathex = x -> finitepathex && y -> finitepathex;
	finitepathall = y -> finitepathall && y -> finitepathall;
    }];

AllPath(x) -> [temporal: x 
    {
        cannotcompute = (x -> cannotcompute || (!toplevel && !(x->validCTLPathFormula)));
        if(! cannotcompute)
        {
	    type = FORMULA_CTL;
        }
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = x->validCTLPathFormula;
    	validCTLPathFormula = false;
    	validLTLFormula = false;
	singlepathex = false;
	singlepathall = x -> singlepathall;
	if(singlepathall) type = FORMULA_SINGLEPATHALL;
	containsUniversal = true;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x -> finitepathall;
	finitepathall = false;
	finitepathex = false;
    }];

ExPath(x) -> [temporal: x 
    {
        cannotcompute = (x -> cannotcompute || !(x->validCTLPathFormula));
        if(! cannotcompute)
        {
	    type = FORMULA_CTL;
        }
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = x->validCTLPathFormula;
    	validCTLPathFormula = false;
    	validLTLFormula = false;
	singlepathall = false;
	singlepathex = x -> singlepathex;
	if(singlepathex) type = FORMULA_SINGLEPATHEX;
	containsUniversal = x -> containsUniversal;
	containsExistential = true;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x -> finitepathex;
	finitepathall = false;
	finitepathex = false;
    }];

NextState(x) -> [temporal: x 
    {
	cannotcompute = (x -> cannotcompute || not(x->validLTLFormula || x->validCTLStateFormula));
	if((not cannotcompute)&& x->validLTLFormula)
	{
		type = FORMULA_LTL;
	}
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = false;
    	validCTLPathFormula = x->validCTLStateFormula;
    	validLTLFormula = x->validLTLFormula;
	singlepathex = false;
	singlepathall = false;
	containsUniversal = x -> containsUniversal;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x -> finitepath;
	finitepathall = x -> finitepath;
	finitepathex = x -> finitepath;
    }];

Eventually(x) -> [temporal: x 
    {
	cannotcompute = (x -> cannotcompute || not(x->validLTLFormula || x->validCTLStateFormula));
	if((not cannotcompute)&& x->validLTLFormula)
	{
		type = FORMULA_LTL;
	}
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = false;
    	validCTLPathFormula = x->validCTLStateFormula;
    	validLTLFormula = x->validLTLFormula;
	singlepathex = x -> singlepathex;
	singlepathall = not x -> containsTemporal;
	containsUniversal = x -> containsUniversal;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = false;
	finitepathex = x -> finitepath;
	finitepathall = false;
    }];

Always(x) -> [temporal: x 
    {
	cannotcompute = (x -> cannotcompute || not(x->validLTLFormula || x->validCTLStateFormula));
	if((not cannotcompute)&& x->validLTLFormula)
	{
		type = FORMULA_LTL;
	}
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
    	validCTLStateFormula = false;
    	validCTLPathFormula = x->validCTLStateFormula;
    	validLTLFormula = x->validLTLFormula;
	singlepathall = x -> singlepathall;
	singlepathex = not x -> containsTemporal;
	containsUniversal = x -> containsUniversal;
	containsExistential = x -> containsExistential;
	containsFireable = x -> containsFireable;	
	containsUnfireable = x -> containsUnfireable;	
	finitepath = x->finitepath;
	finitepathex = false;
	finitepathall = x -> finitepath;
    }];

Release(x,y) -> [temporal: x y
    {
RT::rep->status("RELEASE");
	validLTLFormula=x->validLTLFormula && y->validLTLFormula;
	validCTLStateFormula = false;
	validCTLPathFormula=x->validCTLStateFormula && y ->validCTLStateFormula;
	cannotcompute = (x->cannotcompute || y->cannotcompute || !(validLTLFormula || validCTLStateFormula));
	if((not cannotcompute)&& x->validLTLFormula)
	{
		type = FORMULA_LTL;
	}
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
	singlepathex = (x -> singlepathex) && (not y -> containsTemporal);
	singlepathall = (y -> singlepathall) && (not x -> containsTemporal);
	containsUniversal = x -> containsUniversal || y -> containsUniversal;
	containsExistential = x -> containsExistential || y -> containsExistential;
	containsFireable = x -> containsFireable || y -> containsFireable;	
	containsUnfireable = x -> containsUnfireable || y -> containsUnfireable;	
	finitepath = x->finitepath;
	finitepathex = false;
	finitepathall = x -> finitepath && y -> finitepath;
    }];

Until(x,y) -> [temporal: x y
    {
	validLTLFormula=x->validLTLFormula && y->validLTLFormula;
	validCTLStateFormula = false;
	validCTLPathFormula=x->validCTLStateFormula && y ->validCTLStateFormula;
	cannotcompute = (x->cannotcompute || y->cannotcompute || !(validLTLFormula || validCTLStateFormula));
	if((not cannotcompute)&& x->validLTLFormula)
	{
		type = FORMULA_LTL;
	}
	else
	{
	   type = FORMULA_MODELCHECKING;
	}
	singlepathall = (x -> singlepathall) && (not y -> containsTemporal);
	singlepathex = (y -> singlepathex) && (not x -> containsTemporal);
	containsUniversal = x -> containsUniversal || y -> containsUniversal;
	containsExistential = x -> containsExistential || y -> containsExistential;
	containsFireable = x -> containsFireable || y -> containsFireable;	
	containsUnfireable = x -> containsUnfireable || y -> containsUnfireable;	
	finitepath = false;
	finitepathex = x -> finitepathex && y -> finitepathex;
	finitepathall = false;
    }];

//////////////////////////////////////////////////////////////////////////
%uview internal;

%{ KC_UNPARSE
#include <config.h>
#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/StatePredicate/FalsePredicate.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Formula/StatePredicate/TruePredicate.h>

#include <map>
#include <vector>
#include <stack>

// mapping from node ids to factors
std::map<int, int> current;

// mapping from tree node ids to list of intermediate formulas
std::map<int, std::vector<StatePredicate *> > formulas;

// invariant: top is always my parent
std::stack<size_t> id_stack;

// next free id
size_t nextId = 0;
%}

f=StatePredicateFormula(x) -> [internal: { 
	// clear all data-structures
	current.clear();
	formulas.clear();
	while (not id_stack.empty())
		id_stack.pop();
	nextId = 0;
    id_stack.push(nextId++);
}
x
{
    nextId = id_stack.top();
    id_stack.pop();
    // return the only entry for the root
    assert(nextId == 0);
    assert(formulas[0].size() == 1);
    f->formula = formulas[0][0];
}];

Conjunction(a,b) -> [internal: {
    id_stack.push(nextId++);
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = id_stack.top();
    id_stack.pop();

    AtomicBooleanPredicate *f = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
    for (size_t i = 0; i < formulas[myId].size(); ++i)
    {
        assert(formulas[myId][i]);
        f->addSub(formulas[myId][i]);
    }

    formulas[id_stack.top()].push_back(f);
}];

ConjunctionList(l) -> [internal: {
    id_stack.push(nextId++);
}
l
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = id_stack.top();
    id_stack.pop();

    AtomicBooleanPredicate *f = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
    
    for (int i = 0; i < l->length(); ++i)
    {
        assert(formulas[myId][i]);
        f->addSub(formulas[myId][i]);
    }

    formulas[id_stack.top()].push_back(f);
}];

Disjunction(a,b) -> [internal: {
    id_stack.push(nextId++);
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t  myId = id_stack.top();
    id_stack.pop();

    AtomicBooleanPredicate *f = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
    for (size_t i = 0; i < formulas[myId].size(); ++i)
    {
        assert(formulas[myId][i]);
        f->addSub(formulas[myId][i]);
    }

    formulas[id_stack.top()].push_back(f);
}];

DisjunctionList(l) -> [internal: {
    id_stack.push(nextId++);
}
l
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = id_stack.top();
    id_stack.pop();

    AtomicBooleanPredicate *f = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
    
    for (int i = 0; i < l->length(); ++i)
    {
        assert(formulas[myId][i]);
        f->addSub(formulas[myId][i]);
    }

    formulas[id_stack.top()].push_back(f);
}];

a=AtomicProposition(NNegation(x=Elementary())) -> [internal: 
{
    formula = x->pred -> copy(NULL);
    formula = formula -> negate();
	
    formulas[id_stack.top()].push_back(formula);    
}];

a=AtomicProposition(x=Elementary()) -> [internal: 
{
    formulas[id_stack.top()].push_back(x->pred->copy(NULL));    
}];

AtomicProposition(x) -> [internal: x];


True() -> [internal: {
    StatePredicate *f = new TruePredicate(Petrinet::InitialNet);
    // register formula at parent
    formulas[id_stack.top()].push_back(f);    
}];

False() -> [internal: {
    StatePredicate *f = new FalsePredicate(Petrinet::InitialNet);
    // register formula at parent
    formulas[id_stack.top()].push_back(f);    
}];

Deadlock() -> [internal: {
    StatePredicate *f = new DeadlockPredicate(Petrinet::InitialNet,true);
    // register formula at parent
    formulas[id_stack.top()].push_back(f);    
}];

NoDeadlock() -> [internal: {
    StatePredicate *f = new DeadlockPredicate(Petrinet::InitialNet,false);
    // register formula at parent
    formulas[id_stack.top()].push_back(f);    
}];

//////////////////////////////////////////////////////////////////////////
%uview buechi;


%{ KC_UNPARSE
#include<Net/Petrinet.h>
std::map<int, std::vector<std::pair<int,int> > > transitions ;
std::set<int> acceptingset;
std::map<int, StatePredicate*> predicates;
std::map<int, int> states_to_interal;

int get_state_number(int state){
	if (states_to_interal.find(state) == states_to_interal.end()){
		// get a new state number
		int new_state_number = states_to_interal.size();
		states_to_interal[state] = new_state_number;
		return new_state_number;
	}
	return states_to_interal[state];
}
%}


b=BuechiNull -> [buechi: { RT::rep->message("NULL BUECHI"); b->automata = 0; }];
b=BuechiAutomaton(trans,accept) -> [buechi:
	trans
	accept
    {
    	//RT::rep->message("REAL BUECHI");
    	
    	// get number of states
    	int number_of_states = states_to_interal.size();
		
		// construction of the buechi automata
		b->automata = new BuechiAutomata(Petrinet::InitialNet);
		b->automata->cardStates = number_of_states;

		//RT::rep->message("transfer predicates");
    	// transfer all state predicates
    	
    	//RT::rep->message("transfer transitions");
    	// build the transition-data-structure
    	b->automata->cardTransitions = new int[number_of_states]();
    	b->automata->nextstate = new int*[number_of_states]();
    	b->automata->guard = new StatePredicate**[number_of_states]();
    	for (int i = 0; i < number_of_states; i++){
    		b->automata->cardTransitions[i] = transitions[i].size();
    		b->automata->nextstate[i] = new int[transitions[i].size()]();
    		b->automata->guard[i] = new StatePredicate*[transitions[i].size()]();
    		for (int j = 0; j < transitions[i].size(); j++){
    			b->automata->guard[i][j] = predicates[transitions[i][j].first];
    			b->automata->nextstate[i][j] = transitions[i][j].second;
    		}
    	}
    	
    	//RT::rep->message("transfer accepting states");
        // produce a list for the accepting states
        b->automata->isStateAccepting = new bool[number_of_states]();
        for (__typeof__(acceptingset.begin()) i = acceptingset.begin(); i != acceptingset.end(); i++)
        	b->automata->isStateAccepting[*i] = true;
    }
];


ExpandedBuechiRule(from, formula, to) -> [buechi:
    {
    
    	formula->unparse(myprinter, kc::internal);
        StatePredicate* result = formula->formula;
        //formula->free(true);
        
        
        
    	//RT::rep->message("RULE %d -> %d (%d)", from, to,formula->formula);
    	// get a number for the formula
    	int formula_number = (int)(predicates.size());
    	predicates[formula_number] = result;
    	// get numbers for the from and to state
    	int from_number = get_state_number(from->value);
    	int to_number = get_state_number(to->value);
    	// insert transition
        transitions[from_number].push_back(std::make_pair(formula_number, to_number));
    } 
];

EmptyBuechiRules() -> [buechi:];

BuechiRules(lrules, rrules) -> [buechi:
    lrules
    rrules
];

AcceptingState(state) -> [buechi:
    {
        acceptingset.insert(get_state_number(state->value));
    }
];

AcceptingSet(set1, set2) -> [buechi:
  set1
  set2
];

EmptyAcceptingSet() -> [buechi:];



//////////////////////////////////////////////////////////////////////////
%uview ctl;
%{ KC_UNPARSE

#include <Core/Dimensions.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/CTL/AUFormula.h>
#include <Formula/CTL/ARFormula.h>
#include <Formula/CTL/AGFormula.h>
#include <Formula/CTL/AFFormula.h>
#include <Formula/CTL/AXFormula.h>
#include <Formula/CTL/AtomicFormula.h>
#include <Formula/CTL/CTLFormula.h>
#include <Formula/CTL/ConjunctionFormula.h>
#include <Formula/CTL/DisjunctionFormula.h>
#include <Formula/CTL/EUFormula.h>
#include <Formula/CTL/ERFormula.h>
#include <Formula/CTL/EFFormula.h>
#include <Formula/CTL/EGFormula.h>
#include <Formula/CTL/EXFormula.h>
#include <Formula/CTL/NotFormula.h>
#include <Net/Petrinet.h>

#include <map>
#include <vector>
#include <stack>

// mapping from node ids to factors
std::map<int, int> ctl_current;

// mapping from tree node ids to list of intermediate formulas
std::map<int, std::vector<CTLFormula_p> > ctl_formulas;

// invariant: top is always my parent
std::stack<size_t> ctl_id_stack;

// next free id
size_t ctl_nextId = 0;

// needs dfs-storage;
size_t ctl_dfs_id = 0;

// needs cached results id
size_t ctl_result_cache_id = 0;

// temporal storage to be able to include the LessEqualStatePredicate into an AtomicFormula
StatePredicateProperty* spp;
%}

//EINSTIEG

f=AtomicProposition(x=Elementary()) -> [ctl: {
            AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,x->pred->copy(NULL));
	  ctl_formulas[ctl_id_stack.top()].push_back(af);
}];
f=AtomicProposition(NNegation(x=Elementary())) -> [ctl: {
            AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,x->pred->copy(NULL)->negate());
	  ctl_formulas[ctl_id_stack.top()].push_back(af);
}];

f=AtomicProposition(True()) -> [ctl: {
 AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,new TruePredicate(ParserPTNet::relabelnet));
          ctl_formulas[ctl_id_stack.top()].push_back(af);
}];

f=AtomicProposition(False()) -> [ctl: {
 AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,new FalsePredicate(ParserPTNet::relabelnet));
          ctl_formulas[ctl_id_stack.top()].push_back(af);
}];

f=AtomicProposition(Deadlock()) -> [ctl: {
 AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,new DeadlockPredicate(ParserPTNet::relabelnet,1));
          ctl_formulas[ctl_id_stack.top()].push_back(af);
}];

f=AtomicProposition(NoDeadlock()) -> [ctl: {
 AtomicFormula* af = new AtomicFormula(ParserPTNet::relabelnet);
            af->inner = new StatePredicateProperty(ParserPTNet::relabelnet,new DeadlockPredicate(ParserPTNet::relabelnet,0));
          ctl_formulas[ctl_id_stack.top()].push_back(af);
}];

f=StatePredicateFormula(x) -> [ctl: {
	ctl_current.clear();
	ctl_formulas.clear();
	while(ctl_id_stack.size()) ctl_id_stack.pop();
	ctl_nextId = 0;
	ctl_dfs_id = 0;
	ctl_result_cache_id = 0;

    f->unparse(myprinter, kc::containstemp);
    // making sure that correct information about contained temporal operators is available at every node. Note that this information is _inevietable_ in order to unparse CTL-Formulas correctly!
    ctl_id_stack.push(ctl_nextId++);
}
x
{
    ctl_nextId = ctl_id_stack.top();
    ctl_id_stack.pop();

    // return the only entry for the root
    assert(ctl_nextId == 0);
    f->ctl_formula = ctl_formulas[0][0];
    f->ctl_formula->payloadsize = ctl_dfs_id*SIZEOF_STATENUMBER_T + (2*ctl_result_cache_id+7)/8;
}];

//Boolsche Operatoren

// Negation can only be in as consequence of workarounds, see phase 5 in rewrite
Negation(a) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
a
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

    assert(ctl_formulas[myId].size() == 1);
    NotFormula *nf = new NotFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
    nf -> containsNext = containsNext;
    ctl_formulas[ctl_id_stack.top()].push_back(nf);
}];

f=Conjunction(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

    ConjunctionFormula *cf = new ConjunctionFormula(ParserPTNet::relabelnet,ctl_formulas[myId]);
    cf -> containsNext = containsNext;
    ctl_formulas[ctl_id_stack.top()].push_back(cf);
}];

f=Disjunction(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

    DisjunctionFormula *df = new DisjunctionFormula(ParserPTNet::relabelnet,ctl_formulas[myId]);
     df -> containsNext = containsNext;
    ctl_formulas[ctl_id_stack.top()].push_back(df);
}];

//CTL-Operatoren

AX(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	AXFormula* axf = new AXFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	axf -> containsNext = true;
	
	ctl_formulas[ctl_id_stack.top()].push_back(axf);
}];

AF(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	AFFormula* aff = new AFFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	aff -> containsNext = containsNext;
	
	ctl_formulas[ctl_id_stack.top()].push_back(aff);
}];

AG(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	AGFormula* agf = new AGFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	agf -> containsNext = containsNext;
	
	ctl_formulas[ctl_id_stack.top()].push_back(agf);
}];

AU(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
	const size_t myDFSId = ctl_dfs_id++;
	const size_t myCacheId = ctl_result_cache_id++;
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 2);
	AUFormula* auf = new AUFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0], ctl_formulas[myId][1]);
	auf->index = 2 * myCacheId;
	auf->dfsindex = myDFSId * SIZEOF_STATENUMBER_T;
	auf -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(auf);
}];

AR(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
	const size_t myDFSId = ctl_dfs_id++;
	const size_t myCacheId = ctl_result_cache_id++;
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 2);
	ARFormula* arf = new ARFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0], ctl_formulas[myId][1]);
	arf->index = 2 * myCacheId;
	arf->dfsindex = myDFSId * SIZEOF_STATENUMBER_T;
	arf -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(arf);
}];

EX(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	EXFormula* exf = new EXFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	exf -> containsNext = true;

	ctl_formulas[ctl_id_stack.top()].push_back(exf);
}];

EG(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	EGFormula* egf = new EGFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	egf -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(egf);
}];

EF(x) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
}
x
{
	// after returning from my childs, I am the top of the stack
	const arrayindex_t myId = ctl_id_stack.top();
	ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 1);
	EFFormula* eff = new EFFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0]);
	eff -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(eff);
}];

EU(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
	const size_t myDFSId = ctl_dfs_id++;
	const size_t myCacheId = ctl_result_cache_id++;
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 2);
	EUFormula* euf = new EUFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0], ctl_formulas[myId][1]);
	euf->index = 2 * myCacheId;
	euf->dfsindex = myDFSId * SIZEOF_STATENUMBER_T;
	euf -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(euf);
}];

ER(a,b) -> [ctl: {
    ctl_id_stack.push(ctl_nextId++);
	const size_t myDFSId = ctl_dfs_id++;
	const size_t myCacheId = ctl_result_cache_id++;
}
a b
{
    // after returning from my childs, I am the top of the stack
    const arrayindex_t myId = ctl_id_stack.top();
    ctl_id_stack.pop();

	assert(ctl_formulas[myId].size() == 2);
	ERFormula* erf = new ERFormula(ParserPTNet::relabelnet,ctl_formulas[myId][0], ctl_formulas[myId][1]);
	erf->index = 2 * myCacheId;
	erf->dfsindex = myDFSId * SIZEOF_STATENUMBER_T;
	erf -> containsNext = containsNext;

	ctl_formulas[ctl_id_stack.top()].push_back(erf);
}];



//////////////////////////////////////////////////////////////////////////
%uview ltl;

%{ KC_UNPARSE
#include <Core/Runtime.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <vector>

extern int current_next_string_index_number;
std::vector<LTLTree_p> ltlstack;
%}


f=StatePredicateFormula(x) -> [ltl:
        {
		ltlstack.clear();
		current_next_string_index_number = 1;
        }
	x
	{
		// put top of stack into memoy
		f->ltl_tree = bin_simpler(push_negation(tl_nn(NOT, ltlstack.back(), NULL)));
		//f->ltl_tree = ltlstack.back();
	}];

f=AtomicProposition(NNegation(x=Elementary()))  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = x->pred -> copy(NULL);
	result = result -> negate();
        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

f=AtomicProposition(x=Elementary())  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = x->pred->copy(NULL);
        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

f=AtomicProposition(True())  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = new TruePredicate(Petrinet::InitialNet);
        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

f=AtomicProposition(Deadlock())  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = new DeadlockPredicate(Petrinet::InitialNet,true);
        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

f=AtomicProposition(NoDeadlock())  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = new DeadlockPredicate(Petrinet::InitialNet,false);
        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

f=AtomicProposition(False())  -> [ltl:
	{
		// treat this as atomic in Buchi automaton
        StatePredicate * result = new FalsePredicate(Petrinet::InitialNet);

        
        // produce a new string
        int num;
        char_p sym = produce_next_string(&num);
        // I know what i am doing, the current sub-tree is an atomic propositon
        predicateMap[num] = result;
        LTLTree_p newTree = tl_nn(PREDICATE, NULL, NULL);
        newTree->sym = tl_lookup(sym);
        newTree = bin_simpler(newTree);
        //RT::rep->message("SYM %x %x",newTree,newTree->sym);
        assert(newTree->sym);
        ltlstack.push_back(newTree);
	}
];

Eventually(x) -> [ltl: 
	x
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(U_OPER, tl_nn(TTRUE, NULL, NULL), a)));
	}
 ];
Always(x)     -> [ltl:
	x
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(V_OPER, tl_nn(FFALSE, NULL, NULL), a)));
	}
];
NextState(x)   -> [ltl:
	x
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(NEXT, a, NULL)));
	}
];
Until(phi,psi) -> [ltl: 
	phi
	psi
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		LTLTree_p b = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(U_OPER, b, a)));
	}
];

Release(phi,psi) -> [ltl: 
	phi
	psi
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		LTLTree_p b = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(V_OPER, b, a)));
	}
];

Negation(x) -> [ltl:
	x
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(NOT, a, NULL)));
	}
];
Conjunction(x, y) -> [ltl: 
	x
	y
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		LTLTree_p b = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(AND, a, b)));
	}
];
Disjunction(x, y) -> [ltl:
	x
	y
	{
		LTLTree_p a = ltlstack.back();
		ltlstack.pop_back();
		LTLTree_p b = ltlstack.back();
		ltlstack.pop_back();
		ltlstack.push_back(bin_simpler(tl_nn(OR, a, b)));
	}
];


/*************************************************************************/
/*                   High Level Net                                      */
/*************************************************************************/

%uview hl_staticanalysis;

Net(c,d,p,m,t) -> [hl_staticanalysis: c d p m t]; 

EmptyDefinitionsList() -> [hl_staticanalysis: ];
DefinitionsList(l,d) -> [hl_staticanalysis: l d];

Constant(t,c,a,e) -> [hl_staticanalysis: t c a e 
{
	/* check whether assigned expression is constant */
	if(! (e->is_constant))
	{
		/* todo: error message */
	}
 	/* todo: determine required type for c from t and a */
	/* todo: check compatibility between required and inferred type */
	/* todo: check whether value of e is within boundaries */
	/* todo: create constant object */
	
}
];
Sort(t,c,a) -> [hl_staticanalysis: t c 
{
	a -> inherited_type = t;
}
a 
{
	/* todo: create type object from a -> type */
}
];
Function(t,c,a,f,b) -> [hl_staticanalysis: t c 
{
	a -> inherited_type = t;
}
a f b
{
	/* todo: create return type from a -> type*/
	/* todo: determine parameter list with types */
        /* todo: create function object */
}
];

TypeBool() -> [hl_staticanalysis: 
{
	size = 2;
	is_finite = true;
	is_scalar = true;
	/* todo: create type object */
}
];

TypeEnum(i) -> [hl_staticanalysis:  i
{
	size = i -> nrvalues;
	if(size == 0)
	{
		/* todo: error message */
	}
	is_finite = true;
	is_scalar = true;
	/* todo: create type object */
}
];

TypeIdent(c) -> [hl_staticanalysis:
{
	/* todo: retrieve type object from c */
	/* todo: set is_finite, is_scalar, and size */
}
];

TypeIntInterval(e,f) -> [hl_staticanalysis: e f 
{
	if(!(e->is_constant && f->is_constant))
	{
		/* todo: error message */
	}
	if(!(e -> is_scalar && f -> is_scalar))
	{
		/* todo: error message */
	}
	arrayindex_t left = *(reinterpret_cast<arrayindex_t *>(e->value));
	arrayindex_t right = *(reinterpret_cast<arrayindex_t *>(f->value));
	if(right < left)
	{
		/* todo: error message */
	}
	is_finite = true;
	is_scalar = true;
	size = right - left +1;
}
];
TypeStruct(s) -> [hl_staticanalysis: s
{
	size = s -> size;
	is_finite = s -> is_finite;
	is_scalar = false;
}
];
TypeInt() -> [hl_staticanalysis:
{
	is_finite = false;
	is_scalar = true;
	size = 0; /* irrelevant */
}
];
TypeMultiset(t) -> [hl_staticanalysis: t
{
	if(! (t->is_finite))
	{
		/* todo: error message */
	}
	is_finite = false;
	is_scalar = false;
	size = 0; /* irrelevant */
}
];
TypeBlack()  -> [hl_staticanalysis: 
{
	is_finite = true;
	is_scalar = true;
	size = 1;
}
];

EmptyOptNumber() -> [hl_staticanalysis:
{
	value = ARRAYINDEX_T_MAX;
	set = false;
}
];

OptNumber(i) -> [hl_staticanalysis:
{
	value = i-> value;
	set = true;
}
];

EmptyIdentList() -> [hl_staticanalysis: 
{
	maxvalue = 0;
	nrvalues = 0;
}
];

IdentList(i,d) -> [hl_staticanalysis: i d
{
	nrvalues = i -> nrvalues + 1;
	if(d -> value <= i -> maxvalue)
	{
		/* todo: error message */
	}
	if(!(d -> set))
	{
		d -> value = i -> maxvalue + 1;
	}
	/* todo: record d -> value in symboltable of symbol d -> c */
	maxvalue = d -> value;
	
}
];

Idents(c,o) -> [hl_staticanalysis: c o
{
	value = o -> value;
	set = o -> set;
}
];

EmptyStructTypeList() -> [hl_staticanalysis:
{
	size = 0;
	nrcomponents = 0;
	is_finite = true;
}
];
StructTypeList(l,s) -> [hl_staticanalysis: l s
{
	size = l -> size * s -> size;
	nrcomponents = l -> nrcomponents + 1;
	is_finite = (l -> is_finite) && (s -> is_finite);
}
];

StructType(t,c,a) -> [hl_staticanalysis: t c 
{
	a -> inherited_type = t;
}
a
{
	size = t -> size * a -> nrelements;
	is_finite = t -> is_finite;
	is_scalar = false;
}
];

VarOrArray(c,a) -> [hl_staticanalysis: 
{
	a -> inherited_type = inherited_type;
}
c a 
{
	type = a -> type;
}
];


EmptyArrayList() -> [hl_staticanalysis:
{
	nrdimensions = 0;
	nrelements = 1;
	type = inherited_type;
}
];
ArrayList(a,e) -> [hl_staticanalysis: 
{
	a -> inherited_type = inherited_type;
}
a e 
{
	if(!(e -> is_constant))
	{
		/* todo: error message */
	}
	if(e -> value == 0)
	{
		/* todo: error message */
	}
	nrdimensions = a -> nrdimensions + 1;
	// todo: nrelements = (a -> nrelements) * reinterpret_cast<int>(e -> value[0]);
	// todo: type = kc::TypeArray(a -> type, e -> value);
	type -> size = nrelements * a -> type -> size;
	type -> is_finite = a -> type -> is_finite;
	type -> is_scalar = false;
	/* todo: test for arrays with too many elements */
}
];
/*
functionParametersList:
  EmptyFunctionParametersList()
| FunctionParametersList(functionParameters functionParametersList)
;

functionParameters:
  FunctionParameters(type varOrArray)
;

expression:
  ExprLeftvalue(leftvalue)
| ExprInitializerList(initializerList)
| AssignEqual(leftvalue expression)
| AssignPlus(leftvalue expression)
| AssignMinus(leftvalue expression)
| AssignTimes(leftvalue expression)
| AssignDivide(leftvalue expression)
| AssignMod(leftvalue expression)
| IncrementVal(leftvalue)
| DecrementVal(leftvalue)
| ValIncrement(leftvalue)
| ValDecrement(leftvalue)
| PositiveExpr(expression)
| NegativeExpr(expression)
| ExprAddition(expression expression)
| ExprSubtraction(expression expression)
| ExprMultiplication(expression expression)
| ExprDivision(expression expression)
| ExprModulo(expression expression)
| ExprNumber(integer)
| NotExpr(expression)
| ExprAnd(expression expression)
| ExprOr(expression expression)
| ExprTrue()
| ExprFalse()
| ExprEquivalent(expression expression)
| ExprNotEqual(expression expression)
| ExprLessThan(expression expression)
| ExprGreaterThan(expression expression)
| ExprLessOrEqual(expression expression)
| ExprGreaterOrEqual(expression expression)
| FunctionCall(casestring expressionlist)
| ExprCommaSeparated(expressionlist)
| ExprOtherIf(expression expression expression)
| ExprAll(type)
{ checkName check = UNDEF; }
;


leftvalue:
  LeftValIdent(casestring)       
| LeftValBrackets(leftvalue expression)
| LeftValDot(leftvalue casestring)
;

expressionlist:
   EmptyExpressionList()
|  ExpressionList(expression expressionlist)
;

initializerList:
  ExprInBraces(expressionlist)
| ExprInBracesColon(expression expression expressionListColon)
;

expressionListColon:
  EmptyExpressionListColon()
| ExpressionListColon(expression expression expressionListColon)
;

placeblocklist:
  EmptyPlaceBlockList()
| PlaceBlockList(placeblock placeblocklist)
;

optSafe:
  EmptySafe()
| Safe(integer)
;

place:
  Place(casestring)
;

placelist:
  EmptyPlaceList()
| PlaceList(place placelist)
;

placeblock:
  PlaceBlock(optSafe type placelist)
;

marking:
  EmptyMarking()
| Marking(casestring expression marking)
;

transition:
  EmptyTransition()
| Transition(casestring fairness variable guard identExprList identExprList transition)
;

fairness:
  EmptyFairness()
| WeakFair()
| StrongFair()
;

variable:
  EmptyVariable()
| Variable(type varOrArrayList variable)
;

varOrArrayList:
  EmptyVarOrArrayList()
| VarOrArrayList(varOrArray varOrArrayList)
;

guard:
 Guard(expression)
;


identExprList:
  EmptyIdentExprList()
| IdentExprList(casestring expression identExprList)
;

block:
  Block(declarationOrStatement)
;

declarationOrStatement:
  EmptyDeclarationOrStatement()
| DeclOrStatemDeclaration(declaration declarationOrStatement)
| DeclOrStatemStatement(statement declarationOrStatement)
;

declaration: 
  Declaration(type varOrArrayList)
;

statement:
  StatementBlock(block)
| StatementExprSemicolon(expression)
| StatementIf(expression statement statement)
| StatementWhile(expression statement)
| StatementDoWhile(statement expression)
| StatementForExpr(expression expression expression statement)
| StatementForTypeExpr(type casestring expression expression expression statement)
| StatementForIdentColon(casestring expression statement)
| StatementForAll(type casestring type)
| StatementSwitch(expression switchCaseList)
| StatementBreak()
| StatementContinue()
| StatementReturn(expression)
| StatementSkip()
;

switchCase:
  SwitchCase(expression statement)
| SwitchDefault(statement)
;

switchCaseList:
  EmptySwitchCaseList()
| SwitchCaseList(switchCase switchCaseList)
;
*/

///////////////////////////////////////////////////////////////
%uview visible;

// Marks transitions as visible that may change value of any subformula

//StatePredicateFormula(x) -> [visible: 
//{
	//for(arrayindex_t i = 0; i < Net::Card[TR]; i++)
	//{
		//Transition::Visible[i] = false;
	//}
//} x
//{
	//containsNext = x -> containsNext;
//}];
////ComputeBound(LessEqualAtomicProposition(x,y)) -> [visible: x y {containsNext = true;}];
//
//AtomicProposition(x=Elementary()) -> [visible: {x -> pred -> setVisible();}];
//AtomicProposition(NNegation(x=Elementary())) -> [visible: {x -> pred -> setVisible();}];
//Negation(x) -> [visible: x {containsNext = x -> containsNext;}];
//Conjunction(x, y) -> [visible:  x  y {containsNext = x -> containsNext || y -> containsNext;}];
//Disjunction(x, y) -> [visible: x y {containsNext = x -> containsNext || y -> containsNext;}];
//AllPath(x) -> [visible: x {containsNext = x -> containsNext;}];
//ExPath(x)  -> [visible: x {containsNext = x -> containsNext;}];
//Always(x)     -> [visible: x {containsNext = x -> containsNext;}];
//Eventually(x) -> [visible: x {containsNext = x -> containsNext;}];
//NextState(x) -> [visible: x {containsNext = true;}];
//Until(phi,psi) -> [visible: phi  psi {containsNext = phi->containsNext || psi -> containsNext;}];
//Release(phi,psi) -> [visible: phi psi {containsNext = phi->containsNext || psi->containsNext;}];
//EU(x,y) -> [visible: x y {containsNext = x -> containsNext || y -> containsNext;}];
//AR(x,y) -> [visible: x y {containsNext = x -> containsNext || y -> containsNext;}];
//ER(x,y) -> [visible: x y {containsNext = x -> containsNext || y -> containsNext;}];
//AU(x,y) -> [visible: x y {containsNext = x -> containsNext || y -> containsNext; }];
//AF(x)   -> [visible:  x {containsNext = x -> containsNext;}];
//AG(x)   -> [visible: x {containsNext = x -> containsNext;}];
//AX(x)   -> [visible: x {
//containsNext = true;}];
//EX(x)   -> [visible: x {containsNext = true;}];
//EF(x)   -> [visible: x {containsNext = x -> containsNext;}];
//EG(x)   -> [visible: x {containsNext = x -> containsNext;}];

//ConjunctionList(l) -> [visible: l {containsNext = l -> containsNext;}];
//ConstConjunction_list(h, NiltConjunction_list()) -> [visible: h {containsNext = h -> containsNext;}];
//ConstConjunction_list(h, l) -> [visible: h l {containsNext = h -> containsNext || l -> containsNext;}];
//
//DisjunctionList(l) -> [visible: l {containsNext = l -> containsNext;}];
//ConstDisjunction_list(h, NiltDisjunction_list()) -> [visible: h {containsNext = h -> containsNext;}];
//ConstDisjunction_list(h, l) -> [visible: h l {containsNext = h -> containsNext || l -> containsNext;}];
//

//True() -> [visible:  {containsNext = false;}];
//False() -> [visible:  {containsNext = false;}];
//Deadlock() -> [visible: 
//{
	//for(arrayindex_t i = 0; i < Net::Card[TR];i++)
	//{
		//Transition::Visible[i] = true;
	//}
	//containsNext = false;
//}
//];
//NoDeadlock() -> [visible: 
//{
	//for(arrayindex_t i = 0; i < Net::Card[TR];i++)
	//{
		//Transition::Visible[i] = true;
	//}
	//containsNext = false;
//}
//];
///////////////////////////////////////////////////////////////////////////
// make sure that atomic propositions appear in unique incarnations
// we assume that formula as such is already copied using the kimwitu method

%uview copy;

StatePredicateFormula(x) -> [copy: x];
c=CompBound() -> [copy:  {c -> formula = c -> formula -> copy(NULL);}];

a=AtomicProposition(x=Elementary()) -> [copy:  {x -> pred = x->pred->copy(NULL);}];
a=AtomicProposition(NNegation(x=Elementary())) -> [copy: {x->pred = x->pred->copy(NULL);}];
Negation(x) -> [copy: x ];
Conjunction(x, y) -> [copy: x y];
Disjunction(x, y) -> [copy: x y ];
ExclusiveDisjunction(x, y) -> [copy: x y ];
Implication(x, y) -> [copy: x y ];
Equivalence(x, y) -> [copy: x y];
AllPath(x) -> [copy: x ];
ExPath(x)  -> [copy: x ];
Always(x)     -> [copy: x];
Eventually(x) -> [copy: x ];
NextState(x) -> [copy: x ];
Until(phi,psi) -> [copy: phi psi ];
Release(phi,psi) -> [copy: phi psi ];
EU(x,y) -> [copy: x  y];
AR(x,y) -> [copy: x  y ];
ER(x,y) -> [copy: x y ];
AU(x,y) -> [copy: x y ];
AF(x)   -> [copy: x];
AG(x)   -> [copy: x ];
AX(x)   -> [copy: x ];
EX(x)   -> [copy: x];
EF(x)   -> [copy:  x ];
EG(x)   -> [copy:  x ];

ConjunctionList(l) -> [copy:  l ];
ConstConjunction_list(h, NiltConjunction_list()) -> [copy: h];
ConstConjunction_list(h, l) -> [copy: h  l];

DisjunctionList(l) -> [copy: l ];
ConstDisjunction_list(h, NiltDisjunction_list()) -> [copy: h];
ConstDisjunction_list(h, l) -> [copy: h l];


True() -> [copy: ];
False() -> [copy: ];
Deadlock() -> [copy: ];
NoDeadlock() -> [copy: ];


