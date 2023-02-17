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

\brief This file provides data structures for a preliminary
Petri net format that is generated directly from the parsers
and used for net reduction.

*/



#include <Net/Protonet.h>
#include <Net/Petrinet.h>
#include <Core/Runtime.h>
#include <Frontend/Parser/ParserPnml.h>
#include "../../libs/lp_solve_5.5/lp_lib.h"

ProtoNet * ProtoNet::currentnet = NULL;

int ProtoNet::currentvisibility = 0;


ProtoPlace::ProtoPlace(ProtoNet * N, PlaceSymbol * S, capacity_t C, capacity_t M)
{
	net = N;
	visible = false;
	sy = S;
	capacity = C;
	marking = M;
	cardPre = cardPost = 0;
	firstArcPre = lastArcPre = firstArcPost = lastArcPost = NULL;
	if(N -> firstplace)
	{
		assert(N -> lastplace);
		prev = N -> lastplace;
		next = NULL;
		N -> lastplace -> next = this;
		N -> lastplace = this; 
	}
	else
	{
		assert(!(N->lastplace));
		prev = next = NULL;
		N -> firstplace = N -> lastplace = this;
	}
	sortedPre = sortedPost = false;
	(net -> cardPL)++;
	
}

ProtoPlace::ProtoPlace(ProtoPlace & P)
{
	net = P.net;
	sy = P.sy;
	index = P.index;
	capacity = P.capacity;
	marking = P.marking;
	cardPre = cardPost = 0;
	firstArcPre = lastArcPre = firstArcPost = lastArcPost = NULL;
	for(Arc * a = P.firstArcPre; a; a = a -> next_pl)
	{
		new Arc(this, a -> tr,a -> mult, TP);
	}
	for(Arc * a = P.firstArcPost; a; a = a -> next_pl)
	{
		new Arc(this, a -> tr, a -> mult, PT);
	}
	if(net -> firstplace)
	{
		assert(net -> lastplace);
		prev = net -> lastplace;
		next = NULL;
		net -> lastplace -> next = this;
		net -> lastplace = this; 
	}
	else
	{
		assert(!(net->lastplace));
		prev = next = NULL;
		net -> firstplace = net -> lastplace = this;
	}
	sortedPre = sortedPost = false;
	(net->cardPL)++;
	
}

ProtoPlace::~ProtoPlace()
{
	while(firstArcPre) delete firstArcPre;
	while(firstArcPost) delete firstArcPost;
	if(prev)
	{
		prev -> next = next;
	}
	else
	{
		net -> firstplace = next;
	}
	if(next)
	{
		next -> prev = prev;
	}
	else
	{
		net -> lastplace = prev;
	}
	(net -> cardPL)--;
}


ProtoTransition::ProtoTransition(ProtoNet * N, TransitionSymbol * S)
{
	net = N;
	sy = S;
	index = S -> getIndex();
	cardPre = cardPost = 0;
	firstArcPre = lastArcPre = firstArcPost = lastArcPost = NULL;
	if(N -> firsttransition)
	{
		assert(N -> lasttransition);
		prev = N -> lasttransition;
		next = NULL;
		N -> lasttransition -> next = this;
		N -> lasttransition = this; 
	}
	else
	{
		assert(!(N->lasttransition));
		prev = next = NULL;
		N -> firsttransition = N -> lasttransition = this;
	}
	sortedPre = sortedPost = false;
	(net -> cardTR)++;
	
}

ProtoTransition::ProtoTransition(ProtoTransition & T)
{
	net = T.net;
	sy = T.sy;
	cardPre = cardPost = 0;
	firstArcPre = lastArcPre = firstArcPost = lastArcPost = NULL;
	for(Arc * a = T.firstArcPre; a; a = a -> next_tr)
	{
		new Arc(a -> pl, this, a -> mult, PT);
	}
	for(Arc * a = T.firstArcPost; a; a = a -> next_tr)
	{
		new Arc(a -> pl, this,a -> mult, TP);
	}
	if(net -> firsttransition)
	{
		assert(net -> lasttransition);
		prev = net -> lasttransition;
		next = NULL;
		net -> lasttransition -> next = this;
		net -> lasttransition = this; 
	}
	else
	{
		assert(!(net->lasttransition));
		prev = next = NULL;
		net -> firsttransition = net -> lasttransition = this;
	}
	sortedPre = sortedPost = false;
	(net -> cardTR)++;
	
}

ProtoTransition::~ProtoTransition()
{
	while(firstArcPre) delete firstArcPre;
	while(firstArcPost) delete firstArcPost;
	if(prev)
	{
		prev -> next = next;
	}
	else
	{
		net -> firsttransition = next;
	}
	if(next)
	{
		next -> prev = prev;
	}
	else
	{
		net -> lasttransition = prev;
	}
	(net -> cardTR)--;
}

Arc::Arc(ProtoPlace * P, ProtoTransition * T, mult_t W, bool D)
{
	pl = P;
 	tr = T;
	mult = W;
	toplace = D;
	if(D)
	{
		// direction TP
		
		// link in pre arc list of place
		
		if(pl -> lastArcPre)
		{
			prev_pl = pl -> lastArcPre;
			next_pl = NULL;
			pl -> lastArcPre -> next_pl = this;
			pl -> lastArcPre = this;
		}
		else
		{
			pl -> firstArcPre = pl -> lastArcPre = this;
			prev_pl = next_pl = NULL;
		}
		pl -> sortedPre = false;
		(pl -> cardPre)++;
		
		// link in post arc list of transition

		if(tr -> lastArcPost)
		{
			prev_tr = tr -> lastArcPost;
			next_tr = NULL;
			tr -> lastArcPost -> next_tr = this;
			tr -> lastArcPost = this;
		}
		else
		{
			tr -> firstArcPost = tr -> lastArcPost = this;
			prev_tr = next_tr = NULL;
		}
		tr -> sortedPost = false;
		(tr -> cardPost)++;
	}
	else
	{
		// direction PT
		
		// link in post arc list of place
		
		if(pl -> lastArcPost)
		{
			prev_pl = pl -> lastArcPost;
			next_pl = NULL;
			pl -> lastArcPost -> next_pl = this;
			pl -> lastArcPost = this;
		}
		else
		{
			pl -> firstArcPost = pl -> lastArcPost = this;
			prev_pl = next_pl = NULL;
		}
		pl -> sortedPost = false;
		(pl -> cardPost)++;
		
		// link in pre arc list of transition

		if(tr -> lastArcPre)
		{
			prev_tr = tr -> lastArcPre;
			next_tr = NULL;
			tr -> lastArcPre -> next_tr = this;
			tr -> lastArcPre = this;
		}
		else
		{
			tr -> firstArcPre = tr -> lastArcPre = this;
			prev_tr = next_tr = NULL;
		}
		tr -> sortedPre = false;
		(tr->cardPre++);
	}
}

Arc::~Arc()
{
	// unlink in list of place

	if(prev_pl)
	{
		prev_pl -> next_pl = next_pl;
	}
	else
	{
		if(toplace)
		{	
			// direction TP
			pl -> firstArcPre = next_pl;
		}
		else
		{
			// direction PT
			pl -> firstArcPost = next_pl;
		}
	}
	if(next_pl)
	{
		next_pl -> prev_pl = prev_pl;
	}
	else
	{
		if(toplace)
		{
			// direction TP
			pl -> lastArcPre = prev_pl;
		}
		else
		{
			// direction PT
			pl -> lastArcPost = prev_pl;
		}
	}
	if(toplace)
	{
		(pl -> cardPre)--;
	}
	else
	{
		(pl -> cardPost)--;
	}

	// unlink in list of transition

	if(prev_tr)
	{
		prev_tr -> next_tr = next_tr;
	}
	else
	{
		if(toplace)
		{
			// direction TP
			tr -> firstArcPost = next_tr;
		}
		else
		{
			tr -> firstArcPre = next_tr;
		}
	}
	if(next_tr)
	{
		next_tr -> prev_tr = prev_tr;
	}
	else
	{
		if(toplace)
		{
			// direction TP
			tr -> lastArcPost = prev_tr;
		}
		else
		{
			tr -> lastArcPre = prev_tr;
		}
	}
	if(toplace)
	{
		(tr -> cardPost)--;
	}
	else
	{
		(tr -> cardPre)--;
	}
}

ProtoNet::ProtoNet()
{
	lp = NULL;
	firstplace = lastplace = NULL;
	firsttransition = lasttransition = NULL;
	cardPL = cardTR = 0;
	data = NULL;
}

ProtoNet::ProtoNet(ProtoNet & N)
{
	std::map<ProtoPlace *, ProtoPlace *> mmm; // association p in old net --> p in new net
	firstplace = lastplace = NULL;
	firsttransition = lasttransition = NULL;
	cardPL = cardTR = 0;

	for(ProtoPlace * ppp = N.firstplace; ppp; ppp = ppp -> next)
	{
		ProtoPlace * newppp = new ProtoPlace(this, ppp->sy, ppp->capacity, ppp -> marking);
		newppp -> index = ppp -> index;
		mmm[ppp] = newppp;
	}
	for(ProtoTransition * ttt = N.firsttransition; ttt; ttt = ttt -> next)
	{
		ProtoTransition * newttt = new ProtoTransition(this,ttt->sy);
		newttt -> index = ttt -> index;
		for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
		{
			new Arc(mmm[aaa -> pl],newttt, aaa->mult, PT); 
		}
		for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			new Arc(mmm[aaa -> pl],newttt, aaa->mult, TP); 
		}
	}
}

ProtoNet::~ProtoNet()
{
	while(firstplace) delete firstplace;
	while(firsttransition) delete firsttransition;
}

void sortpl(Arc ** aaa, int card)
{
	std::less<ProtoTransition * > lessthan = std::less<ProtoTransition *>(); 
	int b = 0;
	int w = 1;
	int r = card;
	ProtoTransition * pivot = aaa[0]->tr;
	while(w < r)
	{
		if(lessthan(aaa[w]->tr,pivot))
		{
			Arc * tmp = aaa[w];
			aaa[w++] = aaa[b];
			aaa[b++] = tmp;
			continue;
		}
		if(lessthan(pivot,aaa[w]->tr))
		{
			Arc * tmp = aaa[w];
			aaa[w] = aaa[--r];
			aaa[r] = tmp;
			continue;
		}
		w++;
	}
	if(b > 1) sortpl(aaa,b);
	if(card - r > 1) sortpl(aaa+w,card - r);
}


void ProtoNet::sortArcsPl(Arc ** first, Arc ** last, int card)
{
	if(card < 2) return;
	Arc ** arcarray = new Arc * [card];
	int i = 0;
	for(Arc * a = * first; a ; a = a -> next_pl)
	{
		arcarray[i++] = a;
	}
	sortpl(arcarray,card);
	*first = arcarray[0];
	*last = arcarray[card-1];
	arcarray[0]->prev_pl = NULL;
	arcarray[card-1]->next_pl = NULL;
	for(int i = 0; i < card-1;i++)
	{
		arcarray[i]->next_pl = arcarray[i+1];
		arcarray[i+1] -> prev_pl = arcarray[i];
	}
	delete [] arcarray;
}

void sorttr(Arc ** aaa, int card)
{
	std::less<ProtoPlace * > lessthan = std::less<ProtoPlace *>(); 
	int b = 0;
	int w = 1;
	int r = card;
	ProtoPlace * pivot = aaa[0]->pl;
	while(w < r)
	{
		if(lessthan(aaa[w]->pl,pivot))
		{
			Arc * tmp = aaa[w];
			aaa[w++] = aaa[b];
			aaa[b++] = tmp;
			continue;
		}
		if(lessthan(pivot,aaa[w]->pl))
		{
			Arc * tmp = aaa[w];
			aaa[w] = aaa[--r];
			aaa[r] = tmp;
			continue;
		}
		w++;
	}
	if(b > 1) sorttr(aaa,b);
	if(card - r > 1) sorttr(aaa+w,card - r);
}

void ProtoNet::sortArcsTr(Arc ** first, Arc ** last, int card)
{
	if(card < 2) return;
	Arc ** arcarray = new Arc * [card];
	int i = 0;
	for(Arc * a = * first; a ; a = a -> next_tr)
	{
		arcarray[i++] = a;
	}
	sorttr(arcarray,card);
	*first = arcarray[0];
	*last = arcarray[card-1];
	arcarray[0]->prev_tr = NULL;
	arcarray[card-1]->next_tr = NULL;
	for(int i = 0; i < card-1;i++)
	{
		arcarray[i]->next_tr = arcarray[i+1];
		arcarray[i+1] -> prev_tr = arcarray[i];
	}
	delete [] arcarray;
}

extern bool HighLevelNet;

