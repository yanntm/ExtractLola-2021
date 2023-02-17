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
\file FireablePredicate.cc
\author Karsten
\status new

\brief class implementation for fireability state predicates
*/

#include <config.h>
#include <Core/Dimensions.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Net/NetState.h>
#include <Net/Petrinet.h>
#include <Net/Protonet.h>
#include <CoverGraph/CoverGraph.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/ParserPnml.h>
#include <Formula/FormulaInfo.h>
#include <Formula/StatePredicate/MagicNumber.h>

extern ParserPTNet * symbolTables;
extern TransitionSymbol ** TheTransitionSymbols;
extern PlaceSymbol ** ThePlaceSymbols;

FireablePredicate::FireablePredicate(Petrinet * n, arrayindex_t tt, bool ssign) :
    t(tt),sign(ssign)
{
	isatomic = false;
	isfireable = true;
         net = n;
        literals = 1;
	parent = NULL;
	trsymbol = n -> thesymbol[TR][tt];
	if(sign)
	{	
		magicnumber = MAGIC_NUMBER_FIREABLE(t);
	}
	else
	{
		magicnumber = MAGIC_NUMBER_UNFIREABLE(t);
		
	}
}

FireablePredicate::FireablePredicate(Petrinet * n, TransitionSymbol * sss, bool ssign) :
    t(sss -> getIndex()),sign(ssign)
{
	isatomic = false;
	isfireable = true;
         net = n;
        literals = 1;
	parent = NULL;
	trsymbol = sss;
	if(sign)
	{	
		magicnumber = MAGIC_NUMBER_FIREABLE(t);
	}
	else
	{
		magicnumber = MAGIC_NUMBER_UNFIREABLE(t);
		
	}
}

/*!
\brief participate in finding an upset: for deadlock: set need_enabled to true; for no_deadlock: return empty set

\param need_enabled reference parameter that signals that final up-set needs to contain an enabled transition
\return number of elements on stack
*/
arrayindex_t FireablePredicate::getUpSet(arrayindex_t * stack, bool * onstack, bool *need_enabled) 
{
    * need_enabled = false;
    if (sign) // fireable
    {
	if(onstack[t])
	{
		return 0; // no need to add element
	}
	else
	{
		onstack[t] = true;
		stack[0] = t;
		return 1;  // one element added
	}
    }
    else // not fireable
    {
	arrayindex_t stackpointer = 0;
	for(arrayindex_t ttt = 0; ttt < net->TrCardDecreasing[t];++ttt)
	{
		arrayindex_t element = net->TrDecreasing[t][ttt];
		if(!onstack[element])
		{
			onstack[element] = true;
			stack[stackpointer++] = element;
		}
	}
	return stackpointer;
    }
}

/*!
If value of this changes, it needs to be propagated to its parent. The
parameter is the change in the formal sum k_1 p_1 + ... + k_n p_n between the
previously considered marking and the current marking. Having a precomputed
value for this change, evaluation of the formula is accelerated.
*/
void FireablePredicate::update(NetState &ns)
{
    bool newvalue = (sign ? ns.Enabled[t] : !ns.Enabled[t] );
    if (parent)
    {
        if (newvalue && ! value)
        {
            value = newvalue;
            parent->updateFT(position);
            return;
        }
        if (!newvalue && value)
        {
	    value = newvalue;
            parent->updateTF(position);
            return;
        }
    }
    value = newvalue;
}

/*!
Evaluation starts top/down, so the whole formula is examined. Evaluation is
done w.r.t. ns.Current.

\param ns  net state to evaluate the formula
*/
void FireablePredicate::evaluate(NetState &ns)
{
    value = sign ? ns.Enabled[t]  : ! ns.Enabled[t];
}

/*!
Evaluation with Omega starts top/down, so the whole formula is examined. Evaluation is
done w.r.t. ns.Current.

\param ns  net state to evaluate the formula
*/
void FireablePredicate::evaluateOmega(NetState &ns)
{
    unknown = false;
    value = sign ? ns.Enabled[t]  : ! ns.Enabled[t];
//    if (ns.CardEnabled != 0)
//    {
//        for (arrayindex_t i = 0; i < net->CardPre[TR][t]; i++)
//        {
//            if (ns.Current[net->Pre[TR][t][i]] == OMEGA)
//            {
//                unknown = true;
//            }
//        }
//    }
}

