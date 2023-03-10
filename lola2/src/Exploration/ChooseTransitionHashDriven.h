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

\brief Selects a transition from a fire list based on counting hash values and
a priority imposed on the fire list
*/

#pragma once

#include<Net/Petrinet.h>
class ChooseTransition;

class ChooseTransitionHashDriven : public ChooseTransition
{
public:
    ChooseTransitionHashDriven(Petrinet * n);
    ~ChooseTransitionHashDriven();

    virtual arrayindex_t choose(NetState &ns, arrayindex_t cardfirelist, arrayindex_t *firelist);

private:
    int *table;
};
