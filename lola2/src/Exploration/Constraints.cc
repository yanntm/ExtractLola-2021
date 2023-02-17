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

*/

#include <Core/Dimensions.h>
#include <Core/Handlers.h>
#include <Core/Runtime.h>
#include <Exploration/AutomorphismGraph.h>
#include <Exploration/Constraints.h>
#include <Frontend/Parser/ParserPTNet.h>


Create_HL_Net * Create_HL_Net::current;
Constraint_HL::Constraint_HL() 
{

}


Create_HL_Net::Create_HL_Net(ProtoNet * n, tFormula f)
{
	net = n;
	formula = f;
	cardConstraints = 0;
	elements = NULL;
	constraints = NULL;
	firstTouched = NULL;
	todo = NULL;
}

ParserPTNet * Create_HL_Net::Create_HL_Net::create_HL_Net()
{

	// index given proto net
	time_t starttime = time(NULL);
	originalplace = new ProtoPlace * [net -> cardPL];
	int index = 0;
	for(ProtoPlace * ppp = net -> firstplace; ppp; ppp = ppp -> next)
	{
		originalplace[index] = ppp;
		ppp -> index = index++;
	}
	originaltransition = new ProtoTransition * [net -> cardTR];
	index = 0;
	for(ProtoTransition * ttt = net -> firsttransition; ttt; ttt = ttt -> next)
	{
		originaltransition[index] = ttt;
		ttt -> index = index++;
	}

    // retrieve graph from net, initialize constraints data structure
    initGraph();


    // split constraints 

    initialRefinement();

    // prepare for the integrated Findlow check: We need to identify "duplicates" concering the
    // pre-set of LL transitions.

	int tablesize = 2 * net -> cardTR;
	ProtoTransition ** hashtable = new ProtoTransition * [tablesize];
	bool * duplicate = new bool[net -> cardTR];
	memset(hashtable,0,tablesize * sizeof(ProtoTransition *));
	memset(duplicate,0,net -> cardTR * sizeof(bool));

	for(ProtoTransition * ttt = net -> firsttransition; ttt; ttt = ttt -> next)
	{
		// hash pre-set; enforce order for hashing
		int hash = 0;
		int lastindex = -1;
		while(true)
		{
			int currentmin = net -> cardTR;
			int currentmult;
			for(Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
			{
				int c = aaa -> pl -> index;
				if(c <= lastindex) continue;
				if(c < currentmin) 
				{
					currentmin = c;
					currentmult = aaa -> mult;
				}
			}
			if(currentmin == net -> cardTR) break;
			hash += currentmin;
			hash += hash << 10;
			hash ^= hash >> 6;
			hash += currentmult;
			hash += hash << 10;
			hash ^= hash >> 6;
			lastindex = currentmin;
		}
		hash %=  tablesize;
                while(hash < 0) hash += tablesize;
        	while(hash >= tablesize) hash -= tablesize;

		// check in to hash table

		while(hashtable[hash])
		{
			ProtoTransition * qqq = hashtable[hash];
			Arc * aaa;
			Arc * bbb;
			for(aaa = ttt -> firstArcPre, bbb = qqq -> firstArcPre; aaa && bbb; aaa = aaa -> next_tr, bbb = bbb -> next_tr)
			{
				if(aaa -> pl != bbb -> pl || aaa -> mult != bbb -> mult) break;
			}	
			if(aaa || bbb)
			{
				// no duplicate 
				hash += 1;
				if( hash >= tablesize) hash -= tablesize;
				continue;
			}
			// here: duplicate
			duplicate[ttt->index] = true;
			break;
		}
		if(!duplicate[ttt->index])
		{
			hashtable[hash] = ttt;
		}
	}
	delete [] hashtable;


    // create skeleton net structures from constraints

	ParserPTNet * Skeleton = new ParserPTNet();

	// collect places
	placemapping = new PlaceSymbol * [net -> cardPL];
	index = 0;
	for(int i = 0; i < cardConstraints; i++)	
	{
		arrayindex_t id = elements[constraints[i].from]->id;
		if(id >= net -> cardPL) continue; // skip transition classes
		PlaceSymbol * ps = new PlaceSymbol(originalplace[id]->sy->getKey(),MAX_CAPACITY);
		Skeleton->PlaceTable -> insert(ps);
		ps -> size = 0;
		int initial = 0;
		for(int j = constraints[i].from; j <= constraints[i].to; j++)
		{
			arrayindex_t idd = elements[j]->id;
			initial += originalplace[idd]->marking;
			placemapping[idd] = constraints[i].sy = ps;
			(ps -> size)++;
		}
		ps -> addInitialMarking(initial);
		ps -> setIndex(index++);
		constraints[i].sy = ps;
	}


	// collect transitions
	transitionmapping = new TransitionSymbol * [net -> cardTR];
	for(int i = 0; i < cardConstraints; i++)	
	{
		arrayindex_t id = elements[constraints[i].from]->id;
		if(id < net -> cardPL) continue; // skip place classes
		id -= net -> cardPL;
		ProtoTransition * ttt = originaltransition[id];
		TransitionSymbol * ts = new TransitionSymbol(ttt->sy->getKey(), NO_FAIRNESS,NULL,NULL);
		ts -> size = 0;
		for(int j = constraints[i].from; j <= constraints[i].to; j++)
		{
			arrayindex_t idd = elements[j]->id - net -> cardPL;
			transitionmapping[idd] = constraints[i].tsy = ts;
			if(!duplicate[idd]) (ts -> size) ++;
		}
		Skeleton -> TransitionTable -> insert(ts);
		
		// collect arcs
		
		// incoming
		for(Arc *  aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr) 
		{
			ProtoPlace * ppp = aaa -> pl;
			PlaceSymbol * ps = placemapping[ppp->index];
			ArcList * al;
			for(al = ts -> Pre; al; al = al -> next)
			{
				if(al -> getPlace() == ps) break;
			}
			if(al)
			{
				al->addMultiplicity(aaa->mult);
			}
			else
			{
				al = new ArcList(ps, aaa->mult);
				al -> setNext (ts -> Pre);
				ts -> Pre = al;
				ts -> cardPre++;
				ps -> notifyPost();
			}
		}

		// outgoing
		for(Arc * aaa = ttt -> firstArcPost; aaa ; aaa = aaa -> next_tr)
		{
			ProtoPlace * ppp = aaa -> pl;
			PlaceSymbol * ps = placemapping[ppp->index];
			ArcList * al;
			for(al = ts -> Post; al; al = al -> next)
			{
				if(al -> getPlace() == ps) break;
			}
			if(al)
			{
				al -> addMultiplicity(aaa->mult);
			}
			else
			{
				al = new ArcList(ps, aaa -> mult);
				al -> setNext (ts -> Post);
				ts -> Post = al;
				ts -> cardPost++;
				ps -> notifyPre();
			}
		}
	}

	delete [] duplicate;

	// analyse Findlow criterion

	// find mininmal transition classes

        arrayindex_t cardT = Skeleton -> TransitionTable -> getCard();
        bool * isminimal = new bool [cardT];
        arrayindex_t * isequivalent = new arrayindex_t [cardT];
        arrayindex_t * trans = new arrayindex_t [cardT];
        arrayindex_t * fanin = new arrayindex_t [cardT];
	TransitionSymbol ** hlt = new TransitionSymbol * [cardT];

	int idx = 0;
	for(Symbol * sy = Skeleton->TransitionTable->first(); sy; sy = Skeleton->TransitionTable->next())
	{
		hlt[idx++] = (TransitionSymbol *) sy;
	}

	

        for(arrayindex_t i = 0; i < cardT; i++)
        {
                isminimal[i] = true;
                trans[i] = isequivalent[i] = i;
                fanin[i] = 0;
                TransitionSymbol * tsy = hlt[i];
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
                        for(al = (hlt[trans[i]])->Pre; al; al = al -> next) // for all preplaces of t
                        {
                                // find corresponding preplace for t'
                                for(all = (hlt[trans[j]])->Pre; all; all = all -> next)
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
	delete [] fanin;

	// compute Findlow values
	Skeleton -> findlowvalue = true;
 	for(int index = 0; index < cardT; index++)
        {
                // consider representative ttt of equivalence class

                TransitionSymbol * tsy = hlt[trans[index]];
		tsy -> findlow = true;
                if(!isminimal[index]) continue;
                if(trans[index] != isequivalent[index]) continue;
                arrayindex_t tttindex = trans[index];

		// compute number of firing modes required by places

		int placemodes = 1;
		for(ArcList * a = tsy -> Pre; a; a = a -> next)
		{
			int mult = a -> multiplicity;
			int card = a -> getPlace() -> size;
			int factor = 1;
			for(int i = 0; i < mult; i++)
			{
				factor *= card--;
			}
			for(int i = 2; i <= mult; i++)
			{
				factor /= i;
			}
			placemodes *= factor;
		}

		// compute number of firing modes provided by transitions

		int transitionmodes = 0;
                for(int i = 0; i < cardT; i++)
                {
                        if(isequivalent[i] != tttindex) continue;
                        TransitionSymbol * usy = hlt[trans[i]];
			transitionmodes += usy -> size;
		}
		if(placemodes != transitionmodes)
		{
			//RT::rep->status("Findlow criterion violated for transition class containing %s", tsy -> getKey());
			Skeleton -> findlowvalue = tsy -> findlow = false;
			for(int i = 0; i < cardT; i++)
			{
				if(isequivalent[i] != tttindex) continue;
				TransitionSymbol * usy = hlt[trans[i]];
				usy -> findlow = false;
			}
		}
	}				
	Skeleton -> setIndices();
	RT::rep->status("Created skeleton in %f secs.", difftime(time(NULL), starttime));
	return Skeleton;

}

tFormula Create_HL_Net::create_HL_Formula(tFormula f)
{
	tFormula result = portfoliomanager::copyFormula(f);
	pthread_mutex_lock(&kimwitu_mutex);
	result -> unparse(myprinter, kc::foldformula);
	pthread_mutex_unlock(&kimwitu_mutex);
	return result;
	
}


void Create_HL_Net::initGraph()
{

    // 1. Initialization

    // 1.1 Get size of graph
    // #vertices = #P + #T 
     arrayindex_t cardVertices = net->cardPL + net->cardTR;

    // 1.2 allocate memory for calculator data structures

    elements = new Vertex_HL * [cardVertices];
    constraints = new Constraint_HL[cardVertices];

    // 1.3 fill places into elements array and create graph nodes for places

    // check list
    // have: nodes  arcs
    //       P: no      to T: no    from T: no  
    //       T: no      to P: no    from P: no

    for (ProtoPlace * ppp = net -> firstplace; ppp; ppp = ppp -> next) // for all places do
    {
        // create vertex
        Vertex_HL *v = new Vertex_HL();
        v->id = ppp->index;
        v->color = ppp->marking;
        v->cardSucc = ppp -> cardPost;
        v->cardPre = ppp->cardPre;

        // the actual arc arrays will be handled later, since
        // at this time, it is unclear how many formula nodes
        // connect to the place.
        elements[ppp->index] = v;
    }

    // check list
    // have: nodes  arcs
    //       P: yes     to T: no    from T: no     
    //       T: no      to P: no    from P: no

    // 1.4 fill transitions into elements array and create graph nodes
    for (ProtoTransition * ttt = net -> firsttransition; ttt; ttt = ttt -> next)
    {
        // create vertex
        Vertex_HL *v = new Vertex_HL();
        v->id = ttt -> index + net->cardPL;
        v->color = ttt -> sy -> getFairness();
        v->cardSucc = ttt -> cardPost;
        v->cardPre = ttt -> cardPre;

        // Transitions are only connected to places. Hence,
        // actual Pre + Succ arrays can be filled now.

        v->succ = new Vertex_HL * [v->cardSucc];
        v->colorSucc = new arrayindex_t[v->cardSucc];
        v->pre = new Vertex_HL * [v->cardPre];
        v->colorPre = new arrayindex_t[v->cardPre];
	int j = 0;
        for (Arc * aaa = ttt -> firstArcPost; aaa ; aaa = aaa -> next_tr)
        {
            v->succ[j] = elements[aaa -> pl -> index];
            v->colorSucc[j++] = aaa -> mult;
        }
	j = 0;
        for (Arc * aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
        {
            v->pre[j] = elements[aaa -> pl -> index];
            v->colorPre[j++] = aaa->mult;
        }
        elements[ttt -> index + net->cardPL] = v;
    }


    // check list
    // have: nodes  arcs
    //       P: yes     to T: no    from T: no      
    //       T: yes     to P: yes   from P: yes

    // 1.8 allocate edges for places and connect to transitions

    {
        for (ProtoPlace * ppp = net -> firstplace; ppp; ppp = ppp -> next)
        {
            Vertex_HL *v = elements[ppp->index];
            v->succ = new Vertex_HL * [v->cardSucc];
            v->colorSucc = new arrayindex_t[v->cardSucc];
            v->pre = new Vertex_HL * [v->cardPre];
            v->colorPre = new arrayindex_t[v->cardPre];
	    int j = 0;
            for (Arc * aaa = ppp -> firstArcPost; aaa; aaa = aaa -> next_pl)
            {
                v->succ[j] = elements[aaa->tr -> index + net->cardPL];
                v->colorSucc[j++] = aaa->mult;
            }
	     j = 0;
            for (Arc * aaa = ppp -> firstArcPre; aaa; aaa = aaa -> next_pl) 
            {
                v->pre[j] = elements[aaa-> tr -> index + net->cardPL];
                v->colorPre[j++] = aaa->mult;
            }
        }
    }

    // check list
    // have: nodes  arcs
    //       P: yes     to T: yes   from T: yes   
    //       T: yes     to P: yes   from P: yes

    // 1.10 sort all arc arrays by increasing colors

    for (arrayindex_t i = 0; i < cardVertices; i++)
    {
        Vertex_HL *v = elements[i];
        if (v->cardSucc > 1)
        {
            v->sortArcs(v->succ, v->colorSucc, 0, v->cardSucc);
        }
        if (v->cardPre > 1)
        {
            v->sortArcs(v->pre, v->colorPre, 0, v->cardPre);
        }
    }

    cardConstraints = 0;
    // 2. initialize constraints
    // We start with two constraints: (P),(T)
    if (net->cardPL)
    {
        // there are places

        // index: number of constraint
        constraints[0].from = 0;
        constraints[0].to = net->cardPL - 1;
        constraints[0].unprocessed = true;
        for (arrayindex_t i = 0; i < net->cardPL; i++)
        {
            elements[i]->inConstraint = constraints /* = &(constraints[0]) */ ;
        }
    }
    if (net->cardTR)
    {
        // if there are transitions
        constraints[1].from = net->cardPL;
        constraints[1].to = net->cardPL + net->cardTR - 1;
        constraints[1].unprocessed = true;
        for (arrayindex_t i = 0; i < net->cardTR; i++)
        {
            elements[net->cardPL + i]->inConstraint = constraints + 1 /* = &(constraints[1]) */;
        }
        cardConstraints = 2;
    }

}

void Create_HL_Net::sort(arrayindex_t from, arrayindex_t to)
{
    arrayindex_t blue = from;
    arrayindex_t white = from + 1;
    arrayindex_t red = to;

    int pivot = elements[from]->property;
    while (red >= white)
    {
        if (elements[white]->property < pivot)
        {
            // sort into blue area
            Vertex_HL *tmp = elements[white];
            elements[white++] = elements[blue];
            elements[blue++] = tmp;
        }
        else
        {
            if (elements[white]->property == pivot)
            {
                // sort into white area
                ++white;
            }
            else
            {
                //sort into red area
                Vertex_HL *tmp = elements[white];
                elements[white] = elements[red];
                elements[red--] = tmp;
            }
        }
    }
    // sort blue area
    if (blue - from >= 2)
    {
        sort(from, blue - 1);
    }
    // sort red area
    if (to - red >= 2)
    {
        sort(red + 1, to);
    }
}

bool Create_HL_Net::initialSplit(Constraint_HL *const c)
{
    // separates an existing initial constraint into many,
    // defined by the entry in the property field of the
    // Element data structure

    // 1. sort elements according to property
    sort(c->from, c->to);

    // 2. while exist differences: append new constraint

    // return value: actual split happened
    arrayindex_t return_value = false;
    while (elements[c->from]->property != elements[c->to]->property)
    {
        return_value = true;
        // one more split required

        // search split point
        arrayindex_t i;
        for (i = c->from + 1; elements[i]->property == elements[c->from]->property;
                i++)
        {}

        // add new constraint
        assert(i <= c->to);
        constraints[cardConstraints].from = c->from;
        constraints[cardConstraints].to = i - 1;
        for (arrayindex_t j = constraints[cardConstraints].from;
                j <= constraints[cardConstraints].to; j++)
        {
            elements[j]->inConstraint = constraints + cardConstraints;
        }
        constraints[cardConstraints].touched = false;
        constraints[cardConstraints++].unprocessed = true;
        firstTouched = NULL;


        // update old constraint
        c->from = i;
    }
    c -> touched = false;
    return return_value;
}

void Create_HL_Net::initialArcRefine()
{
    // for all constraints c' and all edge colors x, we
    // count, for all nodes n in c', how many incoming (outgoing)
    // edges n has with source (target) in c and color x. If that number
    // is different for two nodes in c', they are separated into
    // different constraints as no symmetry can map one to the other.

    // We rely that, by prior computation,
    // - all nodes in c have the same number of incoming/outgoing edges
    // - for all nodes in c, the color of the i-th edge in the Pre/post
    //   list is the same.
    // - the Pre/Post arrays are sorted by color

    Constraint_HL *c = todo;
    todo = todo->next; // this constraint will be processed in the end.
    c->unprocessed = false;

    Vertex_HL *reference = elements[c->from];  // a typical vertex in c

reentry: // used for restart if c itself has been changed

    // process all incoming edges;
    for (arrayindex_t j = 0; j < reference->cardPre;) // for all edge colors do
    {
        firstTouched = NULL;
        arrayindex_t color = reference ->colorPre[j];
        // now we start a counting process
        do // for all edges with this color
        {
            for (arrayindex_t k = c->from; k <= c->to; k++)
            {
                ++(elements[k]->pre[j]->property);
                Constraint_HL *cc = elements[k]->pre[j]->inConstraint;
                if (!cc->touched)
                {
                    cc->touched = true;
                    cc->nextTouched = firstTouched;
                    firstTouched = cc;
                }
            }
        }
        while ((++j) < reference->cardPre && reference->colorPre[j] == color);

        // check effect of counting for all constraints
        arrayindex_t currentCardConstraint = cardConstraints;
        for (Constraint_HL *l = firstTouched; l;  l = l -> nextTouched)
        {
            l->touched = false;
            bool splitted = initialSplit(l);
            if (splitted)
            {
                // append l to
                // unprocessed list
                if (!(l->unprocessed))
                {
                    l->unprocessed = true;
                    if (l != c)
                    {
                        l->next = todo;
                        todo = l;
                    }
                }
            }
        }

        // append new constraints to unprocessed list
        for (arrayindex_t m = currentCardConstraint; m < cardConstraints; m++)
        {
            constraints[m].next = todo;
            todo = constraints + m;
            constraints[m].unprocessed = true;
        }

        if (c->unprocessed)
        {
            c -> unprocessed = false;
            goto reentry;
        }
    }

    // process all outgoing edges;

    for (arrayindex_t j = 0; j < reference->cardSucc;)
    {
        firstTouched = NULL;
        arrayindex_t color = reference->colorSucc[j];
        // now we start a counting process
        do // for all edges with this color
        {
            for (arrayindex_t k = c->from; k <= c->to; k++)
            {
                ++(elements[k]->succ[j]->property);
                Constraint_HL *cc = elements[k]->succ[j]->inConstraint;
                if (!cc->touched)
                {
                    cc -> touched = true;
                    cc -> nextTouched = firstTouched;
                    firstTouched = cc;
                }
            }
        }
        while ((++j) < reference->cardSucc && reference->colorSucc[j] == color);

        // check effect of counting for all constraints
        arrayindex_t currentCardConstraint = cardConstraints;
        for (Constraint_HL *l = firstTouched; l ; l = l->nextTouched)
        {
            l->touched = false;
            bool splitted = initialSplit(l);
            if (splitted)
            {
                // append l to
                // unprocessed list
                if (!(l->unprocessed))
                {
                    l->unprocessed = true;
                    if (l != c)
                    {
                        l->next = todo;
                        todo = l;
                    }
                }
            }
        }

        // append new constraints to unprocessed list
        for (arrayindex_t m = currentCardConstraint; m < cardConstraints; m++)
        {
            constraints[m].next = todo;
            todo = constraints + m;
            constraints[m].unprocessed = true;
        }
        if (c->unprocessed)
        {
            c -> unprocessed = false;
            goto reentry;
        }
    }
}


void Create_HL_Net::initialRefinement()
{
    // the original constraint (P),(T) is further split
    // according to
    // - colors of nodes
    // - number of incoming/outgoing edges
    // - number of incoming/outgoing edges with same color

    // 1. split according to node color

    // 1.1. load property field with node color

    for (arrayindex_t i = 0; i < net->cardPL + net->cardTR; i++)
    {
        elements[i]->property = elements[i]->color;
    }

    // 1.2 split all existing constraints
    arrayindex_t currentCardConstraints = cardConstraints;
    for (arrayindex_t i = 0; i < currentCardConstraints; i++)
    {
        initialSplit(constraints + i);
    }

    // 2. split according to number of incoming arcs
    // 2.1. load property field with number of incoming arcs

    for (arrayindex_t i = 0; i < net->cardPL + net->cardTR ; i++)
    {
        elements[i]->property = elements[i]->cardPre;
    }

    // 2.2 split all existing constraints
    currentCardConstraints = cardConstraints;
    for (arrayindex_t i = 0; i < currentCardConstraints; i++)
    {
        initialSplit(constraints + i);
    }

    // 3. split according to number of outgoing arcs
    // 3.1. load property field with number of outgoing arcs

    for (arrayindex_t i = 0; i < net->cardPL + net->cardTR; i++)
    {
        elements[i]->property = elements[i]->cardSucc;
    }

    // 3.2 split all existing constraints
    currentCardConstraints = cardConstraints;
    for (arrayindex_t i = 0; i < currentCardConstraints; i++)
    {
        initialSplit(constraints + i);
    }

    // 4. Split according to number of edges with same color
    // We rely on the following observations for the present
    // data structures:
    // (a) By step 2 and 3, all nodes inside one constraint
    //     have the same number of outgoing and incoming edges
    // (b) Edge lists are sorted by colors. Hence, two nodes
    //     have the same number of edges with same color (for
    //     all colors) if and only if for every j, the color of
    //     the j-th edge is the same for both nodes.

    // We have three nested loops:
    // - For all currently present constraints C do (implemented by
    //   the index i from 0 to currentCardConstraints)
    // - For all positions in the arc list (implemented by index j)
    // - for all constraints that have resulted from C (implemented
    //   through the next pointer in the Constraint record)

    // for all currently existing constraints
    currentCardConstraints = cardConstraints;
    for (arrayindex_t i = 0; i < currentCardConstraints; i++)
    {
        arrayindex_t fanin = elements[constraints[i].from]->cardPre;
        constraints[i].next = NULL;
        todo = constraints + i;
        for (arrayindex_t j = 0; j < fanin; j++)
        {
            arrayindex_t firstnewconstraint = cardConstraints;
            for (Constraint_HL *c = todo; c ; c = c->next)
            {
                // fill property field with color of j-th edge
                for (arrayindex_t k = c->from;  k <= c->to; k++)
                {
                    elements[k]->property =
                        elements[k]->colorPre[j];
                }
                // split
                initialSplit(c);
            }
            if (firstnewconstraint != cardConstraints)
            {
                for (arrayindex_t l = firstnewconstraint; l < cardConstraints - 1; l++)
                {
                    constraints[l].next = constraints + (l + 1);
                }
                constraints[cardConstraints - 1].next = todo;
                todo = constraints + firstnewconstraint;
            }
        }
    }

    // split according to the atomic propositions of the formula

    if(formula) 
    {
	    pthread_mutex_lock(&kimwitu_mutex);
	    current = this;
	    formula -> unparse(myprinter, kc::createskeleton);
	    pthread_mutex_unlock(&kimwitu_mutex);
    }

    // Now, we prepare for the (still initial, hence symmetric) refinement
    // according to the relation between two constraints.
    // To this end, we need to initialize the todo/next link.

    for (arrayindex_t i = 0; i < cardConstraints - 1; i++)
    {
        constraints[i].next = constraints + (i + 1);
        constraints[i].unprocessed = true;
        constraints[i].touched = false;
    }
    firstTouched = NULL;
    todo = constraints;
    constraints[cardConstraints - 1].next = NULL;
    constraints[cardConstraints - 1].touched = false;
    while (todo)
    {
        initialArcRefine();
    }
}



void Create_HL_Net::printId(arrayindex_t id)
{
  if(id < net->cardPL)
  {
      std::cout << originalplace[id]->sy->getKey(); return;
  }
  if(id < net->cardPL+net->cardTR)
  {
      std::cout << originaltransition[id - net->cardPL]->sy->getKey(); return;
  }
  std::cout << id-(net->cardPL+net->cardTR); return;
}
//
void Create_HL_Net::printGraph()
{
  std::cout << std::endl;
  for(arrayindex_t i = 0; i < net->cardPL+net->cardTR;i++)
  {
      Vertex_HL * vv = elements[i];
      // print node
      printId(vv->id);
      std::cout << " " << vv->color << std::endl;

      // print succs
      for(arrayindex_t j = 0; j < vv->cardSucc; j++)
      {
          std::cout << "\t-->";
          printId(vv->succ[j]->id);
          std::cout << " " << vv->colorSucc[j] << std::endl;
      }
      // print pres
      for(arrayindex_t j = 0; j < vv->cardPre; j++)
      {
          std::cout << "\t<--";
          printId(vv->pre[j]->id);
          std::cout << " " << vv->colorPre[j] << std::endl;
      }
  }
}

