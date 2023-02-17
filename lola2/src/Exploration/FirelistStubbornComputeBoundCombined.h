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

\brief class for firelist generation for bound computations. Use up sets.
Run deletion algorithm on the resulting stubborn set for getting an
inclusion-minimal set.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Exploration/FirelistStubbornDeadlock.h>

class Firelist;
class StatePredicate;

/// a stubborn firelist for bound computations
class FirelistStubbornComputeBoundCombined : public Firelist
{
public:
    explicit FirelistStubbornComputeBoundCombined(Petrinet *, StatePredicate *);
    ~FirelistStubbornComputeBoundCombined();

    /// return value contains number of elements in fire list, argument is reference parameter for actual list
    virtual arrayindex_t getFirelist(NetState &, arrayindex_t **);

    /// create a new Stubborn Firelist form the current one
    virtual Firelist *createNewFireList(Petrinet *, StatePredicate *);

private:
    StatePredicate *predicate;
    arrayindex_t *dfsStack;
    bool *onStack;
 // data structure for subsequent deletion algorithm
    int * status;  // in, out, nailed, or tmp_out in deletion alg.
    bool speculate(NetState &,arrayindex_t); // try to remove node + consequences
    void nail(NetState &,arrayindex_t); // mark t as unremovable + consequences
    arrayindex_t card_enabled;
    arrayindex_t temp_card_enabled;
    int counter;
};
