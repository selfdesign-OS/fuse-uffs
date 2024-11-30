#ifndef _UFFS_TREE_H_
#define _UFFS_TREE_H_

#include "uffs_types.h"
#include "uffs_device.h"
#include "uffs_core.h"

struct BlockListSt {	/* 12 bytes */
	struct uffs_TreeNodeSt * next;
	struct uffs_TreeNodeSt * prev;
	u16 block;
	union {
		u16 serial;			/* for suspended block list */
		u8 need_check;		/* for erased block list */
	} u;
};

struct DirhSt {		/* 8 bytes */
	u16 block;
	u16 checksum;	/* check sum of dir name */
	u16 parent;
	u16 serial;
};


struct FilehSt {	/* 12 bytes */
	u16 block;
	u16 checksum;	/* check sum of file name */
	u16 parent;
	u16 serial;
	u32 len;		/* file length total */
};

struct FdataSt {	/* 10 bytes */
	u16 block;
	u16 parent;
	u32 len;		/* file data length on this block */
	u16 serial;
};

//UFFS TreeNode (14 or 16 bytes)
typedef struct uffs_TreeNodeSt {
	union {
		struct BlockListSt list;
		struct DirhSt dir;
		struct FilehSt file;
		struct FdataSt data;
	} u;
	u16 hash_next;		
	u16 hash_prev;			
} TreeNode;

#define DIR_NODE_HASH_MASK		0x1f
#define DIR_NODE_ENTRY_LEN		(DIR_NODE_HASH_MASK + 1)

#define FILE_NODE_HASH_MASK		0x3f
#define FILE_NODE_ENTRY_LEN		(FILE_NODE_HASH_MASK + 1)

#define DATA_NODE_HASH_MASK		0x1ff
#define DATA_NODE_ENTRY_LEN		(DATA_NODE_HASH_MASK + 1)

struct uffs_TreeSt {
	TreeNode *erased;					//!< erased block list head
	TreeNode *erased_tail;				//!< erased block list tail
	int erased_count;					//!< erased block counter

	TreeNode *suspend;					//!< suspended block list, this is just a staging zone
										//   that prevent the serial number of the block be re-used.
	TreeNode *bad;						//!< bad block list
	int bad_count;						//!< bad block counter

	u16 dir_entry[DIR_NODE_ENTRY_LEN];
	u16 file_entry[FILE_NODE_ENTRY_LEN];
	u16 data_entry[DATA_NODE_ENTRY_LEN];
	u16 max_serial;
};