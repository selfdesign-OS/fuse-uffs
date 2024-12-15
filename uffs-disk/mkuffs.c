/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall uffs.c `pkg-config fuse --cflags --libs` -o uffs
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "uffs_tree.h"
#include "uffs_types.h"
#include "uffs_disk.h"
#include <errno.h>

uffs_Device dev = {0};

int uffs_init()
{
	fprintf(stdout, "[uffs_init] called\n");
	uffs_TreeInit(&dev);
	uffs_BuildTree(&dev, fd);
	fprintf(stdout, "[uffs_init] finished\n");
	return 0;
}

int uffs_getattr(const char *path, struct stat *stbuf)
{
	fprintf(stdout, "[uffs_getattr] called\n");
	fprintf(stdout, "[uffs_getattr] path: %s\n", path);

	TreeNode *node;
	URET result;
    u8 type = UFFS_TYPE_DIR;
	uffs_ObjectInfo object_info ={0};

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		result = uffs_TreeFindNodeByName(&dev, &node, path, &type,&object_info);
	}
	else {
		if (result = uffs_TreeFindNodeByName(&dev, &node, path, &type, &object_info) != U_SUCC) {
			fprintf(stderr, "[uffs_getattr] result is U_FAIL\n");
            return -ENOENT;
        }
	}
    readPage(dev.fd,node->u.data.block)
    stbuf->st_mode = (object_info.info.attr & FILE_ATTR_DIR ? US_IFDIR : US_IFREG);
    stbuf->st_nlink = 2;
    stbuf->st_size = object_info.len;


	fprintf(stdout, "[uffs_getattr] finished\n");
	return 0;
}

int uffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi)
{
    fprintf(stdout, "[uffs_readdir] called\n");
    fprintf(stdout, "[uffs_readdir] path: %s\n", path);

    URET result;
    TreeNode *node = NULL;
    char *path_copy = NULL;
    u16 parent_serial;
    u8 type = UFFS_TYPE_DIR;
	uffs_ObjectInfo object_info ={0};

    // path를 strtok에서 안전하게 사용하기 위해 복사
    path_copy = strdup(path);
    if (!path_copy) {
        fprintf(stderr, "[uffs_readdir] memory allocation failed\n");
        return -ENOMEM;
    }

    // 해당 path에 대응하는 node 찾기
    result = uffs_TreeFindNodeByName(&dev, &node, path_copy, &type, &object_info);
    free(path_copy); // TreeFindNodeByName 호출 후 복사본은 더 이상 필요 없음
    path_copy = NULL;
    if (result != U_SUCC || node == NULL) {
        fprintf(stderr, "[uffs_readdir] node not found for path: %s\n", path);
        return -ENOENT;
    }
    if (type != UFFS_TYPE_DIR) {
        fprintf(stderr, "[uffs_readdir] this is not dir node: %s\n", path);
        return -ENOTDIR; // 디렉토리가 아님을 나타냄
    }

    // 현재 디렉토리의 serial 번호
    parent_serial = node->u.dir.serial;

    // '.'과 '..' 추가
    // '.'은 현재 디렉토리 자신
    filler(buf, ".", NULL, 0);

    // '..'은 상위 디렉토리. 루트일 경우 부모가 자기 자신일 수도 있으니
    // 실제로 상위가 없거나 ROOT_SERIAL 일 경우에는 루트로 매핑
    if (parent_serial != ROOT_SERIAL) {
        filler(buf, "..", NULL, 0);
    } else {
        // 루트 디렉토리면 상위가 없으나 Fuse는 '..'를 요구할 수 있으니 동일하게 루트로 처리
        filler(buf, "..", NULL, 0);
    }

    // 해당 디렉토리 하위에 존재하는 디렉토리 엔트리 출력
    uffs_FileInfo file_info = {0};
    for (int i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
        TreeNode *dnode = dev.tree.dir_entry[i];
        while (dnode != EMPTY_NODE) {
            if (dnode->u.dir.parent == parent_serial) {
                // '.'와 '..'를 제외한 실제 하위 디렉토리 엔트리 이름 추가
                if (getFileInfoBySerial(dev.fd, dnode->u.dir.serial, &file_info) == U_SUCC &&
                strcmp(file_info.name, "/") != 0) { 
                    // 루트 노드 이름 '/'는 하위에 직접 표시하지 않음 
                    // 필요에 따라 이 조건은 제거할 수 있음
                    filler(buf, file_info.name, NULL, 0);
                }
            }
            dnode = dnode->hash_next;
        }
    }

    // 해당 디렉토리 하위에 존재하는 파일 엔트리 출력
    for (int i = 0; i < FILE_NODE_ENTRY_LEN; i++) {
        TreeNode *fnode = dev.tree.file_entry[i];
        while (fnode != EMPTY_NODE) {
            if (fnode->u.file.parent == parent_serial) {
                if (getFileInfoBySerial(dev.fd, fnode->u.file.serial, &file_info) == U_SUCC)
                    filler(buf, file_info.name, NULL, 0);
            }
            fnode = fnode->hash_next;
        }
    }

    fprintf(stdout, "[uffs_readdir] finished\n");
    return 0;
}

