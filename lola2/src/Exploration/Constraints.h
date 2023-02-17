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

\brief A constraint is a set of nodes. Ultimately, the set represents a HL node.

Starting with two constraints (P) and (T),
we develop HL nodes by an iterative process of
splitting constraints.
*/

#pragma once

#include <Formula/StatePredicate/StatePredicate.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>
#include <Core/Dimensions.h>
#include <Net/Protonet.h>
#include <Exploration/AutomorphismGraph.h>

// break circular dependencies
class ParserPTNet;
class PlaceSymbol;
class TransitionSymbol;
class ProtoNet;
class ProtoNode;
class ProtoPlace;
class ProtoTransition;
/*!
\brief a class of nodes
*/
class Constraint_HL
{
public:
    Constraint_HL();

    /// index of first element in Elements
    arrayindex_t from;
    /// index of last element in Elements
    arrayindex_t to;

    // signals whether this constraint still needs to be processed in refinement process
    bool unprocessed;
    bool touched;

    /// a linked (todo) list for all unprocess constraints
    Constraint_HL *next;
    Constraint_HL *nextTouched;
    PlaceSymbol * sy;
    TransitionSymbol * tsy;
};


/*!
a class to encapsulate the skeleton calculation

*/
class Create_HL_Net
{
public:
    ProtoPlace ** originalplace;
    ProtoTransition ** originaltransition;

    ProtoNet * net;
    tFormula formula;
    Create_HL_Net(ProtoNet *, tFormula);

    PlaceSymbol ** placemapping;
    TransitionSymbol ** transitionmapping;

    /// elements array 
    Vertex_HL **elements;

    /// an array of constraints
    Constraint_HL *constraints;

    /// the number of constraints
    arrayindex_t cardConstraints;

    /// a linked list of constraints that need to be refined
    Constraint_HL *todo;

    /// sorts an element array by property 
    /// arg1: from index, arg2: to index
    void sort(arrayindex_t, arrayindex_t);

    ParserPTNet * create_HL_Net();
    tFormula create_HL_Formula(tFormula);

    static Create_HL_Net * current;


    /// constructs a graph from net
    void initGraph();

    /// creates an initial refinement refinement from (P, T)
    void initialRefinement();

    /// initial split of a constraint (only on one array)
    bool initialSplit(Constraint_HL *const);

    /// refine by number of neighbors (within initial refinement)
    void initialArcRefine();

    /// debug output of initial constraints
    void printInitialConstraints();
    /// debug output of initial elements
    void printInitialElements();
    void printGraph();
    void printId(arrayindex_t);

    /// anchor for a linked list containing all constraints touched by considered arcs
    Constraint_HL *firstTouched;
};
