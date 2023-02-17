#include<Frontend/Parser/ParserPnml.h>
#include<Frontend/Parser/ValueAutomaton.h>
#include<Rapidxml/rapidxml.h>
#include <Rapidxml/rapidxml_iterators.h>
#include <Rapidxml/rapidxml_utils.h>
#include <Rapidxml/rapidxml_print.h>
#include <Core/Runtime.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <Core/Dimensions.h>
#include <Frontend/SymbolTable/ArcList.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <config.h>
#include <Core/Dimensions.h>
#include <CoverGraph/CoverGraph.h>
#include <Frontend/SymbolTable/ArcList.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/error.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-yystype.h>
#include <Formula/StatePredicate/DeadlockPredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <Formula/StatePredicate/TruePredicate.h>
#include <Formula/StatePredicate/FalsePredicate.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Formula/StatePredicate/Term.h>

#include <limits.h>
#include <libgen.h>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <set>

bool HighLevelNet = false;

tShape oppShape(tShape);

hlvariable ** hlvariable::hash_table;
hlvariable ** hlvariable::placehash_table;
int hlvariable::card = 0;
int hlvariable::placecard = 0;
hlvariable ** unfoldlist = NULL;
hlvariable ** findlowlist = NULL;
int unfoldcard;
int findlowcard;
sort ** sort::hash_table;
hlplace ** hlplace::hash_table;
hltransition ** hltransition::hash_table;
constant ** constant::hash_table;
setconstant ** setconstant::hash_table;
sort * sort::dotsort;

int hlplace::card = 0;
int hltransition::card = 0;

typedef enum { ST_TEMP, ST_COMP, ST_FIR, ST_DL, ST_AND, ST_OR, ST_TRUE, ST_FALSE} shapetype;

PlaceSymbol ** TheHLPlaceSymbols = NULL;
TransitionSymbol ** TheHLTransitionSymbols = NULL;
PlaceSymbol ** ThePlaceSymbols = NULL;
TransitionSymbol ** TheTransitionSymbols = NULL;


void hlvariable::init()
{
	hash_table = new hlvariable * [HLHASHTABLESIZE];
	placehash_table = new hlvariable * [HLHASHTABLESIZE];
	memset(hash_table,0, HLHASHTABLESIZE * sizeof(hlvariable *));
	memset(placehash_table,0, HLHASHTABLESIZE * sizeof(hlvariable *));
}

void hltransition::init()
{
	hash_table = new hltransition * [HLHASHTABLESIZE];
	memset(hash_table, 0, HLHASHTABLESIZE * sizeof(hltransition *));
	card = 0;
}

void sort::init()
{
	dotsort = NULL;
	hash_table = new sort * [HLHASHTABLESIZE];
	memset(hash_table, 0, HLHASHTABLESIZE * sizeof(sort *));
}

void hlplace::init()
{
	hash_table = new hlplace * [HLHASHTABLESIZE];
	memset(hash_table, 0, HLHASHTABLESIZE * sizeof(hlplace *));
	card = 0;
}

void constant::init()
{
	hash_table = new constant * [HLHASHTABLESIZE];
	memset(hash_table, 0, HLHASHTABLESIZE * sizeof(constant *));
}

void setconstant::init()
{
	hash_table = new setconstant * [HLHASHTABLESIZE];
	memset(hash_table, 0, HLHASHTABLESIZE * sizeof(setconstant *));
}

int getHash(const char * name)
{
  int i;
  int h = 0;

  for (i=0; i<strlen(name); ++i)

  {

    h += name[i];

    h += (h << 10);

    h ^= (h >> 6);

  }

  h += (h << 3);

  h ^= (h >> 11);

  h += (h << 15);

  h %=  HLHASHTABLESIZE;
  while(h < 0) h += HLHASHTABLESIZE;
  while(h >= HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
  return h;
}

hlvariable * hlvariable::lookup(char * name, bool isplace)
{
	int h = getHash(name);
	if(isplace)
	{
		while(placehash_table[h] && std::strcmp(name,placehash_table[h]->name))
		{
			h += 7;
			if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
		}
		return placehash_table[h];
	}
	else
	{
		while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
		{
			h += 7;
			if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
		}
		return hash_table[h];
	}
}

hltransition * hltransition::lookup(const char * name)
{
	int h = getHash(name);
	while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
	{
		h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
	}
	return hash_table[h];
}

sort * sort::lookup(char * name)
{
	int h = getHash(name);
	while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
	{
		h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
	}
	return hash_table[h];
}

hlplace * hlplace::lookup(char * name)
{
	int h = getHash(name);
	while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
	{
		h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
	}
	return hash_table[h];
}

constant * constant::lookup(char * name)
{
	int h = getHash(name);
	while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
	{
		h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
	}
	return hash_table[h];
}

setconstant * setconstant::lookup(char * name)
{
	int h = getHash(name);
	while(hash_table[h] && std::strcmp(name,hash_table[h]->name))
	{
		h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
	}
	return hash_table[h];
}

void constant::insert(constant * v)
{
	// assume: no constant with this name inserted yet
	int h = getHash(v->name);
        while(hash_table[h])
        {
                h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
        }
	hash_table[h] = v;
}

void setconstant::insert(setconstant * v)
{
	// assume: no constant with this name inserted yet
	int h = getHash(v->name);
        while(hash_table[h])
        {
                h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
        }
	hash_table[h] = v;
}

void hlvariable::insert(hlvariable * v, bool isplace)
{
	// assume: no variable with this name inserted yet
	int h = getHash(v->name);
	if(isplace)
	{
		while(placehash_table[h])
		{
			h += 7;
			if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
		}
		placehash_table[h] = v;
		placecard++;
	}
	else
	{
		while(hash_table[h])
		{
			h += 7;
			if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
		}
		hash_table[h] = v;
		card++;
		v -> place = NULL;
	}
	v->child = v->next = NULL;
	// if variable has product type: insert generated variables for components
	if(v -> so -> tag == SO_PRO)
	{
		hlvariable ** anchor = & (v->child);
		productsort * ps = (productsort *) v -> so;
		for(int i = 0; i < ps -> cardsubsorts;i++)
		{
			char * newname = (char *) malloc(strlen(v->name) + 20);
			strcpy(newname,v->name);
			strcpy(newname+strlen(newname),"[");
			sprintf(newname+strlen(newname),"%d",i);
			strcpy(newname+strlen(newname),"]");
			hlvariable * newvar = new hlvariable();
			newvar -> name = newname;
			newvar -> place = v->place;
			newvar -> so = ps->subsort[i];
			newvar -> child = newvar -> next = NULL;
			* anchor = newvar;
			anchor = &(newvar -> next);
			hlvariable::insert(newvar,isplace);
		}
	}
}

void sort::insert(sort * v)
{
	// assume: no sort with this name inserted yet
	int h = getHash(v->name);
        while(hash_table[h])
        {
                h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
        }
	hash_table[h] = v;
}

void hlplace::insert(hlplace * v)
{
	// assume: no hlplace with this name inserted yet
	int h = getHash(v->name);
        while(hash_table[h])
        {
                h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
        }
	hash_table[h] = v;
	v->runplace = true;
	v->constantplace = true;
	v -> open = true;
	v -> stable = true;
	card++;
}

void hltransition::insert(hltransition * v)
{
	// assume: no hltransition with this name inserted yet
	int h = getHash(v->name);
        while(hash_table[h])
        {
                h += 7;
		if(h > HLHASHTABLESIZE) h -= HLHASHTABLESIZE;
        }
	hash_table[h] = v;
	v -> open = true;
	card++;
}

std::string dotsort::value2name(int i)
{
	return "dot";
}

std::string finitesort::value2name(int i)
{
	return std::string(symbol[i]->name);
}

std::string productsort::value2name(int i)
{
	std::string result("");
	for(int j = cardsubsorts -1; j > 0; j--)
	{
		int k;
		k = i % subsort[j]->size;
		result = "_" + subsort[j] ->value2name(k) + result;
		i /= subsort[j]->size;
	}
	result = subsort[0]->value2name(i) + result;
	return result;
}

using namespace rapidxml;
// size of read buffer: 256*1024
#define BUFFERSIZE 262144



int * evaluateInitial(sort * s, xml_node<> * termnode)
{
	if(!std::strcmp(termnode->name(), "subterm"))
	{
		return evaluateInitial(s,termnode->first_node());
	}
	if(!std::strcmp(termnode->name(), "add"))
	{
		xml_node<> * firstsub = termnode->first_node();
		int * result = evaluateInitial(s,firstsub);
		for(xml_node<> * subnode = firstsub->next_sibling(); subnode; subnode = subnode -> next_sibling())
		{
			int * subresult = evaluateInitial(s, subnode);
			for(int i = 0; i < s -> size; i++)
			{
				result[i] += subresult[i];
			}
			delete [] subresult;
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "subtract"))
	{
		xml_node<> * firstsub = termnode->first_node();
		int * result = evaluateInitial(s,firstsub);
		for(xml_node<> * subnode = firstsub->next_sibling();  subnode; subnode = subnode->next_sibling())
		{
			int * subresult = evaluateInitial(s, subnode);
			for(int i = 0; i < s -> size; i++)
			{
				result[i] -= subresult[i];
			}
			delete [] subresult;
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "numberof"))
	{
		int * result;
		int mult = 1;
		xml_node<> * firstsub = termnode->first_node("subterm");
		if(firstsub->first_node("numberconstant"))
		{
			mult = atoi(firstsub -> first_node("numberconstant") -> first_attribute("value")->value());
			xml_node<> * firstnode = firstsub->next_sibling(); 
			result = evaluateInitial(s, firstnode);
			for(xml_node<> * subnode = firstnode->next_sibling(); subnode; subnode = subnode -> next_sibling())
			{
				int * subresult = evaluateInitial(s, subnode);
				for(int i = 0; i < s -> size; i++)
				{
					result[i] += subresult[i];
				}
				delete [] subresult;
			}
		}
		else
		{
			xml_node<> * firstnode = firstsub; 
			result = evaluateInitial(s, firstnode);
			for(xml_node<> * subnode = firstnode->next_sibling(); subnode; subnode = subnode -> next_sibling())
			{
				int * subresult = evaluateInitial(s, subnode);
				for(int i = 0; i < s -> size; i++)
				{
					result[i] += subresult[i];
				}
				delete [] subresult;
			}
		}
		for(int i = 0; i < s -> size; i++)
		{
			result[i] *= mult;
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "dotconstant"))
	{
		if(s -> tag != SO_DOT)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		int * result = new int[1];
		result[0] = 1;
		return result;
	}
	if(!std::strcmp(termnode->name(), "useroperator"))
	{
		if(s -> tag != SO_CYC && s -> tag != SO_INT && s -> tag != SO_FIN)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		finitesort * fs = (finitesort *) s;
		char * pnmlname = termnode -> first_attribute("declaration")->value();
		setconstant * sc;
		constant * c = constant::lookup(pnmlname);
		bool isset = false;
		if(!c)
		{
			 sc = setconstant::lookup(pnmlname);
			if(!sc)
			{
				RT::rep->status("initial marking uses unknown constant symbol");
				RT::rep->abort(ERROR_SYNTAX);
			}
			isset = true;
		}
		if((isset ? sc -> so : c -> so) != s)
		{
			RT::rep -> status("constant with wrong type used in initial marking");
			RT::rep -> status("c = %s, s = %s", pnmlname, s -> name);
			RT::rep->abort(ERROR_SYNTAX);
		}
		int * result = new int[fs->size];
		if(isset)
		{
			memcpy(result, sc -> index, sc -> so -> size * sizeof(int));
		}
		else
		{
			memset(result,0, fs->size * sizeof(int));
			result[c->index] = 1;
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "finiteintrangeconstant"))
	{
		if(s -> tag != SO_INT)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		finitesort * fs = (finitesort *) s;
		char * pnmlname = termnode -> first_attribute("value")->value();
		char constanttext[1000];
		sprintf(constanttext,"%s_%s", s->name, pnmlname);
		constant * c = constant::lookup(constanttext);
		if(!c)
		{
			RT::rep->status("initial marking uses unknown value");
			RT::rep->status("c = %s", constanttext);
			RT::rep->abort(ERROR_SYNTAX);
		}
		if(c -> so != s)
		{
			RT::rep -> status("constant with wrong type used in initial marking");
			RT::rep -> status("c = %s, s = %s", constanttext, s -> name);
			RT::rep->abort(ERROR_SYNTAX);
		}
		int * result = new int[fs->size];
		memset(result,0, fs->size * sizeof(int));
		result[c->index] = 1;
		return result;
	}
	if(!std::strcmp(termnode->name(), "all"))
	{
		int * result = new int[s->size];
		for(int i = 0; i < s->size;i++)
		{
			result[i] = 1;
		}
		return result;
	}
	if(!std::strcmp(termnode->name(),"tuple"))
	{
		if(s -> tag != SO_PRO)
		{
			RT::rep->status("Initial marking contains tuple but is not product type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		productsort * ps = (productsort *) s;
		int card = 0;
		for(xml_node<> * subnode = termnode -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			card++;
		}
		if(card != ps -> cardsubsorts)
		{
			RT::rep->status("Initial marking contains tuple of wrong dimension");
			RT::rep->abort(ERROR_SYNTAX);
		}
		int ** subresult = new int * [card];
		card = 0;
		for(xml_node<> * subnode = termnode -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			subresult[card] = evaluateInitial(ps->subsort[card], subnode);
			card++;
		}
		int * result = new int[s->size];
		memset(result,0,s->size * sizeof(int));
		int subindex[card];
		memset(subindex, 0, card * sizeof(int));
		for(int i = 0; i < card;i++)
		{
			int j;
			for(j = 0; j < ps->subsort[i]->size;j++)
			{
				if(subresult[i][j]) break;
			}
			if(j == ps->subsort[i]->size)
			{
				RT::rep->status("Initial marking has empty tuple");
				RT::rep->abort(ERROR_SYNTAX);
			}
			subindex[i] = j;
		}
		while(true)
		{
			int value = subindex[0];
			for(int i = 1; i < card; i++)
			{
				value *= ps->subsort[i]->size;
				value += subindex[i];
			}
			result[value] = 1;
			bool ueberlauf;
			int i = card - 1;
			do
			{
				ueberlauf = false;
				int j;
				for(j = subindex[i]+1;j < ps->subsort[i]->size;j++)
				{
					if(subresult[i][j]) break;
				}	
				if(j >= ps -> subsort[i]->size)
				{
					ueberlauf = true;
					for(j = 0; j < ps->subsort[i]->size;j++)
					{
						if(subresult[i][j]) break;
					}
				}
				subindex[i] = j;
				if(i-- == 0) break;
			}
			while(ueberlauf);
			if(ueberlauf) break;
		}

		for(int i = 0; i < card;i++)
		{
			delete [] subresult[i];
		}
		delete [] subresult;
		return result;

	}
	RT::rep->status("unknown operation in initial marking: %s", termnode -> name());
	RT::rep->abort(ERROR_SYNTAX);
}

int skeletonEvaluateInitial(sort * s, xml_node<> * termnode)
{
	if(!std::strcmp(termnode->name(), "subterm"))
	{
		return skeletonEvaluateInitial(s,termnode->first_node());
	}
	if(!std::strcmp(termnode->name(), "add"))
	{
		xml_node<> * firstsub = termnode->first_node();
		int result = skeletonEvaluateInitial(s,firstsub);
		for(xml_node<> * subnode = firstsub->next_sibling(); subnode; subnode = subnode -> next_sibling())
		{
			result += skeletonEvaluateInitial(s, subnode);
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "subtract"))
	{
		xml_node<> * firstsub = termnode->first_node();
		int result = skeletonEvaluateInitial(s,firstsub);
		for(xml_node<> * subnode = firstsub->next_sibling(); subnode; subnode = subnode->next_sibling())
		{
			result -= skeletonEvaluateInitial(s, subnode);
		}
		return result;
	}
	if(!std::strcmp(termnode->name(), "numberof"))
	{
		int result;
		int mult = 1;
		xml_node<> * firstsub = termnode->first_node("subterm");
		if(firstsub->first_node("numberconstant"))
		{
			mult = atoi(firstsub -> first_node("numberconstant") -> first_attribute("value")->value());
			xml_node<> * firstnode = firstsub->next_sibling(); 
			result = skeletonEvaluateInitial(s, firstnode);
			for(xml_node<> * subnode = firstnode->next_sibling(); subnode; subnode = subnode -> next_sibling())
			{
				result += skeletonEvaluateInitial(s, subnode);
			}
		}
		else
		{
			xml_node<> * firstnode = firstsub; 
			result = skeletonEvaluateInitial(s, firstnode);
			for(xml_node<> * subnode = firstnode->next_sibling(); subnode; subnode = subnode -> next_sibling())
			{
				result += skeletonEvaluateInitial(s, subnode);
			}
		}
		result *= mult;
		return result;
	}
	if(!std::strcmp(termnode->name(), "dotconstant"))
	{
		if(s -> tag != SO_DOT)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		return 1;
	}
	if(!std::strcmp(termnode->name(), "useroperator"))
	{
		if(s -> tag != SO_CYC && s -> tag != SO_FIN)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		finitesort * fs = (finitesort *) s;
		char * pnmlname = termnode -> first_attribute("declaration")->value();
		bool isset = false;
		setconstant * sc;
		constant * c = constant::lookup(pnmlname);
		if(!c)
		{
			sc = setconstant::lookup(pnmlname);
			if(!sc)
			{
				RT::rep->status("initial marking uses unknown constant symbol");
				RT::rep->abort(ERROR_SYNTAX);
			}
			isset = true;
		}
		if((isset ? sc-> so : c -> so) != s)
		{
			RT::rep -> status("constant with wrong type used in initial marking");
			RT::rep -> status("cc = %s, s = %s", pnmlname, s -> name);
			RT::rep->abort(ERROR_SYNTAX);
		}
		return isset ? sc -> card : 1;
	}
	if(!std::strcmp(termnode->name(), "finiteintrangeconstant"))
	{
		if(s -> tag != SO_INT)
		{
			RT::rep->status("initial marking has value of wrong type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		finitesort * fs = (finitesort *) s;
		char * pnmlname = termnode -> first_attribute("value")->value();
		char valuetext [1000];
		sprintf(valuetext,"%s_%s", s -> name, pnmlname);
		constant * c = constant::lookup(valuetext);
		if(!c)
		{
			RT::rep->status("initial marking uses unknown value");
			RT::rep->status("c = %s", valuetext);
			RT::rep->abort(ERROR_SYNTAX);
		}
		if(c -> so != s)
		{
			RT::rep -> status("constant with wrong type used in initial marking");
			RT::rep -> status("c = %s, s = %s", valuetext, s -> name);
			RT::rep->abort(ERROR_SYNTAX);
		}
		return 1;
	}
	if(!std::strcmp(termnode->name(), "all"))
	{
		return s->size;
	}
	if(!std::strcmp(termnode->name(),"tuple"))
	{
		if(s -> tag != SO_PRO)
		{
			RT::rep->status("Initial marking contains tuple but is not product type");
			RT::rep->abort(ERROR_SYNTAX);
		}
		productsort * ps = (productsort *) s;
		int card = 0;
		for(xml_node<> * subnode = termnode -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			card++;
		}
		if(card != ps -> cardsubsorts)
		{
			RT::rep->status("Initial marking contains tuple of wrong dimension");
			RT::rep->abort(ERROR_SYNTAX);
		}
		card = 0;
		int result = 1;
		for(xml_node<> * subnode = termnode -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			result *= skeletonEvaluateInitial(ps->subsort[card++], subnode);
		}
		return result;

	}
	RT::rep->status("unknown operation in initial marking: %s", termnode -> name());
	RT::rep->abort(ERROR_SYNTAX);
}

hlterm * readPnmlTerm(xml_node<> * node)
{
	if(!std::strcmp(node -> name(), "subterm"))
	{
		return readPnmlTerm(node -> first_node());
	}
	if(!std::strcmp(node -> name(), "variable"))
	{
		char * varname = node -> first_attribute("refvariable")->value();
		hlvariable * v = hlvariable::lookup(varname);
		if(!v)
		{
			RT::rep->status("transition condition uses undeclared variable");
			RT::rep->abort(ERROR_SYNTAX);
		}
		return new variableterm(v);
	}
	if(!std::strcmp(node -> name(), "successor"))
	{
		return new succterm(readPnmlTerm(node -> first_node()));
	}
	if(!std::strcmp(node -> name(), "predecessor"))
	{
		return new predterm(readPnmlTerm(node -> first_node()));
	}
	if(!std::strcmp(node -> name(), "useroperator"))
	{
		constant * c = constant::lookup(node -> first_attribute("declaration")->value());
		if(!c)
		{
			RT::rep->status("transition condition uses undefined constant");
			RT::rep->abort(ERROR_SYNTAX);
		}
		constantterm * result = new constantterm(c->index);
		result -> so = c -> so;
		return result;
	}
	if(!std::strcmp(node -> name(), "finiteintrangeconstant"))
	{
		int lower;
		sscanf(node -> first_node("finiteintrange") -> first_attribute("start")->value(), "%d",&lower);
		int upper;
		sscanf(node -> first_node("finiteintrange") -> first_attribute("end")->value(), "%d",&upper);

		sort * s;
		int i;
		for(i = 0; i < HLHASHTABLESIZE;i++)
		{
			s = sort::hash_table[i];
			if(s && s -> tag == SO_INT &&
                  	   ((finitesort *) s) -> lower == lower &&
                  	   ((finitesort *) s) -> upper == upper)
			{
				break;
			}
		}
	
		if(i >= HLHASHTABLESIZE)
		{
			RT::rep->status("Integer range constant does not fit to defined sort");
			RT::rep->abort(ERROR_SYNTAX);
		}
		int value;
		sscanf(node -> first_attribute("value")->value(), "%d", &value);
		constantterm * result = new constantterm(value - lower);
		result -> so = s;
		return result;
	}
	if(!std::strcmp(node -> name(), "dotconstant"))
	{
		constantterm * result = new constantterm(0);
		result -> so = sort::dotsort;
		return result;
	}
	RT::rep->status("Unknown symbol in term: %s", node -> name());
	RT::rep->abort(ERROR_SYNTAX);
}

ValueAutomaton * coefficienthlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	if(submt -> nroftokens == 1) // no permutation required
	{
		ValueAutomaton * result = submt->createfindlowautomaton(tobeused,1);
		for(int i = 1; i < coefficient; i++)
		{
			ValueAutomaton * rresult = submt->createfindlowautomaton(tobeused+i, 1);
			result -> andProduct(findlowlist,findlowcard,rresult);
		}
		return result;
	}
	if(coefficient == 1) // no permutation required
	{
		return createfindlowautomaton(tobeused, cardtobeused);
	}
	return createcoefficientfindlowautomaton(tobeused,cardtobeused,0,0);
}


ValueAutomaton * coefficienthlmultiterm::createcoefficientfindlowautomaton(hlvariable ** tobeused, int cardtobeused, int cardin, int cardout)
{

	// pre: tobeused is multiple of submt->nroftokens
	// cardin < submt->nroftokens
	// idea: c * M is interpreted as M + M + M + ... + M
	// first cardin variables fixed for first incarnation of M
	// last cardout variables fixed for other incarnations of M
	// choose one of the undecided variables
	// try in  + try out + descend + OR the results
	// if any try leads to full determination of IN or OUT, call submt -> createfindlow,
	// otherwise, call this recursively

	if(cardin == submt -> nroftokens)
	{
		ValueAutomaton * A1 = submt -> createfindlowautomaton(tobeused, cardin);
		ValueAutomaton * A2;
		if(cardtobeused - cardin <= cardin)
		{
			A2 = submt -> createfindlowautomaton(tobeused + cardin, cardtobeused - cardin);
		}
		else
		{
			A2 = createcoefficientfindlowautomaton(tobeused + cardin, cardtobeused - cardin, 0,0);
		}
		A1 -> orProduct(findlowlist,findlowcard,A2);
		return A1;
	}
	if(cardtobeused - cardout == submt -> nroftokens)
	{
		cardin = submt -> nroftokens;
		ValueAutomaton * A1 = submt -> createfindlowautomaton(tobeused, cardin);
		ValueAutomaton * A2;
		if(cardtobeused - cardin <= cardin)
		{
			A2 = submt -> createfindlowautomaton(tobeused + cardin, cardtobeused - cardin);
		}
		else
		{
			A2 = createcoefficientfindlowautomaton(tobeused + cardin, cardtobeused - cardin, 0, 0);
		}
		A1 -> orProduct(findlowlist,findlowcard,A2);
		return A1;
	}
	ValueAutomaton * A1 = createcoefficientfindlowautomaton(tobeused, cardtobeused, cardin+1,cardout);
	hlvariable ** newtobeused = new hlvariable * [cardtobeused];
	memcpy(newtobeused, tobeused, cardtobeused * sizeof(hlvariable *));
	hlvariable * tmp = newtobeused[cardin];
	newtobeused[cardin] = newtobeused[cardtobeused - cardout - 1];
	ValueAutomaton * A2 = createcoefficientfindlowautomaton(newtobeused, cardtobeused, cardin, cardout+1 );
	A1 -> orProduct(findlowlist,findlowcard,A2);
	delete [] newtobeused;
	return A1;
}

coefficienthlmultiterm::coefficienthlmultiterm(sort * s, int i, hlmultiterm * sub)
{
	tag = CO_MT;
	nroftokens = i * sub -> nroftokens;
	coefficient = i;
	submt = sub;
	so = s;
	matchable = sub -> matchable;
}

int * coefficienthlmultiterm::evaluate(int * ass, hlvariable ** list, int cardlist)
{
	int * result = submt->evaluate(ass,list,cardlist);
	for(int i = 0; i < so->size;i++)
	{
		result[i] *= coefficient;
	}
	return result;
}

bool coefficienthlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != CO_MT) return false;
	coefficienthlmultiterm * co = (coefficienthlmultiterm *) other;
	if(coefficient != co -> coefficient) return false;
	return submt -> equivalent(co -> submt);
}

ValueAutomaton * allhlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	return new ValueAutomaton();
}

allhlmultiterm::allhlmultiterm(sort * s)
{
	so = s;
	tag = AL_MT;	
	matchable = false;
	nroftokens = so -> size;
}

int * allhlmultiterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	int * result = new int[so->size];
	for(int i = 0; i < so->size; i++)
	{
		result[i] = 1;
	}
	return result;
}

