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

#include <Exploration/Firelist.h>
#include <Exploration/LTLExploration.h>
#include <Exploration/LTLStack.h>
#include <Formula/LTL/BuechiAutomata.h>
#include <Net/NetState.h>
#include <Exploration/FirelistStubbornCloseGiven.h>
#include <Witness/Path.h>


LTLPayload::LTLPayload()
{}
LTLPayload::~LTLPayload()
{}


LTLExploration::LTLExploration(Petrinet * n) 
{net = n;}


bool LTLExploration::checkProperty(BuechiAutomata &automaton,
                                   Store<LTLPayload> &store, Firelist &firelist, NetState &ns)
{

FirelistStubbornCloseGiven * fl_ignorance = new FirelistStubbornCloseGiven(net);
//net -> print();
//RT::rep->status("at exploration %s",net -> name);
//RT::args.writeBuechi_arg = net -> name;
//automaton.writeBuechi();

//for(int i = 0; i < net -> Card[PL];i++)
//{
//std::cout << net -> Name[PL][i] << ": " << ns.Current[i] << ", ";
//}
//std::cout << std::endl;

//RT::rep->status("significant places: %d", net -> CardSignificant);
//for(int i = 0; i < net -> Card[PL];i++)
//{
//std::cout << " " << net -> Name[PL][i];
//}
//std::cout << std::endl;


    // prepare list of all visible transitions

    LTLPayload * stateinfo;

     path = new Path(net);
    /// State of Buchi automaton
    ns.buchistate = 0;

    // In general, buchistate is part of currentmarking, at index
    // CardSignificant -1. For the initial marking, 0 is
    // already set by function insertbuchistate() in Planning/LTLTask.cc

    // Initial state gets dfs number 1. Value 0 is reserved for states
    // that have left the tarjan stack (are member of completed SCC).
    store.searchAndInsert(ns, &stateinfo, 0);
    stateinfo->dfsnum = 1;
    stateinfo->lowlink = 1;
    stateinfo->witness = net->Card[TR]; // this state has its lowlink from itself
    stateinfo->witnessstate = NULL;
    /// current global dfs number
    int nextDFSNumber = 2;

    // Initialize stacks. We work with 3 stacks:
    // * DFSStack: for organizing depth first search, carries firelist
    //   Information.
    // * TarjanStack: for states that are fully processed but their SCC
    //   has not yet been completed. Carries pointer to payload. 
    //   State is pushed when first visited and popped when its SCC is
    //   complete.
    //   When state is pushed, payload of state is set to [0,0].
    // * AcceptingStack: for managing the accepting state with largest
    //   dfs on DFSStack. Carries dfsnums. State is pushed when 
    //   visited first, and popped when it leaves the DFSStack.

    SearchStack<LTLSearchStackEntry> * DFSStack = new SearchStack<LTLSearchStackEntry>;
    SearchStack<LTLPayload *> TarjanStack;
    SearchStack<arrayindex_t> AcceptingStack;

    // initialize AcceptingStack with a dummy entry containing the
    // unused dfs number 0. This way, we can refer to top of stack
    // even if no accepting states have been seen so far...

    arrayindex_t * acc = AcceptingStack.push(); 
    * acc = 0; // set stack content

    if(automaton.isAcceptingState(ns.buchistate))
    {
	// the first "real" entry on AcceptingStack
	acc = AcceptingStack.push();
        * acc = 1;
    }

    // Initialize TarjanStack with stateinfo
    LTLPayload ** ppp = TarjanStack.push();
    * ppp = stateinfo;

    // get first firelist
    // note that we process firelist and automaton transition list from
    // card to 0. This way, we do not need to store the size of the lists.

    arrayindex_t *currentFirelist;
    if(RT::args.stubborn_arg != stubborn_arg_off) firelist.visible = automaton.visible[ns.buchistate];
    arrayindex_t currentEntry = firelist.getFirelist(ns, &currentFirelist);
    stateinfo->ignorance = (currentEntry == ns.CardEnabled) ? 1 : 0; // firelist contains all enabled transitions
//RT::rep->status("ENABLED: %d", ns.CardEnabled);
//RT::rep -> status("INFO (INIT): DFS %d, LL %d, IG %d", stateinfo -> dfsnum, stateinfo -> lowlink, stateinfo -> ignorance);
    if(currentEntry == 0)
    {
	// Initial state is deadlock state. 
	if(RT::args.disregardfinitetraces_given)
	{
		// there is no infinite trace, i.e. no counterexample
		return false;
	}
	else
	{
		// We need to continue since
		// LTL semantics turns deadlock into tau transition.
		currentFirelist = new arrayindex_t[1]; 
		currentFirelist[0] = net->Card[TR]; // our code for tau transition
		currentEntry = 1;
	}
    }

    // get first transition list in automaton
    arrayindex_t * currentBuchiList = reinterpret_cast<arrayindex_t *>(malloc(SIZEOF_ARRAYINDEX_T * automaton.cardTransitions[ns.buchistate]));
    arrayindex_t currentBuchiTransition = 0;
    for(arrayindex_t i = 0; i < automaton.cardTransitions[ns.buchistate];i++)
    {
	StatePredicate * predicate = automaton.guard[ns.buchistate][i];
//RT::rep->status("EVALUATE %s", predicate -> toString());
        predicate -> evaluate(ns);
	if(predicate->value)
	{
//RT::rep->status("...TRUE");
		currentBuchiList[currentBuchiTransition++] = automaton.nextstate[ns.buchistate][i];
	}
    }
    arrayindex_t buchiLength = currentBuchiTransition;
    if(!buchiLength)
    {
	// situation: no buchi transition enabled in initial state->
	// cannot accept anything
	return false;
    }
    currentBuchiList = reinterpret_cast<arrayindex_t *>(realloc(currentBuchiList,SIZEOF_ARRAYINDEX_T * buchiLength));
    // Initialize DFSStack with ingredients of initial state
    LTLSearchStackEntry * stackentry = DFSStack->push();
    stackentry  = new (stackentry) LTLSearchStackEntry(currentFirelist,currentEntry,ns.buchistate,currentBuchiList,buchiLength,currentBuchiTransition,stateinfo);

     while(true) // DFS search loop
     {
//RT::rep->status("new round  %s",net -> name);
//for(int i = 0; i < currentEntry; i++)
//{
//RT::rep-> status("net %s firelist %d is %s", net -> name, i, currentFirelist[i] == net->Card[TR] ? "tau" :currentFirelist[i] == net->Card[TR] ? "tau" : net -> Name[TR][currentFirelist[i]]);
//}
//for(int i = 0; i < net -> Card[PL];i++)
//{
//if(ns.Current[i])
//RT::rep->status("net %s marking %s is %d", net -> name, net -> Name[PL][i], ns.Current[i]);
//}
//RT::rep->status("net %s Buchi state %d trans %d", net -> name, ns.buchistate, currentBuchiTransition);
//RT::rep->status("new round at state %d", stateinfo->dfsnum);
	// situation: we have a state, given by ns and buchistate.
        // We have a firelist with an entry and a buchi transition.
	// task: continue processing this state.
//RT::rep -> status("INFO (NEW ROUND): DFS %d, LL %d, IG %d CURR ENTRY %d", stateinfo -> dfsnum, stateinfo -> lowlink, stateinfo -> ignorance, currentEntry);
//LTLPayload * lll = DFSStack->init_traversal().info;
//while(true)
//{
//RT::rep -> status("STACK %d, LL %d, IG %d", lll -> dfsnum, lll -> lowlink, lll -> ignorance);
	//if(lll -> dfsnum == 1) break;
	//lll = DFSStack->next_traversal().info;
//}
       
        if(currentEntry)
	{
//RT::rep->status("has enabled net transition");
		// situation: our state has enabled net transitions (or tau
		// transition) that
		// have not been fully explored. The one to be considered
 		// is currentEntry - 1.
		// task: continue processing this net transition by
		// joining it with next enabled buchi transition

		if(currentBuchiTransition)
		{
			// situation: There is an enabled buchi transition 
			// not yet explored (currentBuchiTransition-1)
			// task: explore this buchi transition together
			// with current net transition
//RT::rep->status("has enabled buchi transition");
			

			if(currentFirelist[currentEntry-1] >= net->Card[TR])
			{
				// fire tau transition = do nothing
//RT::rep->status("fire tau");
			}
			else
			{
				net->fire(ns, currentFirelist[currentEntry-1]);
//RT::rep->status("fire %s",net->Name[TR][currentFirelist[currentEntry-1]]);
			}
			ns.buchistate = currentBuchiList[--currentBuchiTransition];
//for(int i = 0; i < net -> Card[PL];i++)
//{
//if(ns.Current[i])
//RT::rep->status("net %s new marking %s is %d", net -> name, net -> Name[PL][i], ns.Current[i]);
//}
//RT::rep->status("net %s new buchi state is %d", net -> name, ns.buchistate);

			// new product state ready for search

			LTLPayload * newstateinfo;
			if(store.searchAndInsert(ns,&newstateinfo,0))
			{
//RT::rep -> status("INFO (NOT NEW AFTER SEARCH): DFS %d, LL %d, IG %d", newstateinfo -> dfsnum, newstateinfo -> lowlink, newstateinfo -> ignorance);
				// situation: new state exists
				// task: check back-edge, update lowlink, check acceptance,
				// continue at current state
				//RT::rep->status("new state exists dfs %u low %u",newstateinfo->dfsnum,newstateinfo->lowlink);

				// check back-edge for implementing ignored transitions
				if(newstateinfo->dfsnum > 0)
				{
//RT::rep->status("back edge");
					// new state on stack -> this edge is a back-edge
					bool have_fired_all = false;
					LTLPayload * lll = DFSStack->init_traversal().info;
					while(true)
					{
//RT::rep -> status("INFO (LLL): DFS %d, LL %d, IG %d", lll -> dfsnum, lll -> lowlink, lll -> ignorance);
						if(lll->ignorance > 0)
						{
							have_fired_all = true;
							break;
						}
						if(lll == newstateinfo) break;
						lll = DFSStack->next_traversal().info;
					}
					if(!have_fired_all)
					{
						// mark new state as "need to fire all"
						newstateinfo -> ignorance = 2;
//RT::rep->status("need to fire all at %d", newstateinfo -> dfsnum);
					}
				}

				// update lowlink
				if(newstateinfo->dfsnum != 0)
				{
					// situation: new state still on
					// tarjan stack
					// task: update lowlink and
					// check acceptance
//RT::rep->status("new state on tarjan");

					if(stateinfo->lowlink > newstateinfo->lowlink)
					{
						// situation: lowlink improves
						// task: update, check acceptance
//RT::rep->status("lowlink improves");
						stateinfo->lowlink = newstateinfo->lowlink;
						stateinfo->witness = currentFirelist[currentEntry-1]; // this transition leads to the state defining its lowlink
						stateinfo->witnessstate = newstateinfo;

						// acceptance check
						if(stateinfo->lowlink <= AcceptingStack.top())
						{
							// situation: ACCEPT
							// task: tidy up and
							// return true
//RT::rep->status("accept");
	
//if(!ns.CardEnabled) RT::rep->status("DEADLOCK");
							createWitness(newstateinfo, DFSStack);
							return true;
						}
						// situation: not accepted
						// task: continue at current
//RT::rep->status("do not accept");

						if(currentFirelist[currentEntry-1] < net->Card[TR])
						{
							net->backfire(ns, currentFirelist[currentEntry-1]);
						}
						continue;
					}
					else
					{
						// situation: lowlink not improved
						// task: check accepting self-loop
						if((stateinfo->dfsnum == newstateinfo->dfsnum) && (stateinfo->dfsnum == AcceptingStack.top()))
						{
							// situation: have accepting self-loop
							// task: accpet
							createWitness(newstateinfo, DFSStack);
							return true;
						}
						// situation: lowlink not improved, no accepting self-loop
						// task: no accept, continue
//RT::rep->status("lowlink not improved");
						if(currentFirelist[currentEntry-1] < net->Card[TR])
						{
							net->backfire(ns, currentFirelist[currentEntry-1]);
						}
						continue;
					}
					// end of complete if-else
					assert(false);
				}
				else
				{
					// situation: new state not on 
					// tarjan stack
					// task: no lowlink update, no
					// acceptance check necessary
					// continue at current state
//RT::rep->status("new state not on trajan");

					if(currentFirelist[currentEntry-1] < net->Card[TR])
					{
						net->backfire(ns, currentFirelist[currentEntry-1]);
					}
					continue;
				}
				// end of complete if-else
				assert(false);
			
			}
			else
			{
				// situation: new state does not yet exist
				// task: fully switch to new state and
				// continue search at new state
//RT::rep->status("new state does not yet exist");

				// update stack entry of current state
				stackentry -> netIndex = currentEntry;
				stackentry -> buchiIndex = currentBuchiTransition;
				
				// initialize data for new state
				
				// dfsnum + lowlink
				stateinfo = newstateinfo;
				stateinfo->dfsnum = stateinfo->lowlink = nextDFSNumber++;
				stateinfo->witness = net->Card[TR];
				stateinfo->witnessstate = NULL;

				// state of buchi automaton
				ns.buchistate = currentBuchiList[currentBuchiTransition];
				// acceptance stack
				if(automaton.isAcceptingState(ns.buchistate))
				{
					acc = AcceptingStack.push();
					* acc = stateinfo->dfsnum;
				}

				// tarjan stack
			        ppp = TarjanStack.push();
			        * ppp = stateinfo;

				// net firelist

//RT::rep->status(" 1 new state does not yet exist %u %u",currentEntry,currentFirelist[currentEntry - 1]);
				if(currentFirelist[currentEntry - 1] < net->Card[TR])
				{
					// net transition was not tau
					net->updateEnabled(ns, currentFirelist[currentEntry-1]);
				}
//RT::rep->status(" 2 new state does not yet exist");
			    if(RT::args.stubborn_arg != stubborn_arg_off) firelist.visible = automaton.visible[ns.buchistate];
				currentEntry = firelist.getFirelist(ns, &currentFirelist);
				stateinfo -> ignorance = (currentEntry == ns.CardEnabled) ? 1 : 0;
				if(currentEntry == 0)
				{
					//State is deadlock state
					if(!RT::args.disregardfinitetraces_given)
					{
						// We need to continue since
						// LTL semantics turns deadlock into tau transition.
						currentFirelist = new arrayindex_t[1]; 
						currentFirelist[0] = net->Card[TR]; // our code for tau transition
						currentEntry = 1;
					}
				}
	
				// buchi firelist
				currentBuchiList = reinterpret_cast<arrayindex_t *>(malloc(SIZEOF_ARRAYINDEX_T * automaton.cardTransitions[ns.buchistate]));
				currentBuchiTransition = 0;
				for(arrayindex_t i = 0; i < automaton.cardTransitions[ns.buchistate];i++)
				{
					StatePredicate * predicate = automaton.guard[ns.buchistate][i];
//RT::rep->status("EVALUATING PRED %s", predicate -> toString());
					predicate -> evaluate(ns);
					if(predicate->value)
					{
//RT::rep->status(".... TRUE");
						currentBuchiList[currentBuchiTransition++] = automaton.nextstate[ns.buchistate][i];
					}
				}
				buchiLength = currentBuchiTransition;
				if(!buchiLength)
				{
//RT::rep->status("empty buchi");
					// situation: no enabled buchi
					// transition, i.e. no successor in
					// product automaton
					// task: shortcut: set net fl to
					// completed
					currentBuchiList = NULL;
					currentEntry = 0;
				}
				else
				{
					currentBuchiList = reinterpret_cast<arrayindex_t *>(realloc(currentBuchiList,SIZEOF_ARRAYINDEX_T * buchiLength));
				}
				
//RT::rep->status(" 3 new state does not yet exist");
				// search stack
				stackentry = DFSStack->push();
				stackentry  = new (stackentry) LTLSearchStackEntry(currentFirelist,currentEntry,ns.buchistate,currentBuchiList,buchiLength,currentBuchiTransition,stateinfo);
				continue;
			}
			// end of complete if-else
			assert(false);
		}
		else
		{
			// situation: all buchi transitions have been
			// explored.
			// task: proceed with next net transition, 
			
//RT::rep->status("all buchi transitions explored");
			--currentEntry;
			currentBuchiTransition = buchiLength;
			continue;
		}
		// end of complete if-else
		assert(false);
	}
	else
	{
		// situation: all enabled transitions of this state have
		// been processed.
		// task: close this state (check ignorance, check termination, check scc, backtrack)
//RT::rep->status("closing %d",stateinfo -> dfsnum);

		// check ignorance
		if(stateinfo->ignorance == 2)
		{
//RT::rep->status("extending firelist");
			stateinfo->ignorance = 1; // have handled ignored in this state
			// need to fire stubborn set around all visible transitions
			int cardvis = 0;
			arrayindex_t * st = new arrayindex_t[net -> Card[TR]];
			memset(st,0,net -> Card[TR] * sizeof(arrayindex_t));
			bool * onst = new bool[net -> Card[TR]];
			memset(onst,0,net -> Card[TR] * sizeof(bool));
			int j = 0;
			for(int i = 0; i < net->Card[TR];i++)
			{
				if(automaton.visible[ns.buchistate][i]) 
				{
//RT::rep->status("add %s", net -> Name[TR][i]);
					st[j++] = i;
					onst[i] = true;
				}
			}
			if(j)
			{
				delete [] currentFirelist;
				bool dummy = false; // "need enabled" information for stubborn set
				currentEntry = fl_ignorance -> getFirelist(ns,&currentFirelist, st,onst, j, &dummy);
				DFSStack->top().fl = currentFirelist;
				DFSStack->top().netIndex = currentEntry;
//RT::rep->status("added %d transitions", currentEntry);
				continue;
			}
			else
			{
//RT::rep->status("added 0 transitions");
				delete [] st;
				delete [] onst;
			}
		}

//RT::rep->status("do not need to add transitions");
		
		if(stateinfo->dfsnum == 1)
		{
			// situation: we are closing initial state
//RT::rep->status("closing initial");
			// situation: initial state is root of TSCC

			// task: exit
			
			return false;
			// end of complete if/else
			assert(false);
		}
		// situation: we are closing any non-initial state
		// task: check scc, backtrack
		
		// keep lowlink for update of predecessor state
		arrayindex_t lowli = stateinfo->lowlink;
		if(stateinfo->lowlink == stateinfo->dfsnum)
		{
			// situation: scc found

			// task: remove scc from tarjan stack
//RT::rep->status("closing scc %d",stateinfo->dfsnum);

			LTLPayload * pp;
			do
			{
				pp = TarjanStack.top();
//RT::rep -> status("INFO (PP): DFS %d, LL %d, IG %d", pp -> dfsnum, pp -> lowlink, pp -> ignorance);
				TarjanStack.pop();
//RT::rep->status("popping %d",pp->dfsnum);
				pp->dfsnum = pp->lowlink = 0;
			}
			while(pp!=stateinfo);
		}
		// situation: no scc found, or scc already removed, state is
		// not initial
		// task: backtrack
//RT::rep->status("backtracking");

		stateinfo -> dfsnum *= -1; // negative dfs: not on search stack but still on tarjan stack
		// backtrack search stack
		DFSStack->pop();

		// load data for previous state from search stack
		stackentry = &(DFSStack->top());
		currentFirelist = stackentry->fl;
		currentEntry = stackentry->netIndex;
		ns.buchistate = stackentry->buchiState;
		currentBuchiList = stackentry->bl;
		buchiLength = stackentry->buchiLength;
		currentBuchiTransition = stackentry->buchiIndex;
		LTLPayload * oldstateinfo = stateinfo;
		stateinfo = stackentry->info;

		// update lowlink
		if(stateinfo->lowlink > lowli && lowli > 0)
		{
			stateinfo->lowlink = lowli;
			stateinfo->witnessstate = oldstateinfo;
			stateinfo->witness = currentFirelist[currentEntry-1];
		}

		// backtrack marking
		if(currentFirelist[currentEntry-1] < net->Card[TR])
		{
//RT::rep->status("BACKFIRE %s AT POS %d", net -> Name[TR][currentFirelist[currentEntry-1]],currentEntry);
			net->backfire(ns, currentFirelist[currentEntry-1]);
			net->revertEnabled(ns, currentFirelist[currentEntry-1]);
		}

		// backtrack acceptance stack
		while(AcceptingStack.top() > stateinfo->dfsnum)
		{
			AcceptingStack.pop();
		}
		continue;
	}
	// end of complete if-else
	assert(false);
     }
     // end of while loop, only left by return true/return false
     assert(false);
}

