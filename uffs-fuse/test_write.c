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
/* 테스트 3: 소용량 파일은 파일 블록 page 1에 저장, data_node 없음               */
/* -------------------------------------------------------------------------- */
static void test_write_data_in_file_block(void)
{
    SECTION("소용량 파일 — 파일 블록 page 1에 저장, data_node 없음");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/test.txt", 0644, &fi);

    const char *msg = "test data";
    size_t msg_len = strlen(msg);
    uffs_write("/test.txt", msg, msg_len, 0, &fi);

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/test.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음"); close(fd); return; }

    /* 소용량 파일은 data_node 없음 */
    TreeNode *data_node = uffs_TreeFindDataNodeByParent(&dev, file_node->u.file.serial);
    CHECK(data_node == NULL, "소용량 파일은 data_node 없음");

    /* 파일 블록 page 1에 데이터가 있어야 함 */
    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    uffs_MiniHeader mini = {0};
    readPage(fd, file_node->u.file.block, 1, &mini, read_buf, NULL);
    CHECK(mini.status != 0xFF, "파일 블록 page 1이 사용 중");
    CHECK(memcmp(read_buf, msg, msg_len) == 0, "파일 블록 page 1 데이터 일치");

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

    uffs_MiniHeader mini = {0};
    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    uffs_Tag tag = {0};

    // 파일 블록 page 1에서 데이터 읽기
    URET r = readPage(fd, file_node->u.file.block, 1, &mini, read_buf, &tag);
    CHECK(r == U_SUCC, "readPage 성공");
    CHECK(memcmp(read_buf, msg, msg_len) == 0, "파일 블록 page 1 데이터 = 쓴 데이터");
    CHECK((size_t)tag.s.data_len == msg_len,   "tag.data_len = 쓴 크기");
    CHECK(tag.s.type == UFFS_TYPE_FILE,        "tag.type = UFFS_TYPE_FILE");

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

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, file_node->u.file.block, 1, NULL, read_buf, NULL);

    CHECK(memcmp(read_buf, second, len) == 0,
          "두 번째 쓰기 후 파일 블록 page 1 = 'SECOND_DAT'");

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

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, file_node->u.file.block, 1, NULL, read_buf, NULL);
    CHECK(memcmp(read_buf, buf, PAGE_DATA_SIZE_DEFAULT) == 0,
          "512바이트 파일 블록 page 1 데이터 일치");

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
/* 테스트 8: offset > 0 쓰기 — 앞 바이트 보존 확인                              */
/* -------------------------------------------------------------------------- */
static void test_write_with_offset(void)
{
    SECTION("offset > 0 쓰기 — 앞 바이트 보존 (partial overwrite)");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/offset.txt", 0644, &fi);

    /* 먼저 "AAAAAAAAAA" (10바이트) 를 offset 0에 쓴다 */
    const char *first = "AAAAAAAAAA";
    uffs_write("/offset.txt", first, 10, 0, &fi);

    /* 이후 "BBB" (3바이트) 를 offset 4에 덮어쓴다 → "AAAABBBAA" 가 되어야 함 */
    const char *patch = "BBB";
    int ret = uffs_write("/offset.txt", patch, 3, 4, &fi);
    CHECK(ret == 3, "offset 4 쓰기 반환 = 3");

    /* 파일 블록 page 1 을 직접 읽어 검증 */
    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/offset.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음"); close(fd); return; }

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, file_node->u.file.block, 1, NULL, read_buf, NULL);

    CHECK(memcmp(read_buf,     "AAAA", 4) == 0, "offset 0~3: 'AAAA' 보존");
    CHECK(memcmp(read_buf + 4, "BBB",  3) == 0, "offset 4~6: 'BBB' 덮어쓰기");
    CHECK(memcmp(read_buf + 7, "AAA",  3) == 0, "offset 7~9: 'AAA' 보존");

    close(fd);
}

/* -------------------------------------------------------------------------- */
/* 테스트 9: append — offset = file_len 으로 이어쓰기                           */
/* -------------------------------------------------------------------------- */
static void test_write_append(void)
{
    SECTION("append — 기존 데이터 뒤에 이어쓰기");

    int fd = create_disk();
    if (fd < 0) { FAIL("디스크 이미지 생성 실패"); return; }
    reset_dev(fd);

    struct fuse_file_info fi = {0};
    uffs_create("/append.txt", 0644, &fi);

    uffs_write("/append.txt", "HELLO", 5, 0, &fi);

    TreeNode *file_node = NULL;
    uffs_TreeFindFileNodeByNameWithoutParent(&dev, &file_node, "/append.txt");
    if (file_node == NULL) { FAIL("파일 노드 없음"); close(fd); return; }

    /* file_len == 5 이므로 offset 5에 이어쓴다 */
    int ret = uffs_write("/append.txt", "WORLD", 5, (off_t)file_node->u.file.len, &fi);
    CHECK(ret == 5, "append 쓰기 반환 = 5");
    CHECK(file_node->u.file.len == 10, "file_len == 10");

    char read_buf[PAGE_DATA_SIZE_DEFAULT] = {0};
    readPage(fd, file_node->u.file.block, 1, NULL, read_buf, NULL);
    CHECK(memcmp(read_buf, "HELLOWORLD", 10) == 0, "page 1 내용 = 'HELLOWORLD'");

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
    test_write_data_in_file_block();
    test_write_disk_content();
    test_write_twice_overwrites();
    test_write_full_page();
    test_write_in_subdir();
    test_write_with_offset();
    test_write_append();

    printf("\n================================================\n");
    printf("  결과: %d PASS  /  %d FAIL\n", g_pass, g_fail);
    printf("================================================\n");

    return g_fail == 0 ? 0 : 1;
}