int uffs_opendir(const char *path, struct fuse_file_info *fu)
{
    fprintf(stdout, "[uffs_opendir] called\n");
    TreeNode* node;
    uffs_ObjectInfo object_info = {0};
	int result;
	if (strcmp("/", path) == 0) {
        fprintf(stdout, "[uffs_opendir] finished\n");
		return 0;
	}

	result = uffs_TreeFindDirNodeByNameWithoutParent(&dev, &node, path, &object_info);

	if (result == U_SUCC) {
        fprintf(stdout, "[uffs_opendir] finished\n");
        return 0;
    }
    fprintf(stderr, "[uffs_opendir] error\n");
	return -ENOENT;
}

int uffs_open(const char *path, struct fuse_file_info *fi)
{
    fprintf(stdout, "[uffs_open] called\n");
    TreeNode* node;
    int result;
	uffs_ObjectInfo object_info ={0};
    result = uffs_TreeFindNodeByName(&dev, &node, path, NULL, &object_info);

	if (result == U_SUCC){
        fprintf(stdout, "[uffs_open] finished\n");
		return 0;	
	}
    fprintf(stderr, "[uffs_open] error\n");
	return -ENOENT;
}

/*
    1. 경로 이름을 통해서 노드를 찾아온다.
    2. 노드 안 정보를 이용해서 블록 안 페이지에 있는 데이터를 buf에 넣는다.
    3. 읽은 크기를 반환한다.
*/
int uffs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    fprintf(stdout, "[uffs_read] called\n");
    TreeNode* node;
    int result;

    // 파일 노드를 찾는다.
    result = uffs_TreeFindNodeByName(&dev, &node, path, UFFS_TYPE_FILE);
    //result = uffs_TreeFindFileNodeByNameWithoutParent(&dev, &node, path);

    if (result == U_FAIL) {
        fprintf(stderr, "[uffs_read] error\n");
        return -ENOENT;	
    } 

    if (size > node->info.len) {
        size = node->info.len;
    }
    
    // 파일 노드에서 페이지 읽어서 보내기(1파일, 1블록, 여러 페이지)
    // 가져온 파일 노드의 블록 안에서 여러 페이지들을 버퍼 안에 차례대로 넣는다.
    // 이때 페이지는 미니헤더 보고 사용중이지 않는 곳까지 읽게 한다.
    uffs_MiniHeader miniHeader = {0};
    char data[PAGE_DATA_SIZE_DEFAULT] = {0};
    uffs_Tag tag = {0};
    int page_id = 0;
    int bytes_read = 0; // 현재까지 읽은 바이트 수
    int bytes_to_read = size; // 읽어야할 바이트 수
    
    while(bytes_to_read > 0){
        readPage(dev.fd, node->u.file.block, page_id, &miniHeader, data, &tag);

        if(miniHeader.status == 0xFF) // miniheader 사용중이 아니면 페이지 없음
            break;

        // 현재 페이지에서 읽을 수 있는 최대 바이트 계산
        int bytes_from_page = bytes_to_read < PAGE_DATA_SIZE_DEFAULT ? bytes_from_page : PAGE_DATA_SIZE_DEFAULT;

        // 데이터를 버퍼로 복사
        memcpy(buf + bytes_read, data, bytes_from_page);         

        bytes_read += bytes_from_page;
        bytes_to_read -= bytes_from_page;
        page_id++;
    }
    
    fprintf(stdout, "[uffs_read] finished\n");
    return bytes_read;
}

