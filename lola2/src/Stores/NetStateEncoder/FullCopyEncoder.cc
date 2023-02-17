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

#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Net/Petrinet.h>
#include <Net/NetState.h>
#include <Stores/NetStateEncoder/FullCopyEncoder.h>

FullCopyEncoder::FullCopyEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all places.
    insize = net->Card[PL];
}

FullCopyEncoder::~FullCopyEncoder()
{
}

capacity_t *FullCopyEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t)
{
    // calculate the length (in bit) of the returned vector.
    bitlen = insize * VECTOR_WIDTH;
    return ns.Current;
}

// decodes a given encoded state and sets the netstate appropriately
void FullCopyEncoder::decodeNetState(NetState &ns, capacity_t *data)
{
    // just copy the data back to the NetState
    memcpy(ns.Current, data, net->Card[PL] * SIZEOF_CAPACITY_T);
}

LTLFullCopyEncoder::LTLFullCopyEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all places.
    insize = net->Card[PL]+1;
        nocopy = false;
        inputs = new capacity_t *[numThreads];
        for (int i = 0; i < numThreads; i++)
        {
            inputs[i] = new capacity_t[insize];
        }
}

LTLFullCopyEncoder::~LTLFullCopyEncoder()
{
        for (int i = 0; i < numThreads; i++)
        {
            delete[] inputs[i];
        }
        delete[] inputs;
}

capacity_t *LTLFullCopyEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t threadIndex)
{
    bitlen = insize * VECTOR_WIDTH;
    memcpy(inputs[threadIndex], ns.Current, net->Card[PL] * SIZEOF_CAPACITY_T);
    inputs[threadIndex][net->Card[PL]] = ns.buchistate;
    return inputs[threadIndex];
}

// decodes a given encoded state and sets the netstate appropriately
void LTLFullCopyEncoder::decodeNetState(NetState &ns, capacity_t *data)
{
    // just copy the data back to the NetState
    memcpy(ns.Current, data, net->Card[PL] * SIZEOF_CAPACITY_T);
    ns.buchistate  = data[net->Card[PL]];
}
