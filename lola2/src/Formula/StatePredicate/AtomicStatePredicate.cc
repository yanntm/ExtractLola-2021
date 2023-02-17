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

\brief class implementation for atomic state predicates
*/

#include <config.h>
#include <Core/Handlers.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Net/LinearAlgebra.h>
#include <Net/NetState.h>
#include <Net/Petrinet.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <CoverGraph/CoverGraph.h>
#include <Formula/FormulaInfo.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Frontend/Parser/ParserPnml.h>
#include "../../../libs/lp_solve_5.5/lp_lib.h"

/*!
\brief creates a state predicate with a formal sum of p places with positive
factor, n places with negative factor, and constant k particular places are
added using addPos and addNeg

\param p  number of places with positive factor
\param n  number of places with negative factor
\param k  constant

\todo Schleifen behandeln - können evtl. rausgenommen werden
*/
AtomicStatePredicate::AtomicStatePredicate(Petrinet * ne, arrayindex_t p, arrayindex_t n, int k) :
    posPlaces(new arrayindex_t[p]), negPlaces(new arrayindex_t[n]),
    posMult(new capacity_t[p]), negMult(new capacity_t[n]), cardPos(p),
    cardNeg(n), up(NULL), cardUp(0), threshold(k), sum(0), original(true),
    posSymbol(new Symbol *[p]), negSymbol(new Symbol *[n])
{
    isfireable = false;
    isatomic = true;
    net = ne;
    parent = NULL;
    magicnumber = MagicNumber::assign();
    literals = 1;
    forBound = false;
}

