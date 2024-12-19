/**
 * \file uffs_tree.c
 * \brief seting up uffs tree data structure
 * \author Ricky Zheng, created 13th May, 2005
 */

#include "uffs_tree.h"

#include <string.h>

static void _InsertToEntry(uffs_Device *dev, uint64_t *entry,
						   int hash, TreeNode *node)
{
    // fprintf(stdout,"[_InsertToEntry] called\n");
	node->hash_next = entry[hash];
	node->hash_prev = EMPTY_NODE;
    if ((TreeNode*)node->hash_next != EMPTY_NODE) {
        TreeNode* temp_node = (TreeNode*)node->hash_next;
        temp_node->hash_prev = (uint64_t) node;
    }
    entry[hash] = node;
    // fprintf(stdout,"[_InsertToEntry] finished\n");
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
    // fprintf(stdout,"[uffs_InsertNodeToTree] called\n");
    switch (type) {
    case UFFS_TYPE_DIR:
        fprintf(stdout,"[uffs_InsertNodeToTree] dir node inserted.\n");
        uffs_InsertToDirEntry(dev, node);
        break;
    case UFFS_TYPE_FILE:
        fprintf(stdout,"[uffs_InsertNodeToTree] file node inserted\n");
        uffs_InsertToFileEntry(dev, node);
        break;
    case UFFS_TYPE_DATA:
        fprintf(stdout,"[uffs_InsertNodeToTree] data node inserted\n");
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

    // 블록 및 페이지 초기화
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
            node->u.file.len = tag.s.data_len;
            uffs_InsertToFileEntry(dev, node);
            fprintf(stdout, "[uffs_BuildTree] made file node - name: %s\n", ((uffs_FileInfo *)data)->name);
			break;
		case UFFS_TYPE_DATA:
			node->u.data.parent = tag.s.parent;
			node->u.data.serial = tag.s.serial;
			node->u.data.block = block;
            node->u.data.len=tag.s.data_len;
                
            uffs_InsertToDataEntry(dev, node);
            fprintf(stdout, "[uffs_BuildTree] made data node\n");
			break;
		default:
			fprintf(stderr, "[uffs_BuildTree] UNKNOW TYPE error\n");
			break;
		}
    }

    // 성공적으로 초기화된 경우
    fprintf(stderr,"[uffs_BuildTree] finished\n");
    return U_SUCC;
}

static URET getRootDir(uffs_Device *dev, TreeNode **cur_node) {
    fprintf(stdout, "[getRootDir] called\n");
    int hash = GET_DIR_HASH(ROOT_DIR_SERIAL);
    *cur_node = dev->tree.dir_entry[hash];
    while ((*cur_node)->u.dir.serial != ROOT_DIR_SERIAL) {
        if ((*cur_node)->hash_next == EMPTY_NODE) {
            fprintf(stderr, "[getRootDir] fail - can't find root node\n");
            return U_FAIL;
        }
        *cur_node = (*cur_node)->hash_next;
    }
    fprintf(stdout, "[getRootDir] finished - found root node\n");
    return U_SUCC;
}

