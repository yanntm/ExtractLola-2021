#include <Formula/StatePredicate/MagicNumber.h>
#include <pthread.h>

int MagicNumber::nextMagicNumber = 0;
pthread_mutex_t magicmutex = PTHREAD_MUTEX_INITIALIZER;
int MagicNumber::assign()
{
	pthread_mutex_lock(&magicmutex);
	int result = nextMagicNumber--;
	pthread_mutex_unlock(&magicmutex);
	return result;
}