AtomicStatePredicate::AtomicStatePredicate(Petrinet * n,Term * T) :
    up(NULL), cardUp(0), threshold(0), sum(0), original(true)
{
    isfireable = false;
    isatomic = true;
    magicnumber = MagicNumber::assign();
	net = n;
    literals = 1;
    forBound = false;
    int maxplace = 0;
    for(Term * ttt = T; ttt; ttt = ttt -> next)
    {
	if(ttt -> place != UINT32_MAX && ttt -> place > maxplace) 
	{
		maxplace = ttt -> place;
	}
    }
    maxplace++;
    int * mult = new int[maxplace];
    Symbol ** sym = new Symbol * [maxplace];
    memset(mult, 0, maxplace * sizeof(int));
    memset(sym, 0, maxplace * sizeof(Symbol *));
    parent = NULL;
    cardPos = 0;
    cardNeg = 0;
    while(T)
    {
	if(T->place == UINT32_MAX)
	{
		// constant factor
		if(T->mult == OMEGA)
		{
			if(threshold == -OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			threshold = OMEGA;
		}
		else if(T->mult == -OMEGA)
		{
			if(threshold == OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			threshold = -OMEGA;
	
		}
		else if((threshold != OMEGA) && (threshold != -OMEGA))
		{
			threshold += T->mult;
		}
		// else: adding omega with nonomega -> no change
	}
	else
	{
		// real place
		sym[T->place] = T->sy;
		if(T->mult == OMEGA)
		{
			if(mult[T->place] == -OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			mult[T->place] = OMEGA;
		}
		else if(T->mult == -OMEGA)
		{
			if(mult[T->place] == OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}		
			mult[T->place] = OMEGA;
		}
		else if((mult[T->place] != OMEGA) && (mult[T->place] != -OMEGA))
		{
				mult[T->place] += T->mult;
		}
		// else: adding omega with nonomega -> no change
	}
	    Term * oldT = T;
	    T = T -> next;
	    oldT -> next = NULL;
	    delete oldT;
    }
    cardPos = 0;
    cardNeg = 0;
    for(arrayindex_t i = 0; i < maxplace;i++)
    {
	if(mult[i] > 0)
	{
		cardPos++;
  	}
	else if(mult[i] < 0)
	{
		cardNeg++;
	}
    }
    posPlaces = new arrayindex_t [cardPos];
    posMult = new capacity_t [cardPos];
    posSymbol = new Symbol * [cardPos];
    negPlaces = new arrayindex_t [cardNeg];
    negMult = new capacity_t [cardNeg];
    negSymbol = new Symbol * [cardNeg];
    cardPos = 0;
    cardNeg = 0;
    for(arrayindex_t i = 0; i < maxplace;i++)
    {
	if(mult[i] > 0)
	{
		posPlaces[cardPos] = i;
		posSymbol[cardPos] = sym[i] ? sym[i] : n -> thesymbol[PL][i];
		posMult[cardPos++] = mult[i];
  	}
	else if(mult[i] < 0)
	{
		negPlaces[cardNeg] = i;
		negSymbol[cardNeg] = sym[i] ? sym[i] : n -> thesymbol[PL][i];
		negMult[cardNeg++] = -mult[i];
	}
    }
    threshold = - threshold; // threshold must be put to the other side
			    // of the inequation
    delete[] mult;
    delete[] sym;


    if(!forBound)
    {
	    // assign magic number
	    if((cardPos == 0) && (threshold >= 0))
	    {
		magicnumber = MAGIC_NUMBER_TRUE;
	    }
	    else if((cardNeg == 0) && (threshold < 0))
	    {
		magicnumber = MAGIC_NUMBER_FALSE;
	    }
	    else if((cardPos == 1) && (cardNeg == 0) && (threshold == 0) && (posMult[0] == 1))
	    {
		magicnumber = MAGIC_NUMBER_EMPTY(posPlaces[0]);  // p<1
	    }
	    else if((cardPos == 0) &&(cardNeg == 1) && (threshold == -1) && (negMult[0] == 1))
	    {
		magicnumber = MAGIC_NUMBER_MARKED(negPlaces[0]); // p>0
	    }
	    else
	    {
		magicnumber = MagicNumber::assign(); // any predicate
	    } 

	    if(magicnumber == MAGIC_NUMBER_TRUE || magicnumber == MAGIC_NUMBER_FALSE) 
	    {
		return;
	    }
	}
}

AtomicStatePredicate::AtomicStatePredicate(Petrinet * n, Term * T,bool b) :
    up(NULL), cardUp(0), threshold(0), sum(0), original(true)
{
    isfireable = false;
    isatomic = true;
    magicnumber = MagicNumber::assign();
    net = n;
    literals = 1;
    forBound = b;
    int maxplace = 0;
    for(Term * ttt = T; ttt; ttt = ttt -> next)
    {
	if(ttt -> place != UINT32_MAX && ttt -> place > maxplace)
	{
		maxplace = ttt -> place;
	}
    }
    maxplace++;
    int * mult = new int[maxplace];
    Symbol ** sym = new Symbol * [maxplace];
    memset(mult,0,maxplace * sizeof(int));
    memset(sym,0,maxplace * sizeof(Symbol *));
    parent = NULL;
    cardPos = 0;
    cardNeg = 0;
    while(T)
    {
	if(T->place == UINT32_MAX)
	{
		// constant factor
		if(T->mult == OMEGA)
		{
			if(threshold == -OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			threshold = OMEGA;
		}
		else if(T->mult == -OMEGA)
		{
			if(threshold == OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			threshold = -OMEGA;
	
		}
		else if((threshold != OMEGA) && (threshold != -OMEGA))
		{
			threshold += T->mult;
		}
		// else: adding omega with nonomega -> no change
	}
	else
	{
		// real place
		sym[T->place] = T->sy;
		if(T->mult == OMEGA)
		{
			if(mult[T->place] == -OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}
			mult[T->place] = OMEGA;
		}
		else if(T->mult == -OMEGA)
		{
			if(mult[T->place] == OMEGA)
			{
                    		RT::rep->message("%s: addition -oo + oo",
              			RT::rep->markup(MARKUP_WARNING, "error").str());
				RT::rep->abort(ERROR_SYNTAX);
			}		
			mult[T->place] = OMEGA;
		}
		else if((mult[T->place] != OMEGA) && (mult[T->place] != -OMEGA))
		{
				mult[T->place] += T->mult;
		}
		// else: adding omega with nonomega -> no change
	}
	    Term * oldT = T;
	    T = T -> next;
	    oldT -> next = NULL;
	    delete oldT;
    }
    cardPos = 0;
    cardNeg = 0;
    for(arrayindex_t i = 0; i < maxplace;i++)
    {
	if(mult[i] > 0)
	{
		cardPos++;
  	}
	else if(mult[i] < 0)
	{
		cardNeg++;
	}
    }
    posPlaces = new arrayindex_t [cardPos];
    posSymbol = new Symbol * [cardPos];
    posMult = new capacity_t [cardPos];
    negPlaces = new arrayindex_t [cardNeg];
    negSymbol = new Symbol * [cardNeg];
    negMult = new capacity_t [cardNeg];
    cardPos = 0;
    cardNeg = 0;
    for(arrayindex_t i = 0; i < maxplace;i++)
    {
	if(mult[i] > 0)
	{
		posPlaces[cardPos] = i;
		posSymbol[cardPos] = sym[i] ? sym[i] : n-> thesymbol[PL][i];
		posMult[cardPos++] = mult[i];
  	}
	else if(mult[i] < 0)
	{
		negPlaces[cardNeg] = i;
		negSymbol[cardNeg] = sym[i] ? sym[i] : n -> thesymbol[PL][i];
		negMult[cardNeg++] = -mult[i];
	}
    }
    threshold = - threshold; // threshould must be put to the other side
			    // of the inequation
    delete[] mult;

    if(!forBound)
    {
	    // assign magic number
	    if((cardPos == 0) && (threshold >= 0))
	    {
		magicnumber = MAGIC_NUMBER_TRUE;
	    }
	    else if((cardNeg == 0) && (threshold < 0))
	    {
		magicnumber = MAGIC_NUMBER_FALSE;
	    }
	    else if((cardPos == 1) && (cardNeg == 0) && (threshold == 0) && (posMult[0] == 1))
	    {
		magicnumber = MAGIC_NUMBER_EMPTY(posPlaces[0]);  // p<1
	    }
	    else if((cardPos == 0) &&(cardNeg == 1) && (threshold == -1) && (negMult[0] == 1))
	    {
		magicnumber = MAGIC_NUMBER_MARKED(negPlaces[0]); // p>0
	    }
	    else
	    {
		magicnumber = MagicNumber::assign(); // any predicate
	    } 

	    if(magicnumber == MAGIC_NUMBER_TRUE || magicnumber == MAGIC_NUMBER_FALSE) 
	    {
		return;
	    }
	}
}

AtomicStatePredicate::AtomicStatePredicate(Petrinet * n) :
    posPlaces(NULL), negPlaces(NULL), posMult(NULL), negMult(NULL), cardPos(0), posSymbol(NULL), negSymbol(0),
    cardNeg(0), up(NULL), cardUp(0), threshold(0), sum(0), original(true)
{
    isfireable = false;
    parent = NULL;
    isatomic = true;
	net = n;
	magicnumber = MAGIC_NUMBER_TRUE; // 0 <= 0
        literals = 1;
	forBound = false;
}


AtomicStatePredicate::~AtomicStatePredicate()
{
    if (!original)
    {
        return;
    }
    delete [] posPlaces;
    delete [] negPlaces;
    delete [] posSymbol;
    delete [] negSymbol;
    delete [] posMult;
    delete [] negMult;
    delete [] up;
}

arrayindex_t AtomicStatePredicate::getUpSet(arrayindex_t *stack, bool *onstack, bool * needEnabled) 
{
    assert(onstack);
    arrayindex_t stackpointer = 0;
    for (arrayindex_t i = 0; i < cardUp; i++)
    {
        arrayindex_t element;
        if (!onstack[element = up[i]])
        {
            onstack[element] = true;
            stack[stackpointer++] = element;
        }
    }
    * needEnabled = false;
    return stackpointer;
}

arrayindex_t AtomicStatePredicate::getDownSet(arrayindex_t *stack, bool *onstack, bool * needEnabled) 
{
    * needEnabled = false;
    assert(onstack);
    arrayindex_t stackpointer = 0;
    for (arrayindex_t i = 0; i < cardDown; i++)
    {
        arrayindex_t element;
        if (!onstack[element = down[i]])
        {
            onstack[element] = true;
            stack[stackpointer++] = element;
        }
    }
    return stackpointer;
}

/*!
If value of this changes, it needs to be propagated to its parent. The
parameter is the change in the formal sum k_1 p_1 + ... + k_n p_n between the
previously considered marking and the current marking. Having a precomputed
value for this change, evaluation of the formula is accelerated.
*/
void AtomicStatePredicate::update(NetState &, int delta)
{
    sum += delta;
    if (sum <= threshold && !value)
    {
        value = true;
        if (parent)
        {
            parent->updateFT(position);
        }
        return;
    }
    if (sum > threshold && value)
    {
        value = false;
        if (parent)
        {
            parent->updateTF(position);
        }
        return;
    }
}

/*!
Evaluation starts top/down, so the whole formula is examined. Evaluation is
done w.r.t. arking::Current.

\param ns  net state to evaluate the formula
*/
void AtomicStatePredicate::evaluate(NetState &ns)
{
    sum = 0;
    for (arrayindex_t i = 0; i < cardPos; ++i)
    {
        sum += ns.Current[posPlaces[i]] * posMult[i];
    }
    for (arrayindex_t i = 0; i < cardNeg; ++i)
    {
        sum -= ns.Current[negPlaces[i]] * negMult[i];
    }


    value = (sum <= threshold);
}

/*!
Evaluation with Omega starts top/down, so the whole formula is examined. Evaluation is
done w.r.t. arking::Current.

\param ns  net state to evaluate the formula
*/
void AtomicStatePredicate::evaluateOmega(NetState &ns)
{
    sum = 0;
    unknown = false;
    for (arrayindex_t i = 0; i < cardPos; ++i)
    {
        if (ns.Current[posPlaces[i]] == OMEGA)
        {
            sum = OMEGA;
        }
        else if (sum < OMEGA)
        {
            sum += ns.Current[posPlaces[i]] * posMult[i];
        }
    }
    assert(sum >= 0);
    for (arrayindex_t i = 0; i < cardNeg; ++i)
    {
        if (ns.Current[negPlaces[i]] == OMEGA)
        {
            if (sum == OMEGA && threshold < OMEGA)
            {
                unknown = true;
            }
            else
            {
                sum = -OMEGA;
            }
        }
        else if (sum > -OMEGA && sum < OMEGA)
        {
            sum -= ns.Current[negPlaces[i]] * negMult[i];
        }
    }

    if ((sum == OMEGA || sum == -OMEGA) && threshold < OMEGA && threshold > -OMEGA)
    {
        unknown = true;
    }
    value = (sum <= threshold);
}

arrayindex_t AtomicStatePredicate::countAtomic() 
{
    return 1;
}

arrayindex_t AtomicStatePredicate::collectAtomic(AtomicStatePredicate **c)
{
    c[0] = this;
    return 1;
}


arrayindex_t AtomicStatePredicate::countDeadlock() 
{
    return 0;
}

arrayindex_t AtomicStatePredicate::collectDeadlock(DeadlockPredicate **)
{
    return 0;
}

arrayindex_t AtomicStatePredicate::countFireable() 
{
    return 0;
}

arrayindex_t AtomicStatePredicate::collectFireable(FireablePredicate **)
{
    return 0;
}

void AtomicStatePredicate::initUpSet()
{
	cardUp = 0;
        up = reinterpret_cast<arrayindex_t *>(malloc(net->Card[TR] * SIZEOF_ARRAYINDEX_T));
}

void AtomicStatePredicate::finitUpSet()
{
    // shrink up array to size actually needed
    up = reinterpret_cast<arrayindex_t *>(realloc(up, cardUp * SIZEOF_ARRAYINDEX_T));
}

void AtomicStatePredicate::addToUpSet(arrayindex_t t)
{
	up[cardUp++] = t;
}

void AtomicStatePredicate::initDownSet()
{
	cardDown = 0;
        down = reinterpret_cast<arrayindex_t *>(malloc(net->Card[TR] * SIZEOF_ARRAYINDEX_T));
}

void AtomicStatePredicate::finitDownSet()
{
    // shrink up array to size actually needed
    down = reinterpret_cast<arrayindex_t *>(realloc(down, cardDown * SIZEOF_ARRAYINDEX_T));
}

void AtomicStatePredicate::addToDownSet(arrayindex_t t)
{
	down[cardDown++] = t;
}

// LCOV_EXCL_START
bool AtomicStatePredicate::DEBUG__consistency(NetState &ns)
{
    // 1. check sum
    int s = 0;
    for (arrayindex_t i = 0; i < cardPos; i++)
    {
        s += posMult[i] * ns.Current[posPlaces[i]];
    }
    for (arrayindex_t i = 0; i < cardNeg; i++)
    {
        s -= negMult[i] * ns.Current[negPlaces[i]];
    }
    assert(s == sum);
    if (value)
    {
        assert(sum <= threshold);
    }
    else
    {
        assert(sum > threshold);
    }
    /* if (this != top)
     {
         assert(parent);
     }*/
    return true;
}
// LCOV_EXCL_STOP

/*!
\param parent  the parent predicate for the new, copied, object
*/
StatePredicate *AtomicStatePredicate::copy(StatePredicate *parent)
{
    AtomicStatePredicate *af = new AtomicStatePredicate(net,0, 0, 0);
    af->magicnumber = magicnumber;
    af->value = value;
    af->upper_bound = upper_bound;
    af->lower_bound = lower_bound;
    af->position = position;
    af->parent = parent;
    // we can copy the pointers, so use the same arrays as they are not changed!
    af->posPlaces = new arrayindex_t[cardPos];
    memcpy(af->posPlaces, posPlaces, cardPos * sizeof(arrayindex_t));
    af->posSymbol = new Symbol *[cardPos];
    memcpy(af->posSymbol, posSymbol, cardPos * sizeof(Symbol *));
    af->negPlaces = new arrayindex_t[cardNeg];
    memcpy(af->negPlaces, negPlaces, cardNeg * sizeof(arrayindex_t));
    af->negSymbol = new Symbol *[cardNeg];
    memcpy(af->negSymbol, negSymbol, cardNeg * sizeof(Symbol *));
    af->posMult = new capacity_t[cardPos];
    memcpy(af->posMult, posMult, cardPos * sizeof(capacity_t));
    af->negMult = new capacity_t[cardNeg];
    memcpy(af->negMult, negMult, cardNeg * sizeof(capacity_t));
    af->cardPos = cardPos;
    af->cardNeg = cardNeg;
    af->threshold = threshold;
    af->sum = sum;
    //af->up = reinterpret_cast<arrayindex_t *>(malloc(net->Card[TR] * SIZEOF_ARRAYINDEX_T));;
    //memcpy(af->up,up, cardUp * SIZEOF_ARRAYINDEX_T);
    af->cardUp = cardUp;
    af->original = false;
    return af;
}

arrayindex_t AtomicStatePredicate::getSubs( StatePredicate * **)

{
    return 0;
}

StatePredicate *AtomicStatePredicate::negate()
{
    arrayindex_t tmp;
    arrayindex_t * tmpp;
    capacity_t * tmppppp;
    Symbol ** tmppp;
    tmp = cardPos;
    cardPos = cardNeg;
    cardNeg = tmp;
    tmpp = posPlaces;
    posPlaces = negPlaces;
    negPlaces = tmpp;
    tmppppp = posMult;
    posMult = negMult;
    negMult = tmppppp;
    tmppp = posSymbol;
    posSymbol = negSymbol;
    negSymbol = tmppp;
    threshold = - threshold - 1;
    magicnumber = -magicnumber;
    return this;
}

FormulaInfo *AtomicStatePredicate::getInfo() 
{
    FormulaInfo *Info = new FormulaInfo();
    Info->tag = formula_atomic;
    Info->cardChildren = 0;
    Info->f = const_cast<AtomicStatePredicate *>(this);
    return Info;
}

int AtomicStatePredicate::countSubFormulas() 
{
    return 1;
}

/*!
Reduces all factors and the threshold with the gcd thereof.

\note This function needs to be called after all addPos/addNeg calls are
complete.
*/
void AtomicStatePredicate::reduceFactors()
{
    // make sure there is at least one multiplicity
    assert(cardPos + cardNeg > 0);

    // initialize result value
    int gcd = (cardPos > 0) ? posMult[0] : negMult[0];

    // find gcd of threshold and all multiplicities
    for (arrayindex_t i = 0; i < cardPos; i++)
    {
        gcd = ggt(gcd, posMult[i]);
    }
    for (arrayindex_t i = 0; i < cardNeg; i++)
    {
        gcd = ggt(gcd, negMult[i]);
    }

    // add threshold to the result
    if (threshold != 0)
    {
        gcd = ggt(gcd, threshold);
    }

    assert(gcd);

    // make sure the gcd is positive
    gcd = (gcd < 0) ? -gcd : gcd;

    // apply ggt
    threshold /= gcd;
    for (arrayindex_t i = 0; i < cardPos; i++)
    {
        assert(posMult[i] % gcd == 0);
        posMult[i] /= gcd;
    }
    for (arrayindex_t i = 0; i < cardNeg; i++)
    {
        assert(negMult[i] % gcd == 0);
        negMult[i] /= gcd;
    }
}

char * addNumber(char * text, int num)
{
	char * result = text;
	result = (char *) realloc(result,strlen(result) + 32);
	if(num == OMEGA)
	{
		sprintf(result+strlen(result),"oo");
	}
	else if(num == FINITE)
	{
		sprintf(result+strlen(result)," FINITE ");
	}
	else
	{
		sprintf(result+strlen(result),"%d",num);
	}
	return result;
}

char * addText(char * text,  const char * ttext)
{
	char * result = text;
	result = (char *) realloc(result,strlen(result) + strlen(ttext) + 1);
	sprintf(result+strlen(result),"%s",ttext);
	return result;
}

char * AtomicStatePredicate::addSummand(char * text, arrayindex_t p, arrayindex_t mult)
{
	char * result = text;
	char * place;
	if(net && net -> Name && net -> Name[PL] && p < net -> Card[PL] && net -> Name[PL][p])
	{
		place = deconst(net->Name[PL][p]);
	}
	else
	{
		place = new char[1000];
		sprintf(place,"§%u{%p}", p, this);
	}
	result = (char *) realloc(result,strlen(result) + strlen(place) + 50);
	if(mult != 1)
	{
		sprintf(result+strlen(result)," %d * ",mult);
	}
	sprintf(result+strlen(result),"%s",place);
	return result;
}

char * AtomicStatePredicate::addSummand(char * text, Symbol * p, arrayindex_t mult)
{
	char * result = text;
	char * place;
		place = deconst(p->getKey());
	result = (char *) realloc(result,strlen(result) + strlen(place) + 50);
	if(mult != 1)
	{
		sprintf(result+strlen(result)," %d * ",mult);
	}
	sprintf(result+strlen(result),"%s",place);
	return result;
}

char * AtomicStatePredicate::toString()
{
	char * result = (char *) malloc(sizeof(char));
	result[0] = '\0';
	result = addText(result,"(");


	// build string left of <=
	// - all positive terms
	// - -threshold (if negative)
	// - 0 if none of these

	if(cardPos == 0)
	{
		if(threshold >= 0)
		{
			result = addText(result,"0");
		}
		else
		{
			result = addNumber(result,-threshold);
		}
		
	}
	else
	{
		result = addSummand(result,posSymbol[0],posMult[0]);
		for(arrayindex_t i = 1; i < cardPos;i++)
		{
			result = addText(result," + ");
			result = addSummand(result,posSymbol[i],posMult[i]);
		}
		if(threshold < 0)
		{
			result = addText(result," + ");
			result = addNumber(result,-threshold);
		}
	}
	result = addText(result," <= ");

	// build string  right of <=
	// - all negative terms
	// - threshold (if positive)
	// - 0 if none of these

	if(cardNeg == 0)
	{
		if(threshold <= 0)
		{
			result = addText(result,"0");
		}
		else
		{
			result = addNumber(result,threshold);
		}
		
	}
	else
	{
		result = addSummand(result,negSymbol[0],negMult[0]);
		for(arrayindex_t i = 1; i < cardNeg;i++)
		{
			result = addText(result," + ");
			result = addSummand(result,negSymbol[i],negMult[i]);
		}
		if(threshold > 0)
		{
			result = addText(result," + ");
			result = addNumber(result,threshold);
		}
	}
	result = addText(result,")");
	if(magicnumber == MAGIC_NUMBER_TRUE) result = addText(result,"MT");
	if(magicnumber == MAGIC_NUMBER_FALSE) result = addText(result,"MF");

	return result;
	
}

char * AtomicStatePredicate::toCompString()
{
	char * result = (char *) malloc(sizeof(char));
	result[0] = '\0';
	if(cardPos == 0)
	{
		char * t = new char[100];
		sprintf(t,"%d",-threshold);
		result = addText(result,t);
		delete[] t;
	}
	else	
	{
		result = addSummand(result,posSymbol[0],posMult[0]);	
	}
	for(arrayindex_t i = 1; i < cardPos;i++)
	{
		result = addText(result," + ");
		result = addSummand(result,posSymbol[i],posMult[i]);	
	}
	for(arrayindex_t i = 0; i < cardNeg;i++)
	{
		result = addText(result," - ");
		result = addSummand(result,negSymbol[i],negMult[i]);	
	}
	return result;
}
void AtomicStatePredicate::adjust(arrayindex_t old, arrayindex_t nw)
{
	for(arrayindex_t i = 0; i < cardPos; i++)
	{
		if(posPlaces[i] == old) posPlaces[i] = nw;
	}
	for(arrayindex_t i = 0; i < cardNeg; i++)
	{
		if(negPlaces[i] == old) negPlaces[i] = nw;
	}
}

void AtomicStatePredicate::setVisible(bool * vis)
{
	for(arrayindex_t i = 0; i < cardPos; i++)
	{
		arrayindex_t p = posPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][PRE][p];j++)
		{
			vis[net->Arc[PL][PRE][p][j]] = true;
		}
		for(arrayindex_t j = 0; j < net->CardArcs[PL][POST][p];j++)
		{
			vis[net->Arc[PL][POST][p][j]] = true;
		}
	}
	for(arrayindex_t i = 0; i < cardNeg; i++)
	{
		arrayindex_t p = negPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][PRE][p];j++)
		{
			vis[net->Arc[PL][PRE][p][j]] = true;
		}
		for(arrayindex_t j = 0; j < net->CardArcs[PL][POST][p];j++)
		{
			vis[net->Arc[PL][POST][p][j]] = true;
		}
	}
}

void AtomicStatePredicate::setDown(bool * dwn)
{
	for(arrayindex_t i = 0; i < cardPos; i++)
	{
		arrayindex_t p = posPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][PRE][p];j++)
		{
			dwn[net->Arc[PL][PRE][p][j]] = true;
		}
	}
	for(arrayindex_t i = 0; i < cardNeg; i++)
	{
		arrayindex_t p = negPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][POST][p];j++)
		{
			dwn[net->Arc[PL][POST][p][j]] = true;
		}
	}
}

