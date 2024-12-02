#ifndef _UFFS_MTB_H_
#define _UFFS_MTB_H_

#include "uffs_types.h"
#include "uffs_core.h"
// #include "uffs.h"

#ifdef __cplusplus
extern "C"{
#endif


typedef struct uffs_MountTableEntrySt {
	uffs_Device *dev;		// UFFS 'device' - core internal data structure for partition
	int start_block;		// partition start block
	int end_block;			// partition end block ( if < 0, reserve space form the end of storage)
	const char *mount;		// mount point
	struct uffs_MountTableEntrySt *prev;
	struct uffs_MountTableEntrySt *next;
} uffs_MountTable;

// /** Register mount entry, will be put at 'unmounted' list */
// int uffs_RegisterMountTable(uffs_MountTable *mtb);

// /** Remove mount entry from the list */
// int uffs_UnRegisterMountTable(uffs_MountTable *mtb);

/** mount partition */
int uffs_Mount(uffs_Device *dev);

// /** unmount parttion */
// int uffs_UnMount(const char *mount);

// /** get mounted entry list */
// uffs_MountTable * uffs_MtbGetMounted(void);

// /** get unmounted entry list */
// uffs_MountTable * uffs_MtbGetUnMounted(void);

// /** get matched mount point from absolute path */
// int uffs_GetMatchedMountPointSize(const char *path);			

// /** get uffs device from mount point */
// uffs_Device * uffs_GetDeviceFromMountPoint(const char *mount);	

// /** get uffs device from mount point */
// uffs_Device * uffs_GetDeviceFromMountPointEx(const char *mount, int len);	

// /** get mount point name from uffs device */
// const char * uffs_GetDeviceMountPoint(uffs_Device *dev);		

// /** down crease uffs device references by uffs_GetDeviceXXX() */
// void uffs_PutDevice(uffs_Device *dev);							

#ifdef __cplusplus
}
#endif
#endif