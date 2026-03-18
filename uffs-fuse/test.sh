#!/bin/bash
# =============================================================================
# test.sh
# FUSE2 마운트 기반 통합 테스트 — 파일시스템 핵심 동작 검증
#
# 구현된 기능: getattr, readdir, open/opendir, read, write(offset 지원), create, mkdir
#
# 빌드: make (mkuffs 실행파일 필요)
# 실행: ./test.sh
# =============================================================================

MKUFFS=./mkuffs
DISK_IMG=/tmp/uffs_integ_disk.img
MOUNT_DIR=/tmp/uffs_integ_mnt
# 128 blocks * 32 pages * 528 bytes/page
DISK_SIZE=$(( 128 * 32 * 528 ))

g_pass=0
g_fail=0

# ─────────────────────────────────────────────────────────────────────────────
# 출력 헬퍼
# ─────────────────────────────────────────────────────────────────────────────
C_PASS="\033[32m"
C_FAIL="\033[31m"
C_SKIP="\033[33m"
C_BOLD="\033[1m"
C_RST="\033[0m"

PASS="${C_PASS}[PASS]${C_RST}"
FAIL="${C_FAIL}[FAIL]${C_RST}"
SKIP="${C_SKIP}[SKIP]${C_RST}"

check() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$actual" = "$expected" ]; then
        printf "  $PASS %s\n" "$desc"
        (( g_pass++ ))
    else
        printf "  $FAIL %s\n" "$desc"
        printf "         ${C_BOLD}기대${C_RST}: '%s'\n" "$expected"
        printf "         ${C_BOLD}실제${C_RST}: '%s'\n" "$actual"
        (( g_fail++ ))
    fi
}

check_contains() {
    local desc="$1" haystack="$2" needle="$3"
    if echo "$haystack" | grep -qF "$needle"; then
        printf "  $PASS %s\n" "$desc"
        (( g_pass++ ))
    else
        printf "  $FAIL %s\n" "$desc"
        printf "         ${C_BOLD}찾는 값${C_RST}: '%s'\n" "$needle"
        printf "         ${C_BOLD}실제 목록${C_RST}: '%s'\n" "$haystack"
        (( g_fail++ ))
    fi
}

skip() { printf "  $SKIP %s\n" "$1"; }

section() {
    printf "\n${C_BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${C_RST}\n"
    printf "${C_BOLD}[TEST]${C_RST} %s\n" "$1"
}

# ─────────────────────────────────────────────────────────────────────────────
# 마운트 헬퍼
# ─────────────────────────────────────────────────────────────────────────────
MKUFFS_PID=""

setup() {
    rm -f "$DISK_IMG"
    dd if=/dev/zero of="$DISK_IMG" bs="$DISK_SIZE" count=1 2>/dev/null
    mkdir -p "$MOUNT_DIR"
}

