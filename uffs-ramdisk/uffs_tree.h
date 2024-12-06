#ifndef _UFFS_TREE_H_
#define _UFFS_TREE_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "uffs_types.h"

#define UFFS_TYPE_DIR		0
#define UFFS_TYPE_FILE		1
#define UFFS_TYPE_DATA		2
#define UFFS_TYPE_RESV		3
#define UFFS_TYPE_INVALID	0xFF

#define MAX_FILENAME_LENGTH         128

#define EMPTY_NODE 0xffff				//!< special index num of empty node.
#define ROOT_DIR_SERIAL	0				//!< serial num of root dir

struct DirhSt {		/* 8 bytes */
	u16 block;
	u16 parent;
	u16 serial;
};

struct FilehSt {	/* 12 bytes */
	u16 block;
	u16 parent;
	u16 serial;
};

struct FdataSt {	/* 10 bytes */
	u16 block;
	u16 parent;
	u16 serial;
};

struct uffs_FileInfoSt {
    u32 create_time;
    u32 last_modify;
    u32 access;
    u32 reserved;
    u32 name_len;           //!< length of file/dir name
    char name[MAX_FILENAME_LENGTH];
	// TODO: warning
	short nlink;
	u32 len;
	u16 mode;
};
typedef struct uffs_FileInfoSt uffs_FileInfo;

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
	uffs_FileInfo info;
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
TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent);
TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent);
TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial);

void uffs_InsertNodeToTree(uffs_Device *dev, u8 type, TreeNode *node);

// custom 
#define UDIR 0
#define DIR 1
#define ROOT_SERIAL 0
URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name);

#endif