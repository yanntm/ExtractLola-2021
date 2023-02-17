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
sets for LTL search.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>

class Firelist;

/// a stubborn firelist for the search for deadlocks 
class FirelistStubbornLTLDeletion : public Firelist
{
public:
    /// constructor for deadlock search
    FirelistStubbornLTLDeletion(Petrinet *);

    /// destructor
    ~FirelistStubbornLTLDeletion();

    /// return value contains number of elements in fire list, argument is reference
    /// parameter for actual list
    virtual arrayindex_t getFirelist(NetState &ns, arrayindex_t **);


private:
    // data structure for subsequent deletion algorithm
    int * status;  // in, out, nailed, or tmp_out in deletion alg.
    bool speculate(NetState &,arrayindex_t); // try to remove node + consequences
    void nail(NetState &,arrayindex_t); // mark t as unremovable + consequences
    arrayindex_t card_enabled;
    arrayindex_t temp_card_enabled;
    int counter;
};
	
