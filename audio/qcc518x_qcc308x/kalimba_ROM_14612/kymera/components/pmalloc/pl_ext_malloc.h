/*************************************************************************
 * Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
*************************************************************************/
/**
 * \file pl_ext_malloc.h
 * \ingroup pl_malloc
 *
 * Interface to allocate memory from an external device.
 */

#ifndef _PL_EXT_MALLOC_H_
#define _PL_EXT_MALLOC_H_

/****************************************************************************
Include Files
*/
#include "types.h"

/**
 * NAME
 *   ext_malloc_enable
 *
 * \brief  enable or disable the external malloc
 *
 * \param[in]  enable : TRUE to enable and FALSE to disable
 *
 * \return bool
 */
extern bool ext_malloc_enable(bool enable);

/**
 * NAME
 *   is_addr_in_ext_heap
 *
 * \brief  Determine if an address in the the external heap (SRAM)
 *
 * \param[in]  ptr: pointer to the memory
 *
 * \return bool
 */
extern bool is_addr_in_ext_heap(void *ptr);

#endif /*_PL_EXT_MALLOC_H_*/

