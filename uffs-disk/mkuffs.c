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

#include "uffs_types.h"
#include "uffs_tree.h"
#include <errno.h>

uffs_Device dev = {0};

int uffs_init()
{
	fprintf(stdout, "[uffs_init] called\n");
	uffs_TreeInit(&dev);
	uffs_BuildTree(&dev);
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
		result = uffs_TreeFindNodeByName(&dev, &node, path, &type, &object_info);
	}
	else {
		if (result = uffs_TreeFindNodeByName(&dev, &node, path, &type, &object_info) != U_SUCC) {
			fprintf(stderr, "[uffs_getattr] result is U_FAIL\n");
            return -ENOENT;
        }
	}
    
    stbuf->st_mode = (object_info.info.attr & FILE_ATTR_DIR ? US_IFDIR : US_IFREG);
    // TODO: should implement nlink
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

    // path를 strtok에서 안전하게 사용하기 위해 복사
    path_copy = strdup(path);
    if (!path_copy) {
        fprintf(stderr, "[uffs_readdir] memory allocation failed\n");
        return -ENOMEM;
    }

    // 해당 path에 대응하는 node 찾기
    result = uffs_TreeFindNodeByName(&dev, &node, path_copy, &type, NULL);
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
    
	URET result;
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
    URET result;
    result = uffs_TreeFindNodeByName(&dev, &node, path, NULL, NULL);

	if (result == U_SUCC){
        fprintf(stdout, "[uffs_open] finished\n");
		return 0;	
	}
    fprintf(stderr, "[uffs_open] error\n");
	return -ENOENT;
}

