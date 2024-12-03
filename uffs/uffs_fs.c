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

/**
 * alloc a new object structure
 * \return the new object
 */
uffs_Object * uffs_GetObject(void)
{
	uffs_Object * obj;

	obj = (uffs_Object *) uffs_PoolGet(&_object_pool);
	if (obj) {
		memset(obj, 0, sizeof(uffs_Object));
		obj->attr_loaded = U_FALSE;
		obj->open_succ = U_FALSE;
	}

	return obj;
}

/**
 * Open object under the given dir.
 *
 * \param[in|out] obj to be open, obj is returned from uffs_GetObject()
 * \param[in] dev uffs device
 * \param[in] dir object parent dir serial NO.
 * \param[in] name point to the object name
 * \param[in] name_len object name length
 * \param[in] oflag open flag. UO_DIR should be passed for an dir object.
 *
 * \return U_SUCC or U_FAIL (error code in obj->err).
 */
// URET uffs_OpenObjectEx(uffs_Object *obj, uffs_Device *dev, 
// 					   int dir, const char *name, int name_len, int oflag)
// {

// 	obj->err = UENOERR;
// 	obj->open_succ = U_FALSE;

// 	if (dev == NULL) {
// 		obj->err = UEINVAL;
// 		goto ext;
// 	}

// 	if ((oflag & (UO_WRONLY | UO_RDWR)) == (UO_WRONLY | UO_RDWR)) {
// 		/* UO_WRONLY and UO_RDWR can't appear together */
// 		uffs_Perror(UFFS_MSG_NOISY,
// 					"UO_WRONLY and UO_RDWR can't appear together");
// 		obj->err = UEINVAL;
// 		goto ext;
// 	}

// 	obj->oflag = oflag;
// 	obj->parent = dir;
// 	obj->type = (oflag & UO_DIR ? UFFS_TYPE_DIR : UFFS_TYPE_FILE);
// 	obj->pos = 0;
// 	obj->dev = dev;
// 	obj->name = name;
// 	obj->name_len = name_len;

// 	// adjust the name length
// 	if (obj->type == UFFS_TYPE_DIR) {
// 		if (obj->name_len > 0 && name[obj->name_len - 1] == '/')
// 			obj->name_len--;	// truncate the ending '/' for dir
// 	}

// 	obj->sum = (obj->name_len > 0 ? uffs_MakeSum16(name, obj->name_len) : 0);
// 	obj->head_pages = obj->dev->attr->pages_per_block - 1;

// 	if (obj->type == UFFS_TYPE_DIR) {
// 		if (obj->name_len == 0) {
// 			if (dir != PARENT_OF_ROOT) {
// 				uffs_Perror(UFFS_MSG_SERIOUS, "Bad parent for root dir!");
// 				obj->err = UEINVAL;
// 			}
// 			else {
// 				obj->serial = ROOT_DIR_SERIAL;
// 			}
// 			goto ext;
// 		}
// 	}
// 	else {
// 		if (obj->name_len == 0 || name[obj->name_len - 1] == '/') {
// 			uffs_Perror(UFFS_MSG_SERIOUS, "Bad file name.");
// 			obj->err = UEINVAL;
// 		}
// 	}


// 	uffs_ObjectDevLock(obj);

// 	if (obj->type == UFFS_TYPE_DIR) {
// 		obj->node = uffs_TreeFindDirNodeByName(obj->dev, obj->name,
// 												obj->name_len, obj->sum,
// 												obj->parent);
// 	}
// 	else {
// 		obj->node = uffs_TreeFindFileNodeByName(obj->dev, obj->name,
// 												obj->name_len, obj->sum,
// 												obj->parent);
// 	}

// 	if (obj->node == NULL) {			// dir or file not exist
// 		if (obj->oflag & UO_CREATE) {	// expect to create a new one
// 			uffs_ObjectDevUnLock(obj);
// 			if (obj->name == NULL || obj->name_len == 0)
// 				obj->err = UEEXIST;
// 			else
// 				uffs_CreateObjectEx(obj, dev, dir, obj->name, obj->name_len, oflag);
// 			goto ext;
// 		}
// 		else {
// 			obj->err = UENOENT;
// 			goto ext_1;
// 		}
// 	}

// 	if ((obj->oflag & (UO_CREATE | UO_EXCL)) == (UO_CREATE | UO_EXCL)){
// 		obj->err = UEEXIST;
// 		goto ext_1;
// 	}

// 	obj->serial = GET_OBJ_NODE_SERIAL(obj);
// 	obj->open_succ = U_TRUE;

// 	if (obj->oflag & UO_TRUNC)
// 		if (do_TruncateObject(obj, 0, eDRY_RUN) == U_SUCC) {
// 			//NOTE: obj->err will be set in do_TruncateObject() if failed.
// 			do_TruncateObject(obj, 0, eREAL_RUN);
// 		}

