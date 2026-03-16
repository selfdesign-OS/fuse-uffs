/*
 * test_write.c
 * uffs_write 단위 테스트 — ASan 빌드 전용
 *
 * 빌드: make test_write
 * 실행: ./test_write
 */

#define UNIT_TEST
#include "fuse_compat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "uffs_types.h"
#include "uffs_disk.h"
#include "uffs_tree.h"
#include "uffs_device.h"

/* mkuffs.c 에서 정의된 전역 장치 및 콜백 함수 선언 */
extern uffs_Device dev;

int uffs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int uffs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);
int uffs_mkdir(const char *path, mode_t mode);

/* -------------------------------------------------------------------------- */
/* 테스트 헬퍼                                                                  */
/* -------------------------------------------------------------------------- */
static int g_pass = 0;
static int g_fail = 0;

#define PASS(msg) \
    do { printf("  \033[32m[PASS]\033[0m %s\n", (msg)); g_pass++; } while (0)

#define FAIL(msg) \
    do { printf("  \033[31m[FAIL]\033[0m %s  (line %d)\n", (msg), __LINE__); g_fail++; } while (0)

#define CHECK(cond, msg) \
    do { if (cond) PASS(msg); else FAIL(msg); } while (0)

#define SECTION(name) \
    printf("\n\033[1m[TEST]\033[0m %s\n", (name))

/* 포맷된 임시 디스크 이미지를 생성하고 fd 반환 (실패 시 -1) */
static int create_disk(void)
{
    char path[] = "/tmp/test_uffs_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return -1;
    unlink(path); /* close 시 자동 삭제 */

    size_t disk_size = (size_t)TOTAL_BLOCKS_DEFAULT
                     * PAGES_PER_BLOCK_DEFAULT
                     * PAGE_SIZE_DEFAULT;
    if (ftruncate(fd, (off_t)disk_size) < 0) { close(fd); return -1; }
    if (diskFormat(fd) != U_SUCC)            { close(fd); return -1; }
    return fd;
}

/* 전역 dev를 초기화하고 트리를 빌드 */
static void reset_dev(int fd)
{
    memset(&dev, 0, sizeof(dev));
    dev.fd = fd;
    uffs_TreeInit(&dev);
    uffs_BuildTree(&dev);
}

