#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uffs_fileem.h"
#include "uffs_core.h"

#define DEFAULT_EMU_FILENAME "uffsemfile.bin"
const char * conf_emu_filename = DEFAULT_EMU_FILENAME;

/* default basic parameters of the NAND device */
#define PAGES_PER_BLOCK_DEFAULT			32
#define PAGE_DATA_SIZE_DEFAULT			512
#define PAGE_SPARE_SIZE_DEFAULT			16
#define STATUS_BYTE_OFFSET_DEFAULT		5
#define TOTAL_BLOCKS_DEFAULT			128
#define ECC_OPTION_DEFAULT				UFFS_ECC_SOFT

static int conf_pages_per_block = PAGES_PER_BLOCK_DEFAULT;
static int conf_page_data_size = PAGE_DATA_SIZE_DEFAULT;
static int conf_page_spare_size = PAGE_SPARE_SIZE_DEFAULT;
static int conf_status_byte_offset = STATUS_BYTE_OFFSET_DEFAULT;
static int conf_total_blocks = TOTAL_BLOCKS_DEFAULT;
static int conf_ecc_option = ECC_OPTION_DEFAULT;
static int conf_ecc_size = 0; // 0 - Let UFFS choose the size

static uffs_Device conf_device = {0};

static void setup_storage(struct uffs_StorageAttrSt *attr)
{
	attr->total_blocks = conf_total_blocks;				/* total blocks */
	attr->page_data_size = conf_page_data_size;			/* page data size */
	attr->spare_size = conf_page_spare_size;			/* page spare size */
	attr->pages_per_block = conf_pages_per_block;		/* pages per block */

	attr->block_status_offs = conf_status_byte_offset;	/* block status offset is 5th byte in spare */
	attr->ecc_opt = conf_ecc_option;					/* ECC option */
	attr->ecc_size = conf_ecc_size;						/* ECC size */
	attr->layout_opt = UFFS_LAYOUT_UFFS;				/* let UFFS handle layout */
}

static void setup_device(uffs_Device *dev)
{
	// TODO: We should call Init function.
	dev->Init = femu_InitDevice;
	dev->Release = femu_ReleaseDevice;
	dev->attr = femu_GetStorage();
}

static void setup_emu_private(uffs_FileEmu *emu)
{
	memset(emu, 0, sizeof(uffs_FileEmu));
	emu->emu_filename = conf_emu_filename;
}

int setup_ramdisk() {
	// setup file emulator storage with parameters from command line
	setup_storage(femu_GetStorage());

	// setup file emulator private data
	setup_emu_private(femu_GetPrivate());
}

static int init_uffs_fs(void)
{
	struct uffs_Device *dev = &conf_device
	
	uffs_MemSetupSystemAllocator(dev->mem);

	setup_device(dev);

	// uffs_Mount(???);

	return uffs_InitFileSystemObjects() == U_SUCC ? 0 : -1;
}

int uffs_getattr(const char *path, struct stat *stbuf)
{
    return 0;
}

int uffs_open(const char *path, struct fuse_file_info *fi)
{   
    return 0;
}

int uffs_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    return 0;
}

struct fuse_operations uffs_oper = {
	.getattr	= uffs_getattr,
	.open       = uffs_open,
    .read       = uffs_read
};

int main(int argc, char *argv[])
{
	// if (argc < 3) {
	// 	fprintf(stderr, "Usage: %s <mount-directory> <sizeinMB> [<disk-image>]\n", argv[0]);
	// 	return -1;
	// }

	// if (argc == 4) {
	// 	strcpy(filename, argv[3]);
	// }

	setup_ramdisk();

	ret = init_uffs_fs();
	if (ret != 0) {
		MSGLN ("Init file system fail: %d", ret);
		return -1;
	}
	
	//size_t storage = NBLOCKS*sizeof(block);
	//fprintf(stderr,"number of blocks: %d\n", NBLOCKS);
	//fprintf(stderr,"number of nodes: %d\n", NNODES);
	//fprintf(stderr,"Total space for storage: %lu\n", storage);

	argc = 2;
	return fuse_main(argc, argv, &uffs_oper);
}