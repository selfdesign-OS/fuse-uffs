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

URET diskFormatCheck(int fd){
    fprintf(stdout,"[diskFormatCheck] called\n");
    char magic[PAGE_DATA_SIZE_DEFAULT];

    readPage(fd,0,0,NULL,magic,NULL);

    if (memcmp(magic, "UFFS",4) == 0) {
        fprintf(stdout,"[diskFormatCheck] finished\n");
        return U_SUCC;
    }
    fprintf(stderr,"[diskFormatCheck] is not uffs\n");
    return U_FAIL;
}

static u32 GET_CURRENT_TIME() {
    time_t now = time(NULL);
    return (u32)now;
}

static int writeRootTag(uffs_Tag *tag) {
    // 태그 값 설정
    tag->data_sum = 0;               // 루트 디렉토리의 이름 체크섬
    tag->seal_byte = 0;              // seal byte 초기화
    tag->s.dirty = 1;                // 페이지가 깨끗함
    tag->s.valid = 0;                // 유효한 페이지
    tag->s.type = UFFS_TYPE_DIR;     // 디렉토리 타입
    tag->s.block_ts = 0;             // 블록 타임스탬프 초기화
    tag->s.data_len = 0;             // 데이터 길이 (디렉토리라서 0)
    tag->s.serial = ROOT_DIR_SERIAL; // 루트 디렉토리 시리얼 번호
    tag->s.parent = ROOT_DIR_SERIAL; // 루트 디렉토리는 부모 없음
    tag->s.page_id = 0;              // 첫 번째 페이지
    tag->s.tag_ecc = TAG_ECC_DEFAULT; // 태그 ECC 기본값
    
    return 0; // 성공
}
static int writeRootMiniHeader(struct uffs_MiniHeaderSt *miniHeader) {

    // 루트 블록의 첫 번째 페이지에 미니 헤더 초기화
    miniHeader->status = 0x01; // 페이지 상태 (예: 유효한 페이지)
    miniHeader->reserved = 0x00; // 예약된 값
    miniHeader->crc = 0xFFFF; // 초기 CRC 값 (임의로 설정)

    return 0;
}
URET diskFormat(int fd) {
    fprintf(stdout, "[diskFormat] Disk formatting started\n");

    char data[PAGE_DATA_SIZE_DEFAULT] = {0};

    // 블록 및 페이지 초기화
    for (int block = 0; block < TOTAL_BLOCKS_DEFAULT; block++) {
        for (int page = 0; page < PAGES_PER_BLOCK_DEFAULT; page++) {
            struct uffs_MiniHeaderSt mini_header = {0xFF, 0x00, (u16)(block + page)}; // CRC: 블록+페이지 합
            struct uffs_TagsSt tag = {0};

            // 태그 초기화
            tag.s.dirty = 1;
            tag.s.valid = 0;
            tag.s.type = 0; 
            tag.s.block_ts = 0;
            tag.s.data_len = 0; 
            tag.s.serial = block;
            tag.s.parent = 0;
            tag.s.page_id = page;
            tag.s.tag_ecc = TAG_ECC_DEFAULT;

            tag.data_sum = 0;
            tag.seal_byte = 0;
            // 페이지 작성
            if (writePage(fd, block, page, &mini_header, data, &tag) < 0) {
                fprintf(stderr, "[diskFormat] Failed to write block %d, page %d\n", block, page);
                return U_FAIL;
            }
        }
    }

    // write magic number
    char magic[PAGE_DATA_SIZE_DEFAULT];
    uffs_MiniHeader mini_header = {0};
    uffs_Tag tag={0};
    // write root block
    writeRootTag(&tag);
    writeRootMiniHeader(&mini_header);

    memset(magic, 0, sizeof(magic));
    memcpy(magic, "UFFS", 4);

    if (writePage(fd,1,0,&mini_header,magic,&tag) < 0) {
        fprintf(stderr, "[diskFormat] write magic number error\n");    
        return U_FAIL;
    }
    fprintf(stdout, "[diskFormat] Disk formatting complete\n");
    return U_SUCC;
}




URET readPage(int fd, int block_id, int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag){
    fprintf(stdout, "[readPage] readPage called.\n");
    char page_buf[PAGE_SIZE_DEFAULT];
    if(pread(fd,page_buf,sizeof(page_buf),block_id*(PAGES_PER_BLOCK_DEFAULT*PAGE_SIZE_DEFAULT))<0){
        fprintf(stderr, "[readPage] readPage error.\n");
        return U_FAIL;
    }
    off_t offset = 0;
    if(mini_header != NULL)
        memcpy(mini_header, page_buf+offset,sizeof(uffs_MiniHeader));
    offset += sizeof(uffs_MiniHeader);
    if(data!=NULL)
        memcpy(data, page_buf+offset,PAGE_DATA_SIZE_DEFAULT);
    offset += sizeof(PAGE_DATA_SIZE_DEFAULT);
    if(tag!=NULL)
        memcpy(tag, page_buf+offset,sizeof(uffs_Tag));

    fprintf(stdout, "[readPage] readPage finished.\n");
    return U_SUCC;
}

URET writePage(int fd, int block_id,int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag){
    fprintf(stdout, "[writePage] writePage called.\n");
    char page_buf[PAGE_SIZE_DEFAULT];

    off_t offset = 0;
    memcpy(page_buf+offset,mini_header,sizeof(uffs_MiniHeader));
    offset += sizeof(uffs_MiniHeader);
    memcpy(page_buf+offset,data,PAGE_DATA_SIZE_DEFAULT);
    offset += sizeof(PAGE_DATA_SIZE_DEFAULT);
    memcpy(page_buf+offset,tag,sizeof(uffs_Tag));

    if(pwrite(fd,page_buf,sizeof(page_buf),block_id*(PAGES_PER_BLOCK_DEFAULT*PAGE_SIZE_DEFAULT))<0){
        fprintf(stderr, "[writePage] writePage error.\n");
        return U_FAIL;
    }
    fprintf(stdout, "[writePage] writePage finished.\n");
    return U_SUCC;
}
    
