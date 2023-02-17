// An automaton that accepts the vectors that correspond to firing modes of transitions.

#pragma once
#include<Frontend/Parser/ParserPnml.h>

class sort;
class hlterm;
class hlarc;

class ValueAutomatonState;

class producthashtable;
class minimisehashtable;

class ValueAutomatonTransition
{
public:
	// semantics: if a character between lower and upper (both inlcuding)
	// is read, succ is the appropriate successor state
	int lower;
	int upper;
	ValueAutomatonState * succ;
	ValueAutomatonTransition * next; // next in the list of transitions of a state
	ValueAutomatonTransition();
};

class ValueAutomatonState
{
public:
	int id;
	static int maxid;
	bool final; // is a final state
	int value; // arithmetic value attached to state if it is final and represents a term
	ValueAutomatonTransition * transition; // transitions having this as their source
	ValueAutomatonState * next_sibling; // next state on same depth
	ValueAutomatonState * oldstate1;	// original state in product constructions
	ValueAutomatonState * oldstate2;	// original state in product constructions
	ValueAutomatonState();
	int card; // number of accepting paths starting from here
};

class hlvariable;

class ValueAutomaton
{
public:
	ValueAutomatonState * initialState;
	int depth; // (uniform) length of accepted sequences
	sort ** alphabet; // subsort at level i, array of length depth
	hlvariable ** vars; // array of length depth, variable where i-th letter carries assignment for
///int * originaloffset;
	static void init(); // init static stuff
	ValueAutomatonState ** level; // level[i] points to a state of level i, i.e. processing i-th letter
	ValueAutomatonState ** iterationstatestack; // iss[i] points to a state of level i
	ValueAutomatonTransition ** iterationtransitionstack; // its[i] points to a state of level i
	int * iterationprevstack;
	int * iterationnextstack;
	void andProduct(hlvariable **, int, ValueAutomaton *); // 1st arg: list of vars that determines ordering
	void orProduct(hlvariable **, int, ValueAutomaton *);
	ValueAutomaton(); // create automata that accepts empty language
	ValueAutomaton(hlterm *, hlvariable **,int ); // 1st arg: global card vars 2nd arg: ordering (array of size 1st); create automaton that represents given term
	ValueAutomaton(hlvariable *, hlvariable **, int); // create automaton that represents given variable
	ValueAutomaton(int, bool *, hlvariable **, hlarc *);  // create automaton that represents arc inscription
	void eqProduct(hlvariable **, int, ValueAutomaton *);
	void neqProduct(hlvariable **, int, ValueAutomaton *);
	void ltProduct(hlvariable **, int, ValueAutomaton *);
	void gtProduct(hlvariable **, int, ValueAutomaton *);
	void leProduct(hlvariable **, int, ValueAutomaton *);
	void geProduct(hlvariable **, int, ValueAutomaton *);
	void minimise();
	void print();
	~ValueAutomaton();
	int count(int, hlvariable **, bool *); // determine number of accepting runs
	int * first_assignment(int, hlvariable **, bool *); // first accepting run translated into variable assignment
	int * next_assignment(); // returns NULL if beyond last assigment
	int * currentassignment; // returns NULL if beyond last assigment
	void project(int); // project to prefixes of length i
	int mycard;
	hlvariable ** myordering;
	bool * myoccurring;
	static ValueAutomatonState * makeValueAutomatonState(producthashtable *,ValueAutomatonState *, ValueAutomatonState *, ValueAutomatonState **);
};


class producthashtable
{
public:
	int card;
	int size;
	int * id1table;
	int * id2table;
	ValueAutomatonState ** statetable;
	void insert(int,int,ValueAutomatonState *);
	ValueAutomatonState * lookup(int,int);
	int getHash(int,int);
	producthashtable();
	~producthashtable();
};

class minimisehashtable
{
public:
	ValueAutomatonState ** statetable;
	void insert(ValueAutomatonState *);
	ValueAutomatonState * lookup(ValueAutomatonState *);
	int getHash(ValueAutomatonState *);
	minimisehashtable();
	~minimisehashtable();
	int card;
	int size;
};
