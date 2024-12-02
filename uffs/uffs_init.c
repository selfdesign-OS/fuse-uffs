#include "uffs_types.h"
#include "uffs_public.h"

URET uffs_InitDevice(uffs_Device *dev)
{
	URET ret;

    // check pages_per_block is within valid range
    if (dev->attr->pages_per_block > UFFS_MAX_PAGES_PER_BLOCK) {
        printf("page_per_block should not exceed %d !", UFFS_MAX_PAGES_PER_BLOCK);
        return U_FAIL;
    }

	if (dev->mem.init) {
		if (dev->mem.init(dev) != U_SUCC) {
			printf("Init memory allocator fail.\n");
			return U_FAIL;
		}
	}

	// memset(&(dev->st), 0, sizeof(uffs_FlashStat));

	// uffs_DeviceInitLock(dev);
	// uffs_BadBlockInit(dev);

	ret = uffs_TreeInit(dev);
	if (ret != U_SUCC) {
		printf("fail to init tree buffers\n");
		// goto fail;
	}

	ret = uffs_BuildTree(dev);
	if (ret != U_SUCC) {
		printf("fail to build tree\n");
		// goto fail;
	}

	return U_SUCC;

// fail:
// 	uffs_DeviceReleaseLock(dev);

// 	return U_FAIL;
}