arrayindex_t FireablePredicate::countAtomic() 
{
    return 0;
}

arrayindex_t FireablePredicate::collectAtomic(AtomicStatePredicate **)
{
    return 0;
}

arrayindex_t FireablePredicate::countDeadlock() 
{
    return 0;
}

arrayindex_t FireablePredicate::collectDeadlock(DeadlockPredicate **c)
{
    return 0;
}

arrayindex_t FireablePredicate::countFireable() 
{
    return 1;
}

arrayindex_t FireablePredicate::collectFireable(FireablePredicate **c)
{
    c[0] = this;
    return 1;
}

// LCOV_EXCL_START
bool FireablePredicate::DEBUG__consistency(NetState &)
{
    return true;
}
// LCOV_EXCL_STOP

/*!
\param parent  the parent predicate for the new, copied, object
*/
 StatePredicate *FireablePredicate::copy(StatePredicate *parent)
{
    FireablePredicate *af = new FireablePredicate(net,(TransitionSymbol *) trsymbol,sign);
    af->value = value;
    af->position = position;
    af->parent = parent;
    af->magicnumber = magicnumber;
    af -> t = t;
    return af;
}

arrayindex_t FireablePredicate::getSubs( StatePredicate * **) 
{
    return 0;
}

StatePredicate *FireablePredicate::negate()
{
    FireablePredicate *af = new FireablePredicate(net,(TransitionSymbol *) trsymbol,!sign);
    af -> magicnumber = - magicnumber;
    af -> t =  t;
    return af;
}

FormulaInfo *FireablePredicate::getInfo() 
{
    FormulaInfo *Info = new FormulaInfo();
    if (sign)
    {
        Info->tag = formula_fireable;
    }
    else
    {
        Info->tag = formula_unfireable;
    }
    Info->cardChildren = 1;
    Info->f = NULL;
    return Info;
}

int FireablePredicate::countSubFormulas() 
{
    return 0;
}

char * FireablePredicate::toString()
{
	char * result;
	char * name = deconst(trsymbol->getKey());
	if(sign)
	{
		result = (char *) malloc((11+strlen(name)) * sizeof(char));
		sprintf(result,"FIREABLE(%s)",name);
		return result;
	}
	else
	{
		result = (char *) malloc((15+strlen(name)) * sizeof(char));
		sprintf(result,"NOT FIREABLE(%s)",name);
		return result;
	}
}

void FireablePredicate::setVisible(bool * vis)
{
	for(arrayindex_t i = 0; i < net->TrCardDecreasing[t];i++)
	{
		vis[net->TrDecreasing[t][i]] = true;
	}
	for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][t];i++)
	{
		arrayindex_t p = net->Arc[TR][PRE][t][i];
		for(arrayindex_t j = 0; j < net->PlCardIncreasing[p];j++)
		{
			vis[net->PlIncreasing[p][j]] = true;
		}
	}
}

void FireablePredicate::setDown(bool * dwn)
{
	if(sign)
	{
		for(arrayindex_t i = 0; i < net->TrCardDecreasing[t];i++)
		{
				dwn[net->TrDecreasing[t][i]] = true;
		}
		return;
	}
	for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][t];i++)
	{
		arrayindex_t p = net->Arc[TR][PRE][t][i];
		for(arrayindex_t j = 0; j < net->PlCardIncreasing[p];j++)
		{
			dwn[net->PlIncreasing[p][j]] = true;
		}
	}
}

void FireablePredicate::setNondestroying(bool * nd)
{
	if(sign)
	{
		for(arrayindex_t i = 0; i < net->TrCardDecreasing[t];i++)
		{
				nd[net->TrDecreasing[t][i]] = true;
		}
		return;
	}
	for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][t];i++)
	{
		arrayindex_t p = net->Arc[TR][PRE][t][i];
		for(arrayindex_t j = 0; j < net->PlCardIncreasing[p];j++)
		{
			nd[net->PlIncreasing[p][j]] = true;
		}
	}
}

