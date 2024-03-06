#ifndef CSR_HYDRA_PMALLOC_H__
#define CSR_HYDRA_PMALLOC_H__

/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_hydra_types.h"


/**
 * Allocate a block of private memory
 *
 * Allocate a block of private memory of at least "size" locations.
 *
 * If "size" is zero then a zero-length block is allocated, and a valid
 * pointer returned. Note that this still uses resources.
 *
 * The z...() forms of the function initialise the block to zero, otherwise
 * the contents are undefined.
 *
 * Returns
 *
 * A pointer to the allocated memory block. The x...() forms of the function
 * returns NULL if the requested memory is not available.
 */
extern void *PanicUnlessMalloc(size_t size);

/**
 * Return a block of private memory to the pool
 *
 * Deallocate a block of memory previously allocated by pmalloc() or
 * prealloc(). If "ptr" is NULL then no action is taken.
 */
extern void pfree(void *ptr);


/**
 *  \brief Frees the message memory pointed to by data.
 *  \param id The message identifier.
 *  \param data A pointer to the memory to free.
 *
 * \ingroup trapset_core
 */
extern void MessageFree(uint16 id, const void *msg);

#endif /* CSR_HYDRA_PMALLOC_H__ */