// node찾아서 매개변수 node에 넣어주기
// return: U_SUCC 또는 U_FAIL
URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name, u8 *type, uffs_ObjectInfo* object_info) {
    // fprintf(stdout, "[uffs_TreeFindNodeByName] called\n");

    char *token;
    const char delimiter[] = "/";
    // copy name
    int name_len = strlen(name) > MAX_FILENAME_LENGTH ? MAX_FILENAME_LENGTH : strlen(name);
    char temp_name[MAX_FILENAME_LENGTH + 1];
    strncpy(temp_name, name, name_len);
    temp_name[name_len] = '\0';

    token = strtok(temp_name, delimiter);
    
    TreeNode *cur_node;
    TreeNode *tmp_node;
    int getRootResult;
    getRootResult = getRootDir(dev, &cur_node);
    if (getRootResult == U_FAIL) {
        return U_FAIL;
    }

    while (token != NULL) {
        // 디렉토리 검색 성공 시
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        if (tmp_node != NULL) {
            if (type != NULL) {
                *type = UFFS_TYPE_DIR;
            }
            cur_node = tmp_node;
            // 다음 토큰 처리
            token = strtok(NULL, "/");
            continue;
        }
        
        // 디렉토리에서 못 찾았을 경우 파일 검색
        tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        if (tmp_node != NULL) {
            if (type != NULL) {
                *type = UFFS_TYPE_FILE;
            }
            cur_node = tmp_node;
            // 다음 토큰 처리
            token = strtok(NULL, "/");
            continue;
        }

        // 둘 다 못 찾은 경우
        fprintf(stderr, "[uffs_TreeFindNodeByName] error: Node not found for %s\n", token);
        return U_FAIL;

    }

    *node = cur_node;
    if (object_info != NULL) {
        uffs_FileInfo temp_info;
        u32 file_len = 0; // 태그에서 가져올 파일 길이

        if (getFileInfoBySerial(dev->fd, (*node)->u.file.serial, &temp_info, &file_len) == U_SUCC) {
            memcpy(&object_info->info, &temp_info, sizeof(uffs_FileInfo));
            object_info->len = file_len;
            object_info->serial = (*node)->u.file.serial;
        } else {
            // 파일 정보 가져오기 실패시 처리
            fprintf(stderr, "[uffs_TreeFindNodeByName] can't get file info by serial\n");
        }
    }


    if (type != NULL) {
        // fprintf(stdout, "[uffs_TreeFindNodeByName] finished - type: %d\n", *type);
    } else {
        // fprintf(stdout, "[uffs_TreeFindNodeByName] finished - type is NULL\n");
    }    
    return U_SUCC;
}

TreeNode * uffs_TreeFindFileNode(uffs_Device *dev, u16 serial) {
    return NULL;
}

TreeNode * uffs_TreeFindFileNodeWithParent(uffs_Device *dev, u16 parent) {
    return NULL;
}

TreeNode * uffs_TreeFindDirNode(uffs_Device *dev, u16 serial) {
    // fprintf(stdout,"[uffs_TreeFindDirNode] called\n");
    int i;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		node = tree->dir_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.dir.serial == serial) {
				return node;
			}
			node = node->hash_next;
		}
	}
    // fprintf(stdout,"[uffs_TreeFindDirNode] finished\n");
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeWithParent(uffs_Device *dev, u16 parent) {
	return NULL;
}

static UBOOL uffs_TreeCompareFileName(uffs_Device* dev, char* name, u16 parent, u8 expected_type, uffs_ObjectInfo* object_info) {
    uffs_ObjectInfo temp_objectInfo = {0};

    for (int i =1; i<TOTAL_BLOCKS_DEFAULT; i++) {
        uffs_Tag tag={0};
        if (readPage(dev->fd,i,0,NULL,(char*)&temp_objectInfo.info,&tag) == U_SUCC) {
            if (tag.s.parent == parent && tag.s.type == expected_type &&
                strcmp(temp_objectInfo.info.name, name) == 0) {
                temp_objectInfo.len = 0; 
                temp_objectInfo.serial = tag.s.serial;
                if(object_info != NULL) {
                    *object_info = temp_objectInfo;
                }
                // fprintf(stdout,"[uffs_TreeCompareFileName] finished\n");
                return U_TRUE;
            }
        }
    }
    return U_FAIL;
}


TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info) {
    // fprintf(stdout,"[uffs_TreeFindFileNodeByName] called\n");
    int i;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < FILE_NODE_ENTRY_LEN; i++) {
		node = tree->file_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.file.parent == parent && uffs_TreeCompareFileName(dev, name, parent,UFFS_TYPE_FILE,object_info) == U_TRUE) {
                // fprintf(stdout,"[uffs_TreeFindFileNodeByName] find node success\n");
                return node;
			}
			node = node->hash_next;
		}
	}
    // fprintf(stdout,"[uffs_TreeFindFileNodeByName] finished: can't find file node by name.\n");
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent, uffs_ObjectInfo* object_info) {
    // fprintf(stdout,"[uffs_TreeFindDirNodeByName] called\n");
    int i;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		node = tree->dir_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.dir.parent == parent && uffs_TreeCompareFileName(dev, name, parent, UFFS_TYPE_DIR,object_info) == U_TRUE) {
                // fprintf(stdout,"[uffs_TreeFindDirNodeByName] finished\n");
                return node;
			}
			node = node->hash_next;
		}
	}
    // fprintf(stdout,"[uffs_TreeFindDirNodeByName] finished: can't find dir node by name\n");
    return NULL;
}   


TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial) {
    return NULL;
}

TreeNode * uffs_TreeFindDataNodeByParent(uffs_Device *dev, u16 parent) {
    // fprintf(stdout,"[uffs_TreeFindDataNodeByParent] started\n");
    TreeNode *node;
    struct uffs_TreeSt *tree = &(dev->tree);
    for (int i = 0; i < DATA_NODE_ENTRY_LEN; i++) {
		node = tree->data_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.data.parent == parent) {
                // fprintf(stdout,"[uffs_TreeFindDataNodeByParent] finished\n");
                return node;
			}
			node = node->hash_next;
		}
	}
    // fprintf(stdout,"[uffs_TreeFindDataNodeByParent] failed\n");
    return NULL;
}

URET uffs_TreeFindDirNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    // fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] called\n");

    char *token;
    const char delimiter[] = "/";
    // copy name
    int name_len = strlen(name) > MAX_FILENAME_LENGTH ? MAX_FILENAME_LENGTH : strlen(name);
    char temp_name[MAX_FILENAME_LENGTH + 1];
    strncpy(temp_name, name, name_len);
    temp_name[name_len] = '\0';

    token = strtok(temp_name, delimiter);
    
    TreeNode *cur_node;
    TreeNode *tmp_node;
    int getRootResult;
    getRootResult = getRootDir(dev, &cur_node);
    if (getRootResult == U_FAIL) {
        return U_FAIL;
    }

    while (token != NULL) {
        // printf("[uffs_TreeFindDirNodeByNameWithoutParent] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        if (tmp_node == NULL) {
            fprintf(stderr,"[uffs_TreeFindDirNodeByNameWithoutParent] error 1\n");
            return U_FAIL;
        }
        cur_node = tmp_node;
        
        token = strtok(NULL, delimiter);
    }

    *node = cur_node;

    // fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

URET uffs_TreeFindFileNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    // fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] called\n");

    char *token;
    const char delimiter[] = "/";
    // copy name
    int name_len = strlen(name) > MAX_FILENAME_LENGTH ? MAX_FILENAME_LENGTH : strlen(name);
    char temp_name[MAX_FILENAME_LENGTH + 1];
    strncpy(temp_name, name, name_len);
    temp_name[name_len] = '\0';

    token = strtok(temp_name, delimiter);
    
    TreeNode *cur_node;
    TreeNode *tmp_node;
    int getRootResult;
    getRootResult = getRootDir(dev, &cur_node);
    if (getRootResult == U_FAIL) {
        return U_FAIL;
    }
    int isFile = 0;
    while (token != NULL) {
        // printf("[uffs_TreeFindFileNodeByNameWithoutParent] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        // 없으면 파일에서 찾기
        if (tmp_node == NULL) {
            tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
            isFile = 1;
        }
        if (tmp_node == NULL) {
            fprintf(stderr,"[uffs_TreeFindFileNodeByNameWithoutParent] error 1\n");
            return U_FAIL;
        }
        cur_node = tmp_node;
        token = strtok(NULL, delimiter);
    }
    if (isFile != 1) {
        fprintf(stderr,"[uffs_TreeFindFileNodeByNameWithoutParent] error 2\n");
        return U_FAIL;
    }

    *node = cur_node;

    // fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

