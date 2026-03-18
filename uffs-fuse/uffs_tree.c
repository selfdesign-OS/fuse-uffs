/**
 * \file uffs_tree.c
 * \brief seting up uffs tree data structure
 * \author Ricky Zheng, created 13th May, 2005
 */

#include "uffs_tree.h"

#include <stdlib.h>
#include <string.h>

static void _InsertToEntry(uffs_Device *dev, uint64_t *entry,
						   int hash, TreeNode *node)
{
	node->hash_next = entry[hash];
	node->hash_prev = EMPTY_NODE;
    if ((TreeNode*)node->hash_next != (TreeNode*)EMPTY_NODE) {
        TreeNode* temp_node = (TreeNode*)node->hash_next;
        temp_node->hash_prev = (uint64_t)node;
    }
    entry[hash] = (uint64_t)node;
}

static void uffs_InsertToFileEntry(uffs_Device *dev, TreeNode *node)
{
	_InsertToEntry(dev, dev->tree.file_entry,
					GET_FILE_HASH(node->u.file.serial),
					node);
}

static void uffs_InsertToDirEntry(uffs_Device *dev, TreeNode *node)
{
	_InsertToEntry(dev, dev->tree.dir_entry,
					GET_DIR_HASH(node->u.dir.serial),
					node);
}

static void uffs_InsertToDataEntry(uffs_Device *dev, TreeNode *node)
{
	_InsertToEntry(dev, dev->tree.data_entry,
					GET_DATA_HASH(node->u.data.parent, node->u.data.serial),
					node);
}

void uffs_InsertNodeToTree(uffs_Device *dev, u8 type, TreeNode *node)
{
    fprintf(stdout,"[uffs_InsertNodeToTree] called\n");
    switch (type) {
    case UFFS_TYPE_DIR:
        uffs_InsertToDirEntry(dev, node);
        break;
    case UFFS_TYPE_FILE:
        uffs_InsertToFileEntry(dev, node);
        break;
    case UFFS_TYPE_DATA:
        uffs_InsertToDataEntry(dev, node);
        break;
    default:
        fprintf(stderr, "[uffs_InsertNodeToTree] node type error\n");
        break;
    }
    fprintf(stdout,"[uffs_InsertNodeToTree] finished\n");
}

URET uffs_TreeInit(uffs_Device *dev)
{
    fprintf(stdout, "[uffs_TreeInit] called\n");

    int i;

    for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		dev->tree.dir_entry[i] = EMPTY_NODE;
	}

	for (i = 0; i < FILE_NODE_ENTRY_LEN; i++) {
		dev->tree.file_entry[i] = EMPTY_NODE;
	}

	for (i = 0; i < DATA_NODE_ENTRY_LEN; i++) {
		dev->tree.data_entry[i] = EMPTY_NODE;
	}

	dev->tree.max_serial = ROOT_DIR_SERIAL;

    fprintf(stdout,"[uffs_TreeInit] finished\n");
	return U_SUCC;
}

static u32 GET_CURRENT_TIME() {
    time_t now = time(NULL);
    return (u32)now;
}

