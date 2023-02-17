#include "SATTask.h"

extern kc::tFormula TheFormula;

extern FILE * satreadable_in;
extern FILE * dimacsun_in;
extern int dimacsun_lex();

ternary_t SATTask::satisfiable = TERNARY_UNKNOWN;
arrayindex_t * SATTask::pathArray;

SATTask::SATTask() {
    taskname = "satisfiability";

    if (RT::args.sat_given) {
        steps = RT::args.sat_arg;
    } else {
        // todo: at least throw a warning that default depth is used
    }

    bitsTransitions = (unsigned int) ceil(log(Net::Card[TR])/log(2));
    SATTask::pathArray = (arrayindex_t *) calloc(steps, sizeof(arrayindex_t));
    
    satfile = fopen(SATFILE_PREFIX_PATH, "w");

    initDataStructures();

}

void SATTask::initDataStructures() {

    UnchangedPlaces = (arrayindex_t **) malloc(Net::Card[TR] * sizeof(arrayindex_t *));
    CardUnchangedPlaces = (arrayindex_t *) malloc(Net::Card[TR] * sizeof(arrayindex_t));

    PrePostIntersect = (arrayindex_t **) malloc(Net::Card[TR] * sizeof(arrayindex_t *));
    CardPrePostIntersect = (arrayindex_t *) malloc(Net::Card[TR] * sizeof(arrayindex_t));

    for (arrayindex_t transition = 0; transition < Net::Card[TR]; transition++)
    {
        /*---------------------------------------------*/
        /*                                             */
        /*  Intersecting Set of Pre- and Postplaces    */
        /*                                             */
        /*---------------------------------------------*/
        // intersection of pre- and postplaces can have at most min(Pre, Post) elements
        arrayindex_t MaxCardPrePost = (Net::CardArcs[TR][PRE][transition] < Net::CardArcs[TR][POST][transition]) ? Net::CardArcs[TR][PRE][transition] : Net::CardArcs[TR][POST][transition];
        arrayindex_t * PrePost = new arrayindex_t[MaxCardPrePost];
        
        PrePostIntersect[transition] = PrePost;
        
        arrayindex_t prepost_index = 0;

        for (arrayindex_t arcPre = 0; arcPre < Net::CardArcs[TR][PRE][transition]; arcPre++) {
            for (arrayindex_t arcPost = 0; arcPost < Net::CardArcs[TR][POST][transition]; arcPost++) {
                arrayindex_t pre = Net::Arc[TR][PRE][transition][arcPre];
                arrayindex_t post = Net::Arc[TR][POST][transition][arcPost];
                if (pre == post) {
                    PrePost[prepost_index] = pre;
                    prepost_index++;
                    break;
                }
            }
        }
        CardPrePostIntersect[transition] = prepost_index;

        /*---------------------------------------------*/
        /*                                             */
        /*          Set of unchanged places            */
        /*                                             */
        /*---------------------------------------------*/
        // no. of unchanged places = no. of total places - (no of places with pos. delta + no of places with neg. delta)
        arrayindex_t CardUnchanged = Net::Card[PL] - (Transition::CardDeltaT[PRE][transition] + Transition::CardDeltaT[POST][transition]);
        arrayindex_t *Unchanged = new arrayindex_t[CardUnchanged];

        UnchangedPlaces[transition] = Unchanged;
        CardUnchangedPlaces[transition] = CardUnchanged;
        arrayindex_t unchanged_index = 0;

        // for each transition T find all places that are not connected to T
        for (arrayindex_t place = 0; place < Net::Card[PL]; place++)
        {
            bool found = false;
            for (arrayindex_t pre = 0; pre < Net::CardArcs[TR][PRE][transition]; pre++)
            {
                arrayindex_t actual = Transition::DeltaT[PRE][transition][pre];
                found = (actual == place);
                if (found) break;
            }
            
            if (!found) {
                for (arrayindex_t post = 0; post < Transition::CardDeltaT[POST][transition]; post++)
                {
                    arrayindex_t actual = Transition::DeltaT[POST][transition][post];
                    found = (actual == place);
                    if (found) break;
                }
            }
            if (!found)
            {
                Unchanged[unchanged_index] = place;
                unchanged_index++;
            }
        } 
    }
}

