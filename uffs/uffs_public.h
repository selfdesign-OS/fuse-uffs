/** 
 * \file uffs_public.h
 * \brief public data structures for uffs
 * \author Ricky Zheng
 */

#ifndef _UFFS_PUBLIC_H_
#define _UFFS_PUBLIC_H_

#include "uffs_types.h"
#include "uffs_core.h"
// #include "uffs.h"
#include "uffs_pool.h"

#include "uffs_device.h"

/**
 * \def UFFS_TAG_PAGE_ID_SIZE_BITS
 * \brief define number of bits used for page_id in tag,
 *        this defines the maximum pages per block you can have.
 *        e.g. '9' ==> maximum 512 pages per block
 **/
#define UFFS_TAG_PAGE_ID_SIZE_BITS  6

/**
 * \def UFFS_MAX_PAGES_PER_BLOCK
 * \brief maximum allowed pages per block
 **/
#define UFFS_MAX_PAGES_PER_BLOCK    (1 << UFFS_TAG_PAGE_ID_SIZE_BITS)

#endif