URET uffs_BuildTree(uffs_Device *dev) {
    fprintf(stdout, "[uffs_BuildTree] called\n");

    for (int block = 1; block < TOTAL_BLOCKS_DEFAULT; block++) {
        uffs_Tag tag = {0};
        char data[PAGE_DATA_SIZE_DEFAULT];
        uffs_MiniHeader mini_header = {0};
        readPage(dev->fd, block, 0, &mini_header, data, &tag);
        TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
        memset(node, 0, sizeof(TreeNode));
        fprintf(stdout, "[uffs_BuildTree] block: %d, type of tag: %d\n", block, tag.s.type);
        switch (tag.s.type) {
		case UFFS_TYPE_DIR:
			node->u.dir.parent = tag.s.parent;
			node->u.dir.serial = tag.s.serial;
			node->u.dir.block = block;
			node->u.dir.checksum = tag.data_sum;
            fprintf(stdout, "[uffs_BuildTree] made dir node - name: %s\n", ((uffs_FileInfo *)data)->name);
            uffs_InsertToDirEntry(dev, node);
			break;
		case UFFS_TYPE_FILE:
			node->u.file.parent = tag.s.parent;
			node->u.file.serial = tag.s.serial;
			node->u.file.block = block;
			node->u.file.checksum = tag.data_sum;
            // 파일 블록 page 1~에서 데이터 길이 합산 (page 0은 메타데이터)
            node->u.file.len = 0;
            for (int p = 1; p < PAGES_PER_BLOCK_DEFAULT; p++) {
                uffs_MiniHeader mh = {0};
                uffs_Tag t = {0};
                readPage(dev->fd, block, p, &mh, NULL, &t);
                if (mh.status == 0xFF) break;
                node->u.file.len += t.s.data_len;
            }
            uffs_InsertToFileEntry(dev, node);
            fprintf(stdout, "[uffs_BuildTree] made file node - name: %s, len: %u\n", ((uffs_FileInfo *)data)->name, node->u.file.len);
			break;
		case UFFS_TYPE_DATA:
			node->u.data.parent = tag.s.parent;
			node->u.data.serial = tag.s.serial;
			node->u.data.block = block;

            int page_id = 0;
            while(page_id < PAGES_PER_BLOCK_DEFAULT){
                readPage(dev->fd, node->u.file.block, page_id, &mini_header, NULL, &tag);

                if(mini_header.status == 0xFF)
                    break;

                node->u.data.len+=tag.s.data_len;
                page_id++;
            }
            uffs_InsertToDataEntry(dev, node);
            fprintf(stdout, "[uffs_BuildTree] made data node\n");
			break;
		default:
			fprintf(stderr, "[uffs_BuildTree] UNKNOW TYPE error\n");
			break;
		}
    }

    // 데이터 블록 길이를 파일 노드에 합산
    for (int i = 0; i < DATA_NODE_ENTRY_LEN; i++) {
        TreeNode *dnode = (TreeNode*)dev->tree.data_entry[i];
        while (dnode != (TreeNode*)EMPTY_NODE) {
            TreeNode *fnode = uffs_TreeFindFileNode(dev, dnode->u.data.parent);
            if (fnode != NULL)
                fnode->u.file.len += dnode->u.data.len;
            dnode = (TreeNode*)dnode->hash_next;
        }
    }

    fprintf(stderr,"[uffs_BuildTree] finished\n");
    return U_SUCC;
}

static URET getRootDir(uffs_Device *dev, TreeNode **cur_node) {
    int hash = GET_DIR_HASH(ROOT_DIR_SERIAL);
    *cur_node = (TreeNode*)dev->tree.dir_entry[hash];
    while ((*cur_node)->u.dir.serial != ROOT_DIR_SERIAL) {
        if ((*cur_node)->hash_next == EMPTY_NODE) {
            fprintf(stderr, "[getRootDir] fail - can't find root node\n");
            return U_FAIL;
        }
        *cur_node = (TreeNode*)(*cur_node)->hash_next;
    }
    return U_SUCC;
}

TreeNode * uffs_TreeFindDirNode(uffs_Device *dev, u16 serial) {
    int hash = GET_DIR_HASH(serial);
    TreeNode *node = (TreeNode*)dev->tree.dir_entry[hash];
    while (node != (TreeNode*)EMPTY_NODE) {
        if (node->u.dir.serial == serial)
            return node;
        node = (TreeNode*)node->hash_next;
    }
    return NULL;
}

TreeNode * uffs_TreeFindFileNode(uffs_Device *dev, u16 serial) {
    int hash = GET_FILE_HASH(serial);
    TreeNode *node = (TreeNode*)dev->tree.file_entry[hash];
    while (node != (TreeNode*)EMPTY_NODE) {
        if (node->u.file.serial == serial)
            return node;
        node = (TreeNode*)node->hash_next;
    }
    return NULL;
}

TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial) {
    int hash = GET_DATA_HASH(parent, serial);
    TreeNode *node = (TreeNode*)dev->tree.data_entry[hash];
    while (node != (TreeNode*)EMPTY_NODE) {
        if (node->u.data.parent == parent && node->u.data.serial == serial)
            return node;
        node = (TreeNode*)node->hash_next;
    }
    return NULL;
}

