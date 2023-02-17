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
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Net/Petrinet.h>
#include <Witness/Path.h>
#include <Exploration/Firelist.h>
#include <Exploration/SearchStack.h>
#include <Core/Dimensions.h>
#include <Exploration/DFSStackEntry.h>
#include <Exploration/SimpleProperty.h>

class ChooseTransition;
class Firelist;
class NetState;


class EGExploration
{
public:
    Petrinet * net;
    EGExploration(Petrinet * n):ppp(n) { net = n; }
    /// evaluates a given EG property by standard depth-first-search

    virtual bool depth_first(SimpleProperty &property, NetState &ns, Store<bool> &myStore,
                             Firelist &myFirelist, int threadNumber);

    SearchStack<DFSStackEntry> dfsstack;
    Path ppp;
};

