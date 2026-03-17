# 버그 분석: echo로 쓴 파일이 빈 내용으로 읽히는 문제

## 문제 정의

```bash
echo "hello uffs" > /mnt/uffs/dd.txt
cat /mnt/uffs/dd.txt
# 출력: (빈 내용)
```

`echo`는 exit 0, `cat`도 exit 0이지만 내용이 비어있음.
로그에 `uffs_read` 호출 자체가 나타나지 않는 케이스 포함.

---

## 디버깅 과정

### 테스트 최소화

문제를 재현하고 범위를 좁히기 위해 `test_simple.sh`를 작성했다.

```bash
echo "hello uffs" > "$MOUNT_DIR/dd.txt"   # 쓰기
content=$(cat "$MOUNT_DIR/dd.txt")         # 읽기
check "내용 일치" "hello uffs" "$content"  # 비교
```

3단계(echo → cat → 비교)로 최소화해 재현성을 확보하고, 실패 시 쓰기/읽기/내용 중 어느 단계에서 깨지는지 바로 특정할 수 있도록 했다.

### valgrind로 write 경로 확인

`cat`이 빈 내용을 반환하는 상황에서 read 경로부터 추적하는 대신, `uffs_write` 호출 직후 valgrind로 디스크 이미지의 해당 블록 주소를 확인했다.
기록된 주소에 내용이 비어있었고, 이를 통해 **read가 아닌 write 자체에 문제가 있다**는 것을 특정했다.
이를 단서로 `uffs_write` 내부의 메타데이터 갱신 로직을 추적했고, `updateFileInfoPage`가 데이터 영역을 덮어쓰는 구조적 문제를 발견했다.

---

## 원인 분석

버그는 독립적인 4개가 연쇄 작용했다.

---

### 버그 1 — `file_node->u.file.block`이 데이터 블록으로 오염

**위치**: `mkuffs.c` `uffs_write()`

```c
// 제거 전
file_node->u.file.block = data_block_id;
```

`uffs_write`에서 데이터 노드를 새로 할당할 때 데이터 블록 번호를 파일 노드의 `block` 필드에도 저장했다.
`file_node->u.file.block`은 원래 **파일 메타데이터 블록**을 가리켜야 한다.

**연쇄 결과**:
이후 `updateFileInfoPage`가 `node->u.file.block`에 파일 이름·속성을 기록할 때, 메타데이터 블록이 아닌 **데이터 블록의 page 0**에 기록하게 된다.

---

### 버그 2 — `updateFileInfoPage`에 빈 구조체 전달로 파일 내용 덮어쓰기

**위치**: `mkuffs.c` `uffs_write()`

```c
// 수정 전
uffs_FileInfo file_info = {0};
updateFileInfoPage(&dev, file_node, &file_info, 0, UFFS_TYPE_FILE);

// 수정 후
uffs_FileInfo file_info = {0};
getFileInfoBySerial(dev.fd, file_node->u.file.serial, &file_info);  // 기존 정보 로드
updateFileInfoPage(&dev, file_node, &file_info, 0, UFFS_TYPE_FILE);
```

`updateFileInfoPage`는 타임스탬프 일부만 갱신하는 게 아니라 `file_info` 구조체 전체를 디스크에 쓴다.
`file_info = {0}`이면 `name = ""`, `name_len = 0`인 구조체가 통째로 기록된다.

버그 1과 결합하면:

```
writePage(data_block, page_0, "hello uffs")  ← 실제 데이터 기록
updateFileInfoPage(file_node, file_info={0})
  → writePage(data_block, page_0, zeros)     ← 데이터를 빈 구조체로 덮어씀
```

"hello uffs"가 기록된 직후 page 0 전체가 0으로 덮어씌워진다.
이 때문에 디스크에서 파일 내용을 읽으면 파일 이름도, 내용도 없는 상태가 된다.

---

### 버그 3 — `getattr`에서 `st_size = 0` 반환으로 read 스킵

**위치**: `mkuffs.c` `uffs_getattr()`

```c
// 수정 전
stbuf->st_size = object_info.len;  // object_info가 채워지지 않아 항상 0

// 수정 후
stbuf->st_size = node->u.file.len; // 트리 노드에서 실제 크기 참조
```

`object_info`는 초기화만 되고 실제로 채워지지 않아 `len = 0`이었다.
FUSE는 `getattr`에서 `st_size = 0`을 받으면 파일이 비어있다고 판단하고 `read` syscall을 호출하지 않는다.
`cat`은 exit 0을 반환하지만 내용은 빈 문자열이 된다.

---

### 버그 4 — `data_node->u.data.len` 미갱신으로 read 크기 0

**위치**: `mkuffs.c` `uffs_write()`

```c
// 수정 전
file_node->u.file.len = written;
// data_node->u.data.len은 0인 채로 방치

// 수정 후
file_node->u.file.len = written;
data_node->u.data.len = written;
```

`uffs_read` 내부:

```c
if (size > data_node->u.data.len) {
    size = data_node->u.data.len;  // data_node->u.data.len = 0 → size = 0
}
```

`data_node->u.data.len = 0`이면 읽기 루프가 실행되지 않고 빈 내용이 반환된다.

---

## 버그 연쇄 흐름

```
echo "hello uffs" > dd.txt
  └─ uffs_write
       ├─ [버그1] file_node->u.file.block = data_block  (메타데이터 블록 포인터 오염)
       ├─ [버그4] data_node->u.data.len = 0             (len 미갱신)
       ├─ writePage(data_block, page_0, "hello uffs")   (데이터 기록 성공)
       └─ updateFileInfoPage(file_info={0})
            └─ [버그2] writePage(data_block, page_0, zeros)  (데이터 덮어씀)

cat dd.txt
  └─ uffs_getattr
       └─ [버그3] st_size = object_info.len = 0
            └─ FUSE: read 호출 안 함 → 빈 내용 출력
  └─ uffs_read (버그3이 없는 경우)
       └─ [버그4] data_node->u.data.len = 0 → size = 0 → 루프 미실행
```

---

## 수정 요약

| 파일 | 위치 | 수정 내용 |
|------|------|-----------|
| `mkuffs.c` | `uffs_write` | `file_node->u.file.block = data_block_id` 라인 제거 |
| `mkuffs.c` | `uffs_write` | `updateFileInfoPage` 호출 전 `getFileInfoBySerial`로 기존 메타데이터 로드 |
| `mkuffs.c` | `uffs_write` | `data_node->u.data.len = written` 추가 |
| `mkuffs.c` | `uffs_getattr` | `st_size = object_info.len` → `st_size = node->u.file.len` |

---

## 참고: 1년 전 우회책 (`b25114e`)

당시 근본 원인을 모른 채 증상을 막는 방식으로 우회했다.

```c
// writePage에 static 변수로 중복 호출 차단
static int previous_block_id = -1;
static int previous_page_id  = -1;
if (previous_block_id == block_id && previous_page_id == page_Id) {
    return U_SUCC;  // 같은 page 재기록 차단
}
```

`updateFileInfoPage`가 데이터 블록 page 0을 덮어쓰는 것을 막아 동작은 했지만,
`static` 변수 특성상 같은 파일을 두 번 연속 쓰면 첫 번째 쓰기도 스킵될 수 있는 잠재적 버그가 남아있었다.
`st_size`도 `PAGE_DATA_SIZE_DEFAULT`(512)로 하드코딩해 FUSE가 read를 호출하도록 강제했다.
