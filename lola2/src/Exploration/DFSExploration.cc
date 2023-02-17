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

int blablabla = 0;
/*!
\file
\author Karsten
\status approved 23.05.2012, changed

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/ChooseTransition.h>
#include <SweepLine/Sweep.h>
#include <Planning/StoreCreator.h>
#include <Net/Petrinet.h>
#include <Exploration/StatePredicateProperty.h>

/*!
The result will be
- true, if a marking fulfilling the property was found
- false, if all markings have been explored and no such state was found
- no termination (in time), if the state space is too big and no 'good' marking
  was found

\param property  the property to check
\param ns  The initial state of the net has to be given as a net state object.
If the search has found a state, fulfilling the property this state will be
returned in this parameter.
\param myStore  the store to be used. The selection of the store may greatly influence the
performance of the program
\param myFirelist  the firelists to use in this search. The firelist _must_ be
applicable to the given property, else the result of this function may be
wrong. It is not guaranteed that the given firelist will actually be used. In
the parallel work-mode the given list will just be used as a base list form
which all other lists will be generated
\param threadNumber  will be ignored by the standard seach. In the parallel
execution mode this number indicates the number of threads to be used for the
search
*/
bool DFSExploration::depth_first(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                                 Firelist &myFirelist, int)
{
    // prepare property
    property.value = property.initProperty(ns);

    if (property.value)
    {
        // initial marking satisfies property
        return true;
    }

    // add initial marking to store
    // we do not care about return value since we know that store is empty

    myStore.searchAndInsert(ns, 0, 0);

    // get first firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);



    while (true) // exit when trying to pop from empty stack
    {
//RT::rep->status("NEW ROUND");
//for(int i = 0; i < net -> Card[PL];i++)
//{
//if(ns.Current[i]) RT::rep->status("%s: %d", net -> Name[PL][i], ns.Current[i]);
//}
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new ns->Current
//RT::rep->status("FIRE %s", net -> Name[TR][currentFirelist[currentEntry]]);
            net->fire(ns, currentFirelist[currentEntry]);


            if (myStore.searchAndInsert(ns, 0, 0))
            {
//RT::rep->status("EXISTS");
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
//RT::rep->status("NEW");
                // State does not exist!
                net->updateEnabled(ns, currentFirelist[currentEntry]);
                // check current marking for property
                property.value = property.checkProperty(ns, currentFirelist[currentEntry]);
                if (property.value)
                {
//RT::rep->status("HOLDS");
                    // current  marking satisfies property
                    // push put current transition on stack
                    // this way, the stack contains ALL transitions
                    // of witness path
                    SimpleStackEntry *stack = property.stack.push();
                    stack = new(reinterpret_cast<void *>(stack)) SimpleStackEntry(currentFirelist, currentEntry);
                    return true;
                }
//RT::rep->status("DOES NOT HOLD");
                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = property.stack.push();
                stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

            } // end else branch for "if state exists"
        }
        else
        {
//RT::rep->status("COMPLETED");
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
            if (property.stack.StackPointer == 0)
            {
                // have completely processed initial marking --> state not found
                return false;
            }
            SimpleStackEntry &stack = property.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            property.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
            property.value = property.updateProperty(ns, currentFirelist[currentEntry]);
        }
    }
}

void DFSExploration::check_quasilive(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                                 FirelistStubbornStatePredicate &myFirelist, int)
{
    Petrinet * net = Petrinet::InitialNet;
    arrayindex_t ttt = portfoliomanager::gettargettransition();
//RT::rep->status("-----------------------------------------------------------------------------------------------------------");
//RT::rep->status("NEW SEARCH FOR %s",net ->Name[TR][ttt]);
//RT::rep->status("-----------------------------------------------------------------------------------------------------------");
//RT::rep->status("");
    ternary_t * resultvector = new ternary_t [net -> Card[TR]];
    std::fill(resultvector, resultvector+net->Card[TR], TERNARY_VOID);



    for(int i = 0; i < net->Card[TR];i++)
    {
	if(ns.Enabled[i]) 
	{
		resultvector[i] = TERNARY_TRUE;
	}
    }
    if(resultvector[ttt] == TERNARY_TRUE)
    {
//RT::rep->status("INITIALLY ENABLED -> RETURN");
	portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_SEARCH);
	delete [] resultvector;
        return;
    }
    // add initial marking to store
    // we do not care about return value since we know that store is empty

    myStore.searchAndInsert(ns, 0, 0);

    // get first firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ttt,ns, &currentFirelist);



    int round = 1;
    while (true) // exit when trying to pop from empty stack
    {
//RT::rep->status("NEW ROUND SP %d", property.stack.StackPointer);
//for(int i = 0; i < currentEntry;i++)
//{
//RT::rep->status("FL %s", net -> Name[TR][currentFirelist[i]]);
//}
	if(! (round++ % 10000))
	{
			portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_SEARCH);
			if(portfoliomanager::globalresult[ttt] != TERNARY_VOID) return;
	}
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new ns->Current
            net->fire(ns, currentFirelist[currentEntry]);