int uffs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    fprintf(stdout, "[uffs_read] called\n");
    TreeNode* file_node;
    TreeNode* data_node;
    int result;
    
    // 파일 노드를 찾는다.
    result = uffs_TreeFindNodeByName(&dev, &file_node, path, UFFS_TYPE_FILE, NULL);

    if (result == U_FAIL) {
        fprintf(stderr, "[uffs_read] file node error\n");
        return -ENOENT;	
    } 

    
    data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);

    if(data_node == NULL){
        fprintf(stderr, "[uffs_read] data node error\n");
        return -ENOENT;
    }

    if (size > data_node->u.data.len) {
        size = data_node->u.data.len;
    }

    uffs_MiniHeader miniHeader = {0};
    char data_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    int page_id = 0;
    int bytes_read = 0; // 현재까지 읽은 바이트 수
    int bytes_to_read = size; // 읽어야할 바이트 수
    
    while(bytes_to_read > 0){
        readPage(dev.fd, data_node->u.file.block, page_id, &miniHeader, data_buf, NULL);

        if(miniHeader.status == 0xFF) // miniheader 사용중이 아니면 페이지 없음
            break;

        // 현재 페이지에서 읽을 수 있는 최대 바이트 계산
        int bytes_from_page = bytes_to_read < PAGE_DATA_SIZE_DEFAULT ? bytes_from_page : PAGE_DATA_SIZE_DEFAULT;

        // 데이터를 버퍼로 복사
        memcpy(buf + bytes_read, data_buf, bytes_from_page);         

        bytes_read += bytes_from_page;
        bytes_to_read -= bytes_from_page;
        page_id++;
        if(page_id>=PAGES_PER_BLOCK_DEFAULT)
            break;
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
        u16 serial;
        if (getFreeBlock(dev.fd, &data_block_id, &serial) == U_FAIL) {
            fprintf(stderr, "[uffs_write] no free block available for data\n");
            return -ENOSPC;
        }

        data_node = (TreeNode *)malloc(sizeof(TreeNode));
        if (!data_node) {
            fprintf(stderr, "[uffs_write] memory allocation failed for data_node\n");
            return -ENOMEM;
        }

        if (initNode(&dev, data_node, data_block_id, UFFS_TYPE_DATA, file_node->u.file.serial, serial) == U_FAIL) {
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
    updateFileInfoPage(&dev, file_node, &file_info, 0, UFFS_TYPE_FILE);

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
    if (initNode(&dev, file_node, file_block_id, UFFS_TYPE_FILE, parent_node->u.dir.serial, serial) == U_FAIL) {
        fprintf(stderr, "[uffs_create] file node initialization failed\n");
        return -EIO;
    }

    // 메타데이터 생성 및 작성
    uffs_FileInfo file_info = {0};

    // 파일 이름 추출
    char file_name[MAX_FILENAME_LENGTH]; // 크기가 충분한 버퍼로 선언
    if (strrchr(path, '/')) {
        strncpy(file_name, strrchr(path, '/') + 1, MAX_FILENAME_LENGTH - 1); // 최대 길이만큼 복사
    } else {
        strncpy(file_name, path, MAX_FILENAME_LENGTH - 1);
    }
    file_name[MAX_FILENAME_LENGTH - 1] = '\0'; // 널 종료 보장

    // 길이 검사
    if (strlen(file_name) > MAX_FILENAME_LENGTH - 1) {
        return -ENOENT;
    }

    // 파일 정보에 이름 복사
    strncpy(file_info.name, file_name, MAX_FILENAME_LENGTH - 1);
    file_info.name[MAX_FILENAME_LENGTH - 1] = '\0'; // 널 종료 보장

    fprintf(stdout, "[uffs_create] fileName: %s\n", file_info.name);


    if (updateFileInfoPage(&dev, file_node, &file_info, 1, UFFS_TYPE_FILE) == U_FAIL) {
        fprintf(stderr, "[uffs_create] file metadata write error\n");
        return -EIO;
    }

    // 파일 노드 삽입
    uffs_InsertNodeToTree(&dev, UFFS_TYPE_FILE, file_node);

    fprintf(stdout, "[uffs_create] finished\n");
    return 0;
}

int uffs_mkdir(const char *path, mode_t mode) {
    fprintf(stdout, "[uffs_mkdir] called, path: %s\n", path);
    // 트리 부분
    TreeNode* parent_node;
    TreeNode* dir_node;
	URET result;
    u8 type = UFFS_TYPE_DIR;

    // 부모 디렉토리 노드 찾기
    result = uffs_TreeFindParentNodeByName(&dev, &parent_node, path, 0);
    if (result == U_FAIL) {
        fprintf(stderr, "[uffs_mkdir] parent node not found\n");
        return -ENOENT;
    }

    // 디스크 부분
    uffs_MiniHeader miniHeader = {0};
    uffs_Tag tag = {0};
    uffs_FileInfo dir_file_info = {0};
    int new_block_id = -1;
    u16 serial;

    result = getFreeBlock(dev.fd, &new_block_id, &serial);
    if(result == U_FAIL){
        return -ENOENT;
    }

    dir_node = (TreeNode*)malloc(sizeof(TreeNode));
    initNode(&dev, dir_node, new_block_id, UFFS_TYPE_DIR, parent_node->u.dir.serial, serial);
    
    // 파일 이름 추출
    char dir_name[MAX_FILENAME_LENGTH]; // 크기가 충분한 버퍼로 선언
    if (strrchr(path, '/')) {
        strncpy(dir_name, strrchr(path, '/') + 1, MAX_FILENAME_LENGTH - 1); // 최대 길이만큼 복사
    } else {
        strncpy(dir_name, path, MAX_FILENAME_LENGTH - 1);
    }
    dir_name[MAX_FILENAME_LENGTH - 1] = '\0'; // 널 종료 보장

    // 길이 검사
    if (strlen(dir_name) > MAX_FILENAME_LENGTH - 1) {
        return -ENOENT;
    }

    // 파일 정보에 이름 복사
    strncpy(dir_file_info.name, dir_name, MAX_FILENAME_LENGTH - 1);
    dir_file_info.name[MAX_FILENAME_LENGTH - 1] = '\0'; // 널 종료 보장

    fprintf(stdout, "[uffs_mkdir] fileName: %s\n", dir_file_info.name);
    
    // 디스크 업데이트
    if(updateFileInfoPage(&dev, dir_node, &dir_file_info, 1, UFFS_TYPE_DIR) == U_FAIL){
        return -ENOENT;
    }
    // 트리에 노드 추가
    uffs_InsertNodeToTree(&dev, UFFS_TYPE_DIR, dir_node);

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
    if (dev.fd < 0) {
        fprintf(stderr, "[main] strerror: %s\n", strerror(errno));
        return -1;
    }

    if (argc != 4) {
        fprintf(stderr, "[main] argc is not 4 error\n");
        return -1;
    }

    if(diskFormatCheck(dev.fd) == U_FAIL){
        fprintf(stderr, "[main] disk format check error\n");
        if(diskFormat(dev.fd)==U_FAIL){
            fprintf(stderr, "[main] disk format error\n");
            return -1;
        }
        fprintf(stdout, "[main] disk format success\n");
    }

    fprintf(stderr, "[main] init finished\n");
    return fuse_main(3, argv, &uffs_oper, NULL);
}