// ext_1:
// 	uffs_ObjectDevUnLock(obj);
// ext:
// 	obj->open_succ = (obj->err == UENOERR ? U_TRUE : U_FALSE);

// 	return (obj->err == UENOERR ? U_SUCC : U_FAIL);
// }

/**
 * re-initialize an object.
 *
 * \return U_SUCC or U_FAIL if the object is openned.
 */
URET uffs_ReInitObject(uffs_Object *obj)
{
	if (obj == NULL)
		return U_FAIL;

	if (obj->open_succ == U_TRUE)
		return U_FAIL;	// can't re-init an openned object.

	memset(obj, 0, sizeof(uffs_Object));
	obj->attr_loaded = U_FALSE;
	obj->open_succ = U_FALSE;

	return U_SUCC;	
}

/**
 * return the dir length from a path.
 * for example, path = "abc/def/xyz", return 8 ("abc/def/")
 */
static int GetDirLengthFromPath(const char *path, int path_len)
{
	const char *p = path;

	if (path_len > 0) {
		if (path[path_len - 1] == '/')
			path_len--;		// skip the last '/'

		p = path + path_len - 1;
		while (p > path && *p != '/')
			p--; 
	}

	return p - path;
}

/**
 * Parse the full path name, initialize obj.
 *
 * \param[out] obj object to be initialize.
 * \param[in] name full path name.
 *
 * \return U_SUCC if the name is parsed correctly,
 *			 U_FAIL if failed, and obj->err is set.
 *
 *	\note the following fields in obj will be initialized:
 *			obj->dev
 *			obj->parent
 *			obj->name
 *			obj->name_len
 */
// URET uffs_ParseObject(uffs_Object *obj, const char *name)
URET uffs_ParseObject(uffs_Object *obj, const char *name, uffs_Device *dev)
{
	int len, m_len, d_len;
	// uffs_Device *dev;
	const char *start, *p, *dname;
	u16 dir;
	TreeNode *node;
	u16 sum;

	if (uffs_ReInitObject(obj) == U_FAIL)
		return U_FAIL;

	len = strlen(name);
	m_len = uffs_GetMatchedMountPointSize(name);
	// dev = uffs_GetDeviceFromMountPointEx(name, m_len);

	if (dev) {
		start = name + m_len;
		d_len = GetDirLengthFromPath(start, len - m_len);
		p = start;
		obj->dev = dev;
		if (m_len == len) {
			obj->parent = PARENT_OF_ROOT;
			obj->name = NULL;
			obj->name_len = 0;
		}
		else {
			dir = ROOT_DIR_SERIAL;
			dname = start;
			while (p - start < d_len) {
				while (*p != '/') p++;
                // TODO: check sum.
				// sum = uffs_MakeSum16(dname, p - dname);
				node = uffs_TreeFindDirNodeByName(dev, dname, p - dname, sum, dir);
				if (node == NULL) {
					obj->err = UENOENT;
					break;
				}
				else {
					dir = node->u.dir.serial;
					p++; // skip the '/'
					dname = p;
				}
			}
			obj->parent = dir;
			obj->name = start + (d_len > 0 ? d_len + 1 : 0);
			obj->name_len = len - (d_len > 0 ? d_len + 1 : 0) - m_len;
		}

		if (obj->err != UENOERR) {
			uffs_PutDevice(obj->dev);
			obj->dev = NULL;
		}
	}
	else {
		obj->err = UENOENT;
	}

	return (obj->err == UENOERR ? U_SUCC : U_FAIL);
}

// static void do_ReleaseObjectResource(uffs_Object *obj)
// {
// 	if (obj) {
// 		if (obj->dev) {
// 			if (HAVE_BADBLOCK(obj->dev))
// 				uffs_BadBlockRecover(obj->dev);
// 			if (obj->dev_lock_count > 0) {
// 				uffs_ObjectDevUnLock(obj);
// 			}
// 			uffs_PutDevice(obj->dev);
// 			obj->dev = NULL;
// 			obj->open_succ = U_FALSE;
// 		}
// 	}
// }

/**
 * Open a UFFS object
 *
 * \param[in|out] obj the object to be open
 * \param[in] name the full name of the object
 * \param[in] oflag open flag
 *
 * \return U_SUCC if object is opened successfully,
 *			 U_FAIL if failed, error code will be set to obj->err.
 */
URET uffs_OpenObject(uffs_Object *obj, const char *name, int oflag, uffs_Device *dev)
{
	URET ret;

	if (obj == NULL)
		return U_FAIL;

 	if ((ret = uffs_ParseObject(obj, name, dev)) == U_SUCC) {
        ret = 0;
		// ret = uffs_OpenObjectEx(obj, obj->dev, obj->parent,
		// 							obj->name, obj->name_len, oflag);
 	}
 	if (ret != U_SUCC)
 		// do_ReleaseObjectResource(obj);

	return ret;
}