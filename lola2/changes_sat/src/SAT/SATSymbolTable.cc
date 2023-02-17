#include <SAT/SATSymbolTable.h>

std::map<std::string, unsigned int> SATSymbolTable::name_to_id;
std::map<unsigned int, std::string> SATSymbolTable::id_to_name;
unsigned int SATSymbolTable::next_id = 1;

unsigned int SATSymbolTable::number_clauses = 0;
unsigned int SATSymbolTable::skipped_vars = 0;

unsigned int SATSymbolTable::last_id = 0;

FILE * SATSymbolTable::dimacsFile = 0;
FILE * SATSymbolTable::readableSatFile = 0;

std::map<std::pair<int, int>, unsigned int> SATSymbolTable::parsed_ids[5];

void SATSymbolTable::reset() {
    name_to_id.clear();
    id_to_name.clear();
    next_id = 1;
    number_clauses = 0;
    skipped_vars = 0;
    last_id = 0;

    for (int i = 0; i < 5; i++) {
        parsed_ids[i].clear();
    }
}

void SATSymbolTable::complete() {
    if(last_id == std::numeric_limits<int>::min()) {
        fprintf(dimacsFile, "-%u 0\n", next_id-1);
        number_clauses++;
    }
    fprintf(dimacsFile, "%u 0\n", next_id-1);
    number_clauses++;
    
    fprintf(dimacsFile, "p cnf %u %u", next_id-1, number_clauses);
}

void SATSymbolTable::unparse_var(int id) {
    unsigned int id_abs = abs(id);

    auto search = id_to_name.find(id_abs);
    
    if (search != id_to_name.end()) {
        std::string varname = search->second;
        fprintf(readableSatFile, "%s = %s\n", search->second.c_str(), (id < 0) ? "false" : "true");
        // if parsed variable encodes firing transition, add to witness path array
        std::string prefix = "s_";
        unsigned int bit;
        unsigned int step;
        if (sscanf(varname.c_str(), FORMAT_TRANSITION_BIT, &step, &bit) == 2) {
            if (id > 0) {
                arrayindex_t val = (arrayindex_t) pow(2, bit);
                SATTask::pathArray[step-1] += val;
                }
            }

    } else skipped_vars++;
}

void SATSymbolTable::unparse_sat(int is_satisfiable) {
    if (is_satisfiable) {
        fprintf(readableSatFile, "SATISFIABLE\n");
        SATTask::satisfiable = TERNARY_TRUE;
    } else {
        fprintf(readableSatFile, "UNSATISFIABLE\n");
        SATTask::satisfiable = TERNARY_FALSE;
    }
}

unsigned int SATSymbolTable::parse_clause(clause_t type, int left, int right) {
    last_id = _parse_clause(type, left, right);
    return last_id;
}