int ProtoNet::applyAgglomeration(bool containsnext, bool containsdeadlock, formula_t type)
{
	// check Pre-, Post-, and free agglomeration simultaneously
	if(RT::args.check_arg == check_arg_modelchecking && type != FORMULA_REACHABLE && type != FORMULA_INVARIANT && type != FORMULA_DEADLOCK && type != FORMULA_NODEADLOCK)
	{
		return 0;
	}

	// disclaimers for global properties

	// HL: not supported yet except DL
	if(RT::args.check_arg != check_arg_modelchecking && RT::args.check_arg != check_arg_deadlockfreedom && HighLevelNet)
	{
		return 0;
	}
	// QL, LV, DL: ok
	// OS: not applicable
	if(RT::args.check_arg == check_arg_OneSafe)
	{
		return 0;
	}
	// SM: not applicable
	if(RT::args.check_arg == check_arg_StableMarking)
	{
		return 0;
	}

	std::less<ProtoTransition * > lessthan = std::less<ProtoTransition *>(); 
	std::less<ProtoPlace * > lessthanp = std::less<ProtoPlace *>(); 

	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	int cardinsertedtransitions = 0;
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		if(ppp -> visible) 
		{

			ppp = ppp -> next;
			continue;
		}
		if(ppp -> marking) 
		{
			ppp = ppp -> next;
			continue;
		}
		if(ppp -> cardPre * ppp -> cardPost > 32) 
		{
			ppp = ppp -> next;
			continue; // avoid blowup of transitions
		}
		// check disjoint Pre and Post
		sortArcsPl(&(ppp -> firstArcPre), &(ppp -> lastArcPre), ppp -> cardPre);
		sortArcsPl(&(ppp -> firstArcPost), &(ppp -> lastArcPost), ppp -> cardPost);
		Arc * aaa = ppp -> firstArcPre;
		Arc * bbb = ppp -> firstArcPost;
		while(aaa && bbb)
		{
			if(lessthan(aaa -> tr, bbb -> tr)) 
			{
				aaa = aaa -> next_pl;
				continue;
			}
			if(lessthan(bbb -> tr, aaa -> tr))
			{
				bbb = bbb -> next_pl;
				continue;
			}
			break;
		}
		if(aaa && bbb)
		{
			ppp = ppp -> next;
			continue;
		}
		bool postagg = true; // post-agglomeration is possible
		bool preagg = true; // pre-agglomeration is possible
		bool freeagg = containsdeadlock ? false : true; // free-agglomeration is possible
		if(RT::args.check_arg == check_arg_Liveness) freeagg = false;
		bool havevisible = false;
		for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
		{
			if(aaa -> tr -> visible)
			{
				havevisible = true;
				break;
			}
		}
		if(havevisible)
		{
			ppp = ppp -> next;
			break;
		}
		havevisible = false;
		for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
		{
			if(aaa -> tr -> visible)
			{
				havevisible = true;
				break;
			}
		}
		if(havevisible)
		{
			ppp = ppp -> next;
			break;
		}
		if(freeagg || preagg)
		{
			// check for stuttering feeders 
			for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
			{
				ProtoTransition * ttt = aaa -> tr;
				if(ttt -> cardPost != 1)
				{
					freeagg = preagg = false;
					break;
				}
				if(ttt -> firstArcPost -> mult != 1)
				{
					freeagg = preagg = false;
					break;
				}
				if(!freeagg && !preagg)
				{
					break;
				}
				for(Arc * bbb = ttt -> firstArcPre; bbb; bbb = bbb -> next_tr)
				{
					if(bbb -> pl -> visible)
					{
						freeagg = preagg = false;
						break;
					}
					if(preagg && bbb -> pl -> cardPost != 1)
					{
						preagg = false;
						if(!freeagg) break;
					}
				}
				if(!freeagg && !preagg)
				{
					break;
				}
				for(Arc * bbb = ttt -> firstArcPost; bbb; bbb = bbb -> next_tr)
				{
					if(bbb -> pl -> visible)
					{
						freeagg = preagg = false;
						break;
					}
				}
				if(!freeagg && !preagg)
				{
					break;
				}
			}
			if(!freeagg && !preagg) break;
			for(Arc * aaa = ppp -> firstArcPost; aaa ; aaa = aaa -> next_pl)
			{
				if(aaa -> mult != 1)
				{
					freeagg = preagg = false;
					break;
				}
			}
			if(!freeagg && !preagg) break;

		}
		if(postagg && !preagg && !freeagg)
		{
			// need to check for postagg only if the other aggregations are not applicable
			for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
			{
				ProtoTransition * ttt = aaa -> tr;
				if(ttt -> cardPre != 1)
				{
					postagg = false;
					break;
				}
				if(aaa -> mult != 1) // this is sufficient (but stricter than necessary) for natural ratio between pre and post
				{
					postagg = false;
					break;
				}
				if(!postagg) break;
				for(Arc * bbb = ttt -> firstArcPre;bbb;bbb = bbb -> next_tr)
				{
					if(bbb -> pl -> visible)
					{
						postagg = false;
						break;
					}
				}
				if(!postagg) break;
				for(Arc * bbb = ttt -> firstArcPost;bbb;bbb = bbb -> next_tr)
				{
					if(bbb -> pl -> visible)
					{
						postagg = false;
						break;
					}
				}
		
			}
			if(!postagg) break;
		}
		if(!preagg && !postagg && !freeagg)
		{
			ppp = ppp -> next;
			continue;
		}
		if(!preagg && !postagg && (RT::args.check_arg == check_arg_Liveness))
		{
			ppp = ppp -> next;
			continue;
		}


		// here: agglomeration is indeed applicable
//RT::rep->status("AGG AT PL %s FREE: %d PRE %d POST %d", ppp -> sy -> getKey(), freeagg, preagg,postagg);

		for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
		{
			// disclaimers for global properties

			ProtoTransition * ttt = aaa -> tr;
			for(Arc * bbb = ppp -> firstArcPost; bbb; bbb = bbb -> next_pl)
			{
				ProtoTransition * uuu = bbb -> tr;
				
				char * name = (char *) malloc(sizeof(char) * (strlen(ttt -> sy -> getKey())+strlen(uuu->sy->getKey()) +2));
				strcpy(name,ttt->sy->getKey());
				strcpy(name + strlen(name), "_");
				strcpy(name + strlen(name),uuu->sy->getKey());
				TransitionSymbol * newsymbol = new TransitionSymbol(name, NO_FAIRNESS, NULL, NULL);
				ProtoTransition * vvv = new ProtoTransition(this, newsymbol);
				vvv -> visible = false;
				cardinsertedtransitions++;
				sortArcsTr(&(ttt -> firstArcPre), &(ttt -> lastArcPre), ttt -> cardPre);
				sortArcsTr(&(uuu -> firstArcPre), &(uuu -> lastArcPre), uuu -> cardPre);
				sortArcsTr(&(ttt -> firstArcPost), &(ttt -> lastArcPost), ttt -> cardPost);
				sortArcsTr(&(uuu -> firstArcPost), &(uuu -> lastArcPost), uuu -> cardPost);

				Arc * ccc = ttt -> firstArcPre;
				Arc * ddd = uuu -> firstArcPre;
				while(ccc && ddd)
				{
					if(ddd -> pl == ppp) 
					{
						ddd = ddd -> next_tr;
						continue;
					}
					if(lessthanp(ccc->pl,ddd->pl))
					{
						Arc * eee = new Arc(ccc->pl,vvv,ccc->mult, false); // false = p -> t
						ccc = ccc -> next_tr;
					}					
					else if(lessthanp(ddd->pl,ccc->pl))
					{
						Arc * eee = new Arc(ddd->pl,vvv,ddd->mult, false); // false = p -> t
						ddd = ddd -> next_tr;
					}
					else
					{
						Arc * eee = new Arc(ddd->pl,vvv,ccc->mult+ccc->mult * ddd->mult, false); // false = p -> t
						ccc = ccc -> next_tr;
						ddd = ddd -> next_tr;
					}
				}
				while(ccc)
				{
					Arc * eee = new Arc(ccc->pl,vvv,ccc->mult, false); // false = p -> t
					ccc = ccc -> next_tr;
				}
				while(ddd)
				{
					if(ddd -> pl == ppp) 
					{
						ddd = ddd -> next_tr;
						continue;
					}
					Arc * eee = new Arc(ddd->pl,vvv,ddd->mult, false); // false = p -> t
					ddd = ddd -> next_tr;
				}

				ccc = ttt -> firstArcPost;
				ddd = uuu -> firstArcPost;
				while(ccc && ddd)
				{
					if(ccc -> pl == ppp) 
					{
						ccc = ccc -> next_tr;
						continue;
					}
					if(lessthanp(ccc->pl,ddd->pl))
					{
						Arc * eee = new Arc(ccc->pl,vvv,ccc->mult, true); // true = t -> p
						ccc = ccc -> next_tr;
					}					
					else if(lessthanp(ddd->pl,ccc->pl))
					{
						Arc * eee = new Arc(ddd->pl,vvv,ddd->mult, true); // true = t -> p
						ddd = ddd -> next_tr;
					}
					else
					{
						Arc * eee = new Arc(ddd->pl,vvv,ccc->mult+ccc->mult *ddd->mult, true); // false = p -> t
						ccc = ccc -> next_tr;
						ddd = ddd -> next_tr;
					}
				}
				while(ccc)
				{
					if(ccc -> pl == ppp) 
					{
						ccc = ccc -> next_tr;
						continue;
					}
					Arc * eee = new Arc(ccc->pl,vvv,ccc->mult, true); // false = p -> t
					ccc = ccc -> next_tr;
				}
				while(ddd)
				{
					Arc * eee = new Arc(ddd->pl,vvv,ddd->mult, true); // false = p -> t
					ddd = ddd -> next_tr;
				}

			}
		}

		while(ppp -> firstArcPre)
		{
			delete(ppp -> firstArcPre -> tr);
			cardremovedtransitions++;
			
		}
		while(ppp -> firstArcPost)
		{
			delete(ppp -> firstArcPost -> tr);
			cardremovedtransitions++;
			
		}
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
		delete tmp;

		cardremovedplaces++;

	
	}
	
//RT::rep->status("AGG: %d APPLICATIONS", cardremovedplaces);
//print();
//RT::rep->status("AFTER AGG PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces;
}

int ProtoNet::applyRemoveSinkPlace(bool containsnext, bool containsdeadlock, formula_t type)
{

	// disclaimers for global properties

	// harmless even for HL LV, QL

	// QL, LV, DL: ok
	// OS: not applicable
	if(RT::args.check_arg == check_arg_OneSafe)
	{
		return 0;
	}
	// SM: not applicable
	if(RT::args.check_arg == check_arg_StableMarking)
	{
		return 0;
	}

	int cardremovedplaces = 0;
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		if(ppp -> visible) 
		{
			ppp = ppp -> next;
			continue;
		}
		if(!ppp -> cardPost)
		{	
			ProtoPlace * tmp = ppp;
			ppp = ppp -> next;
			delete tmp;
			continue;
		}
		ppp = ppp -> next;
		
	}
//RT::rep->status("SINK PL: %d APPLICATIONS", cardremovedplaces);
//print();
//RT::rep->status("AFTER SINK PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces;
}
	

