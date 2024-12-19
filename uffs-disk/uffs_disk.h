#ifndef _UFFS_DISK_H_
#define _UFFS_DISK_H_

#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include "uffs_types.h"
#include <errno.h>

#define MAGIC "UFFS" // must 4 char

/** ECC options (uffs_StorageAttrSt.ecc_opt) */
#define UFFS_ECC_NONE		0	//!< do not use ECC
#define UFFS_ECC_SOFT		1	//!< UFFS calculate the ECC
#define UFFS_ECC_HW			2	//!< Flash driver(or by hardware) calculate the ECC
#define UFFS_ECC_HW_AUTO	3	//!< Hardware calculate the ECC and automatically write to spare.

/* default basic parameters of the NAND device */
#define PAGES_PER_BLOCK_DEFAULT			32
#define PAGE_DATA_SIZE_DEFAULT			512
#define PAGE_SPARE_SIZE_DEFAULT			16
#define PAGE_SIZE_DEFAULT               528
#define STATUS_BYTE_OFFSET_DEFAULT		5
#define TOTAL_BLOCKS_DEFAULT			128
#define ECC_OPTION_DEFAULT				UFFS_ECC_SOFT

#define MAX_FILENAME_LENGTH PAGE_DATA_SIZE_DEFAULT - 24

#define UFFS_TYPE_DIR		1
#define UFFS_TYPE_FILE		2
#define UFFS_TYPE_DATA		3
#define UFFS_TYPE_RESV		0
#define UFFS_TYPE_INVALID	0xFF
#define ROOT_DIR_SERIAL		0xFF				//!< serial num of root dir

#define UFFS_TAG_PAGE_ID_SIZE_BITS  6

#define	US_IFREG	0x8000	/* regular */
#define	US_IFDIR	0x4000	/* directory */

/** \note 8-bits attr goes to uffs_dirent::d_type */
#define FILE_ATTR_DIR       (1 << 7)    //!< attribute for directory
#define FILE_ATTR_WRITE     (1 << 0)    //!< writable
/**
 * \struct uffs_TagStoreSt
 * \brief uffs tag, 8 bytes, will be store in page spare area.
 */
struct uffs_TagStoreSt {
	u32 dirty:1;		//!< 0: dirty, 1: clear
	u32 valid:1;		//!< 0: valid, 1: invalid
	u32 type:2;			//!< block type: #UFFS_TYPE_DIR, #UFFS_TYPE_FILE, #UFFS_TYPE_DATA
	u32 block_ts:2;		//!< time stamp of block;
	u32 data_len:12;	//!< length of page data
	u32 serial:14;		//!< serial number
	u32 parent:10;		//!< parent's serial number
	u32 page_id:UFFS_TAG_PAGE_ID_SIZE_BITS;		//!< page id
#if UFFS_TAG_RESERVED_BITS != 0
	u32 reserved:UFFS_TAG_RESERVED_BITS;		//!< reserved, for UFFS2
#endif
	u32 tag_ecc:12;		//!< tag ECC
};

#define TAG_ECC_DEFAULT (0xFFF)	//!< 12-bit '1'


/** 
 * \struct uffs_TagsSt 12바이트
 */ 
struct uffs_TagsSt {
	struct uffs_TagStoreSt s;		/* store must be the first member 8바이트 */ 

	/** data_sum for file or dir name */
	u16 data_sum;

	/** internal used */
	u8 seal_byte;			//!< seal byte.
};
typedef struct uffs_TagsSt uffs_Tag;

/** 4바이트
 * \struct uffs_MiniHeaderSt
 * \brief the mini header resides on the head of page data
 */
struct uffs_MiniHeaderSt {
	u8 status; // 0xFF -> 새것 , 0x01 -> 사용중, 그 외의 상태는 dirty 로 정의
	u8 reserved;
	u16 crc;
};
typedef struct uffs_MiniHeaderSt uffs_MiniHeader;


/**
 * \structure uffs_FileInfoSt
 * \brief file/dir entry info in physical storage format //24바이트+32바이트 
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

URET diskFormatCheck(int fd);
URET diskFormat(int fd);
URET readPage(int fd, int block_id, int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag);
URET writePage(int fd,int block_id,int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag);
URET getFileInfoBySerial(int fd, u32 serial, uffs_FileInfo *file_info, u32 *out_len);
URET getFreeBlock(int fd, int *freeBlockId, u16 *serial);
#endif