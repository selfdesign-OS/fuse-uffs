/**
 * \file uffs_fd.c
 * \brief POSIX like, hight level file operations
 * \author Ricky Zheng, created 8th Jun, 2005
 */

#include <string.h>
#include "uffs_config.h"
#include "uffs_fs.h"
#include "uffs_fd.h"
// #include "uffs_utils.h"
// #include "uffs_version.h"
#include "uffs_mtb.h"
#include "uffs_public.h"
#include "uffs_find.h"

/**
 * \brief POSIX DIR
 */
struct uffs_dirSt {
    struct uffs_ObjectSt   *obj;		/* dir object */
    struct uffs_FindInfoSt f;			/* find info */
    struct uffs_ObjectInfoSt info;		/* object info */
    struct uffs_dirent dirent;			/* dir entry */
};

static int _dir_pool_data[sizeof(uffs_DIR) * MAX_DIR_HANDLE / sizeof(int)];
static uffs_Pool _dir_pool;
static int _uffs_errno = 0;

/**
 * initialise uffs_DIR buffers, called by UFFS internal
 */
URET uffs_DirEntryBufInit(void)
{
	return uffs_PoolInit(&_dir_pool, _dir_pool_data,
							sizeof(_dir_pool_data),
							sizeof(uffs_DIR), MAX_DIR_HANDLE, U_FALSE);
}