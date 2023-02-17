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

\brief Evaluates maximum value of a given expression over all reachable markings.
Expression is wrapped in an atomic state predicate.
*/

#pragma once

#include <config.h>
#include <Stores/Store.h>
#include <Witness/Path.h>
#include <Net/Petrinet.h>
#include<Exploration/FirelistStubbornComputeBound.h>

struct CoverPayload;
class ChooseTransition;
class AtomicStatePredicate;
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
class ComputeBoundExploration
{
public:
    Petrinet * net;
    ComputeBoundExploration(Petrinet * n):_p(n) {net = n;}
    /// evaluates a given property by standard depth-first-search
    int depth_first_num(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                             Firelist &myFirelist, int threadNumber);
	static void check_safety(SimpleProperty & property, NetState &ns, Store<void> &myStore,
        FirelistStubbornComputeBound &myFirelist, int);
	static int lp(Petrinet * net, AtomicStatePredicate * a);
    // remaining methods only here for syntactical completeness
    // LCOV_EXCL_START
    bool virtual depth_first(SimpleProperty &property, NetState &ns, Store<void> &myStore,
                             Firelist &myFirelist, int threadNumber){return false;} // for compatibility only

    /// evaluate property by random walk without storing states.
    bool find_path(SimpleProperty &property, NetState &ns,
                   Firelist &, EmptyStore<void> &e, ChooseTransition &c); // only to confirm structural bound

    /// evaluate property by sweepline method.
    bool virtual sweepline(SimpleProperty &property, NetState &ns, SweepEmptyStore &myStore,
                           Firelist &myFirelist, int number_of_fronts, int number_of_threads){return false;} // for compatibility only


    ternary_t virtual cover_breadth_first(SimpleProperty &property, NetState &ns,
                                          Store<CoverPayload> &store,
                                          Firelist &firelist, int number_of_threads, formula_t type){return TERNARY_FALSE;} // for compatibility only

    /// return a witness path (currently only for find_path)
    Path path() const;

    // LCOV_EXCL_STOP
    virtual ~ComputeBoundExploration() {}
    int result;
    int StructuralBound;
    static void * Safety (void *);
    static void * findpathsafetythread (void *);
    static void * zerooneinvariantthread(void *);
    unsigned int attempts;
    unsigned int maxdepth;

protected:
    /// the witness path (created by calling path())
    Path _p;
};
