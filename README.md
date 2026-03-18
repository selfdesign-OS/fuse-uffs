# fuse-uffs

UFFS(Ultra-low-cost Flash File System) 파일시스템을 FUSE로 구현한 테스트 환경입니다.
임베디드 환경에 UFFS를 올리기 전에 Linux 위에서 동작을 검증하는 목적으로 사용합니다.

---

> **⚠️ 주의: 부분 구현**
>
> 이 구현체는 UFFS 전체 스펙의 약 30%만 구현한 상태입니다.
> 기존 UFFS의 완전한 동작을 기대하고 사용하면 안 됩니다.
>
> **구현된 기능**
> - 마운트 / 언마운트
> - 트리 구조 (dir / file / data 노드)
> - 파일 생성, 쓰기, 읽기
> - 디렉터리 생성, 목록 조회
>
> **미구현 기능**
> - 버퍼 캐시
> - Copy-on-Write (CoW)
> - 가비지 컬렉터
> - CRC 검증
> - 배드 블록 관리
> - 파일 수정 (`truncate` 미구현으로 덮어쓰기(`>`) 불가, `offset` 미처리로 추가쓰기(`>>`) 불가)

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
cd uffs-fuse
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

# 파일 생성, 읽기, 쓰기 등 핵심기능에 대한 41개의 테스트
bash test.sh
```

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


### Members
|Jae-hyeong|Sung-woon|byeong-hyeon|
|:-:|:-:|:-:|
|<img src="https://github.com/user-attachments/assets/ef510219-2286-4048-9810-505ca2b5d5e6" alt="jae-hyeong" width="100" height="100">|<img src="https://github.com/user-attachments/assets/b9478ab7-b1b6-4313-bbc8-38e195364dde" alt="dingwoonee" width="100" height="100">|<img src="https://github.com/user-attachments/assets/d75b48a4-c3bb-4280-be3c-e0fbf4ea8464" alt="dingwoonee" width="100" height="100">|
|[JaehyeongIm](https://github.com/JaehyeongIm)|[DingWoonee](https://github.com/DingWoonee)|[bangbang444](https://github.com/bangbang444)|

### Technologies Used
<img src="https://img.shields.io/badge/C-00599C?style=flat-square&logo=C%2B%2B&logoColor=white"/> <img src="https://img.shields.io/badge/Ubuntu-E95420?style=flat-square&logo=Ubuntu&logoColor=white"/> <img src="https://img.shields.io/badge/FUSE-4E6C50?style=flat-square&logoColor=white"/>