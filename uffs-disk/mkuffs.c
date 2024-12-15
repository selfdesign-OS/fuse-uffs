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

    // path를 strtok에서 안전하게 사용하기 위해 복사
    path_copy = strdup(path);
    if (!path_copy) {
        fprintf(stderr, "[uffs_readdir] memory allocation failed\n");
        return -ENOMEM;
    }

    // 해당 path에 대응하는 node 찾기
    result = uffs_TreeFindNodeByName(&dev, &node, path_copy, NULL);
    free(path_copy); // TreeFindNodeByName 호출 후 복사본은 더 이상 필요 없음
    path_copy = NULL;

    if (result != U_SUCC || node == NULL) {
        fprintf(stderr, "[uffs_readdir] node not found for path: %s\n", path);
        return -ENOENT;
    }

    // TODO: 호출한 path의 node가 디렉토리인지 확인
    // mode_t mode = S_IFDIR | node->info.mode;
    // if (!S_ISDIR(mode)) {
    //     fprintf(stderr, "[uffs_readdir] not a directory: %s\n", path);
    //     return -ENOTDIR;
    // }

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
    for (int i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
        TreeNode *dnode = dev.tree.dir_entry[i];
        while (dnode != EMPTY_NODE) {
            if (dnode->u.dir.parent == parent_serial) {
                // '.'와 '..'를 제외한 실제 하위 디렉토리 엔트리 이름 추가
                if (strcmp(dnode->info.name, "/") != 0) { 
                    // 루트 노드 이름 '/'는 하위에 직접 표시하지 않음 
                    // 필요에 따라 이 조건은 제거할 수 있음
                    filler(buf, dnode->info.name, NULL, 0);
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
                // 파일 이름 출력
                filler(buf, fnode->info.name, NULL, 0);
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
	int result;
	if (strcmp("/", path) == 0) {
        fprintf(stdout, "[uffs_opendir] finished\n");
		return 0;
	}

	result = uffs_TreeFindDirNodeByNameWithoutParent(&dev, &node, path);

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
    result = uffs_TreeFindNodeByName(&dev, &node, path, NULL);

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
		      struct fuse_file_info *fi)
{   
    fprintf(stdout, "[uffs_write] called\n");
    URET block_result, node_result;
    TreeNode* node;
    data_Block* block;

    // 쓰기 범위 확인
    if (size > BLOCK_DATA_SIZE) {
        fprintf(stderr, "[uffs_write] file size too big\n");
        return -EFBIG; // 파일 크기 초과 오류
    }
    node_result = uffs_TreeFindFileNodeByNameWithoutParent(&dev, &node, path);
    if(node_result == U_FAIL) {
        fprintf(stderr, "[uffs_write] find node error\n");
        return -ENOSPC;
    }
    block_result = getUsedBlockById(&disk,&block, node->u.file.block);
    if(block_result == U_FAIL) {
        fprintf(stderr, "[uffs_write] getUsedBlockById error\n");
        return -ENOSPC;
    }
	memcpy(block->data, buf, size);
    node->info.len = size;
    block->tag.data_len = node->info.len;


    return size;
}

int uffs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    fprintf(stdout, "[uffs_create] called, path: %s\n", path);

    // 블록 할당
    data_Block *freeBlock;
    URET result = getFreeBlock(&disk, &freeBlock);
    if (result == U_FAIL) {
        fprintf(stderr, "[uffs_create] get freeblock error\n");
        return -ENOSPC;
    }

    // 블록 초기화
    URET initBlockResult = initBlock(&freeBlock, UFFS_TYPE_FILE, 0);
    if (initBlockResult == U_FAIL) {
        fprintf(stderr, "[uffs_create] initBlock error\n");
        return -EINVAL;
    }

    // 노드 생성
    TreeNode *node = (TreeNode *) malloc(sizeof(TreeNode));
    memset(node, 0, sizeof(TreeNode));
    URET initNodeResult = initNode(&dev, node, freeBlock, path, UFFS_TYPE_FILE);
    if (initNodeResult == U_FAIL) {
        fprintf(stderr, "[uffs_create] init node error\n");
        return -EINVAL;
    }

    freeBlock->status = usedblock;
    uffs_InsertNodeToTree(&dev, UFFS_TYPE_FILE, node);

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
