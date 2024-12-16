#include "uffs_disk.h"

URET diskFormatCheck(int fd){
    fprintf(stdout,"[diskFormatCheck] called\n");
    char magic[PAGE_DATA_SIZE_DEFAULT];

    readPage(fd,0,0,NULL,magic,NULL);

    if (memcmp(magic, MAGIC,4) == 0) {
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

static void setRootTag(uffs_Tag *tag) {
    // 태그 값 설정
    tag->data_sum = 0;               // 루트 디렉토리의 이름 체크섬
    tag->seal_byte = 0;              // seal byte 초기화
    tag->s.dirty = 1;                // 페이지가 깨끗함
    tag->s.valid = 0;                // 유효한 페이지
    tag->s.type = UFFS_TYPE_DIR;     // 디렉토리 타입
    tag->s.block_ts = 0;             // 블록 타임스탬프 초기화
    tag->s.data_len = 0;             // 데이터 길이 (디렉토리라서 0)
    tag->s.serial = ROOT_DIR_SERIAL; // 루트 디렉토리 시리얼 번호
    tag->s.parent = 0; // 루트 디렉토리는 부모 없음
    tag->s.page_id = 0;              // 첫 번째 페이지
    tag->s.tag_ecc = TAG_ECC_DEFAULT; // 태그 ECC 기본값
}
static void setRootMiniHeader(struct uffs_MiniHeaderSt *miniHeader) {

    // 루트 블록의 첫 번째 페이지에 미니 헤더 초기화
    miniHeader->status = 0x01; // 페이지 상태 (예: 유효한 페이지)
    miniHeader->reserved = 0x00; // 예약된 값
    miniHeader->crc = 0xFFFF; // 초기 CRC 값
}

static void setRootFileInfo(uffs_FileInfo *file_info){
    file_info->access = GET_CURRENT_TIME();
    file_info->attr = FILE_ATTR_DIR;
    file_info->create_time = GET_CURRENT_TIME();
    file_info->last_modify = GET_CURRENT_TIME();
    strcpy(file_info->name,"/");
    file_info->name_len = 1;
    file_info->reserved = 0x00;
}

URET diskFormat(int fd) {
    fprintf(stdout, "[diskFormat] Disk formatting started\n");

    char data[PAGE_DATA_SIZE_DEFAULT] = {0};

    // 블록 및 페이지 초기화
    for (int block = 2; block < TOTAL_BLOCKS_DEFAULT; block++) {
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
    uffs_MiniHeader mini_header = {0xFF, 0x00, 0x00}; // CRC: 블록+페이지 합    
    uffs_Tag tag={0};

    memset(magic, 0, sizeof(magic));
    memcpy(magic, MAGIC, 4);

    if (writePage(fd,0,0,&mini_header,magic,&tag) < 0) {
        fprintf(stderr, "[diskFormat] write magic number error\n");    
        return U_FAIL;
    }

    // write root block
    uffs_FileInfo file_info={0};

    setRootFileInfo(&file_info);
    setRootTag(&tag);
    setRootMiniHeader(&mini_header);

    if (writePage(fd,1,0,&mini_header,(char*)&file_info,&tag) < 0) {
        fprintf(stderr, "[diskFormat] write magic number error\n");    
        return U_FAIL;
    }
    fprintf(stdout, "[diskFormat] Disk formatting complete\n");
    return U_SUCC;
}




URET readPage(int fd, int block_id, int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag) {

    char page_buf[PAGE_SIZE_DEFAULT];
    off_t read_offset = block_id * (PAGES_PER_BLOCK_DEFAULT * PAGE_SIZE_DEFAULT) + page_Id * PAGE_SIZE_DEFAULT;

    ssize_t bytes_read = pread(fd, page_buf, sizeof(page_buf), read_offset);
    
    off_t offset = 0;
    if (mini_header != NULL) {
        memcpy(mini_header, page_buf + offset, sizeof(uffs_MiniHeader));
    }
    offset += sizeof(uffs_MiniHeader);

    if (data != NULL) {
        memcpy(data, page_buf + offset, PAGE_DATA_SIZE_DEFAULT);
    }
    offset += PAGE_DATA_SIZE_DEFAULT;

    if (tag != NULL) {
        memcpy(tag, page_buf + offset, sizeof(uffs_Tag));
    }

    return U_SUCC;
}


URET writePage(int fd, int block_id, int page_Id, uffs_MiniHeader* mini_header, char* data, uffs_Tag *tag) {

    fprintf(stdout, "[writePage] Called: block_id=%d, page_Id=%d\n", block_id, page_Id);


    // 페이지 버퍼 초기화
    char page_buf[PAGE_SIZE_DEFAULT];
    memset(page_buf, 0, sizeof(page_buf));

    // 오프셋 설정 및 데이터 복사
    off_t offset = 0;

    // MiniHeader 복사
    if (mini_header != NULL) {
        memcpy(page_buf + offset, mini_header, sizeof(uffs_MiniHeader));
        fprintf(stdout, "[writePage] MiniHeader written: status=%d\n", mini_header->status);
        offset += sizeof(uffs_MiniHeader);
    } else {
        fprintf(stderr, "[writePage] Error: MiniHeader is NULL\n");
        return U_FAIL;
    }

    // Data 복사
    if (data != NULL) {
        memcpy(page_buf + offset, data, PAGE_DATA_SIZE_DEFAULT);
        fprintf(stdout, "[writePage] Data to write: %.*s\n", PAGE_DATA_SIZE_DEFAULT, data);
    } else {
        fprintf(stdout, "[writePage] Warning: Data is NULL\n");
    }
    offset += PAGE_DATA_SIZE_DEFAULT;  // 데이터 크기만큼 오프셋 증가

    // Tag 복사
    if (tag != NULL) {
        memcpy(page_buf + offset, tag, sizeof(uffs_Tag));
        fprintf(stdout, "[writePage] Tag written: valid=%d, dirty=%d\n", tag->s.valid, tag->s.dirty);
    } else {
        fprintf(stderr, "[writePage] Error: Tag is NULL\n");
        return U_FAIL;
    }

    // pwrite 호출: 블록과 페이지에 따른 오프셋 계산
    off_t file_offset = block_id * (PAGES_PER_BLOCK_DEFAULT * PAGE_SIZE_DEFAULT) +
                        page_Id * PAGE_SIZE_DEFAULT;

    ssize_t written = pwrite(fd, page_buf, sizeof(page_buf), file_offset);
    if (written != sizeof(page_buf)) {
        fprintf(stderr, "[writePage] Error: Failed to write full page (expected: %zu, written: %zd)\n", sizeof(page_buf), written);
        return U_FAIL;
    } else {
        fprintf(stdout, "[writePage] Successfully wrote %zd bytes to block_id=%d, page_Id=%d\n", written, block_id, page_Id);
    }

    // 데이터 검증
    char verify_buf[PAGE_SIZE_DEFAULT];
    ssize_t read_bytes = pread(fd, verify_buf, sizeof(page_buf), file_offset);
    if (read_bytes != sizeof(page_buf)) {
        fprintf(stderr, "[writePage] Error: Failed to read back full page (expected: %zu, read: %zd)\n", sizeof(page_buf), read_bytes);
        return U_FAIL;
    }

    fprintf(stdout, "[writePage] Finished successfully.\n");
    return U_SUCC;
}




URET getFileInfoBySerial(int fd, u32 serial, uffs_FileInfo *file_info) {
    uffs_Tag tag = {0};
    for (int block = 0; block < TOTAL_BLOCKS_DEFAULT; block++) {
        if (readPage(fd, block, 0, NULL, (char *)file_info, &tag) == U_SUCC) {
            if (tag.s.serial == serial) {
                return U_SUCC;
            }
        }
    }
    return U_FAIL;
}

// 빈 블록 찾기
URET getFreeBlock(int fd, int *free_block_id, u16 *serial) {
    // 여기서는 2번 블록부터 free라고 가정 (0:마법,1:root)
    for (int i=2;i<TOTAL_BLOCKS_DEFAULT;i++){
        // mini_header의 status로 해당 블록 사용중인지 확인
        uffs_MiniHeader mini_header={0};
        uffs_Tag tag;
        if (readPage(fd,i,0,&mini_header,NULL,&tag)==U_SUCC) {
            if (mini_header.status == 0xFF) {
                *free_block_id = i;
                *serial = tag.s.serial;
                return U_SUCC;
            }
        }
    }
    return U_FAIL;
}