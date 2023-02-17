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

\brief Class for firelist generation. Use up sets for state predicate
*/

#include <Core/Dimensions.h>
#include <Exploration/Firelist.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/FirelistStubbornEU.h>
#include <Exploration/StatePredicateProperty.h>
#include <Formula/StatePredicate/StatePredicate.h>
#include <Net/NetState.h>

FirelistStubbornEU::FirelistStubbornEU(Petrinet * n, StatePredicate *phi,StatePredicate *psi):Firelist(n), 
    phipredicate(phi), psipredicate(psi), dfsStack(new arrayindex_t[n->Card[TR]]), onStack(new bool[n->Card[TR]]()),
    upsetfirelist(new FirelistStubbornStatePredicate(n,psi))
{}

FirelistStubbornEU::~FirelistStubbornEU()
{
    delete[] dfsStack;
    delete[] onStack;
}

arrayindex_t FirelistStubbornEU::getFirelist(NetState &ns, arrayindex_t **result)
{
	arrayindex_t resultcard = upsetfirelist->getFirelist(ns,result);
	bool ok = true;
	bool hasenabled = false;
	for(arrayindex_t i = 0; i < resultcard;i++)
	{
		if(visible[(*result)[i]]) ok = false; break;
		if(ns.Enabled[(*result)[i]]) hasenabled = true;
	}
	if(ok && hasenabled) return resultcard;
	delete [] (*result);
	*result = new arrayindex_t[ns.CardEnabled];
	arrayindex_t rindex = 0;
        for(arrayindex_t i = 0; i < net->Card[TR];i++)
        {
                if(ns.Enabled[i])
                {
                        (*result)[rindex++] = i;
                }
        }
        return ns.CardEnabled;
}

Firelist *FirelistStubbornEU::createNewFireList(Petrinet * n, StatePredicateProperty *phiproperty, StatePredicateProperty * psiproperty)
{
    return new FirelistStubbornEU(n, phiproperty->predicate,psiproperty->predicate);
}
