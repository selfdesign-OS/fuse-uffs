#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

static void setup_emu_private(uffs_FileEmu *emu)
{
	memset(emu, 0, sizeof(uffs_FileEmu));
	emu->emu_filename = conf_emu_filename;
}

int main() {
	// setup file emulator storage with parameters from command line
	setup_storage(femu_GetStorage());

	// setup file emulator private data
	setup_emu_private(femu_GetPrivate());
}