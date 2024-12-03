#include "uffs_config.h"
#include "uffs_public.h"
#include "uffs_os.h"
#include "uffs_pool.h"
#include "uffs_flash.h"
// #include "uffs_badblock.h"

#include <string.h>

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