//RT::rep->status("FIRE %s", net -> Name[TR][currentFirelist[currentEntry]]);


            if (myStore.searchAndInsert(ns, 0, 0))
            {
//RT::rep->status("EXISTS -> BT");
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
//RT::rep->status("NEW");

                // State does not exist!
                net->updateEnabled(ns, currentFirelist[currentEntry]);
		    for(int i = 0; i < net->Card[TR];i++)
		    {
			if(ns.Enabled[i]) 
			{
				resultvector[i] = TERNARY_TRUE;
			}
		    }
		    if(resultvector[ttt] == TERNARY_TRUE)
		    {
//RT::rep->status("TARGET ENABLED -> RETURN");
			portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_SEARCH);
			delete [] resultvector;
			return;
		    }

                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = property.stack.push();
//RT::rep->status("SP AFTER PUSH: %d", property.stack.StackPointer);
                stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(ttt,ns, &currentFirelist);

            } // end else branch for "if state exists"
        }
        else
        {
//RT::rep->status("FL EXHAUSTED -> BT");
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
//RT::rep->status("SP: %d", property.stack.StackPointer);
            if (property.stack.StackPointer == 0)
            {
//RT::rep->status("STACK EXHAUSTED -> RETURN FALSE");
                // have completely processed initial marking --> state not found
		resultvector[ttt] = TERNARY_FALSE;
//RT::rep->status("REP FALSE FOR %s", net -> Name[TR][ttt]);
		portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_SEARCH);
                return;
            }
            SimpleStackEntry &stack = property.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            property.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}