ternary_t SATTask::getResult()
{
    // encode initial marking
    encodeInitialMarking();

    // encode goal
    encodeGoal();

    // chain step formula 
    for (step = 1; (step <= steps) && (satisfiable != TERNARY_TRUE); step++) {
        // opening parenthesis of step + &&
        SAT_PRINT(" && (");
        // encode transitions
        for (arrayindex_t transition = 0; transition < Net::Card[TR]; transition++) {
            SAT_COMMENT("\n// transition: %u\t\t(step %u)\n", transition, step);
            // opening parenthesis of transition
            SAT_PRINT("(");

            
            // encode index of transition, if witness path is relevant
            if (bitsTransitions != 0 && WitnessPathRequired) {
                encodeFireTransition(transition);
            } else {
                SAT_PRINT("T");
            }

            // encode effects

            //      subtractions
            for (arrayindex_t p = 0; p < Transition::CardDeltaT[PRE][transition]; p++) {
                arrayindex_t place = Transition::DeltaT[PRE][transition][p];
                mult_t multipl = Transition::MultDeltaT[PRE][transition][p];
                encodeSubtraction(place, multipl);
            }
            //      additions
            for (arrayindex_t p = 0; p < Transition::CardDeltaT[POST][transition]; p++) {
                arrayindex_t place = Transition::DeltaT[POST][transition][p];
                mult_t multipl = Transition::MultDeltaT[POST][transition][p];
                encodeAddition(place, multipl);
            }
            //      keep
            SAT_COMMENT("\n// keep other places the same\n");
            for (arrayindex_t p = 0; p < CardUnchangedPlaces[transition]; p++) {
                arrayindex_t place = UnchangedPlaces[transition][p];
                encodeKeep(place);
            }
            SAT_COMMENT("\n");

            // encode activation where needed
            for (arrayindex_t p = 0; p < CardPrePostIntersect[transition]; p++) {
                arrayindex_t place = PrePostIntersect[transition][p];
                SAT_PRINT(" && ");
                encodeGreaterThanConst("s" + std::to_string(step) + "p" + std::to_string(place) + "b%u",
                                        Place::CardBits[place],
                                        Net::Arc[TR][PRE][transition][place]-1);
            }

            // close parenthesis of this transition, append || if needed
            SAT_PRINT((transition != Net::Card[TR]-1) ? ") || " : ")");
        }

        // closing parenthesis of step
        SAT_PRINT(")");

        // copy prefix formula
        fclose(satfile);
        system("cp " SATFILE_PREFIX_PATH " " SATFILE_PATH);

        // rename goal variables in copy
        satfile = fopen(SATFILE_PATH, "a");
        renameGoalVariables();
        fclose(satfile);

        // check satisfiability
        checkSatisfiability();

        // return to prefix file
        satfile = fopen(SATFILE_PREFIX_PATH, "a");
    }

    // close formula file (and delete?)
    fclose(satfile);
    //system("rm " SATFILE_PATH " " SATFILE_PREFIX_PATH);

    // set return value
    // todo: check if this behaviour is consistent with behaviour of other tasks
    // if (satisfiable == TERNARY_UNKNOWN) satisfiable = TERNARY_FALSE;

    return satisfiable;

}


void SATTask::encodeInitialMarking() {
    SAT_COMMENT("\n// initial marking: s_0\n");
    SAT_PRINT("(");
    // introduce variables of step 0 so that they encode the initial marking
    for (arrayindex_t place = 0; place < Net::Card[PL]; place++) {
        capacity_t marks = Marking::Initial[place];
        SAT_COMMENT("\n// place: %u, %u marks\n", place, marks);
        // for each bit of the place introduce a variable
        for (unsigned int bit = 0; bit < Place::CardBits[place]; bit++) {
            // if the bit is 0 in the initial marking, negate the variable
            if ((marks >> bit) % 2 == 0) SAT_PRINT("! ");
            SAT_PRINT("s0p%ub%u", place, bit);
            if (bit != Place::CardBits[place]-1) SAT_PRINT(" && ");
        }
        if (place != Net::Card[PL]-1) SAT_PRINT(" && ");
    }
    SAT_PRINT(")");
}

