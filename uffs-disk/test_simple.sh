#!/bin/bash
MKUFFS=./mkuffs
DISK_IMG=/tmp/uffs_test.img
MOUNT_DIR=/tmp/uffs_test_mnt
DISK_SIZE=$(( 128 * 32 * 528 ))

PASS="\033[32m[PASS]\033[0m"
FAIL="\033[31m[FAIL]\033[0m"
g_pass=0; g_fail=0

check() {
    [ "$2" = "$3" ] && { printf "  $PASS %s\n" "$1"; (( g_pass++ )); } \
                    || { printf "  $FAIL %s  (기대='%s' 실제='%s')\n" "$1" "$2" "$3"; (( g_fail++ )); }
}

# 준비
rm -f "$DISK_IMG"
dd if=/dev/zero of="$DISK_IMG" bs="$DISK_SIZE" count=1 2>/dev/null
mkdir -p "$MOUNT_DIR"

# 마운트
"$MKUFFS" "$MOUNT_DIR" -f "$DISK_IMG" >/tmp/uffs_log.txt 2>&1 &
for i in $(seq 1 30); do mountpoint -q "$MOUNT_DIR" && break; sleep 0.1; done
mountpoint -q "$MOUNT_DIR" || { echo "마운트 실패"; exit 1; }
echo "[INFO] 마운트 성공"

# 쓰기
echo "hello uffs" > "$MOUNT_DIR/dd.txt" 2>/dev/null
check "echo > dd.txt 성공" "0" "$?"

# 읽기
content=$(cat "$MOUNT_DIR/dd.txt" 2>/dev/null)
check "cat dd.txt 성공" "0" "$?"
check "내용 일치" "hello uffs" "$content"

# 정리
fusermount -u "$MOUNT_DIR" 2>/dev/null
wait 2>/dev/null
rmdir "$MOUNT_DIR" 2>/dev/null
rm -f "$DISK_IMG"

printf "\n결과: %d PASS / %d FAIL\n" "$g_pass" "$g_fail"
[ $g_fail -eq 0 ] && exit 0 || exit 1
