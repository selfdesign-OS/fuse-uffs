#ifndef UFFS_BUF_H
#define UFFS_BUF_H

#include "uffs/uffs_types.h"
#include "uffs/uffs_device.h"
#include "uffs/uffs_tree.h"
/** uffs page buffer */
struct uffs_BufSt{
	u8 type;							//!< #UFFS_TYPE_DIR or #UFFS_TYPE_FILE or #UFFS_TYPE_DATA
	u8 ext_mark;						//!< extension mark. 
	u16 parent;							//!< parent serial
	u16 serial;							//!< serial 
	u16 page_id;						//!< page id 
	u16 mark;							//!< #UFFS_BUF_EMPTY or #UFFS_BUF_VALID, or #UFFS_BUF_DIRTY ?
	u16 ref_count;						//!< reference counter, or #CLONE_BUF_MARK for a cloned buffer
	u16 data_len;						//!< length of data
	u16 check_sum;						//!< checksum field
	u8 * data;							//!< data buffer
	u8 * header;						//!< header
};
#endif