bool allhlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != AL_MT) return false;
	allhlmultiterm * al = (allhlmultiterm *) other;
	if(so != al -> so) return false;
	return true;
}

ValueAutomaton * termhlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	ValueAutomaton * A1 = new ValueAutomaton(subterm,findlowlist,findlowcard);
	ValueAutomaton * A2 = new ValueAutomaton(new variableterm(tobeused[0]),findlowlist,findlowcard);
	A1 -> eqProduct(findlowlist,findlowcard,A2);
	return A1;
}

termhlmultiterm::termhlmultiterm(sort * s, hlterm * sub)
{
	tag = TE_MT;
	nroftokens = 1;
	subterm = sub;
	so = s;
	matchable = true;
}

int * termhlmultiterm::evaluate(int * ass, hlvariable ** list, int cardlist)
{
	int * result = new int[so->size];
	memset(result,0,so->size * sizeof(int));
	result[subterm->evaluate(ass,list,cardlist)] = 1;
	return result;
}

bool termhlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != TE_MT) return false;
	termhlmultiterm * te = (termhlmultiterm *) other;
	return subterm->equivalent(te->subterm); 
}


ValueAutomaton * sumhlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	return createsumfindlowautomaton(tobeused,cardtobeused,0,card,0,0);
}


ValueAutomaton * sumhlmultiterm::createsumfindlowautomaton(hlvariable ** tobeused, int cardtobeused, int firstsummand, int cardsummands, int cardin, int cardout)
{

	if(cardin == sub[firstsummand] -> nroftokens)
	{
		ValueAutomaton * A1 = sub[firstsummand] -> createfindlowautomaton(tobeused, cardin);
		ValueAutomaton * A2;
		if(cardsummands == 2)
		{
			A2 = sub[firstsummand+1] -> createfindlowautomaton(tobeused + cardin, cardtobeused - cardin);
		}
		else
		{
			A2 = createsumfindlowautomaton(tobeused + cardin, cardtobeused - cardin, firstsummand+1, cardsummands -1, 0, 0);
		}
		A1 -> orProduct(findlowlist,findlowcard,A2);
		return A1;
	}
	if(cardtobeused - cardout == sub[firstsummand] -> nroftokens)
	{
		cardin = sub[firstsummand] -> nroftokens;
		ValueAutomaton * A1 = sub[firstsummand] -> createfindlowautomaton(tobeused, cardin);
		ValueAutomaton * A2;
		if(cardsummands == 2)
		{
			A2 = sub[firstsummand+1] -> createfindlowautomaton(tobeused + cardin, cardtobeused - cardin);
		}
		else
		{
			A2 = createsumfindlowautomaton(tobeused + cardin, cardtobeused - cardin, firstsummand+1, cardsummands-1,0, 0);
		}
		A1 -> orProduct(findlowlist,findlowcard,A2);
		return A1;
	}
	ValueAutomaton * A1 = createsumfindlowautomaton(tobeused, cardtobeused, firstsummand,cardsummands,cardin+1,cardout);
	hlvariable ** newtobeused = new hlvariable * [cardtobeused];
	memcpy(newtobeused, tobeused, cardtobeused * sizeof(hlvariable *));
	hlvariable * tmp = newtobeused[cardin];
	newtobeused[cardin] = newtobeused[cardtobeused - cardout - 1];
	ValueAutomaton * A2 = createsumfindlowautomaton(newtobeused, cardtobeused, firstsummand,cardsummands,cardin, cardout+1 );
	A1 -> orProduct(findlowlist,findlowcard,A2);
	delete [] newtobeused;
	return A1;
}

sumhlmultiterm::sumhlmultiterm(sort * s, int c, hlmultiterm ** su)
{
	tag = SU_MT;
	card = c;
	sub = su;
	so = s;
	matchable = true;
	nroftokens = 0;
	for(int i = 0; i < card; i++)
	{
		if(!sub[i]->matchable) matchable = false;
		nroftokens += sub[i]->nroftokens;
	}
}

int * sumhlmultiterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	int * result = sub[0]->evaluate(ass,list,cardlist);
	for(int i = 1; i < card; i++)
	{
		int * rresult = sub[i]->evaluate(ass,list,cardlist);
		for(int j = 0; j < so->size; j++)
		{
			result[j] += rresult[j];
		}
		delete [] rresult;
	}
	return result;
}

bool sumhlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != SU_MT) return false;
	sumhlmultiterm * su = (sumhlmultiterm *) other;
	if(card != su -> card) return false;
	for(int i = 0; i < card; i++)
	{
		int j; 
		bool found = false;
		for(j = i; j < card; j++)
		{
			if(su -> sub[i]->equivalent(sub[j]))
			{
				found = true;
				hlmultiterm * tmp = sub[j];
				sub[j] = sub[i];
				sub[i] = tmp;
				break;
			}
		}
		if(!found) return false;
	}
	return true;
}

ValueAutomaton * diffhlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	hlvariable ** postobeused = new hlvariable * [sub1->nroftokens];
	memcpy(postobeused, tobeused, cardtobeused * sizeof(hlvariable *));
	memcpy(postobeused + cardtobeused, diffvars, sub2->nroftokens*sizeof(hlvariable *));
	ValueAutomaton * A1 = sub1 -> createfindlowautomaton(postobeused,sub1->nroftokens);
	ValueAutomaton * A2 = sub2 -> createfindlowautomaton(diffvars,sub2->nroftokens);
	A1 -> andProduct(findlowlist,findlowcard,A2);
	delete [] postobeused;
	return A1;
}
diffhlmultiterm::diffhlmultiterm(sort * s, hlmultiterm * su1, hlmultiterm * su2)
{
	tag = DI_MT;
	sub1 = su1;
	sub2 = su2;
	so = s;
	matchable = false;
	nroftokens = sub1->nroftokens - sub2->nroftokens;
}

int * diffhlmultiterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	int * result = sub1->evaluate(ass,list,cardlist);
	int * rresult = sub2->evaluate(ass,list,cardlist);
	for(int j = 0; j < so->size; j++)
	{
		if(rresult[j] > result[j]) result[j] = 0;
		else result[j] -= rresult[j];
	}
	delete [] rresult;
	return result;
}

bool diffhlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != DI_MT) return false;
	diffhlmultiterm * di = (diffhlmultiterm *) other;
	if(sub1->equivalent(di->sub1)) return false;
	return sub2->equivalent(di->sub2);
}


tuplehlmultiterm::tuplehlmultiterm(sort * s, int c, hlterm ** su)
{
	tag = TU_MT;
        card = c;
        sub = su;
        so = s;
	matchable = true;
	nroftokens = 1;
}

ValueAutomaton * tuplehlmultiterm::createfindlowautomaton(hlvariable ** tobeused, int cardtobeused)
{
	hlvariable * current = tobeused[0]->child;
	ValueAutomaton * A1 = new ValueAutomaton(sub[0],findlowlist,findlowcard);
	ValueAutomaton * A2 = new ValueAutomaton(new variableterm(current),findlowlist,findlowcard);
	A1 -> eqProduct(findlowlist,findlowcard,A2);
	ValueAutomaton * result = A1;
	current = current -> next;
	for(int i = 1; i < card; i++)
	{
		A1 = new ValueAutomaton(sub[i],findlowlist,findlowcard);
		A2 = new ValueAutomaton(new variableterm(current),findlowlist,findlowcard);
		A1 -> eqProduct(findlowlist,findlowcard,A2);
		result -> andProduct(findlowlist,findlowcard,A1);
		current = current -> next;
	}
	return result;
}

int * tuplehlmultiterm::evaluate(int * ass,hlvariable **list, int cardlist)
{
	int result = 0;
	for(int i = 0; i < card; i++)
	{
		result *= sub[i]->so->size;
		result += sub[i]->evaluate(ass,list, cardlist);
	}
	int * res = new int[so->size];
	memset(res,0,so->size * sizeof(int));
	res[result] = 1;
	return res;
}

bool tuplehlmultiterm::equivalent(hlmultiterm * other)
{
	if(other -> tag != TU_MT) return false;
	tuplehlmultiterm * tu = (tuplehlmultiterm *) other;
	if(card != tu -> card) return false;
	for(int i = 0; i < card; i++)
	{
		if(!sub[i]->equivalent(tu->sub[i])) return false;
	}
	return true;
}

condition * readTransitionCondition(xml_node<> * node)
{
	if(!std::strcmp(node -> name(), "lessthan"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		ltcondition * result = new ltcondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "equality"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		eqcondition * result = new eqcondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "greaterthan"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		gtcondition * result = new gtcondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "lessthanorequal"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		lecondition * result = new lecondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "greaterthanorequal"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		gecondition * result = new gecondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "inequality"))
	{
		hlterm * T1 = readPnmlTerm(node -> first_node());
		hlterm * T2 = readPnmlTerm(node -> last_node());
		neqcondition * result = new neqcondition();
		result -> sub1 = T1;
		result -> sub2 = T2;
		return result;
	}
	if(!std::strcmp(node -> name(), "or"))
	{
		int card = 0;
		for(xml_node<> * subnode = node -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			card++;
		}
		condition ** sub = new condition * [card];
		card = 0;
		for(xml_node<> * subnode = node -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			sub[card] = readTransitionCondition(subnode);
			card++;
		}
		orcondition * result = new orcondition();
		result -> sub = sub;
		result -> cardsub = card;
		return result;
	}
	if(!std::strcmp(node -> name(), "and"))
	{
		int card = 0;
		for(xml_node<> * subnode = node -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			card++;
		}
		condition ** sub = new condition * [card];
		card = 0;
		for(xml_node<> * subnode = node -> first_node(); subnode; subnode = subnode -> next_sibling())
		{
			sub[card] = readTransitionCondition(subnode);
			card++;
		}
		andcondition * result = new andcondition();
		result -> sub = sub;
		result -> cardsub = card;
		return result;
	}
	if(!std::strcmp(node -> name(), "subterm"))
	{
		return readTransitionCondition(node -> first_node());
	}
	RT::rep->status("unknown element in transition condition: %s", node -> name());
	RT::rep->abort(ERROR_SYNTAX);
}

hlmultiterm * readArcInscription(sort *s, xml_node<> * mtnode)
{
	if(!std::strcmp(mtnode->name(),"add"))
	{
		int card = 0;
		for(xml_node<> * summandnode = mtnode -> first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			card++;
		}
		hlmultiterm ** subs = new hlmultiterm * [card];
		card = 0;
		for(xml_node<> * summandnode = mtnode -> first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			subs[card] = readArcInscription(s, summandnode);
			card++;
		}
		return new sumhlmultiterm(s, card, subs);
	} 
	if(!std::strcmp(mtnode->name(),"subterm"))
	{
		return readArcInscription(s,mtnode->first_node());
	}
	if(!std::strcmp(mtnode->name(),"numberof"))
	{
		if(!mtnode->first_node("subterm") -> next_sibling("subterm")-> next_sibling())
		{
			return new coefficienthlmultiterm(s,atoi(mtnode->first_node("subterm")->first_node("numberconstant")->first_attribute("value")->value()),readArcInscription(s, mtnode->last_node("subterm")->first_node()));
		}
		int card = 0;
		xml_node<> * anchor = mtnode->first_node("subterm") -> next_sibling("subterm");
		for(xml_node<> * elementnode = anchor; elementnode;elementnode = elementnode -> next_sibling())
		{
			card++;
		}
		hlmultiterm ** sub = new hlmultiterm *[card];
		card = 0;
		for(xml_node<> * elementnode = anchor; elementnode;elementnode = elementnode -> next_sibling())
		{
			sub[card] = readArcInscription(s,elementnode);
			card++;
		}
		return new coefficienthlmultiterm(s,atoi(mtnode->first_node("subterm")->first_node("numberconstant")->first_attribute("value")->value()),new sumhlmultiterm(s,card,sub));
	}
	if(!std::strcmp(mtnode->name(),"all"))
	{
		sort * subsort = sort::lookup(mtnode -> first_node("usersort")->first_attribute("declaration")->value());
		if(!subsort)
		{
			RT::rep->status("arc inscription refers to unknwon sort in _all_ construct");
			RT::rep->abort(ERROR_SYNTAX);
		}
		if(subsort != s)
		{
			RT::rep->status("sort in _all_ construct of arc inscription does not match place sort");
			RT::rep->abort(ERROR_SYNTAX);
		}
		return new allhlmultiterm(s);
	}
	if(!std::strcmp(mtnode->name(),"tuple"))
	{
		int card = 0;
		for(xml_node<> * summandnode = mtnode -> first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			card++;
		}
		hlterm ** subs = new hlterm * [card];
		card = 0;
		for(xml_node<> * summandnode = mtnode -> first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			subs[card] = readPnmlTerm(summandnode);
			card++;
		}
		return new tuplehlmultiterm(s, card, subs);
	} 
	if(!std::strcmp(mtnode->name(),"subtract"))
	{
		hlmultiterm * result = readArcInscription(s,mtnode->first_node());
		for(xml_node<> * subtrahend = mtnode->first_node()->next_sibling(); subtrahend; subtrahend = subtrahend -> next_sibling())
		{
			result = new diffhlmultiterm(s, result, readArcInscription(s,subtrahend));
		}
		return result;
	}
	// arriving here means that none of the multiset operators matched.
	// That is: mtnode should be a term
	return new termhlmultiterm(s,readPnmlTerm(mtnode));
}