AtomicBooleanPredicate * FireablePredicate::DNF()
{
	if(sign)
	{
		// FIREABLE: result is conjunction
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(net,true);
		for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][t];i++)
		{
			arrayindex_t p = net->Arc[TR][PRE][t][i];
			arrayindex_t m = net->Mult[TR][PRE][t][i];
			AtomicStatePredicate * A = new AtomicStatePredicate(net,0,1,-m); // 0 pos places, 1 neg place, threshold -m
			A -> negPlaces[0] = p;
			A -> negMult[0] = 1;  // (-1)p <= -m => p >= m
			if(m == 1)
			{
				A->magicnumber = MAGIC_NUMBER_MARKED(p);
			}
			else
			{
				A->magicnumber = MagicNumber::assign();
			}
			result -> addSub(A);
		}
		result -> magicnumber = magicnumber;
		return result;
	}
	else
	{
		// UNFIREABLE: result is disjunction
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(net,false);
		for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][t];i++)
		{
			AtomicBooleanPredicate * rr = new AtomicBooleanPredicate(net,true);
			arrayindex_t p = net->Arc[TR][PRE][t][i];
			arrayindex_t m = net->Mult[TR][PRE][t][i];
			AtomicStatePredicate * A = new AtomicStatePredicate(net,1,0,m-1); // 1 pos places, 0 neg place, threshold m-1
			A -> posPlaces[0] = p;
			A -> posMult[0] = 1;  // p <= m-1 => p < m
			if(m == 1)
			{
				A->magicnumber = MAGIC_NUMBER_EMPTY(p);
			}
			else
			{
				A->magicnumber = MagicNumber::assign();
			}
			rr -> addSub(A);
			rr -> magicnumber = A->magicnumber;
			result -> addSub(rr);
			result -> magicnumber = rr -> magicnumber;
		}
		result -> magicnumber = magicnumber;
		return result;
	}
}

FormulaStatistics * FireablePredicate::count(FormulaStatistics * fs)
{
	if(sign)
	{
		fs -> fir++;
	}
	else
	{
		fs -> unfir++;
	}
	if(!fs->mentioned_transition)
	{
		fs->mentioned_transition = new bool [net->Card[TR]];
		memset(fs->mentioned_transition,0,sizeof(bool) * net->Card[TR]);
	}
	if(!fs->mentioned_transition[t])
	{
		fs->mentioned_transition[t] = true;
		fs->visible_transitions++;
	}
	fs->transition_references++;
	return fs;
}

void FireablePredicate::markvisible()
{
	trsymbol->visible = ProtoNet::currentvisibility; 
}

void FireablePredicate::markslicing(bool parity)
{
	ProtoTransition * ttt;
	for(ttt = ProtoNet::currentnet -> firsttransition; ttt; ttt = ttt -> next)
	{
		if(!strcmp(ttt->sy->getKey(), trsymbol -> getKey())) break;
	}
	assert(ttt);
	for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr) 
	{
		if(parity)
		{
			if(sign)
			{
				((PlaceSymbol *) (aaa->pl->sy)) -> occurspositive = true;
			}
			else
			{
				((PlaceSymbol *) (aaa->pl->sy)) -> occursnegative = true;
			}
		}
		else
		{
			if(sign)
			{
				((PlaceSymbol *) (aaa->pl->sy)) -> occursnegative = true;
			}
			else
			{
				((PlaceSymbol *) (aaa->pl->sy)) -> occurspositive = true;
			}
		}
	}
}

void FireablePredicate::relabel()
{
	net = ParserPTNet::relabelnet;
	t = trsymbol -> index;
}
extern ParserPTNet *HLsymbolTables;
class hltransition;
StatePredicate * FireablePredicate::unfold(Petrinet * llnet)
{
	hltransition * hlt = hltransition::lookup(deconst(net -> Name[TR][t]));
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(llnet, !sign); // fir -> disj, unfir -> conj
	result -> cardSub = hlt -> cardfiringmodes;
	result -> cardSat = 0;
	result -> literals = 0;
	result -> value = false;
	result -> position = 0;
	result -> parent = NULL;
	result -> magicnumber = magicnumber;
	result -> sub = reinterpret_cast<StatePredicate **>(malloc(SIZEOF_VOIDP*(result ->cardSub)));
	for (arrayindex_t i = 0; i < result -> cardSub; i++)
        {
		
		FireablePredicate * fff = new FireablePredicate(llnet,TheTransitionSymbols[hlt->offset + i]->getIndex(),sign);
		result->sub[i] = fff;
		fff->position = i;
		fff->parent = this;
		fff->magicnumber = sign? MAGIC_NUMBER_FIREABLE(fff->t) : MAGIC_NUMBER_UNFIREABLE(t);
		if(result->cardSub == 1) magicnumber = fff -> magicnumber;
		result->literals += fff -> literals;
        }
	return result;
}

