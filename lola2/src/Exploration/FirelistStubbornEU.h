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

\brief class for firelist generation. Use up sets.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Exploration/Firelist.h>
#include <Exploration/StatePredicateProperty.h>
#include <Exploration/FirelistStubbornStatePredicate.h>

class Firelist;
class StatePredicate;

/// a stubborn firelist for the search for a marking fulfilling a state predicate
class FirelistStubbornEU : public Firelist
{
public:
    explicit FirelistStubbornEU(Petrinet *, StatePredicate *,StatePredicate *);
    ~FirelistStubbornEU();

    /// return value contains number of elements in fire list, argument is reference parameter for actual list
    virtual arrayindex_t getFirelist(NetState &, arrayindex_t **);

    /// create a new StubbornStatePredicate-Firelist form the current one
    virtual Firelist *createNewFireList(Petrinet *, StatePredicateProperty *phiproperty, StatePredicateProperty * psiproperty);

private:
    StatePredicate *phipredicate;
    StatePredicate *psipredicate;
    arrayindex_t *dfsStack;
    bool *onStack;
    FirelistStubbornStatePredicate * upsetfirelist;
};
