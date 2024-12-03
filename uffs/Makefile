# 컴파일러와 플래그 설정
CC = gcc
CFLAGS = -Wall -g -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags`
LDFLAGS = `pkg-config fuse --libs`

# 파일 이름 설정
TARGET = mkuffs
SRCS = mkuffs.c uffs_fd.c uffs_fileem.c uffs_fs.c uffs_init.c uffs_mtb.c uffs_os.c uffs_pool.c uffs_tree.c
HEADERS = uffs_buf.h uffs_config.h uffs_core.h uffs_device.h uffs_fd.h uffs_fileem.h uffs_find.h uffs_flash.h uffs_fs.h uffs_mem.h uffs_mtb.h uffs_os.h uffs_pool.h uffs_public.h uffs_tree.h uffs_types.h uffs.h

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
	rm -f $(OBJS) $(TARGET)

# 리빌드
rebuild: clean all