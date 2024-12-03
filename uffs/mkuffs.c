#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uffs_os.h"
#include "uffs_public.h"
#include "uffs_fs.h"
// #include "uffs_utils.h"
#include "uffs_core.h"
#include "uffs_mtb.h"

// #include "cmdline.h"
#include "uffs_fileem.h"

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

static void setup_ramdisk() {
	// setup file emulator storage with parameters from command line
	setup_storage(femu_GetStorage());

	// setup file emulator private data
	setup_emu_private(femu_GetPrivate());
}

static int init_uffs_fs(void)
{
	uffs_Device *dev = &conf_device;
	
	uffs_MemSetupSystemAllocator(&dev->mem);

	setup_device(dev);

	uffs_Mount(dev);

	return uffs_InitFileSystemObjects() == U_SUCC ? 0 : -1;
}

int uffs_getattr(const char *name, struct stat *stbuf)
{
	fprintf(stderr, "[uffs_getattr] called\n");
    uffs_Object *obj;
	int ret = 0;
	int err = 0;
	URET result;

	// uffs_GlobalFsLockLock();

	obj = uffs_GetObject();
	if (obj) {
		if (*name && name[strlen(name) - 1] == '/') {
			result = uffs_OpenObject(obj, name, UO_RDONLY | UO_DIR, &conf_device);
		}
		else {
			if ((result = uffs_OpenObject(obj, name, UO_RDONLY, &conf_device)) != U_SUCC)	// try file
				result = uffs_OpenObject(obj, name, UO_RDONLY | UO_DIR, &conf_device);	// then try dir
		}
		if (result == U_SUCC) {
			ret = 0;
			// ret = do_stat(obj, buf);
			// uffs_CloseObject(obj);
		}
		else {
			err = uffs_GetObjectErr(obj);
			ret = -1;
		}
		// uffs_PutObject(obj);
	}
	else {
		err = UENOMEM;
		ret = -1;
	}

	// uffs_set_error(-err);
	// uffs_GlobalFsLockUnlock();

	fprintf(stderr, "[uffs_getattr] finished\n");
	return ret;
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
    int ret;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mount-directory>\n", argv[0]);
        return -1;
    }

    setup_ramdisk();

    ret = init_uffs_fs();
    if (ret != 0) {
        fprintf(stderr, "Init file system fail: %d\n", ret);
        return -1;
    }

    fprintf(stderr, "[main] init finished\n");

    return fuse_main(argc, argv, &uffs_oper);
}