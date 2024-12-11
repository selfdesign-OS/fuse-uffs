/**
 * \file uffs_tree.c
 * \brief seting up uffs tree data structure
 * \author Ricky Zheng, created 13th May, 2005
 */

#include "uffs_tree.h"

#include <string.h>

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

    TreeNode *root = (TreeNode *) malloc(sizeof(TreeNode));

    memset(root, 0, sizeof(TreeNode));

    // 루트 노드 DirhSt 초기화
    root->u.dir.block = 0;       // 루트 블록 (일반적으로 0)
    root->u.dir.parent = ROOT_SERIAL; // 루트는 부모가 없음
    root->u.dir.serial = ROOT_SERIAL; // 루트의 고유 시리얼 번호 (0으로 초기화)

    // 루트 노드 FileInfo 초기화
    root->info.create_time = GET_CURRENT_TIME(); // 현재 시간 함수 호출
    root->info.last_modify = GET_CURRENT_TIME(); // 생성 시점과 동일하게 초기화
    root->info.access = GET_CURRENT_TIME();      // 액세스 시간도 동일하게 초기화
    root->info.reserved = 0;                     // 예약 필드 초기화
    root->info.name_len = 1;                     // 루트 이름 길이 ("/"만 포함)
    snprintf(root->info.name, MAX_FILENAME_LENGTH, "/"); // 루트 이름 설정
    root->info.nlink = 2;    // 디렉토리의 기본 링크 수는 2 ("."과 "..")
    root->info.len = 0;      // 디렉토리이므로 길이는 0
    root->info.mode = 0755;

    // TreeNode 연결 초기화
    root->hash_prev = EMPTY_NODE;
    root->hash_next = EMPTY_NODE;

    // 해시값 계산 및 루트 노드 설정
    int hash = GET_DIR_HASH(root->u.dir.serial); // 시리얼 번호를 기반으로 해시 계산
    dev->tree.dir_entry[hash] = root;           // 해시 테이블에 루트 노드 등록

    // 성공적으로 초기화된 경우
    fprintf(stderr,"[uffs_BuildTree] finished\n");
    return U_SUCC;
}

// node찾아서 매개변수 node에 넣어주기
// return: U_SUCC 또는 U_FAIL
URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name) {
    fprintf(stdout, "[uffs_TreeFindNodeByName] called\n");

    char *token;
    const char delimiter[] = "/";

    token = strtok(name, delimiter);
    
    int hash = GET_DIR_HASH(ROOT_SERIAL);
    TreeNode *cur_node = dev->tree.dir_entry[hash];
    TreeNode *tmp_node;

    while (token != NULL) {
        printf("[uffs_TreeFindNodeByName] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
        // 없으면 파일에서 찾기
        if (tmp_node == NULL) {
            tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
        }
        if (tmp_node == NULL) {
            fprintf(stderr,"[uffs_TreeFindNodeByName] error 1\n");
            return U_FAIL;
        }
        cur_node = tmp_node;
        
        token = strtok(NULL, delimiter);
    }

    *node = cur_node;

    fprintf(stdout, "[uffs_TreeFindNodeByName] finished\n");
    return U_SUCC;
}

TreeNode * uffs_TreeFindFileNode(uffs_Device *dev, u16 serial) {
    return NULL;
}

TreeNode * uffs_TreeFindFileNodeWithParent(uffs_Device *dev, u16 parent) {
    return NULL;
}

TreeNode * uffs_TreeFindDirNode(uffs_Device *dev, u16 serial) {
    fprintf(stdout,"[uffs_TreeFindDirNode] called\n");
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
    fprintf(stdout,"[uffs_TreeFindDirNode] finished\n");
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeWithParent(uffs_Device *dev, u16 parent) {
	return NULL;
}

TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent) {
    fprintf(stdout,"[uffs_TreeFindFileNodeByName] called\n");
    int i;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < FILE_NODE_ENTRY_LEN; i++) {
		node = tree->file_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.file.parent == parent) {
				//read file name from flash, and compare...
				if (strcmp(node->info.name, name) == 0 && node->info.name_len == len) {
					//Got it!
					return node;
				}
			}
			node = node->hash_next;
		}
	}
    fprintf(stdout,"[uffs_TreeFindFileNodeByName] finished\n");
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 parent) {
    fprintf(stdout,"[uffs_TreeFindDirNodeByName] called\n");
    int i;
	TreeNode *node;
	struct uffs_TreeSt *tree = &(dev->tree);
	
	for (i = 0; i < DIR_NODE_ENTRY_LEN; i++) {
		node = tree->dir_entry[i];
		while (node != EMPTY_NODE) {
			if (node->u.dir.parent == parent) {
				//read file name from flash, and compare...
				if (strcmp(node->info.name, name) == 0 && node->info.name_len == len) {
					//Got it!
					return node;
				}
			}
			node = node->hash_next;
		}
	}
    fprintf(stdout,"[uffs_TreeFindDirNodeByName] finished\n");
    return NULL;
}   

TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial) {
    return NULL;
}

URET uffs_TreeFindDirNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] called\n");

    char *token;
    const char delimiter[] = "/";

    token = strtok(name, delimiter);
    
    int hash = GET_DIR_HASH(ROOT_SERIAL);
    TreeNode *cur_node = dev->tree.dir_entry[hash];
    TreeNode *tmp_node;

    while (token != NULL) {
        printf("[uffs_TreeFindDirNodeByNameWithoutParent] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
        if (tmp_node == NULL) {
            fprintf(stderr,"[uffs_TreeFindDirNodeByNameWithoutParent] error 1\n");
            return U_FAIL;
        }
        cur_node = tmp_node;
        
        token = strtok(NULL, delimiter);
    }

    *node = cur_node;

    fprintf(stdout, "[uffs_TreeFindDirNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

URET uffs_TreeFindFileNodeByNameWithoutParent(uffs_Device *dev, TreeNode **node, const char *name) {
    fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] called\n");

    char *token;
    const char delimiter[] = "/";

    token = strtok(name, delimiter);
    
    int hash = GET_DIR_HASH(ROOT_SERIAL);
    TreeNode *cur_node = dev->tree.dir_entry[hash];
    TreeNode *tmp_node;
    int isFile = 0;
    while (token != NULL) {
        printf("[uffs_TreeFindFileNodeByNameWithoutParent] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
        // 없으면 파일에서 찾기
        if (tmp_node == NULL) {
            tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
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

    fprintf(stdout, "[uffs_TreeFindFileNodeByNameWithoutParent] finished\n");
    return U_SUCC;
}

static void _InsertToEntry(uffs_Device *dev, uint64_t *entry,
						   int hash, TreeNode *node)
{
    fprintf(stdout,"[_InsertToEntry] called\n");
	node->hash_next = entry[hash];
	node->hash_prev = EMPTY_NODE;
    if ((TreeNode*)node->hash_next != EMPTY_NODE) {
        TreeNode* temp_node = (TreeNode*)node->hash_next;
        temp_node->hash_prev = (uint64_t) node;
    }
    entry[hash] = node;
    fprintf(stdout,"[_InsertToEntry] finished\n");
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
    // case UFFS_TYPE_DATA:
    //     uffs_InsertToDataEntry(dev, node);
    //     break;
    default:
        fprintf(stderr, "[uffs_InsertNodeToTree] node type error\n");
        break;
    }
    fprintf(stdout,"[uffs_InsertNodeToTree] finished\n");
}

URET uffs_TreeFindParentNodeByName(uffs_Device *dev, TreeNode **node, const char *name, int isNodeExist) {
    fprintf(stdout, "[uffs_TreeFindParentNodeByName] called\n");
    if(strcmp(name,"/") == 0){
        return U_FAIL;
    }
    char *token;
    const char delimiter[] = "/";

    token = strtok(name, delimiter);
    
    int hash = GET_DIR_HASH(ROOT_SERIAL);
    TreeNode *cur_node = dev->tree.dir_entry[hash];
    TreeNode *tmp_node;
    
    while (token != NULL) {
        printf("[uffs_TreeFindParentNodeByName] directory: %s\n", token);

        // 디렉터리 노드 찾기
        tmp_node = uffs_TreeFindDirNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
        // 없으면 파일에서 찾기
        if (tmp_node == NULL) {
            tmp_node = uffs_TreeFindFileNodeByName(dev, token, strlen(token), cur_node->u.dir.serial);
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
    fprintf(stdout, "[uffs_TreeFindParentNodeByName] finished\n");
    return U_SUCC;
}

// 경로에서 파일 이름 추출 및 길이 반환
int parsePath(const char *path, char *nameBuffer, int maxNameLength) {
    fprintf(stdout,"[parsePath] called\n");
    const char *lastSlash = strrchr(path, '/'); // 마지막 '/' 위치 찾기
    if (lastSlash == NULL) {
        // '/'가 없는 경우 전체 경로가 이름
        strncpy(nameBuffer, path, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0'; // Null-terminate
        return strlen(nameBuffer);
    } else {
        // '/' 이후 부분이 파일 이름
        const char *name = lastSlash + 1;
        strncpy(nameBuffer, name, maxNameLength - 1);
        nameBuffer[maxNameLength - 1] = '\0'; // Null-terminate
        return strlen(nameBuffer);
    }
    fprintf(stdout,"[parsePath] finished\n");
}

URET initNode(uffs_Device *dev, TreeNode *node, data_Block *block, const char *path, u8 type) {
    fprintf(stdout,"[initNode] called\n");
    URET result;
    TreeNode *parent_node;

    // 부모 노드 찾기
    result = uffs_TreeFindParentNodeByName(dev, &parent_node, path, 0);
    if (result == U_FAIL) {
        fprintf(stderr, "[initNode] no parent \n");
        return U_FAIL;
    }
    // 블록 부모 초기화
    block->tag.parent = parent_node->u.dir.serial;

    // 랜덤 시리얼 번호 생성 (16비트)
    srand(time(NULL)); // 시드 초기화
    uint16_t random_serial = (uint16_t)(rand() & 0xFFFF); // 16비트 값 생성
    // 노드 초기화
    if (type==UFFS_TYPE_DIR) {
        node->u.dir.block = block->tag.block_id;        // 블록 ID
        node->u.dir.parent = parent_node->u.dir.serial; // 부모 노드 시리얼
        node->u.dir.serial = random_serial;            // 고유 시리얼 번호
    } else{
        node->u.file.block = block->tag.block_id;        // 블록 ID
        node->u.file.parent = parent_node->u.dir.serial; // 부모 노드 시리얼
        node->u.file.serial = random_serial;            // 고유 시리얼 번호
    }

    
    // 정보 초기화
    node->info.create_time = GET_CURRENT_TIME();
    node->info.last_modify = GET_CURRENT_TIME();
    node->info.access = GET_CURRENT_TIME();
    node->info.reserved = 0;

    // 경로 파싱하여 이름 및 길이 설정
    node->info.name_len = parsePath(path, node->info.name, MAX_FILENAME_LENGTH);
    if(type == UFFS_TYPE_DIR) {
        node->info.nlink = 2;    // 디렉토리 기본 링크 수
        node->info.len = 0;      // 디렉토리이므로 길이는 0
    } else{
        node->info.nlink = 1;    // 디렉토리 기본 링크 수
        node->info.len = block->tag.data_len;      // 디렉토리이므로 길이는 0
    }
    node->info.mode = 0755;
    fprintf(stdout,"[initNode] finished\n");
    return U_SUCC;
}
