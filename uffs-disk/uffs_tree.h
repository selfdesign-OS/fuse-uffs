#ifndef _UFFS_TREE_H_
#define _UFFS_TREE_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "uffs_types.h"
#include "uffs_disk.h"

#define EMPTY_NODE 0xffff				//!< special index num of empty node.

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


#define GET_FILE_HASH(serial)			(serial & FILE_NODE_HASH_MASK)
#define GET_DIR_HASH(serial)			(serial & DIR_NODE_HASH_MASK)
#define GET_DATA_HASH(parent, serial)	((parent + serial) & DATA_NODE_HASH_MASK)

//UFFS TreeNode (14 or 16 bytes)
typedef struct uffs_TreeNodeSt {
	union {
		struct DirhSt dir;
		struct FilehSt file;
		struct FdataSt data;
	} u;
	uint64_t hash_next;
	uint64_t hash_prev;
} TreeNode;

#define DIR_NODE_HASH_MASK		0x1f
#define DIR_NODE_ENTRY_LEN		(DIR_NODE_HASH_MASK + 1)

#define FILE_NODE_HASH_MASK		0x3f
#define FILE_NODE_ENTRY_LEN		(FILE_NODE_HASH_MASK + 1)

#define DATA_NODE_HASH_MASK		0x1ff
#define DATA_NODE_ENTRY_LEN		(DATA_NODE_HASH_MASK + 1)

struct uffs_TreeSt {
	uint64_t dir_entry[DIR_NODE_ENTRY_LEN];
	uint64_t file_entry[FILE_NODE_ENTRY_LEN];
	uint64_t data_entry[DATA_NODE_ENTRY_LEN];
	u16 max_serial;
};


#include "uffs_device.h"

URET uffs_TreeInit(uffs_Device *dev);

// URET uffs_TreeRelease(uffs_Device *dev);
URET uffs_BuildTree(uffs_Device *dev);
// u16 uffs_FindFreeFsnSerial(uffs_Device *dev);
TreeNode * uffs_TreeFindFileNode(uffs_Device *dev, u16 serial);
TreeNode * uffs_TreeFindFileNodeWithParent(uffs_Device *dev, u16 parent);
TreeNode * uffs_TreeFindDirNode(uffs_Device *dev, u16 serial);
TreeNode * uffs_TreeFindDirNodeWithParent(uffs_Device *dev, u16 parent);
TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info);
TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info);
TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial);
TreeNode * uffs_TreeFindDataNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent);

void uffs_InsertNodeToTree(uffs_Device *dev, u8 type, TreeNode *node);

// custom 
#define UDIR 0
#define DIR 1
#define ROOT_SERIAL 0
URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name, u8 *type, uffs_ObjectInfo* object_info);
URET uffs_TreeFindDirNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name);
URET uffs_TreeFindFileNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name);
URET initNode(uffs_Device *dev,TreeNode *node, data_Block *block, const char *path, u8 type);
URET uffs_TreeFindParentNodeByName(uffs_Device *dev, TreeNode **node, const char *name, int isNodeExist);

#endif