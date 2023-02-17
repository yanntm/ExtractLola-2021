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
\author Karsten
\file
\status approved 21.02.2012
\ingroup g_frontend g_symboltable

\brief implementation of class ParserPTNet

\todo Mal den new-Operator in Bezug auf Exceptions ansehen.
*/

#include <config.h>
#include <pthread.h>
#include <config.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/ParserPnml.h>
#include <Frontend/SymbolTable/ArcList.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/Symbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Net/Petrinet.h>
#include <Net/Protonet.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>

int32_t * ParserPTNet::relabelplacemapping = NULL;
int32_t * ParserPTNet::relabeltransitionmapping = NULL;
Petrinet * ParserPTNet::relabelnet = NULL;



template<typename T>
void sortarcs(arrayindex_t * indexarray, T * multarray,arrayindex_t card)
{
        arrayindex_t pivot = indexarray[0];
        arrayindex_t blue = 0;
        arrayindex_t white = 1;
        arrayindex_t red = card;

        while(red > white)
        {
                if(indexarray[white] < pivot)
                {
                        arrayindex_t tmp = indexarray[white];
                        T tmpmult = multarray[white];
                        indexarray[white] = indexarray[blue];
                        multarray[white++] = multarray[blue];
                        indexarray[blue] = tmp;
                        multarray[blue++] = tmpmult;
                }
                else
                {
                        // indexarray[white > pivot] (assume absence of duplicates in indexarray)
                        arrayindex_t tmp = indexarray[white];
                        T tmpmult = multarray[white];
                        indexarray[white] = indexarray[--red];
                        multarray[white] = multarray[red];
                        indexarray[red] = tmp;
                        multarray[red] = tmpmult;
                }
        }
        if(blue > 1)
        {
                sortarcs<T>(indexarray,multarray,blue);
        }
        if(card - red > 1)
        {
                sortarcs<T>(indexarray + red, multarray + red, card - red);
        }
}

/*!
\post Memory is allocated for the symbol tables. It is released by the
destructor.
*/
ParserPTNet::ParserPTNet() 
{
	PlaceTable = new SymbolTable();
	TransitionTable = new SymbolTable();
	findlowstarted = findlowfinished = false;
	pthread_mutex_init(&findlowmutex,NULL);
	pthread_cond_init(&findlowcond,NULL);
}

/*!
\post Memory of the symbol tables is deallocated.
*/
ParserPTNet::~ParserPTNet()
{
    delete PlaceTable;
    delete TransitionTable;
}

void ParserPTNet::setIndices()
{
    const arrayindex_t cardPL = PlaceTable->getCard();
    const arrayindex_t cardTR = TransitionTable->getCard();
    PlaceSymbol *ps;
    arrayindex_t i;
    for ((ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->first())), (i = 0); ps;
            ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->next()), i++)
    {
        ps->setIndex(i);
    }
    TransitionSymbol *ts;
    for ((ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->first())), (i = 0); ts;
            ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->next()), i++)
    {
        ts->setIndex(i);
    }
}

