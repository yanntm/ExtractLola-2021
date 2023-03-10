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

\brief VectorStore implementation using binary prefix trees. Based on BinStore.
Relies on the assumption that different input vectors (possibly of different
length) are not prefix of another.
*/

#include <config.h>
#include <Net/Net.h>
#include <Stores/VectorStores/PrefixTreeStore.h>
#include <Memory/Mara.h>

template <typename T>
PrefixTreeStore<T>::PrefixTreeStore(threadid_t _number_of_threads)
{

    MemoryMara = new Mara();
    MemoryMara->setPageSize((size_t)(1024*RT::args.prealloc_arg));
    number_of_threads = _number_of_threads;
    branch = NULL;
    firstvector = NULL;
    // popVectorCache = (vectordata_t **) calloc(SIZEOF_VOIDP, number_of_threads);
    popVectorCache = (vectordata_t **) MemoryMara -> staticCalloc(SIZEOF_VOIDP , number_of_threads);

    

    pthread_rwlock_init(&rwlock, NULL);
}

template <typename T>
inline size_t PrefixTreeStore<T>::getPayloadSize()
{
    return sizeof(T);
}

//template <typename T>
//PrefixTreeStore<T>::~PrefixTreeStore()
//{
//    if (branch)
//    {
//        delete branch;
//    }
//    if (firstvector)
//    {
//        free(firstvector);
//    }
//
//    for (threadid_t i = 0; i < number_of_threads; ++i)
//        if (popVectorCache[i])
//        {
//            free(popVectorCache[i]);
//        }
//    free(popVectorCache);
//
//    pthread_rwlock_destroy(&rwlock);
//}
//
/// create a new branch in the decision tree at depth b.
template <typename T>
PrefixTreeStore<T>::Decision::Decision(bitarrayindex_t b) : bit(b) , vector(NULL), nextold(NULL),
    nextnew(NULL)
{
}

//template <typename T>
//PrefixTreeStore<T>::Decision::~Decision()
//{
//    if (vector)
//    {
//        free(vector);
//    }
//    if (nextnew)
//    {
//        delete nextnew;
//    }
//    if (nextold)
//    {
//        delete nextold;
//    }
//}

