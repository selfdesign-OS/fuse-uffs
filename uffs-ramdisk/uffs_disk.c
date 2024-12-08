#include "uffs_disk.h"

URET getFreeBlock(data_Disk* disk, data_Block** freeBlock) {
    for(int i = 0; i<BLOCK_COUNT;i++){
        if(disk->blocks[i].status == unusedblock) {
            srand(time(NULL)); // 시드 초기화
            u16 random_serial = (u16)(rand() & 0xFFFF); // 16비트 값 생성
            disk->blocks[i].tag.block_id = i;
            disk->blocks[i].tag.serial = random_serial;
            *freeBlock = &disk->blocks[i];
            return U_SUCC;
        }
    }
    return U_FAIL;
};

URET initBlock(data_Block* block, u8 type, u16 data_len) {
    memset(block,0,sizeof(data_Block));
    block->tag.data_len = data_len;
    block->tag.type = type;
    block->tag.page_offset = 0;
}




