#!/bin/bash
# =============================================================================
# test_write.sh
# FUSE2 마운트 기반 통합 테스트 - 파일 쓰기 시나리오
#
# 실행: ./test_write.sh
# =============================================================================

MKUFFS=./mkuffs
DISK_IMG=/tmp/uffs_integ_disk.img
MOUNT_DIR=/tmp/uffs_integ_mnt
# 128 blocks * 32 pages * 528 bytes/page
DISK_SIZE=$(( 128 * 32 * 528 ))

g_pass=0
g_fail=0

# ------------------------------------------------------------------------------
# 헬퍼
# ------------------------------------------------------------------------------
PASS="\033[32m[PASS]\033[0m"
FAIL="\033[31m[FAIL]\033[0m"
INFO="\033[34m[INFO]\033[0m"
SKIP="\033[33m[SKIP]\033[0m"

check() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$actual" = "$expected" ]; then
        printf "  $PASS %s\n" "$desc"
        (( g_pass++ ))
    else
        printf "  $FAIL %s  (기대='%s'  실제='%s')\n" "$desc" "$expected" "$actual"
        (( g_fail++ ))
    fi
}

check_nonzero() {
    local desc="$1" val="$2"
    if [ -n "$val" ] && [ "$val" -ne 0 ] 2>/dev/null; then
        printf "  $PASS %s\n" "$desc"
        (( g_pass++ ))
    else
        printf "  $FAIL %s  (실제='%s')\n" "$desc" "$val"
        (( g_fail++ ))
    fi
}

skip() {
    printf "  $SKIP %s\n" "$1"
}

section() {
    printf "\n\033[1m[TEST]\033[0m %s\n" "$1"
}

# ------------------------------------------------------------------------------
# 디스크 / 마운트 헬퍼
# ------------------------------------------------------------------------------
MKUFFS_PID=""

setup() {
    rm -f "$DISK_IMG"
    dd if=/dev/zero of="$DISK_IMG" bs="$DISK_SIZE" count=1 2>/dev/null
    mkdir -p "$MOUNT_DIR"
}

do_mount() {
    # argv 레이아웃: mkuffs <mountpoint> <fuse-opt> <diskimg>
    # fuse_main(3, argv) → fuse 는 argv[0..2] 만 봄
    "$MKUFFS" "$MOUNT_DIR" -f "$DISK_IMG" >/tmp/mkuffs_log.txt 2>&1 &
    MKUFFS_PID=$!

    # 마운트 완료 대기 (최대 3초)
    for i in $(seq 1 30); do
        mountpoint -q "$MOUNT_DIR" 2>/dev/null && return 0
        sleep 0.1
    done
    return 1
}

do_unmount() {
    fusermount -u "$MOUNT_DIR" 2>/dev/null
    [ -n "$MKUFFS_PID" ] && wait "$MKUFFS_PID" 2>/dev/null
    MKUFFS_PID=""
}

teardown() {
    do_unmount
    rm -f "$DISK_IMG"
    rmdir "$MOUNT_DIR" 2>/dev/null
}

# 데몬이 살아있는지 확인
is_mounted() {
    mountpoint -q "$MOUNT_DIR" 2>/dev/null
}

# ------------------------------------------------------------------------------
# 테스트 1: 마운트/언마운트
# ------------------------------------------------------------------------------
section "마운트 및 언마운트"
setup
do_mount
ret=$?
check "mkuffs 마운트 성공" "0" "$ret"
if [ $ret -eq 0 ]; then
    is_mounted
    check "mountpoint 확인" "0" "$?"
fi
do_unmount
is_mounted
check "언마운트 후 마운트포인트 해제 확인" "1" "$?"
teardown

# ------------------------------------------------------------------------------
# 테스트 2: 루트 디렉터리 ls
# ------------------------------------------------------------------------------
section "루트 readdir"
setup
do_mount
if is_mounted; then
    entries=$(ls "$MOUNT_DIR" 2>/dev/null | wc -l)
    check "빈 루트 ls 성공 (엔트리 0개)" "0" "$entries"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 3: 파일 생성 (create)
# ------------------------------------------------------------------------------
section "파일 생성 (touch)"
setup
do_mount
if is_mounted; then
    touch "$MOUNT_DIR/hello.txt" 2>/dev/null
    ret=$?
    check "touch /hello.txt 반환 0" "0" "$ret"

    ls "$MOUNT_DIR/hello.txt" >/dev/null 2>&1
    check "생성 후 ls 로 파일 확인" "0" "$?"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 4: 디렉터리 생성 (mkdir)
