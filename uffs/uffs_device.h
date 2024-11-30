#ifndef UFFS_DEVICE_H
#define UFFS_DEVICE_H

#include "uffs_types.h"
#include "uffs_buf.h"
#include "uffs_pool.h"
#include "uffs_tree.h"
#include "uffs_mem.h"
#include "uffs_core.h"
#include "uffs_flash.h"

// /**
//  * \def MAX_DIRTY_BUF_GROUPS
//  */
// #define MAX_DIRTY_BUF_GROUPS    3

/** 
 * \struct uffs_PageBufDescSt
 * \brief uffs page buffers descriptor
 */
struct uffs_PageBufDescSt {
	uffs_Buf *head;			//!< head of buffers (double linked list)
	uffs_Buf *tail;			//!< tail of buffers (double linked list)
	uffs_Buf *clone;		//!< head of clone buffers (single linked list)
	// struct uffs_DirtyGroupSt dirtyGroup[MAX_DIRTY_BUF_GROUPS];	//!< dirty buffer groups
	int buf_max;			//!< maximum buffers
	int dirty_buf_max;		//!< maximum dirty buffer allowed
	void *pool;				//!< memory pool for buffers
};

/** 
 * \struct uffs_PageCommInfoSt
 * \brief common data for device, should be initialised at early
 * \note it is possible that pg_size is smaller than physical page size, but normally they are the same.
 * \note page data layout: [HEADER] + [DATA]
 */
struct uffs_PageCommInfoSt {
	u16 pg_data_size;			//!< page data size
	u16 header_size;			//!< header size
	u16 pg_size;				//!< page size
};


/** 
 * \struct uffs_DeviceSt
 * \brief The core data structure of UFFS, all information needed by manipulate UFFS object
 * \note one partition corresponding one uffs device.
 */
struct uffs_DeviceSt {
	URET (*Init)(uffs_Device *dev);				//!< low level initialisation
	URET (*Release)(uffs_Device *dev);			//!< low level release
	void *_private;								//!< private data for device

	struct uffs_StorageAttrSt		*attr;		//!< storage attribute
	// struct uffs_FlashOpsSt			*ops;		//!< flash operations
	struct uffs_PageBufDescSt		buf;		//!< page buffers
	struct uffs_PageCommInfoSt		com;		//!< common information
	struct uffs_TreeSt				tree;		//!< tree list of block
	// struct uffs_FlashStatSt			st;			//!< statistic (counters)
	struct uffs_memAllocatorSt		mem;		//!< uffs memory allocator
	u32	ref_count;								//!< device reference count
	int	dev_num;								//!< device number (partition number)	
};

#endif