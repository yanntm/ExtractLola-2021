#pragma once
#include<Frontend/Parser/ParserPTNet.h>
#include<Frontend/Parser/ValueAutomaton.h>
#include<Core/Dimensions.h>

class ParserPTNet;

ParserPTNet* ReadPnmlFile();
ParserPTNet* unfoldHLNet();
ParserPTNet* unfoldHLNetStableMarking();
void ReadPnmlFormula();
char * ReadPnmlFormulaId();
void ReadHLPnmlFormula();

#define HLHASHTABLESIZE 10000000
class ParserPTNet;
class ValueAutomaton;

extern bool HighLevelNet;

typedef enum {SO_FIN, SO_CYC, SO_INT, SO_PRO, SO_DOT, SO_DIFF} tSortTag;
class sort
{
public:
	char * name;
	int vectorlength; // overall length of tuple
	int size;
	tSortTag tag;
	static sort ** hash_table;
	static void insert(sort *);
	static sort * lookup(char *);
	static void init();
	static sort * dotsort;
	virtual std::string value2name(int) = 0;
};

class constant;

class finitesort : public sort
{
public:
	finitesort(tSortTag t, int n){cyclic = (t == SO_FIN )? false : true; symbol = new constant *[n]; size = n; tag = t; vectorlength = 1;}
	constant ** symbol;
	bool cyclic;
	int lower; // only relevant for SO_INT
	int upper; // only relevant for SO_INT
	virtual std::string value2name(int);
};

class productsort : public sort
{
public:
	productsort(int n) {cardsubsorts = n; subsort = new sort * [n]; size = 1; tag = SO_PRO; vectorlength = 0;}
	int cardsubsorts;
	sort ** subsort;
	void addSubSort(int i, sort * s) { subsort[i] = s; size *= s -> size; vectorlength += s -> vectorlength;}
	virtual std::string value2name(int);
};

class dotsort : public sort
{
public:
	dotsort(){ size = 1; tag = SO_DOT;vectorlength = 1;}
	virtual std::string value2name(int);
};

class constant
{
public:
	char * name;
	sort * so;
	int index;
        static constant ** hash_table;
        static void insert(constant *);
        static constant * lookup(char *);
        static void init();
};

class setconstant
{
public:
	char * name;
	sort * so;
	int * index;
	int card;
        static setconstant ** hash_table;
        static void insert(setconstant *);
        static setconstant * lookup(char *);
        static void init();
};

class hlplace;

class hlvariable
{
public: 
	static int card;
	static int placecard;
	char * name;
	sort * so;
	int offset;
	static hlvariable ** hash_table;
	static hlvariable ** placehash_table;
	static hlvariable * lookup(char *, bool isplace = false);
	static void insert(hlvariable *, bool isplace = false);
	static void init();
	hlplace * place; // NULL if not place variable (for findlow)
	bool diff; // true if place variable for handling difference multisets
	hlvariable * child; // if tuple sort, link to variables for components
	hlvariable * next; // if tuple component, link to next component
};

class hlplace
{
public:
	char * name;
	static int card;
	sort * so;
	int offset;
	int index;
	static hlplace ** hash_table;
	static hlplace * lookup(char *);
	static void insert(hlplace *);
	static void init();
	int * initialmarking;
	int  skeletonInitialmarking;
	bool runplace; // never changes marking due to structural equivalence of inscriptions
	bool constantplace; // never changes token count due to numerical equaulity of arc inscriptions
	bool open; // for StableMarking: true = stability not yet determined
	bool stable; // for StableMarking: if not open: true = proven to be constant, false = proven to be nonconstant
	hlvariable ** placevariables;
};


typedef enum { TE_VAR, TE_CON, TE_SUC, TE_PRE, TE_PL } tHlTermTag;

class hlterm
{
public:
	tHlTermTag tag;
	sort * so;
	virtual int evaluate(int *,hlvariable **, int) = 0;
	virtual void print() = 0;
	virtual bool * getvars(hlvariable **, int) = 0;
	virtual bool equivalent(hlterm *) = 0;
	virtual void processrunplace(int,int,int*,int **,int *, int*) = 0;

};

class variableterm : public hlterm
{
public:
	hlvariable * var;
	variableterm(hlvariable * v);
	int evaluate(int *,hlvariable **, int);
	bool * getvars(hlvariable **, int);
	void print()
	{
		std::cout << var -> name;
	}
	bool equivalent(hlterm *);
	void processrunplace(int,int,int*,int **,int *,int*);
	int getoffset(hlvariable **, int); // position in assignment vector
};