# ------------------------------------------------------------------------------
section "디렉터리 생성 (mkdir)"
setup
do_mount
if is_mounted; then
    mkdir "$MOUNT_DIR/subdir" 2>/dev/null
    check "mkdir /subdir 반환 0" "0" "$?"

    ls -d "$MOUNT_DIR/subdir" >/dev/null 2>&1
    check "생성 후 ls -d 로 디렉터리 확인" "0" "$?"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 5: 파일 쓰기 (write) - echo 를 통한 단순 쓰기
# ------------------------------------------------------------------------------
section "파일 쓰기 - echo 단순 쓰기"
setup
do_mount
if is_mounted; then
    echo "Hello UFFS" > "$MOUNT_DIR/msg.txt" 2>/dev/null
    ret=$?
    check "echo > /msg.txt 반환 0" "0" "$ret"

    # getattr 으로 파일 크기 확인 (uffs_read 버그로 cat 불가)
    fsize=$(stat -c "%s" "$MOUNT_DIR/msg.txt" 2>/dev/null)
    check_nonzero "write 후 getattr 파일 크기 > 0" "$fsize"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 6: 파일 쓰기 - dd 를 이용한 정확한 크기 쓰기
# ------------------------------------------------------------------------------
section "파일 쓰기 - dd 정확한 크기"
setup
do_mount
if is_mounted; then
    # 정확히 100 바이트 쓰기
    dd if=/dev/zero of="$MOUNT_DIR/dd_test.bin" bs=100 count=1 2>/dev/null
    check "dd 100바이트 쓰기 반환 0" "0" "$?"

    ls "$MOUNT_DIR/dd_test.bin" >/dev/null 2>&1
    check "dd 쓰기 후 파일 존재 확인" "0" "$?"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 7: 여러 파일 순차 쓰기
# ------------------------------------------------------------------------------
section "여러 파일 순차 쓰기"
setup
do_mount
if is_mounted; then
    for i in 1 2 3; do
        echo "file $i content" > "$MOUNT_DIR/file${i}.txt" 2>/dev/null
        check "file${i}.txt 쓰기 반환 0" "0" "$?"
    done

    count=$(ls "$MOUNT_DIR" 2>/dev/null | wc -l)
    check "readdir 에서 3개 파일 확인" "3" "$count"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 8: 서브 디렉터리 아래 파일 쓰기
# ------------------------------------------------------------------------------
section "서브 디렉터리 아래 파일 쓰기"
setup
do_mount
if is_mounted; then
    mkdir "$MOUNT_DIR/logs" 2>/dev/null
    check "mkdir /logs 반환 0" "0" "$?"

    echo "log data" > "$MOUNT_DIR/logs/app.log" 2>/dev/null
    check "echo > /logs/app.log 반환 0" "0" "$?"

    ls "$MOUNT_DIR/logs/app.log" >/dev/null 2>&1
    check "서브 디렉터리 파일 존재 확인" "0" "$?"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 9: 한 페이지(512바이트) 정확히 쓰기
# ------------------------------------------------------------------------------
section "페이지 경계 쓰기 (512바이트)"
setup
do_mount
if is_mounted; then
    dd if=/dev/urandom of=/tmp/page_data.bin bs=512 count=1 2>/dev/null
    cp /tmp/page_data.bin "$MOUNT_DIR/page.bin" 2>/dev/null
    check "512바이트 파일 쓰기 반환 0" "0" "$?"

    ls "$MOUNT_DIR/page.bin" >/dev/null 2>&1
    check "512바이트 파일 존재 확인" "0" "$?"
    rm -f /tmp/page_data.bin
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 테스트 10: 같은 파일 두 번 쓰기 (writePage 중복 방지 버그 노출)
# ------------------------------------------------------------------------------
section "같은 파일 두 번 쓰기 (writePage 버그 시나리오)"
setup
do_mount
if is_mounted; then
    echo "first" > "$MOUNT_DIR/twice.txt" 2>/dev/null
    check "첫 번째 쓰기 반환 0" "0" "$?"

    echo "second" > "$MOUNT_DIR/twice.txt" 2>/dev/null
    ret=$?
    check "두 번째 쓰기 반환 0" "0" "$ret"

    # NOTE: uffs_read 버그(UFFS_TYPE_FILE을 u8* 로 전달)로 cat 불가.
    # getattr 은 가능하므로 파일 존재만 확인.
    ls "$MOUNT_DIR/twice.txt" >/dev/null 2>&1
    check "두 번째 쓰기 후 파일 존재 확인" "0" "$?"
    skip "두 번째 쓰기 데이터 검증: uffs_read SIGSEGV 버그로 read 불가 (수정 후 활성화)"
fi
do_unmount
teardown

# ------------------------------------------------------------------------------
# 결과 요약
# ------------------------------------------------------------------------------
printf "\n================================================\n"
printf "  결과: %d PASS  /  %d FAIL\n" "$g_pass" "$g_fail"
printf "================================================\n"

[ $g_fail -eq 0 ] && exit 0 || exit 1