unsigned int SATSymbolTable::_parse_clause(clause_t type, int left, int right) {

    if (left == std::numeric_limits<int>::max()) {
        switch (type) {
            // not T = F
            case SAT_NOT: return std::numeric_limits<int>::min();
            // T and X = X
            // T <-> X = X
            case SAT_AND: return right;
            case SAT_EQUALS: return right;
            // T or X = T
            case SAT_OR: return std::numeric_limits<int>::max();
            // T xor X = not X
            case SAT_XOR: return parse_clause(SAT_NOT, right, -1);
        }
    } else if (left == std::numeric_limits<int>::min()) {
        switch (type) {
            // not F = T
            case SAT_NOT: return std::numeric_limits<int>::max();
            // F and X = F
            case SAT_AND: return std::numeric_limits<int>::min();
            // F <-> X = not X
            case SAT_EQUALS: return parse_clause(SAT_NOT, right, -1);
            // F or X = X
            // F xor X = X
            case SAT_OR: return right;
            case SAT_XOR: return right;
        }
    }

    if (right == std::numeric_limits<int>::max()) {
        switch (type) {
            // T and X = X
            // T <-> X = X
            case SAT_AND: return left;
            case SAT_EQUALS: return left;
            // T or X = T
            case SAT_OR: return std::numeric_limits<int>::max();
            // T xor X = not X
            case SAT_XOR: return parse_clause(SAT_NOT, left, -1);
        }
    } else if (right == std::numeric_limits<int>::min()) {
        switch (type) {
            // F and X = F
            case SAT_AND: return std::numeric_limits<int>::min();
            // F <-> X = not X
            case SAT_EQUALS: return parse_clause(SAT_NOT, left, -1);
            // F or X = X
            // F xor X = X
            case SAT_OR: return left;
            case SAT_XOR: return left;
        }
    }

    // lookup pair to see if it already has an id
    std::pair<int, int> ppair((left > right) ? right : left, (left > right) ? left : right);
    auto search = parsed_ids[type].find(ppair);

    if (search != parsed_ids[type].end()) {
        return search->second;
    }

    unsigned int clause_id = next_id;
    next_id++;

    parsed_ids[type].insert(std::pair<std::pair<int, int>, unsigned int>(ppair, clause_id));

    unsigned int a = left;
    unsigned int b = right;
    unsigned int x = clause_id;

#ifdef COMMENTS
    fprintf(dimacsFile, "c ---- BEGIN %s ----\n", type_to_string(type).c_str());
#endif
    switch (type) {
        case SAT_NOT:
            // X iff not A => (not A or not X) and (A or X)
            fprintf(dimacsFile, "-%i -%i 0\n%i %i 0\n", a, x, a, x);
            number_clauses += 2;
            break;
        case SAT_AND:
            // X iff (A and B) => (not A or not B or X) and (A or not X) and (B or not X)
            fprintf(dimacsFile, "-%i -%i %i 0\n%i -%i 0\n%i -%i 0\n", a, b, x, a, x, b, x);
            number_clauses += 3;
            break;
        case SAT_OR:
            // X iff (A or B) => (A or B or not X) and (not A or X) and (not B or X)
            fprintf(dimacsFile, "%i %i -%i 0\n-%i %i 0\n-%i %i 0\n", a, b, x, a, x, b, x);
            number_clauses += 3;
            break;
        case SAT_XOR:
            // X iff (A xor B) => (not A or not B or not X) and (not A or B or X) and (A or not B or X) and (A and B and not X)
            fprintf(dimacsFile, "-%i -%i -%i 0\n-%i %i %i 0\n%i -%i %i 0\n%i %i -%i 0\n", a, b, x, a, b, x, a, b, x, a, b, x);
            number_clauses += 4;
            break;
        case SAT_IMPLIES:
            // X iff (A -> B) => (not A or B or not X) and (A or X) and (not B or X)
            fprintf(dimacsFile, "-%i %i -%i 0\n%i %i 0\n-%i %i 0\n", a, b, x, a, x, b, x);
            number_clauses += 3;
            break;
        case SAT_EQUALS:
            // X iff (A iff B) => (not A or not B or X) and (not A or B or not X) and (A or not B or not X) and (A or B or X)
            fprintf(dimacsFile, "-%i -%i %i 0\n-%i %i -%i 0\n%i -%i -%i 0\n%i %i %i 0\n", a, b, x, a, b, x, a, b, x, a, b, x);
            number_clauses += 4;
            break;
        default:
            fprintf(stderr, "registered wrong clause type");
    }
#ifdef COMMENTS
    fprintf(dimacsFile, "c ----  END  %s ----\n", type_to_string(type).c_str());
#endif

    return clause_id;
}

unsigned int SATSymbolTable::parse_variable(char * name) {

    auto search = name_to_id.find(name);
    unsigned int id = next_id;

    if (search == name_to_id.end()) {
        name_to_id[name] = id;
        id_to_name[id] = name;
        next_id++;
    } else {
        id = search->second;
    }
    return id;
}

std::string SATSymbolTable::type_to_string(clause_t type) {
    std::string s;
#define PROCESS_VAL(p) case(p): s = #p; break;
    switch(type){
        PROCESS_VAL(SAT_NOT);
        PROCESS_VAL(SAT_AND);
        PROCESS_VAL(SAT_OR);
        PROCESS_VAL(SAT_XOR);
        PROCESS_VAL(SAT_IMPLIES);
        PROCESS_VAL(SAT_EQUALS);
    }
#undef PROCESS_VAL
    return s;
}