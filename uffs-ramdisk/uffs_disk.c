#include "uffs_disk.h"

URET getFreeBlock(data_Disk* disk, data_Block** freeBlock) {
    fprintf(stdout,"[getFreeBlock] called\n");
    for(int i = 0; i<BLOCK_COUNT;i++){
        if(disk->blocks[i].status == unusedblock) {
            memset(&disk->blocks[i],0,sizeof(data_Block));
            srand(time(NULL)); // 시드 초기화
            u16 random_serial = (u16)(rand() & 0xFFFF); // 16비트 값 생성
            disk->blocks[i].tag.block_id = i;
            disk->blocks[i].tag.serial = random_serial;
            *freeBlock = &disk->blocks[i];
            fprintf(stdout,"[getFreeBlock] finished\n");
            return U_SUCC;
        }
    }
    fprintf(stderr,"[getFreeBlock] error 1\n");
    return U_FAIL;
};

URET initBlock(data_Block** block, u8 type, u16 data_len) {
    fprintf(stdout,"[initBlock] called\n");
    (*block)->tag.data_len = data_len;
    (*block)->tag.type = type;
    (*block)->tag.page_offset = 0;
    fprintf(stdout,"[initBlock] finished\n");
}

URET getUsedBlockById(data_Disk *disk, data_Block **block, u16 block_id){
    fprintf(stdout,"[getUsedBlockById] called\n");
    for(int i =0;i<BLOCK_COUNT;i++){
        if(disk->blocks[i].tag.block_id==block_id && disk->blocks[i].status==usedblock){
            *block=&disk->blocks[i];
            fprintf(stdout,"[getUsedBlockById] finished\n");
            return U_SUCC;
        }
    }
    fprintf(stderr,"[getUsedBlockById] error 1\n");
    return U_FAIL;
    
}

void uffs_InitBlock(data_Disk *disk){
    fprintf(stdout,"[uffs_InitBlock] called\n");
    for(int i=0;i<BLOCK_DATA_SIZE;i++){
        disk->blocks[i].status = unusedblock;
    }
    fprintf(stdout,"[uffs_InitBlock] finished\n");
}


