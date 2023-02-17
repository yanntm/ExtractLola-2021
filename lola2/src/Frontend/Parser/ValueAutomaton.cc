#include<Frontend/Parser/ValueAutomaton.h>
#include<Core/Runtime.h>

int ValueAutomatonState::maxid;

producthashtable::producthashtable()
{
	// we are hashing pairs (id1,id2) of states
	// purpose: identify copies in product constructions
	card = 0;
	size = 1024;
	statetable = new ValueAutomatonState * [size];
	id1table = new int  [size];
	id2table = new int  [size];
	memset(statetable,0,sizeof(ValueAutomatonState *) * size);
}

minimisehashtable::minimisehashtable()
{
	card = 0;
	size = 1024;
	statetable = new ValueAutomatonState * [size];
	memset(statetable,0,sizeof(ValueAutomatonState *) * size);
}

minimisehashtable::~minimisehashtable()
{
	delete [] statetable;
}

producthashtable::~producthashtable()
{
	delete [] statetable;
	delete [] id1table;
	delete [] id2table;
}

int producthashtable::getHash(int x, int y)
{
	int h = x;
	h += (h << 10);
	h ^= (h >> 6);
	h += y;
	h += (h << 10);
	h ^= (h >> 6);
	h += (h << 3);
	h ^= (h >> 11);
	h += (h << 15);

	h %=  size;
	while(h < 0) h += size;
	while(h >= size) h -= size;
	return h;
}

int minimisehashtable::getHash(ValueAutomatonState * s)
{
	int h = 0;
	for(ValueAutomatonTransition * t = s -> transition; t; t = t -> next)
	{
		h += t -> lower;
		h += (h << 10);
		h ^= (h >> 6);
		h += t -> upper;
		h += (h << 10);
		h ^= (h >> 6);
		h += t -> succ -> id;
		h += (h << 10);
		h ^= (h >> 6);
	}
	h += (h << 3);
	h ^= (h >> 11);
	h += (h << 15);

	h %=  size;
	while(h < 0) h += size;
	while(h >= size) h -= size;
	return h;
}

void producthashtable::insert(int x, int y, ValueAutomatonState * s)
{
	// first, extend overcrowded table
	if((card * 100) / size > 90)  // more than 90% of cell populated
	{
		ValueAutomatonState ** oldstatetable = statetable;
		int * oldid1table = id1table;
		int * oldid2table = id2table;
		int oldsize = size;
		card = 0;
		size *= 2;
		statetable = new ValueAutomatonState * [size];
		id1table = new int  [size];
		id2table = new int  [size];
		memset(statetable,0,sizeof(ValueAutomatonState *) * size);
		for(int i = 0; i < oldsize; i++)
		{
			if(!oldstatetable[i]) continue;
			insert(oldid1table[i], oldid2table[i], oldstatetable[i]);
		}
		
	}
	int key = getHash(x,y);
	while(statetable[key]) 
	{
		key +=7;
		if(key >= size) key -= size;
	}
	statetable[key] = s;
	id1table[key] = x;
	id2table[key] = y;
	card++;
}

void minimisehashtable::insert(ValueAutomatonState * s)
{
	// first, extend overcrowded table
	if((card * 100) / size > 90)  // more than 90% of cell populated
	{
		ValueAutomatonState ** oldstatetable = statetable;
		int oldsize = size;
		card = 0;
		size *= 2;
		statetable = new ValueAutomatonState * [size];
		memset(statetable,0,sizeof(ValueAutomatonState *) * size);
		for(int i = 0; i < oldsize; i++)
		{
			if(!oldstatetable[i]) continue;
			insert(oldstatetable[i]);
		}
		
	}
	int key = getHash(s);
	while(statetable[key]) 
	{
		key +=7;
		if(key >= size) key -= size;
	}
	statetable[key] = s;
	card++;
}

ValueAutomatonState * producthashtable::lookup(int x, int y)
{
	int key = getHash(x,y);
	while(statetable[key])
	{
		if(id1table[key] == x && id2table[key] == y)
		{
			return statetable[key];
		}
		key += 7;
		if(key >= size) key -= size;
	}
	return NULL;
}

ValueAutomatonState * minimisehashtable::lookup(ValueAutomatonState * s)
{
	ValueAutomatonTransition * t1 = s -> transition;
	int key = getHash(s);
	while(statetable[key])
	{
		bool identical = true;
		ValueAutomatonTransition * t2 = statetable[key]->transition;
		while(t1 && t2)
		{
			if(t1 -> lower != t2 -> lower || t1 -> upper != t2 -> upper || t1 -> succ -> id != t2 -> succ -> id)
			{
				identical = false;
				break;
			}
			t1 = t1 -> next;
			t2 = t2 -> next;
		}
		if(identical && !t1 && !t2)
		{
			return statetable[key];
		}
		key += 7;
		if(key >= size) key -= size;
	}
	return NULL;
}

void ValueAutomaton::init()
{
	ValueAutomatonState::maxid = 1;
}

ValueAutomaton::ValueAutomaton()
{
	// create automaton that accepts empty language
	depth = 0;
	alphabet = NULL;
	vars = NULL;
	initialState = new ValueAutomatonState();
	initialState -> final = false;
	initialState -> transition = NULL;
	initialState -> next_sibling = NULL;
	level = new ValueAutomatonState * [1];
	level[0] = initialState;
}

