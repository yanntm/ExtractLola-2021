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

\brief Class for firelist generation. Default is firelist consisting of all
enabled transitions.
*/

#include <Exploration/Firelist.h>
#include <Net/NetState.h>
#include <Core/Runtime.h>

/*!
\param ns the net-state for which the firelist should be determined
\param[in,out] result the actual fire list
\result number of elements in fire list
*/

Firelist::Firelist(Petrinet * n)
{
	net = n;
	usedAsScapegoat = new bool[net->Card[PL]];
}

arrayindex_t Firelist::getFirelist(NetState &ns, arrayindex_t **result)
{
    assert(ns.CardEnabled <= net->Card[TR]);

    *result = new arrayindex_t[ns.CardEnabled];
    arrayindex_t i = 0;
    for (arrayindex_t t = 0; t < net->Card[TR]; ++t)
    {
        if (ns.Enabled[t])
        {
            assert(i < ns.CardEnabled);
            (*result)[i++] = t;
        }
    }
    return ns.CardEnabled;
}

/*!
This function will create a new firelist of the same type as the current one
based on the given simple property. in some cases this property will be
ignored, but if this is a stubborn-state firelist the property will be used.

This function is essential for the parallel exploration of the state space.
Each thread will have its own firelist with the current simple property.

\todo shouldnt something happen to this function? is it right that the property
doesnt get used?
*/
Firelist *Firelist::createNewFireList(Petrinet * n, SimpleProperty *)
{
    return new Firelist(n);
}

arrayindex_t Firelist::selectScapegoat(NetState & ns, arrayindex_t currenttransition)
{
       arrayindex_t scapegoat;
       for(arrayindex_t i = 0; i < net->CardArcs[TR][PRE][currenttransition];i++)
       {

		arrayindex_t candidate = net->Arc[TR][PRE][currenttransition][i];
                if(net->Mult[TR][PRE][currenttransition][i] > ns.Current[candidate])
                {
                        scapegoat = candidate;
			if(Firelist::usedAsScapegoat[scapegoat])
			{
				// this place has already been used by another transition
				return scapegoat;
			}
                }
        }
        assert(scapegoat < net->CardArcs[TR][PRE][currenttransition]);
	// have a scapegoat that has not yet been used by another transition
	usedAsScapegoat[scapegoat] = true;
	return scapegoat;
}

void Firelist::scapegoatNewRound()
{
	memset(usedAsScapegoat,0,net->Card[PL]*sizeof(bool));
}