class succterm : public hlterm
{
public:
	hlterm * subterm;
	int evaluate(int *,hlvariable **, int);
	succterm(hlterm *);
	bool * getvars(hlvariable **, int);
	void print()
	{
		std::cout << "succ(";
		subterm -> print();
		std::cout << ")";
	}
	bool equivalent(hlterm *);
	void processrunplace(int,int,int*,int **,int *,int*);
};

class predterm : public hlterm
{
public:
	hlterm * subterm;
	int evaluate(int *,hlvariable **, int);
	predterm(hlterm *);
	bool * getvars(hlvariable **, int);
	void print()
	{
		std::cout << "pred(";
		subterm -> print();
		std::cout << ")";
	}
	bool equivalent(hlterm *);
	void processrunplace(int,int,int*,int **,int *,int*);
	
};

class constantterm : public hlterm
{
public:
	int value;
	int evaluate(int *,hlvariable **, int);
        constantterm(int);
	bool * getvars(hlvariable **, int);
	void print()
	{
		std::cout << value;
	}
	bool equivalent(hlterm *);
	void processrunplace(int,int,int*,int **,int *,int*);

};

class condition
{
public:
	virtual ValueAutomaton * evaluate(hlvariable **, int) = 0; // return array of all satisfying assignments
	virtual void print() = 0;
	virtual bool * getvars(hlvariable **, int) = 0;
};

class hlterm;

class andcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	int cardsub;
	condition ** sub;
	void print()
	{
		std::cout << "(";
		for(int i = 0; i < cardsub;i++)
		{
			sub[i]->print();
			if(i < cardsub -1) std::cout << " && ";
		}
		std::cout << ")";
	}
};

class orcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	int cardsub;
	condition ** sub;
	void print()
	{
		std::cout << "(";
		for(int i = 0; i < cardsub;i++)
		{
			sub[i]->print();
			if(i < cardsub -1) std::cout << " || ";
		}
		std::cout << ")";
	}
};

class eqcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " == ";
		sub2->print();
		std::cout << ")";
	}
};

class neqcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " != ";
		sub2->print();
		std::cout << ")";
	}
};

class ltcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " < ";
		sub2->print();
		std::cout << ")";
	}
};

class lecondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " <= ";
		sub2->print();
		std::cout << ")";
	}
};

class gtcondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " > ";
		sub2->print();
		std::cout << ")";
	}
};

class gecondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	hlterm * sub1;
	hlterm * sub2;
	void print()
	{
		std::cout << "(";
		sub1->print();
		std::cout << " >= ";
		sub2->print();
		std::cout << ")";
	}
};

class nocondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int);
	void print()
	{
		std::cout << "NONE";
	}
};

class hlmultiterm;

class runplacecondition : public condition
{
public:
	ValueAutomaton * evaluate(hlvariable **, int);
	bool * getvars(hlvariable **, int card){bool * result = new bool[card]; memset(result,0,sizeof(bool) * card); return result;}
	hlplace * p;
	hlmultiterm * inscription;
	void print(){}
};
class hlarc;

class hltransition
{
public:
	char * name;
	static int card;
	condition * guard;
	hlarc * pre;
	hlarc * post;
	static hltransition ** hash_table;
	static hltransition * lookup(const char *);
	static void insert(hltransition *);
	static void init();
	bool * getvars(hlvariable **, int);
	bool * vars;
	bool open; // StableMarking: true = quasiliveness not determined
	bool quasilive; // StableMarking, if not open: true = transition quasilive, false = transition not quasilive
	ValueAutomaton * firingmodes;
	int cardfiringmodes;
	int offset;
	int index;
};


class hlarc
{
public:
	bool pre;
	hlplace * place;
	hltransition * transition;
	hlmultiterm * inscription;
	hlarc * next;
	int skeletonMult;
};

typedef enum {CO_MT, AL_MT, TE_MT, SU_MT, DI_MT, TU_MT} tMTTag;

class hlmultiterm
{
public:
	tMTTag tag;
	int nroftokens; 
	sort * so;
	virtual int * evaluate(int *,hlvariable **, int) = 0; // returns multiset of sort so
	bool matchable; // simple enough for inferring firing modes from initial marking
	virtual ValueAutomaton * match(hlplace *) = 0; // must be called only if matchable=true
	virtual void print() = 0;
	virtual bool * getvars(hlvariable **, int) = 0;
	virtual bool equivalent(hlmultiterm *) = 0;
	virtual ValueAutomaton * initial2automaton(hlplace *) = 0;
	virtual void adddiffvars() = 0;
	virtual ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size) = 0;
};

