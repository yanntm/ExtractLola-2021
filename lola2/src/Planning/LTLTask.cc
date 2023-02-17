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
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/SymbolTable/Symbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <config.h>
#include <pthread.h>
#include <Core/Dimensions.h>
#include <Planning/StoreCreator.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/Firelist.h>
#include <Exploration/LTLExploration.h>
#include <Exploration/SimpleProperty.h>
#include <Formula/LTL/BuechiFromLTL.h>
#include <Formula/StatePredicate/TruePredicate.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Formula/StatePredicate/FalsePredicate.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Planning/LTLTask.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Exploration/FirelistStubbornLTLTarjan.h>
#include <Exploration/FirelistStubbornLTLDeletion.h>
// #include <Exploration/FirelistStubbornLTL.h>

extern ParserPTNet* symbolTables;
extern int ptbuechi_parse();

extern int ptbuechi_lex_destroy();
pthread_mutex_t ltlmutex = PTHREAD_MUTEX_INITIALIZER;
// input files
extern FILE *ptformula_in;
extern FILE *ptbuechi_in;

// code to parse from a string
struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE ptbuechi__scan_string(const char *yy_str);

extern SymbolTable *buechiStateTable;
extern FILE *tl_out;

// Kimwitu++ objects
extern kc::tBuechiAutomata TheBuechi;


/*!
\ingroup g_globals
\todo Is this mapping actually needed or was it this just added for debugging
purposes.
 */
std::map<int, StatePredicate *> predicateMap;


/* prints the content of a set for spin */
StatePredicate *LTLTask::buildPropertyFromList(int *pos, int *neg)
{
    std::vector<StatePredicate *> subForms;

    // bad hack from library
    int mod = 8 * SIZEOF_INT;

    for (int i = 0; i < sym_size; i++)
    {
        for (int j = 0; j < mod; j++)
        {
            if (pos[i] & (1 << j))
            {
                // the compiler doesn't have a clue which function i mean, so tell him
                if (atoi(sym_table[mod * i + j]) > 1)
                {
                    subForms.push_back(
                            predicateMap[atoi(sym_table[mod * i + j])]->copy(NULL));
                }
            }
            if (neg[i] & (1 << j))
            {
                if (atoi(sym_table[mod * i + j]) > 1)
                {
		     StatePredicate * x = predicateMap[atoi(sym_table[mod * i + j])]->copy(NULL);
		x = x -> negate();

                    subForms.push_back(x);
                }
            }
        }
    }

    if (subForms.empty())
    {
        return new TruePredicate(net);
    }

    if(subForms.size() == 1)
    {
	return subForms[0];
    }

    AtomicBooleanPredicate *result = new AtomicBooleanPredicate(net,true);
    for (arrayindex_t i = 0; i < subForms.size(); i++)
    {
        result->addSub(subForms[i]);
    }
    if(result -> magicnumber == MAGIC_NUMBER_FALSE) 
    {
		
	return new FalsePredicate(net);
    }
   if(result -> magicnumber == MAGIC_NUMBER_TRUE)
   {
	return new TruePredicate(net);
   }
   if(result -> cardSub == 1)
   {
	return result -> sub[0];
  } 
    return result;
}