TreeNode * uffs_TreeFindFileNodeWithParent(uffs_Device *dev, u16 parent) {
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeWithParent(uffs_Device *dev, u16 parent) {
	return NULL;
}

TreeNode * uffs_TreeFindDataNodeByParent(uffs_Device *dev, u16 parent) {
    struct uffs_TreeSt *tree = &(dev->tree);
    for (int i = 0; i < DATA_NODE_ENTRY_LEN; i++) {
        TreeNode *node = (TreeNode*)tree->data_entry[i];
		while (node != (TreeNode*)EMPTY_NODE) {
			if (node->u.data.parent == parent)
                return node;
			node = (TreeNode*)node->hash_next;
		}
	}
    return NULL;
}

static UBOOL uffs_TreeCompareFileName(uffs_Device* dev, const char* name, u16 parent, uffs_ObjectInfo* object_info, u8 type){
    uffs_ObjectInfo temp_objectInfo = {0};

    for(int i = 1; i < TOTAL_BLOCKS_DEFAULT; i++){
        uffs_Tag tag = {0};
        readPage(dev->fd, i, 0, NULL, (char*)&temp_objectInfo.info, &tag);
        if(tag.s.parent == parent && strcmp(temp_objectInfo.info.name, name) == 0 && tag.s.type == type){
            temp_objectInfo.len = 0;
            temp_objectInfo.serial = tag.s.serial;
            if(object_info != NULL)
                *object_info = temp_objectInfo;
            return U_TRUE;
        }
    }
    return U_FALSE;
}

TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info) {
    uffs_ObjectInfo temp_info = {0};
    uffs_ObjectInfo *info = object_info ? object_info : &temp_info;
    if (uffs_TreeCompareFileName(dev, name, parent, info, UFFS_TYPE_FILE) != U_TRUE)
        return NULL;
    return uffs_TreeFindFileNode(dev, info->serial);
}

TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info) {
    uffs_ObjectInfo temp_info = {0};
    uffs_ObjectInfo *info = object_info ? object_info : &temp_info;
    if (uffs_TreeCompareFileName(dev, name, parent, info, UFFS_TYPE_DIR) != U_TRUE)
        return NULL;
    return uffs_TreeFindDirNode(dev, info->serial);
}

// 경로 문자열을 토큰 단위로 루트에서부터 순회하여 최종 노드를 반환
// out_type: UFFS_TYPE_DIR 또는 UFFS_TYPE_FILE
static URET walk_path(uffs_Device *dev, const char *path,
                      TreeNode **out_node, u8 *out_type,
                      uffs_ObjectInfo *object_info)
{
    char temp[MAX_FILENAME_LENGTH + 1];
    size_t path_len = strlen(path);
    if (path_len > MAX_FILENAME_LENGTH)
        path_len = MAX_FILENAME_LENGTH;
    strncpy(temp, path, path_len);
    temp[path_len] = '\0';

    TreeNode *cur;
    if (getRootDir(dev, &cur) == U_FAIL)
        return U_FAIL;

    u8 cur_type = UFFS_TYPE_DIR;
    char *token = strtok(temp, "/");
    while (token != NULL) {
        TreeNode *next = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur->u.dir.serial, object_info);
        if (next != NULL) {
            cur_type = UFFS_TYPE_DIR;
        } else {
            next = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur->u.dir.serial, object_info);
            if (next == NULL)
                return U_FAIL;
            cur_type = UFFS_TYPE_FILE;
        }
        cur = next;
        token = strtok(NULL, "/");
    }

    *out_node = cur;
    if (out_type)
        *out_type = cur_type;
    return U_SUCC;
}

URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name, u8 *type, uffs_ObjectInfo* object_info) {
    fprintf(stdout, "[uffs_TreeFindNodeByName] called\n");
    URET ret = walk_path(dev, name, node, type, object_info);
    if (ret == U_SUCC)
        fprintf(stdout, "[uffs_TreeFindNodeByName] finished - type: %d\n", type ? *type : -1);
    else
        fprintf(stderr, "[uffs_TreeFindNodeByName] failed\n");
    return ret;
}

URET uffs_TreeFindDirNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] called\n");
    u8 type;
    if (walk_path(dev, name, node, &type, NULL) == U_FAIL) {
        fprintf(stderr, "[uffs_TreeFindDirNodeByNameWithoutParent] failed\n");
        return U_FAIL;
    }
    if (type != UFFS_TYPE_DIR) {
        fprintf(stderr, "[uffs_TreeFindDirNodeByNameWithoutParent] not a dir\n");
        return U_FAIL;
    }
    fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

URET uffs_TreeFindFileNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] called\n");
    u8 type;
    if (walk_path(dev, name, node, &type, NULL) == U_FAIL) {
        fprintf(stderr, "[uffs_TreeFindFileNodeByNameWithoutParent] failed\n");
        return U_FAIL;
    }
    if (type != UFFS_TYPE_FILE) {
        fprintf(stderr, "[uffs_TreeFindFileNodeByNameWithoutParent] not a file\n");
        return U_FAIL;
    }
    fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

