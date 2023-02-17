/***************************************************************************
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
\author Markus
\status new

\brief compute reachability using relaxed stubborn sets
*/

#include <Core/Dimensions.h>
#include <Planning/StoreCreator.h>
#include <Exploration/TSCCExploration.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Exploration/ChooseTransition.h>
#include <Exploration/FirelistStubbornCloseGiven.h>
#include <Exploration/FirelistStubbornDeadlock.h>


bool TSCCExplorationAGEFRelaxed::depth_first(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    statenumber_t lasttrue = 0;
    StatePredicateProperty * p = reinterpret_cast<StatePredicateProperty *> (&property);
    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    if(property.initProperty(ns)) lasttrue = 1;

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    memset(inUpset,0,net->Card[TR]*sizeof(bool));
    
    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t currentDFSNumber = 1;
    statenumber_t maxDFSNumber = 1;
    statenumber_t highest_lowlink = 0;

    //set initial dfs to one
    currentDFSNumber = ++maxDFSNumber; 
    setDFS(payload, currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                if (dfsstack.StackPointer != 0)
                {
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
		currentDFSNumber = ++maxDFSNumber;
                setDFS(newPayload, currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put a entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
                if (property.checkProperty(ns, currentFirelist[currentEntry]))
                {
                    lasttrue = currentDFSNumber;
                }

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
		// this is root of tscc 
		// --> check for ignored transitions in an up-set of this marking
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
		cardUpset = p->predicate -> getUpSet(Upset,inUpset,&needEnabled);
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(myFirelist.visible[t] && !inUpset[t])
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
			}
		}
		for(arrayindex_t i = 0; i < cardUpset;i++)
		{
			arrayindex_t t = Upset[i];
			if(lastfired[t] >= getDFS(payload))
			{
				// this transition is not ignored
				inUpset[t] = false;
				Upset[i--] = Upset[--cardUpset];
			}
			else
			{
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed

		if(lasttrue < getDFS(payload)) return false;
		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
                //set the property value
                return true;
            }
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
	    currentDFSNumber = getDFS((statenumber_t *) dfsstack.top().payload);
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            
            //update the property
            property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}

void * TSCCExplorationAGEFRelaxed::Globalliveness(void *)
{
	Petrinet * net = Petrinet::InitialNet;
	TSCCExplorationAGEFRelaxed * globalexploration = new TSCCExplorationAGEFRelaxed(net);
	globalexploration -> globalliveness_check( * new SimpleProperty(net), * NetState::createNetStateFromInitial(net),*StoreCreator<statenumber_t>::createStore(net,new Mara(),1),* new FirelistStubbornDeadlock(net),1);
	return NULL;
}

void TSCCExplorationAGEFRelaxed::globalliveness_check(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    int * lastenabled = new int [net->Card[TR]];
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    for(int i = 0; i < net -> Card[TR]; i++)
    {
	lastenabled[i] = ns.Enabled[i] ? 1 : 0;
    }

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    memset(inUpset,0,net->Card[TR]*sizeof(bool));

    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t maxDFSNumber = 1;
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 0;

    //set initial dfs to one
    
    setDFS(payload, currentDFSNumber);
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
//RT::rep->status("NEW ROUND DFS %d MAX %d LOWLINK %d SP %d", currentDFSNumber, maxDFSNumber, currentLowlink, dfsstack.StackPointer);
        if (currentEntry-- > 0)
        {
//RT::rep->status("FIRE @ %d : %s", currentEntry, net -> Name[TR][currentFirelist[currentEntry]]);
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
//RT::rep->status("VISITED");
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
            }
            else
            {
//RT::rep->status("NEW");
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
  		currentDFSNumber = ++maxDFSNumber;
                setDFS(newPayload, currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
		for(int i = 0; i < net -> Card[TR];i++)
		{
			if(ns.Enabled[i]) lastenabled[i] = currentDFSNumber;
		}

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
//RT::rep->status("FIRELIST EXHAUSTED: HIGHEST LL: %d", highest_lowlink);
            //check if this is the root of a tscc
            if (currentDFSNumber == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
//RT::rep->status("ROOT OF TSCC");
		// this is root of tscc 
		// --> check for ignored transitions 
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
		cardUpset = 0;
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(lastfired[t] < getDFS(payload))
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
//RT::rep->status("HAVE IGNORED");
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed

//RT::rep->status("TSCC REALLY CLOSED");
		for(int i = 0; i < net->Card[TR];i++)
		{
			if(lastenabled[i] < getDFS(payload))
			{
//RT::rep->status("RELAXED SEARCH REPORTS FALSE FOR %s", net -> Name[TR][i]);
				portfoliomanager::synchronisetransition(i,TERNARY_FALSE,GLOBAL_GENERALSEARCH);
				return;
			}
		}
		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
		ternary_t * resultvector = new ternary_t[net -> Card[TR]];
		std::fill(resultvector, resultvector+net->Card[TR], TERNARY_TRUE);
//RT::rep->status("RELAXED SEARCH REPORTS TRUE FOR ALL");
		portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_GENERALSEARCH);
                //set the property value
                return;
            }
//RT::rep->status("BACKTRACK");
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
	    currentDFSNumber = getDFS(payload);
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}


void * TSCCExplorationAGEFRelaxed::GlobalHLliveness(void *)
{
	Petrinet * net = Petrinet::InitialNet;
	TSCCExplorationAGEFRelaxed * globalexploration = new TSCCExplorationAGEFRelaxed(net);
	globalexploration -> globalhlliveness_check( * new SimpleProperty(net), * NetState::createNetStateFromInitial(net),*StoreCreator<statenumber_t>::createStore(net,new Mara(),1),* new FirelistStubbornDeadlock(net),1);
	return NULL;
}

void TSCCExplorationAGEFRelaxed::globalhlliveness_check(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    int * tcount = new int[hltransition::card];
    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    int * lastenabled = new int [net->Card[TR]];
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    for(int i = 0; i < net -> Card[TR]; i++)
    {
	lastenabled[i] = ns.Enabled[i] ? 1 : 0;
    }

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    memset(inUpset,0,net->Card[TR]*sizeof(bool));

    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t maxDFSNumber = 1;
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 0;

    //set initial dfs to one
    
    setDFS(payload, currentDFSNumber);
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
//RT::rep->status("NEW ROUND DFS %d MAX %d LOWLINK %d SP %d", currentDFSNumber, maxDFSNumber, currentLowlink, dfsstack.StackPointer);
        if (currentEntry-- > 0)
        {
//RT::rep->status("FIRE @ %d : %s", currentEntry, net -> Name[TR][currentFirelist[currentEntry]]);
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
//RT::rep->status("VISITED");
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
            }
            else
            {
//RT::rep->status("NEW");
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
  		currentDFSNumber = ++maxDFSNumber;
                setDFS(newPayload, currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);
		for(int i = 0; i < net -> Card[TR];i++)
		{
			if(ns.Enabled[i]) lastenabled[i] = currentDFSNumber;
		}

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
//RT::rep->status("FIRELIST EXHAUSTED: HIGHEST LL: %d", highest_lowlink);
            //check if this is the root of a tscc
            if (currentDFSNumber == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
//RT::rep->status("ROOT OF TSCC");
		// this is root of tscc 
		// --> check for ignored transitions 
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
		cardUpset = 0;
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(lastfired[t] < getDFS(payload))
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
//RT::rep->status("HAVE IGNORED");
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed

//RT::rep->status("TSCC REALLY CLOSED");
	        memset(tcount,0,hltransition::card * sizeof(int));
		for(int i = 0; i < net->Card[TR];i++)
		{
			if(lastenabled[i] >= getDFS(payload))
			{
//RT::rep->status("SEEN TR %s FOR HL TR %s", net -> Name[TR][i], ((TransitionSymbol *)(net -> thesymbol[TR][i]))->origin->name);
				tcount[((TransitionSymbol *) (net -> thesymbol[TR][i]))->origin->index]++;
			}
		}
		for(int i = 0; i < hltransition::card;i++)
		{
			if(!tcount[i])
			{
//RT::rep->status("IDENTIFIED NONLIVE TR");
				ternary_t * resultvector = new ternary_t[net -> Card[TR]];
				for(int j = 0; j < net->Card[TR];j++)
				{
					if(((TransitionSymbol *) (net -> thesymbol[TR][j]))->origin->index == i)
					{
						resultvector[j] = TERNARY_FALSE;
					}		
					else
					{
						resultvector[j] = TERNARY_VOID;
					}
				}
				portfoliomanager::synchronisetransition(i,TERNARY_FALSE,GLOBAL_GENERALSEARCH);
				return;

			}
		}
		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
		ternary_t * resultvector = new ternary_t[net -> Card[TR]];
		std::fill(resultvector, resultvector+net->Card[TR], TERNARY_TRUE);
//RT::rep->status("RELAXED SEARCH REPORTS TRUE FOR ALL");
		portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_GENERALSEARCH);
                //set the property value
                return;
            }
//RT::rep->status("BACKTRACK");
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
	    currentDFSNumber = getDFS(payload);
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}


void TSCCExplorationAGEFRelaxed::GlobalReversibility(void)
{
	Petrinet * net = Petrinet::InitialNet;
	TSCCExplorationAGEFRelaxed * globalexploration = new TSCCExplorationAGEFRelaxed(net);
	globalexploration -> globalreversibility_check( * new SimpleProperty(net), * NetState::createNetStateFromInitial(net),*StoreCreator<statenumber_t>::createStore(net,new Mara(),1),* new FirelistStubbornDeadlock(net),1);
	return;
}

void TSCCExplorationAGEFRelaxed::globalreversibility_check(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    memset(inUpset,0,net->Card[TR]*sizeof(bool));

    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t maxDFSNumber = 1;
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 0;

    //set initial dfs to one
     currentDFSNumber = ++maxDFSNumber;
    setDFS(payload, currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                if (dfsstack.StackPointer != 0)
                {
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
		currentDFSNumber = ++maxDFSNumber;
                setDFS(newPayload, currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put a entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
		// this is root of tscc 
		// --> check for ignored transitions 
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
		cardUpset = 0;
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(lastfired[t] < getDFS(payload))
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed

		if(getDFS(payload) > 2)
		{
			// have tscc that does not contain initial state
			RT::rep -> status(RT::rep->markup(MARKUP_BAD,"The net is not reversible").str());
			_exit(1);
		}
		else
		{
			// have tscc that contains initial state
			RT::rep -> status(RT::rep->markup(MARKUP_GOOD,"The net is reversible").str());
			_exit(0);
		}

		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
                //set the property value
                return;
            }
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
	    currentDFSNumber = getDFS((statenumber_t *) dfsstack.top().payload);
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}


void* TSCCExplorationAGEFRelaxed::GlobalHomestates(void*)
{
	Petrinet * net = Petrinet::InitialNet;
	TSCCExplorationAGEFRelaxed * globalexploration = new TSCCExplorationAGEFRelaxed(net);
	globalexploration -> globalhomestates_check( * new SimpleProperty(net), * NetState::createNetStateFromInitial(net),*StoreCreator<statenumber_t>::createStore(net,new Mara(),1),* new FirelistStubbornDeadlock(net),1);
	return NULL;
}

void TSCCExplorationAGEFRelaxed::globalhomestates_check(SimpleProperty &property, NetState &ns,
                                      Store<statenumber_t> &myStore, Firelist &myFirelist, int)
{

    FirelistStubbornCloseGiven * flclose = new FirelistStubbornCloseGiven(net);
    int * lastfired = new int [net->Card[TR]];
    int cardtscc = 0; // total number of tscc
    int carddltscc = 0; // total number of deadlocks (= trivial tscc)

    // if cardtscc = 1 --> net has homestates
    // if cardtscc > 1 and carddltscc > 0 --> net does not have home states (two distinct tscc)
    // if cardtscc > 1 and carddltscc = 0 --> no definite answer (the tscc in reduced graph may belong to the same original tscc)
    for(arrayindex_t i = 0; i < net->Card[TR];i++)
    {
	lastfired[i] = 0;
    }
    // add initial marking to store
    statenumber_t *payload;
    myStore.searchAndInsert(ns, &payload, 0);

    bool * inUpset = new bool[net->Card[TR]];
    arrayindex_t * Upset = new arrayindex_t [net->Card[TR]];
    arrayindex_t cardUpset;
    memset(inUpset,0,net->Card[TR]*sizeof(bool));

    // get initial firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

    //initialise dfsnumber,lowlink and highest_lowlink
    statenumber_t maxDFSNumber = 1;
    statenumber_t currentDFSNumber = 1;
    statenumber_t highest_lowlink = 0;

    //set initial dfs to one
    currentDFSNumber = ++maxDFSNumber;
    setDFS(payload, currentDFSNumber);
    
    statenumber_t currentLowlink = currentDFSNumber;
    
    while (true)
    {
        if (currentEntry-- > 0)
        {
            //fire the next enabled transition
	    arrayindex_t t = currentFirelist[currentEntry];
	    if(getDFS(payload) > lastfired[t]) lastfired[t] = getDFS(payload);
            net->fire(ns, t);

            //already in store eg visited?
            statenumber_t *newPayload;

            // search and insert the current netstate
            if (myStore.searchAndInsert(ns, &newPayload, 0))
            {
                //already visited
                //get the dfs number from the found payload
                statenumber_t newDFS = getDFS(newPayload);
                //backfire the transition because already known
                net->backfire(ns, currentFirelist[currentEntry]);

                //set the lowlink of the top element on the stack to this
                //because we have reached it from this state from the
                //topelement... (only if its smaller)
                if (dfsstack.StackPointer != 0)
                {
                    if (newDFS < currentLowlink)
                    {
                        currentLowlink = newDFS;
                    }
                }
            }
            else
            {
                //not yet visited
                //set the dfs number
                new(dfsstack.push()) DFSStackEntry(currentFirelist, currentEntry, payload, currentLowlink);
		currentDFSNumber = ++maxDFSNumber;
                setDFS(newPayload, currentDFSNumber);
                //set the currentlowlink
                currentLowlink = currentDFSNumber;
	  	payload = newPayload;

                //put a entry on the searchstack
                //with the current dfs number and the current lowlink

                //check the given property and
                //update enabled transitions
                net->updateEnabled(ns, currentFirelist[currentEntry]);

                //update the firelist
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);
            }
        }
        else
        {
            //check if this is the root of a tscc
            if (getDFS( payload) == currentLowlink
                    && highest_lowlink < currentLowlink)
            {
		// this is root of tscc 
		// --> check for ignored transitions 
		bool needEnabled = false;

		memset(inUpset,0,net->Card[TR]*sizeof(bool));
		cardUpset = 0;
		for(arrayindex_t t = 0; t < net->Card[TR];t++)
		{
			if(lastfired[t] < getDFS(payload))
			{
				inUpset[t] = true;
				Upset[cardUpset++] = t;
				lastfired[t] = getDFS(payload);
			}
		}
		if(cardUpset)
		{
			// there are indeed ignored transitions

			// compute new stubborn set around ignored transitions
		    delete [] currentFirelist;
			currentEntry = flclose -> getFirelist(ns, &currentFirelist,Upset,inUpset,cardUpset,&needEnabled);
			if(currentEntry) continue;
		}
		// this tscc is finally closed
		if(!ns.CardEnabled)
		{
			carddltscc++;
		}

		if(++cardtscc > 1)
		{
			if(carddltscc)
			{
				RT::rep -> status(RT::rep->markup(MARKUP_BAD,"The net does not have home states").str());
				_exit(1);
			}
		}

		//update the highest lowlink to the current
		highest_lowlink = currentLowlink;
	    }
            //getting v'
	    delete [] currentFirelist;
	    statenumber_t tmpLowlink = currentLowlink;
            if (dfsstack.StackPointer == 0)
            {
		if(cardtscc <= 1)
		{
			//set the property value
			RT::rep -> status(RT::rep->markup(MARKUP_GOOD,"The net has home states").str());
			_exit(0);
		}
		RT::rep -> status(RT::rep->markup(MARKUP_WARNING,"The net may or may not have home states").str());
		_exit(0);

                return;
            }
            DFSStackEntry &stackentry = dfsstack.top();
            currentLowlink = stackentry.lowlink;
            currentEntry = stackentry.flIndex;
            currentFirelist = stackentry.fl;
  	    payload = reinterpret_cast<statenumber_t *>(stackentry.payload);
            stackentry.fl = NULL;
            dfsstack.pop();
            
            currentDFSNumber = getDFS((statenumber_t *) dfsstack.top().payload);
            if (currentLowlink > tmpLowlink)
            {
                currentLowlink = tmpLowlink;
            }
            assert(currentEntry < net->Card[TR]);
            //backfire and revert the enabledness
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}

