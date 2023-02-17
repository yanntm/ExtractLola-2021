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

#include <config.h>
#include <Core/Dimensions.h>
#include <CoverGraph/CoverPayload.h>
#include <Planning/StoreCreator.h>
#include <InputOutput/InputOutput.h>
#include <Stores/Store.h>
#include <SweepLine/SweepEmptyStore.h>
#include <Symmetry/Symmetry.h>
#include <Memory/Mara.h>

/*!
\ingroup g_globals
\todo Is this mapping actually needed or was it this just added for debugging
purposes.
*/


/// special store creation for stores without payload support (e.g. BloomStore)
template<>
Store<void> *StoreCreator<void>::createSpecializedStore(Petrinet * net, Mara * mem, threadid_t number_of_threads)
{
    NetStateEncoder *enc = 0;

    if(net->cardbuchistates < 0) // we are not in the LTL business
    {
	    switch (RT::args.encoder_arg)
	    {
	    case encoder_arg_bit:
		if(net -> isSkeleton)
		{
RT::rep->status("HAI");
			enc = new SimpleCompressedEncoder(net,number_of_threads);
		}
		else
		{
			enc = new BitEncoder(net,number_of_threads);
		}
		break;
	    case encoder_arg_copy:
		enc = new CopyEncoder(net,number_of_threads);
		break;
	    case encoder_arg_simplecompressed:
		enc = new SimpleCompressedEncoder(net,number_of_threads);
		break;
	    case encoder_arg_fullcopy:
		enc = new FullCopyEncoder(net,number_of_threads);
		break;
	    case encoder__NULL:
		assert(false);
	    }
    }
    else
    {
	    switch (RT::args.encoder_arg)
	    {
	    case encoder_arg_bit:
		if(net -> isSkeleton)
		{
			enc = new LTLSimpleCompressedEncoder(net,number_of_threads);
		}
		else
		{
			enc = new LTLBitEncoder(net,number_of_threads);
		}
		break;
	    case encoder_arg_copy:
		enc = new LTLCopyEncoder(net,number_of_threads);
		break;
	    case encoder_arg_simplecompressed:
		enc = new LTLSimpleCompressedEncoder(net,number_of_threads);
		break;
	    case encoder_arg_fullcopy:
		enc = new LTLFullCopyEncoder(net,number_of_threads);
		break;
	    case encoder__NULL:
		assert(false);
	    }
    }

    Store<void> * st;
    switch (RT::args.store_arg)
    {
    case store_arg_bloom:

        if (RT::args.bucketing_given)
        {
            st = new PluginStore<void>(enc,
                                         new HashingWrapperStore<void>(new BinaryVectorStoreCreator < void,
                                                 VBloomStore < (unsigned long)  BLOOM_FILTER_SIZE / SIZEOF_MARKINGTABLE + 1 > , arrayindex_t,
                                                 size_t > (net,mem,number_of_threads, RT::args.hashfunctions_arg),net,mem), net,mem,number_of_threads);
        }
        else
        {
            st = new PluginStore<void>(enc,
                                         new VBloomStore<BLOOM_FILTER_SIZE>(net,mem,number_of_threads,
                                                 RT::args.hashfunctions_arg), net,mem, number_of_threads);
        }
	break;

    default:
        storeCreationError();
        return NULL;
    }
 // symmetry reduction: wrap created store
        if (RT::args.symmetry_given)
        {
            // wrap current store in CycleStore object
            st = new SymmetryStore<void>(net,mem,number_of_threads, st);
        }
        return st;

}

