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
\author Christian Koch
\status new

\brief NetStateEncoder implementation that copies the marking, while ignoring
capacity limitations. The copy operation isn't done at all if possible (just
passing the marking pointer), otherwise memcpy is used
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Stores/PluginStore.h>

class FullCopyEncoder : public NetStateEncoder
{
public:
    /// constructor
    /// @param numThreads maximum number of threads that may work with this NetStateEncoder concurrently.
    explicit FullCopyEncoder(Petrinet *, int numThreads);
    /// destructor
    ~FullCopyEncoder();

    /// converts the given NetState marking into an input vector.
    /// @param ns input: NetState that needs to be converted
    /// @param bitlen output: reference to length of input vector (in bits). Will be filled by the method.
    /// @param threadIndex input: the index of the thread that requests this call. Values will range from 0 to (numThreads - 1). Used to allow using thread-local auxiliary data structures without locking any variables.
    /// @return the final input vector.
    unsigned int *encodeNetState(NetState &ns, bitarrayindex_t &bitlen, arrayindex_t threadIndex);

    /// decodes a given encoded state and sets the netstate appropriately
    /// @param ns NetState the decoded state will be written to
    /// @param data vector to be decoded
    virtual void decodeNetState(NetState &ns, unsigned int *data);

private:
    /// number of words in input vector
    arrayindex_t insize;

    /// instead of copying the data, the marking pointer can be casted directly into the input vector pointer.
};

class LTLFullCopyEncoder : public NetStateEncoder
{
public:
    /// constructor
    /// @param numThreads maximum number of threads that may work with this NetStateEncoder concurrently.
    explicit LTLFullCopyEncoder(Petrinet *, int numThreads);
    /// destructor
    ~LTLFullCopyEncoder();

    /// converts the given NetState marking into an input vector.
    /// @param ns input: NetState that needs to be converted
    /// @param bitlen output: reference to length of input vector (in bits). Will be filled by the method.
    /// @param threadIndex input: the index of the thread that requests this call. Values will range from 0 to (numThreads - 1). Used to allow using thread-local auxiliary data structures without locking any variables.
    /// @return the final input vector.
    unsigned int *encodeNetState(NetState &ns, bitarrayindex_t &bitlen, arrayindex_t threadIndex);

    /// decodes a given encoded state and sets the netstate appropriately
    /// @param ns NetState the decoded state will be written to
    /// @param data vector to be decoded
    virtual void decodeNetState(NetState &ns, capacity_t *data);

private:
    /// number of words in input vector
    arrayindex_t insize;
    bool nocopy;
    /// vector of input vectors that are returned from encodeNetState. Each thread has its own vector to avoid conflicts. Only needed if memcpy is actually used.
    capacity_t **inputs;
};
