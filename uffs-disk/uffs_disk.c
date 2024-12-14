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

URET diskFormatCheck(int fd){
    fprintf(stdout,"[uffs_InitBlock] called\n");
    char buf[4];
    memset(buf,0,sizeof(buf));
    if (pread(fd, buf, sizeof(buf), 0) != sizeof(buf)) {
        fprintf(stderr,"[diskFormatCheck] read magic number error\n");
        return U_FAIL;
    }
    if (memcmp(buf, "UFFS",4) == 0) {
        fprintf(stdout,"[diskFormatCheck] finished\n");
        return U_SUCC;
    }
    fprintf(stderr,"[diskFormatCheck] error\n");
    return U_FAIL;
}

static u32 GET_CURRENT_TIME() {
    time_t now = time(NULL);
    return (u32)now;
}

static int write_root_block(int fd) {
    char buf[BLOCK_SIZE] = {0}; // 512바이트 버퍼 초기화
    data_Tag tag;
    // 루트 블록 tag 초기화
    tag.block_id = 1;
    tag.data_len = 0;
    tag.status = usedblock;
    tag.type = UFFS_TYPE_DIR;
    tag.serial = ROOT_DIR_SERIAL;
    tag.parent = ROOT_DIR_SERIAL;
    uffs_FileInfo fileInfo = {0};
    // 루트 블록 FileInfo 초기화
    fileInfo.create_time = GET_CURRENT_TIME(); // 현재 시간 함수 호출
    fileInfo.last_modify = GET_CURRENT_TIME(); // 생성 시점과 동일하게 초기화
    fileInfo.access = GET_CURRENT_TIME();      // 액세스 시간도 동일하게 초기화
    fileInfo.reserved = 0;                     // 예약 필드 초기화
    fileInfo.name_len = 1;                     // 루트 이름 길이 ("/"만 포함)
    fileInfo.nlink = 2;    // 디렉토리의 기본 링크 수는 2 ("."과 "..")
    fileInfo.len = 0;      // 디렉토리이므로 길이는 0
    fileInfo.mode = 0666;

    // 구조체를 버퍼에 복사
    size_t offset = 0;

    // 1. tag 복사
    memcpy(buf + offset, tag, sizeof(data_Tag));
    offset += sizeof(data_Tag);

    // 2. fileInfo 복사
    memcpy(buf + offset, fileInfo, sizeof(uffs_FileInfo));
    offset += sizeof(uffs_FileInfo);

    // 3. pwrite로 버퍼 쓰기
    if (pwrite(fd, buf, sizeof(buf), BLOCK_SIZE) < 0) {
        fprintf(stderr, "[diskFormat] write tag and fileInfo error\n");
        return U_FAIL;
    }

    return 0; // 성공
}

URET diskFormat(int fd){
     // 디스크 초기화
    char buffer[BLOCK_SIZE] = {0}; // 0으로 초기화된 블록
    off_t offset = 0;

    while (1) {
        ssize_t bytes_written = pwrite(fd, buffer, BLOCK_SIZE, offset);
        if (bytes_written < 0) {
            fprintf(stderr, "[diskFormat] Disk initialization error.\n");
            close(fd);
            return -1;
        }

        offset += BLOCK_SIZE;

        if (offset > BLOCK_SIZE*(BLOCK_COUNT-1)) {
            break; // EOF 도달
        }
    }
    fprintf(stdout, "[diskFormat] Disk reset complete.\n");    

    // write magic number
    char buf[4];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "UFFS", 4);
    if (pwrite(fd, buf, sizeof(buf), 0) < 0) {
        fprintf(stderr, "[diskFormat] write magic number error\n");    
        return U_FAIL;
    }
    
    // write root block
    write_root_block(fd);
    char buf[512];
    if (pwrite(fd, buf, sizeof(buf), BLOCK_SIZE) < 0) {
        fprintf(stderr, "[diskFormat] write magic number error\n");    
        return U_FAIL;
    }

    fprintf(stdout, "[diskFormat] Disk initialization complete.\n");
    return U_SUCC;
}