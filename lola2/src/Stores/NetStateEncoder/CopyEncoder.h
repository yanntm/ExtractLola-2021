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

class CopyEncoder : public NetStateEncoder
{
public:
    explicit CopyEncoder(Petrinet *, int numThreads);
    ~CopyEncoder();

    unsigned int *encodeNetState(NetState &ns, bitarrayindex_t &bitlen, arrayindex_t threadIndex);

private:
    /// number of words in input vector
    arrayindex_t insize;

    /// instead of copying the data, the marking pointer can be casted directly into the input vector pointer.
};

class LTLCopyEncoder : public NetStateEncoder
{
public:
    explicit LTLCopyEncoder(Petrinet *, int numThreads);
    ~LTLCopyEncoder();

    unsigned int *encodeNetState(NetState &ns, bitarrayindex_t &bitlen, arrayindex_t threadIndex);

private:
    /// number of words in input vector
    arrayindex_t insize;

    unsigned int **inputs;
};