void DFSExploration::check_constantplace(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                                 FirelistStubbornTsccAlwaysUpset &myFirelist, int)
{
    Petrinet * net = Petrinet::InitialNet;
    arrayindex_t ppp = portfoliomanager::gettargetplace();
    ternary_t * resultvector = new ternary_t [net -> Card[PL]];
    std::fill(resultvector, resultvector+net->Card[PL], TERNARY_VOID); 
    arrayindex_t stubbornbasecard = net -> PlCardIncreasing[ppp] + net -> PlCardDecreasing[ppp];
    arrayindex_t * stubbornbasevector = new arrayindex_t[stubbornbasecard];
    for(arrayindex_t i = 0; i < net -> PlCardIncreasing[ppp];i++)
    {
	stubbornbasevector[i] = net -> PlIncreasing[ppp][i];
    }
    if(net -> Initial[ppp] == 0)
    {
	stubbornbasecard = net -> PlCardIncreasing[ppp];
    }
    else
    {
	for(int i = 0; i < net->PlCardDecreasing[ppp];i++)
	{
		stubbornbasevector[i + net -> PlCardIncreasing[ppp]] = net->PlDecreasing[ppp][i];
	}
    }

    // add initial marking to store
    // we do not care about return value since we know that store is empty

    myStore.searchAndInsert(ns, 0, 0);

    // get first firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(stubbornbasevector, stubbornbasecard,ns, &currentFirelist);



    while (true) // exit when trying to pop from empty stack
    {
        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new ns->Current
            net->fire(ns, currentFirelist[currentEntry]);


            if (myStore.searchAndInsert(ns, 0, 0))
            {
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
                // State does not exist!
                net->updateEnabled(ns, currentFirelist[currentEntry]);
		    for(int i = 0; i < net->Card[PL];i++)
		    {
			if(ns.Current[i] != net -> Initial[i]) resultvector[i] = TERNARY_TRUE;
		    }
		    if(resultvector[ppp] == TERNARY_TRUE)
		    {
			portfoliomanager::synchroniseplacevector(resultvector,GLOBAL_SEARCH);
			delete [] resultvector;
			return;
		    }

                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = property.stack.push();
                stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(stubbornbasevector,stubbornbasecard,ns, &currentFirelist);

            } // end else branch for "if state exists"
        }
        else
        {
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
            if (property.stack.StackPointer == 0)
            {
                // have completely processed initial marking --> state not found
  		resultvector[ppp] = TERNARY_FALSE;
		portfoliomanager::synchroniseplacevector(resultvector,GLOBAL_SEARCH);
                return;
            }
            SimpleStackEntry &stack = property.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            property.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}

/*!
The result will be
- true, if the state was found
- false, if max attempt(with maxdepth) exhausted without having found state

\param property the property for which a path should be found
\param ns the net state of the initial marking
\param attempts number of attempts. An argument value of 0 will lead to an
unlimited number of attempts: the function will not return if the property not
satisfiable
\param maxdepth maximal depth of each attempt
\param myFirelist the object used to generate the actual firelists, this must
correspond with given property
\param s the user have to given an instance of the empty store
\param c ChooseTransition object to determine which enabled transition to fire.
*/
bool DFSExploration::find_path(SimpleProperty &property, NetState &ns,
                               Firelist &myFirelist, EmptyStore<void> &s,
                               ChooseTransition &c)
{
    // this table counts hits for various hash buckets. This is used for
    // steering search into less frequently entered areas of the state space.

StatePredicateProperty * spp = (StatePredicateProperty *) &property;

    unsigned int currentattempt = 0;

    // get memory for witness path info
    arrayindex_t *witness = new arrayindex_t[maxdepth];

    _p = Path(net);

    // loop #attempts times
//RT::rep->status("NEW ATT");
    while (attempts == 0 || currentattempt++ < attempts)
    {
        // register this try
        s.tries++;

        // copy initial marking into current marking
        memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
        ns.HashCurrent = net->HashInitial;

        // reset enabledness information
        ns.CardEnabled = net->Card[TR];
        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            ns.Enabled[t] = true;
        }

        for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
        {
            net->checkEnabled(ns, t);
        }

        // prepare property
        property.value = property.initProperty(ns);


        if (property.value)
        {
            // initial marking satisfies property: witness path is empty
            _p.initialized = true;
            delete[] witness;
            return true;
        }

        // generate a firing sequence until given depth or deadlock is reached
        for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
        {
            // register this transition's firing
            s.searchAndInsert(ns, 0, 0);

            // get firelist
            arrayindex_t *currentFirelist;
            arrayindex_t cardFirelist = myFirelist.getFirelist(ns, &currentFirelist);
            if (cardFirelist == 0)
            {
                // deadlock or empty up set (i.e. property not reachable)
                break; // go to next attempt
            }


            arrayindex_t chosen = c.choose(ns, cardFirelist, currentFirelist);
            delete [] currentFirelist;
//RT::rep->status("FP %s", net -> Name[TR][chosen]);


            witness[depth] = chosen;
            net->fire(ns, chosen);
            net->updateEnabled(ns, chosen);

            property.value = property.checkProperty(ns, chosen);
	    if(property.value)
            {

//RT::rep->status("FOUND");
                // witness path is witness[0],...,witness[depth-1],
                // witness[depth]; current marking satisfies property.
                // copy witness to Path object _p
                _p.initialized = true;
                for (arrayindex_t i = 0; i <= depth; ++i)
                {
                    _p.addTransition(witness[i], false);
                }
                delete[] witness;
                return true;
            }
        }
    }

    // we did not find a state after #attempts
    delete[] witness;
    return false;
}

Path DFSExploration::path() const
{
    return _p;
}

/*!
\param property the property for which a witness state should be found
\param ns the net state of the initial marking
\param myStore A dummy store for counting markings
\param myFirelist the object used to generate the actual firelists, this must correspond with given property
\param frontNumber  indicates the number of sweepline-fronts to be used
\param threadNumber will be ignored by the standard search.
       In the parallel execution mode this number indicates the number of threads to be used for the search
\return if the state was found
*/
bool DFSExploration::sweepline(SimpleProperty &property, NetState &ns, SweepEmptyStore &myStore,
                               Firelist &myFirelist, int frontNumber, int threadNumber,Mara * mem)
{
    s = new Sweep<void> (net,property, ns, myStore, myFirelist, frontNumber, threadNumber,mem);
    bool result = (*s).run();
    return result;
}