/* -------------------------------------------------------------------------- */
/* 테스트 1: uffs_write 반환값 = 쓴 바이트 수                                    */
/* -------------------------------------------------------------------------- */
static void test_write_returns_size(void)
{
    SECTION("uffs_write - 반환값 = 쓴 바이트 수");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    int ret = uffs_create("/hello.txt", 0644, &fi);
    CHECK(ret == 0, "uffs_create /hello.txt 반환 0");

    const char *msg = "Hello UFFS";
    ret = uffs_write("/hello.txt", msg, strlen(msg), 0, &fi);
    CHECK(ret == (int)strlen(msg), "uffs_write 반환값 = strlen(msg)");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 2: uffs_write 후 file_node.u.file.len 갱신 확인                       */
/* -------------------------------------------------------------------------- */
static void test_write_updates_file_len(void)
{
    SECTION("uffs_write 후 file_node.u.file.len 갱신");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/data.bin", 0644, &fi);

    const char *buf = "0123456789"; /* 10 bytes */
    uffs_write("/data.bin", buf, 10, 0, &fi);

    TreeNode *node = NULL;
    URET r = uffs_TreeFindFileNodeByNameWithoutParent(&dev, &node, "/data.bin");
    CHECK(r == U_SUCC, "파일 노드 트리에서 찾기 성공");
    if (r == U_SUCC)
        CHECK(node->u.file.len == 10, "file_node.len == 10");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 3: uffs_write 후 data_node 트리 삽입 확인                              */
/* -------------------------------------------------------------------------- */
static void test_write_inserts_data_node(void)
{
    SECTION("uffs_write 후 data_node 트리 삽입 확인");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/test.txt", 0644, &fi);

    const char *buf = "test data";
    uffs_write("/test.txt", buf, strlen(buf), 0, &fi);

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/test.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음 — data_node 검증 불가"); close(fd); return; }

    TreeNode *data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    CHECK(data_node != NULL, "data_node 트리에 삽입됨");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 4: uffs_write 후 readPage로 실제 디스크 데이터 검증                    */
/* -------------------------------------------------------------------------- */
static void test_write_disk_content(void)
{
    SECTION("uffs_write 후 readPage로 디스크 데이터 직접 검증");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/disk.txt", 0644, &fi);

    const char *msg = "DISK_VERIFY";
    size_t msg_len = strlen(msg);
    uffs_write("/disk.txt", msg, msg_len, 0, &fi);

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/disk.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음"); close(fd); return; }

    TreeNode *data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    if (data_node == NULL) { FAIL("data_node 없음 — readPage 검증 불가"); close(fd); return; }

    int block = data_node->u.data.block;

    uffs_MiniHeader mini = {0};
    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    uffs_Tag tag = {0};

    URET r = readPage(fd, block, 0, &mini, read_buf, &tag);
    CHECK(r == U_SUCC, "readPage 성공");
    CHECK(memcmp(read_buf, msg, msg_len) == 0, "page0 데이터 = 쓴 데이터");
    CHECK((size_t)tag.s.data_len == msg_len,   "tag.data_len = 쓴 크기");
    CHECK(tag.s.type == UFFS_TYPE_DATA,        "tag.type = UFFS_TYPE_DATA");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 5: 같은 파일 두 번 쓰기 — 두 번째 데이터가 디스크에 반영되는지 확인      */
/* (writePage 중복 방지 버그 노출 시나리오)                                       */
/* -------------------------------------------------------------------------- */
static void test_write_twice_overwrites(void)
{
    SECTION("같은 파일 두 번 쓰기 — 두 번째 데이터 디스크 반영 확인");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/twice.txt", 0644, &fi);

    const char *first  = "FIRST_DATA";
    const char *second = "SECOND_DAT"; /* 동일 길이 10 bytes */
    size_t len = strlen(first);

    int r1 = uffs_write("/twice.txt", first,  len, 0, &fi);
    int r2 = uffs_write("/twice.txt", second, len, 0, &fi);

    CHECK(r1 == (int)len, "첫 번째 uffs_write 반환 = 쓴 바이트 수");
    CHECK(r2 == (int)len, "두 번째 uffs_write 반환 = 쓴 바이트 수");

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/twice.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음 — 두 번째 쓰기 검증 불가"); close(fd); return; }

    TreeNode *data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    if (data_node == NULL) { FAIL("data_node 없음 — 두 번째 쓰기 검증 불가"); close(fd); return; }

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, data_node->u.data.block, 0, NULL, read_buf, NULL);

    CHECK(memcmp(read_buf, second, len) == 0,
          "두 번째 쓰기 후 page0 = 'SECOND_DAT'");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 6: 512바이트(1페이지 전체) 쓰기                                        */
/* -------------------------------------------------------------------------- */
static void test_write_full_page(void)
{
    SECTION("512바이트(1페이지 전체) 쓰기");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/page.bin", 0644, &fi);

    char buf[PAGE_DATA_SIZE_DEFAULT];
    memset(buf, 0xAB, PAGE_DATA_SIZE_DEFAULT);

    int ret = uffs_write("/page.bin", buf, PAGE_DATA_SIZE_DEFAULT, 0, &fi);
    CHECK(ret == PAGE_DATA_SIZE_DEFAULT, "512바이트 쓰기 반환 = 512");

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/page.bin");
    if (file_node == NULL) { FAIL("파일 노드 없음"); close(fd); return; }

    TreeNode *data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    if (data_node == NULL) { FAIL("data_node 없음 — 페이지 검증 불가"); close(fd); return; }

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, data_node->u.data.block, 0, NULL, read_buf, NULL);
    CHECK(memcmp(read_buf, buf, PAGE_DATA_SIZE_DEFAULT) == 0,
          "512바이트 디스크 데이터 일치");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 7: 서브 디렉터리 아래 파일 쓰기                                         */
/* -------------------------------------------------------------------------- */
static void test_write_in_subdir(void)
{
    SECTION("서브 디렉터리 아래 파일 쓰기");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    int ret = uffs_mkdir("/logs", 0755);
    CHECK(ret == 0, "uffs_mkdir /logs 반환 0");

    ret = uffs_create("/logs/app.log", 0644, &fi);
    CHECK(ret == 0, "uffs_create /logs/app.log 반환 0");

    const char *msg = "log entry";
    ret = uffs_write("/logs/app.log", msg, strlen(msg), 0, &fi);
    CHECK(ret == (int)strlen(msg), "서브 디렉터리 파일 쓰기 반환 = 쓴 바이트 수");

    TreeNode *file_node = NULL;
    URET r = uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/logs/app.log");
    CHECK(r == U_SUCC, "서브 디렉터리 파일 노드 트리에 존재");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* main                                                                        */
/* -------------------------------------------------------------------------- */
int main(void)
{
    printf("================================================\n");
    printf("  uffs_write 단위 테스트 (ASan 빌드)\n");
    printf("================================================\n");

    test_write_returns_size();
    test_write_updates_file_len();
    test_write_inserts_data_node();
    test_write_disk_content();
    test_write_twice_overwrites();
    test_write_full_page();
    test_write_in_subdir();

    printf("\n================================================\n");
    printf("  결과: %d PASS  /  %d FAIL\n", g_pass, g_fail);
    printf("================================================\n");

    return g_fail == 0 ? 0 : 1;
}
