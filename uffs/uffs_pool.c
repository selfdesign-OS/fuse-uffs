#include "uffs_config.h" 
#include "uffs_types.h"
#include "uffs_os.h"
#include "uffs_public.h"
#include "uffs_pool.h"

/**
 * \brief Initializes the memory pool.
 * \param[in] pool memory pool
 * \param[in] mem pool memory
 * \param[in] mem_size size of pool memory
 * \param[in] buf_size size of a single buffer
 * \param[in] num_bufs number of buffers
 * \param[in] lock create semaphore for locking the memory pool
 * \return Returns U_SUCC if successful.
 */
URET uffs_PoolInit(uffs_Pool *pool,
				   void *mem, u32 mem_size, u32 buf_size, u32 num_bufs, UBOOL lock)
{
	unsigned int i;
	uffs_PoolEntry *e1, *e2;

	// if (!uffs_Assert(pool != NULL, "pool missing") ||
	// 	!uffs_Assert(mem != NULL, "pool memory missing") ||
	// 	!uffs_Assert(num_bufs > 0, "not enough buffers") ||
	// 	!uffs_Assert(buf_size % sizeof(void *) == 0,
	// 				"buffer size not aligned to pointer size") ||
	// 	!uffs_Assert(mem_size == num_bufs * buf_size,
	// 				"pool memory size is wrong"))
	// {
	// 	return U_FAIL;
	// }

	pool->mem = (u8 *)mem;
	pool->buf_size = buf_size;
	pool->num_bufs = num_bufs;

	pool->sem = OSSEM_NOT_INITED;
	if (lock) {
		uffs_SemCreate(&pool->sem);
		uffs_SemWait(pool->sem);
	}

	// Initialize the free_list
	e1 = e2 = pool->free_list = (uffs_PoolEntry *) pool->mem;
	for (i = 1; i < pool->num_bufs; i++) {
		e2 = (uffs_PoolEntry *) (pool->mem + i * pool->buf_size);
		e1->next = e2;
		e1 = e2;
	}
	e2->next = NULL;
	
	if (lock)
		uffs_SemSignal(pool->sem);

	return U_SUCC;
}

/**
 * \brief Get a buffer from the memory pool.
 * \param[in] pool memory pool
 * \return Returns a pointer to the buffer or NULL if none is available.
 */
void *uffs_PoolGet(uffs_Pool *pool)
{
	uffs_PoolEntry *e;

	// TODO: pool check
	// if (!uffs_Assert(pool != NULL, "pool missing"))
	// 	return NULL;

	e = pool->free_list;
	if (e)
		pool->free_list = e->next;

	return e;
}

/**
 * \brief Gets a buffer by index (offset).
 * This method returns a buffer from the memory pool by index.
 * \param[in] pool memory pool
 * \param[in] index index
 * \return Returns a pointer to the buffer.
 */
void *uffs_PoolGetBufByIndex(uffs_Pool *pool, u32 index)
{
	// TODO: pool check
	// if (!uffs_Assert(pool != NULL, "pool missing") ||
	// 	!uffs_Assert(index < pool->num_bufs,
	// 			"index(%d) out of range(max %d)", index, pool->num_bufs))
	// {
	// 	return NULL;
	// }

	return (u8 *) pool->mem + index * pool->buf_size;
}