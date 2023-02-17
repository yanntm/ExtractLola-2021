#pragma once

#include <map>
#include <string>
#include <limits>
#include <Planning/SATTask.h>

#define DIMACS_COMMENTS1

#define DIMACS_FILE "dimacs.out"
#define SOLUTION_FILE "minisat.out"
#define READABLE_SOLUTION_FILE "result.out"

typedef enum {
    SAT_NOT, SAT_AND, SAT_OR, SAT_XOR, SAT_EQUALS, SAT_IMPLIES
} clause_t;

class SATSymbolTable {
private:
    static std::string type_to_string(clause_t type);
    static std::map<std::pair<int, int>, unsigned int> parsed_ids[5];
    static std::map<std::string, unsigned int> name_to_id;
    static std::map<unsigned int, std::string> id_to_name;

    static unsigned int _parse_clause(clause_t type, int, int);

public:

    static FILE * dimacsFile;
    static FILE * readableSatFile;

    static unsigned int parse_clause(clause_t type, int, int);
    static unsigned int parse_variable(char * name);

    static unsigned int number_clauses;
    static unsigned int skipped_vars;

    static unsigned int next_id;
    static unsigned int last_id;

    static void unparse_var(int);
    static void unparse_sat(int);

    static void reset();

    static void complete();

};