void LTLTask::ParseBuechi()
{
            RT::currentInputFile = NULL;
            buechiStateTable = new SymbolTable();

            // Check if the paramter of --buechi is a file that we can open: if that
            // works, parse the file. If not, parse the string.
            FILE *file;
            if ((file = fopen(RT::args.buechi_arg, "r")) == NULL and errno == ENOENT)
            {
                // reset error
                errno = 0;
                ptbuechi__scan_string(RT::args.buechi_arg);
            } else
            {
                fclose(file);
                RT::currentInputFile = new Input("Buechi", RT::args.buechi_arg);
                ptbuechi_in = *RT::currentInputFile;
            }

            //RT::rep->message("Parsing Büchi-Automaton");
            // parse the formula
            ptbuechi_parse();

            //RT::rep->message("Finished Parsing");

	pthread_mutex_lock(&kimwitu_mutex);
            // restructure the formula: unfold complex constructs and handle negations and tautologies
            //TheBuechi = TheBuechi->rewrite(kc::goodbye_doublearrows);
            //TheBuechi = TheBuechi->rewrite(kc::goodbye_singlearrows);
            //TheBuechi = TheBuechi->rewrite(kc::goodbye_xor);
            //TheBuechi = TheBuechi->rewrite(kc::goodbye_initial);

            // restructure the formula: again tautoglies and simplification
            TheBuechi = TheBuechi->rewrite(kc::tautology);

            // expand the transitions rules
            TheBuechi = TheBuechi->rewrite(kc::rbuechi);

            //RT::rep->message("parsed Buechi");
            //TheBuechi->unparse(myprinter, kc::out);
	pthread_mutex_unlock(&kimwitu_mutex);

            //RT::rep->message("checking LTL");

}