int skeletonReadArcInscription(sort *s, xml_node<> * mtnode)
{
	if(!std::strcmp(mtnode->name(),"add"))
	{
		int result = 0;
		for(xml_node<> * summandnode = mtnode -> first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			result += skeletonReadArcInscription(s, summandnode);
		}
		return result;
	} 
	if(!std::strcmp(mtnode->name(),"subterm"))
	{
		return skeletonReadArcInscription(s,mtnode->first_node());
	}
	if(!std::strcmp(mtnode->name(),"numberof"))
	{
		if(!mtnode->first_node("subterm") -> next_sibling("subterm")-> next_sibling())
		{
			return atoi(mtnode->first_node("subterm")->first_node("numberconstant")->first_attribute("value")->value()) * skeletonReadArcInscription(s, mtnode->last_node("subterm")->first_node());
		}
		int result = 0;
		xml_node<> * anchor = mtnode->first_node("subterm") -> next_sibling("subterm");
		for(xml_node<> * elementnode = anchor; elementnode;elementnode = elementnode -> next_sibling())
		{
			result += skeletonReadArcInscription(s,elementnode);
		}
		return atoi(mtnode->first_node("subterm")->first_node("numberconstant")->first_attribute("value")->value()) * result;
	}
	if(!std::strcmp(mtnode->name(),"all"))
	{
		sort * subsort = sort::lookup(mtnode -> first_node("usersort")->first_attribute("declaration")->value());
		if(!subsort)
		{
			RT::rep->status("arc inscription refers to unknwon sort in _all_ construct");
			RT::rep->abort(ERROR_SYNTAX);
		}
		if(subsort != s)
		{
			RT::rep->status("sort in _all_ construct of arc inscription does not match place sort");
			RT::rep->abort(ERROR_SYNTAX);
		}
		return s -> size;
	}
	if(!std::strcmp(mtnode->name(),"tuple"))
	{
		return 1;
	} 
	if(!std::strcmp(mtnode->name(),"subtract"))
	{
		int result = skeletonReadArcInscription(s,mtnode->first_node());
		for(xml_node<> * subtrahend = mtnode->first_node() -> next_sibling(); subtrahend; subtrahend = subtrahend -> next_sibling())
		{
			result -= skeletonReadArcInscription(s,subtrahend);
		}
		return result;
	}
	// arriving here means that none of the multiset operators matched.
	// That is: mtnode should be a term
	return 1;
}

char * pnmlstring;
xml_document<> * doc;
 
nestedunit * startunit;

ParserPTNet * ReadPnmlFile()
{
ParserPTNet* TheResult = new ParserPTNet();
RT::rep->status("reading pnml");
    // 1. convert file into this string
    pnmlstring = (char *) malloc(BUFFERSIZE);
    int pnmlstringsize = BUFFERSIZE;
	
    char  *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    while (1) {

        if (used + BUFFERSIZE + 1 > size) {
            size = used + BUFFERSIZE + 1;


            temp = (char *) realloc(data, size);
            if (temp == NULL) {
		RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
		RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
		RT::rep->abort(ERROR_FILE);
            }
            data = temp;
        }

        n = fread(data + used, 1, BUFFERSIZE, *RT::currentInputFile);
        if (n == 0)
            break;

        used += n;
    }

    if (ferror(*RT::currentInputFile)) {
		RT::rep->status("Read error when accessing %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
		RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
		RT::rep->abort(ERROR_FILE);
    }

    temp = (char *) realloc(data, used + 1);
    if (temp == NULL) {
		RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
		RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
		RT::rep->abort(ERROR_FILE);
    }
    data = temp;
    data[used] = '\0';

    pnmlstring = data;
    pnmlstringsize = used;

   // 2. run rapdidxml

	doc = new xml_document<>();    // character type defaults to char
	doc->parse<0>(pnmlstring);    // 0 means default parse flags

   // 3. Retrieve net type
	xml_node<> * netnode = doc->first_node()->first_node("net");
	char * nettype = netnode -> first_attribute("type")->value();
	if(strstr(nettype,"ptnet"))
	{
   // 4. Scan places and transitions of PT net
		RT::rep->status("PNML file contains place/transition net");
		for(xml_node<> * pagenode = netnode -> first_node("page"); pagenode; pagenode = pagenode -> next_sibling("page"))
		{
			for(xml_node<> * placenode = pagenode -> first_node("place"); placenode; placenode = placenode->next_sibling("place"))
			{
				char * pnmlplacename = placenode->first_attribute("id")->value();
				char * placename = (char *) malloc(strlen(pnmlplacename)+1);
				strcpy(placename,pnmlplacename);
				PlaceSymbol *p = new PlaceSymbol(placename, MAX_CAPACITY);
				if (UNLIKELY (! TheResult->PlaceTable->insert(p)))
				{
				    RT::rep->status("place '%s' name used twice", placename);
		RT::rep->abort(ERROR_FILE);
				}
				if(placenode->first_node("initialMarking"))
				{
					p-> addInitialMarking(atoi(placenode->first_node("initialMarking")->first_node("text")->value()));
				}
			}
			for(xml_node<> * transitionnode = pagenode->first_node("transition"); transitionnode; transitionnode = transitionnode->next_sibling("transition"))
			{
				char * pnmltransitionname = transitionnode->first_attribute("id")->value();
				char * transitionname = (char *) malloc(strlen(pnmltransitionname)+1);
				strcpy(transitionname,pnmltransitionname);
				TransitionSymbol * t = new TransitionSymbol(transitionname,NO_FAIRNESS,NULL,NULL);
				if (UNLIKELY (! TheResult->TransitionTable->insert(t)))
				{
				    RT::rep->status("transition '%s' name used twice", transitionname);
		RT::rep->abort(ERROR_FILE);
				}
			}
		}
    // 5. Scan arcs of PT net
		for(xml_node<> * pagenode = netnode->first_node("page"); pagenode; pagenode = pagenode->next_sibling("page"))
		{
			for(xml_node<> * arcnode = pagenode->first_node("arc"); arcnode; arcnode = arcnode->next_sibling("arc"))
			{
				char * sourcetext = arcnode -> first_attribute("source") -> value();
				char * targettext = arcnode -> first_attribute("target") -> value();
				mult_t weight = 1;
				if(arcnode -> first_node("inscription"))
				{
					weight = atoi(arcnode -> first_node("inscription")->first_node("text")->value());
				}
				

				PlaceSymbol * p = reinterpret_cast<PlaceSymbol*>(TheResult->PlaceTable->lookup(sourcetext));
				TransitionSymbol * t;
				if(p)
				{
					//arc from place to transition
					t = reinterpret_cast<TransitionSymbol*>(TheResult->TransitionTable->lookup(targettext));
					if(UNLIKELY(!t))
					{
						RT::rep->status("target transtion %s in arc %s does not exist", targettext,arcnode->first_attribute("id")->value());
						RT::rep->abort(ERROR_FILE);
					}
					ArcList * al = new ArcList(p,weight);
					al->setNext(t->Pre);
					t->Pre = al;
					t->cardPre++;
					p->notifyPost();
				}
				else
				{
					// arc from transition to place
					t = reinterpret_cast<TransitionSymbol*>(TheResult->TransitionTable->lookup(sourcetext));
                                        if(UNLIKELY(!t))
                                        {
                                                RT::rep->status("source node %s in arc %s does not exist", sourcetext,arcnode->first_attribute("id")->value());
                                                RT::rep->abort(ERROR_FILE);
                                        }
					p = reinterpret_cast<PlaceSymbol*>(TheResult->PlaceTable->lookup(targettext));
                                        if(UNLIKELY(!p))
                                        {
                                                RT::rep->status("target place %s in arc %s does not exist", targettext,arcnode->first_attribute("id")->value());
                                                RT::rep->abort(ERROR_FILE);
                                        }
					ArcList * al = new ArcList(p,weight);
					al->setNext(t->Post);
					t->Post = al;
					t->cardPost++;
					p->notifyPre();
				}

			}
       // 6. Parse nested unit information
			xml_node<> * toolspecific;
			for(toolspecific = pagenode->first_node("toolspecific"); toolspecific; toolspecific = toolspecific->next_sibling("toolspecific"))
			{
				if(! std::strcmp(toolspecific -> first_attribute("tool")->value(),"nupn")) break;
			}
			if(toolspecific)
			{
				// nested units only defined if net is safe
				RT::args.safe_given = true;
				RT::args.encoder_arg = encoder_arg_bit;
				for (PlaceSymbol * ps = reinterpret_cast<PlaceSymbol *>(TheResult->PlaceTable->first()); ps;
            					ps = reinterpret_cast<PlaceSymbol *>(TheResult->PlaceTable->next()))
    				{
					ps -> capacity = 1;
				}
				
				int unitindex = 0;
				// there is nupn information

				// 1. count units
				int cardunits = 0;
				for(xml_node<> * unit = toolspecific -> first_node("structure")->first_node("unit"); unit; unit = unit -> next_sibling("unit"))
                                {
					cardunits++;
				}
				nestedunit ** units = new nestedunit * [cardunits];
				memset(units,0, cardunits * sizeof(nestedunit *));
				nestedunit *** anchor = new nestedunit ** [cardunits];
				for(int i = 0; i < cardunits;i++)
				{
					anchor[i] = units + i;
				}
				for(xml_node<> * unit = toolspecific -> first_node("structure")->first_node("unit"); unit; unit = unit -> next_sibling("unit"))
				{
					char * places = unit -> first_node("places")->value();
					while(strlen(places) > 0)
					{
						if(*places <= ' ') 
						{
							places++;
							continue;
						}
						int i;
						for(i = 1; i < strlen(places);i++)
						{
							if(places[i] <= ' ' )
							{
								break;
							}
						}
						bool finished = false;
						if(i >= strlen(places)) finished = true;
						places[i] = '\0';
						PlaceSymbol* p = reinterpret_cast<PlaceSymbol*>(TheResult->PlaceTable->lookup(places));
						if(p) 
						{
							nestedunit * nu = new nestedunit();
							p -> nested = nu;
							*anchor[unitindex] = nu;
							anchor[unitindex] = &(nu -> nextinunit);
							nu -> nextinunit = NULL;
							nu -> firstinunit = units[unitindex];
							nu -> nextunit = NULL;
							nu -> name = unit->first_attribute("id")->value();
							nu -> leafunit = true;
							nu -> p = p;	
							nu -> marked = false;	
							//if(m0 < p -> capacity) p -> capacity = m0;
						}
						if(finished) break;
						places += i+1;
					}
					unitindex++;
				}

				// 3. evaluate subunit information

				unitindex = 0;
				for(xml_node<> * unit = toolspecific -> first_node("structure")->first_node("unit"); unit; unit = unit -> next_sibling("unit"))
				{
					char * subunits = unit -> first_node("subunits")->value();
                                        while(strlen(subunits) > 0)
                                        {
                                                if(*subunits <= ' ')
                                                {
                                                        subunits++;
                                                        continue;
                                                }
                                                int i;
                                                for(i = 1; i < strlen(subunits);i++)
                                                {
                                                        if(subunits[i] <= ' ' )
                                                        {
                                                                break;
                                                        }
                                                }
                                                bool finished = false;
                                                if(i >= strlen(subunits)) finished = true;
                                                subunits[i] = '\0';
						int otherunit;
						for(otherunit = 0; otherunit < cardunits; otherunit++)
						{
							if(units[otherunit] &&units[otherunit] && !strcmp(subunits, units[otherunit]->name)) break;
						}
	
						if(otherunit < cardunits)
						{
							// have found subunit 
							// --> link subunit to this unit
							*anchor[otherunit] = units[unitindex];
							for(nestedunit * nu = units[unitindex]; nu; nu = nu -> nextinunit)
							{
								nu -> leafunit = false;
							}
						}
                                                if(finished) break;
                                                subunits += i+1;
                                        }
					unitindex++;
				}
				int i = units[0] ? 0 : 1; // root unit may be empty
				units[i]->nextunit = NULL;
				for(i++; i < cardunits;i++)
				{
					if(!units[i]) continue;
					units[i]-> nextunit = units[i-1] ? units[i-1] : units[i-2];
					units[i]-> firstunit = units[cardunits-1] ? units[cardunits-1] : units[cardunits-2];
				}
				startunit = units[cardunits-1] ? units[cardunits -1 ] : units[cardunits-2];
				delete [] units;
				delete [] anchor;

/*
				// 4. Check State Machine Property

				for (TransitionSymbol * ts = reinterpret_cast<TransitionSymbol *>(TheResult->TransitionTable->first()); ts;
            					ts = reinterpret_cast<TransitionSymbol *>(TheResult->TransitionTable->next()))
    				{
					for (ArcList * al = ts->getPre(); al; al = al->getNext())
        				{
						PlaceSymbol * pl = al -> getPlace();
						nestedunit * nu = pl -> nestedunit;
						if(!nu) continue;
						for(ArcList * aal = ts -> getPre();aal;aal = aal -> getNext())
						{
							if(aal == al) continue;
							PlaceSymbol * ppl = aal -> getPlace();
							if(!ppl->nestedunit) continue;
							nestedunit * nnu = ppl -> nestedunit;
							if(!nnu) continue;
							for(nestedunit * n = nu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nnu) 
								{
									RT::rep->status("UNIT %s IS NOT STATE MACHINE: TR %s HAS PREPLACES %s and %s", nnu->name, ts -> getKey(), nu -> p -> getKey(), nnu -> p -> getKey() );
								}
							}
							for(nestedunit * n = nnu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nu) 
								{
									RT::rep->status("UNIT %s IS NOT STATE MACHINE: TR %s HAS PREPLACES %s and %s", nnu->name, ts -> getKey(), nu -> p -> getKey(), nnu -> p -> getKey() );
								}
							}
						}
						bool found = false;
						nestedunit * foundnu = NULL;
						for(ArcList * aal = ts -> getPost();aal;aal = aal -> getNext())
						{
							PlaceSymbol * ppl = aal -> getPlace();
							if(!ppl->nestedunit) continue;
							nestedunit * nnu = ppl -> nestedunit;
							if(!nnu) continue;
							for(nestedunit * n = nu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nnu) 
								{
									found = true;
									foundnu = n;
								}
							}
							for(nestedunit * n = nnu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == foundnu) continue;
								if(n == nu) 
								{
									foundnu = n;
									found = true;
								}
							}
						}
						if(!found)
						{
							RT::rep->status("UNIT %s: TR %s HAS PREPLACE %s BUT NO POSTPLACE", nu -> name, ts -> getKey(), pl -> getKey());
						}
					}
					for (ArcList * al = ts->getPost(); al; al = al->getNext())
        				{
						PlaceSymbol * pl = al -> getPlace();
						nestedunit * nu = pl -> nestedunit;
						if(!nu) continue;
						for(ArcList * aal = ts -> getPost();aal;aal = aal -> getNext())
						{
							if(aal == al) continue;
							PlaceSymbol * ppl = aal -> getPlace();
							if(!ppl->nestedunit) continue;
							nestedunit * nnu = ppl -> nestedunit;
							if(!nnu) continue;
							for(nestedunit * n = nu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nnu) 
								{
									RT::rep->status("UNIT %s IS NOT STATE MACHINE: TR %s HAS POSTPLACES %s AND %s", nnu->name, ts -> getKey(), pl -> getKey(), ppl->getKey());
								}
							}
							for(nestedunit * n = nnu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nu) 
								{
									RT::rep->status("UNIT %s IS NOT STATE MACHINE: TR %s HAS POSTPLACES %s AND %s", nnu->name, ts -> getKey(), pl -> getKey(), ppl->getKey());
								}
							}
						}
						bool found = false;
						nestedunit * foundnu = NULL;
						for(ArcList * aal = ts -> getPre();aal;aal = aal -> getNext())
						{
							PlaceSymbol * ppl = aal -> getPlace();
							if(!ppl->nestedunit) continue;
							nestedunit * nnu = ppl -> nestedunit;
							if(!nnu) continue;
							for(nestedunit * n = nu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == nnu) 
								{
									if(!found)
									{	
										found = true;
										foundnu = n;
									}
								}
							}
							for(nestedunit * n = nnu -> firstinunit; n; n = n -> nextinunit)
							{
								if(n == foundnu) continue;
								if(n == nu) 
								{
									if(!found)
									{
										foundnu = n;
										found = true;
									}
								}
							}
						}
						if(!found)
						{
							RT::rep->status("UNIT %s: TR %s HAS POSTPLACE %s BUT NO PREPLACE", nu -> name, ts -> getKey(), pl -> getKey());
						}
					}
				}
				for(nestedunit * uuu = startunit; uuu; uuu = uuu -> nextunit)
				{
					if(!uuu->leafunit) continue;
					int m0 = 0;
					for(nestedunit * nnn = uuu; nnn ; nnn = nnn -> nextinunit)
					{
						m0 += nnn -> p -> getInitialMarking();
					}
					if(m0 > 1)
					{
						RT::rep->status("UNIT %s HAS %d TOKENS", uuu -> name, m0);
					}
				}
				// print units
*/
				
			}
		}
		//delete doc;
		//delete data;
		free(pnmlstring);
	}
	else if(strstr(nettype,"symmetricnet"))
	{
		HighLevelNet = true;
		// create skeleton net
		int cardllplaces = 0;
		RT::rep->status("PNML file contains High-Level net");
// 7. read declaration
		sort::init();
		hlvariable::init();
		hlplace::init();
		constant::init();
		setconstant::init();
		hltransition::init();
		xml_node<> * declarationnode = netnode -> first_node("declaration")->first_node("structure") -> first_node("declarations");

		// need two passes for sorts since some bad guys do not care about order of declarations

		// first pass: collect elementary sorts
		for(xml_node<> * sortnode = declarationnode->first_node("namedsort"); sortnode; sortnode = sortnode->next_sibling("namedsort"))
		{
			if(!std::strcmp(sortnode ->first_node()->name(),"cyclicenumeration"))
			{
				// count elements
				xml_node<> * cynode = sortnode ->first_node();
				int card = 0;
				for(xml_node<> * elementnode = cynode->first_node("feconstant"); elementnode; elementnode = elementnode->next_sibling("feconstant"))
				{
					card++;
				}
				finitesort * csort = new finitesort(SO_CYC,card);
				card = 0;
				for(xml_node<> * elementnode = cynode->first_node("feconstant"); elementnode; elementnode = elementnode->next_sibling("feconstant"))
				{
					char * pnmltext = elementnode->first_attribute("id")->value();
					char * mytext = (char *) malloc(strlen(pnmltext)+1);
					strcpy(mytext,pnmltext);
					constant * c = new constant();
					c -> name = mytext;
					c -> so = csort;
					c -> index = card;
					constant::insert(c);
					csort -> symbol[card++] = c;
				}
				char * pnmlsortname = sortnode -> first_attribute("id")->value();
				csort -> name = (char *) malloc(strlen(pnmlsortname)+1);
				strcpy(csort->name,pnmlsortname);
				sort::insert(csort);
			}
			else if(!std::strcmp(sortnode ->first_node()->name(),"finiteintrange"))
			{
				// count elements
				xml_node<> * finode = sortnode ->first_node();
				int lower;
				int upper;
				sscanf(finode -> first_attribute("start")->value(),"%d",&lower);
				sscanf(finode -> first_attribute("end")->value(),"%d",&upper);
				int card = upper - lower + 1;
				finitesort * csort = new finitesort(SO_INT,card);
				char * pnmltext = new char[1000];
				char * pnmlsortname = sortnode -> first_attribute("id")->value();
RT::rep->status("L %d U %d C %d", lower, upper, card);
				for(int i = 0; i < card;i++)
				{
					sprintf(pnmltext,"%s_%d",pnmlsortname,lower + i);
RT::rep->status("INS %s", pnmltext);
					char * mytext = (char *) malloc(strlen(pnmltext)+1);
					strcpy(mytext,pnmltext);
					constant * c = new constant();
					c -> name = mytext;
					c -> so = csort;
					c -> index = i;
					constant::insert(c);
					csort -> symbol[i] = c;
				}
				delete [] pnmltext;
				csort -> name = (char *) malloc(strlen(pnmlsortname)+1);
				strcpy(csort->name,pnmlsortname);
				csort -> lower = lower;
				csort -> upper = upper;
				sort::insert(csort);
			}
			else if(!std::strcmp(sortnode ->first_node()->name(),"finiteenumeration"))
			{
				// count elements
				xml_node<> * cynode = sortnode ->first_node();
				int card = 0;
				for(xml_node<> * elementnode = cynode->first_node("feconstant"); elementnode; elementnode = elementnode->next_sibling("feconstant"))
				{
					card++;
				}
				finitesort * csort = new finitesort(SO_FIN,card);
				card = 0;
				for(xml_node<> * elementnode = cynode->first_node("feconstant"); elementnode; elementnode = elementnode->next_sibling("feconstant"))
				{
					char * pnmltext = elementnode->first_attribute("id")->value();
					char * mytext = (char *) malloc(strlen(pnmltext)+1);
					strcpy(mytext,pnmltext);
					constant * c = new constant();
					c -> name = mytext;
					c -> so = csort;
					c -> index = card;
					constant::insert(c);
					csort -> symbol[card++] = c;
				}
				char * pnmlsortname = sortnode -> first_attribute("id")->value();
				csort -> name = (char *) malloc(strlen(pnmlsortname)+1);
				strcpy(csort->name,pnmlsortname);
				sort::insert(csort);
			}
			else if(!std::strcmp(sortnode ->first_node()->name(),"dot"))
			{
				if(sort::dotsort) 
				{
					RT::rep->status("Dot sort defined twice");
					RT::rep->abort(ERROR_SYNTAX);
				}
				dotsort * dsort = new dotsort();
				char * pnmlsortname = sortnode -> first_attribute("id")->value();
				dsort -> name = (char *) malloc(strlen(pnmlsortname)+1);
				strcpy(dsort->name,pnmlsortname);
				sort::insert(dsort);
				sort::dotsort = dsort;
				
			}
			else if(!std::strcmp(sortnode->first_node()->name(),"productsort"))
			{
				continue; // left to second pass
			}
			else
			{
				RT::rep->status("unknown sort specification");
				RT::rep->abort(ERROR_SYNTAX);
			}
		}

		// second pass: collect product sorts
		for(xml_node<> * sortnode = declarationnode->first_node("namedsort"); sortnode; sortnode = sortnode->next_sibling("namedsort"))
		{
			if(!std::strcmp(sortnode->first_node()->name(),"productsort"))
			{
				// count subsorts
				int card = 0;
				xml_node<> * productnode = sortnode->first_node();
				for(xml_node<> * subnode = productnode->first_node("usersort"); subnode; subnode = subnode->next_sibling("usersort"))
				{
					card++;
				}
				productsort * psort = new productsort(card);
				card = 0;
				for(xml_node<> * subnode = productnode->first_node("usersort"); subnode; subnode = subnode->next_sibling("usersort"))
				{
					sort * subsort = sort::lookup(subnode->first_attribute("declaration")->value());
					if(!subsort)
					{
						RT::rep->status("product sort refers to unknown sort");
						RT::rep->abort(ERROR_SYNTAX);
					}
					psort -> addSubSort(card,subsort);
					card++;
				}
				char * pnmlsortname = sortnode -> first_attribute("id")->value();
				psort -> name = (char *) malloc(strlen(pnmlsortname)+1);
				strcpy(psort->name,pnmlsortname);
				sort::insert(psort);
			}
		}

		// check for "partitions"

		for(xml_node<> * partitionnode = declarationnode -> first_node("partition"); partitionnode; partitionnode = partitionnode -> next_sibling("partition"))
		{
			char * sortname = partitionnode -> first_node("usersort") -> first_attribute("declaration") -> value();
			sort * s = sort::lookup(sortname);
			if(!s)
			{
				RT::rep->status("Partition refers to unknown sort");
				RT::rep->abort(ERROR_SYNTAX);
			}
			for(xml_node<> * setnode = partitionnode -> first_node("partitionelement"); setnode; setnode = setnode -> next_sibling("partitionelement"))
			{
				int card = 0;
				for(xml_node<> * elementnode = setnode -> first_node(); elementnode; elementnode = elementnode -> next_sibling())
				{
					card++;
				}
				setconstant * sc = new setconstant();
				char * setname = setnode -> first_attribute("id") -> value();
				if(setconstant::lookup(setname))
				{
					RT::rep->status("Partition element used twice");
					RT::rep->abort(ERROR_SYNTAX);
				}
				sc -> name = (char *) malloc(strlen(setname) + 1);
				sc -> so = s;
				strcpy(sc->name,setname);
				sc -> card = card;
				sc -> index = new int [s -> size];
				memset(sc -> index, 0, s -> size * sizeof(int));
				for(xml_node<> * elementnode = setnode -> first_node(); elementnode; elementnode = elementnode -> next_sibling())
				{
					int * subresult = evaluateInitial(s, elementnode);
					for(int i = 0; i < s -> size; i++)
					{
						sc -> index[i] += subresult[i];
					}
					delete [] subresult;
				}
				setconstant::insert(sc);
			}
		}
		for(xml_node<> * hlvariablenode = declarationnode->first_node("variabledecl"); hlvariablenode; hlvariablenode = hlvariablenode->next_sibling("variabledecl"))
		{
			hlvariable * v = new hlvariable();
			char * pnmlvname = hlvariablenode -> first_attribute("id")->value();
			v -> name = (char *) malloc(strlen(pnmlvname)+1);
			strcpy(v->name,pnmlvname);
			sort * s = sort::lookup(hlvariablenode->first_node("usersort")->first_attribute("declaration") -> value());
			if(!s)
			{
				RT::rep->status("variable refers to unknwon sort");
				RT::rep->abort(ERROR_SYNTAX);
			}
			v -> so = s;
			v -> place = NULL;
			hlvariable::insert(v);
		}
		unfoldcard = 0;
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlvariable::hash_table[i]) continue;
			if(hlvariable::hash_table[i]->so->tag == SO_PRO) continue; // do not count tuple variables
										// since we have inserted their components
										// as separate variables
			unfoldcard++;
		}
		unfoldlist = new hlvariable * [unfoldcard];
		int q = 0;
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlvariable::hash_table[i]) continue;
			if(hlvariable::hash_table[i]->so->tag == SO_PRO) continue; // do not count tuple variables
										// since we have inserted their components
										// as separate variables
			unfoldlist[q++] = hlvariable::hash_table[i];
		}
		for(xml_node<> * pagenode = netnode -> first_node("page"); pagenode; pagenode = pagenode->next_sibling("page"))
		{
// 8. read HL places and initial marking
			for(xml_node<> * placenode = pagenode->first_node("place"); placenode; placenode = placenode->next_sibling("place"))
			{
				char * pnmlplacename = placenode->first_attribute("id")->value();
				if(hlplace::lookup(pnmlplacename))
				{
					RT::rep->status("HL place name used twice");
					RT::rep->abort(ERROR_SYNTAX);
				}
				hlplace * p = new hlplace();
				p -> name = (char *) malloc(strlen(pnmlplacename)+1);
				strcpy(p -> name, pnmlplacename);
				hlplace::insert(p);
				p -> so = sort::lookup(placenode->first_node("type")->first_node("structure")->first_node("usersort")->first_attribute("declaration")->value());
				if(!p->so)
				{
					RT::rep->status("HL place has unknown sort");
					RT::rep->abort(ERROR_SYNTAX);
				}
				cardllplaces += p->so->size;
				xml_node<> * initialnode = placenode->first_node("hlinitialMarking");	
				if(initialnode)
				{
					p -> skeletonInitialmarking = skeletonEvaluateInitial(p->so, initialnode->first_node("structure")->first_node());

					if(p -> skeletonInitialmarking > 1 && RT::args.check_arg == check_arg_OneSafe)
					{
						RT::rep->status(RT::rep->markup(MARKUP_BAD,"The net is not safe").str()); 
						RT::rep->status("Example for none-safe place: %s", p -> name);
						portfoliomanager::compareresult(false);
						if(RT::args.mcc_given)
						{       
							portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_FALSE);
						}
						_exit(1);
					}
				}
				else
				{
					p -> skeletonInitialmarking = 0;
				}
			}
