/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall uffs.c `pkg-config fuse --cflags --libs` -o uffs
*/

#ifdef UNIT_TEST
#include "fuse_compat.h"
#else
#define FUSE_USE_VERSION 26
#include <fuse.h>
#endif
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
    
    // stbuf->st_mode = (object_info.info.attr & FILE_ATTR_DIR ? US_IFDIR : US_IFREG);
    // // TODO: should implement nlink
    // stbuf->st_nlink = 2;
    // stbuf->st_size = object_info.len;

    if (type == UFFS_TYPE_DIR) {
        // 디렉토리인 경우
        stbuf->st_mode = __S_IFDIR | 0755;
        stbuf->st_nlink = 2; // 기본적으로 '.'과 '..' 때문에 최소 2
        stbuf->st_size = object_info.len; // 일반적으로 디렉토리는 고정 크기로 설정
    } else if (type == UFFS_TYPE_FILE) {
        // 파일인 경우
        stbuf->st_mode = __S_IFREG | 0644;
        stbuf->st_nlink = 1; // 일반적으로 파일은 링크 개수가 1
        stbuf->st_size = node->u.file.len; // 파일의 실제 길이
    } else {
        // 알려지지 않은 타입일 경우 에러 처리
        return -ENOENT;
    }

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
    if (parent_serial != ROOT_DIR_SERIAL) {
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
    fprintf(stdout, "[uffs_read] called, path: %s, size: %zu, offset: %lld\n",
            path, size, (long long)offset);
    TreeNode *file_node;
    u8 type = UFFS_TYPE_FILE;

    if (uffs_TreeFindNodeByName(&dev, &file_node, path, &type, NULL) == U_FAIL || type != UFFS_TYPE_FILE) {
        fprintf(stderr, "[uffs_read] file node not found\n");
        return -ENOENT;
    }

    // offset이 파일 끝을 넘으면 읽을 것 없음
    if ((size_t)offset >= (size_t)file_node->u.file.len)
        return 0;

    // 읽을 수 있는 바이트 수 제한
    size_t avail = (size_t)file_node->u.file.len - (size_t)offset;
    if (size > avail)
        size = avail;

    int file_data_pages = PAGES_PER_BLOCK_DEFAULT - 1;
    size_t file_block_end = (size_t)file_data_pages * PAGE_DATA_SIZE_DEFAULT;

    TreeNode *data_node = NULL;
    if ((size_t)offset + size > file_block_end)
        data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);

    uffs_MiniHeader mh = {0};
    char data_buf[PAGE_DATA_SIZE_DEFAULT];
    uffs_Tag tag = {0};
    size_t bytes_read = 0;
    size_t cur_offset = (size_t)offset;

    while (bytes_read < size) {
        int logical_page = (int)(cur_offset / PAGE_DATA_SIZE_DEFAULT);
        int byte_in_page = (int)(cur_offset % PAGE_DATA_SIZE_DEFAULT);

        int block, phys_page;
        if (logical_page < file_data_pages) {
            block     = file_node->u.file.block;
            phys_page = logical_page + 1;
        } else {
            if (data_node == NULL)
                break;
            block     = data_node->u.data.block;
            phys_page = logical_page - file_data_pages;
        }

        memset(data_buf, 0, sizeof(data_buf));
        readPage(dev.fd, block, phys_page, &mh, data_buf, &tag);
        if (mh.status == 0xFF)
            break;

        // 이 페이지에서 읽을 수 있는 바이트 수
        int page_avail = (int)tag.s.data_len - byte_in_page;
        if (page_avail <= 0)
            break;
        int from_page = (int)(size - bytes_read) < page_avail
                        ? (int)(size - bytes_read) : page_avail;

        memcpy(buf + bytes_read, data_buf + byte_in_page, from_page);
        bytes_read += from_page;
        cur_offset += from_page;
    }

    fprintf(stdout, "[uffs_read] finished, read: %zu\n", bytes_read);
    return (int)bytes_read;
}

