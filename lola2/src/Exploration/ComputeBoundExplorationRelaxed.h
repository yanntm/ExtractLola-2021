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
#include <Witness/Path.h>
#include <Exploration/Firelist.h>
#include <Exploration/SearchStack.h>
#include <Core/Dimensions.h>
#include <Exploration/DFSStackEntry.h>
#include <Exploration/SimpleProperty.h>
#include <Net/Petrinet.h>

class ChooseTransition;
class NetState;


class ComputeBoundExplorationRelaxed
{
public:
    Petrinet * net;
    ComputeBoundExplorationRelaxed(Petrinet * n){net = n;}
    /// evaluates a given property by standard depth-first-search
    virtual statenumber_t getDFS(statenumber_t *payload) const
    {
        return *payload;
    }
    virtual void setDFS(statenumber_t *payload, statenumber_t dfs)
    {
        *payload = dfs;
    }

    virtual int depth_first(SimpleProperty &property, NetState &ns, Store<statenumber_t> &myStore,
                             Firelist &myFirelist, int threadNumber);

    SearchStack<DFSStackEntry> dfsstack;
int lp(SimpleProperty &property, NetState &ns, Store<statenumber_t> &myStore,
                             Firelist &myFirelist, int threadNumber);

	int StructuralBound;
	int result;
};