URET uffs_TreeFindParentNodeByName(uffs_Device *dev, TreeNode **node, const char *name, int isNodeExist) {
    fprintf(stdout, "[uffs_TreeFindParentNodeByName] called\n");
    if (strcmp(name, "/") == 0)
        return U_FAIL;

    if (isNodeExist) {
        TreeNode *target;
        u8 type;
        if (walk_path(dev, name, &target, &type, NULL) == U_FAIL) {
            fprintf(stderr, "[uffs_TreeFindParentNodeByName] target not found\n");
            return U_FAIL;
        }
        u16 parent_serial = (type == UFFS_TYPE_DIR) ? target->u.dir.parent : target->u.file.parent;
        *node = uffs_TreeFindDirNode(dev, parent_serial);
    } else {
        // 마지막 경로 요소가 아직 존재하지 않으므로, 그 앞까지 탐색
        const char *last_slash = strrchr(name, '/');
        if (last_slash == NULL || last_slash == name) {
            // 부모가 루트 디렉터리
            return getRootDir(dev, node);
        }
        char parent_path[MAX_FILENAME_LENGTH + 1];
        size_t parent_len = last_slash - name;
        if (parent_len > MAX_FILENAME_LENGTH)
            parent_len = MAX_FILENAME_LENGTH;
        strncpy(parent_path, name, parent_len);
        parent_path[parent_len] = '\0';
        u8 type;
        if (walk_path(dev, parent_path, node, &type, NULL) == U_FAIL) {
            fprintf(stderr, "[uffs_TreeFindParentNodeByName] parent not found\n");
            return U_FAIL;
        }
    }

    if (*node == NULL) {
        fprintf(stderr, "[uffs_TreeFindParentNodeByName] parent dir not found\n");
        return U_FAIL;
    }
    fprintf(stdout, "[uffs_TreeFindParentNodeByName] finished\n");
    return U_SUCC;
}

// 경로에서 파일 이름 추출 및 길이 반환
int parsePath(const char *path, char *nameBuffer, int maxNameLength) {
    fprintf(stdout,"[parsePath] called\n");
    const char *lastSlash = strrchr(path, '/');
    if (lastSlash == NULL) {
        strncpy(nameBuffer, path, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0';
    } else {
        strncpy(nameBuffer, lastSlash + 1, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0';
    }
    fprintf(stdout,"[parsePath] finished - name: %s\n", nameBuffer);
    return strlen(nameBuffer);
}

// 노드 초기화
URET initNode(uffs_Device *dev, TreeNode *node, int block_id, u8 type, u16 parent_serial, u16 serial) {

    if(block_id == -1)
        return U_FAIL;

    memset(node, 0, sizeof(TreeNode));

    if (type == UFFS_TYPE_FILE) {
        node->u.file.block = block_id;
        node->u.file.checksum = 0;
        node->u.file.parent = parent_serial;
        node->u.file.serial = serial;
        node->u.file.len = 0;
    } else if (type == UFFS_TYPE_DIR) {
        node->u.dir.block = block_id;
        node->u.dir.checksum = 0;
        node->u.dir.parent = parent_serial;
        node->u.dir.serial = serial;
    } else if (type == UFFS_TYPE_DATA) {
        node->u.data.block = block_id;
        node->u.data.parent = parent_serial;
        node->u.data.len = 0;
        node->u.data.serial = serial;
    } else {
        return U_FAIL;
    }

    return U_SUCC;
}

// 메타데이터 작성
URET updateFileInfoPage(uffs_Device *dev, TreeNode *node, uffs_FileInfo *file_info, int is_create, u8 type) {

    uffs_MiniHeader mini_header = {0x01, 0x00, 0xFFFF};
    uffs_Tag tag = {0};
    if(is_create)
        file_info->create_time = GET_CURRENT_TIME();
    if(type == UFFS_TYPE_DIR){
        file_info->attr = FILE_ATTR_DIR;
        tag.s.type = UFFS_TYPE_DIR;
        tag.s.serial = node->u.dir.serial;
        tag.s.parent = node->u.dir.parent;
    } else {
        file_info->attr = FILE_ATTR_WRITE;
        tag.s.type = UFFS_TYPE_FILE;
        tag.s.serial = node->u.file.serial;
        tag.s.parent = node->u.file.parent;
    }
    file_info->access = GET_CURRENT_TIME();
    file_info->last_modify = GET_CURRENT_TIME();
    file_info->name_len = strlen(file_info->name);
    tag.s.dirty = 1;
    tag.s.valid = 0;
    tag.s.block_ts = 0;
    tag.s.data_len = 0;
    tag.s.page_id = 0;
    tag.s.tag_ecc = TAG_ECC_DEFAULT;
    tag.data_sum = 0;
    tag.seal_byte = 0;

    if (writePage(dev->fd, node->u.file.block, 0, &mini_header, (char*)file_info, &tag) < 0)
        return U_FAIL;

    return U_SUCC;
}
