#ifndef _UFFS_DISK_H_
#define _UFFS_DISK_H_

#include "uffs_types.h"
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#define BLOCK_COUNT 512               //!< 블록 개수
#define BLOCK_SIZE 512
#define BLOCK_DATA_SIZE 496
#define MAX_FILENAME_LENGTH 32

#define UFFS_TYPE_DIR		0
#define UFFS_TYPE_FILE		1
#define UFFS_TYPE_DATA		2
#define UFFS_TYPE_RESV		3
#define UFFS_TYPE_INVALID	0xFF
#define ROOT_DIR_SERIAL	0				//!< serial num of root dir
typedef enum {usedblock, unusedblock} block_status;

struct data_TagSt {
    u16 block_id;    //!< 블록 ID
    u8 type;         //!< 블록 타입 (파일, 디렉터리, 데이터 등)
    u16 data_len;    //!< 데이터 길이
    u16 serial;      //!< 고유 번호
    u16 parent;      //!< 부모 블록 ID
    block_status status; //!< 블록 상태
};
typedef struct data_TagSt data_Tag;

struct uffs_FileInfoSt {
    u32 create_time;
    u32 last_modify;
    u32 access;
    u32 reserved;
    u32 name_len;           //!< length of file/dir name
    char name[MAX_FILENAME_LENGTH];
	// custom filed
	short nlink;
	u32 len;
	u16 mode;
};
typedef struct uffs_FileInfoSt uffs_FileInfo;

typedef struct data_BlockSt {
    union{
        char data[BLOCK_DATA_SIZE];
        uffs_FileInfo fileInfo;
    } u;
    struct data_TagSt tag;
} data_Block;

typedef struct data_DiskSt {
    struct data_BlockSt blocks[BLOCK_COUNT];
} data_Disk;

URET getFreeBlock(data_Disk* disk, data_Block** freeBlock);
URET initBlock(data_Block** block, u8 type, u16 data_len);
URET getUsedBlockById(data_Disk *disk, data_Block **block, u16 block_id);
void uffs_InitBlock(data_Disk *disk);
URET diskFormatCheck(int fd);
URET diskFormat(int fd);
#endif