bool FireablePredicate::containsFireable()
{
	return sign;
}

bool FireablePredicate::containsUnfireable()
{
	return !sign;
}

StatePredicate * FireablePredicate::hlunfold()
{
	// fireable = disjunction, unfireable = conjunction
	net = Task::GlobalHLNet;
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Task::GlobalLLNet,!sign);
	hltransition * hlt = hltransition::lookup(deconst(net->Name[TR][t]));
	result -> sub = new StatePredicate * [hlt -> cardfiringmodes];
	result -> cardSub = hlt -> cardfiringmodes;
	int j = 0;
	for(int i = 0; j < result -> cardSub; i++) // j in snd part is intended!!!
	{
		if(TheTransitionSymbols[hlt->offset + i]->inEmptySiphon)
		{
			// transition instance is not fireable due to
			// permanently empty pre-place
			// --> do not insert
			result -> cardSub--;
			
		}
		else
		{
			FireablePredicate * fp;
			result -> sub[j++] = fp = new FireablePredicate(Task::GlobalLLNet,TheTransitionSymbols[hlt->offset + i]->getIndex(), sign);
			fp -> trsymbol = TheTransitionSymbols[hlt->offset + i];
		}
	}
	result -> literals = result -> cardSub;
	result -> magicnumber = magicnumber;
	if(result -> cardSub == 0) result -> magicnumber = sign ? MAGIC_NUMBER_FALSE : MAGIC_NUMBER_TRUE; // no instance fireable
	if(result -> cardSub == 1) result -> magicnumber = result -> sub[0]->magicnumber; 

	return result;
}

bool FireablePredicate::createskeletonatomic(Create_HL_Net * HL)
{
	return false;
}

bool FireablePredicate::createskeletonfireable(Create_HL_Net * HL,bool parentshape,bool parentisroot)
{
	if(sign)
	{
		if(parentshape)
		{
		        // init property field
			for(int i = 0; i < HL -> net ->cardPL + HL -> net -> cardTR;i++)
			{
				HL -> elements[i] -> property = 0;
			}
		}
		// fill property field
		for(int i = 0; i < HL -> net ->cardPL + HL -> net -> cardTR;i++)
		{
			if(HL->elements[i] -> id == t + HL -> net -> cardPL)
			{
				HL -> elements[i] -> property = 1;
				break;
			}
		}

		if(parentshape)
		{
			// split according to finished marking
                        arrayindex_t currentCardConstraints = HL -> cardConstraints;
                        for (arrayindex_t i = 0; i < currentCardConstraints; i++)
                        {
                                HL -> initialSplit(HL -> constraints + i);
                        }

		}
		return true;
	}
	return false;
}

bool FireablePredicate::createskeletonunfireable(Create_HL_Net * HL,bool parentshape,bool parentisroot)
{
	if(!sign)
	{
		if(parentshape)
		{
		        // init property field
			for(int i = 0; i < HL -> net ->cardPL + HL -> net -> cardTR;i++)
			{
				HL -> elements[i] -> property = 0;
			}
		}
		// fill property field
		for(int i = 0; i < HL -> net ->cardPL + HL -> net -> cardTR;i++)
		{
			if(HL->elements[i] -> id == t + HL -> net -> cardPL)
			{
				HL -> elements[i] -> property = 1;
				break;
			}
		}
		if(parentshape)
		{
			// split according to finished marking
                        arrayindex_t currentCardConstraints = HL -> cardConstraints;
                        for (arrayindex_t i = 0; i < currentCardConstraints; i++)
                        {
                                HL -> initialSplit(HL -> constraints + i);
                        }

		}
		return true;
	}
	return false;
}

StatePredicate * FireablePredicate::foldformula(Create_HL_Net * HLN)
{
	trsymbol =  HLN -> transitionmapping[t];
	t = HLN -> transitionmapping[t]->getIndex();
	magicnumber = sign ? MAGIC_NUMBER_FIREABLE(t) : MAGIC_NUMBER_UNFIREABLE(t);
	return this;
}