/*!
\param property the property for which a witness state should be found
\param ns the net state of the initial marking
\param myStore The store where marking are saved
\param myFirelist the object used to generate the actual firelists, this must correspond with given property
\param threadNumber will be ignored by the standard search.
       In the parallel execution mode this number indicates the number of threads to be used for the search
\param type The type of formula to solve. Currently AGEF and EF (liveness/reachability) are recognised.
\return if the state was found
*/
ternary_t DFSExploration::cover_breadth_first(SimpleProperty &property, NetState &ns,
        Store<CoverPayload> &myStore,
        Firelist &myFirelist, int threadNumber, formula_t type)
{
    CoverGraph c(net,property, ns, myStore, myFirelist, _p, threadNumber);
    // the following condition must be replaced by one distinguishing AGEF from EF
    if (type == FORMULA_LIVENESS)
    {
        c.setAGEF();
    }
    return c.run();
}

void * DFSExploration::Quasiliveness(void *)
{       
        // do whole planning stuff
        // create Mara object
        
        
	Mara * mem = new Mara();
        Petrinet * net = Petrinet::InitialNet;
        mem -> taskid = 3;
        portfoliomanager::taskjson[3] = JSON();
        NetState * ns;
        
        // init target place
        arrayindex_t targetplace = 0;
        
        while(true)
        {       
                Store<void> * store = StoreCreator<void>::createStore(net,mem,1);
                ns = NetState::createNetStateFromInitial(net);
                SimpleProperty * property = new SimpleProperty(net);
                // run for target place 
                check_quasilive(*property, *ns, *store, * new FirelistStubbornStatePredicate(net,NULL), 1);     
                
                // reset Mara memory
                mem->reset();
                delete property;
                
        }
}

void * DFSExploration::Constantplace(void *)
{       
        // do whole planning stuff
        // create Mara object
        
        
	Mara * mem = new Mara();
        Petrinet * net = Petrinet::InitialNet;
        mem -> taskid = 3;
        portfoliomanager::taskjson[3] = JSON();
        NetState * ns;
        
        // init target place
        arrayindex_t targetplace = 0;
        
        while(true)
        {       
                Store<void> * store = StoreCreator<void>::createStore(net,mem,1);
                ns = NetState::createNetStateFromInitial(net);
                SimpleProperty * property = new SimpleProperty(net);
                // run for target place 
                check_constantplace(*property, *ns, *store, * new FirelistStubbornTsccAlwaysUpset(net,NULL), 1);     
                
                // reset Mara memory
                mem->reset();
                delete property;
                
        }
}

void *  DFSExploration::findpathquasilivenessthread(void *)
{
    Petrinet * net = Petrinet::InitialNet;
    FirelistStubbornStatePredicate * myFirelist = new FirelistStubbornStatePredicate(net, NULL);
    NetState ns = * NetState::createNetStateFromInitial(net);
    ternary_t * resultvector = new ternary_t [net -> Card[TR]];
    for(int i = 0; i < net -> Card[TR];i++)
    {
	resultvector[i] = TERNARY_VOID;
    }
    while(true)
    {
	    // try new target
	    arrayindex_t ttt = portfoliomanager::gettargettransition();
	    if(ttt == -1) return NULL;
            unsigned int attempts = RT::args.retrylimit_arg;
            if(attempts == 0) attempts = 3;
            unsigned int maxdepth = RT::args.depthlimit_arg;
	    if(maxdepth <= 0) maxdepth = 100000;
   
	    unsigned int currentattempt = 0;

	    // get memory for witness path info

	    // loop #attempts times
	    while (currentattempt++ < attempts)
	    {
		if(portfoliomanager::globalresult[ttt] != TERNARY_VOID) return NULL;
		// copy initial marking into current marking
		memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
		ns.HashCurrent = net->HashInitial;

		// reset enabledness information
		ns.CardEnabled = net->Card[TR];
		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    ns.Enabled[t] = true;
		}

		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    net->checkEnabled(ns, t);
		   if(ns.Enabled[t]) 
		   {
			resultvector[t] = TERNARY_TRUE;
		   }
		}
		if(resultvector[ttt] == TERNARY_TRUE) break;

		// generate a firing sequence until given depth or deadlock is reached
		for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
		{
		    // get firelist
		    arrayindex_t *currentFirelist;
		    arrayindex_t cardFirelist = myFirelist->getFirelist(ttt,ns, &currentFirelist);
		    if (cardFirelist == 0)
		    {
			// deadlock or empty up set (i.e. property not reachable)
			break; // go to next attempt
		    }


                    arrayindex_t chosen = rand() % ((cardFirelist * (cardFirelist +1 )) / 2);
                    int i;
                    for(i = 0; i < cardFirelist; i++)
                    {
                        if(chosen > ((cardFirelist - i - 1) * ( cardFirelist - i)) / 2)
                        {
                                chosen = currentFirelist[i];
                                break;
                        }
                    }
		    if(i >= cardFirelist) chosen = currentFirelist[0];
		    delete [] currentFirelist;

		    net->fire(ns, chosen);
		    net->updateEnabled(ns, chosen);
		    for(int i = 0; i < net->Card[TR];i++)
		    {
			   if(ns.Enabled[i]) 
			   {
				resultvector[i] = TERNARY_TRUE;
			   }
		    }

		    if(resultvector[ttt] == TERNARY_TRUE)
		    {
			break;
		    }
		}
		    if(resultvector[ttt] == TERNARY_TRUE)
		    {
			break;
		    }

		    if(portfoliomanager::globalresult[ttt] != TERNARY_VOID) break;
	            portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_FINDPATH);
	    }
    }
}

