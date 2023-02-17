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
\brief NetStateEncoder implementation that copies the marking bit-perfect with
no compression (analogous to BitStore).
*/

#include <config.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Net/Petrinet.h>
#include <Net/NetState.h>
#include <Stores/NetStateEncoder/BitEncoder.h>

BitEncoder::BitEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all significant bits.
    insize = ((net->SizeOfBitVector + VECTOR_WIDTH - 1) / VECTOR_WIDTH);


    // allocate auxiliary input vectors
    inputs = new capacity_t *[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        inputs[i] = new capacity_t[insize];
    }

    // determine the number of leading places suitable for memcpying (see declaration of memcpylen for details)
    memcpylen = 0;
    while (memcpylen < insize && net->CardBits[memcpylen] == PLACE_WIDTH)
    {
        memcpylen++;
    }
}

BitEncoder::~BitEncoder()
{
    // free all auxiliary input vectors
    for (int i = 0; i < numThreads; i++)
    {
        delete[] inputs[i];
    }
    delete[] inputs;
}

capacity_t *BitEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t threadIndex)
{
    // number of bits in input vector will be exactly the number of bits necessary to store all bits of the significant places.
    bitlen = net->SizeOfBitVector;
    // fetch input vector for this thread
    capacity_t *pCurThreadInput = inputs[threadIndex];

    // use memcpy when possible
    memcpy(pCurThreadInput, ns.Current, memcpylen * SIZEOF_CAPACITY_T);
    // test if we are already done
    if (memcpylen >= net->CardSignificant)
    {
        return pCurThreadInput;
    }

    /// the place we are currently dealing with (skipping all memcpyed places)
    arrayindex_t place_index = memcpylen;
    /// pointer to current place (skipping all memcpyed places)
    capacity_t *pPlace = ns.Current + memcpylen;
    /// the bits of the place's marking we have NOT dealt with so far
    bitarrayindex_t place_bitstogo = net->CardBits[place_index]; // start with msb

    /// pointer to current input word (skipping all memcpyed places)
    capacity_t *pInput = pCurThreadInput + memcpylen;
    /// the bits of the current input word we have NOT dealt with so far
    bitarrayindex_t input_bitstogo = VECTOR_WIDTH;

    // clear input vector (we are going to use bitwise or later on, so all bits have to be zero initially)
    memset(pInput, 0, (insize - memcpylen)*SIZEOF_CAPACITY_T);

    while (true)
    {
        // fill all bits into the input vector that are in the current word of both the input vector and the current place.
        // uses preprocessor to optimize operation (since this is the most time consuming step)
        *pInput |= capacity_t((capacity_t(*pPlace << (PLACE_WIDTH - place_bitstogo)) >>
                                 (PLACE_WIDTH - input_bitstogo)));

        // update position variables
        if (place_bitstogo < input_bitstogo)  // all place bits inserted, go to next place
        {
            place_index++, pPlace++;
            if (place_index >= net->CardSignificant)
            {
                break;
            }
            input_bitstogo -= place_bitstogo;
            place_bitstogo = net->CardBits[place_index];
        }
        else if (place_bitstogo > input_bitstogo)    // current input word full, go to next word
        {
            place_bitstogo -= input_bitstogo;
            input_bitstogo = VECTOR_WIDTH;
            ++pInput;
        }
        else     // both current input word and current place are done, go to next ones.
        {
            place_index++, pPlace++;
            if (place_index >= net->CardSignificant)
            {
                break;
            }
            place_bitstogo = net->CardBits[place_index];
            input_bitstogo = VECTOR_WIDTH;
            ++pInput;
        }
    }
    return pCurThreadInput;
}

LTLBitEncoder::LTLBitEncoder(Petrinet * n, int numThreads) : NetStateEncoder(n,numThreads)
{
    // compute number of input words necessary to store all significant bits.
    buchicapacity = Petrinet::Capacity2Bits(net -> cardbuchistates - 1);
    insize = ((net->SizeOfBitVector + buchicapacity + VECTOR_WIDTH - 1) / VECTOR_WIDTH);


    // allocate auxiliary input vectors
    inputs = new capacity_t *[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        inputs[i] = new capacity_t[insize];
    }

    // determine the number of leading places suitable for memcpying (see declaration of memcpylen for details)
    memcpylen = 0;
    while (memcpylen < insize && memcpylen < net -> CardSignificant && net->CardBits[memcpylen] == PLACE_WIDTH)
    {
        memcpylen++;
    }
}