int FireablePredicate::getLargestConstant()
{
	int result = 0;
	for(int i = 0; i < net -> CardArcs[TR][PRE][t]; i++)
	{
		int result1 = net -> Mult[TR][PRE][t][i];
		if(result1 > result) result = result1;
	}
	return result;
}

void FireablePredicate::checkCons()
{
	if(net && t >= net -> Card[TR]) RT::rep->status("TRANSITION %d CORRUPT", t);
}

int FireablePredicate::checkstable(bool lpsign, lprec * lp)
{
//RT::rep->status("ENTER CHECK FOR TRANSITION %s LPSIGN %d SIGN %d", trsymbol->getKey(), lpsign, sign);
	// avoid singleton test
	if(!lp) return 0;

	// avoid lp if parent is disjunction
	if(parent && !(((AtomicBooleanPredicate*) parent)->isAnd))
	{
		return 0;
	}

	ProtoNet * nnn = ProtoNet::currentnet;
	ProtoTransition * ttt;
	for(ttt = nnn -> firsttransition; ttt; ttt = ttt -> next)
	{
		if(!strcmp(ttt -> sy -> getKey(), trsymbol -> getKey())) break;
	}
	if(!ttt) return 0;
	int cardequations = 0;
//RT::rep->status("REALLY WANT TO CHECK");

	if((lpsign && sign) || (!lpsign && !sign))
	{
//RT::rep->status("NEC FOR FIR");
		// nec cond. for fireability of t
		int Ncol, ret = 0;
		Ncol = nnn->cardPL + nnn->cardTR;
		REAL * row = new REAL[Ncol + 2];
		if(row == NULL)
		{
			return 0;
		}
		for(Arc * aaa = ttt -> firstArcPre; aaa ; aaa = aaa -> next_tr)
		{
//RT::rep->status("CONSIDER ARC FROM PLACE %s AT %d", aaa -> pl -> sy -> getKey(), aaa -> pl -> index);
			std::fill(row, row + Ncol + 1, 0);
			row[aaa -> pl -> index +1] = 1.0;
			if(!add_constraint(lp,row,GE,aaa->mult)) 
			{
				delete [] row;
				return cardequations;
			}
			cardequations++; 
		}
		delete [] row;
//RT::rep->status("DONE MN %d", magicnumber);
		return cardequations;
	}
	else
	{
//RT::rep->status("NEC FOR UNFIR");
		// nec cond for unfireability of t
		
		// Possibility 1: t has one one pre-place
		if(ttt -> firstArcPre && ! (ttt -> firstArcPre -> next_tr))
		{
//RT::rep->status("SINGLE PRE %s AT %d", ttt -> firstArcPre->pl -> sy -> getKey(), ttt -> firstArcPre -> pl ->index);
			int Ncol, ret = 0;
			Ncol = nnn->cardPL + nnn->cardTR;
			REAL * row = new REAL[Ncol + 2];
			if(row == NULL)
			{
				if(row != NULL) delete [] row;
				return 0;
			}
			std::fill(row, row + Ncol + 1, 0);
			row[ttt -> firstArcPre -> pl -> index + 1] = 1.0;
			if(!add_constraint(lp,row,LE,ttt->firstArcPre->mult - 1)) 
			{
				delete [] row;
				return 0;
			}
			return 1;
		}

		// Possibility 2: N is safe 
		if(RT::args.safe_given && ! nnn -> isSkeleton)
		{
//RT::rep->status("SAVE NET");
			int Ncol, ret = 0;
			Ncol = nnn->cardPL + nnn->cardTR;
			REAL * row = new REAL[Ncol + 2];
			if(row == NULL)
			{
				if(row != NULL) delete [] row;
				return 0;
			}
			std::fill(row, row + Ncol + 1, 0);
			int c = 0;
			for(Arc * aaa = ttt -> firstArcPre; aaa ; aaa = aaa -> next_tr)
			{
				if(aaa->mult != 1)
				{
					delete [] row;
					return 0;
				}
				row[aaa->pl->index + 1] = 1.0;
				c++;
			}
			if(!add_constraint(lp,row,LE,c - 1)) 
			{
				delete [] row;
				return 0;
			}
			return 1;
			
		}
	}
	
	return 0;
	
}