void *  DFSExploration::findpathconstantplacegtthread(void *)
{
    Petrinet * net = Petrinet::InitialNet;
    NetState ns = * NetState::createNetStateFromInitial(net);
    FirelistStubbornTsccAlwaysUpset *myFirelist = new FirelistStubbornTsccAlwaysUpset(net,NULL);
    ternary_t * resultvector = new ternary_t [net -> Card[PL]];
    std::fill(resultvector,resultvector+net->Card[PL],TERNARY_VOID);
    while(true)
    {
	    // try new target
	    arrayindex_t ppp = portfoliomanager::gettargetplace();
	    if(ppp == -1) return NULL;
            unsigned int attempts = RT::args.retrylimit_arg;
            if(attempts == 0) attempts = 3;
            unsigned int maxdepth = RT::args.depthlimit_arg;
	   maxdepth = 100000;

	    unsigned int currentattempt = 0;

	    // get memory for witness path info

	    // loop #attempts times
	    while (currentattempt++ < attempts)
	    {
		// copy initial marking into current marking
		memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
		ns.HashCurrent = net->HashInitial;

		// reset enabledness information
		ns.CardEnabled = net->Card[TR];
		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    ns.Enabled[t] = true;
		}

		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    net->checkEnabled(ns, t);
		}

		// generate a firing sequence until given depth or deadlock is reached
		for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
		{
		    // get firelist
		    arrayindex_t *currentFirelist;
		    arrayindex_t cardFirelist = myFirelist->getFirelist(net->PlIncreasing[ppp],net->PlCardIncreasing[ppp],ns, &currentFirelist);
		    if (cardFirelist == 0)
		    {
			// deadlock or empty up set (i.e. property not reachable)
			break; // go to next attempt
		    }

                    arrayindex_t chosen = rand() % ((cardFirelist * (cardFirelist +1 )) / 2);
                    int i;
                    for(i = 0; i < cardFirelist; i++)
                    {
                        if(chosen > ((cardFirelist - i - 1) * ( cardFirelist - i)) / 2)
                        {
                                chosen = currentFirelist[i];
                                break;
                        }
                    }
		    if(i >= cardFirelist) chosen = currentFirelist[0];
		    delete [] currentFirelist;

		    net->fire(ns, chosen);
		    net->updateEnabled(ns, chosen);
		    for(int i = 0; i < net->Card[PL];i++)
		    {
			   if(ns.Current[i] != net -> Initial[i]) 
			   {
				resultvector[i] = TERNARY_TRUE;
			   }
		    }

		    if(resultvector[ppp] == TERNARY_TRUE)
		    {
			break;
		    }
		}
	    }
	    portfoliomanager::synchroniseplacevector(resultvector,GLOBAL_FINDPATH);
    }
}

