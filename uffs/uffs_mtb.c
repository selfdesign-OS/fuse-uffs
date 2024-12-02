// #include "uffs_config.h"
#include "uffs_types.h"
#include "uffs_public.h"
#include "uffs_tree.h"
#include "uffs_mtb.h"
// #include "uffs_fd.h"
// #include "uffs_utils.h"
// #include "uffs_fs.h"
// #include "uffs_badblock.h"
#include <string.h>

int uffs_Mount(uffs_Device *dev) {
	
	dev->par.start = 0;
	dev->par.end = 127;
	
	if (dev->Init(dev) == U_FAIL) {
		printf("init device for mount point fail\n");
		return -1;
	}
	
	if (uffs_InitDevice(dev) != U_SUCC) {
		printf("init device fail !\n");
		return -1;
	}
}