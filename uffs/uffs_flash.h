#ifndef _UFFS_FLASH_H_
#define _UFFS_FLASH_H_

#include "uffs_types.h"
#include "uffs_core.h"
#include "uffs_device.h"
#include "uffs_fs.h"

/** flash operation return code */
#define UFFS_FLASH_NO_ERR		0		//!< no error
#define UFFS_FLASH_ECC_OK		1		//!< bit-flip found, but corrected by ECC
#define UFFS_FLASH_NOT_SEALED	2		//!< page spare area is not sealed properly (only for ReadPageWithLayout())
#define UFFS_FLASH_IO_ERR		-1		//!< I/O error
#define UFFS_FLASH_ECC_FAIL		-2		//!< ECC failed
#define UFFS_FLASH_BAD_BLK		-3		//!< bad block
#define UFFS_FLASH_CRC_ERR		-4		//!< CRC failed
#define UFFS_FLASH_UNKNOWN_ERR	-100	//!< unkown error?

#define UFFS_SPARE_LAYOUT_SIZE	6	//!< maximum spare layout array size, 2 segments

/** spare layout options (uffs_StorageAttrSt.layout_opt) */
#define UFFS_LAYOUT_UFFS	0	//!< do layout by dev->attr information
#define UFFS_LAYOUT_FLASH	1	//!< flash driver do the layout

/** ECC options (uffs_StorageAttrSt.ecc_opt) */
#define UFFS_ECC_NONE		0	//!< do not use ECC
#define UFFS_ECC_SOFT		1	//!< UFFS calculate the ECC
#define UFFS_ECC_HW			2	//!< Flash driver(or by hardware) calculate the ECC
#define UFFS_ECC_HW_AUTO	3	//!< Hardware calculate the ECC and automatically write to spare.

/** 
 * \struct uffs_StorageAttrSt
 * \brief uffs device storage attribute, provide by nand specific file
 */
struct uffs_StorageAttrSt {
	u32 total_blocks;		//!< total blocks in this chip
	u16 page_data_size;		//!< page data size (physical page data size, e.g. 512)
	u16 pages_per_block;	//!< pages per block
	u8 spare_size;			//!< page spare size (physical page spare size, e.g. 16)
	u8 block_status_offs;	//!< block status byte offset in spare
	int ecc_opt;			//!< ecc option ( #UFFS_ECC_[NONE|SOFT|HW|HW_AUTO] )
	int layout_opt;			//!< layout option (#UFFS_LAYOUT_UFFS or #UFFS_LAYOUT_FLASH)
	int ecc_size;			//!< ecc size in bytes
	const u8 *ecc_layout;	//!< page data ECC layout: [ofs1, size1, ofs2, size2, ..., 0xFF, 0]
	const u8 *data_layout;	//!< spare data layout: [ofs1, size1, ofs2, size2, ..., 0xFF, 0]
	u8 _uffs_ecc_layout[UFFS_SPARE_LAYOUT_SIZE];	//!< uffs spare ecc layout
	u8 _uffs_data_layout[UFFS_SPARE_LAYOUT_SIZE];	//!< uffs spare data layout
	void *_private;			//!< private data for storage attribute
};

#endif