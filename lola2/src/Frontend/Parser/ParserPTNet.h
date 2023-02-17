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
\status approved 25.01.2012
\ingroup g_frontend g_symboltable

\brief definition of class ParserPTNet
*/

#pragma once

#include <pthread.h>
#include <Core/Dimensions.h>
#include <Net/Protonet.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>

class SymbolTable;
class PlaceSymbol;
class ProtoNet;
class TransitionSymbol;

/*!
\brief symbol tables for a low-level net

This data structure should be independent from the input file format (LoLA,
PNML, ...).

\ingroup g_frontend g_symboltable
*/
class ParserPTNet
{
public:
    ParserPTNet();
    ~ParserPTNet();

    /// a symbol table for places
    SymbolTable *PlaceTable;
    /// a symbol table for transitions
    SymbolTable *TransitionTable;

    /// translation into main net structures Node, Place, Transition, and Marking
    void symboltable2net(Petrinet * pppnnn = NULL);
    void symboltables2protonet();
    void setIndices();
     bool formulacontainsdeadlock;
     static ParserPTNet * currentsymbols;
    void markVisibleNodes(kc::tFormula);
    void markVisibleBuchiNodes();
    static Petrinet * protonet2net(ProtoNet * PNet);
    static int * relabelplacemapping;
    static int * relabeltransitionmapping;
    static Petrinet * relabelnet;
    void relabelFormula(Petrinet *, kc::tFormula);
    //ParserPTNet * copy();	
    void print();
    bool checkFindlowCriterion(Petrinet * );
    bool findlowstarted;
    bool findlowfinished;
   bool findlowvalue;
    pthread_mutex_t findlowmutex;
    pthread_cond_t findlowcond;
};


