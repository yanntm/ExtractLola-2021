//
// Created by marian on 07.03.17.
//


#ifndef MARA_PAGE_H
#define MARA_PAGE_H

#include "Memory/predefined.h"
#include "Core/Dimensions.h"
#include "Stores/Store.h"
#include "Portfolio/portfoliomanager.h"

#pragma once

class pagelist
{
public:
	pagelist * next;
	byte * page;
};

class Mara {

public:

    static size_t pageSize;

    byte *startOfPage; //equals top of stack on page creation
    byte *endOfPage ; //first byte not in the page anymore
    byte *topOfStack; //points to the next free byte

    int createNewPage(); /// allocate new page for task when all allocated pages are filled. Create if necessary
    void assignPage();  /// allocate new page to suspended task.
   

public:
    static pagelist * freePages;
    static int globalCardPages;
    pagelist * mypages;

    bool stop;
    bool stopped;
    int pagelimit;
    int cardPages;
    static int PagesInUse;
    int taskid;
    void reset();


/**
     * Reserves memory in the static sector. Memory in this sector is expected to live as long as the program. Memory
     * allocated with this function CANNOT be freed. Mara returns a pointer to the location with an unused block with the
     * given size and completely ignore this space in the future. The advantage is that these blocks will produce absolutely
     * overhead inside of its page.
     * \param sizeInByte size of the block you want to use
     * \return a pointer to the first byte of the block you want to use. After this operation the block will stay allocated
     * until complete program termination.
     */
    void *staticNew(size_t sizeInBytes);

    /**
     * Sets the default size for blocks allocated from the system. These blocks are used bei mara for internal memory management.
     * Calling staticNew and dynamicNew will allocate memory in one of these blocks. Because of internal structure, the
     * blocks cannot be greater than approximately 4GB.
     * \param sizeInByte default size for malloc calls from mara
     * \return 0 on success
     */
    int setPageSize(size_t sizeInByte);
    void * staticCalloc(size_t,size_t);
   
    Mara();
    ~Mara();



};


#endif //MARA_PAGE_H
