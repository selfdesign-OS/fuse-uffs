#include "uffs/uffs_device.h"

typedef struct uffs_FileEmuSt {
	int initCount;
	FILE *fp;
	FILE *dump_fp;
	u8 *em_monitor_page;		// page write monitor
	u8 * em_monitor_spare;		// spare write monitor
	u32 *em_monitor_block;		// block erease monitor
	const char *emu_filename;
#ifdef UFFS_FEMU_ENABLE_INJECTION
		// struct uffs_FlashOpsSt ops_orig;
	UBOOL wrap_inited;
#endif
} uffs_FileEmu;