LTLTask::LTLTask(Petrinet * n, int par, tFormula  f, int fid)
{
	preprocessingfinished = false;
    	net = n;
	parent = par;
        formula = f;
        formula_id = fid;
        result = TERNARY_UNKNOWN;
        memory = new Mara();
	taskname = deconst("LTL model checker");
        portfolio_id = portfoliomanager::addTask(n,this, par,fid,FORMULA_LTL,EXCL_MEM_TASK,memory);
    // process formula
    previousNrOfMarkings = 0;
	goStatus = false;

    if (RT::args.formula_given)
    {

pthread_mutex_lock(&ltlmutex);
        // extract the Node*
	pthread_mutex_lock(&kimwitu_mutex);
	unparsed.clear();
Petrinet::InitialNet =net;
        formula->unparse(myprinter, kc::ltl);
	pthread_mutex_unlock(&kimwitu_mutex);

        tl_Node *n = formula->ltl_tree;
        //n = bin_simpler(n);
        assert(n);
        tl_out = stdout;
        ttrans(n);
        // build the buechi-automation structure needed for LTL model checking
        // put the state predicates
        bauto = new BuechiAutomata(net);

        // extract the states from the ltl2ba data structures
        if (bstates->nxt == bstates)
        {
            // TODO the search result is FALSE!
            RT::rep->message("Not yet implemented, result FALSE");
            RT::rep->abort(ERROR_COMMANDLINE);
        }

        if (bstates->nxt->nxt == bstates && bstates->nxt->id == 0)
        {
            // TODO the search result is TRUE!
            RT::rep->message("Not yet implemented, result TRUE");
            RT::rep->abort(ERROR_COMMANDLINE);
        }

        bauto->cardStates = 0;
        // map-> final,id
        std::map<int, std::map<int, int > > state_id;
        BState *s;
        BTrans *t;
        for (s = bstates->prv; s != bstates; s = s->prv)
        {
            state_id[s->final][s->id] = bauto->cardStates;
            bauto->cardStates++;
        }

        //RT::rep->message("Buechi-automaton has %d states", bauto->cardStates);
        // now i do know the number of states
        bauto->cardTransitions = new int[bauto->cardStates]();
        bauto->nextstate = new int *[bauto->cardStates]();
        bauto->guard = new StatePredicate **[bauto->cardStates]();
        bauto->isStateAccepting = new bool[bauto->cardStates]();

        std::vector<StatePredicate *> neededProperties;

        // read out the datastructure
        int curState = -1;
        int curProperty = 0;
        for (s = bstates->prv; s != bstates; s = s->prv)
        {
            curState++;
            if (s->id == 0)
            {
                // build a TRUE-loop
                bauto->isStateAccepting[curState] = true;
                bauto->cardTransitions[curState] = 1;
                bauto->nextstate[curState] = new int [1]();
                bauto->guard[curState] = new StatePredicate *[1]();
                bauto->nextstate[curState][0] = curState;
                neededProperties.push_back(new TruePredicate(net));
                bauto->guard[curState][0] = neededProperties[curProperty];
                curProperty++;
                continue;
            }
            if (s->final == accepting_state)
            {
                bauto->isStateAccepting[curState] = true;
            }

            // build the successor list
            bauto->cardTransitions[curState] = 0;
            for (t = s->trans->nxt; t != s->trans; t = t->nxt)
            {
                // now build the property
                std::vector<StatePredicate *> disjunctionproperty;
                disjunctionproperty.push_back(buildPropertyFromList(t->pos, t->neg));
                BTrans *t1;
                for (t1 = t; t1->nxt != s->trans;)
                {
                    if (t1->nxt->to->id == t->to->id && t1->nxt->to->final == t->to->final)
                    {
                        disjunctionproperty.push_back(buildPropertyFromList(t1->nxt->pos,
                                t1->nxt->neg));
                        t1->nxt = t1->nxt->nxt;
                    } else
                    {
                        t1 = t1->nxt;
                    }
                }

                if (disjunctionproperty.size() == 1)
                {
                    neededProperties.push_back(disjunctionproperty[0]);
                } else
                {
                    AtomicBooleanPredicate *disjucntion = new AtomicBooleanPredicate(net,false);
                    for (size_t i = 0; i < disjunctionproperty.size(); i++)
                    {
                        disjucntion->addSub(disjunctionproperty[i]);
                    }
                    neededProperties.push_back(disjucntion);
                }
                //RT::rep->message("CREATE %d -> %d", neededProperties.size(), curState);
                // increment number of transitions
                bauto->cardTransitions[curState]++;
            }

            bauto->nextstate[curState] = new int [bauto->cardTransitions[curState]]();
            bauto->guard[curState] = new StatePredicate *[bauto->cardTransitions[curState]]();
            int current_on_trans = -1;
            for (t = s->trans->nxt; t != s->trans; t = t->nxt)
            {
                // bauto data structures
                current_on_trans++;
                bauto->guard[curState][current_on_trans] = neededProperties[curProperty++];
                bauto->nextstate[curState][current_on_trans] =
                        state_id[t->to->final][t->to->id];
            }
        }
pthread_mutex_unlock(&ltlmutex);

        if (RT::args.writeBuechi_given)
        {
        RT::args.writeBuechi_arg = net -> name;
            RT::rep->status("output: Buechi automaton (%s)",
                    RT::rep->markup(MARKUP_PARAMETER, "--writeBuechi").str());
            bauto->writeBuechi();
        }
    }
    if (RT::args.buechi_given)
    {
            // prepare counting of place in the formula
            // copy restructured formula into internal data structures
	pthread_mutex_lock(&kimwitu_mutex);
Petrinet::InitialNet =net;
            TheBuechi->unparse(myprinter, kc::buechi);
	pthread_mutex_unlock(&kimwitu_mutex);
            bauto = TheBuechi->automata;
            // XXX: this _must_ work according to the kimwitu docu, but it does not, kimwitu produces memory leaks!
            //TODO: this makes buechi LTL checks segfaulting in some cases ( now leakes
            //memory (we have to take a closer look)
            //TheBuechi->free(true);
            //delete TheBuechi;
            delete buechiStateTable;

            //RT::rep->message("Processed Büchi-Automaton");

            // report places mentioned in the formula and clean up

            // tidy parser
            ptbuechi_lex_destroy();
            //delete formula;

            if (RT::args.buechi_given)
            {
                delete RT::currentInputFile;
                RT::currentInputFile = NULL;
            }

            // reading the buechi automata
            assert(bauto);
	portfoliomanager::taskjson[portfolio_id]["configuration"]["buchi states"] = static_cast<int>(bauto->getNumberOfStates());
    }
    net -> cardbuchistates = bauto->getNumberOfStates();
}

/*!
\post memory for all members is deallocated
 */
