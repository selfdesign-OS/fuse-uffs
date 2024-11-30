#include "uffs_fileem.h"

static struct uffs_StorageAttrSt g_femu_storage = {0};
static struct uffs_FileEmuSt g_femu_private = {0};

struct uffs_StorageAttrSt *femu_GetStorage()
{
	return &g_femu_storage;
}

struct uffs_FileEmuSt *femu_GetPrivate()
{
	return &g_femu_private;
}

URET femu_InitDevice(uffs_Device *dev)
{
	uffs_FileEmu *emu = femu_GetPrivate();

	// setup device storage attr and private data structure.
	// all femu partition share one storage attribute

	dev->attr = femu_GetStorage();
	dev->attr->_private = (void *) emu;

// 	// setup flash driver operations, according to the ecc option.
// 	switch(dev->attr->ecc_opt) {
// 		case UFFS_ECC_NONE:
// 		case UFFS_ECC_SOFT:
// 			dev->ops = &g_femu_ops_ecc_soft;
// 			break;
// 		case UFFS_ECC_HW:
// 			dev->ops = &g_femu_ops_ecc_hw;
// 			break;
// 		case UFFS_ECC_HW_AUTO:
// 			dev->ops = &g_femu_ops_ecc_hw_auto;
// 			break;
// 		default:
// 			break;
// 	}

// #ifdef UFFS_FEMU_ENABLE_INJECTION
// 	// setup wrap functions, for inject ECC errors, etc.
// 	// check wrap_inited so that multiple devices can share the same driver
// 	if (!emu->wrap_inited) {
// 		femu_setup_wrapper_functions(dev);
// 		emu->wrap_inited = U_TRUE;
// 	}
// #endif

	return U_SUCC;
}

/* Nothing to do here */
URET femu_ReleaseDevice(uffs_Device *dev)
{
	return U_SUCC;
}