URET uffs_TreeFindParentNodeByName(uffs_Device *dev, TreeNode **node, const char *name, int isNodeExist) {
    // fprintf(stdout, "[uffs_TreeFindParentNodeByName] called\n");
    if(strcmp(name,"/") == 0){
        return U_FAIL;
    }
    char *token;
    const char delimiter[] = "/";
    // copy name
    int name_len = strlen(name) > MAX_FILENAME_LENGTH ? MAX_FILENAME_LENGTH : strlen(name);
    char temp_name[MAX_FILENAME_LENGTH + 1];
    strncpy(temp_name, name, name_len);
    temp_name[name_len] = '\0';

    token = strtok(temp_name, delimiter);
    
    TreeNode *cur_node;
    TreeNode *tmp_node;
    int getRootResult;
    getRootResult = getRootDir(dev, &cur_node);
    if (getRootResult == U_FAIL) {
        return U_FAIL;
    }
    
    while (token != NULL) {
        // printf("[uffs_TreeFindParentNodeByName] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        // 없으면 파일에서 찾기
        if (tmp_node == NULL) {
            tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial, NULL);
        }
        token = strtok(NULL, delimiter);
        if (tmp_node == NULL) {
            if(!isNodeExist && token == NULL){
                break;
            }else{
                fprintf(stderr,"[uffs_TreeFindParentNodeByName] error 1\n");
                return U_FAIL;
            }
        }
        cur_node = tmp_node;
    }
    TreeNode *parent_node;
    if (isNodeExist) {
        parent_node = uffs_TreeFindDirNode(dev,cur_node->u.file.parent);
    }
    else {
        parent_node = uffs_TreeFindDirNode(dev,cur_node->u.file.serial);
    }
    if (parent_node == NULL) {
        fprintf(stderr,"[uffs_TreeFindParentNodeByName] error 2\n");
        return U_FAIL;
    }
    *node = parent_node;
    // fprintf(stdout, "[uffs_TreeFindParentNodeByName] finished\n");
    return U_SUCC;
}

// 경로에서 파일 이름 추출 및 길이 반환
int parsePath(const char *path, char *nameBuffer, int maxNameLength) {
    // fprintf(stdout,"[parsePath] called\n");
    const char *lastSlash = strrchr(path, '/'); // 마지막 '/' 위치 찾기
    if (lastSlash == NULL) {
        // '/'가 없는 경우 전체 경로가 이름
        strncpy(nameBuffer, path, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0'; // Null-terminate
        // fprintf(stdout,"[parsePath] finished 1 - name: %s\n", nameBuffer);
        return strlen(nameBuffer);
    } else {
        // '/' 이후 부분이 파일 이름
        const char *name = lastSlash + 1;
        strncpy(nameBuffer, name, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0'; // Null-terminate
        // fprintf(stdout,"[parsePath] finished 2 - name: %s\n", nameBuffer);
        return strlen(nameBuffer);
    }
}

// 노드 초기화
URET initNode(uffs_Device *dev, TreeNode *node, int block_id,u8 type, u16 parent_serial, u16 serial) {

    if(block_id == -1){
        return U_FAIL;
    }

    memset(node,0,sizeof(TreeNode));

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

URET updateFileInfoPage(uffs_Device  *dev, TreeNode *node, uffs_FileInfo *file_info, int is_create, u8 type) {
    uffs_MiniHeader mini_header={0x01,0x00,0xFFFF};
    uffs_Tag tag={0};
    if(is_create){
        file_info->create_time = GET_CURRENT_TIME();
    }

    // 파일인지 디렉토리인지에 따라 태그와 file_info 설정
    if(type == UFFS_TYPE_DIR){
        file_info->attr = FILE_ATTR_DIR;
        tag.s.type = UFFS_TYPE_DIR;
        tag.s.serial = node->u.dir.serial;
        tag.s.parent = node->u.dir.parent;
        file_info->name_len = strlen(file_info->name);
        // 디렉토리는 길이 0
        tag.s.data_len = 0; 
    } else {
        file_info->attr = FILE_ATTR_WRITE; // 일반 파일
        tag.s.type = UFFS_TYPE_FILE;
        tag.s.serial = node->u.file.serial;
        tag.s.parent = node->u.file.parent;
        file_info->name_len = strlen(file_info->name);
        
        // 파일 길이를 태그에 반영
        tag.s.data_len = node->u.file.len; 
    }

    file_info->access = GET_CURRENT_TIME();
    file_info->last_modify = GET_CURRENT_TIME();
    // 파일 길이 정보는 file_info 구조체에는 없음.
    // 다만 diskFormat에 file_info 확장을 원치 않으면 file_info에는 길이를 넣지 않고 tag만으로 길이 관리.

    // tag 기본값들
    tag.s.dirty = 1;
    tag.s.valid = 0;
    tag.s.block_ts = 0;
    tag.s.page_id = 0;
    tag.s.tag_ecc = TAG_ECC_DEFAULT;
    tag.data_sum = 0;
    tag.seal_byte = 0;

    if (writePage(dev->fd, node->u.file.block,0,&mini_header,(char*)file_info,&tag)<0) {
        return U_FAIL;
    }

    return U_SUCC;
}
