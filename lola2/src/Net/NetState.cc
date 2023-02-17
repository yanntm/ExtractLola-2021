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
\author Christian K and Gregor B
\status unknown
 */

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Net/NetState.h>

NetState *NetState::createNetStateFromInitial(Petrinet * n)
{
    NetState *ns = new NetState();
    ns->membersInitialized = true;
    ns-> net = n;
    // copy the current marking at its hash
    ns->Current = new capacity_t[n ->Card[PL]];
    for (arrayindex_t i = 0; i < n->Card[PL]; i++)
    {
        ns->Current[i] = n->Initial[i];
    }
    ns->HashCurrent = n->HashInitial;

    // copy the currently enabled transitions
    ns->Enabled = new bool[n->Card[TR]];
    for(arrayindex_t i = 0; i < n->Card[TR];i++)
    {
	ns->Enabled[i] = true;
    }
    ns -> CardEnabled = n->Card[TR];
    for (arrayindex_t i = 0; i < n->Card[TR]; i++)
    {
        n -> checkEnabled(*ns,i);
    }

    return ns;
}

NetState::NetState(const NetState &ons)
{
    membersInitialized = true;
    net = ons.net;
    // copy the current marking at its hash
    Current = new capacity_t[net->Card[PL]];
    for (arrayindex_t i = 0; i < net->Card[PL]; i++)
    {
        Current[i] = ons.Current[i];
    }
    HashCurrent = ons.HashCurrent;

    // copy the currently enabled transitions
    Enabled = new bool[net->Card[TR]];
    for (arrayindex_t i = 0; i < net->Card[TR]; i++)
    {
        Enabled[i] = ons.Enabled[i];
    }
    CardEnabled = ons.CardEnabled;
    buchistate = -1;

}

NetState &NetState::operator=(const NetState &ns)
{
    NetState tmp(ns); // copy and swap
    swap(tmp);
    return *this;
}

void NetState::swap(NetState &ns)
{
    std::swap(Current, ns.Current);
    std::swap(HashCurrent, ns.HashCurrent);
    std::swap(Enabled, ns.Enabled);
    std::swap(CardEnabled, ns.CardEnabled);
    std::swap(membersInitialized, ns.membersInitialized);
}


/*!
\todo Find out why membersInitialized is used here.
*/
NetState::~NetState()
{
    if (!membersInitialized)
    {
        return;
    }
    delete[] Current;
    delete[] Enabled;
}