void *  DFSExploration::findpathconstantplaceltthread(void *)
{
    Petrinet * net = Petrinet::InitialNet;
    NetState ns = * NetState::createNetStateFromInitial(net);
    FirelistStubbornTsccAlwaysUpset *myFirelist = new FirelistStubbornTsccAlwaysUpset(net,NULL);
    ternary_t * resultvector = new ternary_t [net -> Card[PL]];
    std::fill(resultvector,resultvector+net->Card[PL],TERNARY_VOID);
    while(true)
    {
	    // try new target
	    arrayindex_t ppp = portfoliomanager::gettargetplace();
	    if(ppp == -1) return NULL;
            unsigned int attempts = RT::args.retrylimit_arg;
            if(attempts == 0) attempts = 3;
            unsigned int maxdepth = RT::args.depthlimit_arg;
	   maxdepth = 100000;

	    unsigned int currentattempt = 0;

	    // get memory for witness path info

	    // loop #attempts times
	    while (currentattempt++ < attempts)
	    {
		// copy initial marking into current marking
		memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
		ns.HashCurrent = net->HashInitial;

		// reset enabledness information
		ns.CardEnabled = net->Card[TR];
		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    ns.Enabled[t] = true;
		}

		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    net->checkEnabled(ns, t);
		}

		// generate a firing sequence until given depth or deadlock is reached
		for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
		{
		    // get firelist
		    arrayindex_t *currentFirelist;
		    arrayindex_t cardFirelist = myFirelist->getFirelist(net->PlDecreasing[ppp],net->PlCardDecreasing[ppp],ns, &currentFirelist);
		    if (cardFirelist == 0)
		    {
			// deadlock or empty up set (i.e. property not reachable)
			break; // go to next attempt
		    }

                    arrayindex_t chosen = rand() % ((cardFirelist * (cardFirelist +1 )) / 2);
                    int i;
                    for(i = 0; i < cardFirelist; i++)
                    {
                        if(chosen > ((cardFirelist - i - 1) * ( cardFirelist - i)) / 2)
                        {
                                chosen = currentFirelist[i];
                                break;
                        }
                    }
		    if(i >= cardFirelist) chosen = currentFirelist[0];
		    delete [] currentFirelist;

		    net->fire(ns, chosen);
		    net->updateEnabled(ns, chosen);
		    for(int i = 0; i < net->Card[PL];i++)
		    {
			   if(ns.Current[i] != net -> Initial[i]) 
			   {
				resultvector[i] = TERNARY_TRUE;
			   }
		    }

		    if(resultvector[ppp] == TERNARY_TRUE)
		    {
			break;
		    }
		}
	    }
	    portfoliomanager::synchroniseplacevector(resultvector,GLOBAL_FINDPATH);
    }
}

void *  DFSExploration::findpathdeadlocklivenessthread(void *)
{
    Petrinet * net = Petrinet::InitialNet;
    FirelistStubbornDeadlock * myFirelist = new FirelistStubbornDeadlock(net);
    NetState ns = * NetState::createNetStateFromInitial(net);
    ternary_t * resultvector;
    if(RT::args.check_arg != check_arg_deadlockfreedom)
    {
	    resultvector = new ternary_t [net -> Card[TR]];
	    std::fill(resultvector,resultvector+net->Card[TR],TERNARY_FALSE);
    }
    unsigned int maxdepth = RT::args.depthlimit_arg;
    while(true)
    {
		// copy initial marking into current marking
		memcpy(ns.Current, net->Initial, net->Card[PL] * SIZEOF_ARRAYINDEX_T);
		ns.HashCurrent = net->HashInitial;

		// reset enabledness information
		ns.CardEnabled = net->Card[TR];
		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    ns.Enabled[t] = true;
		}

		for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
		{
		    net->checkEnabled(ns, t);
		}
		if(!ns.CardEnabled) 
		{
			if(RT::args.check_arg == check_arg_deadlockfreedom)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"A deadlock is reachable.").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			}
			else
			{
				portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_FINDPATH);
			}
		}

		// generate a firing sequence until given depth or deadlock is reached
		for (arrayindex_t depth = 0; depth < maxdepth; ++depth)
		{
		    // get firelist
		    arrayindex_t *currentFirelist;
		    arrayindex_t cardFirelist = myFirelist->getFirelist(ns, &currentFirelist);
		    if (cardFirelist == 0)
		    {
			if(RT::args.check_arg == check_arg_deadlockfreedom)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"A deadlock is reachable.").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			}
			else
			{
				// deadlock or empty up set (i.e. property not reachable)
				portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_FINDPATH);
				return NULL; 
			}
		    }

                    arrayindex_t chosen = rand() % ((cardFirelist * (cardFirelist +1 )) / 2);
                    int i;
                    for(i = 0; i < cardFirelist; i++)
                    {
                        if(chosen > ((cardFirelist - i - 1) * ( cardFirelist - i)) / 2)
                        {
                                chosen = currentFirelist[i];
                                break;
                        }
                    }
		    if(i >= cardFirelist) chosen = currentFirelist[0];
		    delete [] currentFirelist;

		    net->fire(ns, chosen);
		    net->updateEnabled(ns, chosen);
		}
    }
}