int uffs_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    fprintf(stdout, "[uffs_write] called, path: %s, size: %zu, offset: %lld\n",
            path, size, (long long)offset);

    TreeNode *file_node;
    if (uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, path) == U_FAIL) {
        fprintf(stderr, "[uffs_write] file node not found\n");
        return -ENOENT;
    }

    // 파일 블록 데이터 영역 끝 (byte 기준)
    size_t file_block_end = (size_t)(PAGES_PER_BLOCK_DEFAULT - 1) * PAGE_DATA_SIZE_DEFAULT;
    size_t write_end = (size_t)offset + size;

    // 쓰기가 데이터 블록 영역에 걸쳐 있으면 데이터 노드 미리 확보
    TreeNode *data_node = NULL;
    if (write_end > file_block_end) {
        data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
        if (data_node == NULL) {
            int data_block_id;
            u16 serial;
            if (getFreeBlock(dev.fd, &data_block_id, &serial) == U_FAIL) {
                fprintf(stderr, "[uffs_write] no free block for data\n");
                return -ENOSPC;
            }
            data_node = (TreeNode *)malloc(sizeof(TreeNode));
            if (!data_node) return -ENOMEM;
            if (initNode(&dev, data_node, data_block_id, UFFS_TYPE_DATA,
                         file_node->u.file.serial, serial) == U_FAIL) {
                free(data_node);
                return -EIO;
            }
            uffs_InsertNodeToTree(&dev, UFFS_TYPE_DATA, data_node);
        }
    }

    int file_data_pages = PAGES_PER_BLOCK_DEFAULT - 1; // 파일 블록 데이터 페이지 수 (1~31)
    size_t written = 0;
    size_t cur_offset = (size_t)offset;

    while (written < size) {
        int logical_page  = (int)(cur_offset / PAGE_DATA_SIZE_DEFAULT);
        int byte_in_page  = (int)(cur_offset % PAGE_DATA_SIZE_DEFAULT);
        int bytes_this_page = PAGE_DATA_SIZE_DEFAULT - byte_in_page;
        if ((size_t)bytes_this_page > size - written)
            bytes_this_page = (int)(size - written);

        // 논리 페이지 → 물리 블록/페이지 매핑
        int block, phys_page;
        u8  tag_type;
        u16 tag_serial, tag_parent;
        if (logical_page < file_data_pages) {
            // 파일 블록 (page 0은 메타데이터이므로 +1)
            block      = file_node->u.file.block;
            phys_page  = logical_page + 1;
            tag_type   = UFFS_TYPE_FILE;
            tag_serial = file_node->u.file.serial;
            tag_parent = file_node->u.file.parent;
        } else {
            // 데이터 블록
            block      = data_node->u.data.block;
            phys_page  = logical_page - file_data_pages;
            tag_type   = UFFS_TYPE_DATA;
            tag_serial = data_node->u.data.serial;
            tag_parent = file_node->u.file.serial;
        }

        // 기존 페이지 읽기 (partial write 시 나머지 바이트 보존)
        char page_data[PAGE_DATA_SIZE_DEFAULT] = {0};
        uffs_MiniHeader mh = {0};
        uffs_Tag existing_tag = {0};
        readPage(dev.fd, block, phys_page, &mh, page_data, &existing_tag);

        int existing_data_len = (mh.status == 0xFF) ? 0 : (int)existing_tag.s.data_len;

        // 새 데이터 덮어쓰기
        memcpy(page_data + byte_in_page, buf + written, bytes_this_page);

        // 페이지 유효 길이: 기존과 이번 쓰기 범위 중 큰 값
        int new_data_len = byte_in_page + bytes_this_page;
        if (existing_data_len > new_data_len)
            new_data_len = existing_data_len;

        uffs_MiniHeader new_mh = {0x01, 0x00, 0xFFFF};
        uffs_Tag new_tag = {0};
        new_tag.s.dirty    = 1;
        new_tag.s.valid    = 0;
        new_tag.s.type     = tag_type;
        new_tag.s.data_len = new_data_len;
        new_tag.s.serial   = tag_serial;
        new_tag.s.parent   = tag_parent;
        new_tag.s.page_id  = phys_page;
        new_tag.s.tag_ecc  = TAG_ECC_DEFAULT;

        if (writePage(dev.fd, block, phys_page, &new_mh, page_data, &new_tag) == U_FAIL) {
            fprintf(stderr, "[uffs_write] failed to write page (block=%d, page=%d)\n",
                    block, phys_page);
            return written > 0 ? (int)written : -EIO;
        }

        written    += bytes_this_page;
        cur_offset += bytes_this_page;
    }

    // 파일 길이 갱신: 파일이 확장된 경우에만 업데이트
    if (write_end > (size_t)file_node->u.file.len)
        file_node->u.file.len = write_end;

    // 데이터 노드 길이 갱신
    if (data_node != NULL && write_end > file_block_end) {
        size_t data_used = write_end - file_block_end;
        if (data_used > data_node->u.data.len)
            data_node->u.data.len = data_used;
    }

    // 메타데이터 갱신 (파일 이름 보존)
    uffs_FileInfo file_info = {0};
    getFileInfoBySerial(dev.fd, file_node->u.file.serial, &file_info);
    updateFileInfoPage(&dev, file_node, &file_info, 0, UFFS_TYPE_FILE);

    fprintf(stdout, "[uffs_write] finished, written: %zu\n", written);
    return (int)written;
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

#ifndef UNIT_TEST
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
#endif /* UNIT_TEST */