int ProtoNet::applyRemoveRedundantComposition(bool containsnext, bool containsdeadlock, formula_t type)
{

	if(RT::args.check_arg == check_arg_modelchecking)
	{
		if(type != FORMULA_DEADLOCK && type != FORMULA_NODEADLOCK && type != FORMULA_REACHABLE && type != FORMULA_INVARIANT)
		{
			return 0;
		}
	}
	// disclaimers for global properties

	// HL: ok for DL, SM, OS
	if(HighLevelNet && (RT::args.check_arg == check_arg_Liveness || RT::args.check_arg == check_arg_QuasiLiveness))
	{
		return 0;
	}
	// LV, QL, SM, OS, DL: ok

	if(containsnext) return 0;
	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	std::less<ProtoPlace * > lessthan = std::less<ProtoPlace *>(); 

	// search for transitions t, t1, t2 where
	// - t- >= t1-
	// - t1+ >= t2-
	// t = t1 + t2

	// look for candidate for t2

	ProtoTransition * ttt2 = firsttransition;
	while(ttt2)
	{
		sortArcsTr(&(ttt2->firstArcPre), &(ttt2->lastArcPre), ttt2 -> cardPre);

		// to look for candidates for t1, we select p in pre(t2) with as few as possible pretransitions

		Arc * aaa = ttt2 -> firstArcPre;
		if(!aaa)
		{
			// t2 has no pre-places
			ttt2 = ttt2 -> next;
			continue;
		}
		int minpre = aaa -> pl -> cardPre;
		for(Arc * bbb = aaa -> next_tr; bbb; bbb = bbb -> next_tr)
		{
			if(bbb -> pl -> cardPre < minpre)
			{
				aaa = bbb;
				minpre = bbb -> pl -> cardPre;
			}
		}
		
		// iterate candidates for t1-> must be in pre(p) for all p in pre(t2)
		
		ProtoPlace * ppp = aaa -> pl;
		Arc * bbb = ppp -> firstArcPre;
		while(bbb)
		{
			ProtoTransition * ttt1 = bbb -> tr;
			if(ttt1 == ttt2) 
			{
				bbb = bbb -> next_pl;
				continue;
			}
			sortArcsTr(&(ttt1->firstArcPost), &(ttt1->lastArcPost), ttt1 -> cardPost);

			// check t1+ >= t2- criterion
			
			bool iscandidate = true;
			Arc * ccc = ttt1 -> firstArcPost;
			for(Arc * ddd = ttt2 -> firstArcPre; ddd; ddd = ddd -> next_tr)
			{
				for(; ccc; ccc = ccc -> next_tr)
				{
					if(ccc -> pl == ddd -> pl) break;
				}
				if(!ccc)
				{
					iscandidate = false;
					break;
				}
				if(ccc -> mult < ddd -> mult)
				{
					iscandidate = false;	
					break;
				}
			}
			if(!iscandidate)
			{
				bbb = bbb -> next_pl;
				continue;
			}
			
			// here: t1 satisfies the t1+ >= t2- criterion

			// iterate candidates for t -> must be in post(p) for all p in pre(t1)

			sortArcsTr(&(ttt1->firstArcPre), &(ttt1->lastArcPre), ttt1 -> cardPre);

			// to look for candidates for t, we select p in pre(t1) with as few as possible posttransitions

			Arc * eee = ttt1 -> firstArcPre;
			if(!eee)
			{
				// t1 has no pre-transitions
				bbb = bbb -> next_pl;
				continue;
			}
			int minpre = eee -> pl -> cardPre;
			for(Arc * fff = eee -> next_tr; fff; fff = fff -> next_tr)
			{
				if(fff -> pl -> cardPre < minpre)
				{
					eee = fff;
					minpre = fff -> pl -> cardPre;
				}
			}
			
			ProtoPlace * rrr = eee -> pl;
			Arc * fff = rrr -> firstArcPost;
			while(fff)
			{
				ProtoTransition * ttt = fff -> tr;
				if(ttt == ttt2 || ttt == ttt1 || ttt -> visible) 
				{
					fff = fff -> next_tr;
					continue;
				}

				// check t- >= t1- criterion
				
				sortArcsTr(&(ttt->firstArcPre), &(ttt->lastArcPre), ttt -> cardPre);
				bool isccandidate = true;
				Arc * ccc = ttt -> firstArcPre;
				for(Arc * ddd = ttt1 -> firstArcPre; ddd; ddd = ddd -> next_tr)
				{
					for(; ccc; ccc = ccc -> next_tr)
					{
						if(ccc -> pl == ddd -> pl) break;
					}
					if(!ccc)
					{
						isccandidate = false;
						break;
					}
					if(ccc -> mult < ddd -> mult)
					{
						isccandidate = false;	
						break;
					}
				}
				if(!isccandidate)
				{
					fff = fff -> next_pl;
					continue;
				}

				// check t = t1 + t2 criterion
				sortArcsTr(&(ttt2 -> firstArcPost), &(ttt2 -> lastArcPost), ttt2 -> cardPost);
				sortArcsTr(&(ttt -> firstArcPost), &(ttt -> lastArcPost), ttt -> cardPost);

				

				Arc * aa = ttt -> firstArcPre;
				Arc * bb = ttt -> firstArcPost;
				Arc * cc = ttt1 -> firstArcPre;
				Arc * dd = ttt1 -> firstArcPost;
				Arc * ee = ttt2 -> firstArcPre;
				Arc * ff = ttt2 -> firstArcPost;
				bool iscccandidate = true;
				while(aa || bb || cc || dd || ee || ff)
				{
					ProtoPlace * minpl = NULL;
					int delta = 0;

					if(aa)
					{
						minpl = aa -> pl;
						delta = aa -> mult;
					}
					if(bb)
					{
						if(minpl)
						{
							if(lessthan(bb -> pl, minpl))
							{
								minpl = bb -> pl;
								delta = - bb -> mult;
							}
							else if (!lessthan(minpl, bb ->pl))
							{
								delta -= bb -> mult;
							}
						}
						else
						{
							minpl = bb -> pl;
							delta =  - bb -> mult;
						}
					}
					if(cc)
					{
						if(minpl)
						{
							if(lessthan(cc -> pl, minpl))
							{
								minpl = cc -> pl;
								delta = - cc -> mult;
							}
							else if (!lessthan(minpl, cc ->pl))
							{
								delta -= cc -> mult;
							}
						}
						else
						{
							minpl = cc -> pl;
							delta =  - cc -> mult;
						}
					}
					if(dd)
					{
						if(minpl)
						{
							if(lessthan(dd -> pl, minpl))
							{
								minpl = dd -> pl;
								delta = dd -> mult;
							}
							else if (!lessthan(minpl, dd ->pl))
							{
								delta += dd -> mult;
							}
						}
						else
						{
							minpl = dd -> pl;
							delta =  dd -> mult;
						}
					}
					if(ee)
					{
						if(minpl)
						{
							if(lessthan(ee -> pl, minpl))
							{
								minpl = ee -> pl;
								delta = - ee -> mult;
							}
							else if (!lessthan(minpl, ee ->pl))
							{
								delta -= ee -> mult;
							}
						}
						else
						{
							minpl = ee -> pl;
							delta =  - ee -> mult;
						}
					}
					if(ff)
					{
						if(minpl)
						{
							if(lessthan(ff -> pl, minpl))
							{
								minpl = ff -> pl;
								delta = ff -> mult;
							}
							else if (!lessthan(minpl, ff ->pl))
							{
								delta += ff -> mult;
							}
						}
						else
						{
							minpl = ff -> pl;
							delta =  ff -> mult;
						}
					}
					if(delta)
					{
						iscccandidate = false;
						break;
					}
					if(aa && aa -> pl == minpl) aa = aa -> next_tr;
					if(bb && bb -> pl == minpl) bb = bb -> next_tr;
					if(cc && cc -> pl == minpl) cc = cc -> next_tr;
					if(dd && dd -> pl == minpl) dd = dd -> next_tr;
					if(ee && ee -> pl == minpl) ee = ee -> next_tr;
					if(ff && ff -> pl == minpl) ff = ff -> next_tr;
				}
				if(!iscccandidate)
				{
					fff = fff -> next_pl;
					continue;
				}

				// at this point, t can be deleted
//RT::rep->status("t = %s, t1 = %s t2 = %s", ttt -> sy -> getKey(), ttt1 -> sy -> getKey(), ttt2 -> sy -> getKey());
							
				fff = fff -> next_pl;
				delete ttt;
				cardremovedtransitions++;
			}
			
			

			bbb = bbb -> next_pl;
		}
		ttt2 = ttt2 -> next;
	}
// RT::rep->status("RED COMP: %d APPLICATIONS", cardremovedplaces);
//print();
//RT::rep->status("AFTER RED COMP PL: %d TR: %d", cardPL,cardTR);
	return cardremovedtransitions;
}


int ProtoNet::applyRemoveSequentialTransition(bool containsnext, bool containsdeadlock, formula_t type)
{
//print();

	// disclaimers for global properties

	// HL: not supported
	if(HighLevelNet && RT::args.check_arg != check_arg_deadlockfreedom)
	{
		return 0;
	}

	// LV, QL, OS, DL: ok	
	if(RT::args.check_arg == check_arg_StableMarking)
	{
		return 0;
	}
	// this rule is sensitive to stuttering
	if(containsnext) return 0;
	
	// this rule preserves reachability of deadlocks but not general formulas
	// that contain a DEADLOCK atomic proposition
	if(containsdeadlock && type != FORMULA_DEADLOCK && type != FORMULA_NODEADLOCK) return 0;

	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	time_t starttime = time(NULL);
	// precondition: arc lists are sorted

	std::less<ProtoTransition *> lessthan = std::less<ProtoTransition *>();
	// iterate through all places
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		// check applicability of rule
		// ===========================

		// single post-transition


		// check pathological application for liveness

		if(RT::args.check_arg == check_arg_Liveness && ppp -> cardPre == 0 && ppp -> cardPost != 0)
		{
		                RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live.").str());
RT::rep->status("FOUND BY REDUCTION");
                                portfoliomanager::compareresult(false);
                                if(RT::args.mcc_given)
                                {
                                        portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
                                }
                                _exit(0);
                     
		}

		if(ppp -> cardPost != 1)
		{
			ppp = ppp -> next;
			continue;
		}

		// check pathological application for quasiliveness

		if(RT::args.check_arg == check_arg_QuasiLiveness && ppp -> cardPre == 0 && ppp -> marking < ppp -> firstArcPost -> mult)
		{
		                RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not quasilive.").str());
                                portfoliomanager::compareresult(false);
                                if(RT::args.mcc_given)
                                {
                                        portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
                                }
                                _exit(0);
                     
		}

		// invisible

		if(ppp -> visible)
		{
			ppp = ppp -> next;
			continue;
		}

		ProtoTransition * ttt = ppp -> firstArcPost -> tr;

		// ppp is only pre-place of ttt

		if(ttt -> cardPre != 1)
		{
			ppp = ppp -> next;
			continue;
		}

		// ttt invisible
		if(ttt -> visible)
		{
			ppp = ppp -> next;
			continue;
		}
		// arc multiplicity is 1
		
		if(ppp -> firstArcPost -> mult != 1)
		{
			ppp = ppp -> next;
			continue;
		}

		// ttt is no pre-transition of ppp, post-places of ttt invisible

		Arc * aaa;
		for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			if(aaa->pl == ppp)
			{
				break;
			}
			ProtoPlace * ppp = aaa -> pl;
			if(ppp -> visible)
			{
				break;
			}
			bool havevis = false;
			for(Arc * bbb = ppp -> firstArcPost; bbb; bbb = bbb -> next_pl)
			{
				if(bbb -> tr -> visible)
				{
					havevis = true;
					break;	
				}
			}
			if(havevis) break;
		}
		if(aaa)
		{
			ppp = ppp -> next;
			continue;
		}

		// apply rule
		// ==========

		// disclaimer for global properties

		if(!ppp->cardPre && !ppp->marking && RT::args.check_arg == check_arg_QuasiLiveness)
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not quasilive").str());
			portfoliomanager::compareresult(false);
			if(RT::args.mcc_given)
			{
				portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			_exit(0);
		}

		// update initial marking

		if(ppp -> marking)
		{
//RT::rep->status("CHOSEN PLACE MARKED %s", ppp -> sy -> getKey());
			capacity_t delta = ppp -> marking;
			for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
			{
//RT::rep->status("ADD %d TOKENS TO  %s",  delta * aaa -> mult, aaa -> pl  -> sy -> getKey());
				aaa -> pl -> marking += delta * aaa -> mult;
			}
		}

		// add shortcut arcs
		for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			ProtoPlace * pi = aaa -> pl;
			mult_t wi = aaa -> mult;
			for(Arc * bbb = ppp -> firstArcPre; bbb; bbb = bbb -> next_pl)
			{
//RT::rep->status("DRAW ARC FROM TR %s TO PL %s MULT %d", bbb -> tr -> sy -> getKey(), pi -> sy -> getKey(), bbb -> mult * wi);
				new Arc(pi, bbb -> tr, bbb -> mult * wi, TP);
			}
		} 

		// merge duplicates
		for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			ProtoPlace * pi = aaa -> pl;
			sortArcsPl(&(pi -> firstArcPre), &(pi -> lastArcPre), pi -> cardPre);
			pi -> sortedPre = true;
			Arc * ccc = pi -> firstArcPre;
			while(true)
			{
				Arc * ddd = ccc -> next_pl;
				if(!ddd) break;
				if(ccc -> tr == ddd -> tr)
				{
					// duplicate detected
					ccc -> mult += ddd -> mult;
//RT::rep->status("MERGED MULT FROM %s TO %s MULT %d", ccc -> tr -> sy -> getKey(), ccc -> pl -> sy -> getKey(), ccc -> mult);
					delete ddd;
				}
				else
				{
					ccc = ddd;
				}
			}
		}

		// remove ppp and ttt

//RT::rep->status("REM SEQ TR: t = %s, p = %s", ttt -> sy -> getKey(), ppp -> sy ->getKey());
		
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
		delete tmp;
		cardremovedplaces++;
		delete ttt;
		cardremovedtransitions++;
	}
	//(*data)["remove sequential transition"]["removed transitions"] = cardremovedtransitions +
	//(int) (*data)["remove sequential transition"]["removed transitions"] ;
	//(*data)["remove sequential transition"]["removed places"] = cardremovedplaces +
	//(int) (*data)["remove sequential transition"]["removed places"];
	//(*data)["remove sequential transition"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove sequential transition"]["time"] ;
//RT::rep->status("AFTER SEQ TR PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces + cardremovedtransitions;
}

