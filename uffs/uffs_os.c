#include "uffs_os.h"
#include "uffs_public.h"
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
// #include <windows.h>

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