void AtomicStatePredicate::setNondestroying(bool * nd)
{
	for(arrayindex_t i = 0; i < cardPos; i++)
	{
		arrayindex_t p = posPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][PRE][p];j++)
		{
			nd[net->Arc[PL][PRE][p][j]] = true;
		}
	}
	for(arrayindex_t i = 0; i < cardNeg; i++)
	{
		arrayindex_t p = negPlaces[i];
		for(arrayindex_t j = 0; j < net->CardArcs[PL][POST][p];j++)
		{
			nd[net->Arc[PL][POST][p][j]] = true;
		}
	}
}

AtomicBooleanPredicate * AtomicStatePredicate::DNF()
{
	// result is singleton conjunction
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(net,true);
	result -> addSub(this); // call DNF on a copy of orginal formula,
				// otherwise parent link breaks!!!!!!!!!
	result -> magicnumber = magicnumber; // result is eq to this!
	return result;
}

FormulaStatistics * AtomicStatePredicate::count(FormulaStatistics * fs)
{
	if(magicnumber == MAGIC_NUMBER_TRUE)
	{
		fs -> taut++;
		return fs;
	}
	if(magicnumber == MAGIC_NUMBER_FALSE)
	{
		fs -> cont++;
		return fs;
	}
	fs->comp++;
	fs-> place_references += cardPos;
	fs-> place_references += cardNeg;
	if(!fs -> mentioned_place)
	{
		int maxplace;
		if(net) 
		{
			maxplace =  net -> Card[PL];
		}
		else
		{
			maxplace = 0;
			for(int i = 0; i < cardPos; i++)
			{
				if(posPlaces[i] > maxplace) maxplace = posPlaces[i];
			}
			for(int i = 0; i < cardNeg; i++)
			{
				if(negPlaces[i] > maxplace) maxplace = negPlaces[i];
			}
			maxplace++;
		}

		fs->mentioned_place = new bool[maxplace];
		memset(fs->mentioned_place,0,sizeof(bool) * maxplace);
	}
	for(arrayindex_t i = 0; i < cardPos;i++)
	{
		if(!fs->mentioned_place[posPlaces[i]])
		{
			fs->mentioned_place[posPlaces[i]] = true;
			fs->visible_places++;
		}
	}
	for(arrayindex_t i = 0; i < cardNeg;i++)
	{
		if(!fs->mentioned_place[negPlaces[i]])
		{
			fs->mentioned_place[negPlaces[i]] = true;
			fs->visible_places++;
		}
	}
	return fs;
}

