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

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Core/Runtime.h>
#include <Stores/CompareStore.h>
#include <Stores/CycleStore.h>
#include <Stores/EmptyStore.h>
#include <Stores/NetStateEncoder/BitEncoder.h>
#include <Stores/NetStateEncoder/CopyEncoder.h>
#include <Stores/NetStateEncoder/FullCopyEncoder.h>
#include <Stores/NetStateEncoder/SimpleCompressedEncoder.h>
#include <Stores/PluginStore.h>
#include <Stores/Store.h>
#include <Stores/SymmetryStore.h>
#include <Stores/VectorStores/HashingWrapperStore.h>
#include <Stores/VectorStores/PrefixTreeStore.h>
#include <Stores/VectorStores/VBloomStore.h>
#include <Stores/VectorStores/VSTLStore.h>
#include <Symmetry/Constraints.h>
#include <Witness/Path.h>
#include <Memory/Mara.h>

// forward declarations
class AutomataTree;
class BuechiAutomata;
class LTLExploration;
struct CTLFormula;
class ChooseTransition;
class DFSExploration;
class TSCCExploration;
class Firelist;
class NetState;
class SimpleProperty;
class StatePredicate;
class CTLExploration;
struct CoverPayload;
class SymmetryCalculator;


/// generic store creator, basically a templatarized namespace for static creation methods.
/// Resolves the code duplication problem with using different templates in Task::setStore
template<typename T>
class StoreCreator
{
public:
    /// creates a new store based on the specified template and the command line arguments
    static Store<T> *createStore(Petrinet * net, Mara * mem, threadid_t number_of_threads = 1)
    {
        // create an encode according --encoder
        NetStateEncoder *enc = NULL;
	if(net->cardbuchistates < 1) // we are not in the LTL business
	{
		switch (RT::args.encoder_arg)
		{
		case encoder_arg_bit:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "bit-perfect";
	            if(net -> isSkeleton)
		    {
			    enc = new SimpleCompressedEncoder(net,number_of_threads);
		    }
		    else
		    {
			    enc = new BitEncoder(net,number_of_threads);
		    }
		    break;
		}

		case encoder_arg_copy:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "copy";
		    enc = new CopyEncoder(net,number_of_threads);
		    break;
		}

		case encoder_arg_simplecompressed:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "simple compression";
		    enc = new SimpleCompressedEncoder(net,number_of_threads);
		    break;
		}

		case encoder_arg_fullcopy:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "full copy";
		    enc = new FullCopyEncoder(net,number_of_threads);
		    break;
		}

		case encoder__NULL:
		{
		    // cannot happen
		    assert(false);
		}
		}
	}
	else
	{
		switch (RT::args.encoder_arg)
		{
		case encoder_arg_bit:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "bit-perfect";
		    if(net -> isSkeleton)
		    {
			    enc = new LTLSimpleCompressedEncoder(net,number_of_threads);
  		    }
		    else
		    {
			    enc = new LTLBitEncoder(net,number_of_threads);
		    }
		    break;
		}

		case encoder_arg_copy:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "copy";
		    enc = new LTLCopyEncoder(net,number_of_threads);
		    break;
		}

		case encoder_arg_simplecompressed:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "simple compression";
		    enc = new LTLSimpleCompressedEncoder(net,number_of_threads);
		    break;
		}

		case encoder_arg_fullcopy:
		{
		    //portfoliomanager::taskjson[mem->taskid]["configuration"]["encoder"] = "full copy";
		    enc = new LTLFullCopyEncoder(net,number_of_threads);
		    break;
		}

		case encoder__NULL:
		{
		    // cannot happen
		    assert(false);
		}
		}
	}

        // create a store according to --store
        Store<T> *st = NULL;

        switch (RT::args.store_arg)
        {
        case store_arg_comp:
        {
            //portfoliomanager::taskjson[mem->taskid]["configuration"]["store"] = "compare";
            st = new CompareStore<T>(
                new PluginStore<T>(enc, new PrefixTreeStore<T>(net,mem), net,mem, number_of_threads),
                new PluginStore<T>(enc, new VSTLStore<T>(net,mem,number_of_threads), net,mem,number_of_threads),
	net, 	mem,
                number_of_threads);
            break;
        }

        case store_arg_prefix:
        {
            //portfoliomanager::taskjson[mem->taskid]["configuration"]["store"] = "prefix";
            if (RT::args.bucketing_given)
            {
                st = new PluginStore<T>(enc, new HashingWrapperStore<T>(new
                                        NullaryVectorStoreCreator<T, PrefixTreeStore<T> >(net, mem),net, mem), net, mem,number_of_threads);
            }
            else
            {
                st = new PluginStore<T>(enc, new PrefixTreeStore<T>(net,mem), net,mem,number_of_threads);
            }
            break;
        }

        case store_arg_stl:
        {
            //portfoliomanager::taskjson[mem->taskid]["configuration"]["store"] = "stl";
            if (RT::args.bucketing_given)
            {
                st = new PluginStore<T>(enc, new HashingWrapperStore<T>(new
                                        UnaryVectorStoreCreator<T, VSTLStore<T>, arrayindex_t>(net,mem,number_of_threads),net, mem),net, mem,number_of_threads);
            }
            else
            {
                st = new PluginStore<T>(enc, new VSTLStore<T>(net,mem,number_of_threads), net,mem, number_of_threads);
            }
            break;
        }

        case store_arg_bloom:
        {
            //portfoliomanager::taskjson[mem->taskid]["configuration"]["store"] = "bloom";
            st = createSpecializedStore(net,mem,number_of_threads);
            break;
        }

        case store__NULL:
        {
            // cannot happen
            assert(false);
        }
        }

        // cycle reduction: wrap created store
        if (RT::args.cycle_given)
        {
            // wrap current store in CycleStore object
            st = new CycleStore<T>(net,mem,number_of_threads, st, RT::args.cycleheuristic_arg);
        }

        // symmetry reduction: wrap created store
        if (RT::args.symmetry_given)
        {
            // wrap current store in CycleStore object
            st = new SymmetryStore<T>(net,mem,number_of_threads, st);
        }

        return st;
    }

private:
    /// creates specialized stores for different payload types.
    /// the default implementation raises an error, but template specializations of it may create stores with restricted payload type compatibility.
    static Store<T> *createSpecializedStore(Petrinet *, Mara *,threadid_t)
    {
        storeCreationError();
        return NULL;
    }
    /// auxiliary method that raises an error. Will be called by the different specializations of createSpecializedStore.
    static void storeCreationError()
    {
        RT::rep->message(RT::rep->markup(MARKUP_BAD, "specified store does not fit the given task").str());
        RT::rep->abort(ERROR_COMMANDLINE);
    }
};

/// forward declaration of special store creation for stores without payload support (e.g. BloomStore)
template<>
Store<void> *StoreCreator<void>::createSpecializedStore(Petrinet * net,Mara * mem, threadid_t number_of_threads);


