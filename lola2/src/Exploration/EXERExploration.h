/****************************************************************************
  This file is part of LoLA.

  LoLA is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  LoLA is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MEXERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
  more details.

  You should have received a copy of the GNU Affero General Public License
  along with LoLA. If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

/*!
\file
\author Karsten
\status new

\brief Evaluates EXER property 
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Net/Petrinet.h>
#include <Exploration/StatePredicateProperty.h>

class ChooseTransition;
class Firelist;
class NetState;

/*!
\author Karsten
\status new

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

class EXERStackEntry
{
  public:
    /// ordinary constructor for entry
    EXERStackEntry(arrayindex_t *f, arrayindex_t c, int *p)
    {
        assert(f); //firelist, first processed in firelist
        current = c;
        fl = f;
        payload = p;
    }
    /// copy constructor used by the search stack
    EXERStackEntry(const EXERStackEntry &src)
    {
        current = src.current;
        fl = new arrayindex_t[current + 1];
        memcpy(fl, src.fl, (current + 1) * SIZEOF_ARRAYINDEX_T);
        payload = src.payload;
    }
    ~EXERStackEntry()
    {
        if (fl)
        {
            delete[] fl;
        }
        fl = NULL;
    }
    arrayindex_t *fl;     // array to take a firelist
    arrayindex_t current; // index of first processed element of fl
    int *payload;
};

class EXERExploration
{
  public:
    Petrinet * net;
    EXERExploration(Petrinet * n):_p(n) {net = n;}
    /// evaluates a given property by standard depth-first-search
    bool virtual depth_first(StatePredicateProperty &phiproperty, StatePredicateProperty &psiproperty, NetState &ns, Store<int> &myStore,
                             Firelist &myFirelist, int threadNumber);

    /// return a witness path (currently only for find_path)
    Path path() const;

    virtual ~EXERExploration() {}
    SearchStack<EXERStackEntry> dfsstack;

  protected:
    /// the witness path (created by calling path())
    Path _p;
};
