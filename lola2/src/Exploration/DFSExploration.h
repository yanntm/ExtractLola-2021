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

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <SweepLine/Sweep.h>
#include <Memory/Mara.h>
#include <Net/Petrinet.h>
#include <Exploration/FirelistStubbornStatePredicate.h>
#include <Exploration/FirelistStubbornTsccAlwaysUpset.h>

struct CoverPayload;
class ChooseTransition;
class Firelist;
class NetState;
class SimpleProperty;
class SweepEmptyStore;

/*!
\author Karsten
\status new

\brief Evaluates simple property (only SET of states needs to be computed).
Actual property is a parameter of the constructor
*/
class DFSExploration
{
public:
    int attempts;
    int maxdepth;
	Petrinet * net;
	Mara * memory;
    DFSExploration(Petrinet * n, Mara * mem):_p(n){net = n; memory = mem;}
     Sweep<void> *s; // object for sweep-line method
    /// evaluates a given property by standard depth-first-search
    bool virtual depth_first(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                             Firelist &myFirelist, int threadNumber);

    static void check_quasilive(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                             FirelistStubbornStatePredicate &myFirelist, int threadNumber);
    static void check_constantplace(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                             FirelistStubbornTsccAlwaysUpset &myFirelist, int threadNumber);
    static void * Quasiliveness(void *);
    static void * Constantplace(void *);

    /// evaluate property by random walk without storing states.
    bool find_path(SimpleProperty &property, NetState &ns,
                   Firelist &, EmptyStore<void> &e, ChooseTransition &c);
    
    static void * findpathquasilivenessthread(void *);
    static void * findpathconstantplacegtthread(void *);
    static void * findpathconstantplaceltthread(void *);
    static void * findpathdeadlocklivenessthread(void *);
    static void * deadlockthread(void *);

    /// evaluate property by sweepline method.
    bool virtual sweepline(SimpleProperty &property, NetState &ns, SweepEmptyStore &myStore,
                           Firelist &myFirelist, int number_of_fronts, int number_of_threads, Mara * mem);

    /// evaluate property by breadth-first search through the coverability graph.
    ternary_t virtual cover_breadth_first(SimpleProperty &property, NetState &ns,
                                          Store<CoverPayload> &store,
                                          Firelist &firelist, int number_of_threads, formula_t type);

    /// return a witness path (currently only for find_path)
    Path path() const;

    virtual ~DFSExploration() {}

protected:
    /// the witness path (created by calling path())
    Path _p;
};