int ProtoNet::applyRemoveSequentialPlace(bool containsnext, bool containsdeadlock, formula_t type)
{

// disclaimers for global properties

// HL: only DL supported
if(HighLevelNet && RT::args.check_arg != check_arg_modelchecking && RT::args.check_arg != check_arg_deadlockfreedom)
{
	return 0;
}

// QL, LV ok
if(RT::args.check_arg == check_arg_OneSafe || RT::args.check_arg == check_arg_StableMarking)
{
	return 0;
}

// this rule is sensitive to stuttering
if(containsnext) return 0;

// this rule preserves reachability of deadlocks but not general formulas
// that contain a DEADLOCK atomic proposition
if(containsdeadlock && type != FORMULA_DEADLOCK && type != FORMULA_NODEADLOCK) return 0;


if(containsnext) return 0;
time_t starttime = time(NULL);
int cardremovedplaces = 0;
int cardremovedtransitions = 0;
std::less<ProtoTransition *> lessthan = std::less<ProtoTransition *>();
// iterate through all places
ProtoPlace * ppp = firstplace;
while(ppp)
{
	// check applicability of rule
	// ===========================

	// single post-transition

	if(ppp -> cardPre != 1)
	{
		ppp = ppp -> next;
		continue;
	}
	if(ppp -> cardPost != 1)
	{
		ppp = ppp -> next;
		continue;
	}

	// invisible

	if(ppp -> visible)
	{
		ppp = ppp -> next;
		continue;
	}

	ProtoTransition * ttt = ppp -> firstArcPost -> tr;

	// invisible

	if(ttt -> visible)
	{
		ppp = ppp -> next;
		continue;
	}

	// p only pre-place
	if(ttt -> cardPre != 1)
	{
		ppp = ppp -> next;
		continue;
	}

	// post- and pre-transition of ppp is different

	if(ppp->firstArcPre -> tr == ttt)
	{
		ppp = ppp -> next;
		continue;
	}

	// arc multiplicity is mutiple
	
	if((ppp -> firstArcPre -> mult) % ppp -> firstArcPost -> mult)
	{
		ppp = ppp -> next;
		continue;
	}

	int kkk = ppp -> firstArcPre -> mult / ppp -> firstArcPost -> mult;

	// post-places of ttt invisible

	Arc * aaa;
	for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
	{
		ProtoPlace * ppp = aaa -> pl;
		if(ppp -> visible)
		{
			break;
		}
		/// post-transitions must be invisibe, too
		bool havevis = false;
		for(Arc * bbb = ppp -> firstArcPost; bbb; bbb = bbb -> next_pl)
		{
			if(bbb -> tr -> visible)	
			{
				havevis = true;
				break;
			}
		}
		if(havevis)
		{
			break;
		}
	}
	if(aaa)
	{
		ppp = ppp -> next;
		continue;
	}

	// apply rule
	// ==========

	// update initial marking

	if(ppp -> marking)
	{
		capacity_t delta = ppp -> marking / ppp->firstArcPost -> mult;
		for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			aaa -> pl -> marking += delta * aaa -> mult;
		}
	}

	// add shortcut arcs
	ProtoTransition * uuu = ppp -> firstArcPre -> tr;
	for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
	{
		new Arc(aaa->pl, uuu, aaa -> mult * kkk, TP);
	} 

	// merge duplicates
	sortArcsTr(&(uuu -> firstArcPost), &(uuu -> lastArcPost), uuu -> cardPost);
	uuu -> sortedPost = true;
	Arc * ccc = uuu -> firstArcPost;
	while(true)
	{
		Arc * ddd = ccc -> next_tr;
		if(!ddd) break;
		if(ccc -> pl == ddd -> pl)
		{
			// duplicate detected
			ccc -> mult += ddd -> mult;
			delete ddd;
		}
		else
		{
			ccc = ddd;
		}
	}

	// remove ppp and ttt

//RT::rep->status("REM SEQ PL: p = %s, t = %s", ppp->sy->getKey(),ttt -> sy -> getKey());
	ProtoPlace * tmp = ppp;
	ppp = ppp -> next;
	delete tmp;
	delete ttt;
	cardremovedplaces++;
	cardremovedtransitions++;
}
//(*data)["remove sequential place"]["removed transitions"] = cardremovedtransitions;
//(*data)["remove sequential place"]["removed places"] = cardremovedplaces;
//(*data)["remove sequential place"]["time"] = (int) difftime(time(NULL),starttime);
//print();
//RT::rep->status("AFTER SEQ PL PL: %d TR: %d", cardPL,cardTR);
return cardremovedtransitions + cardremovedplaces;

}

/// computes the ggt of two integers
inline int ggt(int a, int b)
{
    assert(b != 0);
    if(a < 0) a = -a;
    if(b < 0) b = -b;
    while (true) // LCOV_EXCL_LINE
    {
        a %= b;
        if (!a)
        {
            return b;
        }
        b %= a;
        if (!b)
        {
            return a;
        }
    }
}


struct TransitionHashTable
{
	ProtoTransition * t;
	TransitionHashTable * next;
	bool tobedeleted;
};

int getHash(ProtoTransition * ttt, int ggg)
{ 
  int h = 0;
  
  for (Arc * aaa = ttt -> firstArcPre; aaa ; aaa = aaa -> next_tr)
  { 
    uint64_t iii = reinterpret_cast<uint64_t>(aaa->pl);
    iii %= 1 << 30;
    h += iii;
    h += (h << 10);
    h ^= (h >> 6);

    h += aaa->mult / ggg;
    h += (h << 10);
    h ^= (h >> 6);
  }
  h += ttt -> cardPre;
  h += (h << 10);
  h ^= (h >> 6);

  for (Arc * aaa = ttt -> firstArcPost; aaa ; aaa = aaa -> next_tr)
  { 
    uint64_t iii = reinterpret_cast<uint64_t>(aaa->pl);
    iii %= 1 << 30;
    h += iii;
    h += (h << 10);
    h ^= (h >> 6);

    h += aaa->mult / ggg;
    h += (h << 10);
    h ^= (h >> 6);
  }
  h += ttt -> cardPost;
  h += (h << 10);
  h ^= (h >> 6);
 
  
  h += (h << 3);
  
  h ^= (h >> 11);
  
  h += (h << 15);
  
  h %=  SIZEOF_SYMBOLTABLE;
  while(h < 0) h += SIZEOF_SYMBOLTABLE;
  while(h >= SIZEOF_SYMBOLTABLE) h -= SIZEOF_SYMBOLTABLE;
  return h;
}

int getHashEffect(ProtoTransition * ttt)
{ 
	std::less<ProtoPlace *> lessthan = std::less<ProtoPlace *>();
  int h = 0;
  
  Arc * aaa = ttt -> firstArcPre;
  Arc * bbb = ttt -> firstArcPost;
  while(aaa && bbb)
  {
	if(aaa->pl == bbb -> pl)
	{
	    if(aaa->mult == bbb -> mult) 
	    {
	        aaa = aaa -> next_tr;
		bbb = bbb -> next_tr;
		continue; // no effect is not hashed
	    }
	    uint64_t iii = reinterpret_cast<uint64_t>(aaa->pl);
	    iii %= 1 << 30;
	    h += iii;
	    h += (h << 10);
	    h ^= (h >> 6);

	    h += aaa->mult - bbb -> mult;
	    h += (h << 10);
	    h ^= (h >> 6);
	    aaa = aaa -> next_tr;
	    bbb = bbb -> next_tr;
	    continue;
	}
	if(lessthan(aaa->pl,bbb->pl))
	{
	    uint64_t iii = reinterpret_cast<uint64_t>(aaa->pl);
	    iii %= 1 << 30;
	    h += iii;
	    h += (h << 10);
	    h ^= (h >> 6);

	    h += aaa->mult;
	    h += (h << 10);
	    h ^= (h >> 6);
	    aaa = aaa -> next_tr;
	    continue;
	}
		// bbb -> pl less than aaa -> pl
	    uint64_t iii = reinterpret_cast<uint64_t>(bbb->pl);
	    iii %= 1 << 30;
	    h += iii;
	    h += (h << 10);
	    h ^= (h >> 6);

	    h += bbb->mult;
	    h += (h << 10);
	    h ^= (h >> 6);
	    bbb = bbb -> next_tr;
  }
  for (; aaa ; aaa = aaa -> next_tr)
  { 
    uint64_t iii = reinterpret_cast<uint64_t>(aaa->pl);
    iii %= 1 << 30;
    h += iii;
    h += (h << 10);
    h ^= (h >> 6);

    h += aaa->mult ;
    h += (h << 10);
    h ^= (h >> 6);
  }

  for (; bbb ; bbb = bbb -> next_tr)
  { 
    uint64_t iii = reinterpret_cast<uint64_t>(bbb->pl);
    iii %= 1 << 30;
    h += iii;
    h += (h << 10);
    h ^= (h >> 6);

    h += bbb->mult ;
    h += (h << 10);
    h ^= (h >> 6);
  }
  h += (h << 3);
  h ^= (h >> 11);
  h += (h << 15);
  h %=  SIZEOF_SYMBOLTABLE;
  while(h < 0) h += SIZEOF_SYMBOLTABLE;
  while(h >= SIZEOF_SYMBOLTABLE) h -= SIZEOF_SYMBOLTABLE;
  return h;
}

int ProtoNet::applyRemoveParallelPlace(bool containsnext,bool containsdeadlock, formula_t type)
{
//print();

// disclaimers for global properties

// HL: ok for LV, QL, DL
if(HighLevelNet && (RT::args.check_arg == check_arg_StableMarking || RT::args.check_arg == check_arg_OneSafe))
{
	return 0;
}

// all ok for LL

time_t starttime = time(NULL);
int cardremovedplaces = 0;
std::less<ProtoTransition *> lessthan = std::less<ProtoTransition *>();
ProtoPlace * ppp = firstplace;
while(ppp)
{
	if(difftime(time(NULL),starttime) > 120.0) break;
	ProtoPlace * qqq = firstplace;
	while(qqq)
	{
		if(qqq == ppp)
		{
			qqq = qqq -> next;
			continue;
		}
		if(qqq -> visible) 
		{
			qqq = qqq -> next;
			continue;
		}
		if(qqq -> cardPre < ppp -> cardPre)
		{
			qqq = qqq -> next;
			continue;
		}
		if(qqq -> cardPost > ppp -> cardPost)
		{
			qqq = qqq -> next;
			continue;
		}
		if(qqq -> marking < ppp -> marking || (RT::args.check_arg == check_arg_OneSafe && qqq -> marking > ppp -> marking))
		{
			qqq = qqq -> next;
			continue;
		}
		if(RT::args.check_arg == check_arg_StableMarking && (ppp -> cardPost != qqq -> cardPost || ppp -> cardPre != qqq -> cardPre))
		{
			qqq = qqq -> next;
			continue;
		}
		sortArcsPl(&(ppp -> firstArcPre), &(ppp -> lastArcPre), ppp -> cardPre);
		ppp -> sortedPre = true;
		sortArcsPl(&(qqq -> firstArcPre), &(qqq -> lastArcPre), qqq -> cardPre);
		qqq -> sortedPre = true;
		bool applicable = true;
		int kkk = -1;
		if(ppp -> marking)
		{
			kkk = qqq -> marking / ppp -> marking;
		}

		Arc * aaa = ppp -> firstArcPre;
		Arc * bbb = qqq -> firstArcPre;
		while(aaa && bbb)
		{
			if(lessthan(aaa->tr,bbb -> tr))
			{
				applicable = false;
				break;
			}
			else if(aaa -> tr == bbb -> tr)
			{
				if(bbb -> mult < aaa -> mult || (RT::args.check_arg == check_arg_StableMarking && aaa -> mult !=bbb -> mult) || (RT::args.check_arg == check_arg_OneSafe && aaa -> mult == 1 && bbb -> mult != 1))
				{
					applicable = false;
					break;
				}
				int kkk1 = bbb -> mult / aaa -> mult;
				if(kkk < 0 || kkk1 < kkk) kkk = kkk1;
				aaa = aaa -> next_pl;
				bbb = bbb -> next_pl;
			}
			else
			{
				if(RT::args.check_arg == check_arg_OneSafe || RT::args.check_arg == check_arg_StableMarking)
				{
					applicable = false;
					break;
				}
				bbb = bbb -> next_pl;
			}
		}
		if(!applicable)
		{
			qqq = qqq -> next;
			continue;
		}
		if(aaa)
		{
			qqq = qqq -> next;
			continue;
		}
		if(bbb && (RT::args.check_arg == check_arg_StableMarking || RT::args.check_arg == check_arg_OneSafe))
		{
			qqq = qqq -> next;
			continue;
		}
		sortArcsPl(&(ppp -> firstArcPost), &(ppp -> lastArcPost), ppp -> cardPost);
		ppp -> sortedPost = true;
		sortArcsPl(&(qqq -> firstArcPost), &(qqq -> lastArcPost), qqq -> cardPost);
		qqq -> sortedPost = true;
		aaa = ppp -> firstArcPost;
		bbb = qqq -> firstArcPost;
		while(aaa && bbb)
		{
			if(lessthan(bbb->tr,aaa->tr))
			{
				applicable = false;
				break;
			}
			else if(bbb -> tr == aaa -> tr)
			{
				if(kkk < 0 || aaa -> mult * kkk < bbb -> mult)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_pl;
				bbb = bbb -> next_pl;
			}
			else
			{
				if(RT::args.check_arg == check_arg_StableMarking)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_pl;
			}
		}
		if(!applicable)
		{
			qqq = qqq -> next;
			continue;
		}
		if(bbb)
		{
			qqq = qqq -> next;
			continue;
		}
		if(aaa && (RT::args.check_arg == check_arg_StableMarking || RT::args.check_arg == check_arg_OneSafe))
		{
			qqq = qqq -> next;
			continue;
		}
		

		// qqq can be removed.

		ProtoPlace * tmp = qqq;
		qqq = qqq -> next;
//RT::rep->status("REM PAR PL: p = %s, PAR TO %s", tmp->sy->getKey(), ppp -> sy -> getKey());
		delete tmp;
		cardremovedplaces++;
		
	}
	ppp = ppp -> next;
}
//(*data)["remove parallel place"]["removed places"] = cardremovedplaces+
//(int) (*data)["remove parallel place"]["removed places"] ;
//(*data)["remove parallel place"]["time"] = (int) difftime(time(NULL),starttime)+
//(int) (*data)["remove parallel place"]["time"] ;
//print();
//RT::rep->status("AFTER PAR PL PL: %d TR: %d", cardPL,cardTR);
return cardremovedplaces;
}

