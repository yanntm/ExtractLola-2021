#include<Core/mysemaphore.h>
mysemaphore::mysemaphore(unsigned int v)
{	
	value = v;
	pthread_mutex_init(&mymutex,NULL);
	pthread_cond_init(&mycondition,NULL);
}

void mysemaphore::get()
{
	pthread_mutex_lock(&mymutex);
	while(!value)
	{
		pthread_cond_wait(&mycondition,&mymutex);
	}
	value--;
	pthread_mutex_unlock(&mymutex);
}

void mysemaphore::put()
{
	pthread_mutex_lock(&mymutex);
	value++;
	pthread_cond_signal(&mycondition);
	pthread_mutex_unlock(&mymutex);
}
