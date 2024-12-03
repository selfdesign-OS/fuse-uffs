#include "uffs_config.h"
#include "uffs_types.h"
#include "uffs_public.h"
#include "uffs_tree.h"
#include "uffs_mtb.h"
#include "uffs_fd.h"
// #include "uffs_utils.h"
#include "uffs_fs.h"
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

/**
 * find the matched mount point from a given full absolute path.
 *
 * \param[in] path full path
 * \return the length of mount point.
 */
int uffs_GetMatchedMountPointSize(const char *path)
{
	printf("[uffs_GetMatchedMountPointSize] path: %s", path);
	return 0;

	// int pos;
	// uffs_Device *dev;

	// if (path[0] != '/')
	// 	return 0;

	// pos = strlen(path);

	// while (pos > 0) {
	// 	if ((dev = uffs_GetDeviceFromMountPointEx(path, pos)) != NULL ) {
	// 		uffs_PutDevice(dev);
	// 		return pos;
	// 	}
	// 	else {
	// 		if (path[pos-1] == '/') 
	// 			pos--;
	// 		//back forward search the next '/'
	// 		for (; pos > 0 && path[pos-1] != '/'; pos--)
	// 			;
	// 	}
	// }

	// return pos;
}

void uffs_PutDevice(uffs_Device *dev)
{
	dev->ref_count--;
}