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
#include <Stores/NetStateEncoder/CopyEncoder.h>

CopyEncoder::CopyEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all significant places.
    insize = net->CardSignificant;
}

CopyEncoder::~CopyEncoder()
{
}

capacity_t *CopyEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t)
{
    // calculate the length (in bit) of the returned vector.
    bitlen = insize * VECTOR_WIDTH;
    return ns.Current;
}

LTLCopyEncoder::LTLCopyEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all significant places.
    insize = net->CardSignificant+1;
        // initialize auxiliary vectors that hold the memcpy result (one for each thread)
        inputs = new capacity_t *[numThreads];
        for (int i = 0; i < numThreads; i++)
        {
            inputs[i] = new capacity_t[insize];
        }
}

LTLCopyEncoder::~LTLCopyEncoder()
{
    // free input vectors (if at all used)
    // if condition (a) is met, none of this code is required
    // check if condition (b) is met. The memory was allocated (and therefore needs to be freed) only if it is not.
        for (int i = 0; i < numThreads; i++)
        {
            delete[] inputs[i];
        }
        delete[] inputs;
}

capacity_t *LTLCopyEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t threadIndex)
{
    // calculate the length (in bit) of the returned vector.
    bitlen = insize * VECTOR_WIDTH;

    memcpy(inputs[threadIndex], ns.Current, net->CardSignificant * SIZEOF_CAPACITY_T);
    inputs[threadIndex][net->CardSignificant] = ns.buchistate;
    return inputs[threadIndex];
}