// 9. read HL transitions
			for(xml_node<> * transitionnode = pagenode -> first_node("transition"); transitionnode; transitionnode = transitionnode -> next_sibling("transition"))
			{
				hltransition * t = new hltransition();
				char * pnmltransitionname = transitionnode->first_attribute("id")->value();
				t->name = (char *) malloc(strlen(pnmltransitionname) +1 );
				strcpy(t->name,pnmltransitionname);
				xml_node<> * conditionnode = transitionnode->first_node("condition");
				if(conditionnode)
				{
					t -> guard = readTransitionCondition(conditionnode->first_node("structure")->first_node());
				}
				else
				{
					t -> guard = new nocondition();
				}
				t -> pre = t -> post = NULL;
				if(hltransition::lookup(t->name))
				{
					RT::rep->status("transition name used twice");
					RT::rep->abort(ERROR_SYNTAX);
				}
				hltransition::insert(t);
			}
// 10. read HL arcs
			for(xml_node<> * arcnode = pagenode -> first_node("arc"); arcnode; arcnode = arcnode->next_sibling("arc"))
			{
				bool ispre; // is pre arc wrt transition
				hltransition * t;
				hlplace * p = hlplace::lookup(arcnode -> first_attribute("source")->value());
				if(p)
				{
					ispre = true; // from place to transition
					t = hltransition::lookup(arcnode->first_attribute("target")->value());
					if(!t)
					{
						RT::rep->status("arc has unknown target transition");
						RT::rep->abort(ERROR_SYNTAX);
					}
				}
				else
				{
					ispre = false;
					t = hltransition::lookup(arcnode->first_attribute("source")->value());
                                        if(!t)
                                        {
                                                RT::rep->status("arc has unknown source node");
                                                RT::rep->abort(ERROR_SYNTAX);
                                        }
					p = hlplace::lookup(arcnode->first_attribute("target")->value());
                                        if(!p)
                                        {
                                                RT::rep->status("arc has unknown target place");
                                                RT::rep->abort(ERROR_SYNTAX);
                                        }
				}
				hlarc * a = new hlarc();
				a -> pre = ispre;
				a -> place = p;
				a -> transition = t;
				a -> inscription = readArcInscription(p->so, arcnode->first_node("hlinscription")->first_node("structure")->first_node());
				a -> skeletonMult = skeletonReadArcInscription(p->so, arcnode->first_node("hlinscription")->first_node("structure")->first_node());
				if(a->pre)
				{
					a -> next = t -> pre;
					t -> pre = a;
				}
				else
				{
					a -> next = t -> post;
					t -> post = a;
				}
			}
			
		}

// 12. Actual skeleton generation

		ThePlaceSymbols = new PlaceSymbol * [hlplace::card];
		TheTransitionSymbols = new TransitionSymbol * [hltransition::card];
		int j = 0;
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlplace::hash_table[i]) continue;
			hlplace * p = hlplace::hash_table[i];
			PlaceSymbol * ps = new PlaceSymbol(p->name,MAX_CAPACITY);
			ps->addInitialMarking(p->skeletonInitialmarking);
			TheResult->PlaceTable->insert(ps);
			ThePlaceSymbols[j] = ps;
			p -> offset = j++;
		}
		j = 0;
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hltransition::hash_table[i]) continue;
			hltransition * t = hltransition::hash_table[i];
			ArcList * llpre = NULL;
			for(hlarc * a = t -> pre; a; a = a -> next)
			{
				ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset],a -> skeletonMult);
				al -> setNext( llpre);
				llpre = al;
			}
			ArcList * llpost = NULL;
			for(hlarc * a = t -> post; a; a = a -> next)
			{
				ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset],a->skeletonMult);
				al -> setNext( llpost);
				llpost = al;
			}
			TheTransitionSymbols[j] = new TransitionSymbol(t->name,NO_FAIRNESS,llpre,llpost);
			t -> offset = j;
			TheResult -> TransitionTable->insert(TheTransitionSymbols[j++]);
			
		}
	}
	else
	{
		RT::rep->status("PNML file contains unknown net type");
		RT::rep->abort(ERROR_SYNTAX);
		free(pnmlstring);
	}
	return TheResult;
}

ParserPTNet * unfoldHLNetStableMarking()
{
	ParserPTNet* TheResult = new ParserPTNet();
	int cardllplaces = 0;

	xml_node<> * netnode = doc->first_node()->first_node("net");
	for(xml_node<> * pagenode = netnode -> first_node("page"); pagenode; pagenode = pagenode->next_sibling("page"))
	{
// 8. Unfold initial marking
		for(xml_node<> * placenode = pagenode->first_node("place"); placenode; placenode = placenode->next_sibling("place"))
		{
			char * pnmlplacename = placenode->first_attribute("id")->value();
			hlplace * p = hlplace::lookup(pnmlplacename);
			cardllplaces += p -> so -> size;
			xml_node<> * initialnode = placenode->first_node("hlinitialMarking");	
			if(initialnode)
			{
				p -> initialmarking = evaluateInitial(p->so, initialnode->first_node("structure")->first_node());
			}
			else
			{
				p -> initialmarking = NULL;
			}
		}
	}
// Detect constant places
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		for(hlarc * a = t -> post; a; a = a -> next)
		{
			bool found = false;
			if(! (a -> place -> constantplace)) 
			{
				continue;
			}

			for(hlarc ** aa = &(t -> pre); *aa; aa = &((*aa) -> next))
			{
				if(a -> place != (*aa)->place) continue;
				if(a -> inscription -> nroftokens == ((*aa) -> inscription)->nroftokens)
				{
					found = true;
					break;
				}
			}
			if(!found)
			{
				a -> place -> constantplace = false;
			}
		}
		for(hlarc * a = t -> pre; a; a = a -> next)
		{
			bool found = false;
			if(! (a -> place -> constantplace)) 
			{
				continue;
			}

			for(hlarc ** aa = &(t -> post); *aa; aa = &((*aa) -> next))
			{
				if(a -> place != (*aa)->place) continue;
				if(a -> inscription -> nroftokens == ((*aa) -> inscription)->nroftokens)
				{
					found = true;
					break;
				}
			}
			if(!found)
			{
				a -> place -> constantplace = false;
			}
		}
	}
	// If we have a constant place, existence of stable marking is already proven

	bool haveconstant = false;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hlplace::hash_table[i]) continue;
		if(hlplace::hash_table[i]->constantplace)
		{
			haveconstant = true;
RT::rep->status("Place %s is stable place", hlplace::hash_table[i]->name);
			break;
		}
	}
	if(haveconstant)
	{
		RT::rep->status(RT::rep->markup(MARKUP_GOOD,"The net has a stable place").str());
		if(RT::args.mcc_given)
		{
			portfoliomanager::mcc_boolean(ReadPnmlFormulaId(), TERNARY_TRUE);
		}
		_exit(0);

	}

// Detect run places
// may still have effect since places without pre-trans may be runplace but not constant
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		hlarc * matched = NULL;
		for(hlarc * a = t -> post; a; a = a -> next)
		{
			bool found = false;
			if(! (a -> place -> runplace)) 
			{
				continue;
			}

			for(hlarc ** aa = &(t -> pre); *aa; aa = &((*aa) -> next))
			{
				if(a -> place != (*aa)->place) continue;
				if(a -> inscription -> equivalent((*aa) -> inscription))
				{
					found = true;
					hlarc * tmp = *aa;
					*aa = (*aa) -> next;
					tmp -> next = matched;
					matched = tmp;
					break;
				}
			}
			if(!found)
			{
				a -> place -> runplace = false;
			}
		}
		while(matched)
		{
			hlarc * tmp = matched;
			matched = matched -> next;
			tmp -> next = t -> pre;
			t -> pre = tmp;
		}
	}
// 11. determine transition bindungs
	int cardlltransitions = 0;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		condition * runplaceguard = NULL;
		for(hlarc * a = t -> post; a; a = a -> next)
		{
			if(a -> place -> runplace && a -> inscription -> matchable)
			{
				runplacecondition * rc = new runplacecondition();
				rc -> p = a -> place;
				rc -> inscription = a -> inscription;
				if(runplaceguard)
				{
					andcondition * ac = new andcondition();
					ac -> cardsub = 2;
					ac -> sub = new condition * [2];
					ac -> sub[0] = runplaceguard;
					ac -> sub[1] = rc;
					runplaceguard = ac;
				}
				else
				{
					runplaceguard = rc;
				}
			}
		}
		if(runplaceguard)
		{
RT::rep->status("HAVE RUN");
			andcondition * ac = new andcondition();
			ac -> cardsub = 2;
			ac -> sub = new condition *[2];
			ac -> sub[0] = t -> guard;
			ac -> sub[1] = runplaceguard;
			t -> guard = ac;
		}
		for(int i = 0; i < unfoldcard; i++)
		{
			unfoldlist[i]->offset = i;
		}
		t -> firingmodes = t -> guard -> evaluate(unfoldlist, unfoldcard);
		for(int i = 0; i < unfoldcard; i++)
		{
			unfoldlist[i]->offset = i;
		}
		cardlltransitions += t -> cardfiringmodes = t -> firingmodes -> count(unfoldcard, unfoldlist, t->getvars(unfoldlist, unfoldcard));
	}

RT::rep->status("Places: %d, Transitions: %d",cardllplaces,cardlltransitions);
	if(cardllplaces + cardlltransitions > RT::args.unfoldingnodelimit_arg)
	{
		RT::rep->status("Place/transition net would exceed node limit");
		RT::rep->abort(ERROR_SYNTAX);
	}