void ParserPTNet::symboltable2net(Petrinet * pppnnn)
{
	if(!pppnnn) pppnnn = new Petrinet();
	Petrinet::InitialNet = pppnnn;
    /*********************************************
    * 1. Allocate memory for basic net structure *
    *********************************************/

    // 1.1 set cardinalities
    const arrayindex_t cardPL = PlaceTable->getCard();
    const arrayindex_t cardTR = TransitionTable->getCard();
    Petrinet::InitialNet->Card[PL] = cardPL;
    Petrinet::InitialNet->Card[TR] = cardTR;
    Petrinet::InitialNet->CardSignificant = UINT_MAX;  // mark as "still to be computed"

    // 1.2 allocate arrays for node (places and transitions) names, arcs, and multiplicities
    for (int type = PL; type <= TR; type ++)
    {
        Petrinet::InitialNet->Name[type] = new const char *[Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->thesymbol[type] = new Symbol * [Petrinet::InitialNet->Card[type]];
        for (int direction = PRE; direction <= POST; direction ++)
        {
            Petrinet::InitialNet->CardArcs[type][direction] = new arrayindex_t[Petrinet::InitialNet->Card[type]];
            Petrinet::InitialNet->Arc[type][direction] = new arrayindex_t *[Petrinet::InitialNet->Card[type]];
            Petrinet::InitialNet->Mult[type][direction] = new mult_t *[Petrinet::InitialNet->Card[type]];
	  
        }
        Petrinet::InitialNet->OnlyPre[type] = new arrayindex_t * [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->GreaterPre[type] = new arrayindex_t * [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->PreEqualPost[type] = new arrayindex_t * [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->GreaterPost[type] = new arrayindex_t * [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->OnlyPost[type] = new arrayindex_t * [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->CardOnlyPre[type] = new arrayindex_t  [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->CardGreaterPre[type] = new arrayindex_t  [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->CardPreEqualPost[type] = new arrayindex_t  [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->CardGreaterPost[type] = new arrayindex_t  [Petrinet::InitialNet->Card[type]];
        Petrinet::InitialNet->CardOnlyPost[type] = new arrayindex_t  [Petrinet::InitialNet->Card[type]];
    }


    /********************************
    * 2. Allocate memory for places *
    *********************************/

    Petrinet::InitialNet->Capacity = new capacity_t[cardPL];



    /**********************************
    * 3. Allocate memory for markings *
    ***********************************/

    Petrinet::InitialNet->Initial = new capacity_t[cardPL];


    /***********************************************
    * 4. Copy data from the symbol table to places *
    ************************************************/

    // fill all information that is locally available in symbols, allocate node specific arrays
    PlaceSymbol *ps;
    for ((ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->first())); ps;
            ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->next()))
    {
	int i = ps -> index;
        const arrayindex_t tempCardPre = ps->getCardPre();
        const arrayindex_t tempCardPost = ps->getCardPost();

        // we take care of the place name (not destructed by SymbolTable)
        Petrinet::InitialNet->Name[PL][i] = ps->getKey();
        Petrinet::InitialNet->thesymbol[PL][i] = ps;
        Petrinet::InitialNet->CardArcs[PL][PRE][i] = tempCardPre;
        Petrinet::InitialNet->CardArcs[PL][POST][i] = tempCardPost;

	//Pre- and post-transitions are recorded in a single array consisting of 5 separate sections:
	// 1. t | t is only pre-transition
	// 2. t | t is pre- and post-transition, W(t,p) > W(p,t)
	// 3. t | t is pre and post-transition, W(t,p) = W(p,t)
	// 4. t | t is pre and post-transition, W(t,p) < W(p,t)
	// 5. t | t is only post-transition
	// Every section may be empty.
	// The following information can be derived:
	// pre-transitions: Sec 1-4: start: Petrinet::InitialNet->Arc[PL][PRE][i], size: Petrinet::InitialNet->CardArcs[PL][PRE][i]
	// post-transition: Sec 2-5: start: Petrinet::InitialNet->Arc[PL][POST][i] = Petrinet::InitialNet->Arc[PL][PRE][i] + size(Sec1), size: Petrinet::InitialNet->CardArcs[PL][POST][i] = tempCardPost;
	// increasing transitions: Sec 1-2: start: Petrinet::InitialNet->Increasing[i] = Petrinet::InitialNet->Arc[PL][PRE][i], size: Petrinet::InitialNet->CardIncreasing[i]
	// decreasing transitions: Sec 4-5: start: Petrinet::InitialNet->Decreasing[i], size: Petrinet::InitialNet->CardDecreasing[i];

        // allocate memory for place's arcs (is copied later with transitions, size is upper approx - realloced lateron)
        Petrinet::InitialNet->Arc[PL][PRE][i] = new arrayindex_t[tempCardPre + tempCardPost];
        Petrinet::InitialNet->Arc[PL][POST][i] = Petrinet::InitialNet->Arc[PL][PRE][i] + tempCardPre;
        Petrinet::InitialNet->Mult[PL][PRE][i] = new mult_t[tempCardPre + tempCardPost];
        Petrinet::InitialNet->Mult[PL][POST][i] = Petrinet::InitialNet->Mult[PL][PRE][i] + tempCardPre;

        // capacity
        Petrinet::InitialNet->Capacity[i] = ps->getCapacity();

        // set initial marking and calculate hash
        // note: seems like overkill, but will be needed for reversibility etc.
        Petrinet::InitialNet->Initial[i] = ps->getInitialMarking();
    }

    /*************************************
    * 5. Allocate memory for transitions *
    **************************************/

    // allocate memory for static data
    Petrinet::InitialNet->Fairness = new fairnessAssumption_t[cardTR];


    /****************************************************
    * 6. Copy data from the symbol table to transitions *
    *****************************************************/

    // current_arc is used for filling in arcs and multiplicities of places
    // calloc: no arcs there yet
    arrayindex_t *current_arc_post = new arrayindex_t[cardPL];
    arrayindex_t *current_arc_pre = new arrayindex_t[cardPL];
    memset(current_arc_pre,0,cardPL * sizeof(arrayindex_t));
    memset(current_arc_post,0,cardPL * sizeof(arrayindex_t));

    TransitionSymbol *ts;
    for (ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->first()); ts;
            ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->next()))
    {
	int i = ts -> index;
        const arrayindex_t tempCardPre = ts->getCardPre();
        const arrayindex_t tempCardPost = ts->getCardPost();

        // we need to take care of the name (not destructed by SymbolTable)
        Petrinet::InitialNet->Name[TR][i] = ts->getKey();
        Petrinet::InitialNet->thesymbol[TR][i] = ts;
        Petrinet::InitialNet->CardArcs[TR][PRE][i] = tempCardPre;
        Petrinet::InitialNet->CardArcs[TR][POST][i] = tempCardPost;

        // allocate memory for transition's arcs

	// list of arcs consists of 5 separate sections:
	// 1. pure pre-places: W(t,p) = 0
	// 2. decreased places: 0 < W(t,p) < W(p,t)
	// 3. test places: 0 != W(t,p) = W(p,t)
	// 4. increased places: W(t,p) > W(p,t) > 0
	// 5. pure post-places: W(p,t) = 0
	
	// This array (a single one for every transition) is shared for 
	// - Petrinet::InitialNet->Arc[TR][PRE][i] 
	// - Petrinet::InitialNet->Arc[TR][POST][i] 
	// - Petrinet::InitialNet->DeltaT[PRE][i] 
	// - Petrinet::InitialNet->DeltaT[POST][i] 
	
        Petrinet::InitialNet->Arc[TR][PRE][i] = new arrayindex_t[tempCardPre + tempCardPost];
        Petrinet::InitialNet->Arc[TR][POST][i] = Petrinet::InitialNet->Arc[TR][PRE][i] + tempCardPre;
        Petrinet::InitialNet->Mult[TR][PRE][i] = new mult_t[tempCardPre + tempCardPost];
        Petrinet::InitialNet->Mult[TR][POST][i] = Petrinet::InitialNet->Mult[TR][PRE][i] + tempCardPre; 

        Petrinet::InitialNet->Fairness[i] = ts->getFairness();

        // copy arcs (for transitions AND places)
        ArcList *al;
        arrayindex_t j;
        for (al = ts->getPre(), j = 0; al; al = al->getNext(), j++)
        {
            const arrayindex_t k = al->getPlace()->getIndex();
            Petrinet::InitialNet->Arc[TR][PRE][i][j] = k;
            Petrinet::InitialNet->Arc[PL][POST][k][current_arc_post[k]] = i;
            Petrinet::InitialNet->Mult[PL][POST][k][(current_arc_post[k])++] = Petrinet::InitialNet->Mult[TR][PRE][i][j] = al->getMultiplicity();
        }
        for (al = ts->getPost(), j = 0; al; al = al->getNext(), j++)
        {
            const arrayindex_t k = al->getPlace()->getIndex();
            Petrinet::InitialNet->Arc[TR][POST][i][j] = k;
            Petrinet::InitialNet->Arc[PL][PRE][k][current_arc_pre[k]] = i;
            Petrinet::InitialNet->Mult[PL][PRE][k][(current_arc_pre[k])++] = Petrinet::InitialNet->Mult[TR][POST][i][j] = al->getMultiplicity();
        }
    }


    for(arrayindex_t p = 0; p < Petrinet::InitialNet->Card[PL];p++)
    {
	if(Petrinet::InitialNet->CardArcs[PL][PRE][p]) sortarcs<mult_t>(Petrinet::InitialNet->Arc[PL][PRE][p],Petrinet::InitialNet->Mult[PL][PRE][p],Petrinet::InitialNet->CardArcs[PL][PRE][p]);
	if(Petrinet::InitialNet->CardArcs[PL][POST][p])sortarcs<mult_t>(Petrinet::InitialNet->Arc[PL][POST][p],Petrinet::InitialNet->Mult[PL][POST][p],Petrinet::InitialNet->CardArcs[PL][POST][p]);
    }
    for(arrayindex_t t = 0; t < Petrinet::InitialNet->Card[TR];t++)
    {
	if(Petrinet::InitialNet->CardArcs[TR][PRE][t]) sortarcs<mult_t>(Petrinet::InitialNet->Arc[TR][PRE][t],Petrinet::InitialNet->Mult[TR][PRE][t],Petrinet::InitialNet->CardArcs[TR][PRE][t]);
	if(Petrinet::InitialNet->CardArcs[TR][POST][t])sortarcs<mult_t>(Petrinet::InitialNet->Arc[TR][POST][t],Petrinet::InitialNet->Mult[TR][POST][t],Petrinet::InitialNet->CardArcs[TR][POST][t]);
    }

    delete[] current_arc_pre;
    delete[] current_arc_post;
}

Petrinet *  ParserPTNet::protonet2net(ProtoNet * PNet)
{
	Petrinet * Net = new Petrinet();
    /*********************************************
    * 1. Allocate memory for basic net structure *
    *********************************************/

    // 1.1 set cardinalities
    const arrayindex_t cardPL = PNet -> cardPL;
    const arrayindex_t cardTR = PNet -> cardTR;
    Net->Card[PL] = cardPL;
    Net->Card[TR] = cardTR;
    Net->CardSignificant = UINT_MAX;  // mark as "still to be computed"

    // 1.2 allocate arrays for node (places and transitions) names, arcs, and multiplicities
    for (int type = PL; type <= TR; type ++)
    {
        Net->Name[type] = new const char *[Net->Card[type]];
        Net->thesymbol[type] = new Symbol *[Net->Card[type]];
        for (int direction = PRE; direction <= POST; direction ++)
        {
            Net->CardArcs[type][direction] = new arrayindex_t[Net->Card[type]];
            Net->Arc[type][direction] = new arrayindex_t *[Net->Card[type]];
            Net->Mult[type][direction] = new mult_t *[Net->Card[type]];
	  
        }
        Net->OnlyPre[type] = new arrayindex_t * [Net->Card[type]];
        Net->GreaterPre[type] = new arrayindex_t * [Net->Card[type]];
        Net->PreEqualPost[type] = new arrayindex_t * [Net->Card[type]];
        Net->GreaterPost[type] = new arrayindex_t * [Net->Card[type]];
        Net->OnlyPost[type] = new arrayindex_t * [Net->Card[type]];
        Net->CardOnlyPre[type] = new arrayindex_t  [Net->Card[type]];
        Net->CardGreaterPre[type] = new arrayindex_t  [Net->Card[type]];
        Net->CardPreEqualPost[type] = new arrayindex_t  [Net->Card[type]];
        Net->CardGreaterPost[type] = new arrayindex_t  [Net->Card[type]];
        Net->CardOnlyPost[type] = new arrayindex_t  [Net->Card[type]];
    }


    /********************************
    * 2. Allocate memory for places *
    *********************************/

    Net->Capacity = new capacity_t[cardPL];


    /**********************************
    * 3. Allocate memory for markings *
    ***********************************/

    Net->Initial = new capacity_t[cardPL];


    ProtoTransition * ttt;
	arrayindex_t i;
    for (ttt = PNet->firsttransition, (i = 0); ttt;
            ttt = ttt->next, i++)
    {
	ttt -> dfs = i;
    }
    /***********************************************
    * 4. Copy data from protonet to places *
    ************************************************/

    // fill all information that is locally available in symbols, allocate node specific arrays

    ProtoPlace * ppp;
    for(ppp = PNet -> firstplace,(i=0); ppp; ppp = ppp -> next,i++)
    {
        const arrayindex_t tempCardPre = ppp->cardPre;
        const arrayindex_t tempCardPost = ppp->cardPost;
	ppp -> dfs = i;


        // we take care of the place name (not destructed by SymbolTable)
        Net->Name[PL][i] = ppp -> sy ->getKey();
        Net->thesymbol[PL][i] = ppp -> sy;
        Net->CardArcs[PL][PRE][i] = tempCardPre;
        Net->CardArcs[PL][POST][i] = tempCardPost;

	//Pre- and post-transitions are recorded in a single array consisting of 5 separate sections:
	// 1. t | t is only pre-transition
	// 2. t | t is pre- and post-transition, W(t,p) > W(p,t)
	// 3. t | t is pre and post-transition, W(t,p) = W(p,t)
	// 4. t | t is pre and post-transition, W(t,p) < W(p,t)
	// 5. t | t is only post-transition
	// Every section may be empty.
	// The following information can be derived:
	// pre-transitions: Sec 1-4: start: Petrinet::InitialNet->Arc[PL][PRE][i], size: Petrinet::InitialNet->CardArcs[PL][PRE][i]
	// post-transition: Sec 2-5: start: Petrinet::InitialNet->Arc[PL][POST][i] = Petrinet::InitialNet->Arc[PL][PRE][i] + size(Sec1), size: Petrinet::InitialNet->CardArcs[PL][POST][i] = tempCardPost;
	// increasing transitions: Sec 1-2: start: Petrinet::InitialNet->Increasing[i] = Petrinet::InitialNet->Arc[PL][PRE][i], size: Petrinet::InitialNet->CardIncreasing[i]
	// decreasing transitions: Sec 4-5: start: Petrinet::InitialNet->Decreasing[i], size: Petrinet::InitialNet->CardDecreasing[i];

        // allocate memory for place's arcs (is copied later with transitions, size is upper approx - realloced lateron)
        Net->Arc[PL][PRE][i] = new arrayindex_t[tempCardPre + tempCardPost];
        Net->Arc[PL][POST][i] = Net->Arc[PL][PRE][i] + tempCardPre;
        Net->Mult[PL][PRE][i] = new mult_t[tempCardPre + tempCardPost];
        Net->Mult[PL][POST][i] = Net->Mult[PL][PRE][i] + tempCardPre;
        // copy arcs (for places )
        Arc *aaa;
        arrayindex_t j;
        for (aaa = ppp->firstArcPre, j = 0; aaa; aaa = aaa->next_pl, j++)
        {
            Net->Arc[PL][PRE][i][j] = aaa->tr->dfs;
            Net->Mult[PL][PRE][i][j] = aaa -> mult;
        }
        for (aaa = ppp->firstArcPost, j = 0; aaa; aaa = aaa->next_pl, j++)
        {
            Net->Arc[PL][POST][i][j] = aaa->tr->dfs;
            Net->Mult[PL][POST][i][j] = aaa->mult;
        }

        // capacity
        Net->Capacity[i] = ppp -> capacity;

        // set initial marking and calculate hash
        // note: seems like overkill, but will be needed for reversibility etc.
        Net->Initial[i] = ppp->marking;
    }

    /*************************************
    * 5. Allocate memory for transitions *
    **************************************/

    // allocate memory for static data
    Net->Fairness = new fairnessAssumption_t[cardTR];


    /****************************************************
    * 6. Copy data from the symbol table to transitions *
    *****************************************************/

    // current_arc is used for filling in arcs and multiplicities of places
    // calloc: no arcs there yet

    for (ttt = PNet->firsttransition, (i = 0); ttt;
            ttt = ttt->next, i++)
    {
        const arrayindex_t tempCardPre = ttt->cardPre;
        const arrayindex_t tempCardPost = ttt->cardPost;

        // we need to take care of the name (not destructed by SymbolTable)
        Net->Name[TR][i] = ttt->sy->getKey();
        Net->thesymbol[TR][i] = ttt->sy;
        Net->CardArcs[TR][PRE][i] = tempCardPre;
        Net->CardArcs[TR][POST][i] = tempCardPost;

        // allocate memory for transition's arcs

	// list of arcs consists of 5 separate sections:
	// 1. pure pre-places: W(t,p) = 0
	// 2. decreased places: 0 < W(t,p) < W(p,t)
	// 3. test places: 0 != W(t,p) = W(p,t)
	// 4. increased places: W(t,p) > W(p,t) > 0
	// 5. pure post-places: W(p,t) = 0
	
	// This array (a single one for every transition) is shared for 
	// - Petrinet::InitialNet->Arc[TR][PRE][i] 
	// - Petrinet::InitialNet->Arc[TR][POST][i] 
	// - Petrinet::InitialNet->DeltaT[PRE][i] 
	// - Petrinet::InitialNet->DeltaT[POST][i] 
	
        Net->Arc[TR][PRE][i] = new arrayindex_t[tempCardPre + tempCardPost];
        Net->Arc[TR][POST][i] = Net->Arc[TR][PRE][i] + tempCardPre;
        Net->Mult[TR][PRE][i] = new mult_t[tempCardPre + tempCardPost];
        Net->Mult[TR][POST][i] = Net->Mult[TR][PRE][i] + tempCardPre; 

        Net->Fairness[i] = ttt->sy -> getFairness();

        // copy arcs (for transitions )
        Arc *aaa;
        arrayindex_t j;
        for (aaa = ttt->firstArcPre, j = 0; aaa; aaa = aaa->next_tr, j++)
        {
            Net->Arc[TR][PRE][i][j] = aaa->pl->dfs;
            Net->Mult[TR][PRE][i][j] = aaa -> mult;
        }
        for (aaa = ttt->firstArcPost, j = 0; aaa; aaa = aaa->next_tr, j++)
        {
            Net->Arc[TR][POST][i][j] = aaa->pl->dfs;
            Net->Mult[TR][POST][i][j] = aaa->mult;
        }
    }

    for(arrayindex_t p = 0; p < cardPL;p++)
    {
	if(Net->CardArcs[PL][PRE][p]) sortarcs<mult_t>(Net->Arc[PL][PRE][p],Net->Mult[PL][PRE][p],Net->CardArcs[PL][PRE][p]);
	if(Net->CardArcs[PL][POST][p])sortarcs<mult_t>(Net->Arc[PL][POST][p],Net->Mult[PL][POST][p],Net->CardArcs[PL][POST][p]);
    }
    for(arrayindex_t t = 0; t < cardTR;t++)
    {
	if(Net->CardArcs[TR][PRE][t]) sortarcs<mult_t>(Net->Arc[TR][PRE][t],Net->Mult[TR][PRE][t],Net->CardArcs[TR][PRE][t]);
	if(Net->CardArcs[TR][POST][t])sortarcs<mult_t>(Net->Arc[TR][POST][t],Net->Mult[TR][POST][t],Net->CardArcs[TR][POST][t]);
    }

   return Net;
}

void ParserPTNet::symboltables2protonet()
{
	ProtoNet * nnn = ProtoNet::currentnet = new ProtoNet();
	std::map<PlaceSymbol *, ProtoPlace *> pmap;

        PlaceSymbol * ps;

	pmap.clear();
	for ((ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->first())); ps;
            ps = reinterpret_cast<PlaceSymbol *>(PlaceTable->next()))
	{
		pmap[ps] = new ProtoPlace(nnn,ps,ps->getCapacity(), ps -> getInitialMarking());
		pmap[ps]->index = ps -> index;
	}
      
	TransitionSymbol * ts;
        for (ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->first()); ts;
            ts = reinterpret_cast<TransitionSymbol *>(TransitionTable->next()))
	{
		ProtoTransition * ttt = new ProtoTransition(nnn,ts);
		ArcList *al;
		for (al = ts->getPre(); al; al = al->getNext())
		{
			new Arc(pmap[al->getPlace()], ttt, al->getMultiplicity(), PT);
		}
		for (al = ts->getPost(); al; al = al->getNext())
		{
			new Arc(pmap[al->getPlace()], ttt, al->getMultiplicity(), TP);
		}
	}
}

pthread_mutex_t visibilitymutex = PTHREAD_MUTEX_INITIALIZER;
ParserPTNet * ParserPTNet::currentsymbols = NULL;

void ParserPTNet::markVisibleNodes(kc::tFormula  F)
{
	pthread_mutex_lock(&kimwitu_mutex);
	ParserPTNet::currentsymbols = this;
	F -> unparse(myprinter, kc::markvisible);
	F -> unparse(myprinter, kc::reduction);
	pthread_mutex_unlock(&kimwitu_mutex);
}

extern tBuechiAutomata TheBuechi;

void ParserPTNet::markVisibleBuchiNodes()
{
	ProtoNet::currentvisibility++;
	ParserPTNet::currentsymbols = this;
	
	pthread_mutex_lock(&kimwitu_mutex);
	TheBuechi -> unparse(myprinter, kc::markvisible);
	pthread_mutex_unlock(&kimwitu_mutex);
	
}

void ParserPTNet::relabelFormula(Petrinet * ppn, kc::tFormula F)
{

	// F is a formula, built according to the indices recorded in symbolTable
	// ppn is a Petrinet 
	// goal of this procedure is to recompute indices in F such that 
	// they fit to ppn.
	pthread_mutex_lock(&visibilitymutex);

	
	for(arrayindex_t i = 0; i < ppn ->Card[PL];i++)
	{
		ppn->thesymbol[PL][i]->index = i;
	}
	for(arrayindex_t i = 0; i < ppn ->Card[TR];i++)
	{
		ppn->thesymbol[TR][i]->index = i;
	}
	pthread_mutex_lock(&kimwitu_mutex);
	relabelnet = ppn;
	F -> unparse(myprinter,kc::relabelformula);
	pthread_mutex_unlock(&kimwitu_mutex);
	pthread_mutex_unlock(&visibilitymutex);
}

//ParserPTNet * ParserPTNet::copy()
//{
	//ParserPTNet * result = new ParserPTNet();
	//for(Symbol * sy = PlaceTable -> first(); sy; sy = PlaceTable -> next())
	//{
		//result -> PlaceTable -> insert(((PlaceSymbol *) sy) -> copy());
	//}
	//for(Symbol * sy = TransitionTable -> first(); sy; sy = TransitionTable -> next())
	//{
		//result -> TransitionTable -> insert(((TransitionSymbol *) sy) -> copy());
	//}
	//return result;
//}

void ParserPTNet::print()
{
	std::cout << "PLACE" << std::endl;
	for(Symbol * sy = PlaceTable->first(); sy; sy = PlaceTable->next())
	{
		std::cout << "    " << sy -> getKey() <<  " (idx " << sy -> getIndex() << ") ," << " pre " << ((PlaceSymbol *) sy) -> getCardPre() << " post " << ((PlaceSymbol *) sy)-> getCardPost() << std::endl;
	}
	for(Symbol * sy = TransitionTable->first(); sy; sy = TransitionTable->next())
	{
		std::cout << "TRANSITION " << sy -> getKey() << ":" << std::endl;
		TransitionSymbol * tsy = (TransitionSymbol *) sy;
		if(tsy -> origin)
		{
			std::cout << "// origin: " << tsy -> origin -> name << tsy << std::endl;
		}
		TransitionSymbol * ttt = (TransitionSymbol *) sy;
		std::cout << "CONSUME" << std::endl;
		for(ArcList * al = ttt -> Pre; al ; al = al -> next)
		{
			std::cout << "    " << al -> getPlace() -> getKey() << " : " << al -> getMultiplicity() << std::endl;
		}
		std::cout << "PRODUCE" << std::endl;
		for(ArcList * al = ttt -> Post; al ; al = al -> next)
		{
			std::cout << "    " << al -> getPlace() -> getKey() << " : " << al -> getMultiplicity() << std::endl;
		}
	}
}

bool ParserPTNet::checkFindlowCriterion(Petrinet * n)
{
	arrayindex_t cardT = TransitionTable -> getCard();
	bool * isminimal = new bool [cardT];
	arrayindex_t * isequivalent = new arrayindex_t [cardT];
	arrayindex_t * trans = new arrayindex_t [cardT];
	arrayindex_t * fanin = new arrayindex_t [cardT];
	
	for(arrayindex_t i = 0; i < cardT; i++)
	{
		isminimal[i] = true;
		trans[i] = isequivalent[i] = i;
		fanin[i] = 0;
		TransitionSymbol * tsy = (TransitionSymbol *) n->thesymbol[TR][i];
		for(ArcList * al = tsy -> Pre; al; al = al -> next)
		{
			fanin[tsy -> index] += al -> getMultiplicity();
		}
	}

	// sort trans array according to fanin

	for(int i = 0; i < cardT -1; i++)
	{
		// find smallest value in [i, .. ;end]
		int min = i;
		for(int j = i + 1; j < cardT; j++)
		{
			if(fanin[j] > fanin[i]) min = j;
		}
		// swap min to front
		if(min != i)
		{
			arrayindex_t tmp = fanin[i];
			fanin[i] = fanin[min];
			fanin[min] = tmp;
			tmp = isequivalent[i];
			isequivalent[i] = isequivalent[min];
			isequivalent[min] = tmp;
			tmp = trans[i];
			trans[i] = trans[min];
			trans[min] = tmp;
		}
	}

	// find minimal transition classes

	for(int i = 0; i < cardT; i++) // for all transitions t
	{
		if(!isminimal[i]) continue; // only interested in minimal equivalence classes
		if(isequivalent[i] != trans[i]) continue; // is already assigned to an equivalence class

		
		for(int j = i+1; j < cardT; j++) // for all transtions t' right of t in trans array
		{
			bool skipped = false;
			ArcList * al;
			ArcList * all;
			for(al = ((TransitionSymbol *) n->thesymbol[TR][trans[i]])->Pre; al; al = al -> next) // for all preplaces of t
			{
				// find corresponding preplace for t'
				for(all = ((TransitionSymbol *) n->thesymbol[TR][trans[j]])->Pre; all; all = all -> next)
				{
					if(al -> getPlace() == all -> getPlace()) break;
				}
				if(!all || al -> getMultiplicity() != all -> getMultiplicity())
				{
					skipped = true;
					break;
				}
			} 
			if(skipped) continue;
			if(fanin[i] == fanin[j])
			{
				isequivalent[j] = trans[i];
			}
			else
			{
				isminimal[j] = false;
			}
		}
	}
	
	bool result = analyseTransitionClasses(this, trans, isequivalent, isminimal);


	// report result
	for(int i = 0; i < cardT; i++)
	{
		RT::rep -> status("TRANS %d: %s is %s, eq to %d", trans[i], n->thesymbol[TR][trans[i]]->getKey(), (isminimal[i] ? "minimal" : "not minimal"), isequivalent[i]);
	}
	return result;
}
