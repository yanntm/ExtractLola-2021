//
// Created by marian on 07.03.17.
//

//#include <malloc.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <assert.h>
#include <stdlib.h>
#include <cstdlib>
#include <Core/Dimensions.h>

#include "Memory/Mara.h"

int Mara::PagesInUse = 0;
size_t Mara::pageSize = 512 << 20;

Mara::Mara()
{
	startOfPage = NULL;
	endOfPage = NULL;
	topOfStack = NULL;
	pagelimit = 0;
	cardPages = 0;
	mypages = NULL;
	stop = false;
	stopped = false;
}

Mara::~Mara()
{
	while(mypages)
	{
		pagelist * tmp = mypages;
		mypages = mypages -> next;
		tmp -> next = freePages;
		freePages = tmp;
	}
	cardPages = 0;
}

pagelist * Mara::freePages = NULL;
int Mara::globalCardPages = INT_MAX;

void* Mara::staticNew(size_t sizeInBytes) {
     if(stop)
     {
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	stopped = true;
	while(mypages)
	{
		pagelist * tmp = mypages;
		mypages = mypages -> next;
		tmp -> next = freePages;
		freePages = tmp;
	}
	cardPages = 0;
	startOfPage = NULL;
	endOfPage = NULL;
	topOfStack = NULL;
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
        pthread_exit(NULL);
	return NULL;
     }
    //in case we get a request with size 0
    if(UNLIKELY(!sizeInBytes)) return NULL;
    //if we don't have a page yet
    if(UNLIKELY(!startOfPage)) {
        //if we couldn't get one
        if(UNLIKELY(!createNewPage())) return malloc(sizeInBytes);
    }
    //if the request doesn't fit into the page
    if(UNLIKELY(topOfStack + sizeInBytes > endOfPage)){
        //if we couldn't get a new one
        if(UNLIKELY(!createNewPage())) return malloc(sizeInBytes);
    }
    //default case: shift the top of stack, return the old topOfStack as pointer
    byte* p = topOfStack;
    topOfStack = topOfStack+sizeInBytes;
    assert(startOfPage);
    assert(topOfStack);
    assert(endOfPage);
    assert(startOfPage < topOfStack);
    assert(endOfPage >= topOfStack);
    return p;
}

int Mara::createNewPage(){
    pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
    // handle quick check ressource limit
    if(pagelimit && (cardPages >= pagelimit))
    {
	portfoliomanager::status[taskid] = CANCELED_MEM;
	portfoliomanager::derail(taskid);
	RT::rep->status("CANCELED task # %d (type %s) for %s (memory limit exceeded)", taskid, portfoliomanager::tasktype2string(portfoliomanager::attributes[taskid]),portfoliomanager::name[portfoliomanager::roottask[taskid]]);
	while(mypages)
	{
		pagelist * tmp = mypages;
		mypages = mypages -> next;
		tmp -> next = freePages;
		freePages = tmp;
	}
	cardPages = 0;
	startOfPage = NULL;
	endOfPage = NULL;
	topOfStack = NULL;
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	pthread_exit(NULL);
	return 0;
    }
    if(freePages)
    {
	pagelist * tmp = freePages;
	freePages = freePages -> next;
	tmp -> next = mypages;
	mypages = tmp;
	cardPages++;
	startOfPage = topOfStack = tmp -> page;
	endOfPage = startOfPage + pageSize;
    pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return 1;
    }

    startOfPage = (byte *) malloc(pageSize);
    if(UNLIKELY(!startOfPage)){
	RT::rep->status("Failed to get new memory from operating system");
	portfoliomanager::memorylimit =  portfoliomanager::memorylimit < pageSize * PagesInUse ? portfoliomanager::memorylimit : pageSize * PagesInUse;
	if(cardPages >= PagesInUse) 
	{
		// I am using all ressources
		portfoliomanager::status[taskid] = CANCELED_MEM;
		portfoliomanager::derail(taskid);
	RT::rep->status("CANCELED task # %d (type %s) for %s (OS memory exhausted)", taskid, portfoliomanager::tasktype2string(portfoliomanager::attributes[taskid]),portfoliomanager::name[portfoliomanager::roottask[taskid]]);
		while(mypages)
		{
			pagelist * tmp = mypages;
			mypages = mypages -> next;
			tmp -> next = freePages;
			freePages = tmp;
		}
		cardPages = 0;
	startOfPage = NULL;
	endOfPage = NULL;
	topOfStack = NULL;
		pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
		pthread_exit(NULL);
		return 0;
	}
	portfoliomanager::status[taskid] = SUSPENDED;
	portfoliomanager::consumedtime[taskid] += difftime(time(NULL),portfoliomanager::starttime[taskid]);
        if(cardPages > portfoliomanager::consumedmemory[taskid])
        {
                portfoliomanager::consumedmemory[taskid] = cardPages;
        }
	RT::rep->status("SUSPEND task # %d (type %s) for %s (waiting for memory from other tasks)", taskid, portfoliomanager::tasktype2string(portfoliomanager::attributes[taskid]),portfoliomanager::name[portfoliomanager::roottask[taskid]]);
	pthread_cond_wait(&portfoliomanager::suspended_cond[taskid], &portfoliomanager::portfolio_mutex);
	// In suspension, portfolio manager will allocate a page for us before waking us up
	RT::rep->status("RESUME task # %d (type %s) for %s (got memory from other tasks)", taskid, portfoliomanager::tasktype2string(portfoliomanager::attributes[taskid]),portfoliomanager::name[portfoliomanager::roottask[taskid]]);
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
	return 1;
    }
    pagelist * tmp = new pagelist;
    tmp -> page = startOfPage;
    tmp -> next = mypages;
    mypages = tmp;
    endOfPage = startOfPage + pageSize;
    topOfStack = startOfPage;
    cardPages++;
    PagesInUse++;
    pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
    return 1;
}

void Mara::assignPage()
{
    	assert(freePages);
	pagelist * tmp = freePages;
	freePages = freePages -> next;
	tmp -> next = mypages;
	mypages = tmp;
	cardPages++;
	startOfPage = topOfStack = tmp -> page;
	endOfPage = startOfPage + pageSize;
}

void * Mara::staticCalloc(size_t A, size_t B)
{
	size_t C = A * B;
	void * mmm = (byte *) staticNew(C);
	memset(mmm,'\0',C);
	return mmm;
}

void Mara::reset()
{
	pageSize = DEFAULT_PAGE_SIZE;
	startOfPage = NULL;
	endOfPage = NULL;
	topOfStack = NULL;
	pagelimit = 0;
	cardPages = 0;
	mypages = NULL;
}