do_mount() {
    "$MKUFFS" "$MOUNT_DIR" -f "$DISK_IMG" >/tmp/mkuffs_log.txt 2>&1 &
    MKUFFS_PID=$!
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

is_mounted() { mountpoint -q "$MOUNT_DIR" 2>/dev/null; }

# ─────────────────────────────────────────────────────────────────────────────
# 1. 마운트 / 언마운트
# ─────────────────────────────────────────────────────────────────────────────
section "마운트 / 언마운트"
setup
if do_mount; then
    check "마운트 성공" "0" "0"
    do_unmount
    is_mounted
    check "언마운트 후 마운트포인트 해제" "1" "$?"
else
    check "마운트 성공" "0" "1"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 2. 빈 루트 readdir
# ─────────────────────────────────────────────────────────────────────────────
section "빈 루트 readdir"
setup
if do_mount; then
    entries=$(ls "$MOUNT_DIR" 2>/dev/null | wc -l | tr -d ' ')
    check "빈 루트 엔트리 수 = 0" "0" "$entries"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 3. 파일 생성 (create / getattr)
# ─────────────────────────────────────────────────────────────────────────────
section "파일 생성 (create / getattr)"
setup
if do_mount; then
    printf '' > "$MOUNT_DIR/hello.txt" 2>/dev/null
    check "파일 생성 반환 0" "0" "$?"

    ls "$MOUNT_DIR/hello.txt" >/dev/null 2>&1
    check "ls 로 파일 존재 확인" "0" "$?"

    ftype=$(stat -c "%F" "$MOUNT_DIR/hello.txt" 2>/dev/null)
    check "getattr 타입 = 'regular empty file'" "regular empty file" "$ftype"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 4. 디렉터리 생성 (mkdir / getattr)
# ─────────────────────────────────────────────────────────────────────────────
section "디렉터리 생성 (mkdir / getattr)"
setup
if do_mount; then
    mkdir "$MOUNT_DIR/subdir" 2>/dev/null
    check "mkdir 반환 0" "0" "$?"

    [ -d "$MOUNT_DIR/subdir" ]
    check "디렉터리 타입 확인" "0" "$?"

    dtype=$(stat -c "%F" "$MOUNT_DIR/subdir" 2>/dev/null)
    check "getattr 타입 = 'directory'" "directory" "$dtype"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 5. 쓰기 / 읽기 왕복 (write → read 내용 일치)
# ─────────────────────────────────────────────────────────────────────────────
section "쓰기/읽기 왕복 — 내용 및 크기 검증"
setup
if do_mount; then
    printf 'Hello UFFS' > "$MOUNT_DIR/msg.txt"
    check "write 반환 0" "0" "$?"

    content=$(cat "$MOUNT_DIR/msg.txt" 2>/dev/null)
    check "read 내용 = 'Hello UFFS'" "Hello UFFS" "$content"

    fsize=$(stat -c "%s" "$MOUNT_DIR/msg.txt" 2>/dev/null)
    check "파일 크기 = 10" "10" "$fsize"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 6. 부분 쓰기 — offset > 0 (read-modify-write)
# ─────────────────────────────────────────────────────────────────────────────
section "부분 쓰기 (offset=4, 앞뒤 바이트 보존 확인)"
setup
if do_mount; then
    # 10 A 기록
    printf 'AAAAAAAAAA' > "$MOUNT_DIR/partial.txt"
    check "초기 10바이트 쓰기" "0" "$?"

    # offset 4에 3 B 덮어쓰기 (conv=notrunc 로 기존 데이터 유지)
    printf 'BBB' | dd of="$MOUNT_DIR/partial.txt" bs=1 seek=4 conv=notrunc 2>/dev/null
    check "offset=4 부분 쓰기" "0" "$?"

    content=$(cat "$MOUNT_DIR/partial.txt" 2>/dev/null)
    check "내용: offset 0-3 보존, 4-6 덮어쓰기, 7-9 보존" "AAAABBBAAA" "$content"

    fsize=$(stat -c "%s" "$MOUNT_DIR/partial.txt" 2>/dev/null)
    check "파일 크기 = 10 (길이 변경 없음)" "10" "$fsize"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 7. 이어쓰기 (append, >>)
# ─────────────────────────────────────────────────────────────────────────────
section "이어쓰기 (append, >>)"
setup
if do_mount; then
    printf 'HELLO' > "$MOUNT_DIR/append.txt"
    printf 'WORLD' >> "$MOUNT_DIR/append.txt"
    check "append 쓰기 반환 0" "0" "$?"

    content=$(cat "$MOUNT_DIR/append.txt" 2>/dev/null)
    check "append 후 내용 = 'HELLOWORLD'" "HELLOWORLD" "$content"

    fsize=$(stat -c "%s" "$MOUNT_DIR/append.txt" 2>/dev/null)
    check "파일 크기 = 10" "10" "$fsize"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 8. readdir — 여러 파일 이름 확인
# ─────────────────────────────────────────────────────────────────────────────
section "여러 파일 readdir (파일명 포함 여부)"
setup
if do_mount; then
    printf 'a' > "$MOUNT_DIR/alpha.txt"
    printf 'b' > "$MOUNT_DIR/beta.txt"
    printf 'c' > "$MOUNT_DIR/gamma.txt"

    listing=$(ls "$MOUNT_DIR" 2>/dev/null)
    count=$(echo "$listing" | wc -l | tr -d ' ')
    check "readdir 엔트리 수 = 3" "3" "$count"
    check_contains "readdir 에 alpha.txt 포함" "$listing" "alpha.txt"
    check_contains "readdir 에 beta.txt 포함"  "$listing" "beta.txt"
    check_contains "readdir 에 gamma.txt 포함" "$listing" "gamma.txt"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 9. 서브 디렉터리 아래 파일 쓰기 / 읽기
# ─────────────────────────────────────────────────────────────────────────────
section "서브 디렉터리 파일 쓰기/읽기 (/logs/app.log)"
setup
if do_mount; then
    mkdir "$MOUNT_DIR/logs" 2>/dev/null
    check "mkdir /logs 반환 0" "0" "$?"

    printf 'log entry 1' > "$MOUNT_DIR/logs/app.log"
    check "쓰기 반환 0" "0" "$?"

    content=$(cat "$MOUNT_DIR/logs/app.log" 2>/dev/null)
    check "read 내용 = 'log entry 1'" "log entry 1" "$content"

    sub_listing=$(ls "$MOUNT_DIR/logs" 2>/dev/null)
    check_contains "서브 디렉터리 readdir 에 app.log 포함" "$sub_listing" "app.log"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 10. 멀티 페이지 파일 — 1025 bytes (512바이트 페이지 2개 이상)
# ─────────────────────────────────────────────────────────────────────────────
section "멀티 페이지 파일 쓰기/읽기 (1025 bytes)"
setup
if do_mount; then
    dd if=/dev/urandom of=/tmp/uffs_large.bin bs=1025 count=1 2>/dev/null
    cp /tmp/uffs_large.bin "$MOUNT_DIR/large.bin" 2>/dev/null
    check "1025바이트 쓰기 반환 0" "0" "$?"

    diff /tmp/uffs_large.bin "$MOUNT_DIR/large.bin" 2>/dev/null
    check "1025바이트 읽기 내용 일치 (diff)" "0" "$?"

    fsize=$(stat -c "%s" "$MOUNT_DIR/large.bin" 2>/dev/null)
    check "파일 크기 = 1025" "1025" "$fsize"

    rm -f /tmp/uffs_large.bin
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 11. 덮어쓰기 — 같은 파일 두 번 쓰기
# ─────────────────────────────────────────────────────────────────────────────
section "덮어쓰기 — 같은 파일 두 번 쓰기"
setup
if do_mount; then
    printf 'FIRST_DATA' > "$MOUNT_DIR/twice.txt"
    check "첫 번째 쓰기 반환 0" "0" "$?"

    # truncate 미구현 → > 대신 dd conv=notrunc 로 덮어쓰기 (같은 길이, 10 bytes)
    printf 'SECOND_DAT' | dd of="$MOUNT_DIR/twice.txt" bs=1 seek=0 conv=notrunc 2>/dev/null
    check "두 번째 쓰기 반환 0" "0" "$?"

    content=$(cat "$MOUNT_DIR/twice.txt" 2>/dev/null)
    check "두 번째 쓰기 내용 반영 확인" "SECOND_DAT" "$content"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 12. 512 바이트 경계 쓰기/읽기 (정확히 1 페이지)
# ─────────────────────────────────────────────────────────────────────────────
section "512바이트 경계 쓰기/읽기 (1 page 정확히)"
setup
if do_mount; then
    dd if=/dev/urandom of=/tmp/uffs_page.bin bs=512 count=1 2>/dev/null
    cp /tmp/uffs_page.bin "$MOUNT_DIR/page.bin" 2>/dev/null
    check "512바이트 쓰기 반환 0" "0" "$?"

    diff /tmp/uffs_page.bin "$MOUNT_DIR/page.bin" 2>/dev/null
    check "512바이트 읽기 내용 일치 (diff)" "0" "$?"

    fsize=$(stat -c "%s" "$MOUNT_DIR/page.bin" 2>/dev/null)
    check "파일 크기 = 512" "512" "$fsize"

    rm -f /tmp/uffs_page.bin
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 13. 중첩 디렉터리 (/a/b/c) + 파일 쓰기/읽기
# ─────────────────────────────────────────────────────────────────────────────
section "중첩 디렉터리 파일 쓰기/읽기 (/a/b/c/deep.txt)"
setup
if do_mount; then
    mkdir "$MOUNT_DIR/a" 2>/dev/null
    check "mkdir /a 반환 0" "0" "$?"
    mkdir "$MOUNT_DIR/a/b" 2>/dev/null
    check "mkdir /a/b 반환 0" "0" "$?"
    mkdir "$MOUNT_DIR/a/b/c" 2>/dev/null
    check "mkdir /a/b/c 반환 0" "0" "$?"

    printf 'deep file' > "$MOUNT_DIR/a/b/c/deep.txt"
    check "/a/b/c/deep.txt 쓰기 반환 0" "0" "$?"

    content=$(cat "$MOUNT_DIR/a/b/c/deep.txt" 2>/dev/null)
    check "/a/b/c/deep.txt 읽기 내용 일치" "deep file" "$content"
fi
teardown

# ─────────────────────────────────────────────────────────────────────────────
# 결과 요약
# ─────────────────────────────────────────────────────────────────────────────
total=$(( g_pass + g_fail ))
printf "\n${C_BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${C_RST}\n"
printf "  전체 %d 개 중  ${C_PASS}%d PASS${C_RST}  /  ${C_FAIL}%d FAIL${C_RST}\n" \
    "$total" "$g_pass" "$g_fail"
if [ "$g_fail" -eq 0 ]; then
    printf "  ${C_PASS}${C_BOLD}✔  ALL TESTS PASSED${C_RST}\n"
else
    printf "  ${C_FAIL}${C_BOLD}✖  %d 개 테스트 실패${C_RST}\n" "$g_fail"
fi
printf "${C_BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${C_RST}\n"

[ "$g_fail" -eq 0 ] && exit 0 || exit 1