ValueAutomatonState * ValueAutomaton::makeValueAutomatonState(producthashtable * pht, ValueAutomatonState * oldstate1, ValueAutomatonState * oldstate2, ValueAutomatonState ** position)
{
	ValueAutomatonState * result = pht->lookup((oldstate1 ? oldstate1->id : 0), (oldstate2 ? oldstate2->id : 0));
	if(result) return result;
	result = new ValueAutomatonState();
	result -> oldstate1 = oldstate1;
	result -> oldstate2 = oldstate2;
	result -> next_sibling = * position;
	* position = result;
	result -> transition = NULL;
	pht->insert((oldstate1 ? oldstate1->id : 0), (oldstate2 ? oldstate2->id : 0),result);
	return result;
	
}
ValueAutomatonState::ValueAutomatonState()
{
	id = maxid++;
	final = false;
	value = 0;
	transition = NULL;
	next_sibling = NULL;
}

ValueAutomaton::ValueAutomaton(hlterm * term, hlvariable ** list, int cardlist)
{
	bool * occurring = term -> getvars(list,cardlist);
	int position;
	for(position = 0; position < cardlist; position++)
	{
		if(occurring[position]) break;
	}

	// care about special case of constant term (no occurring vars)
	if(position  >= cardlist)
	{
		depth = 0;
		alphabet = NULL;
		vars = NULL;
		level = new ValueAutomatonState * [1];
		initialState = new ValueAutomatonState;
		initialState -> final = true;
		initialState -> value = term -> evaluate(NULL, list, cardlist); // no variable, no problem
		initialState -> transition = 0;
		initialState -> next_sibling = NULL;
		level[0] = initialState;
		return;
	}

	// now: have variable -> arrays have nonzero length
	depth = 1;
	hlvariable * myvar = list[position];
	alphabet = new sort * [1];
	alphabet[0] = myvar->so;
	vars = new hlvariable * [1];
	vars[0] = myvar;
	level = new ValueAutomatonState * [2];
	int * ass = new int [cardlist];
	memset(ass, 0, depth * sizeof(int));
	initialState = level[0] = new ValueAutomatonState();
	initialState -> next_sibling = NULL;
	initialState -> transition = NULL;
	initialState -> final = false;
	level[0] = initialState;
	level[1] = NULL;

	ValueAutomatonTransition ** anchor = &(initialState -> transition);
	for(int i = 0; i < myvar -> so -> size; i++)
	{
		ValueAutomatonState * sss = new ValueAutomatonState;
		sss -> transition = NULL;
		sss -> next_sibling = level[1];
		level[1] = sss;
		sss -> final = true;
		ass[position] = i;
		sss -> value = term -> evaluate(ass,list,cardlist);
		ValueAutomatonTransition * ttt = new ValueAutomatonTransition();
		ttt -> lower = ttt -> upper = i;
		ttt -> next = NULL;
		ttt -> succ = sss;
		(* anchor) = ttt;
		anchor = &(ttt -> next); 
		
	}
	return;
}

ValueAutomaton::ValueAutomaton(hlvariable * vvv, hlvariable ** list, int cardlist)
{
	depth = 1;
	alphabet = new sort * [1];
	vars = new hlvariable * [1];
	vars[0] = vvv;
	alphabet[0] = vvv -> so;
	level = new ValueAutomatonState * [2];
	initialState = level[0] = new ValueAutomatonState();
	initialState -> next_sibling = NULL;
	initialState -> final = false;
	initialState->transition = NULL;
	level[1] = NULL;
	for(int i = vvv -> so -> size - 1; i >= 0; i--)
	{
		ValueAutomatonState * sss = new ValueAutomatonState();
		sss -> next_sibling = level[1];
		sss -> value = i;
		sss -> transition = NULL;
		sss -> final = true;
		level[1] = sss;
		ValueAutomatonTransition * ttt = new ValueAutomatonTransition();
		ttt -> next = initialState->transition;
		initialState->transition = ttt;
		ttt -> lower = ttt -> upper = i;
	}
}

void ValueAutomaton::andProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{

	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}

		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( ((currentlevel1 == depth) || (vars[currentlevel1] != ordering[orderingindex])) && ((currentlevel2 == other -> depth) || (other->vars[currentlevel2] != ordering[orderingindex]))) 
		{
			orderingindex++;
		}
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex] ))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ,newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	delete pht;
	initialState = newInitial;
	minimise();
	delete other;
}

