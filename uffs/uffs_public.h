/** 
 * \file uffs_public.h
 * \brief public data structures for uffs
 * \author Ricky Zheng
 */

#ifndef _UFFS_PUBLIC_H_
#define _UFFS_PUBLIC_H_

#include "uffs_types.h"
#include "uffs_core.h"
#include "uffs.h"
#include "uffs_pool.h"

#include "uffs_device.h"

#include <stdio.h>

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(ar) (sizeof(ar) / sizeof(ar[0]))
#endif

#ifndef offsetof
# define offsetof(T, x) ((size_t) &((T *)0)->x)
#endif
#ifndef container_of
#define container_of(p, T, x) ((T *)((char *)(p) - offsetof(T,x)))
#endif

/** 
 * \def CONFIG_UFFS_DBG_SHOW_LINE_NUM
 * \brief show function and line number in debug message
 */
#define CONFIG_UFFS_DBG_SHOW_LINE_NUM

/** 
 * \def MAX_FILENAME_LENGTH 
 * \note Be careful: it's part of the physical format (see: uffs_FileInfoSt.name)
 *    !!DO NOT CHANGE IT AFTER FILE SYSTEM IS FORMATED!!
 */
#define MAX_FILENAME_LENGTH         128

/** \note 8-bits attr goes to uffs_dirent::d_type */
#define FILE_ATTR_DIR       (1 << 7)    //!< attribute for directory
#define FILE_ATTR_WRITE     (1 << 0)    //!< writable

/**
 * \def UFFS_TAG_PAGE_ID_SIZE_BITS
 * \brief define number of bits used for page_id in tag,
 *        this defines the maximum pages per block you can have.
 *        e.g. '9' ==> maximum 512 pages per block
 **/
#define UFFS_TAG_PAGE_ID_SIZE_BITS  6

#if UFFS_TAG_RESERVED_BITS > 10
#error "UFFS_TAG_PAGE_ID_SIZE_BITS can not bigger than 10 !"
#endif

/**
 * \def UFFS_TAG_RESERVED_BITS
 * \brief the number of bits left to be used for the furture (UFFS2).
 **/
#define UFFS_TAG_RESERVED_BITS (10 - UFFS_TAG_PAGE_ID_SIZE_BITS)

/********************************** Public defines *****************************/
/**
 * \def UFFS_ALL_PAGES 
 * \brief UFFS_ALL_PAGES if this value presented, that means the objects are all pages in the block
 */
#define UFFS_ALL_PAGES (0xffff)

/** 
 * \def UFFS_INVALID_PAGE
 * \brief macro for invalid page number
 */
#define UFFS_INVALID_PAGE	(0xfffe)

/**
 * \def UFFS_MAX_PAGES_PER_BLOCK
 * \brief maximum allowed pages per block
 **/
#define UFFS_MAX_PAGES_PER_BLOCK    (1 << UFFS_TAG_PAGE_ID_SIZE_BITS)

/** 
 * \def UFFS_INVALID_BLOCK
 * \brief macro for invalid block number
 */
#define UFFS_INVALID_BLOCK	(0xfffe)

/**
 * \structure uffs_FileInfoSt
 * \brief file/dir entry info in physical storage format
 */
struct uffs_FileInfoSt {
    u32 attr;               //!< file/dir attribute
    u32 create_time;
    u32 last_modify;
    u32 access;
    u32 reserved;
    u32 name_len;           //!< length of file/dir name
    char name[MAX_FILENAME_LENGTH];
};
typedef struct uffs_FileInfoSt uffs_FileInfo;

/**
 * \struct uffs_ObjectInfoSt
 * \brief object info
 */
typedef struct uffs_ObjectInfoSt {
    uffs_FileInfo info;
    u32 len;                //!< length of file
    u16 serial;             //!< object serial num
} uffs_ObjectInfo;

/* some functions from uffs_fd.c */
// void uffs_FdSignatureIncrease(void);
URET uffs_DirEntryBufInit(void);
// URET uffs_DirEntryBufRelease(void);
// uffs_Pool * uffs_DirEntryBufGetPool(void);
// int uffs_DirEntryBufPutAll(uffs_Device *dev);

/************************************************************************/
/*  init functions                                                                     */
/************************************************************************/
URET uffs_InitDevice(uffs_Device *dev);

#endif