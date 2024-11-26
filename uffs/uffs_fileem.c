#include "uffs_fileem.h"

static struct uffs_StorageAttrSt g_femu_storage = {0};
static struct uffs_FileEmuSt g_femu_private = {0};

struct uffs_StorageAttrSt * femu_GetStorage()
{
	return &g_femu_storage;
}

struct uffs_FileEmuSt *femu_GetPrivate()
{
	return &g_femu_private;
}