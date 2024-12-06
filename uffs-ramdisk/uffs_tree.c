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
    root->u.dir.parent = EMPTY_NODE; // 루트는 부모가 없음
    root->u.dir.serial = 0;      // 루트의 고유 시리얼 번호 (0으로 초기화)

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
    return U_SUCC;
}

// node찾아서 매개변수 node에 넣어주기
// return: U_SUCC 또는 U_FAIL
URET uffs_TreeFindNodeByName(uffs_Device *dev, TreeNode **node, const char *name, int isDir) {
    fprintf(stdout, "[uffs_TreeFindNodeByName] called\n");

    char *token;
    char *last_token;
    const char delimiter[] = "/";

    token = strtok(name, delimiter);
    
    int hash = GET_DIR_HASH(0);
    TreeNode *cur_node = dev->tree.dir_entry[hash];

    while (token != NULL) {
        printf("[uffs_TreeFindNodeByName] directory: %s\n", token);

        // TODO: should implement to go down tree

        strcpy(last_token, token);
        token = strtok(NULL, delimiter);
    }

    // if (isDir == DIR) {
        
    // }
    // else (isDir == UDIR) {

    // }

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
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeWithParent(uffs_Device *dev, u16 parent) {
    return NULL;
}

TreeNode * uffs_TreeFindFileNodeByName(uffs_Device *dev, const char *name, u32 len, u16 sum, u16 parent) {
    return NULL;
}

TreeNode * uffs_TreeFindDirNodeByName(uffs_Device *dev, const char *name, u32 len, u16 sum, u16 parent) {
    return NULL;
}   

TreeNode * uffs_TreeFindDataNode(uffs_Device *dev, u16 parent, u16 serial) {
    return NULL;
}

void uffs_InsertNodeToTree(uffs_Device *dev, u8 type, TreeNode *node) {
    return NULL;
}