int ProtoNet::applyRemoveParallelTransition(bool containsnext, bool containsdeadlock, formula_t type)
{

	// disclaimers for global properties

	// HL: not applicable to QL, LV

	if(HighLevelNet && (RT::args.check_arg == check_arg_Liveness || RT::args.check_arg == check_arg_QuasiLiveness)) return 0;

	// LL: all ok

	time_t starttime = time(NULL);
	int cardremovedtransitions = 0;
	TransitionHashTable ** hashtable = new TransitionHashTable * [SIZEOF_SYMBOLTABLE];
	memset(hashtable,0,SIZEOF_SYMBOLTABLE * sizeof(TransitionHashTable *));

	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		if(ttt -> visible) continue;
		// step 1: get ggt of all multiplicities in environment
		int thisggt;
		if(ttt -> cardPre)
		{
			thisggt = ttt -> firstArcPre -> mult;
			for(Arc * aaa = ttt -> firstArcPre -> next_tr; aaa; aaa = aaa -> next_tr)
			{
				thisggt = ggt(thisggt, aaa -> mult);
				if(thisggt == 1) break;
			}
			for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
			{
				thisggt = ggt(thisggt,aaa->mult);
				if(thisggt == 1) break;
			}
		}
		else if(ttt -> cardPost)
		{
			thisggt = ttt -> firstArcPost -> mult;
			for(Arc * aaa = ttt -> firstArcPost -> next_tr; aaa; aaa = aaa -> next_tr)
			{
				thisggt = ggt(thisggt,aaa->mult);
				if(thisggt == 1) break;
			}
		}
		else
		{
			continue;
		}

		// step 2: hash places and multiplicities in environment

		sortArcsTr(&(ttt -> firstArcPre), &(ttt -> lastArcPre), ttt -> cardPre);
		sortArcsTr(&(ttt -> firstArcPost), &(ttt -> lastArcPost), ttt -> cardPost);
		ttt -> sortedPre = ttt -> sortedPost = true;
		TransitionHashTable * hhh = new TransitionHashTable;
		int vvv = getHash(ttt,thisggt);
		hhh -> t = ttt;
		hhh -> next = hashtable[vvv];
		hhh -> tobedeleted = false;
		hashtable[vvv] = hhh;
	}
	for(uint i = 0; i < SIZEOF_SYMBOLTABLE; i++)
	{
		if(!hashtable[i]) continue;
		if(!hashtable[i]->next) 
		{
			delete hashtable[i];
			continue;
		}

		for(TransitionHashTable * hhh1 = hashtable[i]; hhh1; hhh1 = hhh1 -> next)
		{
			ProtoTransition * ttt1 = hhh1 -> t;
			for(TransitionHashTable * hhh2 = hhh1 -> next; hhh2; hhh2 = hhh2 -> next)
			{
				ProtoTransition * ttt2 = hhh2 -> t;

				// detailled check for parallelity;
				if(ttt1 -> cardPre != ttt2 -> cardPre) continue;
				if(ttt1 -> cardPost != ttt2 -> cardPost) continue;
				int kkk;
				bool reverse;
				if(ttt1 -> cardPre)
				{
					if(!((ttt1 -> firstArcPre ->mult)%( ttt2 -> firstArcPre -> mult)))
					{
						kkk = ttt1 -> firstArcPre -> mult / ttt2 -> firstArcPre -> mult;
						reverse = true;
					}
					else if (!((ttt2 -> firstArcPre ->mult)%( ttt1 -> firstArcPre -> mult)))
					{
						kkk = ttt2 -> firstArcPre -> mult / ttt1 -> firstArcPre -> mult;
						reverse = false;
					}
					else
					{
						continue;
					}
				}
				else if(ttt1 -> cardPost)
				{
					if(!((ttt1 -> firstArcPost ->mult)%( ttt2 -> firstArcPost -> mult)))
					{
						kkk = ttt1 -> firstArcPost -> mult / ttt2 -> firstArcPost -> mult;
						reverse = true;
					}
					else if (!((ttt2 -> firstArcPost ->mult)%( ttt1 -> firstArcPost -> mult)))
					{
						kkk = ttt2 -> firstArcPost -> mult / ttt1 -> firstArcPost -> mult;
						reverse = false;
					}
					else
					{
						continue;
					}
				}
				if(reverse && hhh1 -> tobedeleted) continue;
				if(!reverse && hhh2 -> tobedeleted) continue;
				Arc * aaa;
				Arc * bbb;
				for(aaa = ttt1 -> firstArcPre, bbb = ttt2 -> firstArcPre; aaa; aaa = aaa -> next_tr, bbb = bbb -> next_tr)
				{
					if(aaa -> pl != bbb -> pl) break;
					if((reverse && aaa -> mult != kkk * bbb -> mult) || (!reverse && kkk * aaa -> mult !=  bbb -> mult)) break;
				}
				if(aaa) continue;
				for(aaa = ttt1 -> firstArcPost, bbb = ttt2 -> firstArcPost; aaa; aaa = aaa -> next_tr, bbb = bbb -> next_tr)
				{
					if(aaa -> pl != bbb -> pl) break;
					if((reverse && aaa -> mult != kkk * bbb -> mult) || (!reverse && kkk * aaa -> mult !=  bbb -> mult)) break;
				}
				if(aaa) continue;

				if(reverse)
				{
					hhh1 -> tobedeleted = true;
				}
				else
				{
					hhh2 -> tobedeleted = true;
				}
			}
		}
		while(hashtable[i])
		{
			if(hashtable[i]->tobedeleted)
			{

//RT::rep->status("REM PAR TR: t = %s",  hashtable[i]->t->sy->getKey());
				delete hashtable[i]->t;
			cardremovedtransitions++;
			}
			TransitionHashTable * tmp = hashtable[i];
			hashtable[i] = tmp -> next;
			delete tmp;
		}
	}
	delete [] hashtable;
	//(*data)["remove parallel transition"]["removed transitions"] = cardremovedtransitions+
	//(int) (*data)["remove parallel transition"]["removed transitions"] ;
	//(*data)["remove parallel transition"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove parallel transition"]["time"];
//print();
//RT::rep->status("AFTER PAR TR PL: %d TR: %d", cardPL,cardTR);
	return cardremovedtransitions;
}

int ProtoNet::applyRemoveDominatedTransition(bool containsnext, bool containsdeadlock, formula_t type)
{
	// disclaimers for global properties

	// no difference between HL and LL

	if(RT::args.check_arg == check_arg_Liveness || RT::args.check_arg == check_arg_QuasiLiveness) 
	{
		return 0;
	}
	time_t starttime = time(NULL);
	int cardremovedtransitions = 0;
	TransitionHashTable ** hashtable = new TransitionHashTable * [SIZEOF_SYMBOLTABLE];
	memset(hashtable,0,SIZEOF_SYMBOLTABLE * sizeof(TransitionHashTable *));
	std::less<ProtoPlace *> lessthan = std::less<ProtoPlace *>();

	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		if(ttt -> visible) continue;

		// step 1: hash places and multiplicities in effect

		sortArcsTr(&(ttt -> firstArcPre), &(ttt -> lastArcPre), ttt -> cardPre);
		sortArcsTr(&(ttt -> firstArcPost), &(ttt -> lastArcPost), ttt -> cardPost);
		ttt -> sortedPre = ttt -> sortedPost = true;
		TransitionHashTable * hhh = new TransitionHashTable;
		int vvv = getHashEffect(ttt);
		hhh -> t = ttt;
		hhh -> next = hashtable[vvv];
		hhh -> tobedeleted = false;
		hashtable[vvv] = hhh;
	}
	for(uint i = 0; i < SIZEOF_SYMBOLTABLE; i++)
	{
		// empty hash bucket
		if(!hashtable[i]) continue;

		// singleton hash bucket --> no dominated transition
		if(!hashtable[i]->next) 
		{
			delete hashtable[i];
			continue;
		}

		// shared hash bucket -> need to look closer for dominated transitions
		for(TransitionHashTable * hhh1 = hashtable[i]; hhh1; hhh1 = hhh1 -> next)
		{
			if(hhh1-> tobedeleted) continue; // do not need to consider transitions that are sentenced to removal
			ProtoTransition * ttt1 = hhh1 -> t;
			for(TransitionHashTable * hhh2 = hhh1 -> next; hhh2; hhh2 = hhh2 -> next)
			{
				if(hhh2-> tobedeleted) continue; // do not need to consider transitions that are sentenced to removal
				ProtoTransition * ttt2 = hhh2 -> t;

				// detailled check for domination;
				int direction = 0; // -1: t1 has stricter preconditions than t2
						   // 1: t2 has stricter precondition than t1
						   // 0: both have equal preconditions
				bool founddomination = false;
				Arc * aaa = ttt1 -> firstArcPre;
				Arc * bbb = ttt1 -> firstArcPost;
				Arc * ccc = ttt2 -> firstArcPre;
				Arc * ddd = ttt2 -> firstArcPost;


				while(true) 
				{
					// case 1: t1 not completely processed
					if(aaa || bbb)
					{
						int effect1;
						int threshold1;
						ProtoPlace * p1;
						if(!aaa || (bbb && lessthan(bbb->pl,aaa->pl)))
						{
							p1 = bbb -> pl;
							effect1 = bbb -> mult;
							threshold1 = 0;
						}
						else if(!bbb || (aaa && lessthan(aaa->pl,bbb->pl)))
						{
							p1 = aaa -> pl;
							effect1 = - aaa -> mult;
							threshold1 = aaa -> mult;
						}
						else // aaa && bbb && aaa->pl == bbb -> pl
						{
							p1 = aaa -> pl;
							effect1 = bbb -> mult - aaa->mult;
							threshold1 = aaa -> mult;
						}
						// case 1.1 t2 not completely processed
						if(ccc || ddd)
						{
							int effect2;
							int threshold2;
							ProtoPlace * p2;
							if(!ccc || (ddd && lessthan(ddd->pl,ccc->pl)))
							{
								p2 = ddd -> pl;
								effect2 = ddd -> mult;
								threshold2 = 0;
							}
							else if(!ddd || (ccc && lessthan(ccc->pl,ddd->pl)))
							{
								p2 = ccc -> pl;
								effect2 = ccc -> mult;
								threshold2 = ccc -> mult;
							}
							else
							{
								p2 = ccc -> pl;	
								effect2 = ddd->pl - ccc -> pl;
								threshold2 = ccc -> mult;
							}
							if(lessthan(p1,p2))
							{
								if(effect1 != 0)
								{
									break;
								}
								// effect(p1) == 0, threshold(p1) != 0 --> direction must be -1
								if(direction == 1)
								{
									break;
								}
								if(aaa && aaa -> pl == p1)
								{
									aaa = aaa -> next_tr;
								}
								if(bbb && bbb -> pl == p1)
								{
									bbb = bbb -> next_tr;
								}
								if(ccc && ccc -> pl == p1)
								{
									ccc = ccc -> next_tr;
								}
								if(ddd && ddd -> pl == p1)
								{
									ddd = ddd -> next_tr;
								}
								direction = -1;
							}
							else if(lessthan(p2,p1))
							{
								if(effect2 != 0)
								{
									break;
								}
								// effect(p2) == 0, threshold(p2) != 0 --> direction must be 1
								if(direction == -1)
								{
									break;
								}
								if(aaa && aaa -> pl == p2)
								{
									aaa = aaa -> next_tr;
								}
								if(bbb && bbb -> pl == p2)
								{
									bbb = bbb -> next_tr;
								}
								if(ccc && ccc -> pl == p2)
								{
									ccc = ccc -> next_tr;
								}
								if(ddd && ddd -> pl == p2)
								{
									ddd = ddd -> next_tr;
								}
								direction = 1;
							}
							else // p1 == p2
							{
								if(effect1 != effect2)
								{
									break;
								}
								// effect(p2) == effect1
								if(threshold1 > threshold2)
								{
									if(direction == 1)
									{
										break;
									}
									direction = -1;
								}
								else if(threshold2 > threshold1)
								{
									if(direction == -1)
									{
										break;
									}
									direction = 1;
								}
								if(aaa && aaa -> pl == p2)
								{
									aaa = aaa -> next_tr;
								}
								if(bbb && bbb -> pl == p2)
								{
									bbb = bbb -> next_tr;
								}
								if(ccc && ccc -> pl == p2)
								{
									ccc = ccc -> next_tr;
								}
								if(ddd && ddd -> pl == p2)
								{
									ddd = ddd -> next_tr;
								}
								direction = 1;
							}
						}
						else // case 1.2 t2 completely processed
						{
							if(effect1 != 0) break;
							if(direction == 1) break;
							direction = -1;
							// effect1 = 0 means aaa && bbb && aaa -> pl == bbb -> pl
							aaa = aaa -> next_tr;
							bbb = bbb -> next_tr;
						}
						
					}
					else // case 2: t1 completely processed
					{
						if(ccc || ddd) // case 2.1 t2 not completely processed
						{
							if(!ccc || !ddd || ccc -> pl != ddd -> pl || ccc -> mult != ddd -> mult) 
							{
								// effect2 != 0
								break;
							}
							if(direction == -1)
							{
								break;
							}
							direction = 1;
							ccc = ccc -> next_tr;
							ddd = ddd -> next_tr;
						}
						else
						{
							// case 2.2 everybody processed
							founddomination = true;
							break;
						}
					}
				}
				if(founddomination)
				{
					if(direction == 1)
					{
						hhh2 -> tobedeleted = true;
					}
					else
					{
						hhh1 -> tobedeleted = true;
					}
				}
			}
		}
		while(hashtable[i])
		{
			if(hashtable[i]->tobedeleted)
			{

//RT::rep->status("REM DOM TR: t = %s",  hashtable[i]->t->sy->getKey());
				delete hashtable[i]->t;
			cardremovedtransitions++;
			}
			TransitionHashTable * tmp = hashtable[i];
			hashtable[i] = tmp -> next;
			delete tmp;
		}
	}
	delete [] hashtable;
	//(*data)["remove parallel transition"]["removed transitions"] = cardremovedtransitions+
	//(int) (*data)["remove parallel transition"]["removed transitions"] ;
	//(*data)["remove parallel transition"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove parallel transition"]["time"];