// 12. Actual unfolding
	//delete doc;
	//delete data;
	TheHLPlaceSymbols = ThePlaceSymbols;
	ThePlaceSymbols = new PlaceSymbol * [cardllplaces];
	TheHLTransitionSymbols = TheTransitionSymbols;
	TheTransitionSymbols = new TransitionSymbol * [cardlltransitions];
	int hlindex = 0;
	int offset = 0;
	int pp = cardllplaces;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hlplace::hash_table[i]) continue;
		hlplace * p = hlplace::hash_table[i];
		p -> index = hlindex++;
		if(RT::args.longnames_given)
		{
			p -> offset = offset;
			for(int j = 0; j < p -> so -> size; j++)
			{
				std::string suffix = p -> so->value2name(j);
				std::string name = std::string(p->name) + "." + suffix;
				char * cplacename = (char *) malloc(name.length()+1);
				strcpy(cplacename,name.c_str());
				PlaceSymbol * ps = new PlaceSymbol(cplacename,MAX_CAPACITY);
				if(p->initialmarking) ps->addInitialMarking(p->initialmarking[j]);
				if(UNLIKELY(!TheResult->PlaceTable->insert(ps)))
				{
				    RT::rep->status("place '%s' name used twice", cplacename);
		RT::rep->abort(ERROR_FILE);
				}
				ThePlaceSymbols[offset+j] = ps;
			}
			offset += p -> so -> size;
		}
		else
		{
			p -> offset = offset;
			for(int j = 0; j < p -> so -> size; j++)
			{
				char * cplacename = (char *) malloc(50 * sizeof(char));
				sprintf(cplacename,"p%d",offset+j);
				PlaceSymbol * ps = new PlaceSymbol(cplacename,MAX_CAPACITY);
				if(p->initialmarking) ps->addInitialMarking(p->initialmarking[j]);
				if(UNLIKELY(!TheResult->PlaceTable->insert(ps)))
				{
				    RT::rep->status("place '%s' name used twice", cplacename);
		RT::rep->abort(ERROR_FILE);
				}
				ThePlaceSymbols[offset+j] = ps;
			}
			offset+= p -> so -> size;
		}
	}
	arrayindex_t i = 0;
	for(Symbol * sy = TheResult -> PlaceTable -> first(); sy ; sy = TheResult -> PlaceTable -> next())
	{
		sy -> setIndex(i++);
		
	}
	offset = 0;
	hlindex = 0;
	int current = 0;
	int tt = cardlltransitions;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		t -> index = hlindex++;
		t -> offset = offset;
RT::rep->status("          @ trans %s", t -> name);
		for(int i = 0; i < unfoldcard;i++)
		{
			unfoldlist[i]-> offset = i;
		}
		int * ass = t ->firingmodes -> first_assignment(unfoldcard, unfoldlist, t -> getvars(unfoldlist,unfoldcard));
		int k = 0;
		do
		{
			ArcList * llpre = NULL;
			for(hlarc * a = t -> pre; a; a = a -> next)
			{
				int * val = a -> inscription -> evaluate(ass,unfoldlist,unfoldcard);
				for(int j = 0; j < a -> place -> so -> size; j++)
				{
					if(!val[j]) continue;
					ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset + j],val[j]);
					al -> setNext( llpre);
					llpre = al;
				}
				delete [] val;
			}
			ArcList * llpost = NULL;
			for(hlarc * a = t -> post; a; a = a -> next)
			{
				int * val = a -> inscription -> evaluate(ass,unfoldlist,unfoldcard);
				for(int j = 0; j < a -> place -> so -> size; j++)
				{
					if(!val[j]) continue;
					ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset + j],val[j]);
					al -> setNext( llpost);
					llpost = al;
				}
				delete [] val;
			}
			char * tcname;
			if(RT::args.longnames_given)
			{
				t -> vars = t -> getvars(unfoldlist,unfoldcard);
				std::string tname = std::string(t -> name) + std::string(".");
				for(int j = 0; j < unfoldcard;j++)
				{
					if(!t->vars[j]) continue;
					tname += std::string(unfoldlist[j]->name) + std::string("=") + std::string(unfoldlist[j]->so->value2name(ass[j])) + std::string("_");
				}
				tcname = (char *) malloc(tname.length()+1);
				strcpy(tcname,tname.c_str());
				tcname[strlen(tcname) -1] = '\0';
			}
			else
			{
				tcname = (char *) malloc(50*sizeof(char));
				sprintf(tcname,"t%u",offset+k);
			}
			TheTransitionSymbols[offset+k] = new TransitionSymbol(tcname,NO_FAIRNESS,llpre,llpost);
			
			TheTransitionSymbols[offset+k]->origin = t;
			if(UNLIKELY(!TheResult -> TransitionTable->insert(TheTransitionSymbols[offset+k++])))
			{
			    RT::rep->status("transition '%s' name used twice", tcname);
	RT::rep->abort(ERROR_FILE);
			}
			ass = t -> firingmodes -> next_assignment();
		}
		while(ass);
		delete t -> firingmodes;
		offset += t -> cardfiringmodes;
	}
RT::rep->status("finished unfolding");
	i = 0;
	for(Symbol * sy = TheResult -> TransitionTable -> first(); sy ; sy = TheResult -> TransitionTable -> next())
	{
		sy -> setIndex(i++);
		
	}
	free(pnmlstring);
	return TheResult;
}

ParserPTNet * unfoldHLNet()
{
	ParserPTNet* TheResult = new ParserPTNet();
	int cardllplaces = 0;

	xml_node<> * netnode = doc->first_node()->first_node("net");
	for(xml_node<> * pagenode = netnode -> first_node("page"); pagenode; pagenode = pagenode->next_sibling("page"))
	{
// 8. Unfold initial marking
		for(xml_node<> * placenode = pagenode->first_node("place"); placenode; placenode = placenode->next_sibling("place"))
		{
			char * pnmlplacename = placenode->first_attribute("id")->value();
			hlplace * p = hlplace::lookup(pnmlplacename);
			cardllplaces += p -> so -> size;
			xml_node<> * initialnode = placenode->first_node("hlinitialMarking");	
			if(initialnode)
			{
				p -> initialmarking = evaluateInitial(p->so, initialnode->first_node("structure")->first_node());
			}
			else
			{
				p -> initialmarking = NULL;
			}
		}
	}
// Detect run places
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		hlarc * matched = NULL;
		for(hlarc * a = t -> post; a; a = a -> next)
		{
			bool found = false;
			if(! (a -> place -> runplace)) 
			{
				continue;
			}

			for(hlarc ** aa = &(t -> pre); *aa; aa = &((*aa) -> next))
			{
				if(a -> place != (*aa)->place) continue;
				if(a -> inscription -> equivalent((*aa) -> inscription))
				{
					found = true;
					hlarc * tmp = *aa;
					*aa = (*aa) -> next;
					tmp -> next = matched;
					matched = tmp;
					break;
				}
			}
			if(!found)
			{
				a -> place -> runplace = false;
			}
		}
		while(matched)
		{
			hlarc * tmp = matched;
			matched = matched -> next;
			tmp -> next = t -> pre;
			t -> pre = tmp;
		}
	}

RT::rep->status("TR BINDINGS");
// 11. determine transition bindungs
	int cardlltransitions = 0;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		condition * runplaceguard = NULL;
		for(hlarc * a = t -> post; a; a = a -> next)
		{
			if(a -> place -> runplace && a -> inscription -> matchable)
			{
				runplacecondition * rc = new runplacecondition();
				rc -> p = a -> place;
				rc -> inscription = a -> inscription;
				if(runplaceguard)
				{
					andcondition * ac = new andcondition();
					ac -> cardsub = 2;
					ac -> sub = new condition * [2];
					ac -> sub[0] = runplaceguard;
					ac -> sub[1] = rc;
					runplaceguard = ac;
				}
				else
				{
					runplaceguard = rc;
				}
			}
		}
		if(runplaceguard)
		{
			andcondition * ac = new andcondition();
			ac -> cardsub = 2;
			ac -> sub = new condition *[2];
			ac -> sub[0] = t -> guard;
			ac -> sub[1] = runplaceguard;
			t -> guard = ac;
		}
		for(int i = 0; i < unfoldcard; i++)
		{
			unfoldlist[i]->offset = i;
		}
		t -> firingmodes = t -> guard -> evaluate(unfoldlist,unfoldcard);
		for(int i = 0; i < unfoldcard; i++)
		{
			unfoldlist[i]->offset = i;
		}
		cardlltransitions += t -> cardfiringmodes = t -> firingmodes -> count(unfoldcard, unfoldlist, t->getvars(unfoldlist,unfoldcard));
	}

RT::rep->status("TR BINDINGS DONE");
RT::rep->status("Places: %d, Transitions: %d",cardllplaces,cardlltransitions);
	if(cardllplaces + cardlltransitions > RT::args.unfoldingnodelimit_arg)
	{
		RT::rep->status("Place/transition net would exceed node limit");
		RT::rep->abort(ERROR_SYNTAX);
	}
// 12. Actual unfolding
	//delete doc;
	//delete data;
	TheHLPlaceSymbols = ThePlaceSymbols;
	ThePlaceSymbols = new PlaceSymbol * [cardllplaces];
	TheHLTransitionSymbols = TheTransitionSymbols;
	TheTransitionSymbols = new TransitionSymbol * [cardlltransitions];
	int hlindex = 0;
	int offset = 0;
	int cardhlplaces = 0;
	int pp = cardllplaces;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hlplace::hash_table[i]) continue;
		hlplace * p = hlplace::hash_table[i];
		p -> index = hlindex++;
		if(RT::args.longnames_given)
		{
			p -> offset = offset;
			for(int j = 0; j < p -> so -> size; j++)
			{
				std::string suffix = p -> so->value2name(j);
				std::string name = std::string(p->name) + "." + suffix;
				char * cplacename = (char *) malloc(name.length()+1);
				strcpy(cplacename,name.c_str());
				PlaceSymbol * ps = new PlaceSymbol(cplacename,MAX_CAPACITY);
				ps -> origin = p;
				if(p->initialmarking) ps->addInitialMarking(p->initialmarking[j]);
				if(UNLIKELY(!TheResult->PlaceTable->insert(ps)))
				{
				    RT::rep->status("place '%s' name used twice", cplacename);
		RT::rep->abort(ERROR_FILE);
				}
				ThePlaceSymbols[offset+j] = ps;
				ps -> setIndex(offset+j);
			}
			offset += p -> so -> size;
		}
		else
		{
			p -> offset = offset;
			for(int j = 0; j < p -> so -> size; j++)
			{
				char * cplacename = (char *) malloc(50 * sizeof(char));
				sprintf(cplacename,"p%d",offset+j);
				PlaceSymbol * ps = new PlaceSymbol(cplacename,MAX_CAPACITY);
				ps -> origin = p;
				if(p->initialmarking) ps->addInitialMarking(p->initialmarking[j]);
				if(UNLIKELY(!TheResult->PlaceTable->insert(ps)))
				{
				    RT::rep->status("place '%s' name used twice", cplacename);
		RT::rep->abort(ERROR_FILE);
				}
				ThePlaceSymbols[offset+j] = ps;
				ps -> setIndex(offset+j);
			}
			offset+= p -> so -> size;
		}
		cardhlplaces++;
	}
	arrayindex_t i = 0;
	offset = 0;
	hlindex = 0;
	int current = 0;
	int tt = cardlltransitions;
	for(int i = 0; i < HLHASHTABLESIZE;i++)
	{
		if(!hltransition::hash_table[i]) continue;
		hltransition * t = hltransition::hash_table[i];
		t -> offset = offset;
		t -> index = hlindex++;
RT::rep->status("          @ trans %s", t -> name);
		for(int i = 0; i < unfoldcard;i++)
		{
			unfoldlist[i]-> offset = i;
		}
		int * ass = t ->firingmodes -> first_assignment(unfoldcard, unfoldlist, t -> getvars(unfoldlist,unfoldcard));
		int k = 0;
		do
		{
			ArcList * llpre = NULL;
			for(hlarc * a = t -> pre; a; a = a -> next)
			{
				int * val = a -> inscription -> evaluate(ass,unfoldlist,unfoldcard);
				for(int j = 0; j < a -> place -> so -> size; j++)
				{
					if(!val[j]) continue;
					ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset + j],val[j]);
					al -> setNext( llpre);
					llpre = al;
				}
				delete [] val;
			}
			ArcList * llpost = NULL;
			for(hlarc * a = t -> post; a; a = a -> next)
			{
				int * val = a -> inscription -> evaluate(ass,unfoldlist,unfoldcard);
				for(int j = 0; j < a -> place -> so -> size; j++)
				{
					if(!val[j]) continue;
					ArcList * al = new ArcList(ThePlaceSymbols[a -> place -> offset + j],val[j]);
					al -> setNext( llpost);
					llpost = al;
				}
				delete [] val;
			}
			char * tcname;
			if(RT::args.longnames_given)
			{
				t -> vars = t -> getvars(unfoldlist,unfoldcard);
				std::string tname = std::string(t -> name) + std::string(".");
				for(int j = 0; j < unfoldcard;j++)
				{
					if(!t->vars[j]) continue;
					tname += std::string(unfoldlist[j]->name) + std::string("=") + std::string(unfoldlist[j]->so->value2name(ass[j])) + std::string("_");
				}
				tcname = (char *) malloc(tname.length()+1);
				strcpy(tcname,tname.c_str());
				tcname[strlen(tcname) -1] = '\0';
			}
			else
			{
				tcname = (char *) malloc(50*sizeof(char));
				sprintf(tcname,"t%u",offset+k);
			}
			TheTransitionSymbols[offset+k] = new TransitionSymbol(tcname,NO_FAIRNESS,llpre,llpost);
			TheTransitionSymbols[offset+k] -> setIndex(offset + k);
			TheTransitionSymbols[offset+k] -> origin = t;
			
			if(UNLIKELY(!TheResult -> TransitionTable->insert(TheTransitionSymbols[offset+k++])))
			{
			    RT::rep->status("transition '%s' name used twice", tcname);
	RT::rep->abort(ERROR_FILE);
			}
			ass = t -> firingmodes -> next_assignment();
		}
		while(ass);
		delete t -> firingmodes;
		offset += t -> cardfiringmodes;
	}
	free(pnmlstring);
	return TheResult;
}