void SATTask::encodeAddition(arrayindex_t place, mult_t multipl) {
    SAT_COMMENT("\n// adding %i to postplace: %u (%s)\n", multipl, place, Net::Name[PL][place]);
    std::string fPrefix = "s" + std::to_string(step-1) + "p" + std::to_string(place);

    std::string fCarryBit = fPrefix + "c%u";
    std::string fCurrentStep = fPrefix + "b%u";
    std::string fNextStep = "s" + std::to_string(step) + "p" + std::to_string(place) + "b%u";

    // first bit is a half adder
    SAT_PRINT((" && (" + fNextStep + " <-> (" + fCurrentStep + " ^ %c))").c_str(),
                        0,                      0,           NTH_BIT(multipl, 0));
    SAT_PRINT((" && (" + fCarryBit + " <-> (" + fCurrentStep + " && %c))").c_str(),
                        1,                      0,           NTH_BIT(multipl, 0));

    // following bits are full adders
    for (unsigned int bit = 1; bit < Place::CardBits[place]; bit++) {
        char ithBit = NTH_BIT(multipl, bit);
        // Bit:         a' <-> (a ^ b ^ c)
        SAT_PRINT((" && (" + fNextStep + " <-> (" + fCurrentStep + " ^ %c ^ " + fCarryBit + "))").c_str(),
                                bit,                bit,     ithBit,        bit);
        // Carry out:   c' <-> ((c && (a ^ b)) || (a && b))
        SAT_PRINT((" && (" + fCarryBit + " <-> ((" + fCarryBit + " && (" + fCurrentStep + " ^ %c)) || (" + fCurrentStep + " && %c)))").c_str(),
                                bit+1,              bit,                bit,       ithBit,          bit,        ithBit);
        SAT_COMMENT("\n");
    }
    // last carry out == True iff overflow occured
    SAT_PRINT((" && (! " + fCarryBit + ")").c_str(), Place::CardBits[place]);
    SAT_COMMENT("\n");
}

void SATTask::encodeSubtraction(arrayindex_t place, mult_t multipl) {
    SAT_COMMENT("\n// subtracting %i to postplace: %u (%s)\n", multipl, place, Net::Name[PL][place]);
    std::string fPrefix = "s" + std::to_string(step-1) + "p" + std::to_string(place);

    std::string fCarryBit = fPrefix + "c%u";
    std::string fCurrentStep = fPrefix + "b%u";
    std::string fNextStep = "s" + std::to_string(step) + "p" + std::to_string(place) + "b%u";

    // first bit is a half subtractor
    SAT_PRINT((" && (" + fNextStep + " <-> (" + fCurrentStep + " ^ %c))").c_str(),
                        0,                      0,           NTH_BIT(multipl, 0));
    SAT_PRINT((" && (" + fCarryBit + " <-> (! " + fCurrentStep + " && %c))").c_str(),
                        1,                      0,           NTH_BIT(multipl, 0));

    // following bits are full subtractors
    for (unsigned int bit = 1; bit < Place::CardBits[place]; bit++) {
        char ithBit = NTH_BIT(multipl, bit);
        // Bit:         a' <-> (a ^ b ^ c)
        SAT_PRINT((" && (" + fNextStep + " <-> (" + fCurrentStep + " ^ %c ^ " + fCarryBit + "))").c_str(),
                                bit,                bit,     ithBit,        bit);
        // Carry out:   c' <-> ((c && (a ^ b)) || (a && b))
        SAT_PRINT((" && (" + fCarryBit + " <-> ((! " + fCurrentStep + " && " + fCarryBit + ") " + "|| (! " + fCurrentStep + " && %c) || (%c && " + fCarryBit + ")))").c_str(),
                                bit+1,                      bit,                  bit,                          bit,            ithBit, ithBit,        bit); 
        SAT_COMMENT("\n");
    }
    // last carry out == True iff underflow occured
    SAT_PRINT((" && (! " + fCarryBit + ")").c_str(), Place::CardBits[place]);
    SAT_COMMENT("\n");
}

void SATTask::encodeActivation(std::string stepPrefix, arrayindex_t transition) {
    SAT_COMMENT("\n// activation of transition #%u\n", transition);
    SAT_PRINT("(");
    if (Net::CardArcs[TR][PRE][transition] == 0) SAT_PRINT("T");
    for (arrayindex_t p = 0; p < Net::CardArcs[TR][PRE][transition]; p++) 
    {
        arrayindex_t preplace = Net::Arc[TR][PRE][transition][p];
        const char * name = Net::Name[PL][preplace];
        std::string formatPreplace = stepPrefix + "p" + std::to_string(preplace) + "b%u";
        encodeGreaterThanConst(formatPreplace,Place::CardBits[preplace]-1, Net::Mult[TR][PRE][transition][p]-1);
        if (p != Net::CardArcs[TR][PRE][transition]-1) SAT_PRINT(" && ");
    }
    
    SAT_PRINT(")");
}