class coefficienthlmultiterm : public hlmultiterm
{
public:
	int coefficient;
	hlmultiterm * submt;
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	coefficienthlmultiterm(sort *, int, hlmultiterm *);
	void print()
	{
		std::cout << coefficient;
		std::cout << " * ";
		submt->print();
	}
	bool * getvars(hlvariable **, int);
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *);
	void adddiffvars(){submt -> adddiffvars();}
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
	ValueAutomaton * createcoefficientfindlowautomaton(hlvariable ** tobeused, int size, int cardin, int cardout);
	
};

class allhlmultiterm : public hlmultiterm
{
public:
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	allhlmultiterm(sort *);
	void print()
	{
		std::cout << "ALL(" << so -> name << ")";
	}
	bool * getvars(hlvariable **, int) ;
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *){return NULL;}
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
	void adddiffvars(){}
};


class termhlmultiterm : public hlmultiterm
{
public:
	hlterm * subterm;
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	termhlmultiterm(sort *, hlterm *);
	void print()
	{
		std::cout << "(";
		subterm->print();
		std::cout << ")";
	}
	bool * getvars(hlvariable **, int);
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *);
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
	void adddiffvars(){}
};

class sumhlmultiterm : public hlmultiterm
{
public:
	int card;
	hlmultiterm ** sub;
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	sumhlmultiterm(sort *, int, hlmultiterm **);
	void print()
	{
		std::cout << "(";
		for(int i = 0; i < card; i++)
		{
			sub[i]->print();
			if(i < card - 1) std::cout << " + ";
		}
		std::cout << ")";
	}
	bool * getvars(hlvariable **, int);
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *);
	void adddiffvars()
	{
		for(int i = 0; i < card; i++)
		{
			sub[i]->adddiffvars();
		}
	}
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
	ValueAutomaton * createsumfindlowautomaton(hlvariable ** tobeused, int size, int firstsummand, int cardsummands, int cardin, int cardout);
};

class diffhlmultiterm : public hlmultiterm
{
public:
	hlmultiterm * sub1;
	hlmultiterm * sub2;
	int carddiffvars;
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	diffhlmultiterm(sort *, hlmultiterm *, hlmultiterm *);
	void print()
	{
		std::cout << "(";
		sub1 -> print();
		std::cout << " - ";
		sub2 -> print();
		std::cout << ")";
	}
	bool * getvars(hlvariable **, int);
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *){return NULL;}
	hlvariable ** diffvars;
	void adddiffvars()
	{
		carddiffvars = 0;
		diffvars = new hlvariable * [sub2 -> nroftokens];
		for(int i = 0; i < sub2 -> nroftokens; i++)
		{
			diffvars[i] = new hlvariable();
			diffvars[i] -> diff = true;
			diffvars[i] -> name = deconst("diff");
			diffvars[i]->so = sub2 -> so;
			hlvariable::insert(diffvars[i], true);
		}
		sub1 -> adddiffvars();
		sub2 -> adddiffvars();
	}
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
};


class tuplehlmultiterm : public hlmultiterm
{
public:
	int card;
	hlterm ** sub;
	int * evaluate(int *,hlvariable **, int); 
	ValueAutomaton * match(hlplace *){return NULL;}
	tuplehlmultiterm(sort *, int, hlterm **);
	void print()
	{
		std::cout << "[";
		for(int i = 0; i < card; i++)
		{
			sub[i]->print();
			if(i < card - 1) std::cout << ", ";
		}
		std::cout << "]";
	}
	bool * getvars(hlvariable **, int);
	bool equivalent(hlmultiterm *);
	ValueAutomaton * initial2automaton(hlplace *);
	void adddiffvars(){}
	ValueAutomaton * createfindlowautomaton(hlvariable ** tobeused, int size);
};

bool analyseTransitionClasses(ParserPTNet *, arrayindex_t *, arrayindex_t *, bool *);

class nestedunit
{
public:
	char * name; // name of unit
	PlaceSymbol * p; // an element of this unit
	nestedunit * nextinunit; // next element of same (or parent) unit
	nestedunit * firstinunit; // first element of this unit;
	nestedunit * nextunit; // points to first element of next unit, only set for first elements of units
	nestedunit * firstunit; // points to first element of first unit, only set for first elements of units
	bool leafunit; // true if unit does not contain subunits
	bool marked;
};
