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
\author Niels
\status approved 25.01.2012

\brief Main entry point for LoLA.
 */


#include <string>
#include <fstream>
#include <streambuf>
#include <config.h>
#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Core/Handlers.h>
#include <Core/Runtime.h>
#include <Core/mysemaphore.h>
#include <CoverGraph/CoverGraph.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/formula_abstract.h>
#include <Frontend/Parser/ParserPnml.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <InputOutput/InputOutput.h>
#include <InputOutput/JSON.h>
#include <Planning/Task.h>
#include <Exploration/DFSExploration.h>
#include <Exploration/TSCCExploration.h>
#include <Planning/LTLTask.h>
#include <Planning/EmptyTask.h>
#include <Planning/FullTask.h>
#include <Planning/StateEquationTask.h>
#include <Witness/Path.h>
#include <Memory/Mara.h>
#include <Rapidxml/rapidxml.h>
#include <Rapidxml/rapidxml_iterators.h>
#include <Rapidxml/rapidxml_utils.h>
#include <Rapidxml/rapidxml_print.h>
#include <ctime>
#include <cstdlib>

#include <Exploration/ComputePlacesBounds.h>  //test
#include <Exploration/ComputeBoundExploration.h>  //test
#include <Exploration/FirelistStubbornComputeBound.h>  //test
#include <Planning/SiphonTrapTask.h>

void environment(Petrinet *);
pthread_mutex_t kimwitu_mutex = PTHREAD_MUTEX_INITIALIZER;
/*!
\brief symbol tables for the parsed net
\ingroup g_globals
 */
tFormula TheHLFormula;

/*!
\brief symbol table for a given Büchi automaton
\ingroup g_globals
 */
SymbolTable *buechiStateTable = NULL;

pthread_mutex_t sat_mutex = PTHREAD_MUTEX_INITIALIZER;

/*!
\brief the main workflow of LoLA
 */


// launch portfolio manager in its own thread
void * portfolio_thread(void *)
{
	portfoliomanager::run();
	return NULL;
}

void * globalproperty_timer(void *)
{
	if(RT::args.timelimit_given)
	{
		sleep(RT::args.timelimit_arg);
		_exit(1);
	}
	return NULL;
}

extern int * compoundformulaid;
extern tFormula * compoundformula;
extern tFormula * skeletonformula;

extern ParserPTNet * LLsymbolTables;
extern ParserPTNet * HLsymbolTables;

ProtoNet * skeletonnet;

// Possible status of net evolution:
// 1st dimension: shape
// * SY: as symboltables for places and transitions with arc lists at transitions
// * PR: as protonet (objects for places, transitions, arcs. places and arcs are linked to a symbol table
// * PN: as petrinet (index structures for everything)
//
// 2nd dimension: distribution
// * GL: one for all formulas
// * LC: individual per formula
// 
// 3rd dimension: net class
// * HL: (original CP net)
// * LL: (original PT net)
// * UF: (unfolded to PT net)
// * SK: (folded to skeleton)
//
// 4th dimension: progress
// * CR: as freshly created
// * RG: after global reduction
// * RL: after local reduction
// * Px: after preprocessing step x

// Possible status of formula evolution:
// 1st dimension: net class
// 2nd dimension: attached net 

