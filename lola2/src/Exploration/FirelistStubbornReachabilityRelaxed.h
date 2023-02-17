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

\brief Class for firelist generation by the deletion algorithm for stubborn
sets for reachability search.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Formula/StatePredicate/StatePredicate.h>

class Firelist;

/// a stubborn firelist for the search for deadlocks 
class FirelistStubbornReachabilityRelaxed : public Firelist
{
public:
    /// constructor for deadlock search
    FirelistStubbornReachabilityRelaxed(Petrinet *, StatePredicate *);

    /// destructor
    ~FirelistStubbornReachabilityRelaxed();

    /// return value contains number of elements in fire list, argument is reference
    /// parameter for actual list
    virtual arrayindex_t getFirelist(NetState &ns, arrayindex_t **);


private:
	StatePredicate * predicate;
    // data structure for subsequent deletion algorithm
    int * status;  // in, out, nailed, or tmp_out in deletion alg.
    arrayindex_t * nailstack;
    bool * onnailstack;
    arrayindex_t nailstackpointer;
   arrayindex_t * speculatestack;
    arrayindex_t speculatestackpointer;

    bool * onspeculatestack;
    bool speculate_insert(arrayindex_t);
    bool speculate(NetState & ns,arrayindex_t); // try to remove node + consequences
    void nail(arrayindex_t); // mark t as unremovable + consequences
    void process_nail(NetState &); // mark t as unremovable + consequences
    arrayindex_t card_enabled;
    arrayindex_t temp_card_enabled;
    int counter;
    bool * inUpset;
    arrayindex_t * Upset;
    arrayindex_t cardUpset;
};
	