int AtomicStatePredicate::checkstable(bool sign, lprec * lp)
{

	// 1. check contained trap and contained invariant, but only once
	if(!sign)
	{
		checkonlyimproving();
		if(magicnumber == MAGIC_NUMBER_TRUE || magicnumber == MAGIC_NUMBER_FALSE) return 0;
		checkTrap();
		if(magicnumber == MAGIC_NUMBER_TRUE) return 0;
		checkContainedInvariant();
		if(!cardPos && !cardNeg)
		{
			magicnumber = (0 <= threshold) ? MAGIC_NUMBER_TRUE : MAGIC_NUMBER_FALSE;
			return 0;
		}
	}

	// 2. If there is no parent, I need to provide the lp
	
	bool iamtoplevel = lp ? false : true;
        ProtoNet * nnn = ProtoNet::currentnet;

	if(iamtoplevel)
	{
                nnn -> createLP();
                lp = (lprec *) (nnn -> lp);
                if(!lp)
                {
                        //RT::rep->status("NO LP1");
                        return 0;
                }
	}
	
	// 3. add constraint to parent check
        // sign=true -> inequation as such
        // sign= false -> negation of inequation
        int Ncol, ret = 0;
        Ncol = nnn->cardPL + nnn->cardTR;
        REAL * row = new REAL[Ncol + 2];
        if(row == NULL)
        {
                if(row != NULL) delete [] row;
                return 0;
        }
        std::fill(row, row + Ncol + 1, 0);
        for(int i = 0; i < cardPos; i++)
        {
                row[posPlaces[i]+1] = posMult[i];
        }
        for(int i = 0; i < cardNeg; i++)
        {
                row[negPlaces[i]+1] = - 1.0 *negMult[i];
        }

        if(sign ? add_constraint(lp, row, LE, threshold) : add_constraint(lp,row,GE,threshold+1))
        {
                delete [] row;
                return 0;
        }
        delete [] row;
	if(iamtoplevel)
	{
                if(!solve_lp(nnn,lp))
                {
                        magicnumber = sign ? MAGIC_NUMBER_FALSE : MAGIC_NUMBER_TRUE;
                }
                return 0;

	}
        return 1;

}