/// search for an input vector in the suffix tree and insert it, if it is not present
template <typename T>
bool PrefixTreeStore<T>::searchAndInsert(const vectordata_t *in, bitarrayindex_t bitlen, hash_t,
        T **payload, threadid_t, bool noinsert)
{
    /// If a new decision record is inserted, * anchor must point to it
    Decision **anchor;

    /// the place where the new vector goes to
    vectordata_t **newvector;

    /// the input word we are currently dealing with
    arrayindex_t input_index = 0;

    /// the last input word to consider.
    arrayindex_t max_input_index = (bitlen - 1) / VECTOR_WIDTH;

    /// the number of significant bits in the last input word (aligned at msb)
    arrayindex_t max_input_numbits = bitlen % VECTOR_WIDTH;
    if (!max_input_numbits)
    {
        max_input_numbits = VECTOR_WIDTH;
    }

    /// the bits of the current input word we have NOT dealt with so far
    bitarrayindex_t input_bitstogo = VECTOR_WIDTH; // indicates start with msb

    // pointer to the current input word
    const vectordata_t *pInput = in;


    /// the starting address of the current vector. In each vector, the payload is loacted before the marking data.
    vectordata_t *pPayload;

    /// the vector word we are currently investigating
    vectordata_t *pVector;

    /// the bits of the current vector word we have NOT dealt with so far
    bitarrayindex_t vector_bitstogo = VECTOR_WIDTH; // indicates start with msb

    /// the number of bits processed since reaching the current branch
    bitarrayindex_t position = 0;

    size_t payloadSize = getPayloadSize();

    // lock the current bucket to ensure thread safety
    if (noinsert)
    {
        pthread_rwlock_rdlock(&rwlock);
    }
    else
    {
        pthread_rwlock_wrlock(&rwlock);
    }

    // Is hash bucket empty? If so, assign to currentvector
    if (!(pPayload = firstvector))
    {
        if (noinsert)
        {
            pthread_rwlock_unlock(&rwlock);
            return false;
        }

        // Indeed, hash bucket is empty --> just insert vector, no branch yet.
        newvector = &firstvector;
    }
    else
    {
        // skip payload. Cast to char* is done to ensure skipping exaclty the right amount of bytes.
        pVector = (vectordata_t *)(((char *)pPayload) + payloadSize);

        // Here, hash bucket is not empty.
        anchor = &branch;

        while (true) // just entered new suffix tree vector
        {
            // number of bits to compare at once, searching for differences. Used only to locate the exact bit position of the difference
            bitarrayindex_t comparebits;

            // maximum number of vector words to consider. The actual vector (pVector) can be smaller, but in this case a difference is found before reaching the end anyway (see getInput rule 2).
            arrayindex_t vectorlen = ((bitlen - position) + (VECTOR_WIDTH - 1)) / VECTOR_WIDTH;


            // test for good alignment
            if (input_bitstogo == vector_bitstogo)
            {
                // good alignment, can use memcmp
                if (!memcmp(pInput, pVector, vectorlen * SIZEOF_VECTORDATA_T))
                {
                    // match found, we're done
                    pthread_rwlock_unlock(&rwlock);

                    if (payload)
                    {
                        *payload = (T *) pPayload;
                    }
                    return true;
                }
                // difference found, skip to first differing word.
                while (*pInput == *pVector) // reaching end of input is impossible
                {
                    position += VECTOR_WIDTH, pVector++;
                    input_index++, ++pInput;
                }
                comparebits = VECTOR_WIDTH >> 1; // initialize binary search for differing bit
            }
            else // bad alignment; input_bitstogo < vector_bitstogo, since vector_bitstogo is always VECTOR_WIDTH at this point
            {
                while (true)  // vector_bitstogo == VECTOR_WIDTH == VECTOR_WIDTH
                {
                    // compare remaining input bits with msb bits of current vector
                    if ((vectordata_t(*pInput << (VECTOR_WIDTH - input_bitstogo)))
                            ==
                            (vectordata_t((*pVector >> (VECTOR_WIDTH - input_bitstogo)) << (VECTOR_WIDTH - input_bitstogo))))
                    {
                        // they're equal, input word done. Test for EOI
                        if (++input_index <= max_input_index)
                        {
                            // compare msb of next input word with the remaining bits of the current vector word
                            if ((vectordata_t(*(++pInput) >> input_bitstogo) << input_bitstogo)
                                    ==
                                    (vectordata_t(*pVector << input_bitstogo)))
                            {
                                // they're equal, vector word done. Test for EOV and repeat with next vector word.
                                if (--vectorlen)
                                {
                                    position += VECTOR_WIDTH, pVector++;
                                }
                                else
                                {
                                    pthread_rwlock_unlock(&rwlock);
                                    if (payload)
                                    {
                                        *payload = (T *) pPayload;
                                    }
                                    return true;
                                }
                            }
                            else
                            {
                                // difference found. Update bitstogo variables and setup binary search for differing bit
                                vector_bitstogo -= input_bitstogo;
                                input_bitstogo = VECTOR_WIDTH;
                                comparebits = vector_bitstogo >> 1;
                                break;
                            }
                        }
                        else
                        {
                            pthread_rwlock_unlock(&rwlock);
                            if (payload)
                            {
                                *payload = (T *) pPayload;
                            }
                            return true;
                        }
                    }
                    else
                    {
                        // difference found. Setup binary search for differing bit
                        comparebits = input_bitstogo >> 1;
                        break;
                    }
                }
            }

            // difference was found in current input and vector words. locate the first differing bit using binary search.
            while (comparebits)
            {
                // test if next <comparebits> bits of input and vector are equal
                if ((vectordata_t(*pInput << (VECTOR_WIDTH - input_bitstogo)) >> (VECTOR_WIDTH - comparebits))
                        ==
                        (vectordata_t(*pVector << (VECTOR_WIDTH - vector_bitstogo)) >> (VECTOR_WIDTH - comparebits)))
                {
                    // they're equal, move forward
                    vector_bitstogo -= comparebits;
                    input_bitstogo -= comparebits;
                    if (comparebits > input_bitstogo)
                    {
                        comparebits = input_bitstogo >> 1;
                    }
                    if (comparebits > vector_bitstogo)
                    {
                        comparebits = vector_bitstogo >> 1;
                    }
                }
                else
                {
                    // they differ, repeat using halved comparebits
                    comparebits >>= 1;
                }
            }
            // we're now exactly at the bit that differs. Search for suiting branch
            // skipping all early branches
            while ((*anchor) && (position + (VECTOR_WIDTH - vector_bitstogo)) > (*anchor)->bit)
            {
                anchor = &((*anchor)->nextold);
            }
            // test whether there already is a branch at the differing bit
            if ((*anchor) && (*anchor)->bit == (position + (VECTOR_WIDTH - vector_bitstogo)))
            {
                // Indeed, there is. Switch to that branch and repeat with new suffix tree vector
                pPayload = (* anchor)->vector;
                pVector = (vectordata_t *)(((char *)pPayload) + payloadSize);
                anchor = &((* anchor)->nextnew);

                position += (VECTOR_WIDTH - vector_bitstogo) + 1;
                vector_bitstogo = VECTOR_WIDTH;

                // skip the differing bit. We don't need to store it since its value is determined by the old vector and the branch position.
                input_bitstogo--;
                if (input_index == max_input_index && input_bitstogo + max_input_numbits <= VECTOR_WIDTH)
                {
                    pthread_rwlock_unlock(&rwlock);
                    if (payload)
                    {
                        *payload = (T *) pPayload;
                    }
                    return true;
                }
                if (!input_bitstogo)
                {
                    input_index++, ++pInput, input_bitstogo = VECTOR_WIDTH;
                }
            }
            else
            {
                // there isn't. Place to insert new branch is found.
                break;
            }
        }

        // in case we do not want to insert the state, stop right now
        if (noinsert)
        {
            pthread_rwlock_unlock(&rwlock);
            return false;
        }

        // state not found --> prepare for insertion
	void * mmm = MemoryMara -> staticNew(sizeof(Decision));
        Decision *newdecision = new (mmm) Decision(position + (VECTOR_WIDTH - vector_bitstogo));
        newdecision->nextold = * anchor;
        * anchor = newdecision;
        newdecision->nextnew = NULL;
        newvector = &(newdecision->vector);
        // the mismatching bit itself is not represented in the new vector
        position += (VECTOR_WIDTH - vector_bitstogo) + 1;
        vector_bitstogo = VECTOR_WIDTH;

        input_bitstogo--;
        // difference occurred in the very last bit of the input string
        if (input_index == max_input_index && input_bitstogo + max_input_numbits == VECTOR_WIDTH)
        {
            // allocate space for payload if necessary
            if (payloadSize)
            {
                //*newvector = (vectordata_t *) calloc(payloadSize, 1);
                *newvector = (vectordata_t *) MemoryMara -> staticCalloc(payloadSize, 1);
                pPayload = *newvector;
            }

            pthread_rwlock_unlock(&rwlock);
            if (payload)
            {
                *payload = (T *) pPayload;
            }
            return false;
        }
        if (!input_bitstogo)
        {
            input_index++, ++pInput, input_bitstogo = VECTOR_WIDTH;
        }
    }

    assert(bitlen >= position);
    // vector_bitstogo is always VECTOR_WIDTH here
    int newvectorlen = ((bitlen - position) + (VECTOR_WIDTH - 1)) / VECTOR_WIDTH;
    // \TODO: Understand this construct and how it can be translated to a new
    //*newvector = (vectordata_t *) calloc(payloadSize + newvectorlen * SIZEOF_VECTORDATA_T, 1);
    *newvector = (vectordata_t *) MemoryMara->staticCalloc(payloadSize + newvectorlen * SIZEOF_VECTORDATA_T, 1);
    pPayload = *newvector;
    pVector = (vectordata_t *)(((char *)pPayload) + payloadSize);


    // test for good alignment
    if (input_bitstogo == VECTOR_WIDTH)
    {
        // good alignment, use memcpy
        memcpy(pVector, pInput, newvectorlen * SIZEOF_VECTORDATA_T);
        pthread_rwlock_unlock(&rwlock);
        if (payload)
        {
            *payload = (T *) pPayload;
        }
        return false;
    }
    else
    {
        // bad alignment, copy contents manually
        while (newvectorlen--)
        {
            *pVector |= vectordata_t(*pInput << (VECTOR_WIDTH - input_bitstogo));
            pInput++;
            if (++input_index > max_input_index)
            {
                break;
            }
            *pVector |= vectordata_t(*pInput >> input_bitstogo);
            pVector++;
        }
        pthread_rwlock_unlock(&rwlock);
        if (payload)
        {
            *payload = (T *) pPayload;
        }
        return false;
    }
}

