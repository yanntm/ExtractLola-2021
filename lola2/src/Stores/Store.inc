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
\author Niels
\author Max Görner
\author Christian Koch
\status new
*/

#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Stores/EmptyStore.h>
#include <Core/Runtime.h>


template <typename T>
Store<T>::Store(Petrinet * n, Mara * mem,threadid_t _number_of_threads) :
    number_of_threads(_number_of_threads), markings(0), calls(0)
{
     net = n;
    calls = new int[number_of_threads]();
    markings = new int[number_of_threads]();
    memory = mem;
}

template <typename T>
Store<T>::~Store()
{
    delete[] calls;
    delete[] markings;
}

template <typename T>
int Store<T>::get_number_of_markings()
{
    int result = 0;
    for (int thread_number = 0; thread_number < number_of_threads; thread_number++)
    {
        result += markings[thread_number];
    }
    return result;
}

template <typename T>
int Store<T>::get_number_of_calls()
{
    int result = 0;
    for (int thread_number = 0; thread_number < number_of_threads; thread_number++)
    {
        result += calls[thread_number];
    }
    // First call is for initial state, it does not count for number of fired transitions....
    if(result) 
    {
	--result;
    }
    return result;
}

template <typename T>
bool Store<T>::popState(NetState &)
{
    RT::rep->status("popState method not yet implemented for this store");
    RT::rep->abort(ERROR_COMMANDLINE);

    // line is never reached, but silences the compiler
    return false;
}
