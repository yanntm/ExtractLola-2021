#include<pthread.h>

#pragma once

class mysemaphore
{
public: 
	mysemaphore(unsigned int);
	void get();
	void put();
private:
	unsigned int value;
	pthread_mutex_t mymutex;
	pthread_cond_t mycondition;
};
