/**
 * \file uffs_blockinfo.c
 * \brief block information cache system manipulations
 * \author Ricky Zheng, created 10th May, 2005
 */

#include "uffs_config.h"
#include "uffs_blockinfo.h"
#include "uffs_public.h"
#include "uffs_os.h"
// #include "uffs_badblock.h"

#include <string.h>

/** 
 * \brief load page spare data to given block info structure
 *			with given page number
 * \param[in] dev uffs device
 * \param[in] work given block info to be filled with
 * \param[in] page given page number to be read from,
 *			  if #UFFS_ALL_PAGES is presented, it will read
 *			  all pages, otherwise it will read only one given page.
 * \return load result
 * \retval U_SUCC successful
 * \retval U_FAIL fail to load
 * \note work->block must be set before load block info
 */
URET uffs_BlockInfoLoad(uffs_Device *dev, uffs_BlockInfo *work, int page)
{
	int i, ret, nfailed;
	uffs_PageSpare *spare;

	if (page == UFFS_ALL_PAGES) {
		nfailed = 0;
		for (i = 0; i < dev->attr->pages_per_block; i++) {
			spare = &(work->spares[i]);
			if (spare->expired == 0)
				continue;

			ret = uffs_FlashReadPageTag(dev, work->block, i,
											&(spare->tag));

			uffs_BadBlockAddByFlashResult(dev, work->block, ret);

			if (UFFS_FLASH_HAVE_ERR(ret)) {
				uffs_Perror(UFFS_MSG_SERIOUS,
							"load block %d page %d spare fail.",
							work->block, i);
				TAG_VALID_BIT(&(spare->tag)) = TAG_INVALID;	
				nfailed++;	
			}

			spare->expired = 0;
			work->expired_count--;
		}
		if (nfailed > 0)
			return U_FAIL;
	}
	else {
		if (page < 0 || page >= dev->attr->pages_per_block) {
			uffs_Perror(UFFS_MSG_SERIOUS, "page out of range !");
			return U_FAIL;
		}
		spare = &(work->spares[page]);
		if (spare->expired) {
			ret = uffs_FlashReadPageTag(dev, work->block, page,
											&(spare->tag));

            uffs_BadBlockAddByFlashResult(dev, work->block, ret);

			if (UFFS_FLASH_HAVE_ERR(ret)) {
				uffs_Perror(UFFS_MSG_SERIOUS,
							"load block %d page %d spare fail.",
							work->block, page);
				return U_FAIL;
			}
			spare->expired = 0;
			work->expired_count--;
		}
	}
	return U_SUCC;
}