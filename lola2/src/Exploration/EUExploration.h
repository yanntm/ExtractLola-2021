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

\brief Evaluates EU property 
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Net/Petrinet.h>

class ChooseTransition;
class Firelist;
class NetState;
class SimpleProperty;

/*!
\author Karsten
\status new

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/
class EUExploration
{
public:
    Petrinet * net;
    EUExploration(Petrinet * n):_p(n) {net = n;}
    /// evaluates a given property by standard depth-first-search
    bool virtual depth_first(SimpleProperty &phiproperty, SimpleProperty &psiproperty, NetState &ns, Store<void> &myStore,
                             Firelist &myFirelist, int threadNumber);

    /// return a witness path (currently only for find_path)
    Path path() const;

    virtual ~EUExploration() {}

protected:
    /// the witness path (created by calling path())
    Path _p;
};