//print();
//RT::rep->status("AFTER DOM TR PL: %d TR: %d", cardPL,cardTR);
	return cardremovedtransitions;
}

int ProtoNet::applyRemoveDeadTransition(bool containsnext, bool containsdeadlock, formula_t type)
{
	// disclaimers for global properties

	if(HighLevelNet && (RT::args.check_arg == check_arg_QuasiLiveness || RT::args.check_arg == check_arg_Liveness)) return 0;

	time_t starttime = time(NULL);
	int cardremovedtransitions = 0;
	int cardremovedplaces = 0;
	std::less<ProtoTransition *> lessthan = std::less<ProtoTransition *>();
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		sortArcsPl(&(ppp -> firstArcPre), &(ppp -> lastArcPre), ppp ->cardPre);
		ppp->sortedPre = true;
		sortArcsPl(&(ppp -> firstArcPost), &(ppp -> lastArcPost), ppp ->cardPost);
		ppp->sortedPost = true;

		Arc * aaa = ppp-> firstArcPre;
		Arc * bbb = ppp -> firstArcPost;
		bool applicable = true;
		while(aaa && bbb)
		{
			if(lessthan(aaa -> tr, bbb -> tr))
			{
				applicable = false;
				break;
			}	
			if(aaa -> tr == bbb -> tr)
			{
				if(aaa -> mult > bbb -> mult && bbb -> mult <= ppp -> marking)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_pl;
				bbb = bbb -> next_pl;
			}
			else
			{
				if(ppp -> marking >= bbb -> mult)	
				{
					applicable = false;
					break;
				}
				bbb = bbb -> next_pl;
			}
		}
		if(!applicable)
		{
			ppp = ppp -> next;
			continue;
		}
		if(aaa) 
		{
			applicable = false;
			ppp = ppp -> next;
			continue;
		}
		else
		{
			while(bbb)	
			{
				if(ppp -> marking >= bbb -> mult)
				{
					applicable = false;
					break;
				}
				bbb = bbb -> next_pl;
			}
		}
		if(!applicable)
		{
			ppp = ppp -> next;
			continue;
		}
		// applicable

		bbb = ppp -> firstArcPost;
		while(bbb)
		{
			if(ppp -> marking < bbb -> mult && !(bbb -> tr -> visible))
			{
				ProtoTransition * ttt = bbb -> tr;
				bbb = bbb -> next_pl;
RT::rep->status("REM DEA TR: t = %s", ttt -> sy -> getKey());
		if(RT::args.check_arg == check_arg_Liveness)
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not live").str());
RT::rep->status("FFOUND BY REDUCTION");
			portfoliomanager::compareresult(false);
			if(RT::args.mcc_given)
			{
				portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			_exit(0);
		}
		if(RT::args.check_arg == check_arg_QuasiLiveness)
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is nnot quasilive").str());
			portfoliomanager::compareresult(false);
			if(RT::args.mcc_given)
			{
				portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			_exit(0);
		}
				delete ttt;
				cardremovedtransitions++;
			}
			else
			{	
				bbb = bbb -> next_pl;
			}
		}
		if(RT::args.check_arg == check_arg_OneSafe && ppp ->marking > 1)
		{
			RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str());
			portfoliomanager::compareresult(false);
			if(RT::args.mcc_given)
			{
				portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			_exit(0);
		}
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
		if(tmp -> cardPost == 0 && !(tmp -> visible) && RT::args.check_arg != check_arg_StableMarking) 
		{
			delete tmp;
			cardremovedplaces++;
		}
	}
	//(*data)["remove dead transition"]["removed places"] = cardremovedplaces+
	//(int) (*data)["remove dead transition"]["removed places"] ;
	//(*data)["remove dead transition"]["removed transitions"] = cardremovedtransitions+
	//(int) (*data)["remove dead transition"]["removed transitions"] ;
	//(*data)["remove dead transition"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove dead transition"]["time"] ;
//print();
//RT::rep->status("AFTER DEAD TR PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces + cardremovedtransitions;
}

int ProtoNet::applyRemoveRedundantPlace(bool containsnext, bool containsdeadlock, formula_t type)
{
	// Disclaimers for global properties

	if(RT::args.check_arg == check_arg_OneSafe) return 0;
	if(RT::args.check_arg == check_arg_StableMarking) return 0;
	time_t starttime = time(NULL);
	int cardremovedplaces = 0;
	std::less<ProtoTransition *> lessthan = std::less<ProtoTransition *>();
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		if(ppp -> visible) 
		{
			ppp = ppp -> next;
			continue;
		}
		sortArcsPl(&(ppp -> firstArcPre), &(ppp -> lastArcPre), ppp ->cardPre);
		sortArcsPl(&(ppp -> firstArcPost), &(ppp -> lastArcPost), ppp ->cardPost);
		ppp -> sortedPre = ppp -> sortedPost = true;

		Arc * bbb = ppp-> firstArcPre;
		Arc * aaa = ppp -> firstArcPost;
		bool applicable = true;
		while(aaa && bbb)
		{
			if(lessthan(aaa -> tr, bbb -> tr))
			{
				applicable = false;
				break;
			}	
			if(aaa -> tr == bbb -> tr)
			{
				if(aaa -> mult > bbb -> mult || aaa -> mult > ppp -> marking)
				{
					applicable = false;
					break;
				}
				if(RT::args.check_arg == check_arg_StableMarking && aaa -> mult < bbb -> mult)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_pl;
				bbb = bbb -> next_pl;
			}
			else
			{
				if(RT::args.check_arg == check_arg_StableMarking)
				{
					applicable = false;
					break;
				}
				bbb = bbb -> next_pl;
			}
		}
		if(!applicable)
		{
			ppp = ppp -> next;
			continue;
		}
		if(aaa) 
		{
			applicable = false;
			ppp = ppp -> next;
			continue;
		}
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
//RT::rep->status("REM RED PL: p = %s", tmp -> sy -> getKey());
		delete tmp;
		cardremovedplaces++;
	}
	//(*data)["remove redundant place"]["removed places"] = cardremovedplaces+
	//(int) (*data)["remove redundant place"]["removed places"] ;
	//(*data)["remove redundant place"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove redundant place"]["time"] ;
//print();
//RT::rep->status("AFTER RED PL PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces;
}

int ProtoNet::applyRemoveRedundantTransition(bool containsnext, bool containsdeadlock, formula_t type)
{

	// disclaimers for global properties

	if(RT::args.check_arg == check_arg_deadlockfreedom || RT::args.check_arg == check_arg_QuasiLiveness || RT::args.check_arg == check_arg_Liveness)
	{
		return 0;
	}
	// this rule is neither stuttering invariant nor deadlock preserving
	if(containsdeadlock || containsnext) return 0;

	// this rule only preserves reachability
	if(type != FORMULA_REACHABLE && type != FORMULA_INVARIANT && type != FORMULA_BOUND) return 0;
	time_t starttime = time(NULL);
	int cardremovedtransitions = 0;
	std::less<ProtoPlace *> lessthan = std::less<ProtoPlace *>();
	ProtoTransition * ttt = firsttransition;
	while(ttt)
	{
		if(ttt -> visible) 
		{
			ttt = ttt -> next;
			continue;
		}
		sortArcsTr(&(ttt -> firstArcPre), &(ttt -> lastArcPre), ttt ->cardPre);
		sortArcsTr(&(ttt -> firstArcPost), &(ttt -> lastArcPost), ttt ->cardPost);
		ttt -> sortedPre = ttt -> sortedPost = true;

		Arc * aaa = ttt-> firstArcPre;
		Arc * bbb = ttt -> firstArcPost;
		bool applicable = true;
		while(aaa && bbb)
		{
			if(lessthan(aaa -> pl, bbb -> pl))
			{
				// p is only pre-place
				if(aaa -> pl -> visible || RT::args.check_arg == check_arg_StableMarking)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_tr;
				
			}	
			else if(aaa -> pl == bbb -> pl)
			{
				if(aaa -> mult < bbb -> mult || (RT::args.check_arg == check_arg_StableMarking && aaa -> mult > bbb -> mult))
				{
					applicable = false;
					break;
				}
				if(aaa -> mult > bbb -> mult && aaa -> pl -> visible)
				{
					applicable = false;
					break;
				}
				aaa = aaa -> next_tr;
				bbb = bbb -> next_tr;
			}
			else
			{
				applicable = false;
				break;
			}
		}
		if(!applicable)
		{
			ttt = ttt -> next;
			continue;
		}
		while(aaa)
		{
			if(aaa -> pl -> visible)
                        {       
                                applicable = false;
                                break;
                        }
                        aaa = aaa -> next_tr;
		}
		if(!applicable)
		{
			ttt = ttt -> next;
			continue;
		}
		if(bbb) 
		{
			applicable = false;
			ttt = ttt -> next;
			continue;
		}
		ProtoTransition * tmp = ttt;
		ttt = ttt -> next;
//RT::rep->status("REM RED TR: t = %s", tmp -> sy -> getKey());
		delete tmp;
		cardremovedtransitions ++;
	}
	//(*data)["remove redundant transition"]["removed transitions"] = cardremovedtransitions+
	//(int) (*data)["remove redundant transition"]["removed transitions"];
	//(*data)["remove redundant transition"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["remove redundant transition"]["time"] ;
//print();
//RT::rep->status("AFTER RED TR PL: %d TR: %d", cardPL,cardTR);
	return cardremovedtransitions;
}