void SATTask::encodeFireTransition(arrayindex_t transition) {
    SAT_PRINT("(");
    if (bitsTransitions == 0) SAT_PRINT("T"); // probably redundant since this check should decide if the method is called at all
    for (unsigned int bit = 0; bit < bitsTransitions; bit++)
    {
        if ((transition >> bit) % 2 == 0) SAT_PRINT("! ");
        SAT_PRINT(FORMAT_TRANSITION_BIT, step, bit);
        if (bit != bitsTransitions-1) SAT_PRINT(" && ");
    }
    SAT_PRINT(")");
}

void SATTask::encodeKeep(arrayindex_t place) {
    for (unsigned int bit = 0; bit < Place::CardBits[place]; bit++) {
        SAT_PRINT(" && (s%up%ub%u <-> s%up%ub%u)", step, place, bit, step-1, place, bit);
    }
}

void SATTask::renameGoalVariables() {

    // for each place that is relevant to a goal, rename all bits from s_LASTSTEP_p_X_b_Y to s_goal_p_X_b_Y
    for (std::set<arrayindex_t>::iterator it = relevantPlaces.begin(); it != relevantPlaces.end(); ++it) {
        arrayindex_t place = *it;
        for (unsigned int bit = 0; bit < Place::CardBits[place]; bit++) {
            SAT_PRINT(" && (sGOALp%ub%u <-> s%up%ub%u)",
                place, bit, step, place, bit);
        }
    }
}

void SATTask::encodeGreaterThanConst(std::string formatFirst, int maxBitFirst, int constant) {
    SAT_PRINT("(");
    for (int i = 0; i <= maxBitFirst; i++) {
        SAT_PRINT(("(" + formatFirst + " && ! %c").c_str(), i, NTH_BIT(constant, i));
        for (int k = i+1; k <= maxBitFirst; k++) {
            SAT_PRINT((" && (" + formatFirst + " <-> %c)").c_str(), k, NTH_BIT(constant, i));
        }
        SAT_PRINT(")");
        if (i != maxBitFirst) SAT_PRINT(" || ");
    }
    SAT_PRINT(")");
}

void SATTask::encodeDeadlock() {
        for (arrayindex_t t = 0; t < Net::Card[TR]; t++) {
            SAT_PRINT(" && ! ");
            encodeActivation("sGOAL", t);
            SAT_COMMENT("\n");
        }
}

void SATTask::encodeReachability() {
    // todo or not to do...
}

void SATTask::encodeGoal() {

    SAT_COMMENT("\n// encoding goal\n");
    assert(TheFormula);

    unparsed.clear();
    TheFormula->unparse(myprinter, kc::internal);

    collectRelevantPlaces();

    switch (TheFormula->type) {
        case FORMULA_DEADLOCK:
        encodeDeadlock();
        break;
        case FORMULA_REACHABLE:
        encodeReachability();
        break;
    }
}

void SATTask::checkSatisfiability() {

    // symbol table has to be reset to ensure correct transformation.
    // todo: possible improvement: figure out how to exploit that the first part of the formula has already been translated in the last step
    SATSymbolTable::reset();

    // open files and generate DIMACS-CNF-File
    satreadable_in = fopen(SATFILE_PATH, "r");
    SATSymbolTable::dimacsFile = fopen(DIMACS_FILE, "w");
    satreadable_parse();
    SATSymbolTable::complete();

    fclose(SATSymbolTable::dimacsFile);
    fclose(satreadable_in);

    std::string minisat_cmd = std::string("minisat ") + DIMACS_FILE + " " + SOLUTION_FILE + " > " MINISAT_LOG;

    minisat_start = time(0);
    system(minisat_cmd.c_str());
    
    dimacsun_in = fopen(SOLUTION_FILE, "r");
    SATSymbolTable::readableSatFile = fopen(READABLE_SOLUTION_FILE, "w");
    dimacsun_lex();
    fprintf(SATSymbolTable::readableSatFile, "\n\nSkipped %u aux. variables", SATSymbolTable::skipped_vars);
    fclose(SATSymbolTable::readableSatFile);
    fclose(dimacsun_in);
}