LTLBitEncoder::~LTLBitEncoder()
{
    // free all auxiliary input vectors
    for (int i = 0; i < numThreads; i++)
    {
        delete[] inputs[i];
    }
    delete[] inputs;
}

capacity_t *LTLBitEncoder::encodeNetState(NetState &ns, bitarrayindex_t &bitlen,
        arrayindex_t threadIndex)
{
    // number of bits in input vector will be exactly the number of bits necessary to store all bits of the significant places.
    bitlen = net->SizeOfBitVector + buchicapacity;
    // fetch input vector for this thread
    capacity_t *pCurThreadInput = inputs[threadIndex];

    // use memcpy when possible
    memcpy(pCurThreadInput, ns.Current, memcpylen * SIZEOF_CAPACITY_T);

    /// the place we are currently dealing with (skipping all memcpyed places)
    arrayindex_t place_index = memcpylen;
    /// pointer to current place (skipping all memcpyed places)
    capacity_t *pPlace = ns.Current + memcpylen;
    /// the bits of the place's marking we have NOT dealt with so far
    bitarrayindex_t place_bitstogo = net->CardBits[place_index]; // start with msb

    /// pointer to current input word (skipping all memcpyed places)
    capacity_t *pInput = pCurThreadInput + memcpylen;
    /// the bits of the current input word we have NOT dealt with so far
    bitarrayindex_t input_bitstogo = VECTOR_WIDTH;

    // clear input vector (we are going to use bitwise or later on, so all bits have to be zero initially)
    memset(pInput, 0, (insize - memcpylen)*SIZEOF_CAPACITY_T);


    if (place_index < net->CardSignificant)
    {
	    while (true)
	    {
		// fill all bits into the input vector that are in the current word of both the input vector and the current place.
		// uses preprocessor to optimize operation (since this is the most time consuming step)
		*pInput |= capacity_t((capacity_t(*pPlace << (PLACE_WIDTH - place_bitstogo)) >>
					 (PLACE_WIDTH - input_bitstogo)));

		// update position variables
		if (place_bitstogo < input_bitstogo)  // all place bits inserted, go to next place
		{
		    place_index++, pPlace++;
		    input_bitstogo -= place_bitstogo;
		    place_bitstogo = net->CardBits[place_index];
		    if (place_index >= net->CardSignificant)
		    {
			break;
		    }
		}
		else if (place_bitstogo > input_bitstogo)    // current input word full, go to next word
		{
		    place_bitstogo -= input_bitstogo;
		    input_bitstogo = VECTOR_WIDTH;
		    ++pInput;
		}
		else     // both current input word and current place are done, go to next ones.
		{
		    place_index++, pPlace++;
		    place_bitstogo = net->CardBits[place_index];
		    input_bitstogo = VECTOR_WIDTH;
		    ++pInput;
		    if (place_index >= net->CardSignificant)
		    {
			break;
		    }
		}
	    }
    }
        // fill all bits into the input vector that are in the current word of both the input vector and the current place.
    while(true)
    {
	pPlace = &ns.buchistate;
	place_bitstogo = buchicapacity;
        // uses preprocessor to optimize operation (since this is the most time consuming step)
        *pInput |= capacity_t((capacity_t(*pPlace << (PLACE_WIDTH - place_bitstogo)) >>
                                 (PLACE_WIDTH - input_bitstogo)));

        // update position variables
        if (place_bitstogo > input_bitstogo)    // current input word full, go to next word
        {
            place_bitstogo -= input_bitstogo;
            input_bitstogo = VECTOR_WIDTH;
            ++pInput;
        }
        else     // both current input word and current place are done, go to next ones.
        {
                break;
        }
    }
    return pCurThreadInput;
}
