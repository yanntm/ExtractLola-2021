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
\author Karsten
\status new 

\brief This file provides data structures for a preliminary
Petri net format that is generated directly from the parsers
and used for net reduction.

*/

#pragma once


#include <Core/Dimensions.h>
#include <InputOutput/JSON.h>
#include <Frontend/SymbolTable/ArcList.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>

class Petrinet;
class Arc;
class ProtoTransition;
class ProtoNet;

// arc directions
#define TP true
#define PT false

class ProtoNode
{
public:
	ProtoNet * net;
	int dfs;
	bool visited;
	Arc * currentarc;
	Arc * currentsecarc;
	bool currentdirection;
	bool visible;
	bool included;

	Arc * firstArcPre; /// first incoming arc
	Arc * lastArcPre; /// last incoming arc
	Arc * firstArcPost; /// first outgoing arc
	Arc * lastArcPost; /// last outgoing arc

	arrayindex_t cardPre;
	arrayindex_t cardPost;
	bool sortedPre;
	bool sortedPost;
	int index;
};

class ProtoPlace : public ProtoNode
{
public: 
	PlaceSymbol * sy; /// symbol table entry for this place
	ProtoPlace * next; /// next in double linked list of all places
	ProtoPlace * prev; /// prev in double linked list of all places
	capacity_t capacity;  /// max nr of tokens
	capacity_t marking; /// initial marking
	
	bool finished;
	int lowlink;


	ProtoPlace(ProtoNet*,PlaceSymbol*, capacity_t, capacity_t);
	ProtoPlace(ProtoPlace &);
	~ProtoPlace();
};

class ProtoTransition : public ProtoNode
{
public:
	TransitionSymbol * sy; /// symbol table entry
	ProtoTransition * next;  /// next in double linked list of all transitions
	ProtoTransition * prev; /// prev in double linked list of all transitions

	ProtoTransition(ProtoNet*,TransitionSymbol *);
	ProtoTransition(ProtoTransition &);
	~ProtoTransition();
};

class Arc
{
public:
	mult_t mult;   /// weight of this arc
	bool toplace; /// direction (true = (t,p), false = (p,t)
	ProtoPlace * pl;   /// place attached to this arc
	ProtoTransition * tr;  /// transition attached to this arc
	Arc * next_pl;   /// next arc connected to same place
	Arc * prev_pl; /// prev arc connected to same place
	Arc * next_tr; /// next arc connected to same transition
	Arc * prev_tr; /// prev arc connected to same transition

	Arc(ProtoPlace *, ProtoTransition *, mult_t, bool); // create arc
	~Arc();
};

class ProtoNet
{
public:
	bool isSkeleton;
	ProtoPlace * firstplace;
	ProtoPlace * lastplace;
	ProtoTransition * firsttransition;
	ProtoTransition * lasttransition;
	arrayindex_t cardPL;
	arrayindex_t cardTR;
	lprec * lp; // the LP problem that represents the state equation

	ProtoNet();
	~ProtoNet();
	ProtoNet(ProtoNet&);
	void createLP();

	static ProtoNet * currentnet;
	static void sortArcsPl(Arc ** first, Arc ** last, int card);
	static void sortArcsTr(Arc ** first, Arc ** last, int card);

	static int currentvisibility;
	int markedvisibility;

	int applyRemoveSequentialTransition(bool,bool,formula_t);
	int applyRemoveSequentialPlace(bool,bool,formula_t);
	int applyRemoveParallelTransition(bool,bool,formula_t);
	int applyRemoveDominatedTransition(bool,bool,formula_t);
	int applyRemoveRedundantComposition(bool,bool,formula_t);
	int applyRemoveParallelPlace(bool,bool,formula_t);
	int applyRemoveSinkPlace(bool,bool,formula_t);
	int applyRemoveDeadTransition(bool,bool,formula_t);
	int applyRemoveRedundantPlace(bool,bool,formula_t);
	int applyRemoveRedundantTransition(bool,bool,formula_t);
	int applyAgglomeration(bool,bool,formula_t);
	int applyCondenseStronglyConnectedStateMachine(bool,bool,formula_t);
	int applySlicing(bool,bool,formula_t, tFormula);
        void reduce(bool,bool,formula_t);
	void applyEmptySiphon();
	void detectEmptySiphon();
	void print();
	JSON * data; // place where reduction needs to be reported to
	void compare(Petrinet *);
        void recordVisibleNodes();
        void resymbol(SymbolTable *, SymbolTable *); /// link protonet to new (cloned) symbol table
};