void AtomicStatePredicate::checkTrap()
{

//RT::rep->status("CHECK TRAP %s", toString());
	ProtoNet * nnn = ProtoNet::currentnet;
	// true --> predicate always true by contained marked trap
	// false, otherwise

	// check applicability
	if(cardPos) 
	{
//RT::rep->status("NOT APPLICABLE");
		return;
	}
	if(threshold < -1) 
	{
//RT::rep->status("NOT APPLICABLE1");
		return;
	}
	
	// compute max. trap

	ProtoPlace ** trap = new ProtoPlace * [nnn->cardPL];
	bool * intrap = (bool *) calloc(sizeof(bool), nnn->cardPL);
	int cardtrap = 0;
	int cardmarked = 0;
	for(int i = 0; i < cardNeg; i++)
	{
		intrap[negPlaces[i]] = true;
	}
	int i = 0;
	for(ProtoPlace * ppp = nnn -> firstplace; ppp; ppp = ppp -> next)
	{
		ppp -> index = i++;
		if(!intrap[ppp->index]) continue;
		trap[cardtrap++] = ppp;
		if(ppp -> marking) 
		{
			cardmarked++;
		}
	}
	if(!cardmarked)
	{
		delete [] trap;
		free(intrap);
		return;
	}

	// do until nothing changes

	arrayindex_t oldcard;
	do // repeat until nothing changes
	{
		oldcard = cardtrap;
		for(arrayindex_t i = cardtrap; i > 0; i--)
		{
			ProtoPlace *  ppp = trap[i-1];
			// consider place p
			Arc * aaa;
			for(aaa = ppp -> firstArcPost; aaa;aaa = aaa -> next_pl) 
			{
				// consider post-transition t
				ProtoTransition * ttt = aaa -> tr; 

				// check whether t is pre-transition
				Arc * bbb;
				for(bbb = ttt -> firstArcPost; bbb; bbb = bbb -> next_tr)
				{
					if(intrap[bbb -> pl -> index]) break;
				}
				if(!bbb) 
				{
					// t is post of p but not pre trap
					break;
				}
			}
			if(aaa)
			{
				// p has post-transition that is not pre-transition
				// --> remove p from trap
				intrap[ppp->index] = false;
				if(ppp->marking) cardmarked --;
				if(!cardmarked)
				{
					delete [] trap;
					free (intrap);
//RT::rep->status("EMPTY");
					return;
				}
				trap[i-1] = trap[--cardtrap];
			}
		}
	}
	while(oldcard != cardtrap);

	// max.  trap marked (otherwise, would have left) and not empty (otherwise unmarked)
	magicnumber = MAGIC_NUMBER_TRUE;
	delete [] trap;
	free (intrap);
//RT::rep->status("MARKED TRAP");
	return ;
}