void SATTask::collectRelevantPlaces() {
    if (TheFormula->type == FORMULA_REACHABLE) {
        AtomicStatePredicate * predicate = (AtomicStatePredicate *) TheFormula->formula;
        for (arrayindex_t n = 0; n < predicate->cardNeg; n++) relevantPlaces.insert(predicate->negPlaces[n]);
        for (arrayindex_t p = 0; p < predicate->cardPos; p++) relevantPlaces.insert(predicate->posPlaces[p]);
    } else if (TheFormula->type == FORMULA_DEADLOCK) {
        for (arrayindex_t s = 0; s < Net::Card[PL]; s++) relevantPlaces.insert(s);
    }
}

void SATTask::interpreteResult(ternary_t result) {

    switch (result)
    {
    case TERNARY_TRUE:
        RT::rep->status("result: %s", RT::rep->markup(MARKUP_GOOD, "yes").str());
        RT::rep->status("produced by: %s", taskname);
        RT::data["result"]["value"] = true;
        RT::data["result"]["produced_by"] = std::string(taskname);

            RT::rep->status("%s", RT::rep->markup(MARKUP_GOOD, ("The predicate is reachable in " + std::to_string(step-1) + " steps.").c_str()).str());

        break;

    case TERNARY_FALSE:
        RT::rep->status("result: %s", RT::rep->markup(MARKUP_BAD, "no").str());
        RT::rep->status("produced by: %s", taskname);
        RT::data["result"]["value"] = false;
        RT::data["result"]["produced_by"] = std::string(taskname);

            RT::rep->status("%s", RT::rep->markup(MARKUP_BAD,
                                                  "The predicate is unreachable.").str());
        break;

    case TERNARY_UNKNOWN:
        RT::rep->status("result: %s", RT::rep->markup(MARKUP_WARNING, "unknown").str());
        RT::rep->status("produced by: %s", taskname);
        RT::data["result"]["value"] = JSON::null;
        RT::data["result"]["produced_by"] = std::string(taskname);

            RT::rep->status("%s", RT::rep->markup(MARKUP_WARNING,
                                                  "The predicate may or may not be reachable.").str());
        break;
    }
}

Path SATTask::getWitnessPath() {
    // construct witness path from array
    Path path = Path();
    path.initialized = true;
    for (unsigned int i = 0; i < step-1; i++) {
        path.addTransition(SATTask::pathArray[i]);
    }
    return path;
}

capacity_t *SATTask::getMarking() {
    RT::rep->message("SATTask::getMarking not implemented yet. What would this even do?");
}

void SATTask::getStatistics() {
    RT::rep->message("SATTask::getStatistics not properly implemented, showing minisat stats for good measure");
    // todo: maybe print some more interesting minisat stats
    system("cat " MINISAT_LOG);
}

char * SATTask::getStatus(uint64_t elapsed) {
    time_t current_time = time(0);
    time_t minisat_time = current_time - minisat_start;

    char * result = (char *) malloc(128);
    sprintf(result, "Checking step %2u/%u, MiniSat running for %lis", step, steps, minisat_time);
    return result;
}

int SATTask::encodeMultiplication(arrayindex_t place, capacity_t weight, std::string prefixResult) {

    assert(weight > 0);
    
    std::string formatResult = prefixResult + "b%u";

    std::string formatVariable = "sGOALp" + std::to_string(place) + "b%u";
    capacity_t bitsVariable = Place::CardBits[place];

    SAT_COMMENT(("\n// encoding place p_%i * %i = " + formatResult + "\n").c_str(), place, weight, 0, 0);

    int numberOf1s = 0;
    for (int factorCount = weight; factorCount > 0; factorCount = factorCount >> 1) {
        if (factorCount % 2 == 1) numberOf1s++;
    }

    // index of most significant 1
    unsigned int iMS1 = (unsigned int) floor(log2(weight));
    int layer = numberOf1s - 1;

    SAT_COMMENT("\n// first shift by %u => z%i\n", iMS1, numberOf1s-1);
    for (unsigned int i = 0; i < iMS1; i++) {
        SAT_PRINT((" && (" + formatResult + " <-> F)").c_str(), numberOf1s-1, i);
    }
    for (unsigned int i = 0; i < bitsVariable; i++) {
        SAT_PRINT((" && (" + formatResult + " <-> " + formatVariable + ")").c_str(), numberOf1s-1, iMS1 + i, i);
    }
    layer--;
    
    capacity_t bitsPrevious = bitsVariable + iMS1;
    int factor = weight - (int) pow(2, iMS1);
    while (factor > 0) {
        iMS1 = (unsigned int) floor(log2(factor));
        SAT_COMMENT("\n// add shift by %u\n", iMS1);
        SAT_PRINT(" && ");
        encodeAddShift(layer, formatVariable, bitsVariable, bitsPrevious, iMS1, prefixResult);
        bitsPrevious++;
        layer--;
        factor = factor - (int) pow(2, iMS1);
        SAT_COMMENT("\n");
    }

    return bitsPrevious;
}