int ProtoNet::applyCondenseStronglyConnectedStateMachine(bool containsnext, bool containsdeadlock, formula_t type)
{

	// disclaimers for global properties
	if(RT::args.check_arg != check_arg_modelchecking && RT::args.check_arg != check_arg_deadlockfreedom) return 0;

	
	if(containsnext) return 0;
	time_t starttime = time(NULL);
	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	ProtoPlace ** SearchStack = new ProtoPlace * [cardPL];
	int SearchStackPointer = 0;
	
	int nextdfs = 0;

	ProtoPlace ** TarjanStack = new ProtoPlace * [cardPL];
	int TarjanStackPointer = 0;

	ProtoPlace ** Component = new ProtoPlace * [cardPL];
	int * startcomponent = new int[cardPL/2];
	int * endcomponent = new int[cardPL/2];
	int cardComponents = 0;

	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		ppp -> visited = ppp -> finished = 0;
	}

	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		if(ppp->finished) 
		{
			continue;
		}
		if(ppp->visible) 
		{
			continue;
		}

		// ppp is start place for dfs search

		nextdfs = 1;
		TarjanStackPointer = 0;
		SearchStackPointer = 0;
		ppp -> dfs = ppp -> lowlink = 0;
		SearchStack[0] = TarjanStack[0] = ppp;
		ppp -> currentarc = ppp -> firstArcPost;
		ProtoPlace * currentplace = ppp;
		ppp -> visited = true;

		while(true)
		{
			// continue processing currentplace
			if(currentplace -> currentarc)
			{
				// successor to be considered
				
				// check whether successor is state machine transition
				if(currentplace -> currentarc -> mult != 1) 
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				if(currentplace -> currentarc -> tr -> cardPre != 1) 
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				if(currentplace -> currentarc -> tr -> cardPost != 1) 
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				if(currentplace -> currentarc -> tr -> visible) 
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				if(currentplace -> currentarc -> tr -> firstArcPost -> mult != 1)
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				// successor IS state machine transition -> consider new place

				ProtoPlace * newplace = currentplace -> currentarc -> tr -> firstArcPost -> pl;
				if(newplace -> visible)
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				if(newplace -> finished)
				{
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}

				if(newplace -> visited)
				{
					if(currentplace -> lowlink > newplace -> dfs) currentplace -> lowlink = newplace -> dfs;
					currentplace -> currentarc = currentplace -> currentarc -> next_pl;
					continue;
				}
				
				newplace -> visited = true;
				SearchStack[++SearchStackPointer] = newplace;
				newplace -> currentarc = newplace -> firstArcPost;
				TarjanStack[++TarjanStackPointer] = newplace;
				newplace -> dfs = newplace -> lowlink = nextdfs++;
				currentplace = newplace;
				continue;
			}
			// currentplace completely processed
			if(currentplace -> lowlink == currentplace -> dfs)
			{
				// detected scc
				if(currentplace != TarjanStack[TarjanStackPointer])
				{
					// scc contains more than 1  element -> new component
					if(cardComponents == 0)
					{
						startcomponent[cardComponents] = 0;
					}
					else
					{
						startcomponent[cardComponents] = endcomponent[cardComponents-1];
					}
					endcomponent[cardComponents] = startcomponent[cardComponents];
					do
					{
						TarjanStack[TarjanStackPointer]->finished = true;
						Component[endcomponent[cardComponents]++] = TarjanStack[TarjanStackPointer];
					}
					while(TarjanStack[TarjanStackPointer--] != currentplace);
					cardComponents++;
				}
				else
				{
					// singleton scc
					currentplace -> finished = true;
					// singleton scc -> no component
					TarjanStackPointer--;
				}
				if(TarjanStackPointer < 0) break; // this search completed
			}
			// backtrack from currentplace;
			ProtoPlace * newplace = SearchStack[--SearchStackPointer];
			if(newplace->lowlink > currentplace->lowlink) newplace->lowlink = currentplace -> lowlink;
			newplace -> currentarc = newplace -> currentarc -> next_pl;
			currentplace = newplace;
		}
	}
	for(int i = 0; i < cardComponents; i++)
	{
		ProtoPlace * remainingPlace = Component[startcomponent[i]];
		for(int j = startcomponent[i] + 1; j < endcomponent[i]; j++)
		{
			ProtoPlace * removedPlace = Component[j];
			remainingPlace -> marking += removedPlace -> marking;
			for(Arc * aaa = removedPlace->firstArcPre; aaa; aaa = aaa -> next_pl)
			{
				new Arc(remainingPlace,aaa->tr,aaa->mult, TP);
			}
			for(Arc * aaa = removedPlace->firstArcPost; aaa; aaa = aaa -> next_pl)
			{
				new Arc(remainingPlace,aaa->tr,aaa->mult, PT);
			}
//RT::rep->status("REM SCC: p = %s", removedPlace -> sy -> getKey());
			delete removedPlace;
			cardremovedplaces++;
		}
		
		// merge duplicates
		sortArcsPl(&(remainingPlace -> firstArcPre), &(remainingPlace -> lastArcPre), remainingPlace -> cardPre);
		remainingPlace -> sortedPre = true;
		Arc * ccc = remainingPlace -> firstArcPre;
		while(true)
		{
			Arc * ddd = ccc -> next_pl;
			if(!ddd) break;
			if(ccc -> tr == ddd -> tr)
			{
				// duplicate detected
				ccc -> mult += ddd -> mult;
				delete ddd;
			}
			else
			{
				ccc = ddd;
			}
		}
		sortArcsPl(&(remainingPlace -> firstArcPost), &(remainingPlace -> lastArcPost), remainingPlace -> cardPost);
		remainingPlace -> sortedPost = true;
		ccc = remainingPlace -> firstArcPost;
		while(true)
		{
			Arc * ddd = ccc -> next_pl;
			if(!ddd) break;
			if(ccc -> tr == ddd -> tr)
			{
				// duplicate detected
				ccc -> mult += ddd -> mult;
				delete ddd;
			}
			else
			{
				ccc = ddd;
			}
		}
	}
	//(*data)["merge state machine places"]["removed places"] = cardremovedplaces+
	//(int) (*data)["merge state machine places"]["removed places"] ;
	//(*data)["merge state machine places"]["removed transitions"] = cardremovedtransitions+
	//(int) (*data)["merge state machine places"]["removed transitions"] ;
	//(*data)["merge state machine places"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["merge state machine places"]["time"] ;

	delete [] SearchStack;
	delete [] TarjanStack;
	delete [] Component;
	delete [] startcomponent;
	delete [] endcomponent;
//RT::rep->status("REM SCC: %d APPLICATIONS", cardremovedplaces + cardremovedtransitions);
//print();
//RT::rep->status("AFTER SCC PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces + cardremovedtransitions;
}

int ProtoNet::applySlicing(bool containsnext, bool containsdeadlock, formula_t type, tFormula f)
{
	if(RT::args.check_arg != check_arg_modelchecking) return 0;
	// this rule is not stuttering invariant nor deadlock preserving
	if(containsnext || containsdeadlock) return 0;

	// this rule preserves reachability but no general temporal formulas
	if(RT::args.netreduction_arg == netreduction_arg_off) return 0;
	if(type != FORMULA_REACHABLE && type != FORMULA_INVARIANT && type != FORMULA_BOUND) return 0;
	int starttime = time(NULL);
	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	ProtoNode ** SearchStack = new ProtoNode * [cardPL + cardTR];
	int SearchStackPointer = 0;
	pthread_mutex_lock(&kimwitu_mutex);

	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		ppp -> visited = false;
		ppp -> included = false;
		((PlaceSymbol *)(ppp -> sy)) -> occurspositive = false;
		((PlaceSymbol *)(ppp -> sy)) -> occursnegative = false;
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		if(ttt -> visible)
		{
			for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
			{
				aaa -> pl -> visible = true;
			}
		}
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		ttt -> visited = false;
		ttt -> included = false;
	}

	ProtoNet::currentnet = this;
	f -> unparse(myprinter, kc::markslicingef);
		

	// in the sequel, we want to work on the ProtoNode Level.
	// For distinuishing places and transitions, we set included and visited to true for places, and
	// only visited for transitions. Visible places are only included, since we may need to process them
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
//RT::rep->status("RECOGNISE PLACE %s POS %d NEG %d", ppp -> sy -> getKey(), ((PlaceSymbol *)(ppp -> sy)) -> occurspositive, ((PlaceSymbol *)(ppp -> sy)) -> occursnegative);
		if( ((PlaceSymbol *)(ppp -> sy)) -> occurspositive || ((PlaceSymbol *)(ppp -> sy)) -> occursnegative)
		{
//RT::rep->status("  INCLUDE");
			ppp -> included = true;
			if(ppp->sy->occurspositive )
			{
				if(type == FORMULA_REACHABLE)
				{
					for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
					{
						ProtoTransition * ttt = aaa -> tr;
						if(! ttt -> visited)
						{
							ttt -> visited = true;
							SearchStack[SearchStackPointer++] = ttt;
						}	
					}
				}
				else
				{
					for(Arc * aaa = ppp -> firstArcPre; aaa ; aaa = aaa -> next_pl)
					{
						ProtoTransition * ttt = aaa -> tr;

						if(! ttt -> visited)
						{
							ttt -> visited = true;
							SearchStack[SearchStackPointer++] = ttt;
						}	
					}
				}
			}
			
			if(ppp->sy->occursnegative ) // not exclusive to occurspositive!
			{
				if(type != FORMULA_REACHABLE)
				{
					for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
					{
						ProtoTransition * ttt = aaa -> tr;
						if(! ttt -> visited)
						{
							ttt -> visited = true;
							SearchStack[SearchStackPointer++] = ttt;
						}	
					}
				}
				else
				{
					for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
					{
						ProtoTransition * ttt = aaa -> tr;
						if(! ttt -> visited)
						{
							ttt -> visited = true;
							SearchStack[SearchStackPointer++] = ttt;
						}	
					}
				}
			}
		}
	}
	
	pthread_mutex_unlock(&kimwitu_mutex);

	int processed = 0;

	while(processed < SearchStackPointer)
	{
		ProtoNode * nnn = SearchStack[processed];
		bool isplace = nnn -> included;
//RT::rep->status("PROCESS %s", isplace ? ((ProtoPlace *) (nnn)) -> sy -> getKey() : ((ProtoTransition *) (nnn)) -> sy -> getKey());
		for(Arc *  aaa = nnn -> firstArcPre; aaa; aaa = isplace ? aaa -> next_pl : aaa -> next_tr)
		{
			ProtoNode * ooo = isplace ? (ProtoNode *) aaa -> tr : (ProtoNode *) aaa -> pl;
			if(! ooo -> visited)
			{
				SearchStack[SearchStackPointer++] = ooo;
				ooo -> visited = true;
				ooo -> included = isplace ? false : true;
			}
		}
		processed++;
	}

	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		if(ppp -> included) 
		{
			ppp = ppp -> next;
			continue;
		}
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
//RT::rep->status("SLICE PL %s", tmp -> sy -> getKey());
		delete tmp;
		cardremovedplaces++;
	}
	ProtoTransition * ttt = firsttransition;
	while(ttt)
	{
		if(ttt -> visited || ttt -> visible) 
		{
			ttt = ttt -> next;
			continue;
		}
		ProtoTransition * tmp = ttt;
		ttt = ttt -> next;
//RT::rep->status("SLICE TR %s", tmp -> sy -> getKey());
		delete tmp;
		cardremovedtransitions++;
	}
	delete [] SearchStack;
/*
	//(*data)["slicing"]["removed places"] = cardremovedplaces+
	//(int) (*data)["slicing"]["removed places"] ;
	//(*data)["slicing"]["removed transition"] = cardremovedtransitions+
	//(int) (*data)["slicing"]["removed transition"] ;
	//(*data)["slicing"]["time"] = (int) difftime(time(NULL),starttime)+
	//(int) (*data)["slicing"]["time"] ;
*/
//RT::rep->status("AFTER SLI PL: %d TR: %d", cardPL,cardTR);
	return cardremovedplaces+cardremovedtransitions;
}

void ProtoNet::print()
{
	std::cout << this << ";;; " << cardPL << " places, " << cardTR << " transitions." << std::endl;
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		std::cout << "PL " << ppp -> sy -> getKey()  << " " << "IDX " << ppp -> index << " CAP " << ppp -> capacity  << " M0 " << ppp -> marking << (ppp->visible ? "visible" : "invisible") << " #PRE " << ppp -> cardPre << " #POST " << ppp -> cardPost << std::endl;
		std::cout << "\tPRE ";
		for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
		{
			std::cout << aaa -> tr -> sy -> getKey() << ":" << aaa -> mult << ", ";
		}
		std::cout << std::endl << "\tPOST ";
		for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
		{
			std::cout << aaa -> tr -> sy -> getKey() << ":" << aaa -> mult << ", ";
		}
		std::cout << std::endl ;
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		std::cout << "TR  " << ttt -> sy -> getKey()  << " IDX " << ttt -> index << std::endl;
		std::cout << "\tPRE ";
		for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
		{
			std::cout << aaa -> pl -> sy -> getKey() << ":" << aaa -> mult << ", ";
		}
		std::cout << std::endl << "\tPOST ";
		for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			std::cout << aaa -> pl -> sy -> getKey() << ":" << aaa -> mult << ", ";
		}
		std::cout << std::endl ;
	}
}

void ProtoNet::detectEmptySiphon()
{
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		if(ppp -> marking)
		{
			ppp -> dfs = 1; // not in siphon
		}
		else	
		{
			ppp -> dfs = 0; // perhaps in siphon
		}
	}
	bool somethingchanged;
	do
	{
		somethingchanged = false;
		for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
		{
			bool haspreplace = false;
			for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa ->next_tr)
			{
				if(aaa -> pl -> dfs == 0) 
				{
					haspreplace = true;
					break;
				}
			}
			if(haspreplace) continue;
			for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa ->next_tr)
			{
				if(aaa -> pl -> dfs == 0)
				{
					aaa -> pl -> dfs = 1; 
					somethingchanged = true;
				}
			}
	
		}
	}
	while(somethingchanged);
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		if(ppp -> dfs == 1) continue;
		ppp -> sy -> inEmptySiphon = true;
		for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)		
		{
			aaa -> tr -> sy -> inEmptySiphon = true;
		}
	}
}