// search for embedded place invariant.
// embedded = coefficients between 0 and coeffiecent in sum
void AtomicStatePredicate::checkContainedInvariant()
{

//RT::rep->status("CHECK CONT INV");
	ProtoNet * nnn = ProtoNet::currentnet;
	capacity_t * initialmarking = new capacity_t[nnn->cardPL];
	for(ProtoPlace * ppp = nnn -> firstplace; ppp; ppp = ppp -> next)
	{
		initialmarking[ppp->index] = ppp -> marking;
	}
	/* manage places */
	bool * insum = new bool[nnn->cardPL] ;
	for(arrayindex_t i = 0; i < nnn->cardPL;i++)
	{
		insum[i] = 0;
	}
	arrayindex_t * map = new arrayindex_t[nnn->cardPL];
        lprec *lp;
        int Ncol, *colno = NULL, j, ret = 0;
        REAL *row = NULL;
        Ncol = cardPos + cardNeg;
        lp = make_lp(0, Ncol);
        if(lp == NULL) 
	{
		return;
	}

	/* create space large enough for one row */
	colno = new int[Ncol];
	row = new REAL[Ncol + 1];
	if((colno == NULL) || (row == NULL)) return;

        /* x_p has code p+1 */
	/* set all variables to integer */
	for (int i=1; i<=Ncol; i++) {
		set_int(lp, i, TRUE);
	}
        
        for (int i = 0; i < cardPos; i++) {
		set_bounds(lp,i+1,0.0,static_cast<double>(posMult[i]));
        }
        for (int i = 0; i < cardNeg; i++) {
		set_bounds(lp,cardPos + i+1,-1.0 * negMult[i],0.0);
        }


        /* set the objective function */

	// objective = subtract as much as possible from the sum
        j = 0;

	row[0] = 0.0;
        for (int i = 0; i < cardPos; i++) {
		map[posPlaces[i]] = j;
		insum[posPlaces[i]] = true;
                row[++j] = 1.0;
        }

        for (int i = 0; i < cardNeg; i++) {
		map[negPlaces[i]] = j;
		insum[negPlaces[i]] = true;
                row[++j] = -1.0;
        }

        /* set the objective in lpsolve */
        if(!set_obj_fn(lp,row)) return;

        if(ret == 0) {
                /* set the object direction to maximize */
                set_maxim(lp);
                set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
        }

        /* add the constraits row by row.  */
	/* These are the rows of the invariance equation*/
        for (ProtoTransition * ttt=nnn -> firsttransition; ttt; ttt = ttt -> next) 
	{
		bool flag = false;
		std::fill(row, row + Ncol + 1, 0);
		for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
		{
			ProtoPlace * ppp = aaa -> pl;
			if(insum[ppp -> index]) 
			{
				row[map[ppp->index]+1] = static_cast<double>(aaa -> mult);
				flag = true;
			}
                }

		for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			ProtoPlace * ppp = aaa -> pl;
                        if(insum[ppp->index]) 
			{
				row[map[ppp->index]+1] += static_cast<double>(-1 * aaa->mult);
				flag = true;
			}
                }
                if(flag && !add_constraint(lp, row, EQ, 0.0)) return;
        }
	set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

	/*to show the model in lp format on screen */
	//write_LP(lp, stdout);

	/* I only want to see important messages on screen while solving */
	set_verbose(lp, IMPORTANT);

	/* Now let lpsolve calculate a solution */
	set_timeout(lp,5);
	ret = solve(lp);
	if(ret != OPTIMAL && ret != SUBOPTIMAL) return;

	/* objective value */
	int result = static_cast<int> (get_objective(lp) + 0.5);
	if(result == 0) return;

	/* variable values */
	get_variables(lp, row);

	int tr = 0;
	for(arrayindex_t i = 0; i < cardPos;i++)
	{
		int val = static_cast<int>(row[map[posPlaces[i]]] + 0.5);
		posMult[i] -= val;
		tr += val * initialmarking[posPlaces[i]];
	}
	for(arrayindex_t i = 0; i < cardPos;i++)
	{
		if(!posMult[i]) 
		{
			posPlaces[i] = posPlaces[--cardPos];
			posMult[i--] = posMult[cardPos];	
		}
	}
	for(arrayindex_t i = 0; i < cardNeg;i++)
	{
		int val = static_cast<int>(row[map[negPlaces[i]]] - 0.5);
		negMult[i] += val;
		tr += val * initialmarking[negPlaces[i]];
		if(!negMult[i]) 
		{
			negPlaces[i] = negPlaces[--cardNeg];
			negMult[i--] = negMult[cardNeg];	
		}
	}
	threshold -= tr;

        /* free allocated memory */
	delete [] initialmarking;
        if(row != NULL)
                delete [] row;
        if(colno != NULL)
                delete [] colno;

        if(lp != NULL) {
                /* clean up such that all used memory by lpsolve is freed */
                delete_lp(lp);
        }
	return;
}


bool AtomicStatePredicate::monotonous()
{
	if(cardPos == 0) return true;
	return false;
}