void ValueAutomaton::orProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{
	// oldstate i = NULL means that given prefix is not acceptable in operand i.
	// that is, acceptance is fully left to operand 1-i.
	// both oldstates != 0 means that the given prefix is still in the intersection
	// of the operand languages.


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState,other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] !=  other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				int firstuncovered = 0;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				if(!currentstate -> oldstate2)	
				{
					// we are accepting sequence in L1 \ L2 -> make regular move in this
					for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
					{
						ValueAutomatonState * newstate = makeValueAutomatonState(pht,ttt->succ,currentstate->oldstate2, newlevel + currentlevel + 1);
						ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
						newttt -> lower = ttt -> lower;
						newttt -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						newttt -> succ = newstate;
						newttt -> next = NULL;
						* anchor = newttt;
						anchor = &(newttt -> next);
					}
				}
				else if(!currentstate -> oldstate1)
				{
					// we are accepting sequence in L2 \ L1 -> make universal move keeping oldstate2
					ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * ttt = new ValueAutomatonTransition();
					* anchor = ttt;
					ttt -> lower = 0;
					ttt -> upper = newalphabet[currentlevel]->size - 1;
					firstuncovered = newalphabet[currentlevel]->size;
					ttt -> next = NULL;
					ttt -> succ = sss;
					anchor = &(ttt -> next);
				}
				else
				{
					// we are accepting sequence in L1 cap L2 -> make move to stay in L1 cap L2
					// and, if necessary, make move to L2 \ L1 for transitions not in A1.
					ValueAutomatonTransition ** anchor = &(currentstate -> transition);
					for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
					{
						if(firstuncovered < ttt -> lower)
						{
							// need to insert a transition to L2 \ L1
							ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL,currentstate->oldstate2, newlevel + currentlevel + 1);
							ValueAutomatonTransition * t = new ValueAutomatonTransition();
							t -> lower = firstuncovered;
							t -> upper = ttt -> lower - 1;
							firstuncovered = ttt -> lower;
							t -> succ = sss;
							t -> next = NULL;
							(*  anchor) = t;
							anchor = &(t -> next);
						}
						ValueAutomatonState * newstate = makeValueAutomatonState(pht,ttt->succ,currentstate->oldstate2, newlevel + currentlevel +1);
						ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
						newttt -> lower = ttt -> lower;
						newttt -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						newttt -> succ = newstate;
						newttt -> next = NULL;
						* anchor = newttt;
						anchor = &(newttt -> next);
					}
					if(firstuncovered < newalphabet[currentlevel]->size -1)
					{
						// need to insert a transition to L2 \ L1
						ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL,currentstate->oldstate2, newlevel + currentlevel + 1);
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						t -> lower = firstuncovered;
						t -> upper = newalphabet[currentlevel] ->size- 1;
						firstuncovered = newalphabet[currentlevel]->size;
						t -> succ = sss;
						t -> next = NULL;
						(*  anchor) = t;
						anchor = &(t -> next);
					}
					while(currentstate -> oldstate1 -> transition)
					{
						ValueAutomatonTransition * xxx = currentstate -> oldstate1->transition;
						currentstate -> oldstate1->transition = xxx -> next;
						delete xxx;
					}
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] !=  ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				int firstuncovered = 0;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				if(!currentstate -> oldstate1)	
				{
					// we are accepting sequence in L2 \ L1 -> make regular move in this
					for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
					{
						ValueAutomatonState * newstate = makeValueAutomatonState(pht,NULL,ttt -> succ, newlevel + currentlevel + 1);
						ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
						newttt -> lower = ttt -> lower;
						newttt -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						newttt -> succ = newstate;
						newttt -> next = NULL;
						* anchor = newttt;
						anchor = &(newttt -> next);
					}
				}
				else if(!currentstate -> oldstate2)
				{
					// we are accepting sequence in L1 \ L2 -> make universal move keeping oldstate1
					ValueAutomatonState * sss = makeValueAutomatonState(pht,currentstate->oldstate1,NULL, newlevel + currentlevel + 1);
					ValueAutomatonTransition * ttt = new ValueAutomatonTransition();
					* anchor = ttt;
					ttt -> lower = 0;
					ttt -> upper = newalphabet[currentlevel]->size - 1;
					firstuncovered = newalphabet[currentlevel]->size;
					ttt -> next = NULL;
					ttt -> succ = sss;
					anchor = &(ttt -> next);
				}
				else
				{
					// we are accepting sequence in L1 cap L2 -> make move to stay in L1 cap L2
					// and, if necessary, make move to L1 \ L2 for transitions not in A2.
					ValueAutomatonTransition ** anchor = &(currentstate -> transition);
					int firstuncovered = 0;
					for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
					{
						if(firstuncovered < ttt -> lower)
						{
							// need to insert a transition to L1 \ L2
							ValueAutomatonState * sss = makeValueAutomatonState(pht,currentstate->oldstate1,NULL, newlevel + currentlevel + 1);
							ValueAutomatonTransition * t = new ValueAutomatonTransition();
							t -> lower = firstuncovered;
							t -> upper = ttt -> lower - 1;
							t -> succ = sss;
							t -> next = NULL;
							(*  anchor) = t;
							anchor = &(t -> next);
							firstuncovered = ttt -> lower;
						}
						ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1, ttt -> succ, newlevel + currentlevel + 1);
						ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
						newttt -> lower = ttt -> lower;
						newttt -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						newttt -> succ = newstate;
						newttt -> next = NULL;
						* anchor = newttt;
						anchor = &(newttt -> next);
					}
					if(firstuncovered < newalphabet[currentlevel]->size -1)
					{
						// need to insert a transition to L1 \ L2
						ValueAutomatonState * sss = makeValueAutomatonState(pht,currentstate->oldstate1,NULL, newlevel + currentlevel + 1);
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						t -> lower = firstuncovered;
						t -> upper = newalphabet[currentlevel] -> size - 1;
						firstuncovered = newalphabet[currentlevel] -> size;
						t -> succ = sss;
						t -> next = NULL;
						(*  anchor) = t;
						anchor = &(t -> next);
					}
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step: need to proceed for L1 cap L2 and, where not both automata participating, launch
			// transitions for L1 \L2 and L2 \ L1.
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				int firstuncovered = 0;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				if(!currentstate -> oldstate1)
				{
					for(ValueAutomatonTransition * ttt = currentstate->oldstate2->transition; ttt; ttt = ttt -> next)
					{
						// we are accepting sequence in L2 \ L1 -> make moves according to oldstate2
						ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL, ttt -> succ, newlevel + currentlevel + 1);
						sss -> oldstate1 = NULL;
						sss -> oldstate2 = ttt -> succ;
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						* anchor = t;
						t -> lower = ttt -> lower;
						t -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						t -> next = NULL;
						t -> succ = sss;
						anchor = &(t -> next);
					}
				}
				else if (!currentstate -> oldstate2)
				{
					for(ValueAutomatonTransition * ttt = currentstate->oldstate1->transition; ttt; ttt = ttt -> next)
					{
						// we are accepting sequence in L1 \ L2 -> make moves according to oldstate1
						ValueAutomatonState * sss = makeValueAutomatonState(pht,ttt -> succ, NULL, newlevel + currentlevel + 1);
						sss -> oldstate2 = NULL;
						sss -> oldstate1 = ttt -> succ;
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						* anchor = t;
						t -> lower = ttt -> lower;
						t -> upper = ttt -> upper;
						firstuncovered = ttt -> upper + 1;
						t -> next = NULL;
						t -> succ = sss;
						anchor = &(t -> next);
					}
				}
				else
				{
					// prefix is in L1 cap L2, but continuation can go into either direction
					ValueAutomatonTransition * ttt1 = currentstate -> oldstate1 -> transition;
					ValueAutomatonTransition * ttt2 = currentstate -> oldstate2 -> transition;
					int firstuncovered = 0;
					while(ttt1 && ttt2)
					{
						if(firstuncovered < ttt1 -> lower && firstuncovered < ttt2 -> lower)
						{
							// continuation in complement of L1 cup L2 -> do not generate transition
							firstuncovered = ((ttt1 -> lower < ttt2 -> lower) ? ttt1 -> lower : ttt2 -> lower);
							// do not need "continue" here as we are running into one of the other
							// cases anyway
						}
						if(firstuncovered < ttt1 -> lower)
						{
							// -> firstuncoverd == ttt2 -> lower 
							// -> continue in L2 - L1
							ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL, ttt2 -> succ, newlevel + currentlevel + 1);
							ValueAutomatonTransition * t = new ValueAutomatonTransition();
							* anchor = t;
							t -> next = NULL;
							anchor = & (t -> next);
							t -> lower = firstuncovered;
							t -> succ = sss;
							if(ttt2 -> upper < ttt1 -> lower)
							{
								// no interval overlap 
								t -> upper = ttt2 -> upper;
								firstuncovered = ttt2 -> upper + 1;
								ttt2 = ttt2 -> next;
							}
							else
							{
								// intervals overlap
								t -> upper = ttt1 -> lower - 1;
								firstuncovered = ttt1 -> lower;
							}
							continue;
						}
						if(firstuncovered < ttt2 -> lower)
						{
							// -> firstuncoverd == ttt1 -> lower 
							// -> continue in L1 - L2
							ValueAutomatonState * sss = makeValueAutomatonState(pht,ttt1->succ,NULL, newlevel + currentlevel + 1);
							ValueAutomatonTransition * t = new ValueAutomatonTransition();
							* anchor = t;
							t -> next = NULL;
							anchor = & (t -> next);
							t -> lower = firstuncovered;
							t -> succ = sss;
							if(ttt1 -> upper < ttt2 -> lower)
							{
								// no interval overlap 
								t -> upper = ttt1 -> upper;
								firstuncovered = ttt1 -> upper + 1;
								ttt1 = ttt1 -> next;
							}
							else
							{
								// intervals overlap
								t -> upper = ttt2 -> lower - 1;
								firstuncovered = ttt2 -> lower;
							}
							continue;
						}
						// remaining case firstuncovered >= ttt1 -> lower && firstuncovered >= ttt2 -> lower
						// -> continue in L1 cap L2
						ValueAutomatonState * sss = makeValueAutomatonState(pht,ttt1->succ,ttt2->succ, newlevel + currentlevel + 1);
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						t -> next = NULL;
						* anchor = t;
						anchor = &(t -> next);
						t -> lower = firstuncovered;
						t -> succ = sss;
						if(ttt1 -> upper < ttt2 -> upper)
						{
							t -> upper = ttt1 -> upper;
							firstuncovered = ttt1 -> upper + 1;
							ttt1 = ttt1 -> next;
						}
						else
						{
							t -> upper = ttt2 -> upper;
							firstuncovered = ttt2 -> upper +1 ;
							ttt2 = ttt2 -> next;
							if(firstuncovered > ttt1 -> upper) ttt1 = ttt1 -> next;
						}
					}
					while(ttt1)
					{
						if(firstuncovered < ttt1->lower)
						{
							// continuation in complement of L1 cup L2 -> do not generate transition
							firstuncovered = ttt1 -> lower;
							// do not need "continue" here as we are running into one of the other
							// cases anyway
						}
						ValueAutomatonState * sss = makeValueAutomatonState(pht,ttt1 -> succ, NULL, newlevel + currentlevel + 1);
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						* anchor = t;
						t -> next = NULL;
						anchor = & (t -> next);
						t -> lower = firstuncovered;
						t -> upper = ttt1 -> upper;
						t -> succ = sss;
						firstuncovered = ttt1 -> upper + 1;
						ttt1 = ttt1 -> next;
					}
					while(ttt2)
					{
						if(firstuncovered < ttt2->lower)
						{
							// continuation in complement of L1 cup L2 -> do not generate transition
							firstuncovered = ttt2 -> lower;
							// do not need "continue" here as we are running into one of the other
							// cases anyway
						}
						ValueAutomatonState * sss = makeValueAutomatonState(pht,NULL, ttt2->succ, newlevel + currentlevel + 1);
						ValueAutomatonTransition * t = new ValueAutomatonTransition();
						* anchor = t;
						t -> next = NULL;
						anchor = & (t -> next);
						t -> lower = firstuncovered;
						t -> upper = ttt2 -> upper;
						t -> succ = sss;
						firstuncovered = ttt2 -> upper + 1;
						ttt2 = ttt2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if((currentstate->oldstate1 && currentstate->oldstate2 && (currentstate -> oldstate1->final || currentstate -> oldstate2->final))
			||
		    (currentstate -> oldstate1 && !(currentstate->oldstate2) && currentstate -> oldstate1 -> final)
			||
		    ((!currentstate -> oldstate1) && currentstate -> oldstate2 && currentstate -> oldstate2 -> final))
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			for(ValueAutomatonTransition * t = s -> transition; t; )
			{
				ValueAutomatonTransition * ttt = t;
				t = t -> next;
				delete ttt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::eqProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel+1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel +1 );
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value == currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::neqProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel+currentlevel +1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel+ 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value != currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::leProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value <= currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::ltProduct(hlvariable ** ordering, int cardordering,ValueAutomaton * other)
{


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || ordering[orderingindex] != vars[currentlevel1]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel+1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value < currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::geProduct(hlvariable ** ordering, int cardordering,ValueAutomaton * other)
{


	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value >= currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::gtProduct(hlvariable ** ordering, int cardordering, ValueAutomaton * other)
{
		

	// determine new depth
	int j = 0; 
	int k = 0;
	int newdepth = 0;
	for(int i = 0; i < cardordering;i++)
	{
		if(j < depth && ordering[i] == vars[j])
		{
			if(k < other -> depth && ordering[i] == other->vars[k])
			{
				newdepth++;
				j++;
				k++;
			}
			else
			{
				newdepth++;
				j++;
			}
			
		} 
		else
		{
			if(k < other -> depth && ordering[i] == other -> vars[k])
			{
				newdepth++;
				k++;
			}
		}
	}
		
	sort ** newalphabet = new sort * [newdepth];
	hlvariable ** newvars = new hlvariable * [newdepth];
	ValueAutomatonState ** newlevel = new ValueAutomatonState * [newdepth+1];
	for(int i = 0; i < newdepth+1;i++)
	{
		newlevel[i] = NULL;
	}
	producthashtable * pht = new producthashtable();
	ValueAutomatonState * newInitial = makeValueAutomatonState(pht,initialState, other -> initialState, newlevel);

	int currentlevel1 = 0;
	int currentlevel2 = 0;
	int orderingindex = 0;

	for(int currentlevel = 0; currentlevel < newdepth; currentlevel++)
	{
		delete pht;
		pht = new producthashtable();
		while( (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]) && (currentlevel2 == other -> depth || other->vars[currentlevel2] != ordering[orderingindex])) orderingindex++;
		if(currentlevel1 != depth && (currentlevel2 == other -> depth || ordering[orderingindex] != other -> vars[currentlevel2]))
		{
			// solo step of this
			newvars[currentlevel] = vars[currentlevel1];
			newalphabet[currentlevel] = alphabet[currentlevel1];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{

				// make transition in this, keep state in other
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate1 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht, ttt->succ, currentstate->oldstate2, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel1++;
		}
		else if(currentlevel2 != other -> depth && (currentlevel1 == depth || vars[currentlevel1] != ordering[orderingindex]))
		{
			// solo step of other
			newvars[currentlevel] = other -> vars[currentlevel2];
			newalphabet[currentlevel] = other -> alphabet[currentlevel2];
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				// make transition in other, keep state in this
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				for(ValueAutomatonTransition * ttt = currentstate -> oldstate2 -> transition; ttt; ttt = ttt -> next)
				{
					ValueAutomatonState * newstate = makeValueAutomatonState(pht,currentstate->oldstate1,ttt->succ, newlevel + currentlevel + 1);
					ValueAutomatonTransition * newttt = new ValueAutomatonTransition();
					newttt -> lower = ttt -> lower;
					newttt -> upper = ttt -> upper;
					newttt -> succ = newstate;
					newttt -> next = NULL;
					* anchor = newttt;
					anchor = &(newttt -> next);
				}
			}
			currentlevel2++;
		}
		else
		{
			// tandem step
			for(ValueAutomatonState * currentstate = newlevel[currentlevel]; currentstate; currentstate = currentstate -> next_sibling)
			{
				currentstate -> transition = NULL;
				ValueAutomatonTransition ** anchor = &(currentstate -> transition);
				ValueAutomatonTransition * transition1 = currentstate -> oldstate1 -> transition;
				ValueAutomatonTransition * transition2 = currentstate -> oldstate2 -> transition;
				while(transition1 && transition2)	
				{
					int maxlower = transition1 -> lower < transition2 -> lower ? transition2 -> lower : transition1 -> lower;
					int minupper = transition1 -> upper < transition2 -> upper ? transition1 -> upper : transition2 -> upper;
					if(maxlower <= minupper)
					{
						ValueAutomatonTransition * newtrans = new ValueAutomatonTransition();
						newtrans -> next = NULL;
						*anchor = newtrans;
						anchor = &(newtrans->next);
						newtrans -> lower = maxlower;
						newtrans -> upper = minupper;
						int x = transition1 -> succ -> id;
						int y = transition2 -> succ -> id;
						newtrans -> succ = makeValueAutomatonState(pht,transition1->succ,transition2->succ, newlevel + currentlevel + 1);
					}
					if(transition1->upper == minupper)
					{
						transition1 = transition1 -> next;
					}
					if(transition2->upper == minupper)
					{
						transition2 = transition2 -> next;
					}
				}
			}
			currentlevel1++;
			currentlevel2++;
		}
	}
	// last level
	for(ValueAutomatonState * currentstate = newlevel[newdepth]; currentstate; currentstate = currentstate -> next_sibling)
	{
		if(currentstate -> oldstate1->final && currentstate -> oldstate2->final && currentstate->oldstate1->value > currentstate->oldstate2 -> value)
		{
			currentstate->final = true;
		}
		else
		{
			currentstate->final = false;
		}
		currentstate -> transition = NULL;
	}
	// free memory
	for(int i = 0; i <= depth; i++)
	{
		for(ValueAutomatonState * s = level[i]; s; )
		{
			while(s -> transition)
			{
				ValueAutomatonTransition * tt = s -> transition;
				s -> transition = s -> transition -> next;
				delete tt;
			}
			ValueAutomatonState * tmp = s;
			s = s -> next_sibling;
			delete tmp;
		}
	}
	depth = newdepth;
	delete [] level;
	level = newlevel;
	delete [] alphabet;
	alphabet = newalphabet;
	delete [] vars;
	vars = newvars;
	initialState = newInitial;
	delete pht;
	minimise();
	delete other;
}

void ValueAutomaton::minimise()
{

	minimisehashtable * mht = new minimisehashtable();
	// 1. remove unreachable states and keep just one final state
	ValueAutomatonState * newfinal = new ValueAutomatonState;
	newfinal -> final = true;
	newfinal -> value = 0;
	newfinal -> next_sibling = NULL;
	newfinal -> transition = NULL;
	newfinal -> oldstate1 = NULL;
	newfinal -> oldstate2 = NULL;

	// handle the level that leads to final states
	for(ValueAutomatonState * pred = level[depth-1]; pred; pred = pred -> next_sibling)
	{
		ValueAutomatonTransition ** transition = &(pred -> transition);
		// redirect sussessful transitions to newfinal
		// and remove unsuccessful transitions
		while(*transition != NULL)
		{
			if((*transition)->succ->final)	
			{
				(*transition)->succ = newfinal;
				transition = &((*transition)->next);
			}
			else
			{
				ValueAutomatonTransition * tmp = *transition;
				*transition = (*transition)->next;
				delete tmp;
			}
		}
		// glue transitions where intervals are in contact
		ValueAutomatonTransition * t = pred -> transition;
		while(true)
		{
			if(!t) break;
			if(!(t->next)) break;
			if(! (t -> succ == t -> next -> succ)) 
			{
				t = t -> next;
				continue;
			}
			if(! ( t -> upper + 1 == t -> next -> lower))
			{
				t = t -> next;
				continue;
			}
			t -> upper = t -> next -> upper;
			ValueAutomatonTransition * tmp = t -> next;
			t -> next = t -> next -> next;
			delete tmp;
		}
	}
	// remove old final states
	ValueAutomatonState * s = level[depth];
	while(s)
	{
		ValueAutomatonState * tmp = s;
		s = s -> next_sibling;
		delete tmp;
	}
	level[depth] = newfinal;
	for(int i = depth-2; i >= 0; i--)
	{
		for(ValueAutomatonState * currentstate = level[i]; currentstate; currentstate = currentstate -> next_sibling)
		{
			ValueAutomatonTransition ** transition = &(currentstate -> transition);
			while((*transition) != NULL)
			{
				if( (*transition) -> succ -> transition)
				{
					transition = &((*transition)->next);
				}
				else
				{
					ValueAutomatonTransition * tmp =  *transition;
					* transition = (*transition) -> next;
					delete tmp;
				}
			}
		}
		for(ValueAutomatonState ** s = &(level[depth-1]); *s; )
		{
			if((*s) -> transition)
			{
				s = &((*s) -> next_sibling);
			}
			else
			{
				ValueAutomatonState * temp = (*s);
				*s = (*s) -> next_sibling;
				delete temp;
			}
		}
	}
	// merge identical states
	ValueAutomatonState * tobedeleted = NULL;
	for(int i = depth-1; i >0; i--)
	{
		// pass 1: detect identical states at level i
		for(ValueAutomatonState ** s = level + i; *s; )
		{
			ValueAutomatonState * original = mht->lookup(*s);
			
			if(original)
			{
				(*s)->oldstate1 = original;
				// unchain *s and mark to be deleted
				ValueAutomatonState * tmp = *s;
				*s = (*s) -> next_sibling;
				tmp -> next_sibling = tobedeleted;
				tobedeleted = tmp;
			}
			else
			{
				(*s)->oldstate1 = *s;
				mht->insert(*s);
				s = & ((*s) -> next_sibling);
			}
		}
		// pass 2: redirect arcs in predecessor level
		for(ValueAutomatonState * s = level[i-1];s ; s = s -> next_sibling)
		{
			for(ValueAutomatonTransition * t = s -> transition; t ; t = t -> next)
			{
				t -> succ = t -> succ->oldstate1;
			}
			ValueAutomatonTransition * t = s -> transition;
			while(true)
			{
				if(!t) break;
				if(!(t->next)) break;
				if(! (t -> succ == t -> next -> succ)) 
				{
					t = t -> next;
					continue;
				}
				if(! ( t -> upper + 1 == t -> next -> lower))
				{
					t = t -> next;
					continue;
				}
				t -> upper = t -> next -> upper;
				ValueAutomatonTransition * tmp = t -> next;
				t -> next = t -> next -> next;
				delete tmp;
			}
		}
	}
	while(tobedeleted)
	{
		ValueAutomatonState * tmp = tobedeleted;
		tobedeleted = tobedeleted -> next_sibling;
		while(tmp -> transition)
		{
			ValueAutomatonTransition * temp = tmp -> transition;
			tmp -> transition = tmp -> transition -> next;
			delete temp;
		}
		delete tmp;
	}
}

int ValueAutomaton::count(int cardvars, hlvariable ** ordering, bool * occurring)
{
	int uncontained = 1;
	int j = 0;
	for(int i = 0; i < cardvars; i++)
	{
		if(!occurring[i] ) continue;
		if( j < depth && vars[j] == ordering[i]) 
		{
			j++;
			continue;
		}
		uncontained *= ordering[i]->so->size;
	}
	for(ValueAutomatonState * s = level[depth]; s; s = s -> next_sibling)
	{
		s->card = 1;
	}
	for(int i = depth - 1; i >= 0; i--)
	{
		for(ValueAutomatonState * s = level[i]; s; s = s -> next_sibling)
		{       
			s->card = 0;
			for(ValueAutomatonTransition * t = s -> transition; t; t = t -> next)
			{
				s -> card += t -> succ -> card * (t -> upper - t -> lower + 1);
			}
		}       
	}
	return initialState -> card * uncontained;;
}

void ValueAutomaton::print()
{
	std::cout << std::endl << "AUTOMATON" << std::endl;
	for(int i = 0; i < depth; i++)
	{
		std::cout << "LEVEL " << i << " " << vars[i]->name << ":" << alphabet[i] -> name << " (" << alphabet[i]->size << ")" << std::endl;
		for(ValueAutomatonState * s = level[i]; s; s = s -> next_sibling)
		{
			std::cout << "    STATE " << s -> id << std::endl;
			for(ValueAutomatonTransition * t = s -> transition; t; t = t -> next)
			{
				std::cout << "        " << t -> lower << " .. " << t -> upper << " -> " << t -> succ -> id << std::endl;
			}
		}
	}
		std::cout << "LEVEL " << depth << ":" << std::endl;
	for(ValueAutomatonState * s = level[depth]; s; s = s -> next_sibling)
	{
		std::cout << "    STATE " << s -> id << std::endl;
		if(s -> final) std::cout << "        FINAL" << std::endl;
		std::cout << "        VALUE: " << s -> value << std::endl;
	}
}

ValueAutomaton::~ValueAutomaton()
{
	for(int i = 0; i <=depth;i++)
	{
		if(level)
		for(ValueAutomatonState * s = level[i]; s ;)
		{

			for(ValueAutomatonTransition * t = s -> transition; t ; )
			{
				ValueAutomatonTransition * tt = t;
				t = t -> next;
				delete tt;
			}
			ValueAutomatonState * ss = s;
			s = s -> next_sibling;
			delete ss;
		}
	}
	if(level) delete [] level;
	if(vars) delete [] vars;
	if(alphabet) delete [] alphabet;
}

ValueAutomatonTransition::ValueAutomatonTransition()
{
	succ = NULL;
	next = NULL;
}


void printass(int cardvars, hlvariable ** ordering, bool * occurring, int * ass)
{
	for(int i = 0; i < cardvars; i++)
	{
		if(!occurring[i])
		{
			std::cout << " * ";
			continue;
		}
		std::cout << ordering[i]->name << ": " <<ass[i] << "("<<ordering[i]->so->size << "), ";
	}
	std::cout << std::endl;
}

int * ValueAutomaton::first_assignment(int cardvars, hlvariable ** ordering, bool * occurring)
{
	mycard = cardvars;
	myordering = ordering;
	myoccurring = occurring;
	currentassignment = new int[mycard];
	memset(currentassignment, 0, sizeof(int) * mycard);
	ValueAutomatonState * s = initialState;
	iterationstatestack = new ValueAutomatonState * [mycard+1];
	iterationtransitionstack = new ValueAutomatonTransition * [mycard+1];
	iterationprevstack = new int [mycard+1]; // position of previous 
						// var mentioned in automaton
	iterationnextstack = new int [mycard+1]; // pos of next
						// var mentioned in automaton
	int j = 0;
	iterationprevstack[0] = -1;
	for(int i = 0; i < mycard; i++)
	{
		if(j >= depth) 
		{
			iterationstatestack[i] = NULL;
			iterationtransitionstack[i] = NULL;
			if(i > 0) iterationprevstack[i] = iterationprevstack[i-1];
			continue;
		}
		if(!occurring[i]) 
		{
			iterationstatestack[i] = NULL;
			iterationtransitionstack[i] = NULL;
			if(i > 0) iterationprevstack[i] = iterationprevstack[i-1];
			continue;
		}
		if(vars[j] != myordering[i]) 
		{
			iterationstatestack[i] = NULL;
			iterationtransitionstack[i] = NULL;
			if(i > 0) iterationprevstack[i] = iterationprevstack[i-1];
			continue;
		}
		if(!(s -> transition)) 
		{
			delete [] iterationtransitionstack;
			delete [] iterationstatestack;
			delete [] iterationprevstack;
			delete [] iterationnextstack;
			delete [] currentassignment;
			return NULL;
		}
		iterationstatestack[i] = s;
		iterationtransitionstack[i] = s -> transition;
		iterationprevstack[i] = i;
		currentassignment[i] = s -> transition -> lower;
		s = s -> transition -> succ;
		j++;
	}
	int d = mycard;	
	for(int i = mycard-1; i >=0 ; i--)
	{
		if(iterationstatestack[i]) 
		{
			iterationnextstack[i] = d;	
			d = i;
		}
		else
		{
			iterationnextstack[i] = d;	
		}
	}
	return currentassignment;
}

int * ValueAutomaton::next_assignment()
{
	int j = mycard-1; // index in assignment

	while(true)
	{
		// search, starting from back, first position
		// that is _not_ largest possible value
		if(j < 0)  // no remaining assignment
		{
			delete [] iterationstatestack;
			delete [] iterationtransitionstack;
			delete [] iterationprevstack;
			delete [] iterationnextstack;
			delete [] currentassignment;
			return NULL;
		}
		if(!myoccurring[j]) // skip nonoccurring
		{
			j--;
			continue;
		}
		if(!iterationstatestack[j])
		{
			// j is unmentioned variable
			// --> all values of its sort need to be traversed
			if(++(currentassignment[j]) < myordering[j]->so->size)
			{
				// was not last value -> increment
				// and reset all subsequent automata values to
				// smallest possible
				ValueAutomatonState *s;
				int i = iterationprevstack[j];
				if(i < 0) 
				{
					s = initialState;
				}
				else
				{
					s = iterationtransitionstack[i] -> succ;
				}

				i = iterationnextstack[j];
				while(i < mycard)
				{	
					iterationstatestack[i] = s;
					iterationtransitionstack[i] = s -> transition;
					currentassignment[i] = s -> transition -> lower;
					s = s -> transition -> succ;
					i = iterationnextstack[i];
				}
				return currentassignment;
			}
			currentassignment[j--] = 0;
			continue;
		}

		// mentioned variable
		if(++(currentassignment[j]) <= iterationtransitionstack[j]->upper)
		{
			ValueAutomatonState *s = iterationtransitionstack[j] -> succ;
			int i = iterationnextstack[j];
			while(i < mycard)
			{	
				iterationstatestack[i] = s;
				iterationtransitionstack[i] = s -> transition;
				currentassignment[i] = s -> transition -> lower;
				s = s -> transition -> succ;
				i = iterationnextstack[i];
			}
			return currentassignment;
		}
		if((iterationtransitionstack[j] = iterationtransitionstack[j]->next)) // only one = is deliberate!
		{
			currentassignment[j] = iterationtransitionstack[j]->lower;
			ValueAutomatonState *s = iterationtransitionstack[j] -> succ;
			int i = iterationnextstack[j];
			while(i < mycard)
			{	
				iterationstatestack[i] = s;
				iterationtransitionstack[i] = s -> transition;
				currentassignment[i] = s -> transition -> lower;
				s = s -> transition -> succ;
				i = iterationnextstack[i];
			}
			return currentassignment;
		}
		j--;
	}
	
}

void ValueAutomaton::project(int newdepth)
{
	for(ValueAutomatonState * sss = level[newdepth]; sss; sss = sss -> next_sibling)
	{
		sss -> final = true;
		while(sss -> transition)
		{
			ValueAutomatonTransition * tmp = sss -> transition;
			sss -> transition = sss -> transition -> next;
			delete tmp;
		}
		sss -> transition = NULL;
	}
	for(int i = newdepth+1; i <= depth; i++)
	{
		while(level[i])
		{
			ValueAutomatonState * sss = level[i];
			level[i] = sss -> next_sibling;
			while(sss -> transition)
			{
				ValueAutomatonTransition * tmp = sss -> transition;
				sss -> transition = sss -> transition -> next;
				delete tmp;
			}
			delete sss;
		}
	}
	depth = newdepth;

}