int uffs_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    fprintf(stdout, "[uffs_write] called, path: %s, size: %zu\n", path, size);

    TreeNode *file_node;
    TreeNode *data_node;

    // 파일 노드 찾기
    if (uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, path) == U_FAIL) {
        fprintf(stderr, "[uffs_write] file node not found\n");
        return -ENOENT;
    }

    // 데이터 노드 찾기
    data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    if (data_node == NULL) {
        // 데이터 노드가 없으면 생성
        int data_block_id;
        if (getFreeBlock(dev.fd, &data_block_id) == U_FAIL) {
            fprintf(stderr, "[uffs_write] no free block available for data\n");
            return -ENOSPC;
        }

        data_node = (TreeNode *)malloc(sizeof(TreeNode));
        if (!data_node) {
            fprintf(stderr, "[uffs_write] memory allocation failed for data_node\n");
            return -ENOMEM;
        }

        if (initNode(&dev, data_node, data_block_id, UFFS_TYPE_DATA, file_node->u.file.serial) == U_FAIL) {
            fprintf(stderr, "[uffs_write] data node initialization failed\n");
            free(data_node);
            return -EIO;
        }

        file_node->u.file.block = data_block_id;
        uffs_InsertNodeToTree(&dev, UFFS_TYPE_DATA, data_node);
    }

    // 데이터 블록 초기화 (모든 페이지를 0으로 설정)
    int block_id = data_node->u.data.block;
    for (int page_id = 0; page_id < PAGES_PER_BLOCK_DEFAULT; page_id++) {
        char empty_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
        uffs_MiniHeader mini_header = {0x01, 0x00, 0xFFFF};
        uffs_Tag tag = {0};
        tag.s.dirty = 1;
        tag.s.valid = 0;
        tag.s.type = UFFS_TYPE_DATA;
        tag.s.data_len = 0;  // 초기화된 페이지는 데이터 없음
        tag.s.serial = data_node->u.data.serial;
        tag.s.page_id = page_id;
        tag.s.parent = file_node->u.file.serial;

        // 빈 페이지 쓰기
        if (writePage(dev.fd, block_id, page_id, &mini_header, empty_buf, &tag) == U_FAIL) {
            fprintf(stderr, "[uffs_write] failed to initialize page %d\n", page_id);
            return -EIO;
        }
    }

    // 현재까지 작성된 데이터
    size_t written = 0;

    // 한 블록의 최대 데이터 크기
    size_t max_block_size = PAGES_PER_BLOCK_DEFAULT * PAGE_DATA_SIZE_DEFAULT;

    // 작성할 데이터 크기 제한
    if (size > max_block_size) {
        size = max_block_size;
    }

    // 데이터 쓰기
    int page_id = 0;
    while (written < size) {
        char data_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
        size_t write_size = PAGE_DATA_SIZE_DEFAULT;

        // 남은 데이터 크기 확인
        if (size - written < write_size) {
            write_size = size - written;
        }

        // 입력 데이터를 복사
        memcpy(data_buf, buf + written, write_size);

        // 페이지 태그 설정
        uffs_MiniHeader mini_header = {0x01, 0x00, 0xFFFF};
        uffs_Tag tag = {0};
        tag.s.dirty = 1;
        tag.s.valid = 0;
        tag.s.type = UFFS_TYPE_DATA;
        tag.s.data_len = write_size;
        tag.s.serial = data_node->u.data.serial;
        tag.s.page_id = page_id;
        tag.s.parent = file_node->u.file.serial;

        // 페이지 쓰기
        if (writePage(dev.fd, block_id, page_id, &mini_header, data_buf, &tag) == U_FAIL) {
            fprintf(stderr, "[uffs_write] failed to write page %d\n", page_id);
            return -EIO;
        }

        written += write_size;
        page_id++;
        if(page_id>=PAGES_PER_BLOCK_DEFAULT)
            break;
    }

    // 파일 크기 갱신
    file_node->u.file.len = written;

    // 메타데이터 갱신
    uffs_FileInfo file_info = {0};
    updateFileInfoPage(&dev, file_node, &file_info, 0);

    fprintf(stdout, "[uffs_write] finished\n");
    return written;
}


int uffs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    fprintf(stdout, "[uffs_create] called, path: %s\n", path);

    // 부모 디렉토리 노드 찾기
    TreeNode *parent_node = NULL;
    URET result = uffs_TreeFindParentNodeByName(&dev, &parent_node, path, 0);
    if (result == U_FAIL) {
        fprintf(stderr, "[uffs_create] parent node not found\n");
        return -ENOENT;
    }

    // 파일 블록 할당
    int file_block_id;
    u16 serial;
    if (getFreeBlock(dev.fd, &file_block_id, &serial) == U_FAIL) {
        fprintf(stderr, "[uffs_create] no free block available for file\n");
        return -ENOSPC;
    }

    // 파일 노드 생성
    TreeNode *file_node = (TreeNode *)malloc(sizeof(TreeNode));

    // 파일 노드 초기화
    if (initNode(&dev, file_node, file_block_id, UFFS_TYPE_FILE, serial) == U_FAIL) {
        fprintf(stderr, "[uffs_create] file node initialization failed\n");
        return -EIO;
    }

    // 메타데이터 생성 및 작성
    uffs_FileInfo file_info = {0};

    // 이름 추출
    strncpy(file_info.name, strrchr(path, '/') ? strrchr(path, '/') + 1 : path, MAX_FILENAME_LENGTH);
    fprintf(stderr, "[uffs_create] %s\n",file_info.name);

    if (updateFileInfoPage(&dev, file_node, &file_info) == U_FAIL) {
        fprintf(stderr, "[uffs_create] file metadata write error\n");
        return -EIO;
    }

    // 파일 노드 삽입
    uffs_InsertNodeToTree(&dev, UFFS_TYPE_FILE, file_node);

    fprintf(stdout, "[uffs_create] finished\n");
    return 0;
}