bool AtomicStatePredicate::exec_shift(int *cutoff, capacity_t* m0)
{
		for(int i = 0; i < cardPos; i++)
		{
			threshold -= (m0[posPlaces[i]] - cutoff[posPlaces[i]])* posMult[i];
		}
		for(int i = 0; i < cardNeg; i++)
		{
			threshold += (m0[negPlaces[i]] - cutoff[negPlaces[i]])* negMult[i];
		}
	return true;
}

void AtomicStatePredicate::markvisible()
{

	for(int i = 0; i < cardPos; i++)
	{
		posSymbol[i]->visible = ProtoNet::currentvisibility;
	}
	for(int i = 0; i < cardNeg; i++)
	{
		negSymbol[i] ->visible = ProtoNet::currentvisibility;
	}
}

void AtomicStatePredicate::markslicing(bool parity)
{

	for(int i = 0; i < cardPos; i++)
	{
		if(parity)
		{
			((PlaceSymbol *) posSymbol[i])->occursnegative = true;
		}
		else
		{
			((PlaceSymbol *) posSymbol[i])->occurspositive = true;
		}
	}
	for(int i = 0; i < cardNeg; i++)
	{

		if(parity)
		{
			((PlaceSymbol *) negSymbol[i])->occurspositive = true;
		}
		else
		{
			((PlaceSymbol *) negSymbol[i])->occursnegative = true;
		}
	}
}

void AtomicStatePredicate::relabel()
{
	net = ParserPTNet::relabelnet;
	for(int i = 0; i < cardPos; i++)
	{
		posPlaces[i] = posSymbol[i]->index;

	}
	for(int i = 0; i < cardNeg; i++)
	{
		negPlaces[i] = negSymbol[i]->index;
	}
}

extern PlaceSymbol ** ThePlaceSymbols;

StatePredicate *AtomicStatePredicate::unfold(Petrinet * llnet)
{
    AtomicStatePredicate *af = new AtomicStatePredicate(llnet,0, 0, 0);
    af->magicnumber = magicnumber;
    af->value = value;
    af->position = position;
    af->parent = NULL;

    // determine size of pos and neg entries
    int llCardPos = 0;
    int * posOffset = new int [cardPos];
    int * posSize = new int [cardPos];
    int llCardNeg = 0;
    int * negOffset = new int [cardNeg];
    int * negSize = new int [cardNeg];
    for(int i = 0; i < cardPos; i++)
    {
	hlplace * hlp = hlplace::lookup(deconst(net -> Name[PL][posPlaces[i]]));
	llCardPos += (posSize[i] = hlp -> so -> size); 
	posOffset[i] = hlp -> offset;
    }
    for(int i = 0; i < cardNeg; i++)
    {
	hlplace * hlp = hlplace::lookup(deconst(net -> Name[PL][negPlaces[i]]));
	llCardNeg += (negSize[i] = hlp -> so -> size); 
	negOffset[i] = hlp -> offset;
    }
    af->posPlaces = new arrayindex_t[llCardPos];
    af->posSymbol = new Symbol *[llCardPos];
    af->negPlaces = new arrayindex_t[llCardNeg];
    af->negSymbol = new Symbol * [llCardNeg];
    af->posMult = new capacity_t[llCardPos];
    af->negMult = new capacity_t[llCardNeg];
    af->cardPos = llCardPos;
    af->cardNeg = llCardNeg;
    af->threshold = threshold;
    af->sum = sum;
    for(int i = 0; i < cardPos; i++)
    {
	for(int j = 0; j < posSize[i]; j++)
	{
		af -> posPlaces[i] = ThePlaceSymbols[posOffset[i] + j]->getIndex();
		af -> posSymbol[i] = ThePlaceSymbols[posOffset[i] + j];
		af -> posMult[i] = posMult[i];
	}
    }
    for(int i = 0; i < cardNeg; i++)
    {
	for(int j = 0; j < negSize[i]; j++)
	{
		af -> negPlaces[i] = ThePlaceSymbols[negOffset[i] + j]->getIndex();
		af -> negSymbol[i] = ThePlaceSymbols[negOffset[i] + j];
		af -> negMult[i] = negMult[i];
	}
    }
    af->original = true;

    af->up = NULL;
    af->cardUp = 0;

    return af;
}

bool AtomicStatePredicate::containsFireable()
{
	return false;
}

bool AtomicStatePredicate::containsUnfireable()
{
	return false;
}

StatePredicate * AtomicStatePredicate::hlunfold()
{
	net = Task::GlobalHLNet;
	AtomicStatePredicate * result = new AtomicStatePredicate(Task::GlobalLLNet);
	hlplace ** hlpos = new hlplace * [cardPos];
	result -> cardPos = 0;
	for(int i = 0; i < cardPos; i++)
	{
		hlpos[i] = hlplace::lookup(deconst(net->Name[PL][posPlaces[i]]));
		result -> cardPos += hlpos[i] -> so -> size;
	}
	result -> posPlaces = new arrayindex_t [result -> cardPos];
	result -> posSymbol = new Symbol * [result -> cardPos];
	result -> posMult = new capacity_t [result -> cardPos];
	int j = 0;
	for(int i = 0; i < cardPos; i++)
	{
		for(int k = 0; k < hlpos[i]->so->size; k++)
		{
			if(ThePlaceSymbols[hlpos[i]->offset + k]->inEmptySiphon)
			{
				result -> cardPos--;
			}
			else
			{
				result -> posPlaces[j] = ThePlaceSymbols[hlpos[i]->offset + k]->getIndex();
				result -> posSymbol[j] = ThePlaceSymbols[hlpos[i]->offset + k];
				result -> posMult[j++] = posMult[i];
			}
		}
	}

	delete [] hlpos;
	hlplace ** hlneg = new hlplace * [cardNeg];
	result -> cardNeg = 0;
	for(int i = 0; i < cardNeg; i++)
	{
		hlneg[i] = hlplace::lookup(deconst(net->Name[PL][negPlaces[i]]));
		result -> cardNeg += hlneg[i] -> so -> size;
	}
	result -> negPlaces = new arrayindex_t [result -> cardNeg];
	result -> negSymbol = new Symbol * [result -> cardNeg];
	result -> negMult = new capacity_t [result -> cardNeg];
	j = 0;
	for(int i = 0; i < cardNeg; i++)
	{
		for(int k = 0; k < hlneg[i]->so->size; k++)
		{
			if(ThePlaceSymbols[hlneg[i]->offset + k]->inEmptySiphon)
			{
				result->cardNeg--;
			}
			else
			{
				result -> negPlaces[j] = ThePlaceSymbols[hlneg[i]->offset + k]->getIndex();
				result -> negSymbol[j] = ThePlaceSymbols[hlneg[i]->offset + k];
				result -> negMult[j++] = negMult[i];
			}
		}
	}

	delete [] hlneg;

	result -> threshold = threshold;
	result -> forBound = forBound;
	result -> magicnumber = magicnumber;
	if(result -> cardPos + result -> cardNeg == 0)
	{
		result -> magicnumber = (threshold >= 0) ? MAGIC_NUMBER_TRUE : MAGIC_NUMBER_FALSE;
	}
	result -> literals = 1;
	return result;
}