LTLTask::~LTLTask()
{
    // quick exit to avoid lengthy destructor calls
#ifndef USE_PERFORMANCE
    //delete ns;
    //delete ltlStore;
    //delete fl;
    //delete ltlExploration;
    //delete bauto;
#endif
}

/*!
This method starts the actual state space exploration and returns the raw
result.

\return the result of the state exploration
\note This result needs to be interpreted by Task::interpreteResult.
 */
ternary_t LTLTask::getResult()
{

    if(RT::args.stubborn_arg != stubborn_arg_off)
    {
	bauto->setVisible();
    }


    // prepare task
    ltlStore = StoreCreator<LTLPayload>::createStore(net,memory,1);

    // Check, if stubborn sets should be used
    if(RT::args.formula_given)
    {
    switch(RT::args.stubborn_arg)
    {
    case stubborn_arg_off:
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no";
         fl = new Firelist(net);
         break;
    case stubborn_arg_deletion:
	if(formula->containsNext)
	{
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no (formula contains X operator)";
		fl = new Firelist(net);
	}
	else
	{
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "ltl preserving/deletion";
           fl = new FirelistStubbornLTLDeletion(net);
	}
  
    default: 
	if(formula->containsNext)
	{
		portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no (formula contains X operator)";
		fl = new Firelist(net);
	}
	else
	{
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "ltl preserving/insertion";
           fl = new FirelistStubbornLTLTarjan(net);
	}
    }
    }
    else
    {
	portfoliomanager::taskjson[portfolio_id]["configuration"]["stubborn"] = "no (no formula given)";
	fl = new Firelist(net);
    }
    ltlExploration = new LTLExploration(net);
    ns = NetState::createNetStateFromInitial(net);

    //TODO can we make these assumptions clearer that the asserts are creating
    assert(ns);
    assert(ltlStore);
    assert(bauto);
    assert(ltlExploration);
    assert(fl);

    bool bool_result(false);
    ternary_t result(TERNARY_FALSE);
    goStatus = true;
    bool_result = ltlExploration->checkProperty(*bauto, *ltlStore, *fl, *ns);

    // temporary result transfer, as long as the variable bool_result is needed
    if (bool_result)
    {
        result = TERNARY_TRUE;
    }
    switch(result)
    {
	case TERNARY_TRUE: result = TERNARY_FALSE; break;
	case TERNARY_FALSE: result = TERNARY_TRUE; break;
	case TERNARY_UNKNOWN: result = TERNARY_UNKNOWN; break;
	default: break;
    }

     portfoliomanager::report(portfolio_id,result);
    return result;
}

/*!
\post The result is interpreted and printed using Reporter::message
\warning This method must not be called more than once.

\todo This method should be internal and automatically be called by
Task::getResult after the result was calculated. Then, a member of type
trinary_t can be displayed.
 */
char * LTLTask::interpreteResult(ternary_t result)
{
    switch (result)
    {
        case TERNARY_TRUE: return deconst("The net satisfies the given formula (language of the product automaton is empty).");
        case TERNARY_FALSE: return deconst("The net does not satisfy the given formula (language of the product automaton is nonempty).");
        case TERNARY_UNKNOWN: return deconst("The net may or may not satisfy the given formula.");
	default: assert(false);
    }
	return NULL;
}

Path LTLTask::getWitnessPath()
{
    return *(ltlExploration->path);
}

capacity_t *LTLTask::getMarking()
{
    return NULL;
}

statusrecord * LTLTask::getStatistics()
{
	statusrecord * result = new statusrecord();
    result -> markings = ltlStore->get_number_of_markings();
    result -> calls = ltlStore->get_number_of_calls();
	return result;
}

statusrecord * LTLTask::getStatus()
{
	statusrecord * result = new statusrecord();
    result -> markings = ltlStore -> get_number_of_markings();
	result -> calls = ltlStore->get_number_of_calls();
    return result;
}

void LTLTask::derail()
{
	memory->stop = true;
}

Task * LTLTask::copy()
{
	return new LTLTask(net,parent,portfoliomanager::copyFormula(formula), formula_id);
}
