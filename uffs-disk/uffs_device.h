#ifndef UFFS_DEVICE_H
#define UFFS_DEVICE_H

#include "uffs_types.h"
#include "uffs_tree.h"

/** 
 * \struct uffs_DeviceSt
 * \brief The core data structure of UFFS, all information needed by manipulate UFFS object
 * \note one partition corresponding one uffs device.
 */
typedef struct uffs_DeviceSt {
	struct uffs_TreeSt	tree;		//!< tree list of block
	int					fd;
} uffs_Device;

#endif