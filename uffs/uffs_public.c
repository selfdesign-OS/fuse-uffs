/**
 * \file uffs_public.c
 * \brief public and miscellaneous functions
 * \author Ricky Zheng, created 10th May, 2005
 */

#include "uffs_config.h"
#include "uffs_types.h"
#include "uffs_core.h"
#include "uffs_device.h"
#include "uffs_os.h"
#include "uffs_crc.h"

#include <string.h>

/** 
 * calculate sum of data, 8bit version
 * \param[in] p data pointer
 * \param[in] len length of data
 * \return return sum of data, 8bit
 */
u8 uffs_MakeSum8(const void *p, int len)
{
	return uffs_crc16sum(p, len) & 0xFF;
}

/** 
 * calculate sum of datam, 16bit version
 * \param[in] p data pointer
 * \param[in] len length of data
 * \return return sum of data, 16bit
 */
u16 uffs_MakeSum16(const void *p, int len)
{
	return uffs_crc16sum(p, len);
}