void SATTask::encodeAddShift(int layer, std::string formatVariable, capacity_t bitsVariable, capacity_t bitsPrevious, unsigned int shiftBy, std::string prefixResult) {
    std::string formatResult = prefixResult + "b%u";
    std::string formatCarry = prefixResult + "c%u";
    
    SAT_PRINT("(");
    SAT_COMMENT("\n// first bit\n");
    if (shiftBy == 0) {
        // no shift -> actual addition
        SAT_PRINT(("\t(" + formatResult +      " <-> " + formatVariable +      " ^ " + formatResult + ")\n").c_str(),
                                layer, 0,                   0,                      layer+1, 0+shiftBy);
        SAT_PRINT(("\t\t&& (" + formatCarry +  " <-> (" + formatVariable +     " && " + formatResult + "))\n").c_str(),
                                layer, 1,                   0,                      layer+1, 0+shiftBy);
    } else {
        // shift by at least 1 => LSB same as previous layer, no carry out (but set anyways just to be safe)
        SAT_PRINT(("\t(" + formatResult +      " <-> " + formatResult + ")\n").c_str(),
                            layer, 0,                  layer+1, 0);
        SAT_PRINT(("\t\t&& (! " + formatCarry + ")\n").c_str(), layer, 1);
    }
    SAT_COMMENT("\n// bits until shiftBy\n");
    for (unsigned int i = 1; i < shiftBy; i++) {
        // no carry in possible, all bits same as first, no carry out
        SAT_PRINT(("\t&& (" +      formatResult + " <-> " +    formatResult + ")\n").c_str(),
                                layer, i,                   layer+1, i);
        SAT_PRINT(("\t\t&& (! " +  formatCarry + ")\n").c_str(),
                                layer, i+1);
    }
    SAT_COMMENT("\n// bits between shiftBy and shiftBy + BitsVariable\n");
    for (unsigned int i = shiftBy; i < shiftBy + bitsVariable; i++) {
        // the interesting part where there is actual adding
        SAT_PRINT(("\t&& (" +      formatResult +  " <-> (" +  formatResult +  " ^ " +     formatVariable + " ^ " +   formatCarry + "))\n").c_str(),
                                layer, i,                   layer+1, i,                 i-shiftBy,                 layer, i);
        SAT_PRINT(("\t\t&& (" +    formatCarry +   " <-> ((" + formatCarry +   " && (" +   formatVariable + " ^ " +   formatResult 
            + ")) || (" +   formatVariable + " && " +   formatResult + ")))\n").c_str(),
                                layer, i+1,                 layer, i,                   i-shiftBy,                 layer+1,i,
                            i-shiftBy,                  layer+1, i);
    }   
    for (unsigned int i = shiftBy + bitsVariable; i < bitsPrevious; i++) {
        // first addend is finished, only second addend + carry out remains
        SAT_PRINT(("\t&& (" +      formatResult +  " <-> (" +  formatResult +  " ^ " +     formatCarry + "))\n").c_str(),
                                layer, i,                   layer+1, i,                 layer, i);
        SAT_PRINT(("\t\t&& (" +    formatCarry +   " <-> (" +  formatResult +   " && " +   formatCarry + "))\n").c_str(),
                                layer, i+1,                 layer+1, i,                 layer, i);
    }
    // extra bit for last carry out
    SAT_PRINT(("\t&& (" +  formatResult + " <-> " +    formatCarry + ")\n").c_str(),
                        layer, bitsPrevious,        layer, bitsPrevious);
    SAT_PRINT(")");
}