tTerm  readTerm(xml_node<> * pnmlnode)
{
	if(!std::strcmp(pnmlnode->name(), "integer-constant"))
	{
		tTerm te = Complex();
		te -> ttt = new Term();
		te -> ttt -> place = UINT32_MAX;
		te -> ttt -> mult = atoi(pnmlnode -> value());
		te -> ttt -> next = NULL;
		return te;
	}
	if(!std::strcmp(pnmlnode->name(), "tokens-count"))
	{
		tTerm  te = Complex();
		// init with 0 constant term to avoid returning NULL (if all places are in siphon)
		te -> ttt = new Term();
		te -> ttt -> next = NULL;
		te -> ttt -> mult = 0;
		te -> ttt -> place = UINT32_MAX;
		if(ThePlaceSymbols)
		{
			// HL net
			for(xml_node<> * placenode = pnmlnode->first_node("place"); placenode; placenode = placenode -> next_sibling("place"))
			{
				hlplace * p = hlplace::lookup(placenode-> value());
				if(!p)
				{
					RT::rep->status("unknown place: %s", placenode->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				for(int i = 0; i < p -> so -> size; i++)
				{
					if(ThePlaceSymbols[p->offset + i]->inEmptySiphon) continue;
					Term * newte = new Term();
					newte -> next = te -> ttt;
					newte -> place = ThePlaceSymbols[p->offset + i] ->getIndex();
					newte -> mult = 1;
					te -> ttt = newte;
				}
			}
		}
		else
		{
			// LL net
			for(xml_node<> * placenode = pnmlnode->first_node("place"); placenode; placenode = placenode -> next_sibling("place"))
			{
				Symbol * p = ParserPTNet::currentsymbols -> PlaceTable -> lookup(placenode-> value());
				if(!p)
				{
					RT::rep->status("unknown place: %s", placenode->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				if(p -> inEmptySiphon) continue;
				Term * newte = new Term();
				newte -> next = te -> ttt;
				newte -> place = p->getIndex();
				newte -> mult = 1;
				te -> ttt = newte;
			}
		}
		return te;
	}
	if(!std::strcmp(pnmlnode->name(),"integer-sum"))
	{
		tTerm  te = Complex();
		te -> ttt = NULL;
		for(xml_node<> * summandnode = pnmlnode->first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			tTerm  st = readTerm(summandnode);
			te -> ttt -> append(st -> ttt);
		}
		return te;
	}
	if(!std::strcmp(pnmlnode->name(),"integer-difference"))
	{
		tTerm  te = readTerm(pnmlnode->first_node());
		tTerm  t2 = readTerm(pnmlnode->last_node());
		t2 -> ttt -> multiply(-1);
		te -> ttt -> append(t2 -> ttt);
		return te;
	}
	RT::rep->status("unexpected PNML node: %s; expected term",pnmlnode->name());
	RT::rep->abort(ERROR_SYNTAX);
	
}

tTerm  readHLTerm(xml_node<> * pnmlnode)
{
	if(!std::strcmp(pnmlnode->name(), "integer-constant"))
	{
		tTerm te = Complex();
		te -> ttt = new Term();
		te -> ttt -> place = UINT32_MAX;
		te -> ttt -> mult = atoi(pnmlnode -> value());
		te -> ttt -> next = NULL;
		return te;
	}
	if(!std::strcmp(pnmlnode->name(), "tokens-count"))
	{
		tTerm  te = Complex();
		// init with 0 constant term to avoid returning NULL (if all places are in siphon)
		te -> ttt = new Term();
		te -> ttt -> next = NULL;
		te -> ttt -> mult = 0;
		te -> ttt -> place = UINT32_MAX;
		for(xml_node<> * placenode = pnmlnode->first_node("place"); placenode; placenode = placenode -> next_sibling("place"))
		{
			hlplace * p = hlplace::lookup(placenode-> value());
			if(!p)
			{
				RT::rep->status("unknown place: %s", placenode->value());
				RT::rep->abort(ERROR_SYNTAX);
			}
			Term * newte = new Term();
			newte -> next = te -> ttt;
			newte -> place = ThePlaceSymbols[p->offset] ->getIndex();
			newte -> sy = ThePlaceSymbols[p->offset];
			newte -> mult = 1;
			te -> ttt = newte;
		}
		return te;
	}
	if(!std::strcmp(pnmlnode->name(),"integer-sum"))
	{
		tTerm  te = Complex();
		te -> ttt = NULL;
		for(xml_node<> * summandnode = pnmlnode->first_node(); summandnode; summandnode = summandnode -> next_sibling())
		{
			tTerm  st = readHLTerm(summandnode);
			te -> ttt -> append(st -> ttt);
		}
		return te;
	}
	if(!std::strcmp(pnmlnode->name(),"integer-difference"))
	{
		tTerm  te = readHLTerm(pnmlnode->first_node());
		tTerm  t2 = readHLTerm(pnmlnode->last_node());
		t2 -> ttt -> multiply(-1);
		te -> ttt -> append(t2 -> ttt);
		return te;
	}
	RT::rep->status("unexpected PNML node: %s; expected term",pnmlnode->name());
	RT::rep->abort(ERROR_SYNTAX);
	
}

tShape ashape(shapetype x)
{
	switch(x)
	{
	case ST_COMP:  return AT_COMP;
	case ST_FIR: return AT_FIR;
	case ST_DL: return AT_DL;
	case ST_AND: return AT_AND;
	case ST_OR:  return AT_OR;
	case ST_TRUE:  return AT_TRUE;
	case ST_FALSE: return AT_FALSE;
	case ST_TEMP: return AT_TEMP;
	}
}

void readSubFormula(xml_node<> * pnmlnode, tStatePredicate * tempresult, StatePredicate ** atomicresult, shapetype * shaperesult)
{
	if(!std::strcmp(pnmlnode -> name(),"exists-path"))
	{
		readSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = ExPath(*tempresult);
			*shaperesult = ST_TEMP;
			*atomicresult = NULL;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = ExPath(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"all-paths"))
	{
		readSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = AllPath(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = AllPath(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"globally"))
	{
		readSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = Always(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = Always(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"finally"))
	{
		readSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);

		if(* tempresult) 
		{
			*tempresult = Eventually(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = Eventually(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"next"))
	{
		readSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = NextState(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = NextState(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"until"))
	{
		tStatePredicate  tempsp1;
		tAtomicProposition tempap1;
		tStatePredicate  tempsp2;
		tAtomicProposition tempap2;
		StatePredicate * c1;
		StatePredicate * c2;
		shapetype st1;
		shapetype st2;
		readSubFormula(pnmlnode->first_node("before")->first_node(),&tempsp1,&c1,shaperesult);
		st1 = * shaperesult;
		readSubFormula(pnmlnode->first_node("reach")->first_node(),&tempsp2,&c2,shaperesult);
		st2 = * shaperesult;
		if(c1) 
		{
			tempap1 = Elementary();
			tempap1 -> pred = c1;
			tempap1 -> shape = ashape(st1);
			tempsp1 = AtomicProposition(tempap1);
			tempsp1->formula = tempap1->pred;
		
		}
		if(c2) 
		{
			tempap2 = Elementary();
			tempap2 -> pred = c2;
			tempap2 -> shape = ashape(st2);
			tempsp2 = AtomicProposition(tempap2);
			tempsp2->formula = tempap2->pred;
		}
		* tempresult = Until(tempsp1,tempsp2);
		* atomicresult = NULL;
		* shaperesult = ST_TEMP;
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"integer-le"))
	{
		tTerm  T1 = readTerm(pnmlnode->first_node());
		tTerm  T2 = readTerm(pnmlnode->last_node());
		T2->ttt->multiply(-1);
		T1->ttt->append(T2->ttt);
		* atomicresult = new AtomicStatePredicate(Petrinet::InitialNet,T1->ttt);
		* shaperesult = ST_COMP;
		* tempresult = NULL;
		return;
	}
	if(!std::strcmp(pnmlnode->name(),"is-fireable"))
	{
		if(TheTransitionSymbols)
		{
			// HL net
			if(pnmlnode->first_node("transition") == pnmlnode->last_node("transition"))
			{
				hltransition * t = hltransition::lookup(pnmlnode->first_node("transition")->value());
				if(UNLIKELY(!t))
				{
					RT::rep->status("Transition %s does not exist.",pnmlnode->first_node("transition")->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				if(t -> cardfiringmodes == 1)
				{
					if(TheTransitionSymbols[t->offset]->inEmptySiphon)
					{
						*atomicresult = new FalsePredicate(Petrinet::InitialNet);
						*shaperesult = ST_FALSE;
						*tempresult = NULL;
						return;
					}
					*atomicresult = new FireablePredicate(Petrinet::InitialNet,TheTransitionSymbols[t->offset]->getIndex(),true);
					*shaperesult = ST_FIR;
					* tempresult = NULL;
					return;
				}
				else
				{
					AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
					for(int i = 0; i < t -> cardfiringmodes;i++)
					{
						if(TheTransitionSymbols[t->offset+i]->inEmptySiphon) continue;
						result -> addSub(new FireablePredicate(Petrinet::InitialNet,TheTransitionSymbols[t->offset+i]->getIndex(),true));
					}
					*atomicresult = result;
					*shaperesult = ST_OR;
					*tempresult = NULL;
					return ;
				}
			}
			AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
			for(xml_node<> * transitionnode = pnmlnode->first_node("transition"); transitionnode; transitionnode = transitionnode -> next_sibling("transition"))
			{
				hltransition * t = hltransition::lookup(transitionnode->value());
				if(UNLIKELY(!t))
				{
					RT::rep->status("Transition %s does not exist.",transitionnode->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				for(int i = 0; i < t -> cardfiringmodes;i++)
				{
					if(TheTransitionSymbols[t->offset+i]) continue;
					result -> addSub(new FireablePredicate(Petrinet::InitialNet,TheTransitionSymbols[t->offset+i]->getIndex(),true));
				}
			}
			*atomicresult = result;
			*shaperesult = ST_OR;
			*tempresult = NULL;
			return;
		}
		else	
		{
			// LL net
			if(pnmlnode->first_node("transition") == pnmlnode->last_node("transition"))
			{
				Symbol * t = ParserPTNet::currentsymbols -> TransitionTable->lookup(pnmlnode->first_node("transition")->value());
				if(UNLIKELY(!t))
				{
					RT::rep->status("Transition %s does not exist.",pnmlnode->first_node("transition")->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				if(t->inEmptySiphon)
				{
					*atomicresult = new FalsePredicate(Petrinet::InitialNet);
					*shaperesult = ST_FALSE;
					* tempresult = NULL;
					return;
				}
				*atomicresult = new FireablePredicate(Petrinet::InitialNet,t->getIndex(),true);
				*shaperesult = ST_FIR;
				*tempresult = NULL;
				return;
			}
			AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
			for(xml_node<> * transitionnode = pnmlnode->first_node("transition"); transitionnode; transitionnode = transitionnode -> next_sibling("transition"))
			{
				Symbol * t = ParserPTNet::currentsymbols->TransitionTable->lookup(transitionnode->value());
				if(UNLIKELY(!t))
				{
					RT::rep->status("Transition %s does not exist.",transitionnode->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				if(t -> inEmptySiphon) continue;
				result->addSub(new FireablePredicate(Petrinet::InitialNet,t->getIndex(),true));
			}
			*atomicresult = result;
			*shaperesult = ST_OR;
			*tempresult = NULL;
			return;
		}
	}
	if(!std::strcmp(pnmlnode->name(),"deadlock"))
	{
		*shaperesult = ST_DL;
		*atomicresult = new DeadlockPredicate(Petrinet::InitialNet,true);
		*tempresult = NULL;
		return;
	}
	if(!std::strcmp(pnmlnode->name(),"negation")||!std::strcmp(pnmlnode->name(),"not"))
	{
		tStatePredicate tempsub = NULL;
		shapetype sh;
		StatePredicate * sp = NULL;
		readSubFormula(pnmlnode->first_node(),&tempsub,&sp,&sh);
		if(sh == ST_TEMP)
		{
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
			*tempresult = Negation(tempsub);
			return;
		}
		switch(sh)
		{	
		case ST_DL:
		case ST_FIR:
		case ST_COMP:	
				(*atomicresult) = (sp)->negate();
				(*shaperesult) = sh;
				*tempresult = NULL;
				return ;
		case ST_TRUE:	*atomicresult = new FalsePredicate(Petrinet::InitialNet);
				*shaperesult = ST_FALSE;
				*tempresult = NULL;
				return;
		case ST_FALSE:	*atomicresult = new TruePredicate(Petrinet::InitialNet);
				*shaperesult = ST_TRUE;
				*tempresult = NULL;
				return;
		case ST_AND:	(*atomicresult) = (sp) -> negate();
				*shaperesult =  ST_OR;
				*tempresult = NULL;
				return;
		case ST_OR:	(*atomicresult) = (sp) -> negate();
				*shaperesult = ST_AND;
				*tempresult = NULL;
				return;
		case ST_TEMP:   assert(false);
		}
	}
	if(!std::strcmp(pnmlnode->name(),"conjunction") || !std::strcmp(pnmlnode->name(),"and"))
	{
		* atomicresult = NULL;
		* shaperesult = ST_TRUE;
		* tempresult = NULL;
		tStatePredicate tempaggr = NULL;
		StatePredicate * atomicaggr = NULL;
		shapetype shapeaggr = ST_TRUE;
		for(xml_node<> * subnode = pnmlnode -> first_node(); subnode;subnode = subnode -> next_sibling())
		{
			tStatePredicate subtemp = NULL;
			StatePredicate * subatomic = NULL;
			shapetype subsh;
			readSubFormula(subnode,&subtemp,&subatomic,&subsh);
			if(subsh == ST_FALSE)
			{
				shapeaggr = ST_FALSE;
				break; // cannot become true 
				
			}
			else if(shapeaggr == ST_TRUE)
			{
				tempaggr = subtemp;
				atomicaggr= subatomic;
				shapeaggr = subsh;
			}
			else if(subsh== ST_TRUE)
			{
				// nothing to be done
			}
			else if(shapeaggr == ST_TEMP || subsh == ST_TEMP)
			{
				if(shapeaggr != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = atomicaggr;
					a -> shape = ashape(shapeaggr);
					tempaggr = AtomicProposition(a);
					tempaggr->formula = a -> pred;
				}
				if(subsh != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = subatomic;
					a -> shape = ashape(subsh);
					subtemp = AtomicProposition(a);
					subtemp->formula = a -> pred;
				}
				tempaggr = Conjunction(tempaggr,subtemp);
				shapeaggr = ST_TEMP;
				atomicaggr = NULL;
			}
			else if((shapeaggr == ST_AND) && (subsh != ST_AND))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr) -> addSub(subatomic);
			}
			else if((shapeaggr != ST_AND) && (subsh == ST_AND))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(subatomic) -> addSub(atomicaggr);
				atomicaggr = subatomic;
				shapeaggr = ST_AND;
			}
			else if(shapeaggr == ST_AND && subsh == ST_AND)
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr)->merge(reinterpret_cast<AtomicBooleanPredicate *>(subatomic));
			}
			else
			{
				// none of the subformulas is conjunction
				AtomicBooleanPredicate * newresult = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
				newresult -> addSub(atomicaggr);
				newresult -> addSub(subatomic);
				atomicaggr = newresult;
				tempaggr = NULL;
				shapeaggr = ST_AND;
			}
		}
		if(shapeaggr == ST_TRUE) 
		{
			atomicaggr = new TruePredicate(Petrinet::InitialNet);
			tempaggr = NULL;
		}
		else if(shapeaggr == ST_FALSE) 
		{
			atomicaggr = new FalsePredicate(Petrinet::InitialNet);
			tempaggr = NULL;
		}
			* atomicresult = atomicaggr;
			* tempresult = tempaggr;
			* shaperesult = shapeaggr;
		return ;
	}
	if(!std::strcmp(pnmlnode->name(),"disjunction")||!std::strcmp(pnmlnode->name(),"or"))
	{
		*atomicresult = NULL;
		*tempresult = NULL;
		*shaperesult = ST_FALSE;
		tStatePredicate tempaggr = NULL;
		StatePredicate * atomicaggr = NULL;
		shapetype shapeaggr = ST_FALSE;
		for(xml_node<> * subnode = pnmlnode -> first_node(); subnode;subnode = subnode -> next_sibling())
		{
			tStatePredicate subtemp;
			StatePredicate * subatomic;
			shapetype subsh;
			readSubFormula(subnode,&subtemp,&subatomic,&subsh);
			if(subsh == ST_TRUE)
			{
				shapeaggr = ST_TRUE;
				break;
			}
			else if(shapeaggr == ST_FALSE)
			{
				atomicaggr = subatomic;
				tempaggr = subtemp;
				shapeaggr = subsh;
			}
			else if(subsh == ST_FALSE)
			{
			}
			else if(shapeaggr == ST_TEMP || subsh == ST_TEMP)
			{
				if(shapeaggr != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
		
					a -> pred = (atomicaggr);
					a -> shape = ashape(shapeaggr);
					tempaggr = AtomicProposition(a);
					tempaggr->formula = a->pred;
				}
				if(subsh != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = subatomic;
					a -> shape = ashape(subsh);
					subtemp = AtomicProposition(a);
					subtemp->formula = a -> pred;
				}
				(tempaggr) = Disjunction((tempaggr),subtemp);
				shapeaggr = ST_TEMP;
				atomicaggr = NULL;
			}
			else if((shapeaggr == ST_OR) && (subsh != ST_OR))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr) -> addSub(subatomic);
				
			}
			else if((subsh == ST_OR) && (shapeaggr != ST_OR))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(subatomic) -> addSub(atomicaggr);
				atomicaggr = subatomic;
				shapeaggr = ST_OR;
				
			}
			else if(shapeaggr == ST_OR && subsh == ST_OR)
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr)->merge(reinterpret_cast<AtomicBooleanPredicate *>(subatomic));
			}
			else
			{
				// none of the subformulas is disjunction
				AtomicBooleanPredicate * newresult = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
				newresult -> addSub(atomicaggr);
				newresult -> addSub(subatomic);
				atomicaggr = newresult;
				tempaggr = NULL;
				shapeaggr = ST_OR;
			}
		}
		if(shapeaggr == ST_TRUE) 
		{
			atomicaggr = new TruePredicate(Petrinet::InitialNet);
			tempaggr = NULL;
		}
		if(shapeaggr == ST_FALSE) 
		{
			atomicaggr = new FalsePredicate(Petrinet::InitialNet);
			tempaggr = NULL;
		}
		* tempresult = tempaggr;
		* atomicresult = atomicaggr;
		* shaperesult = shapeaggr;
		return ;
	}
	RT::rep->status("unexpected PNML element: %s",pnmlnode->name());
	RT::rep->abort(ERROR_SYNTAX);
}

void readHLSubFormula(xml_node<> * pnmlnode, tStatePredicate * tempresult, StatePredicate ** atomicresult, shapetype * shaperesult)
{
	if(!std::strcmp(pnmlnode -> name(),"exists-path"))
	{
		readHLSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = ExPath(*tempresult);
			*shaperesult = ST_TEMP;
			*atomicresult = NULL;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = ExPath(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"all-paths"))
	{
		readHLSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = AllPath(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = AllPath(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"globally"))
	{
		readHLSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = Always(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = Always(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"finally"))
	{
		readHLSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = Eventually(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = Eventually(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"next"))
	{
		readHLSubFormula(pnmlnode -> first_node(),tempresult,atomicresult,shaperesult);
		if(* tempresult) 
		{
			*tempresult = NextState(*tempresult);
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		else 
		{
			tAtomicProposition a = Elementary();
			a -> pred = * atomicresult;
			a -> shape = ashape(*shaperesult);
			*tempresult = NextState(AtomicProposition(a));
			(*tempresult) -> formula = a -> pred;
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
		}
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"until"))
	{
		tStatePredicate  tempsp1;
		tAtomicProposition tempap1;
		tStatePredicate  tempsp2;
		tAtomicProposition tempap2;
		StatePredicate * c1;
		StatePredicate * c2;
		shapetype st1;
		shapetype st2;
		readHLSubFormula(pnmlnode->first_node("before")->first_node(),&tempsp1,&c1,shaperesult);
		st1 = * shaperesult;
		readHLSubFormula(pnmlnode->first_node("reach")->first_node(),&tempsp2,&c2,shaperesult);
		st2 = * shaperesult;
		if(c1) 
		{
			tempap1 = Elementary();
			tempap1 -> pred = c1;
			tempap1 -> shape = ashape(st1);
			tempsp1 = AtomicProposition(tempap1);
			tempsp1->formula = tempap1->pred;
		
		}
		if(c2) 
		{
			tempap2 = Elementary();
			tempap2 -> pred = c2;
			tempap2 -> shape = ashape(st2);
			tempsp2 = AtomicProposition(tempap2);
			tempsp2->formula = tempap2->pred;
		}
		* tempresult = Until(tempsp1,tempsp2);
		* atomicresult = NULL;
		* shaperesult = ST_TEMP;
		return;
	}
	if(!std::strcmp(pnmlnode -> name(),"integer-le"))
	{
		tTerm  T1 = readHLTerm(pnmlnode->first_node());
		tTerm  T2 = readHLTerm(pnmlnode->last_node());
		T2->ttt->multiply(-1);
		T1->ttt->append(T2->ttt);
		* atomicresult = new AtomicStatePredicate(Petrinet::InitialNet,T1->ttt);
		* shaperesult = ST_COMP;
		* tempresult = NULL;
		return;
	}
	if(!std::strcmp(pnmlnode->name(),"is-fireable"))
	{
		if(pnmlnode->first_node("transition") == pnmlnode->last_node("transition"))
		{
			hltransition * t = hltransition::lookup(pnmlnode->first_node("transition")->value());
			if(UNLIKELY(!t))
			{
				RT::rep->status("Transition %s does not exist.",pnmlnode->first_node("transition")->value());
				RT::rep->abort(ERROR_SYNTAX);
			}
			*atomicresult = new FireablePredicate(Petrinet::InitialNet,TheTransitionSymbols[t->offset],true);
			*shaperesult = ST_FIR;
			* tempresult = NULL;
			return;
		}
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		for(xml_node<> * transitionnode = pnmlnode->first_node("transition"); transitionnode; transitionnode = transitionnode -> next_sibling("transition"))
		{
			hltransition * t = hltransition::lookup(transitionnode->value());
			if(UNLIKELY(!t))
			{
				RT::rep->status("Transition %s does not exist.",transitionnode->value());
				RT::rep->abort(ERROR_SYNTAX);
			}
				if(TheTransitionSymbols[t->offset]) continue;
				result -> addSub(new FireablePredicate(Petrinet::InitialNet,TheTransitionSymbols[t->offset]->getIndex(),true));
		}
		*atomicresult = result;
		*shaperesult = ST_OR;
		*tempresult = NULL;
		return;
	}
	if(!std::strcmp(pnmlnode->name(),"deadlock"))
	{
		*shaperesult = ST_DL;
		*atomicresult = new DeadlockPredicate(Petrinet::InitialNet,true);
		*tempresult = NULL;
		return;
	}
	if(!std::strcmp(pnmlnode->name(),"negation")||!std::strcmp(pnmlnode->name(),"not"))
	{
		tStatePredicate tempsub = NULL;
		shapetype sh;
		StatePredicate * sp = NULL;
		readHLSubFormula(pnmlnode->first_node(),&tempsub,&sp,&sh);
		if(sh == ST_TEMP)
		{
			*atomicresult = NULL;
			*shaperesult = ST_TEMP;
			*tempresult = Negation(tempsub);
			return;
		}
		switch(sh)
		{	
		case ST_DL:
		case ST_FIR:
		case ST_COMP:	
				(*atomicresult) = (sp)->negate();
				(*shaperesult) = sh;
				*tempresult = NULL;
				return ;
		case ST_TRUE:	*atomicresult = new FalsePredicate(Petrinet::InitialNet);
				*shaperesult = ST_FALSE;
				*tempresult = NULL;
				return;
		case ST_FALSE:	*atomicresult = new TruePredicate(Petrinet::InitialNet);
				*shaperesult = ST_TRUE;
				*tempresult = NULL;
				return;
		case ST_AND:	(*atomicresult) = (sp) -> negate();
				*shaperesult =  ST_OR;
				*tempresult = NULL;
				return;
		case ST_OR:	(*atomicresult) = (sp) -> negate();
				*shaperesult = ST_AND;
				*tempresult = NULL;
				return;
		case ST_TEMP:   assert(false);
		}
	}
	if(!std::strcmp(pnmlnode->name(),"conjunction") || !std::strcmp(pnmlnode->name(),"and"))
	{
		* atomicresult = NULL;
		* shaperesult = ST_TRUE;
		* tempresult = NULL;
		tStatePredicate tempaggr = NULL;
		StatePredicate * atomicaggr = NULL;
		shapetype shapeaggr = ST_TRUE;
		for(xml_node<> * subnode = pnmlnode -> first_node(); subnode;subnode = subnode -> next_sibling())
		{
			tStatePredicate subtemp = NULL;
			StatePredicate * subatomic = NULL;
			shapetype subsh;
			readHLSubFormula(subnode,&subtemp,&subatomic,&subsh);
			if(shapeaggr == ST_FALSE)
			{
				// nothing to be done
			}
			if(subsh == ST_FALSE)
			{
				shapeaggr = ST_FALSE;
				
			}
			else if(shapeaggr == ST_TRUE)
			{
				tempaggr = subtemp;
				atomicaggr= subatomic;
				shapeaggr = subsh;
			}
			else if(subsh== ST_TRUE)
			{
				// nothing to be done
			}
			else if(shapeaggr == ST_TEMP || subsh == ST_TEMP)
			{
				if(shapeaggr != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = atomicaggr;
					a -> shape = ashape(shapeaggr);
					tempaggr = AtomicProposition(a);
					tempaggr->formula = a -> pred;
				}
				if(subsh != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = subatomic;
					a -> shape = ashape(subsh);
					subtemp = AtomicProposition(a);
					subtemp->formula = a -> pred;
				}
				tempaggr = Conjunction(tempaggr,subtemp);
				shapeaggr = ST_TEMP;
				atomicaggr = NULL;
			}
			else if((shapeaggr == ST_AND) && (subsh != ST_AND))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr) -> addSub(subatomic);
			}
			else if((shapeaggr != ST_AND) && (subsh == ST_AND))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(subatomic) -> addSub(atomicaggr);
				atomicaggr = subatomic;
				shapeaggr = ST_AND;
			}
			else if(shapeaggr == ST_AND && subsh == ST_AND)
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr)->merge(reinterpret_cast<AtomicBooleanPredicate *>(subatomic));
			}
			else
			{
				// none of the subformulas is conjunction
				AtomicBooleanPredicate * newresult = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
				newresult -> addSub(atomicaggr);
				newresult -> addSub(subatomic);
				atomicaggr = newresult;
				tempaggr = NULL;
				shapeaggr = ST_AND;
			}
		}
		if(shapeaggr == ST_TRUE) atomicaggr = new TruePredicate(Petrinet::InitialNet);
		if(shapeaggr == ST_FALSE) atomicaggr = new FalsePredicate(Petrinet::InitialNet);
		* atomicresult = atomicaggr;
		* tempresult = tempaggr;
		* shaperesult = shapeaggr;
		return ;
	}
	if(!std::strcmp(pnmlnode->name(),"disjunction")||!std::strcmp(pnmlnode->name(),"or"))
	{
		*atomicresult = NULL;
		*tempresult = NULL;
		*shaperesult = ST_FALSE;
		tStatePredicate tempaggr = NULL;
		StatePredicate * atomicaggr = NULL;
		shapetype shapeaggr = ST_FALSE;
		for(xml_node<> * subnode = pnmlnode -> first_node(); subnode;subnode = subnode -> next_sibling())
		{
			tStatePredicate subtemp;
			StatePredicate * subatomic;
			shapetype subsh;
			readHLSubFormula(subnode,&subtemp,&subatomic,&subsh);
			if(shapeaggr == ST_TRUE)
			{
			}
			if(subsh == ST_TRUE)
			{
				shapeaggr = ST_TRUE;
			}
			else if(shapeaggr == ST_FALSE)
			{
				atomicaggr = subatomic;
				tempaggr = subtemp;
				shapeaggr = subsh;
			}
			else if(subsh == ST_FALSE)
			{
			}
			else if(shapeaggr == ST_TEMP || subsh == ST_TEMP)
			{
				if(shapeaggr != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
		
					a -> pred = (atomicaggr);
					a -> shape = ashape(shapeaggr);
					tempaggr = AtomicProposition(a);
					tempaggr->formula = a->pred;
				}
				if(subsh != ST_TEMP)
				{
					tAtomicProposition a = Elementary();
					a -> pred = subatomic;
					a -> shape = ashape(subsh);
					subtemp = AtomicProposition(a);
					subtemp->formula = a -> pred;
				}
				(tempaggr) = Disjunction((tempaggr),subtemp);
				shapeaggr = ST_TEMP;
				atomicaggr = NULL;
			}
			else if((shapeaggr == ST_OR) && (subsh != ST_OR))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr) -> addSub(subatomic);
				
			}
			else if((subsh == ST_OR) && (shapeaggr != ST_OR))
			{
				reinterpret_cast<AtomicBooleanPredicate *>(subatomic) -> addSub(atomicaggr);
				atomicaggr = subatomic;
				shapeaggr = ST_OR;
				
			}
			else if(shapeaggr == ST_OR && subsh == ST_OR)
			{
				reinterpret_cast<AtomicBooleanPredicate *>(atomicaggr)->merge(reinterpret_cast<AtomicBooleanPredicate *>(subatomic));
			}
			else
			{
				// none of the subformulas is disjunction
				AtomicBooleanPredicate * newresult = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
				newresult -> addSub(atomicaggr);
				newresult -> addSub(subatomic);
				atomicaggr = newresult;
				tempaggr = NULL;
				shapeaggr = ST_OR;
			}
		}
		if(shapeaggr == ST_TRUE) atomicaggr = new TruePredicate(Petrinet::InitialNet);
		if(shapeaggr == ST_FALSE) atomicaggr = new FalsePredicate(Petrinet::InitialNet);
		* tempresult = tempaggr;
		* atomicresult = atomicaggr;
		* shaperesult = shapeaggr;
		return ;
	}
	RT::rep->status("unexpected PNML element: %s",pnmlnode->name());
	RT::rep->abort(ERROR_SYNTAX);
}

char * xmlstring;
void ReadPnmlFormula()
{
RT::rep->status("reading XML formula");
RT::currentInputFile = new Input("formula", RT::args.formula_arg);
RT::inputFormulaFileName = RT::currentInputFile -> getFilename();
    // 1. convert file into this string
    xmlstring = (char *) malloc(BUFFERSIZE);
    int pnmlstringsize = BUFFERSIZE;

    char  *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    while (1) {

        if (used + BUFFERSIZE + 1 > size) {
            size = used + BUFFERSIZE + 1;


            temp = (char *) realloc(data, size);
            if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
            }
            data = temp;
        }

        n = fread(data + used, 1, BUFFERSIZE, *RT::currentInputFile);
        if (n == 0)
            break;

        used += n;
    }
  if (ferror(*RT::currentInputFile)) {
                RT::rep->status("Read error when accessing %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }

    temp = (char *) realloc(data, used + 1);
    if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }
    data = temp;
    data[used] = '\0';

    xmlstring = data;
    pnmlstringsize = used;

   // 2. run rapdidxml

        using namespace rapidxml;
        xml_document<> doc;    // character type defaults to char
        doc.parse<0>(xmlstring);    // 0 means default parse flags
 	xml_node<> * propertysetnode = doc.first_node();
RT::formula_id = (char **) calloc(sizeof(char *),1);
int card_formula = 0;
	for(xml_node<> * propertynode = propertysetnode->first_node("property"); propertynode; propertynode = propertynode->next_sibling("property"))
	{
		char * propertyid = propertynode->first_node("id")->value();
		char * idtext = (char *) malloc(strlen(propertyid)+1);
		strcpy(idtext,propertyid);
		tFormula singleprop;
		if(!std::strcmp(propertynode->first_node("formula") -> first_node()->name(),"place-bound"))
		{
			Term * te = NULL;
			for(xml_node<> * placenode = propertynode->first_node("formula") -> first_node() -> first_node("place"); placenode; placenode = placenode -> next_sibling("place"))
			{
				if(ThePlaceSymbols)
				{
					// HL net
					hlplace * p = hlplace::lookup(placenode->value());
					if(!p)
					{
						RT::rep->status("unknown place: %s", placenode->value());
						RT::rep->abort(ERROR_SYNTAX);
					}
					for(int i = 0; i < p -> so -> size; i++)
					{
						// if place is in empty siphon, it will always ahve 0 tokens
						if(ThePlaceSymbols[p->offset + i] ->inEmptySiphon) continue;

						Term * newte = new Term;
						newte -> next = te;
						newte -> place = ThePlaceSymbols[p->offset + i] -> getIndex();
						newte -> mult = 1;
						te = newte;
					}
				}
				else
				{
					// LL net
					Symbol * p = ParserPTNet::currentsymbols -> PlaceTable -> lookup(placenode-> value());
					if(!p)
					{
						RT::rep->status("unknown place: %s", placenode->value());
						RT::rep->abort(ERROR_SYNTAX);
					}
					if(p -> inEmptySiphon) continue;
					Term * newte = new Term;
					newte -> next = te;
					newte -> place = p -> getIndex();
					newte -> mult = 1;
					te = newte;
				}
			}
			if(!te)
			{
				te = new Term();
				te -> next = NULL;
				te -> place = Petrinet::InitialNet->Card[PL];
				te -> mult = 0;

			}
			singleprop = CompBound();
			singleprop -> formula = new AtomicStatePredicate(Petrinet::InitialNet,te,true);
		}
		else
		{
			tStatePredicate sp =NULL;
			StatePredicate * ap = NULL;
			shapetype sh;
			readSubFormula(propertynode->first_node("formula") -> first_node(),&sp,&ap,&sh);
			if(sh != ST_TEMP) 
			{
				tAtomicProposition a= Elementary();
				a -> pred = ap;
				a -> shape = ashape(sh);
				sp = AtomicProposition(a);
				sp->formula = a -> pred;
				
			}
			singleprop = StatePredicateFormula(sp);
		}
		singleprop->id = idtext;
		Task::CompoundPlanning(singleprop);
	}
}

char * ReadPnmlFormulaId()
{
	if(!RT::args.formula_given)
	{
		switch(RT::args.check_arg)
		{
		case check_arg_Liveness: return deconst("Liveness");
		case check_arg_QuasiLiveness: return deconst("QuasiLiveness");
		case check_arg_StableMarking: return deconst("StableMarking");
		case check_arg_OneSafe: return deconst("OneSafe");
		case check_arg_deadlockfreedom: return deconst("ReachabilityDeadlock");
		default: return deconst("<<unknwon id>>");
		}
	}
RT::rep->status("reading XML formula");
RT::currentInputFile = new Input("formula", RT::args.formula_arg);
RT::inputFormulaFileName = RT::currentInputFile -> getFilename();
    // 1. convert file into this string
    xmlstring = (char *) malloc(BUFFERSIZE);
    int pnmlstringsize = BUFFERSIZE;

    char  *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    while (1) {

        if (used + BUFFERSIZE + 1 > size) {
            size = used + BUFFERSIZE + 1;


            temp = (char *) realloc(data, size);
            if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
            }
            data = temp;
        }

        n = fread(data + used, 1, BUFFERSIZE, *RT::currentInputFile);
        if (n == 0)
            break;

        used += n;
    }
  if (ferror(*RT::currentInputFile)) {
                RT::rep->status("Read error when accessing %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }

    temp = (char *) realloc(data, used + 1);
    if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }
    data = temp;
    data[used] = '\0';

    xmlstring = data;
    pnmlstringsize = used;

   // 2. run rapdidxml

        using namespace rapidxml;
        xml_document<> doc;    // character type defaults to char
        doc.parse<0>(xmlstring);    // 0 means default parse flags
 	xml_node<> * propertysetnode = doc.first_node();
RT::formula_id = (char **) calloc(sizeof(char *),1);
int card_formula = 0;
	xml_node<> * propertynode = propertysetnode->first_node("property"); 
	char * propertyid = propertynode->first_node("id")->value();
	char * idtext = (char *) malloc(strlen(propertyid)+1);
	strcpy(idtext,propertyid);
	return idtext;
}

void ReadHLPnmlFormula()
{
RT::currentInputFile = new Input("formula", RT::args.formula_arg);
RT::inputFormulaFileName = RT::currentInputFile -> getFilename();
    // 1. convert file into this string
    xmlstring = (char *) malloc(BUFFERSIZE);
    int pnmlstringsize = BUFFERSIZE;

    char  *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    while (1) {

        if (used + BUFFERSIZE + 1 > size) {
            size = used + BUFFERSIZE + 1;


            temp = (char *) realloc(data, size);
            if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
            }
            data = temp;
        }

        n = fread(data + used, 1, BUFFERSIZE, *RT::currentInputFile);
        if (n == 0)
            break;

        used += n;
    }
  if (ferror(*RT::currentInputFile)) {
                RT::rep->status("Read error when accessing %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }

    temp = (char *) realloc(data, used + 1);
    if (temp == NULL) {
                RT::rep->status("Overflow when reading %s file %s", RT::rep->markup(MARKUP_OUTPUT, RT::currentInputFile->kind.c_str()).str(),
                RT::rep->markup(MARKUP_FILE,RT::currentInputFile->filename.c_str()).str());
                RT::rep->abort(ERROR_FILE);
    }
    data = temp;
    data[used] = '\0';

    xmlstring = data;
    pnmlstringsize = used;

   // 2. run rapdidxml

        using namespace rapidxml;
        xml_document<> doc;    // character type defaults to char
        doc.parse<0>(xmlstring);    // 0 means default parse flags
 	xml_node<> * propertysetnode = doc.first_node();
RT::formula_id = (char **) calloc(sizeof(char *),1);
int card_formula = 0;

	// evaluate XML

	for(xml_node<> * propertynode = propertysetnode->first_node("property"); propertynode; propertynode = propertynode->next_sibling("property"))
	{
		char * propertyid = propertynode->first_node("id")->value();
		char * idtext = (char *) malloc(strlen(propertyid)+1);
		strcpy(idtext,propertyid);
		tFormula singleprop;
		if(!std::strcmp(propertynode->first_node("formula") -> first_node()->name(),"place-bound"))
		{
			
			// evaluate bound formula

			Term * te = NULL;
			for(xml_node<> * placenode = propertynode->first_node("formula") -> first_node() -> first_node("place"); placenode; placenode = placenode -> next_sibling("place"))
			{
				hlplace * p = hlplace::lookup(placenode->value());
				if(!p)
				{
					RT::rep->status("unknown place: %s", placenode->value());
					RT::rep->abort(ERROR_SYNTAX);
				}
				Term * newte = new Term;
				newte -> next = te;
				newte -> place = ThePlaceSymbols[p->offset] -> getIndex();
				newte -> mult = 1;
				te = newte;
			}
			if(!te)
			{
				te = new Term();
				te -> next = NULL;
				te -> place = Petrinet::InitialNet->Card[PL];
				te -> mult = 0;

			}
			singleprop = CompBound();
			singleprop -> formula = new AtomicStatePredicate(Petrinet::InitialNet,te,true);
		}
		else
		{
			tStatePredicate sp =NULL;
			StatePredicate * ap = NULL;
			shapetype sh;
			readHLSubFormula(propertynode->first_node("formula") -> first_node(),&sp,&ap,&sh);
			if(sh != ST_TEMP) 
			{
				tAtomicProposition a= Elementary();
				a -> pred = ap;
				a -> shape = ashape(sh);
				sp = AtomicProposition(a);
				sp->formula = a -> pred;
				
			}
			singleprop = StatePredicateFormula(sp);
		}
		singleprop->id = idtext;
		Task::CompoundPlanning(singleprop);
	}
}



variableterm::variableterm(hlvariable * v)
{
	var = v;
	tag = TE_VAR;
	so = v -> so;
}

int variableterm::getoffset(hlvariable ** list, int cardlist)
{
	int i;
	for(i = 0; i < cardlist;i++)
	{
		if(var == list[i]) 
		{
			return i;
		};
	}
	return -1;
}

int variableterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	return ass[getoffset(list, cardlist)];
}

succterm::succterm(hlterm * te)
{
	subterm = te;
	tag = TE_SUC;
	so = subterm -> so;
}

int succterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	return (subterm->evaluate(ass,list,cardlist) + 1) % so -> size;
}