void ProtoNet::applyEmptySiphon()
{
	int cardremovedplaces = 0;
	int cardremovedtransitions = 0;
	ProtoPlace * ppp = firstplace;
	while(ppp)
	{
		if(!ppp -> sy -> inEmptySiphon) 
		{
			ppp = ppp -> next;
			continue;
		}
		cardremovedplaces++;
		ProtoPlace * tmp = ppp;
		ppp = ppp -> next;
		delete tmp;
	}
	ProtoTransition * ttt = firsttransition;
	while(ttt)
	{
		if(!ttt -> sy -> inEmptySiphon) 
		{
			ttt = ttt -> next;
			continue;
		}
		cardremovedtransitions++;
		ProtoTransition * tmp = ttt;
		ttt = ttt -> next;
		delete tmp;
	}
	RT::rep->status("Rule S: %d transitions removed,%d places removed", cardremovedtransitions,cardremovedplaces);
	//RT::data["empty siphon"]["removed places"] = cardremovedplaces;
	//RT::data["empty siphon"]["removed transition"] = cardremovedtransitions;
}

void ProtoNet::reduce(bool containsnext, bool containsdeadlock, formula_t type)
{
//RT::rep->status("START REDUCE PL: %d TR: %d", cardPL,cardTR);
//print();

	//(*data)["remove sequential transition"]["removed transitions"] = 0;
	////(*data)["remove sequential transition"]["removed places"] = 0;
	//(*data)["remove sequential transition"]["time"] = 0;
	//(*data)["remove sequential place"]["removed transitions"] = 0;
	//(*data)["remove sequential place"]["removed places"] = 0;
	//(*data)["remove sequential place"]["time"] = 0;
	//(*data)["remove parallel place"]["removed places"] = 0;
	//(*data)["remove parallel place"]["time"] = 0;
	//(*data)["remove parallel transition"]["removed transitions"] = 0;
	//(*data)["remove parallel transition"]["time"] = 0;
	//(*data)["remove dead transition"]["removed places"] = 0;
	//(*data)["remove dead transition"]["removed transitions"] = 0;
	//(*data)["remove dead transition"]["time"] = 0;
	//(*data)["remove redundant place"]["removed places"] = 0;
	//(*data)["remove redundant place"]["time"] = 0;
	//(*data)["remove redundant transition"]["removed transitions"] = 0;
	//(*data)["remove redundant transition"]["time"] = 0;
	//(*data)["merge state machine places"]["removed places"] = 0;
	//(*data)["merge state machine places"]["removed transitions"] = 0;
	//(*data)["merge state machine places"]["time"] = 0;
	//(*data)["slicing"]["removed places"] = 0;
	//(*data)["slicing"]["removed transition"] = 0;
	//(*data)["slicing"]["time"] = 0;
	
	int counter = 0; // counter for sensing "no change"

	if(RT::args.check_arg != check_arg_modelchecking)
	{
		for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
		{
			ttt -> visible = false;
		}		
	}




	// enabling condition of visible transition is always visible
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		if(!ttt -> visible) continue;
		for(Arc * aaa = ttt -> firstArcPre; aaa ; aaa = aaa -> next_tr)
		{
			aaa -> pl -> visible = true;
		}
	}
	while(true)
	{
		counter++;
		if(applyRemoveSequentialTransition(containsnext,containsdeadlock,type))
		{
		 	counter = 0;
		}
		else
		{
			//counter++;
		}
		if(counter >= 11) break;
		if(applyRemoveSequentialPlace(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
		if(applyRemoveParallelPlace(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
	
		if(applyRemoveDominatedTransition(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;

		if(applyRemoveRedundantComposition(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
		if(applyRemoveSinkPlace(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
		if(applyAgglomeration(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;

		if(applyRemoveParallelTransition(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;

		if(applyRemoveDeadTransition(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
		if(applyRemoveRedundantPlace(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
		if(applyRemoveRedundantTransition(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
		}
		if(counter >= 11) break;
		if(applyCondenseStronglyConnectedStateMachine(containsnext,containsdeadlock,type))
		{
			counter = 0;
		}
		else
		{
			counter++;
		}
		if(counter >= 11) break;
	}
//print();
}

void ProtoNet::compare(Petrinet * petrinet)
{
	if(cardPL != petrinet -> Card[PL]) RT::rep->status("diff nr of places"); 
	if(cardTR != petrinet -> Card[TR]) RT::rep->status("diff nr of trans");

	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		int p;
		for(p = 0; p < petrinet->Card[PL];p++)
		{
			if(!strcmp(ppp->sy->getKey(),petrinet->Name[PL][p])) break;
		}
		if(p >= cardPL) RT::rep->status("protoplace not in net");
		if(ppp -> marking != petrinet->Initial[p]) RT::rep->status("diff initial marking");
		if(ppp -> capacity != petrinet->Capacity[p]) RT::rep->status("diff capacity");
		ppp -> dfs = p;
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		int t;
		for(t = 0; t < petrinet->Card[TR];t++)
		{
			if(!strcmp(ttt->sy->getKey(),petrinet->Name[TR][t])) break;
		}
		if(t >= cardTR) RT::rep->status("prototransition not in net");
		ttt -> dfs = t;
	}
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		int p = ppp -> dfs;
		if(ppp -> cardPre != petrinet -> CardArcs[PL][PRE][p]) RT::rep->status("diff card arcs pre");
		if(ppp -> cardPost != petrinet -> CardArcs[PL][POST][p]) RT::rep->status("diff card arcs post");
		for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
		{
			int t = aaa -> tr -> dfs;
			int a;
			for(a = 0; a < ppp -> cardPre; a++)
			{
				if(petrinet -> Arc[PL][PRE][p][a] == t)
				{
					break;
				}
			}
			if(a > ppp -> cardPre) RT::rep->status("pre arc of place not in net");
			if(aaa -> mult != petrinet -> Mult[PL][PRE][p][a]) RT::rep->status("diff mult");
		}
		for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
		{
			int t = aaa -> tr -> dfs;
			int a;
			for(a = 0; a < ppp -> cardPost; a++)
			{
				if(petrinet -> Arc[PL][POST][p][a] == t)
				{
					break;
				}
			}
			if(a > ppp -> cardPost) RT::rep->status("post arc of place not in net");
			if(aaa -> mult != petrinet -> Mult[PL][POST][p][a]) RT::rep->status("diff mult");
		}
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		int t = ttt -> dfs;
		if(ttt -> cardPre != petrinet -> CardArcs[TR][PRE][t]) RT::rep->status("diff card arcs pre");
		if(ttt -> cardPost != petrinet -> CardArcs[TR][POST][t]) RT::rep->status("diff card arcs post");
		for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
		{
			int p = aaa -> pl -> dfs;
			int a;
			for(a = 0; a < ttt -> cardPre; a++)
			{
				if(petrinet -> Arc[TR][PRE][t][a] == p)
				{
					break;
				}
			}
			if(a > ttt -> cardPre) RT::rep->status("pre arc of place not in net");
			if(aaa -> mult != petrinet -> Mult[TR][PRE][t][a]) RT::rep->status("diff mult");
		}
		for(Arc * aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
		{
			int p = aaa -> pl -> dfs;
			int a;
			for(a = 0; a < ttt -> cardPost; a++)
			{
				if(petrinet -> Arc[TR][POST][t][a] == p)
				{
					break;
				}
			}
			if(a > ttt -> cardPost) RT::rep->status("post arc of place not in net");
			if(aaa -> mult != petrinet -> Mult[TR][POST][t][a]) RT::rep->status("diff mult");
		}
	}

}

void ProtoNet::recordVisibleNodes()
{
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		if(ppp->sy->visible == markedvisibility)
		{
			ppp -> visible = true;
		}
		else
		{
			ppp -> visible = false;
		}
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		if(ttt->sy->visible == markedvisibility)
		{
			ttt -> visible = true;
		}
		else
		{
			ttt -> visible = false;
		}
	}
}

void ProtoNet::resymbol(SymbolTable * PlaceTable, SymbolTable * TransitionTable)
{
	for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
	{
		ppp->sy = (PlaceSymbol *) (PlaceTable->lookup(ppp->sy -> getKey()));
	}
	for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
	{
		ttt->sy = (TransitionSymbol *) (TransitionTable->lookup(ttt->sy->getKey()));
	}
}

pthread_mutex_t nestedmutex = PTHREAD_MUTEX_INITIALIZER;

extern nestedunit * startunit;
void  ProtoNet::createLP()
{

//RT::rep->status("CREATE LP");
	lp = NULL;
	lprec * llp;
        //create the lp that represents the state equation for this protonet
        int Ncol, j, ret = 0;
        Ncol = currentnet->cardPL + currentnet->cardTR;
        llp = make_lp(0, Ncol);
	int Nrow = 0;
        if(llp == NULL) 
	{
//RT::rep->status("PROBLEM 1");
		return;
	}

        /* create space large enough for one row */

        REAL * row = new REAL[Ncol + 2];
        if(row == NULL)
        {
                delete_lp(llp);
		llp = NULL;
                if(row != NULL) delete [] row;
//RT::rep->status("PROBLEM 2");
                return;
        }
        /* The enumeration of variables is as follows: p0~1, p1~2,...,pMAX_PLACES-1~MAX_PLACES,t0~MAX_PLACES+1, t1~MAX_PLACES+2,... */

        /* set all variables to integer */
        for (int i=1; i<=cardPL + cardTR; i++) {
                set_int(llp, i, TRUE);
        }

	// set names

//for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next)
//{
		//set_col_name(llp, ppp->index +1 , deconst(ppp->sy->getKey()));
//if(ppp -> index < 0 || ppp -> index > cardPL) RT::rep->status("ALARM 1");
//}
//for(ProtoTransition * ttt = firsttransition; ttt; ttt = ttt -> next)
//{
//if(ttt -> index < 0 || ttt -> index > cardTR) RT::rep->status("ALARM 2");
		//set_col_name(llp,ttt->index + cardPL + 1, deconst(ttt -> sy -> getKey()));
//}

        /* set the objective function */

	std::fill(row, row + Ncol + 1, 0);
        for (int i = cardPL + 1; i <= cardPL + cardTR; i++) {
                row[i] = 1.0;
        }
        set_add_rowmode(llp, TRUE);  /* makes building the model faster if it is done rows by row */

        /* set the objective in lpsolve */
        if(!set_obj_fn(llp, row))
        {
                delete_lp(llp);
		llp = NULL;
                delete [] row;
//RT::rep->status("PROBLEM 3");
                return;
        }
        set_minim(llp);

        /* add the constraits row by row. These are the rows of the state equation*/

        /* the incidence matrix */
        for(ProtoPlace * ppp = firstplace; ppp; ppp = ppp -> next) // one equation per place
        {
//RT::rep->status("CONSIDER PL %s", ppp -> sy -> getKey());
                std::fill(row, row + Ncol + 1, 0);
                for(Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl)
                {
if(aaa -> tr-> index >= cardTR)
RT::rep->status("TRANS %d OF %d", aaa -> tr-> index, cardTR);
                        row[cardPL + 1 + aaa -> tr -> index] = 1.0 * aaa -> mult;
                }
                for(Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
                {
if(aaa -> tr-> index >= cardTR)
RT::rep->status("TRANS %d OF %d", aaa -> tr-> index, cardTR);
                        row[cardPL + 1 + aaa -> tr -> index] += -1.0 * aaa -> mult;
                }
if(ppp -> index >= cardPL)
RT::rep->status("PPPLLL %d OF %d", ppp -> index, cardPL);
		row[ppp -> index+1] = -1.0;

                int m0 = ppp->marking;
                if(!add_constraint(llp, row, EQ, -1.0 * m0))
                {
                        delete [] row;
                        delete_lp(llp);
			llp = NULL;
//RT::rep->status("PROBLEM 4");
                        return;
                }
		//set_row_name(llp,++Nrow, deconst(ppp->sy->getKey()));
        }

	/* nestedunits */

	pthread_mutex_lock(&nestedmutex);
	for(nestedunit * uuu = startunit; uuu; uuu = uuu -> nextunit)
	{
		if(!uuu->leafunit) continue;
//RT::rep->status("NEXT UNIT");
		for(nestedunit * nnn = uuu; nnn; nnn = nnn -> nextinunit)
		{
			nnn -> marked = true;
		}
                std::fill(row, row + Ncol + 1, 0);
		for(ProtoPlace * ppp = firstplace;ppp;ppp = ppp -> next)
		{
			if(!(ppp -> sy -> nested ->marked)) continue;
//RT::rep->status("INSERT PL %s", ppp -> sy -> getKey());
			row[ppp->index + 1] = 1.0;
		}
                if(!add_constraint(llp, row, LE, 1.0 ))
                {
                        delete [] row;
                        delete_lp(llp);
			llp = NULL;
                        return;
                }
		for(nestedunit * nnn = uuu; nnn; nnn = nnn -> nextinunit)
		{
			nnn -> marked = false;
		}
	}
	pthread_mutex_unlock(&nestedmutex);

        delete [] row;
	lp = llp;
//RT::rep->status("SUCCESS");
//print_lp(llp);
        return;
}

