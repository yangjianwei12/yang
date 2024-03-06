/*************************************************************************
 * Copyright (c) 2009 - 2019 Qualcomm Technologies International, Ltd.
*************************************************************************/
/**
 * \file pl_ext_malloc.c
 * \ingroup pl_malloc
 */

/****************************************************************************
Include Files
*/
#include "pl_malloc_private.h"
#if !defined(DESKTOP_TEST_BUILD) && !defined(RUNNING_ON_KALSIM)
#include "hal/hal_sram.h"
#endif

/**
 * \brief  enable or disable the external malloc
 *
 * \param[in]  enable : TRUE to enable and FALSE to disable
 *
 * \return bool - TRUE on success
 */
#ifdef RUNNING_ON_KALSIM
bool ext_malloc_enable(bool enable)
{
   return FALSE;
}
#else
bool ext_malloc_enable(bool enable)
{
   return  ext_heap_enable(enable);
}
#endif /* RUNNING_ON_KALSIM */

#if defined(DESKTOP_TEST_BUILD)
bool ext_malloc_enabled(void)
{
    return TRUE;
}
#elif defined(HAVE_EXTERNAL_HEAP)
bool ext_malloc_enabled(void)
{
    return hal_get_sram_enabled();
}
#endif

/**
 * NAME
 *   is_addr_in_ext_heap
 *
 * \brief  Determine if an address in the the external heap (SRAM)
 *
 * \param[in]  ptr: pointer to the memory
 *
 * \return bool - TRUE if address in external heap
 */
#if !defined(HAVE_EXTERNAL_HEAP)
bool is_addr_in_ext_heap(void *ptr)
{
   return  FALSE;
}
#else
bool is_addr_in_ext_heap(void *ptr)
{
   return  (get_heap_num(ptr) == HEAP_EXT);
}
#endif /* !defined(HAVE_EXTERNAL_HEAP) */