void *  DFSExploration::deadlockthread(void *)
{
    Petrinet * net = Petrinet::InitialNet;
    SimpleProperty property = * new SimpleProperty(net); 
    NetState ns = * NetState::createNetStateFromInitial(net);
    Mara * mem = new Mara();
    Store<void> * myStore = StoreCreator<void>::createStore(net,mem,1);
    Firelist myFirelist = * new FirelistStubbornDeadlock(net);

    if (!ns.CardEnabled)
    {
	if(RT::args.check_arg == check_arg_deadlockfreedom)
	{
		RT::rep->status(RT::rep->markup(MARKUP_BAD,"A deadlock is reachable.").str());
		portfoliomanager::compareresult(true);
		if(RT::args.mcc_given)
		{
			portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
		}
		_exit(0);
	}
	else
	{
		ternary_t * resultvector = new ternary_t[net->Card[TR]];
		std::fill(resultvector,resultvector+net -> Card[TR],TERNARY_FALSE);
		portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_GENERALSEARCH);
		return NULL;
	}
    }

    // add initial marking to store
    // we do not care about return value since we know that store is empty

    myStore->searchAndInsert(ns, 0, 0);

    // get first firelist
    arrayindex_t *currentFirelist;
    arrayindex_t currentEntry = myFirelist.getFirelist(ns, &currentFirelist);



    while (true) // exit when trying to pop from empty stack
    {

        if (currentEntry-- > 0)
        {
            // there is a next transition that needs to be explored in current marking

            // fire this transition to produce new ns->Current
            net->fire(ns, currentFirelist[currentEntry]);


            if (myStore->searchAndInsert(ns, 0, 0))
            {
                // State exists! -->backtracking to previous state
                net->backfire(ns, currentFirelist[currentEntry]);
            }
            else
            {
                // State does not exist!
                net->updateEnabled(ns, currentFirelist[currentEntry]);
                // check current marking for property
                if (!ns.CardEnabled)
                {
			if(RT::args.check_arg == check_arg_deadlockfreedom)
			{
				RT::rep->status(RT::rep->markup(MARKUP_BAD,"A deadlock is reachable.").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			}
			else
			{
				ternary_t * resultvector = new ternary_t[net->Card[TR]];
				std::fill(resultvector,resultvector+net -> Card[TR],TERNARY_FALSE);
				portfoliomanager::synchronisetransitionvector(resultvector,GLOBAL_GENERALSEARCH);
				return NULL;
			}
                }

                // Here: current marking does not satisfy property --> continue search
                SimpleStackEntry *stack = property.stack.push();
                stack = new(stack) SimpleStackEntry(currentFirelist, currentEntry);
                currentEntry = myFirelist.getFirelist(ns, &currentFirelist);

            } // end else branch for "if state exists"
        }
        else
        {
            // firing list completed -->close state and return to previous state
            delete[] currentFirelist;
            if (property.stack.StackPointer == 0)
            {
		if(RT::args.check_arg == check_arg_deadlockfreedom)
		{
			RT::rep->status(RT::rep->markup(MARKUP_GOOD,"No deadlock is reachable.").str());
			portfoliomanager::compareresult(false);
			if(RT::args.mcc_given)
			{
				portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
			}
			_exit(0);
		}
		else
		{
			// have completely processed initial marking --> state not found
			return NULL;
		}
            }
            SimpleStackEntry &stack = property.stack.top();
            currentEntry = stack.current;
            currentFirelist = stack.fl;
            stack.fl = NULL;
            property.stack.pop();
            assert(currentEntry < net->Card[TR]);
            net->backfire(ns, currentFirelist[currentEntry]);
            net->revertEnabled(ns, currentFirelist[currentEntry]);
        }
    }
}

