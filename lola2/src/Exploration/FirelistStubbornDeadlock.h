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

\brief class for firelist generation. Default is firelist consisting of all
enabled transitions.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Exploration/Firelist.h>


/// a stubborn firelist for the search for deadlocks
class FirelistStubbornDeadlock : public Firelist
{
public:
    FirelistStubbornDeadlock(Petrinet *);
    ~FirelistStubbornDeadlock();
    /// return value contains number of elements in fire list, argument is reference
    /// parameter for actual list
    virtual arrayindex_t getFirelist(NetState &ns, arrayindex_t **);

    virtual Firelist *createNewFireList(Petrinet *, SimpleProperty *property);

private:
    arrayindex_t *dfsStack; /// stack for exploring must-be-included (mbi) graph between transition
    arrayindex_t *dfs;     /// depth first search number of stack elements
    arrayindex_t *lowlink;  /// lowlink value of stack alements
    arrayindex_t *currentIndex; /// successor to be explored
    arrayindex_t *TarjanStack;   /// stack where scc are collected from
    arrayindex_t **mustBeIncluded;   /// the actual mbi relation
    bool *visited;            /// for each transition, whether already seen
    bool *onTarjanStack;     /// for each transition, whether still on trajan stack
};