int main(int argc, char **argv)
{


	ParserPTNet * symbolTables;

    // This routine is responsible for the workflow of LoLA between
    // launching the tool and task generation / portfolio management.
    // The workflow has several branches depending on
    // whether net is given as HL net or LL net
    // what verfication option is chosen (full, none, modelchecking
    // whether property is given as formula or as Buchi automaton
    // whether formula is in HL or LL format.

    //=================
    // (1) set up LoLA
    //=================

    portfoliomanager::globalstarttime = time(NULL); // record start time (for portfolio managemment)
    std::srand(42); // initialise random number generator (for repeatable hash parameters)
    RT::initialize(argc, argv); // initialize the runtime (includes parsing of cmdline options)
    for(int i = 0; i < MAX_TASKS;i++)  // initialise parent array in portfolio manager 
    {                                  // (crucial for termination)
            portfoliomanager::parent[i] = -1;
    }
    threadid_t number_of_threads = static_cast<threadid_t>(RT::args.threads_arg);
	portfoliomanager::init();
	RT::args.threads_arg = 1;

    // inspect verdict file, if given, and modify cmdline options

        if(RT::args.verdictfile_given)
        {
                std::ifstream verdict(RT::args.verdictfile_arg);
                if(verdict)
		{
			std::string verdictline;
			while(std::getline(verdict, verdictline))
                        if(verdictline.find("DEADLOCK") != std::string::npos && verdictline.find("false") != std::string::npos)
                        {
                                RT::args.deadlockfree_given = true;
                        }
			if(verdictline.find("SAFE") != std::string::npos && verdictline.find("true") != std::string::npos)
                        {
                                RT::args.safe_given = true;
                        }
                }
        }


    //===================
    // (2) process net
    //===================

    // Net is expected as file (name passed on cmdline) 
    // or as stream (via stdin; no name passed on cmdline). 
    // In both cases, input format may be either pnml (--pnmlnet) or native
    // LoLA input (default).
    
    RT::rep->status("NET"); // prepare status info for net input
    RT::rep->indent(2);

    if(RT::args.pnmlnet_given)
    {
	// input expected in PNML
	RT::rep->status("input: PNML file (%s)", RT::rep->markup(MARKUP_PARAMETER, "--pnmlnet").str());
        if (RT::args.inputs_num == 0)
        {
            // read from stdin
            RT::currentInputFile = new Input("net");
        }
        else if (RT::args.inputs_num == 1)
        {
	    // read from specified file
            RT::currentInputFile = new Input("net", RT::args.inputs[0]);
        }
        else
        {
            RT::rep->message("too many files given - expecting at most one");
            RT::rep->abort(ERROR_COMMANDLINE);
        }
        symbolTables = ReadPnmlFile();

        RT::rep->status("finished parsing");

        // close net file
        delete RT::currentInputFile;
        RT::currentInputFile = NULL;

	// LoLA survives function ReadPnmlFile only if net input is syntactically correct.
	// If PNML file/stream contains a LL net, 
	// this is translated into symbol table representation as is.	
	// If PNML file contains a HL net,
	// only the skeleton of the net is represented in symbol table representation.
	// The signature of the HL net is preserved in the HL net data structures for later
	// unfolding.
    }
    else
    {
        // input expected as LoLA file
        if (RT::args.inputs_num == 0)
        {
            // read from stdin
            RT::currentInputFile = new Input("net");
        }
        else if (RT::args.inputs_num == 1)
        {
	    // read from specified file
            RT::currentInputFile = new Input("net", RT::args.inputs[0]);
        }
        else
        {
            RT::rep->message("too many files given - expecting at most one");
            RT::rep->abort(ERROR_COMMANDLINE);
        }

        // pass the opened file pointer to flex via FILE *yyin
        extern FILE *ptnetlola_in;
        ptnetlola_in = *RT::currentInputFile;

        // read the input file(s)
        extern ParserPTNet * ParserPTNetLoLA();
        symbolTables = ParserPTNetLoLA();

        RT::rep->status("finished parsing");

        // close net file
        delete RT::currentInputFile;
        RT::currentInputFile = NULL;
        
        // tidy parser
        extern int ptnetlola_lex_destroy();
        ptnetlola_lex_destroy();

	// ParserPTNetLoLA() is only survived for syntactiaclly correct input.
	// A file given in LoLA format is always a LL net (a native LoLA HL format is
	// still future work)
    }

    // assign indices to place+transition symbols
	
    ParserPTNet::currentsymbols = symbolTables;
    symbolTables -> setIndices();

    // Parsing the net is complete. We have either a PT net or the skeleton of a HL net.

    // Evaluate the cmdline arguments --check, --formula, and --buechi.
    // Values cause substantially different workflow

    pthread_t timer_id;
    switch(RT::args.check_arg)
    {
    case check_arg_deadlockfreedom:
	 // launch timer
        pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
        portfoliomanager::nodetype = PL;
        {
                if(HighLevelNet)
                {
                        symbolTables = unfoldHLNet();
                            ParserPTNet::currentsymbols = symbolTables;
                }
                symbolTables -> setIndices();
                symbolTables -> symboltables2protonet();
		if(RT::args.netreduction_arg != netreduction_arg_off) ProtoNet::currentnet -> reduce(false,true, FORMULA_DEADLOCK);
		Petrinet::InitialNet = ParserPTNet::protonet2net(ProtoNet::currentnet);
                if (RT::args.printNet_given)
                {
                        RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
                        Petrinet::InitialNet->print();
                }
                Petrinet * net = Petrinet::InitialNet;
                net->preprocess0();
                net->preprocess1();
                net->preprocess2();
                net->preprocess3();

		// launch deadlock search
                pthread_t searchthread;
                pthread_create(&searchthread, NULL, DFSExploration::deadlockthread, NULL);

		// launch deadlock findpath
                pthread_t findpaththread;
                pthread_create(&findpaththread, NULL,DFSExploration::findpathdeadlocklivenessthread , NULL);

		// launch deadlock siphon trap
                pthread_t siphontrapthread;
                pthread_create(&siphontrapthread, NULL,SiphonTrapTask::siphontrapthread , NULL);

                // threads do the whole work
                while(true) sleep(10000);
                break;
        }

    case check_arg_OneSafe:
	if(HighLevelNet)
	{
		RT::rep->status(RT::rep->markup(MARKUP_WARNING,"The net may or may not be safe").str());
		_exit(0);
	}
	// launch timer
	portfoliomanager::nodetype = PL;
	{
		symbolTables -> setIndices();
		if(RT::args.netreduction_arg == netreduction_arg_off)
		{
			symbolTables -> setIndices();
			symbolTables->symboltable2net();
		}
		else
		{
			symbolTables -> symboltables2protonet();
			ProtoNet::currentnet -> reduce(false,false,FORMULA_REACHABLE);
			if(ProtoNet::currentnet -> cardPL == 0)
			{
                                RT::rep->status(RT::rep->markup(MARKUP_GOOD,"1 The net is safe").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);

			}
			if(ProtoNet::currentnet -> cardTR == 0)
			{
				ProtoPlace * ppp;
				for(ppp = ProtoNet::currentnet -> firstplace;ppp;  ppp = ppp -> next)
				{
					if(ppp -> marking > 1)
					{
						break;
					}
				}
				if(ppp)
				{
					RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str());
					portfoliomanager::compareresult(false);
					if(RT::args.mcc_given)
					{
						portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
					}
				}
				else
				{
					RT::rep->status(RT::rep->markup(MARKUP_GOOD,"2 The net is safe").str());
					portfoliomanager::compareresult(true);
					if(RT::args.mcc_given)
					{
						portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
					}
				}
				_exit(0);

			}
			Petrinet::InitialNet = ParserPTNet::protonet2net(ProtoNet::currentnet);
		}
		if (RT::args.printNet_given)
		{
			RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
			Petrinet::InitialNet->print();
		}
		Petrinet * net = Petrinet::InitialNet;
		net->preprocess0();

		portfoliomanager::symmetrymap[PL] = new int[net -> Card[PL]];
		portfoliomanager::symmetrymap[TR] = new int[net -> Card[TR]];
		portfoliomanager::globalresult = new ternary_t[net -> Card[PL]];
		portfoliomanager::globalproducer = new globalproducer_t[net -> Card[PL]];

	
		for(arrayindex_t i = 0; i < net -> Card[PL]; i++)
		{
			portfoliomanager::symmetrymap[PL][i] = -1;
			portfoliomanager::globalresult[i] = TERNARY_VOID;
			portfoliomanager::globalproducer[i] = GLOBAL_VOID;
		}
		for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
		{
			portfoliomanager::symmetrymap[TR][i] = -1;
		}
		portfoliomanager::cardtodo = net -> Card[PL];

		for(int i = 0; i < net -> Card[PL]; i++)
		{
			if(net -> Capacity[i] <= 1)
			{
				// safe by definition, e.g. nested unit
				portfoliomanager::globalresult[i] = TERNARY_TRUE;
				portfoliomanager::globalproducer[i] = GLOBAL_PREPROCESSING;
				portfoliomanager::cardtodo--;
			}
		}
		if(!portfoliomanager::cardtodo)
		{
			RT::rep->status("%s", RT::rep->markup(MARKUP_GOOD,"3 The net is safe").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);
			portfoliomanager::globalplacereport();
			_exit(0);
		}
	pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
		net->preprocess1();
		net->preprocess2();
		net->preprocess3();

		// launch 0/1 invariant task
		ComputeBoundExploration * invariantexploration = new ComputeBoundExploration(net);
		pthread_t invariantthread;
		pthread_create(&invariantthread, NULL, invariantexploration->zerooneinvariantthread, NULL);

		// launching reporter
		pthread_t globalrep;
		pthread_create(&globalrep, NULL, portfoliomanager::globalreporter, NULL);

		// launch state equation
		StateEquationTask * sssttt = new StateEquationTask(Petrinet::InitialNet, -1, NULL, 9);
		pthread_t stateequationthread;
		pthread_create(&stateequationthread,NULL, sssttt->safetystateequationthread, NULL);

		// launch symmetry task
		pthread_t symmthread;
		pthread_create(&symmthread, NULL, portfoliomanager::launchsymmetrythread, NULL);
		pthread_t searchthread;
		pthread_create(&searchthread, NULL, invariantexploration->Safety, NULL);

		// launch findpath
		pthread_t findpaththread;
		pthread_create(&findpaththread, NULL, invariantexploration->findpathsafetythread, NULL);
		
		// threads do the whole work
		while(true) sleep(10000);
		break;
	}
    case check_arg_homestates:
	// launch timer
	pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
	portfoliomanager::nodetype = PL;
	{
		if(HighLevelNet)
		{
			symbolTables = unfoldHLNet();
			    ParserPTNet::currentsymbols = symbolTables;
		}
		symbolTables -> setIndices();
		symbolTables->symboltable2net();
		if (RT::args.printNet_given)
		{
			RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
			Petrinet::InitialNet->print();
		}
		Petrinet * net = Petrinet::InitialNet;
		net->preprocess0();
		net->preprocess1();
		net->preprocess2();
		net->preprocess3();
		pthread_t searchthread;
		pthread_create(&searchthread, NULL, TSCCExplorationAGEFRelaxed::GlobalHomestates, NULL);
		
		// threads do the whole work
		while(true) sleep(10000);
		break;
	}
    case check_arg_QuasiLiveness:
	// launch timer
	pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
	portfoliomanager::nodetype = TR;
	{
		Task::havehlnet = false;
		if(HighLevelNet)
		{
			Task::havehlnet = true;
			symbolTables = unfoldHLNet();
			    ParserPTNet::currentsymbols = symbolTables;
			portfoliomanager::hltrue = new int[hltransition::card];
			portfoliomanager::hlunknown = new int[hltransition::card];
			portfoliomanager::hlcardtodo = hltransition::card;
		}
		if(RT::args.netreduction_arg == netreduction_arg_off)
		{
			symbolTables -> setIndices();
			symbolTables->symboltable2net();
		}
		else
		{
			symbolTables -> symboltables2protonet();
			ProtoNet::currentnet -> reduce(false,false,FORMULA_REACHABLE);
			if(ProtoNet::currentnet -> cardTR == 0 || ProtoNet::currentnet->cardPL == 0)
			{
                                RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is quasilive").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);

			}
			Petrinet::InitialNet = ParserPTNet::protonet2net(ProtoNet::currentnet);
		}
		if (RT::args.printNet_given)
		{
			RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
			Petrinet::InitialNet->print();
		}
		Petrinet * net = Petrinet::InitialNet;
		net->preprocess0();

		portfoliomanager::symmetrymap[PL] = new int[net -> Card[PL]];
		portfoliomanager::symmetrymap[TR] = new int[net -> Card[TR]];
		portfoliomanager::globalresult = new ternary_t[net -> Card[TR]];
		portfoliomanager::globalproducer = new globalproducer_t[net -> Card[TR]];

	
		for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
		{
			portfoliomanager::symmetrymap[TR][i] = -1;
			portfoliomanager::globalresult[i] = TERNARY_VOID;
			portfoliomanager::globalproducer[i] = GLOBAL_VOID;
		}
		for(arrayindex_t i = 0; i < net -> Card[PL]; i++)
		{
			portfoliomanager::symmetrymap[PL][i] = -1;
		}
		portfoliomanager::cardtodo = net -> Card[TR];

		// launching reporter
		pthread_t globalrep;
		pthread_create(&globalrep, NULL, portfoliomanager::globalreporter, NULL);

		net->preprocess1();


		StateEquationTask * sssttt = new StateEquationTask(Petrinet::InitialNet, -1, NULL, 9);
		pthread_t stateequationthread;
		pthread_create(&stateequationthread,NULL, sssttt->quasilivenessstateequationthread, NULL);

		net->preprocess2();
		net->preprocess3();

		// launch symmetry task
		pthread_t symmthread;
		pthread_create(&symmthread, NULL, portfoliomanager::launchsymmetrythread, NULL);
		pthread_t searchthread;
		Mara * memory = new Mara();
		DFSExploration * quasilivenessexploration = new DFSExploration(net,memory);
		pthread_create(&searchthread, NULL, quasilivenessexploration->Quasiliveness, NULL);
		// launch findpath
		pthread_t findpaththread;
		pthread_create(&findpaththread, NULL, DFSExploration::findpathquasilivenessthread, NULL);
		while(true) sleep(10000);
		break;
	}
    case check_arg_StableMarking:
	// launch timer
	pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
	portfoliomanager::nodetype = HighLevelNet ? TR : PL;
	portfoliomanager::constantplace = true;
	{
		Task::havehlnet = false;
		if(HighLevelNet)
		{
			Task::havehlnet = true;
			//symbolTables = unfoldHLNet();
			symbolTables = unfoldHLNetStableMarking();
			    ParserPTNet::currentsymbols = symbolTables;
			portfoliomanager::hlcardtodo = hltransition::card;
			portfoliomanager::hltrue = new int[hltransition::card];
			portfoliomanager::hlunknown = new int[hltransition::card];
		}
		if(RT::args.netreduction_arg == netreduction_arg_off)
		{
			symbolTables -> setIndices();
			symbolTables->symboltable2net();
		}
		else
		{
			symbolTables -> symboltables2protonet();
			ProtoNet::currentnet -> reduce(false,false,FORMULA_REACHABLE);
			if(ProtoNet::currentnet->cardPL == 0)
			{
                                RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net does not have a constant place").str());
				portfoliomanager::compareresult(false);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
				}
				_exit(0);

			}
			else if(ProtoNet::currentnet->cardTR == 0)
			{
                                RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net has a constant place").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);

			}
			Petrinet::InitialNet = ParserPTNet::protonet2net(ProtoNet::currentnet);
		}
		if (RT::args.printNet_given)
		{
			RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
			Petrinet::InitialNet->print();
		}
		Petrinet * net = Petrinet::InitialNet;
		net->preprocess0();

		portfoliomanager::symmetrymap[PL] = new int[net -> Card[PL]];
		portfoliomanager::symmetrymap[TR] = new int[net -> Card[TR]];
		if(HighLevelNet)
		{
			// for HL nets, we reduce StableMarking to Quasiliveness, so we need to
			// record results for transitions
			portfoliomanager::globalresult = new ternary_t[net -> Card[TR]];
			portfoliomanager::globalproducer = new globalproducer_t[net -> Card[TR]];
		}
		else
		{
			portfoliomanager::globalresult = new ternary_t[net -> Card[PL]];
			portfoliomanager::globalproducer = new globalproducer_t[net -> Card[PL]];
		}

	
		if(HighLevelNet)
		{
			for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
			{
				portfoliomanager::symmetrymap[TR][i] = -1;
				portfoliomanager::globalresult[i] = TERNARY_VOID;
				portfoliomanager::globalproducer[i] = GLOBAL_VOID;
			}
			for(arrayindex_t i = 0; i < net -> Card[PL]; i++)
			{
				portfoliomanager::symmetrymap[PL][i] = -1;
			}
			portfoliomanager::cardtodo = net -> Card[TR];
		}
		else
		{
			for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
			{
				portfoliomanager::symmetrymap[TR][i] = -1;
			}
			for(arrayindex_t i = 0; i < net -> Card[PL]; i++)
			{
				portfoliomanager::symmetrymap[PL][i] = -1;
				portfoliomanager::globalresult[i] = TERNARY_VOID;
				portfoliomanager::globalproducer[i] = GLOBAL_VOID;
			}
			portfoliomanager::cardtodo = net -> Card[PL];
		}

		// launching reporter
		pthread_t globalrep;
		pthread_create(&globalrep, NULL, portfoliomanager::globalreporter, NULL);

		// launch symmetry task
		pthread_t symmthread;
		net->preprocess1();

		if(HighLevelNet)
		{
			// the actual task just check quasiliveness. Customisation to stable marking
			// just concerns getCandidate and the reporttransition(vector) / reportsymmetry
			// routines
			StateEquationTask * sssttt = new StateEquationTask(Petrinet::InitialNet, -1, NULL, 9);
			pthread_t stateequationthread;
			pthread_create(&stateequationthread,NULL, sssttt->quasilivenessstateequationthread, NULL);

			net->preprocess2();
			net->preprocess3();
		pthread_create(&symmthread, NULL, portfoliomanager::launchsymmetrythread, NULL);
			pthread_t searchthread;
			Mara * memory = new Mara();
			DFSExploration * quasilivenessexploration = new DFSExploration(net,memory);
			pthread_create(&searchthread, NULL, quasilivenessexploration->Quasiliveness, NULL);
			// launch findpath
			pthread_t findpaththread;
			pthread_create(&findpaththread, NULL, DFSExploration::findpathquasilivenessthread, NULL);
		}
		else
		{
			StateEquationTask * sssttt = new StateEquationTask(Petrinet::InitialNet, -1, NULL, 9);
			pthread_t stateequationthread;
			pthread_create(&stateequationthread,NULL, sssttt->constantplacestateequationthread, NULL);
			net->preprocess2();
			net->preprocess3();
			pthread_t searchthread;
		pthread_create(&symmthread, NULL, portfoliomanager::launchsymmetrythread, NULL);
			Mara * memory = new Mara();
			DFSExploration * constantplaceexploration = new DFSExploration(net,memory);
			pthread_create(&searchthread, NULL, constantplaceexploration->Constantplace, NULL);
			// launch findpath
			pthread_t findpaththreadlt;
			pthread_t findpaththreadgt;
			pthread_create(&findpaththreadgt, NULL, DFSExploration::findpathconstantplaceltthread, NULL);
			pthread_create(&findpaththreadlt, NULL, DFSExploration::findpathconstantplacegtthread, NULL);
			}
		
		// threads do the whole work
		while(true) sleep(10000);
		break;
	}
    case check_arg_Liveness:
	// launch timer
	pthread_create(&timer_id, NULL, globalproperty_timer, NULL);
	portfoliomanager::nodetype = TR;
	{
		Task::havehlnet = false;
		if(HighLevelNet)
		{
			Task::havehlnet = true;
			symbolTables = unfoldHLNet();
			    ParserPTNet::currentsymbols = symbolTables;
			portfoliomanager::hlcardtodo = hltransition::card;
			portfoliomanager::hltrue = new int[hltransition::card];
			portfoliomanager::hlunknown = new int[hltransition::card];
		}
		if(RT::args.netreduction_arg == netreduction_arg_off)
		{
			symbolTables -> setIndices();
			symbolTables->symboltable2net();
		}
		else
		{
			symbolTables -> symboltables2protonet();
			ProtoNet::currentnet -> reduce(false,false,FORMULA_REACHABLE);
			if(ProtoNet::currentnet -> cardTR == 0 || ProtoNet::currentnet->cardPL == 0)
			{
                                RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net is live").str());
				portfoliomanager::compareresult(true);
				if(RT::args.mcc_given)
				{
					portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
				}
				_exit(0);

			}
			Petrinet::InitialNet = ParserPTNet::protonet2net(ProtoNet::currentnet);
		}
		if (RT::args.printNet_given)
		{
			RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
			Petrinet::InitialNet->print();
		}
		Petrinet * net = Petrinet::InitialNet;
		net->preprocess0();

		portfoliomanager::symmetrymap[PL] = new int[net -> Card[PL]];
		portfoliomanager::symmetrymap[TR] = new int[net -> Card[TR]];
		portfoliomanager::globalresult = new ternary_t[net -> Card[TR]];
		portfoliomanager::globalproducer = new globalproducer_t[net -> Card[TR]];

	
		for(arrayindex_t i = 0; i < net -> Card[TR]; i++)
		{
			portfoliomanager::symmetrymap[TR][i] = -1;
			portfoliomanager::globalresult[i] = TERNARY_VOID;
			portfoliomanager::globalproducer[i] = GLOBAL_VOID;
		}
		for(arrayindex_t i = 0; i < net -> Card[PL]; i++)
		{
			portfoliomanager::symmetrymap[PL][i] = -1;
		}
		portfoliomanager::cardtodo = net -> Card[TR];

		// launching reporter
		pthread_t globalrep;
		pthread_create(&globalrep, NULL, portfoliomanager::globalreporter, NULL);

		net->preprocess1();

		net->preprocess2();
		net->preprocess3();
		// launch symmetry task
		pthread_t symmthread;
		pthread_create(&symmthread, NULL, portfoliomanager::launchsymmetrythread, NULL);
		pthread_t searchthread;
		pthread_create(&searchthread, NULL, Task::havehlnet ? TSCCExplorationAGEF::HLLiveness : TSCCExplorationAGEF::Liveness, NULL);
		pthread_t livenessthread;
		pthread_create(&livenessthread, NULL, Task::havehlnet ? TSCCExplorationAGEFRelaxed::GlobalHLliveness: TSCCExplorationAGEFRelaxed::Globalliveness, NULL);
		pthread_t dlthread;
		pthread_create(&dlthread, NULL, DFSExploration::deadlockthread, NULL);
		// launch findpath
		pthread_t findpaththread;
		pthread_create(&findpaththread, NULL, DFSExploration::findpathdeadlocklivenessthread, NULL);
		
		// threads do the whole work
		while(true) sleep(10000);
		break;
	}
    case check_arg_none: 
    case check__NULL:
	{
	// if net is HL net: skeleton is obsolete -> unfold to actual PT net
	if(HighLevelNet)
	{
		symbolTables = unfoldHLNet();
	}
	// now, the symbol table structure definitely contains a PT net.
	// Next step is to transform this into the internal representation

        symbolTables->symboltable2net();

        // have initialised enough data structures for outputting the net
        if (RT::args.printNet_given)
    	{
       		RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
        	Petrinet::InitialNet->print();
    	}
        Petrinet::InitialNet->preprocess0();
        Petrinet::InitialNet->preprocess1();


	// prepare for launching the portfolio manager
        RT::data["portfolio"]= JSON();
        RT::data["portfolio"]["task"] = JSON();
        int myid = portfoliomanager::addFormula(NULL,true);
        portfoliomanager::name[myid] = deconst("empty task");
        portfoliomanager::cardvisible = 1;
        EmptyTask::buildTask(Petrinet::InitialNet,-1, NULL, myid);
	portfoliomanager::pn[myid] = Petrinet::InitialNet;
	Petrinet::InitialNet -> preprocessingfinished = true;
	//environment(Petrinet::InitialNet);
        portfoliomanager::run();
        _exit(EXIT_NORMAL);
	}
    case check_arg_full:
	{
	// if net is HL net: skeleton is obsolete -> unfold to actual PT net
	if(HighLevelNet)
	{
		symbolTables = unfoldHLNet();
	}
	// now, the symbol table structure definitely contains a PT net.
	// Next step is to transform this into the internal representation

		symbolTables -> setIndices();
        symbolTables->symboltable2net();

        // have initialised enough data structures for outputting the net
        if (RT::args.printNet_given)
    	{
       		RT::rep->status("Print net (%s)", RT::rep->markup(MARKUP_PARAMETER, "--printNet").str());
        	Petrinet::InitialNet->print();
    	}
        Petrinet::InitialNet->preprocess0();
        Petrinet::InitialNet->preprocess1();
        Petrinet::InitialNet->preprocess2();
        Petrinet::InitialNet->preprocess3();

	// prepare for launching the portfolio manager
        RT::data["portfolio"]= JSON();
        RT::data["portfolio"]["task"] = JSON();
        int myid = portfoliomanager::addFormula(NULL,true);
	portfoliomanager::pn[myid] = Petrinet::InitialNet;
	Petrinet::InitialNet -> preprocessingfinished = true;
        portfoliomanager::name[myid] = deconst("full task");
        portfoliomanager::cardvisible = 1;
        FullTask::buildTask(Petrinet::InitialNet,-1, NULL, myid);
        portfoliomanager::run();
        _exit(EXIT_NORMAL);

	}
    case check_arg_modelchecking:
	{
	// In this mode, LoLA expects a property. The property can be given either as a 
	// Buchi automaton (then the property is inevitably a single LTL formula)
	// or as a (collection of) CTL or LTL formulas. 

	// check that either --formula or --buechi is specified in cmdline

        if ((not RT::args.formula_given) and (not RT::args.buechi_given))
        {
                RT::rep->message("%s given without %s or %s",
			 RT::rep->markup(MARKUP_PARAMETER, "--check=modelchecking").str(),
			 RT::rep->markup(MARKUP_PARAMETER, "--formula").str(),
			 RT::rep->markup(MARKUP_PARAMETER, "--buechi").str());
		RT::rep->abort(ERROR_COMMANDLINE);
        }
        if (RT::args.formula_given and RT::args.buechi_given)
        {
                RT::rep->message("both %s and %s given",
			 RT::rep->markup(MARKUP_PARAMETER, "--formula").str(),
			 RT::rep->markup(MARKUP_PARAMETER, "--buechi").str());
                RT::rep->abort(ERROR_COMMANDLINE);
        }
	if(RT::args.buechi_given)
	{
		// workflow for property given as Büchi automaton

		// Currently, LoLA does not support HL annotation in Büchi automata. Hence,
		// we cannot exploit an a priori generated HL net skeleton
		// and thus transform it immediately into a PT net

		if(HighLevelNet)
		{
			symbolTables = unfoldHLNet();
		}
		// now, the symbol table structure definitely contains a PT net.
	
		// parse the given Buchi automaton

		MagicNumber::init();
		ParserPTNet::currentsymbols = symbolTables;
		LTLTask::ParseBuechi();
		pthread_mutex_lock(&Task::compound_mutex);
		ProtoNet::currentvisibility++;
		symbolTables -> markVisibleBuchiNodes();

		// net reduction: reduction requires the transformation of the net
		// into a "reduction-friendly" structure: the ProtoNet.

		symbolTables -> symboltables2protonet();
		ProtoNet::currentnet -> isSkeleton = false;
		time_t startreduction = time(NULL);
		ProtoNet::currentnet -> markedvisibility = ProtoNet::currentvisibility;
		pthread_mutex_unlock(&Task::compound_mutex);
		ProtoNet::currentnet -> applyEmptySiphon();
		if(RT::args.netreduction_arg != netreduction_arg_off)
		{
			RT::data["globalreduction"] = JSON();
			ProtoNet::currentnet -> data = &RT::data["globalreduction"];
			ProtoNet::currentnet -> reduce(true, false, FORMULA_LTL);
		}

		// transform reduced proto-net into "search friendly" Petrinet data structure

		Petrinet * petri = ParserPTNet::protonet2net(ProtoNet::currentnet);
		petri -> name = deconst("Buchi Automaton");
		petri -> preprocess0();

		// feed and launch portfolio manager
		RT::data["portfolio"]= JSON();
		RT::data["portfolio"]["task"] = JSON();

                int myid = portfoliomanager::addFormula(NULL,true);
                portfoliomanager::name[myid] = deconst("buchi automaton");
                portfoliomanager::cardvisible = 1;
                RT::data["formula"]["buchi automaton"] = JSON();
                petri -> preprocess1();
                petri -> preprocess2();
                petri -> preprocess3();
                petri -> preprocessingfinished = true;
                LTLTask::buildTask(petri,-1, NULL, myid);

		RT::rep->status("launching portfolio manager");
		portfoliomanager::run();

	        _exit(EXIT_NORMAL);
	}
	// Remaining case: property is given as formula
	// net: single (original input) as symboltable; formula: not yet
	MagicNumber::init();
	Task::bignetsemaphore = new mysemaphore(4);
	if(HighLevelNet)
	{
		Task::havehlnet = true;
		// net: SymbolTable Global HighLevel Unreduced    formula: -
			// transform symboltables into protonet 
	
		symbolTables -> setIndices();
			symbolTables -> symboltables2protonet();
			ProtoNet::currentnet->isSkeleton = true;
			symbolTables -> symboltable2net(); // for unfolding
			Petrinet::InitialNet -> preprocess0();
		if (RT::args.xmlformula_given)
		{
			Task::havehlformula = true;
			// We have a HL net and expect a HL formula. That is, we may use the a priori
			// skeleton approach where the skeleton is derived from the HL net and
			// formula structures.

			// We start with checking whether the HL net may  have deadlocks that are not visible in its skeleton
			// We mark the respective transitions

			ValueAutomaton::init();

			RT::rep->status("Reading HL formula in XML format (%s)", RT::rep->markup(MARKUP_PARAMETER, "--xmlformula").str());
			ReadHLPnmlFormula();

			// net: SymbolTable Global HighLevel Unreduced    formula: HighLevel Local but all attached to global HL net symboltable


			

			// net: SymbolTable+ProtoNet+Net Global HighLevel Unreduced formula: HighLevel Local but all attached to global HL net symboltable

			// Since HL net and HL formula are already given, we prepare and launch
			// the skeleton tasks as soon as possible. This way, we may get results
			// even if net unfolding is prohibitively expensive.

			// 	add skeleton tasks

			Task::GlobalHLNet = Petrinet::InitialNet;
			Task::symboltables = symbolTables;
			HLsymbolTables = symbolTables;
			portfoliomanager::cardvisible = 0;
			for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)	
			{
				if(portfoliomanager::visible[i]) portfoliomanager::cardvisible++;
				// run through all top level skeleton formulas
				if(!portfoliomanager::topskeleton[i]) continue;
				portfoliomanager::formula[i]->portfolio_id = i; 
				portfoliomanager::proto[i] = new ProtoNet(*ProtoNet::currentnet);
			// net: SymbolTable+ProtoNet+Net Local HighLevel Unreduced formula: HighLevel Local but all attached to global HL net symboltable
			}
			for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)	
			{
				if(!portfoliomanager::topskeleton[i]) continue;
				pthread_t ttt;
				pthread_create(&ttt, NULL, Task::process_skeleton_subformula_havehlnet,portfoliomanager::formula[i]);
			}
			// 	start portfolio manager &

			pthread_t portfoliothreadid;
			pthread_create(&portfoliothreadid, NULL, portfolio_thread,NULL);

			// unfold net

			LLsymbolTables = unfoldHLNet();
			LLsymbolTables -> setIndices();

			// net: SymbolTable Global LowLevel Unreduced formula: HighLevel Local but all attached to global HL net symboltable

			// empty siphon

			LLsymbolTables -> symboltables2protonet();
			ProtoNet::currentnet -> isSkeleton = false;
		        //ProtoNet::currentnet -> createLP();
			ProtoNet::currentnet ->  detectEmptySiphon(); 
			LLsymbolTables -> symboltable2net();
			Task::GlobalLLNet = Petrinet::InitialNet;
			Task::GlobalLLNet -> preprocess0();
			Task::GlobalLLNet -> preprocess1();

			// unfold formulas

			pthread_mutex_lock(&Task::compound_mutex);
			ProtoNet::currentvisibility++;
			ProtoNet::currentnet -> markedvisibility = ProtoNet::currentvisibility;
			for(int i = 0; i < portfoliomanager::nr_of_formulas;i++)
			{
				if(!portfoliomanager::top[i]) continue;
				tFormula f = portfoliomanager::formula[i];
				pthread_mutex_lock(&kimwitu_mutex);
				f = f->rewrite(kc::tautology);
				f->unparse(myprinter, kc::containstemp);
				f = f->rewrite(kc::emptyquantifiers);
				portfoliomanager::formula[i] = f;
				f->unparse(myprinter,kc::hlunfold); 
				f->unparse(myprinter,kc::checkstableatomic);
				f = f->rewrite(kc::tautology);
				f->unparse(myprinter, kc::containstemp);
				f->unparse(myprinter,kc::temporal);
				f->unparse(myprinter,kc::reduction);
				portfoliomanager::formula[i] = f;
				pthread_mutex_unlock(&kimwitu_mutex);
			}
			Petrinet * TheLLNetForInitial = NULL;
			bool containsDL = false;
			bool containsNX = false;
			for(int i = 0; i < portfoliomanager::nr_of_formulas;i++)
			{
				tFormula f = portfoliomanager::formula[i];
				if(!portfoliomanager::top[i]) continue;

				if(f -> type == FORMULA_INITIAL)
				{
					portfoliomanager::pn[i] = Task::GlobalLLNet;
					portfoliomanager::formula[i] = f;
					f -> portfolio_id = i;
					Task::process_initial(f);
				}
				else
				{
					LLsymbolTables -> markVisibleNodes(portfoliomanager::formula[i]);
				}

				if(portfoliomanager::formula[i]->containsNext) containsNX = true;
				if(portfoliomanager::formula[i]->containsDeadlock) containsDL = true;
			}
			pthread_mutex_unlock(&Task::compound_mutex);
				
			// global net reduction

			ProtoNet::currentnet -> recordVisibleNodes();
			ProtoNet::currentnet -> applyEmptySiphon();
			if(RT::args.netreduction_arg == netreduction_arg_both ||
 			   RT::args.netreduction_arg == netreduction_arg_global)
			{

				RT::data["hlglobalreduction"] = RT::data["globalreduction"];
				RT::data["globalreduction"] = JSON();
				ProtoNet::currentnet -> data = &RT::data["globalreduction"];
				time_t startreduction = time(NULL);
				ProtoNet::currentnet -> reduce(containsNX, containsDL, FORMULA_MODELCHECKING);
			}

			// local reduction & task building

			for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
			{ 
				if(!portfoliomanager::top[i]) continue;
				if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue; // already planned
				portfoliomanager::formula[i]->portfolio_id = i; 
				portfoliomanager::proto[i] = new ProtoNet(*ProtoNet::currentnet);
			}
			for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
			{ 
				if(!portfoliomanager::top[i]) continue;
				if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue; // already planned
				pthread_t ttt;
				pthread_create(&ttt, NULL, Task::process_subformula,portfoliomanager::formula[i]);
			}

			// let portfolio (in its own thread) do its work
			while(true) sleep(10000);
		}
		else
		{
			Task::havellformula = true;
			// We have a HL net and expect a LL net formula (since formula in LoLA format 
			// are always LL). Hence, we unfold the HL net to a LL net and then
			// continue according to the LL net + LL formula approach

			// unfold net

			HLsymbolTables = symbolTables;
			Petrinet * HLNet = Petrinet::InitialNet; 
			symbolTables = unfoldHLNet();

			// continue at "Have LL net and expect LL formula"
		}
	}
	Task::havellnet = true;
	Task::havellformula = true;
	// Here, we have a LL net and expect a LL formula
	// net: SymbolTable Global LowLevel Unreduced formula: -

	// Before parsing the formula, we compute the largest empty siphon of the net.
	// This way, places in the siphon (that will never get any token anyway) can be
	// removed from the formula already upon parsing the formula.
	// For computing the siphon, net is transformed from the symbol table structure into
	// a net reduction friendly format, the ProtoNet.

	symbolTables -> symboltables2protonet();
	ProtoNet::currentnet -> isSkeleton= false;
	symbolTables -> setIndices();
	ProtoNet * TheLLNet = ProtoNet::currentnet;
	//TheLLNet -> createLP(); // prepare for stable atomic analysis
	TheLLNet -> data = new JSON();
        TheLLNet ->  detectEmptySiphon(); 
	// net: SymbolTable+ProtoNet Global LowLevel Unreduced formula: -
	
	// now we parse the actual LL formula. It may be given in XML or in LoLA format.

	Petrinet::InitialNet = ParserPTNet::protonet2net(TheLLNet);
	RT::rep->status("Reading formula.");
	ParserPTNet::currentsymbols = symbolTables;
	if (RT::args.xmlformula_given)
        {
                RT::rep->status("Using XML format (%s)", RT::rep->markup(MARKUP_PARAMETER, "--xmlformula").str());
                ReadPnmlFormula();
        }
        else
        {
                Task::parseFormula();
        }
	//delete_lp((lprec *) (TheLLNet -> lp));
	TheLLNet -> lp = NULL;

	// net: SymbolTable+ProtoNet Global LowLevel Unreduced formula: -

	// launch skeleton folding and add skeleton tasks

	
	//symbolTables -> symboltable2net(); // folding LL to HL net requires a net in final data structure
	//Petrinet::InitialNet -> preprocess0(); 

	portfoliomanager::cardvisible = 0;
	Task::foldllnetsemaphore = new mysemaphore(4);
	ProtoNet * ProtoNetToBeFolded = new ProtoNet(*TheLLNet);
	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)	
	{
		if(portfoliomanager::visible[i]) portfoliomanager::cardvisible++;
		// run through all top level skeleton formulas
		if(!portfoliomanager::topskeleton[i]) continue;
		if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue;
		portfoliomanager::formula[i]->portfolio_id = i; 
		portfoliomanager::proto[i] = ProtoNetToBeFolded;
	}
	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)	
	{
		if(!portfoliomanager::topskeleton[i]) continue;
		if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue;
		pthread_t ttt;
		pthread_create(&ttt, NULL, Task::process_skeleton_subformula_havellnet,portfoliomanager::formula[i]);
	}

	// start portfolio manager

	pthread_t portfoliothreadid;
	pthread_create(&portfoliothreadid, NULL, portfolio_thread,NULL);

	// launch initial tasks
	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
	{ 
		if(!portfoliomanager::top[i]) continue;
		if(portfoliomanager::formula[i]->type != FORMULA_INITIAL) continue;
		portfoliomanager::formula[i]->portfolio_id = i; 
		portfoliomanager::proto[i] = new ProtoNet(*TheLLNet);
	}

	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
	{ 
		if(!portfoliomanager::top[i]) continue;
		if(portfoliomanager::formula[i]->type != FORMULA_INITIAL) continue;
		pthread_t ttt;
		pthread_create(&ttt, NULL, Task::process_subformula,portfoliomanager::formula[i]);
	}

	// global reduction
	if(RT::args.netreduction_arg == netreduction_arg_both ||
	   RT::args.netreduction_arg == netreduction_arg_global)
	{
		time_t startreduction = time(NULL);
		bool ctdeadlock = false;
		bool ctnext = false;
		bool cttemporal = false;
		
		pthread_mutex_lock (& Task::compound_mutex);
		ProtoNet::currentvisibility++;
		TheLLNet -> markedvisibility = ProtoNet::currentvisibility;
		TheLLNet -> applyEmptySiphon();
		for(int i = 0; i < portfoliomanager::nr_of_formulas;i++)
		{
			if(!portfoliomanager::top[i]) continue;
			symbolTables -> markVisibleNodes(portfoliomanager::formula[i]);
			if(portfoliomanager::formula[i]->containsDeadlock) ctdeadlock = true;
			if(portfoliomanager::formula[i]->containsNext) ctnext = true;
			if(portfoliomanager::formula[i]->containsTemporal) cttemporal = true;
		}
		if(cttemporal)
		{
			TheLLNet -> recordVisibleNodes();
			TheLLNet -> reduce(ctnext, ctdeadlock, FORMULA_CTL);
		}
		pthread_mutex_unlock (& Task::compound_mutex);
	}

	// for all formulas:
	//             * local reduction
	//	       * task building
		 
	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
	{ 
		if(!portfoliomanager::top[i]) continue;
		if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue;
		portfoliomanager::formula[i]->portfolio_id = i; 
		portfoliomanager::proto[i] = new ProtoNet(*TheLLNet);
	}

	for(int i = 0; i < portfoliomanager::nr_of_formulas; i++)
	{ 
		if(!portfoliomanager::top[i]) continue;
		if(portfoliomanager::formula[i]->type == FORMULA_INITIAL) continue;
		pthread_t ttt;
		pthread_create(&ttt, NULL, Task::process_subformula,portfoliomanager::formula[i]);
	}

	// let portfolio (in its own thread) do its work
	while(true) sleep(10000);
	}
    default:
	_exit(EXIT_ERROR);
    }
}

void environment(Petrinet * net)
{
	int minin = net -> Card[TR] +1 ;
	int minout = net -> Card[TR] +1 ;
	int minboth = net -> Card[TR] +1 ;
	int avgin = 0;
	int avgout = 0;
	int avgboth = 0;
	int maxin = 0;
	int maxout = 0;
	int maxboth = 0;

	for(int i = 0; i < net -> Card[PL]; i++)
	{
		int in = net -> CardArcs[PL][PRE][i];
		int out = net -> CardArcs[PL][POST][i];
		int both = in + out;
		if(in < minin) minin = in;
		if(in > maxin) maxin = in;
		avgin += in;
		if(out < minout) minout = out;
		if(out > maxout) maxout = out;
		avgout += out;
		if(both < minboth) minboth = both;
		if(both > maxboth) maxboth = both;
		avgboth += both;
	}
	RT::rep->status("ENVIRONMENT NET %s MININ %d MAXIN %d AVGIN %d MINOUT %d MAXOUT %d AVGOUT %d MINBOTH %d MAXBOTH %d AVGBOTH %d", net -> name, minin, maxin, avgin / net -> Card[PL], minout,maxout,avgout / net -> Card[PL],minboth,maxboth,avgboth / net -> Card[PL]);
	
}