void SATTask::encodeVariableAddition(std::string formatFirst, capacity_t bitsFirst, std::string formatSecond, capacity_t bitsSecond, std::string prefixResult) {

    std::string fr = prefixResult + "b%i";
    std::string fc = prefixResult + "c%i";
    std::string ff = formatFirst;
    std::string fs = formatSecond;

    // first bit: half adder
    SAT_PRINT(("(" + fr + " <-> " + ff + " ^ " + fs + ")").c_str(),
        0, 0, 0);

    SAT_PRINT(("(" + fc + " <-> " + ff + " && " + fs + ")").c_str(),
        1, 0, 0);

    // following bits: full adders

    capacity_t bitsMin = (bitsFirst < bitsSecond) ? bitsFirst : bitsSecond;
    capacity_t bitsMax = (bitsFirst < bitsSecond) ? bitsSecond : bitsFirst;
    
    for (unsigned int i = 1; i < bitsMin; i++) {
        SAT_PRINT((" && (" + fr + " <-> " + ff + " ^ " + fs + " ^ " + fc + ")").c_str(),
            i, i, i, i);

        SAT_PRINT((" && (" + fc + " <-> (" + fc + " && (" + ff + " ^ " + fs + ") || (" + ff + " && " + fs + ")").c_str(),
            i+1, i, i, i, i, i);
    }

    // if one addend has more bits than the other, only the carry out is added 
    if (bitsFirst > bitsSecond) {
        for (unsigned int i = bitsMin; i < bitsFirst; i++) {
            SAT_PRINT((" && (" + fr + " <-> " + ff + " ^ " + fc + ")").c_str(),
                i, i, i);

            SAT_PRINT(("(" + fc + " <-> " + ff + " && " + fc + ")").c_str(),
                i+1, i, i);
        }
    } else {
        for (unsigned int i = bitsMin; i < bitsSecond; i++) {
            SAT_PRINT((" && (" + fr + " <-> " + fs + " ^ " + fc + ")").c_str(),
                i, i, i);

            SAT_PRINT(("(" + fc + " <-> " + fs + " && " + fc + ")").c_str(),
                i+1, i, i);
        }
    }
    
    // last carry out = MSB of result
    SAT_PRINT((" && (" + fr + " <-> " + fc + ")").c_str(),
        bitsMax, bitsMax);
}

void SATTask::encodeVariableSubtraction(std::string formatFirst, capacity_t bitsFirst, std::string formatSecond, capacity_t bitsSecond, std::string prefixResult) {
    std::string fr = prefixResult + "b%i";
    std::string fc = prefixResult + "c%i";
    std::string ff = formatFirst;
    std::string fs = formatSecond;

    // first bit: half adder
    SAT_PRINT((" && (" + fr + " <-> " + ff + " ^ " + fs + ")").c_str(),
        0, 0, 0);

    SAT_PRINT((" && (" + fc + " <-> ! " + ff + " && " + fs + ")").c_str(),
        1, 0, 0);

    // following bits: full adders
    capacity_t bitsMin = (bitsFirst < bitsSecond) ? bitsFirst : bitsSecond;
    capacity_t bitsMax = (bitsFirst < bitsSecond) ? bitsSecond : bitsFirst;

    // looks like a mess, 
    for (unsigned int i = 1; i < bitsMin; i++) {

        SAT_PRINT((" && (" + fr + " <-> " + ff + " ^ " + fs + " ^ " + fc + ")").c_str(),
            i, i, i, i);

        SAT_PRINT((" && (" + fc + " <-> (! " + ff + " && " + fc + ") || (! " + ff + " && " + fs + ") || (" + fs + " && " + fc + "))").c_str(),
                            i+1,                i,           i,                i,            i,              i,            i);
    }

    // if one addend has more bits than the other, only the carry out is added 
    if (bitsFirst > bitsSecond) {
        for (unsigned int i = bitsMin; i < bitsFirst; i++) {
            SAT_PRINT((" && (" + fr + " <-> " + ff + " ^ " + fc + ")").c_str(),
                i, i, i);

            SAT_PRINT(("(" + fc + " <-> !" + ff + " && " + fc + ")").c_str(),
                i+1, i, i);
        }
    } else {
        for (unsigned int i = bitsMin; i < bitsSecond; i++) {
            SAT_PRINT((" && (" + fr + " <-> " + fs + " ^ " + fc + ")").c_str(),
                i, i, i);

            SAT_PRINT(("(" + fc + " <-> " + fs + " || " + fc + ")").c_str(),
                i+1, i, i);
        }
    }

    // last carry out = MSB of result
    SAT_PRINT((" && (" + fr + " <-> " + fc + ")").c_str(),
        bitsMax, bitsMax);
    
}