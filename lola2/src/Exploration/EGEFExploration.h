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

Evaluates EGEF property
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Exploration/SearchStack.h>
#include <Net/Petrinet.h>

class Firelist;
class NetState;
class StatePredicateProperty;

/*!
\author Karsten
\status new
\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

class EGEFStackFrame
{
public:
	EGEFStackFrame(arrayindex_t * f, arrayindex_t c, int d, int * s)
	{
		fl = f;
		current = c;
		dfs = d;
		stinfo = s;;
	}
	arrayindex_t * fl;
	arrayindex_t current;
	int dfs;
	int * stinfo;
};
class EGEFExploration
{
public:
	Petrinet * net;
	EGEFExploration(Petrinet * n):_p(n){net = n;}
    bool depth_first(StatePredicateProperty &property, NetState &ns, Store<int> &myStore,
                             Firelist & , int threadNumber);

    /// return a witness path (currently only for find_path)
    Path path() const;
    SearchStack<EGEFStackFrame> dfsstack;

    virtual ~EGEFExploration() {}

    SearchStack<EGEFStackFrame> stack;
protected:
    /// the witness path (created by calling path())
    Path _p;
};
