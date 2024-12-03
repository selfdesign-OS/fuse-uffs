/**
 * \file uffs_fs.c
 * \brief basic file operations
 * \author Ricky Zheng, created 12th May, 2005
 */
#include "uffs_config.h"
#include "uffs_fs.h"
#include "uffs_pool.h"
// #include "uffs_ecc.h"
// #include "uffs_badblock.h"
#include "uffs_os.h"
#include "uffs_mtb.h"
// #include "uffs_utils.h"
#include <string.h> 
#include <stdio.h>

static int _object_data[(sizeof(struct uffs_ObjectSt) * MAX_OBJECT_HANDLE) / sizeof(int)];

static uffs_Pool _object_pool;

/**
 * initialise object buffers, called by UFFS internal
 */
URET uffs_InitObjectBuf(void)
{
	return uffs_PoolInit(&_object_pool, _object_data, sizeof(_object_data),
			sizeof(uffs_Object), MAX_OBJECT_HANDLE, U_FALSE);
}