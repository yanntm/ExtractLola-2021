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

\brief We partition the net into classes that ultimately become the nodes of the skeleton
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>


class Constraint_HL;

class Vertex_HL
{
public:
    Vertex_HL();

    /// sort arcs according to their colors
    void sortArcs(Vertex_HL **, arrayindex_t *, arrayindex_t, arrayindex_t);

    /// total number of vertices
    static arrayindex_t card;

    /// for places: the real id
    /// for transitions: the real id + card(P)
    /// for formula nodes: some consecutive number beyond card(P) + card(T)
    arrayindex_t id;

    /// inscription of the node
    arrayindex_t color;

    /// number of successors in graph
    arrayindex_t cardSucc;
    /// number of predecessors in graph
    arrayindex_t cardPre;

    /// the successors in graph
    Vertex_HL **succ;
    /// the colors of outgoing edges
    arrayindex_t *colorSucc;

    /// the predecessors in graph
    Vertex_HL **pre;
    /// the colors of incoming edges
    arrayindex_t *colorPre;

    /// two arrays of properties: one for this node as argument of symmetry, one as image; array index = thread using it
    int property;
    /// the constraint that this node is contained in (DOM vs COD), for each thread
    Constraint_HL *inConstraint;
};
