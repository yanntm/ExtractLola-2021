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

template <typename T>
class Store;

class Mara;
/*!
\file
\author Niels
\author Max Görner
\author Christian Koch
\status new
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/NetState.h>
#include <Memory/Mara.h>
#include <Net/Petrinet.h>

/*!
\example

State *s = NULL;

if (searchAndInsert(&s))
{
    // already there - work with *s;
}
else
{
    // *s has just been created
}
*/
template <typename T>
class Store
{
private:

protected:
    /// the maximum number of threads this store has to work with.
    /// The value is used to create thread-local auxiliary data structures if needed.
    const threadid_t number_of_threads;

    /// the number of stored markings. It is stored as an array of integers, one value per thread, to avoid lost updates when counting concurrently. The sum of all values is the actual total number of markings found.
    int *markings;

    /// the number of calls to searchAndInsert(). It is stored as an array of integers, one value per thread, to avoid lost updates when counting concurrently. The sum of all values is the actual total number of calls made.
    int *calls;

public:
     Petrinet * net;
    Mara * memory;

    /// retrieves the current total number of markings stored.
    /// If the Store is used while calling this function, the return value might differ
    /// from the actual value (that continues increasing concurrently).
    /// The value is exact when the search is finished.
    int get_number_of_markings();


    /// retrieves the current total number of calls to searchAndInsert().
    /// If the Store is used while calling this function, the return value might differ
    /// from the actual value (that continues increasing concurrently).
    /// The value is exact when the search is finished.
    int get_number_of_calls();

    /// constructor; initialize auxiliary data structures if needed.
    /// @param number_of_threads maximum number of threads that may work with this Store concurrently.
    explicit Store(Petrinet * n, Mara * mem, threadid_t number_of_threads);

    /// destructor; frees all memory used for auxiliary data structures
    virtual ~Store();

    /// checks whether the marking represented by the given NetState is already stored.
    /// @param ns NetState that needs to be checked
    /// @param payload reference to address of payload data. If the marking is found, the address is set to the (already existing) payload entry of that marking, if a new marking was added the address will be set to a newly allocated payload entry.
    /// @param thread the index of the thread that requests this call. Values will range from 0 to (number_of_threads - 1). Used to allow using thread-local auxiliary data structures without locking any variables.
    /// @param noinsert if set to true only a search is done
    /// @return true, if the marking was found in the store, otherwise false.
    virtual bool searchAndInsert(NetState &ns, T **payload, threadid_t thread,
                                 bool noinsert = false) = 0;

    /// gets and removes a state from the store
    /// @param ns NetState where the removed state will be written to
    /// @return false, if store was already empty
    virtual bool popState(NetState &ns);
};

#include <Stores/Store.inc>