predterm::predterm(hlterm * te)
{
	subterm = te;
	tag = TE_PRE;
	so = subterm -> so;
}

int predterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	int result = subterm->evaluate(ass,list,cardlist);
	if(result == 0)
	{
		result = so -> size - 1;
	}
	else
	{
		result--;
	}
	return result;
}

constantterm::constantterm(int v)
{
	value = v;
	tag = TE_CON;
	so = NULL;
	
}

int constantterm::evaluate(int * ass,hlvariable ** list, int cardlist)
{
	return value;
}

ValueAutomaton * andcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A = sub[0] -> evaluate(list,cardlist);
	for(int i = 1; i < cardsub; i++)
	{
		ValueAutomaton * B = sub[i]->evaluate(list,cardlist);
		A -> andProduct(list,cardlist,B);
	}
	return A;
}

ValueAutomaton * orcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A = sub[0] -> evaluate(list, cardlist);
	for(int i = 1; i < cardsub; i++)
	{
		ValueAutomaton * B = sub[i]->evaluate(list, cardlist);
		A -> orProduct(list,cardlist,B);
	}
	return A;
}

ValueAutomaton * eqcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> eqProduct(list,cardlist,A2);
	return A1;
}

ValueAutomaton * neqcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> neqProduct(list, cardlist,A2);
	return A1;
}

ValueAutomaton * gtcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> gtProduct(list, cardlist,A2);
	return A1;
}

ValueAutomaton * gecondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> geProduct(list, cardlist,A2);
	return A1;
}

ValueAutomaton * ltcondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> ltProduct(list, cardlist,A2);
	return A1;
}

ValueAutomaton * lecondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * A1 = new ValueAutomaton(sub1,list, cardlist);
	ValueAutomaton * A2 = new ValueAutomaton(sub2,list, cardlist);
	A1 -> leProduct(list, cardlist,A2);
	return A1;
}

ValueAutomaton * nocondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * result = new ValueAutomaton();
	result -> initialState -> final = true;
	return result;
}

bool * variableterm::getvars(hlvariable ** list, int card)
{
	bool * result = new bool[card];
	memset(result, 0 , card * sizeof(bool));
	result[getoffset(list,card)] = true;
	return result;
}


bool * constantterm::getvars(hlvariable ** list, int card)
{
	bool * result = new bool[card ];
	memset(result, 0 , card * sizeof(bool));
	return result;
}

bool * succterm::getvars(hlvariable ** list, int card)
{
	return subterm->getvars(list,card);
}

bool * predterm::getvars(hlvariable ** list, int card)
{
	return subterm->getvars(list,card);
}

bool * andcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub[0]->getvars(list,card);
	for(int i = 1; i < cardsub; i++)
	{
		bool * rresult = sub[i]->getvars(list,card);
		for(int j = 0; j < card;j++)
		{
			if(rresult[j]) result[j] = true;
		}
		delete [] rresult;
	}
	return result;
	
}

bool * orcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub[0]->getvars(list,card);
	for(int i = 1; i < cardsub; i++)
	{
		bool * rresult = sub[i]->getvars(list,card);
		for(int j = 0; j < card;j++)
		{
			if(rresult[j]) result[j] = true;
		}
		delete [] rresult;
	}
	return result;
	
}

bool * eqcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * neqcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * ltcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * lecondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * gtcondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * gecondition::getvars(hlvariable ** list, int card)
{
	bool * result = sub1->getvars(list,card);
	bool * rresult = sub2->getvars(list,card);
	for(int j = 0; j < card;j++)
	{
		if(rresult[j]) result[j] = true;
	}
	delete [] rresult;
	return result;
	
}

bool * nocondition::getvars(hlvariable ** list, int card)
{
	bool * result = new bool[card];
	memset(result,0, sizeof(bool) * card);
	return result;
}

bool * hltransition::getvars(hlvariable ** list, int card)
{
	bool * result = guard->getvars(list,card);
	for(hlarc * a = pre; a ; a = a -> next)	
	{
		// inscription variables
		bool * rresult = a -> inscription->getvars(list,card);
		for(int j = 0; j < card;j++)
		{
			if(rresult[j]) result[j] = true;
		}
		delete [] rresult;
		// place variables
		for(int i = 0; i < card;i++)
		{
			if(list[i] -> place == a -> place)
			{
				result[i] = true;
			}
		}
	}
	for(hlarc * a = post; a ; a = a -> next)	
	{
		// inscription variables
		bool * rresult = a -> inscription->getvars(list,card);
		for(int j = 0; j < card;j++)
		{
			if(rresult[j]) result[j] = true;
		}
		delete [] rresult;
		// place variables
		for(int i = 0; i < card;i++)
		{
			if(list[i] -> place == a -> place)
			{
				result[i] = true;
			}
		}
	}
	return result;
}