void LTLExploration::createWitness(LTLPayload * stateinfo, SearchStack<LTLSearchStackEntry> * DFSStack)
{
	if(!RT::args.path_given) return;
	// situation: stateinfo belongs to a state s with a lowlink l less 
	// than some
	// accepting state on the search stack. Hence, the witness path
	// has shape a (b c)* where
	// - c starts at stateinfo (the top of search stack) and ends
	// at a state s* that is on the search stack and has dfsnum l
	// - a is the search stack from m0 to s*
	// - b is the search stack from s* to s
	// We need to start with computing s*
	LTLPayload * sstar;
	for(sstar = stateinfo; ;sstar = sstar -> witnessstate)
	{
		if(! sstar -> witnessstate )	break; // does not lead to 
						// smaller dfs
	}
	// now insert a b in reverse order (as we access stack from top to bottom)
	// all transitions are inserted in front of the others (add(...,true))
	// At the boundary between a and bm add "beginCycle"
	path = new Path(net);
 	path->initialized = true;
	bool ccc = false;
	while(DFSStack->StackPointer != 0)
	{
		LTLSearchStackEntry e = DFSStack->top();
		path->addTransition(e.fl[e.netIndex-1],true);
		if(e.info == sstar){ path->beginCycle(true);ccc=true;}
		DFSStack->pop();
	}
	if(!ccc) path->beginCycle();
	// now: add c at end of sequence
	for(;stateinfo!=sstar;stateinfo = stateinfo->witnessstate)
	{
		//RT::rep->status("add low %u",stateinfo->witness);
		path->addTransition(stateinfo->witness);
	}
	path->endCycle();
}