/*
    --- 디스크 부분 ---
    1. 안 쓰고 있는 마지막 블록 번호 얻기
    2. 페이지 하나 얻기(0번)
    3. 블록에 페이지(미니헤더, 데이터, 태그)채우기 - 완료
    4. 디스크에 페이지 넣기 - 완료

    -- 트리 부분 -- 
    1. 현재 경로에서 현재 디렉터리 정보 얻기
    2. 트리 안 현재 디렉터리 하위에 만들 디렉터리 넣기
    3. 정보 업데이트(링크 수, 업데이트 날짜)
*/
int uffs_mkdir(const char *path, mode_t mode) {
    fprintf(stdout, "[uffs_mkdir] called, path: %s\n", path);

    // 트리 부분
    TreeNode* node;
	URET result;
    u8 type = UFFS_TYPE_DIR;
	uffs_ObjectInfo object_info ={0};

    result = uffs_TreeFindNodeByName(&dev, &node, path, &type,&object_info);

    // 디스크 부분
    uffs_MiniHeader miniHeader = {0};
    uffs_Tag tag = {0};
    uffs_FileInfo fileInfo = {0};
    int new_block_id = -1;

    // 미사용중인 마지막 블록 번호 얻기
    for(int block = 1; i < TOTAL_BLOCKS_DEFAULT; block++){
        readPage(dev.fd, block, 0, &miniHeader, (char*)&fileInfo, &tag);
        if(miniHeader.status != 0xFF){
            new_block_id = block;
            break;
        }
    }

    // 태그 설정
    tag.s.dirty = 1;
    tag.s.valid = 0;
    tag.s.type = UFFS_TYPE_DIR; 
    tag.s.block_ts = GET_CURRENT_TIME();
    tag.s.page_id = 0;
    tag.s.tag_ecc = TAG_ECC_DEFAULT;

    tag.data_sum = 0;
    tag.seal_byte = 0;

    // 디렉터리 데이터 설정
    file_info->access = GET_CURRENT_TIME();
    file_info->attr = FILE_ATTR_DIR;
    file_info->create_time = GET_CURRENT_TIME();
    file_info->last_modify = GET_CURRENT_TIME();
    strcpy(file_info->name,"/"); // TODO: 디렉터리 이름 가져오기
    file_info->name_len = strlen(file_info->name);
    file_info->reserved = 0x00;

    // 미니 헤더 설정
    miniHeader->status = 0x01; // 페이지 상태 (예: 유효한 페이지)


    // 디스크에 쓰기
    if(writePage(dev.fd, new_block_id, 0, &miniHeader, (char*)&fileInfo, &tag) < 0){
        fprintf(stderr, "[uffs_mkdir] Failed to write block %d, page %d\n", block, page);
        return EIO;
    }


    fprintf(stdout, "[uffs_mkdir] finished\n");
    return 0;
}

struct fuse_operations uffs_oper = {
	.init		= uffs_init,
	.getattr	= uffs_getattr,
	.readdir	= uffs_readdir,
    .opendir    = uffs_opendir,
    .open       = uffs_open,
    .read       = uffs_read,
    .write      = uffs_write,
    .create     = uffs_create,
    .mkdir      = uffs_mkdir
};

int main(int argc, char *argv[])
{
    // USB 디바이스 파일 오픈
    dev.fd = open(argv[3], O_RDWR, 0666);
    if (fd < 0) {
        fprintf(stderr, "[main] strerror: %s\n", strerror(errno));
        return -1;
    }

    if (argc != 4) {
        fprintf(stderr, "[main] argc is not 4 error\n");
        return -1;
    }

    if(diskFormatCheck(fd) == U_FAIL){
        fprintf(stderr, "[main] disk format check error\n");
        if(diskFormat(fd)==U_FAIL){
            fprintf(stderr, "[main] disk format error\n");
            return -1;
        }
        fprintf(stdout, "[main] disk format success\n");
    }

    fprintf(stderr, "[main] init finished\n");
    return fuse_main(3, argv, &uffs_oper, NULL);
}
