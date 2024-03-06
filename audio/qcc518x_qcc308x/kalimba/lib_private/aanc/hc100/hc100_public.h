/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup lib_private\aanc
 *
 * \file  hc100_public.h
 * \ingroup lib_private\aanc
 *
 * HC100 library public header file.
 *
 */
#ifndef _HC100_LIB_PUBLIC_H_
#define _HC100_LIB_PUBLIC_H_

/* Imports HC100 structures */
#include "hc100_struct_public.h"
#include "hc100_defs_public.h"

/* Imports AANC_AFB structures */
#include "aanc_afb_struct_public.h"

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Determine how much memory to allocate for HC100 (bytes).
 *
 * \return  size value that will be populated with the memory required for
 *          HC100 (bytes).
 */
extern uint16 aanc_hc100_dmx_bytes(void);

/**
 * \brief  Determine how much memory to allocate for HC100 in DM1.
 *
 * \return  size value that will be populated with the memory required for
 *          HC100 in DM1 (bytes).
 */
extern uint16 aanc_hc100_dm1_bytes(void);

/**
 * \brief  Determine how much memory to allocate for HC100 in DM2.
 *
 * \return  size value that will be populated with the memory required for
 *          HC100 in DM2 (bytes).
 */
extern uint16 aanc_hc100_dm2_bytes(void);

/**
 * \brief  Populate the HC100 data object.
 *
 * \param  p_hc     Pointer to HC100 data object.
 * \param  p_dm1    Pointer to HC100 DM1 memory.
 * \param  p_dm2    Pointer to HC100 DM2 memory.
 *
 * \return  boolean indicating success or failure.
 *
 * The memory for HC100 must be allocated based on the return
 * value of aanc_hc100_dmx_bytes rather than sizeof(HC100).
 */
extern bool aanc_hc100_create(HC100_DMX *p_hc, uint8 *p_dm1, uint8 *p_dm2);

/**
 * \brief  Initialize the HC100 data object.
 *
 * \param  p_asf    Pointer to AANC feature handle.
 * \param  p_hc     Pointer to allocated HC100_DMX object.
 * \param  p_afb    Pointer to AFB object.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc_hc100_initialize(void *p_asf,
                                  HC100_DMX *p_hc,
                                  AANC_AFB *p_afb);

/**
 * \brief  Process data with the HC100 data object.
 *
 * \param  p_asf    Pointer to AANC feature handle.
 * \param  p_hc     Pointer to allocated HC100_DMX object.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc_hc100_process_data(void *p_asf, HC100_DMX *p_hc);

#endif /* _HC100_LIB_PUBLIC_H_ */