bool * coefficienthlmultiterm::getvars(hlvariable ** list,int card)
{
	return submt->getvars(list,card);
}


bool * allhlmultiterm::getvars(hlvariable ** list, int card)
{
	bool * result = new bool[card];
	memset(result,0, sizeof(bool) * card);
	return result;
}

bool * termhlmultiterm::getvars(hlvariable ** list, int card)
{
	return subterm->getvars(list,card);
}


bool * sumhlmultiterm::getvars(hlvariable ** list, int ccard)
{
	bool * result = sub[0]->getvars(list,ccard);
	for(int i = 1; i < card; i++)
	{
		bool * rresult = sub[i]->getvars(list,ccard);
		for(int j = 0; j < ccard;j++)
		{
			if(rresult[j]) result[j] = true;
		}
		delete [] rresult;
	}
	return result;
}

bool * diffhlmultiterm::getvars(hlvariable ** list, int card)
{
        bool * result = sub1->getvars(list,card);
        bool * rresult = sub2->getvars(list,card);
        for(int j = 0; j < card;j++)
        {
                if(rresult[j]) result[j] = true;
        }
        delete [] rresult;
        return result;

}

bool * tuplehlmultiterm::getvars(hlvariable ** list, int ccard)
{
        bool * result = sub[0]->getvars(list,ccard);
        for(int i = 1; i < card; i++)
        {
                bool * rresult = sub[i]->getvars(list,ccard);
                for(int j = 0; j < ccard;j++)
                {
                        if(rresult[j]) result[j] = true;
                }
                delete [] rresult;
        }
        return result;
}

bool succterm::equivalent(hlterm * other)
{
	if(other -> tag != TE_SUC) return false;
	succterm * va = (succterm *) other;
	return subterm -> equivalent(va -> subterm);
}

bool predterm::equivalent(hlterm * other)
{
	if(other -> tag != TE_PRE) return false;
	predterm * va = (predterm *) other;
	return subterm -> equivalent(va -> subterm);
}

bool constantterm::equivalent(hlterm * other)
{
	if(other -> tag != TE_CON) return false;
	constantterm * co = (constantterm *) other;
	if(value != co -> value) return false;
	return true;
}

bool variableterm::equivalent(hlterm * other)
{
	if(other -> tag != TE_VAR) return false;
	variableterm * va = (variableterm *) other;
	if(var == va -> var) return true;
	return false;
}

// component: which tuple component are we referring to
// modification: +1  for succ, -1 for pred = difference variable/constant value - token value
// free[variable] >= 0 --> variable is bound (occurs already in component free[variable])
// free[variable] < 0 --> variable occurs for the first time
// marking = array all (different) tokens in initial marking
// card: number of filled entries in marking
void succterm::processrunplace(int component,int modification, int * free, int ** marking, int * bindingmodification, int *card)
{
	subterm -> processrunplace(component,modification +1, free, marking, bindingmodification,card);
}
void predterm::processrunplace(int component,int modification, int * free, int ** marking, int * bindingmodification, int * card)
{
	subterm -> processrunplace(component,modification -1, free, marking,bindingmodification,card);
}

void constantterm::processrunplace(int component,int modification,int * free, int ** marking, int * bindingmodification, int * card)
{
	int v = value + modification; // this is what we should see in this component 
	for(int i = 0; i < *card; )
	{
		if(!(( marking[i][component] - v) % so -> size))
		{
			// keep
			i++;
		}
		else
		{
			// remove
			delete marking[i];
			marking[i] = marking[--*card];
		}
	}
}

void variableterm::processrunplace(int component, int modification, int * free, int ** marking, int * bindingmodification, int * card)
{
	//  for variable at position i:
	//  - free[i] is the component where variable is bound
	//  - bindingmodification[i] is the modification (#succ - #pred) in the term at component free[i]
	//  - modification is the modification of current component

	if(free[getoffset(unfoldlist,unfoldcard)] >= 0)
	{
		// variable is bound 
		for(int i = 0; i < i* *card; )
		{
			int varvalue = marking[i][free[getoffset(unfoldlist,unfoldcard)]] - bindingmodification[var->offset];
			int termvalue = varvalue + modification; // value of this component under given variable binding
			if(!((marking[i][component] - termvalue) % var -> so -> size))
			{
				// keep
				i++;
			}
			else
			{
				// remove
				delete marking[i];
				marking[i] = marking[--*card];
			}
		}
	}
	else
	{
		// variable is free
		free[getoffset(unfoldlist,unfoldcard)] = component;
		bindingmodification[getoffset(unfoldlist,unfoldcard)] = modification;
	}
}

ValueAutomaton * coefficienthlmultiterm::initial2automaton(hlplace * p)
{
	return submt -> initial2automaton(p);
}

ValueAutomaton * sumhlmultiterm::initial2automaton(hlplace * p)
{
	ValueAutomaton * result = sub[0]->initial2automaton(p);
	for(int i = 1; i < card; i++)
	{
		ValueAutomaton * rresult = sub[i]->initial2automaton(p);
		result -> andProduct(unfoldlist,unfoldcard,rresult);
	}
	return result;
}

ValueAutomaton * tuplehlmultiterm::initial2automaton(hlplace * p)
{
	for(int i = 0; i < unfoldcard;i++)
	{
		unfoldlist[i] -> offset = i;
	}
	productsort * ps = (productsort *) p -> so;
	int dim = ps -> cardsubsorts;
	// 1. unfold initial marking of p into array
	int card = 0;
	if(! p->initialmarking) return NULL;
	for(int i = 0; i < p -> so -> size; i++)
	{
		if(p -> initialmarking[i]) card++;
	}
	int ** im = new int * [card];
	card = 0;
	for(int i = 0; i < p -> so -> size; i++) // for all token colors that occur in m0(p)
	{
		if(!p->initialmarking[i]) continue;
		// split makring into individual component values
		im[card] = new int[dim];
		int m = i;
		for(int j = dim - 1; j >= 0; j--)
		{
			im[card][j] = m % (ps -> subsort[j]->size);
			m /= (ps -> subsort[j]->size);
		}
		card++;
	}
	// 2. filter array according to tuple components

	// set all variables to free
	int * free = new int[unfoldcard];
	int * bindingmodification = new int [unfoldcard];
	for(int i = 0; i < unfoldcard; i++)
	{
		free[i] = -1;
		bindingmodification[i] = 0;
	}
	for(int i = 0; i < dim; i++)
	{
		sub[i]->processrunplace(i, 0, free, im, bindingmodification, &card);
	}
	// 3. turn remaining array into value automaton
	ValueAutomaton * result = new ValueAutomaton();
	if(!card) return result;
	int d = 0;
	for(int j = 0; j < unfoldcard;j++)
	{
		if(free[j] >= 0) d++;
	}
	result -> depth = d ;
	result -> level = new ValueAutomatonState * [d+1];
	result -> alphabet = new sort * [d];
	result -> vars = new hlvariable * [d];
	memset(result -> level, 0, sizeof(ValueAutomatonState *) * (d+1));
	d = 0;
	for(int j = 0; j < unfoldcard;j++)
	{
		if(free[j] >= 0) 
		{
			result -> vars[d] = unfoldlist[j];
			result -> alphabet[d] = result -> vars[d]->so;
			d++;
		}
	}
	result -> level[0] = result -> initialState;
	result -> initialState -> final = 0;
	for(int i = 0; i < card; i++)
	{
		ValueAutomatonState * currentstate = result -> initialState;
		d = 0;
		for(int j = 0; j < unfoldcard; j++)
		{
			if(free[j] < 0) continue; // variable not bound by this runplace
			d++;
			int letter = im[i][free[j]] - bindingmodification[j]; // value of variable if bound to that token
			while(letter < 0) letter += unfoldlist[j]->so->size;
			ValueAutomatonTransition ** t = &(currentstate -> transition);
			while(true)
			{
				if(!(*t) || (*t) -> lower > letter)
				{
					ValueAutomatonTransition * newtransition = new ValueAutomatonTransition();
					newtransition -> lower = newtransition -> upper = letter;
					newtransition -> next = *t;
					*t = newtransition;
					newtransition -> succ = new ValueAutomatonState();
					newtransition -> succ -> next_sibling = result -> level[d];
					result -> level[d] = newtransition -> succ;
					currentstate = newtransition -> succ;
					break;
				}
				if((*t) -> lower < letter)
				{
					t = &((*t) -> next);
					continue;
				}
				if((*t) -> lower == letter)	
				{
					currentstate = (*t) -> succ;
					break;
				}
			}
		}
		for(ValueAutomatonState * s = result -> level[d]; s; s = s->next_sibling)
		{
			s -> final = true;
		}
	}
	result -> minimise();
	return result;
}

ValueAutomaton * termhlmultiterm::initial2automaton(hlplace * p)
{
	for(int i = 0; i < unfoldcard; i++)
	{
		unfoldlist[i]->offset = i;
	}
	productsort * ps = (productsort *) p -> so;
	// 1. unfold initial marking of p into array
	int card = 0;
	if(! p->initialmarking) return NULL;
	for(int i = 0; i < p -> so -> size; i++)
	{
		if(p -> initialmarking[i]) card++;
	}
	int ** im = new int * [card];
	card = 0;
	for(int i = 0; i < p -> so -> size; i++)
	{
		if(!p->initialmarking[i]) continue;
		im[card] = new int[1];
		im[card++][0] = i;
	}
	// 2. filter array according to term
	int * free = new int[unfoldcard];
	int * bindingmodification = new int[unfoldcard];
	for(int i = 0; i < unfoldcard; i++)
	{
		free[i] = -1;
	}
	subterm->processrunplace(0, 0, free, im, bindingmodification,&card);
	// 3. turn remaining array into value automaton
	ValueAutomaton * result = new ValueAutomaton();
	if(!card) return result;
	int d = 0;
	for(int j = 0; j < unfoldcard;j++)
	{
		if(free[j] >= 0) d++;
	}
	result -> depth = d;
	result -> level = new ValueAutomatonState * [d+1];
	result -> vars = new hlvariable * [d];
	result -> alphabet = new sort * [d];
	memset(result -> level, 0, sizeof(ValueAutomatonState *) * (d+1));
	d = 0;
	for(int j = 0; j < unfoldcard;j++)
	{
		if(free[j] >= 0) 
		{
			result -> vars[d] = unfoldlist[j];
			result -> alphabet[d] = result -> vars[d]->so;
			d++;
		}
	}
	result -> level[0] = result -> initialState;
	result -> initialState -> final = 0;
	for(int i = 0; i < card; i++)
	{
		ValueAutomatonState * currentstate = result -> initialState;
		d = 0;
		for(int j = 0; j < unfoldcard; j++)
		{
			if(free[j] < 0) continue;
			d++;
			// is free variable --> create single transition
			int letter = im[i][0] - bindingmodification[j];
			ValueAutomatonTransition ** t = &(currentstate -> transition);
			while(true)
			{
				if(!(*t) || (*t) -> lower > letter)
				{
					ValueAutomatonTransition * newtransition = new ValueAutomatonTransition();
					newtransition -> lower = newtransition -> upper = letter;
					newtransition -> next = *t;
					*t = newtransition;
					newtransition -> succ = new ValueAutomatonState();
					newtransition -> succ -> next_sibling = result -> level[d];
					result -> level[d] = newtransition -> succ;
					currentstate = newtransition -> succ;
					break;
				}
				if((*t) -> lower < letter)
				{
					t = &((*t) -> next);
					continue;
				}
				if((*t) -> lower == letter)	
				{
					currentstate = (*t) -> succ;
					break;
				}
			}
		}
		for(ValueAutomatonState * s = result -> level[d]; s; s = s->next_sibling)
		{
			s -> final = true;
		}
	}
	result -> minimise();
	return result;
}

ValueAutomaton * runplacecondition::evaluate(hlvariable ** list, int cardlist)
{
	ValueAutomaton * result =  inscription -> initial2automaton(p);
	return result;
}


bool analyseTransitionClasses(ParserPTNet * symboltable, arrayindex_t * trans, arrayindex_t * equivalent, bool * isminimal)
{
	RT::rep->status("start findlow");
	TransitionSymbol ** transitionmap = new TransitionSymbol * [symboltable -> TransitionTable -> getCard()];
	for(Symbol * sy = symboltable -> TransitionTable -> first(); sy; sy = symboltable -> TransitionTable -> next())
	{
		transitionmap[sy->index] = (TransitionSymbol *) sy;
	}
	time_t starttime = time(NULL);
	int findlow = 0;
	// input: two arrays of transition indices.
	// transition trans[i] is equivalent to transition trans[j] if equivalent[i] = equivalent[j]

	// goal: for each class, build automaton and check whether there are color distributions of tokens
	// where none of the transitions in class can fire.

	// for all classes do:
	// (for each class there is precisely one transition with trans[i] = equivalent[i])
	// this is the case for class member with smallest i

	int cardT = symboltable -> TransitionTable->getCard();

	for(int index = 0; index < cardT; index++)
	{
		// consider representative ttt of equivalence class

		if(!isminimal[index]) continue;
		if(trans[index] != equivalent[index]) continue;
		TransitionSymbol * tsy = transitionmap[trans[index]];
		hltransition * ttt = hltransition::lookup(tsy->getKey());
RT::rep->status("CHECK FINDLOW FOR TRANS %s", ttt -> name);
		arrayindex_t tttindex = trans[index];
		// check moderate token usage

		bool moderate = true;
		for(hlarc * aaa = ttt -> pre; aaa; aaa = aaa -> next)
		{
			if(aaa -> inscription -> nroftokens > 2)
			{
				moderate = false; break;
			}
		}
		if(!moderate)
		{
			RT::rep->status("nonmoderate token usage");
			findlow++;
			continue;
		}


	
		hlvariable::placecard = 0;
		memset(hlvariable::placehash_table,0, HLHASHTABLESIZE * sizeof(hlvariable *));
		// invent place variable names
		// for all places in pre-set of ttt
		for(hlarc * aaa = ttt -> pre; aaa; aaa = aaa -> next)
		{
			hlplace * ppp = aaa -> place;
RT::rep->status("INVENT VAR FOR PLACE %s", aaa -> place -> name);
			int size = aaa-> inscription -> nroftokens;
			ppp -> placevariables = new hlvariable * [size];
			for(int i = 0; i < size; i++)
			{
				ppp->placevariables[i] = new hlvariable();
				ppp->placevariables[i]->diff = false;
				ppp->placevariables[i]->name = ppp -> name;
				ppp->placevariables[i]->place = ppp;
				ppp->placevariables[i]->so = aaa -> inscription -> so;
				hlvariable::insert(ppp->placevariables[i], true);
			}
		}
		// for all transitions u in class of t
		for(int i = 0; i < cardT; i++)
		{
			if(equivalent[i] != tttindex) continue;
			TransitionSymbol * usy = transitionmap[trans[i]];
			hltransition * uuu = hltransition::lookup(usy->getKey());
RT::rep->status("CHECK EQ TRANS %s", uuu -> name);
			for(hlarc * aaa = uuu -> pre; aaa; aaa = aaa -> next)
			{
				aaa -> inscription -> adddiffvars();
			}
		}

		// create place variable list (and fix their order)

		// first, list all regular place variables

		findlowcard = hlvariable::card + hlvariable::placecard;
		findlowlist = new hlvariable * [findlowcard];
		int q = 0;
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlvariable::placehash_table[i]) continue;
			if(!hlvariable::placehash_table[i]->place) continue;
			if(hlvariable::placehash_table[i]->diff) continue; // only insert regular
			if(hlvariable::placehash_table[i]->so->tag == SO_PRO) continue; // do not count tuple variables
			findlowlist[q++] = hlvariable::placehash_table[i];
		}

		// second, list all diff variables

		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlvariable::placehash_table[i]) continue;
			if(!hlvariable::placehash_table[i]->place) continue;
			if(!hlvariable::placehash_table[i]->diff) continue; // only insert diff
			if(hlvariable::placehash_table[i]->so->tag == SO_PRO) continue; // do not count tuple variables
			findlowlist[q++] = hlvariable::placehash_table[i];
		}

		// third, list all transition variables
		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(!hlvariable::hash_table[i]) continue;
			if(hlvariable::hash_table[i]->place) continue;
			if(hlvariable::hash_table[i]->so->tag == SO_PRO) continue; // do not count tuple variables
										// since we have inserted their components
										// as separate variables
			findlowlist[q++] = hlvariable::hash_table[i];
		}
		findlowcard = q;

		ValueAutomaton * findlowautomaton = new ValueAutomaton();


		// for all transitions u in class of t
		for(int i = 0; i < cardT; i++)
		{
			if(equivalent[i] != tttindex) continue;
			if(!isminimal[i]) continue;
			TransitionSymbol * usy = transitionmap[trans[i]];
			hltransition * uuu = hltransition::lookup(usy->getKey());

			// create occurring vector
			bool * occurring = uuu -> getvars(findlowlist,findlowcard);

			
			// create automaton for guard

			ValueAutomaton * guardautomaton = uuu -> guard -> evaluate(findlowlist,findlowcard);



			// get through all arcs
	
			for(hlarc * aaa = uuu -> pre; aaa; aaa = aaa -> next)
			{
				// create automaton for aaa

				ValueAutomaton * arcautomaton = aaa -> inscription -> createfindlowautomaton(aaa->place->placevariables,aaa->inscription->nroftokens);
				
				// "and" to guard automaton

				guardautomaton -> andProduct(findlowlist,findlowcard,arcautomaton);
			}

			// project to place variables
			

			int j;
			for(j = 0; j < findlowcard;j++)
			{
				if(!findlowlist[j]->place || findlowlist[j]->diff) break;
			}
			guardautomaton -> project(j);

			// "or"  to findlowautomaton

			findlowautomaton -> orProduct(findlowlist,findlowcard,guardautomaton);
			
		}


		// check whether findlowautomaton is full

		bool full = true;
		for(int i = 0; i < findlowautomaton -> depth; i++)
		{
			for(ValueAutomatonState * sss = findlowautomaton->level[i]; sss; sss = sss -> next_sibling)
			{
				if(sss -> transition -> lower != 0)
				{
					full = false;
					break;
				}
				ValueAutomatonTransition * ttt = sss -> transition;
				while(ttt)
				{
					if(ttt -> next)
					{
						if(ttt -> upper != ttt -> next->lower)
						{
							full = false;
							break;
						}
						else
						{
							ttt = ttt -> next;
						}
					}
					else
					{
						if(ttt -> upper != findlowautomaton ->alphabet[i] ->size -1)
						{
							full = false;
						}
						break;
					}
					
				}
				if(!full) break;
			}
			if(!full) break;
		}
		if(!full) 
		{
			RT::rep->status("findlow criterion violated for transition %d", trans[index]);
			for(int k = 0; k < HLHASHTABLESIZE;k++)
			{
				if(hlvariable::hash_table[k] && hlvariable::hash_table[k]->place)
				{
					delete hlvariable::hash_table[k];
				}
			}
			findlow++;
			continue;
		}

		for(int i = 0; i < HLHASHTABLESIZE;i++)
		{
			if(hlvariable::hash_table[i] && hlvariable::hash_table[i]->place)
			{
				delete hlvariable::hash_table[i];
			}
		}

		
	}
	if(findlow == 0)
	{
		RT::rep->status("findlow criterion satisfied");
	RT::rep->status("Time for checking findlow: %f", difftime(time(NULL), starttime));
		return true;
	}
	else
	{
		RT::rep->status("findlow criterion violated for %d clusters", findlow);
	RT::rep->status("Time for checking findlow: %f", difftime(time(NULL), starttime));
		return false;
	}
}
