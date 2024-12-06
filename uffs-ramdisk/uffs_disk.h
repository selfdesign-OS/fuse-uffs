#ifndef _UFFS_DISK_H_
#define _UFFS_DISK_H_

#include "uffs_types.h"

#define BLOCK_COUNT 512               //!< 블록 개수
#define BLOCK_DATA_SIZE 512

typedef enum {usedblock, unusedblock} block_status;

struct data_TagSt {
    u16 block_id;    //!< 블록 ID
    u16 page_offset; //!< 페이지 내 위치
    u8 type;         //!< 블록 타입 (파일, 디렉터리, 데이터 등)
    u16 data_len;    //!< 데이터 길이
    u16 serial;      //!< 고유 번호
    u16 parent;      //!< 부모 블록 ID
};

struct data_BlockSt {
    block_status status; //!< 블록 상태
    char data[BLOCK_DATA_SIZE];
    struct data_TagSt tag;
};

struct data_DiskSt {
    struct data_BlockSt blocks[BLOCK_COUNT];
};

#endif