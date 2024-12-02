// #include "uffs_config.h"
#include "uffs_os.h"
#include "uffs_public.h"
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

int uffs_SemCreate(OSSEM *sem)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	int ret = -1;

	if (mutex) {
		ret = pthread_mutex_init(mutex, NULL /* default attr */);
		if (ret == 0) {
			*sem = (OSSEM *)mutex;			
		}
		else {
			free(mutex);
		}
	}
	
	return ret;
}

int uffs_SemWait(OSSEM sem)
{
	return pthread_mutex_lock((pthread_mutex_t *)sem);
}

int uffs_SemSignal(OSSEM sem)
{
	return pthread_mutex_unlock((pthread_mutex_t *)sem);;
}

int uffs_SemDelete(OSSEM *sem)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) (*sem);
	int ret = -1;
	
	if (mutex) {
		ret = pthread_mutex_destroy(mutex);
		if (ret == 0) {
			free(mutex);
			*sem = 0;
		}			
	}
	return ret;
}

static void * sys_malloc(struct uffs_DeviceSt *dev, unsigned int size)
{
	dev = dev;
    printf("system memory alloc %d bytes\n", size);
	return malloc(size);
}

static URET sys_free(struct uffs_DeviceSt *dev, void *p)
{
	dev = dev;
	free(p);
	return U_SUCC;
}

void uffs_MemSetupSystemAllocator(uffs_MemAllocator *allocator)
{
	allocator->malloc = sys_malloc;
	allocator->free = sys_free;
}