bool AtomicStatePredicate::createskeletonatomic(Create_HL_Net * HL)
{
	// init property field

	for(int i = 0; i < HL -> net ->cardPL + HL -> net -> cardTR;i++)
        {
                HL -> elements[i] -> property = 0;
        }

	// mark positive places
	for(int i = 0; i < cardPos; i++)
	{
		for(int j = 0; j < HL -> net -> cardPL + HL -> net -> cardTR; j++)
		{		
			if(HL->elements[j]->id == posPlaces[i])
			{
				HL -> elements[j] -> property = posMult[i];
				break;
			}
		}
	}
	// mark negative places
	for(int i = 0; i < cardNeg; i++)
	{
		for(int j = 0; j < HL -> net -> cardPL + HL -> net -> cardTR; j++)
		{		
			if(HL->elements[j]->id == negPlaces[i])
			{
				HL -> elements[j] -> property = - negMult[i];
				break;
			}
		}
	}

	// split partition
	arrayindex_t currentCardConstraints = HL -> cardConstraints;
	for (arrayindex_t i = 0; i < currentCardConstraints; i++)
	{
		HL -> initialSplit(HL -> constraints + i);
	}
	return true;
}

bool AtomicStatePredicate::createskeletonfireable(Create_HL_Net *, bool, bool)
{
	return false;
}

bool AtomicStatePredicate::createskeletonunfireable(Create_HL_Net *, bool, bool)
{
	return false;
}

StatePredicate * AtomicStatePredicate::foldformula(Create_HL_Net * HLN)
{
	int j = 0;
	for(int i = 0; i < cardPos; i++)
	{
		if(!strcmp(HLN -> originalplace[posPlaces[i]]->sy->getKey(),HLN -> placemapping[posPlaces[i]]->getKey()))
		{
			int oldindex = posPlaces[i];
			posPlaces[j] = HLN -> placemapping[oldindex]->getIndex();
			posSymbol[j] = HLN -> placemapping[oldindex];
			posMult[j++] = posMult[i];
		}
	}
	cardPos = j;

	j = 0;
	for(int i = 0; i < cardNeg; i++)
	{
		if(!strcmp(HLN -> originalplace[negPlaces[i]]->sy->getKey(),HLN -> placemapping[negPlaces[i]]->getKey()))
		{
			int oldindex = negPlaces[i];
			negPlaces[j] = HLN -> placemapping[oldindex]->getIndex();
			negSymbol[j] = HLN -> placemapping[oldindex];
			negMult[j++] = negMult[i];
		}
	}
	cardNeg = j;

	magicnumber = MagicNumber::assign();
	return this;
}

void AtomicStatePredicate::checkonlyimproving()
{
//RT::rep->status("CHECK ST AT %s", toString());
	if(cardPos == 0 && threshold >= 0) 
	{
		magicnumber = MAGIC_NUMBER_TRUE;
//RT::rep->status("MON POS");
		return;
	}
	if(cardNeg == 0 && threshold < 0) 
	{
		magicnumber = MAGIC_NUMBER_FALSE;
//RT::rep->status("MON NEG");
		return;
	}
	// determine current value of predicate
	ProtoPlace ** protopl = new ProtoPlace * [ProtoNet::currentnet -> cardPL];
	for(ProtoPlace * ppp = ProtoNet::currentnet -> firstplace; ppp; ppp = ppp -> next)
	{
		protopl[ppp->index] = ppp;
	}
	int value = 0;
	for(int i = 0; i < cardPos; i++)
	{
		value += protopl[posPlaces[i]]->marking;
	}
	for(int i = 0; i < cardNeg; i++)
	{
		value -= protopl[negPlaces[i]]->marking;
	}
	bool isvalid = false;
	if(value <= threshold) isvalid = true;

	for(ProtoTransition * ttt = ProtoNet::currentnet -> firsttransition; ttt; ttt = ttt -> next)
	{
		// determine delta of t
		int delta = 0;
		for(int i = 0; i < cardPos; i++)
		{
			Arc * aaa;
			for(aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
			{
				if(aaa -> pl == protopl[posPlaces[i]]) break;
			}
			if(aaa)
			{
				delta += aaa -> mult * posMult[i];
			}
			for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
			{
				if(aaa -> pl == protopl[posPlaces[i]]) break;
			}
			if(aaa)
			{
				delta -= aaa -> mult * posMult[i];
			}
		}
		for(int i = 0; i < cardNeg; i++)
		{
			Arc * aaa;
			for(aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
			{
				if(aaa -> pl == protopl[negPlaces[i]]) break;
			}
			if(aaa)
			{
				delta -= aaa -> mult * negMult[i];
			}
			for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
			{
				if(aaa -> pl == protopl[negPlaces[i]]) break;
			}
			if(aaa)
			{
				delta += aaa -> mult * negMult[i];
			}
		}
		if(isvalid && (delta < 0))
		{
			return;
		}
		else if((!isvalid) && (delta > 0))
		{
			return;
		}

	}
	if(isvalid)
	{
//RT::rep->status("POS DELTA");
		magicnumber = MAGIC_NUMBER_TRUE;
	}
	else
	{
//RT::rep->status("NEG DELTA");
		magicnumber = MAGIC_NUMBER_FALSE;
	}
	delete [] protopl;
	return;
}

int AtomicStatePredicate::getLargestConstant()
{
	return (threshold >= 0) ? threshold : - threshold;
}

void AtomicStatePredicate::checkCons()
{
RT::rep->status("CONS PTR %x NET %s FOR %s SUM %d", this, net -> name, toString(),sum);
	if(cardPos > net -> Card[PL]) RT::rep->status("CARD POS PLACE CORRUPT %d", cardPos);
	if(cardNeg > net -> Card[PL]) RT::rep->status("CARD NEG PLACE CORRUPT %d", cardNeg);
	for(int i = 0; i < cardPos; i++)
	{
		if(posPlaces[i] >= net -> Card[PL]) RT::rep->status("POS PLACE CORRUPT %d %d",i, posPlaces[i]);
	}
	for(int i = 0; i < cardNeg; i++)
	{
		if(negPlaces[i] >= net -> Card[PL]) RT::rep->status("NEG PLACE CORRUPT %d %d",i, negPlaces[i]);
	}
}
