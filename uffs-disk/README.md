# fuse-uffs

UFFS(Ultra-low-cost Flash File System) 파일시스템을 FUSE로 구현한 테스트 환경입니다.
임베디드 환경에 UFFS를 올리기 전에 Linux 위에서 동작을 검증하는 목적으로 사용합니다.

---

## 테스트 환경

- OS: Ubuntu 22.04 (베어메탈)
- FUSE: 2.9.9

---

## 의존성 설치

```bash
sudo apt install gcc make libfuse-dev
```

---

## 빌드

```bash
cd uffs-disk
make
```

---

## 실행

```bash
# 디스크 이미지 생성 (128블록 * 32페이지 * 528바이트)
dd if=/dev/zero of=disk.img bs=$((128 * 32 * 528)) count=1

# 마운트
./mkuffs /mnt/uffs -f disk.img

# 언마운트
fusermount -u /mnt/uffs
```

---

## 테스트

### 통합 테스트 (FUSE 마운트 기반)

```bash
# 파일 쓰기/읽기 기본 테스트
bash test_simple.sh

# 파일 생성, 디렉터리, 다중 파일 등 시나리오 테스트
bash test_write.sh
```

### 단위 테스트 (AddressSanitizer)

```bash
make test_write
./test_write
```

---

## 디스크 레이아웃

| 블록 | 용도 |
|------|------|
| 0 | 매직 넘버 |
| 1 | 루트 디렉터리 메타데이터 |
| 2~ | 파일/디렉터리/데이터 블록 |

페이지 구조: `MiniHeader(4B)` + `Data(512B)` + `Tag(16B)` = 528B

---

## 소스 구조

| 파일 | 설명 |
|------|------|
| `mkuffs.c` | FUSE 콜백 구현 (getattr, read, write, mkdir 등) |
| `uffs_tree.c` | 트리 노드 관리 (dir / file / data) |
| `uffs_disk.c` | 디스크 읽기/쓰기 (readPage, writePage) |