/*!
 * \brief  gets and removes a vector from the store
 */
template <typename T>
bool PrefixTreeStore<T>::popVector(vectordata_t *&out, threadid_t threadIndex)
{
    /// size of resulting vector
    arrayindex_t vectorlen = SIZEOF_CAPACITY_T * Net::Card[PL];
    // create intermediate vector
    if (!(popVectorCache[threadIndex]))
    {
        popVectorCache[threadIndex] = (vectordata_t *) MemoryMara->staticNew(vectorlen);
    }

    // vector currently viewed
    vectordata_t *pPayload = NULL;
    vectordata_t *pVector = NULL;
    // anchor to a decision node
    Decision *anchor = NULL;
    Decision **pAnchor = NULL;

    // lock the current bucket to ensure thread safety
    pthread_rwlock_wrlock(&rwlock);

    if (!(pPayload = firstvector))
    {
        pthread_rwlock_unlock(&rwlock);
        return false;
    }
    // skip payload data
    pVector = (vectordata_t *)(((char *)pPayload) + getPayloadSize());

    // copy "first" vector to return it later
    memcpy(popVectorCache[threadIndex], pVector, vectorlen);
    out = popVectorCache[threadIndex];

    if (!(anchor = branch))
    {
        // no further markings in this bucket
        free(pPayload);
        firstvector = NULL;
        pthread_rwlock_unlock(&rwlock);
        return true;
    }

    // we start with the root of the decision tree
    pAnchor = &branch;
    // and search for the decision node differing as last from the "first" vector
    while (anchor->nextold)
    {
        pAnchor = &(anchor->nextold);
        anchor = anchor->nextold;
    }

    // alignment of both vectors (i.e. relative position of first bit of anchor->vector)
    bitarrayindex_t shift = (anchor->bit + 1) % VECTOR_WIDTH;
    // remaining words to copy
    bitarrayindex_t remaining = (vectorlen * 8 - anchor->bit - 1) / VECTOR_WIDTH;

    // go to position of first differing bit
    pVector += (anchor->bit / VECTOR_WIDTH);
    // toggle bit
    *pVector ^= 1 << (VECTOR_WIDTH - (anchor->bit % VECTOR_WIDTH) - 1);

    // vector to be copied from
    vectordata_t *source = anchor->vector;

    // test for good alignment
    if (shift == 0)
    {
        // good alignment, use memcpy
        memcpy(++pVector, source, remaining * SIZEOF_VECTORDATA_T);
    }
    else
    {
        // bad alignment, copy manually
        // initially clear recent word
        *pVector = ((*pVector >> (VECTOR_WIDTH - shift)) << (VECTOR_WIDTH - shift));

        while (true)
        {
            // copy lower half
            *(pVector++) |= *source >> shift;
            if (remaining-- == 0)
            {
                // finished copying
                break;
            }
            // copy upper half
            *pVector = *(source++) << (VECTOR_WIDTH - shift);
        }
    }

    // remove node
    *pAnchor = anchor->nextnew;
    anchor->nextnew = NULL; // prevent it from being deleted when calling the destructor
    delete anchor;

    // return result
    pthread_rwlock_unlock(&rwlock);
    return true;
}

/*!
 * \brief Check if the store is empty
 * \note This function does not ensure thread safety
 * \return If the store is empty
 */
template <typename T>
bool PrefixTreeStore<T>::empty()
{
    return !firstvector;
}

