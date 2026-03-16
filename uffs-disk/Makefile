# 컴파일러와 플래그 설정
CC = gcc
CFLAGS = -Wall -g -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags`
LDFLAGS = `pkg-config fuse --libs`

# 파일 이름 설정
TARGET = mkuffs
SRCS = mkuffs.c uffs_tree.c uffs_disk.c
HEADERS = uffs_device.h uffs_disk.h uffs_tree.h uffs_types.h

# 오브젝트 파일 생성
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 개별 소스 파일 컴파일
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 제거
clean:
	rm -f $(OBJS) $(TARGET) test_write

# 단위 테스트 (ASan)
TEST_SRCS = test_write.c mkuffs.c uffs_tree.c uffs_disk.c
TEST_CFLAGS = -Wall -g -DUNIT_TEST -fsanitize=address -D_FILE_OFFSET_BITS=64
TEST_LDFLAGS = -fsanitize=address

test_write: $(TEST_SRCS) $(HEADERS) fuse_compat.h
	$(CC) $(TEST_CFLAGS) $(TEST_SRCS) -o test_write $(TEST_LDFLAGS)

# 리빌드
rebuild: clean all