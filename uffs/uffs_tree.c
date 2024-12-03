#include "uffs_config.h"
#include "uffs_public.h"
#include "uffs_os.h"
#include "uffs_pool.h"
#include "uffs_flash.h"
// #include "uffs_badblock.h"

#include <string.h>

#define TPOOL(dev) &(dev->mem.tree_pool)

/** 
 * \brief initialize tree buffers
 * \param[in] dev uffs device
 */
URET uffs_TreeInit(uffs_Device *dev)
{
	int size;
	int num;
	uffs_Pool *pool;
	int i;

	size = sizeof(TreeNode);
	num = dev->par.end - dev->par.start + 1;
	
	pool = &(dev->mem.tree_pool);

	if (dev->mem.tree_nodes_pool_size == 0) {
		if (dev->mem.malloc) {
			dev->mem.tree_nodes_pool_buf = dev->mem.malloc(dev, size * num);
			if (dev->mem.tree_nodes_pool_buf)
				dev->mem.tree_nodes_pool_size = size * num;
		}
	}
	if (size * num > dev->mem.tree_nodes_pool_size) {
		printf("Tree buffer require %d but only %d available.\n",
					size * num, dev->mem.tree_nodes_pool_size);
		memset(pool, 0, sizeof(uffs_Pool));
		return U_FAIL;
	}
	printf("alloc tree nodes %d bytes.\n", size * num);
	
	uffs_PoolInit(pool, dev->mem.tree_nodes_pool_buf,
					dev->mem.tree_nodes_pool_size, size, num, U_FALSE);

	dev->tree.erased = NULL;
	dev->tree.erased_tail = NULL;
	dev->tree.erased_count = 0;
	dev->tree.bad = NULL;
	dev->tree.bad_count = 0;

	for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		dev->tree.dir_entry[i] = EMPTY_NODE;
	}

	for (i = 0; i < FILE_NODE_ENTRY_LEN; i++) {
		dev->tree.file_entry[i] = EMPTY_NODE;
	}

	for (i = 0; i < DATA_NODE_ENTRY_LEN; i++) {
		dev->tree.data_entry[i] = EMPTY_NODE;
	}

	dev->tree.max_serial = ROOT_DIR_SERIAL;
	
	return U_SUCC;
}

TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev,
									  const char *name, u32 len,
									  u16 sum, u16 parent)
{
	int i;
	u16 x;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		x = tree->dir_entry[i];
		while (x != EMPTY_NODE) {
			node = FROM_IDX(x, TPOOL(dev));
			if (
                // node->u.dir.checksum == sum &&
					node->u.dir.parent == parent) {
				// //read file name from flash, and compare...
				// if (uffs_TreeCompareFileName(dev, name, len, sum,
				// 							node, UFFS_TYPE_DIR) == U_TRUE) {
				// 	//Got it!
				// 	return node;
				// }
                return node;
			}
			x = node->hash_next;
		}
	}

	return NULL;

}