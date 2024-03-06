/****************************************************************************
 * Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  noiseid100_public.h
 * \ingroup lib_private\aanc
 *
 * NOISEID100 library public header file.
 *
 */
#ifndef _NOISEID100_LIB_PUBLIC_H_
#define _NOISEID100_LIB_PUBLIC_H_

/* Imports NOISEID100 structures */
#include "noiseid100_struct_public.h"
#include "noiseid100_defs_public.h"

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Determine how much memory to allocate for NOISEID100_DMX (bytes).
 *
 * \return  size value that will be populated with the memory required for
 *          NOISEID100_DMX (bytes).
 */
extern uint16 aanc_noiseID100_dmx_bytes(void);

/**
 * \brief  Determine how much memory to allocate for NOISEID100 in DM1.
 *
 * \return  size value that will be populated with the memory required for
 *          NOISEID100 in DM1 (bytes).
 */
extern uint16 aanc_noiseID100_dm1_bytes(void);

/**
 * \brief  Create the NOISEID100 data object.
 *
 * \param  p_noiseID  Pointer to allocated NOISEID100_DMX object. This should be
 *                allocated using the value returned from
 *                aanc_noiseID100_dmx_bytes.
 * \param  p_dm1  Pointer to memory space allocated for NOISEID100 in DM1. This
 *                should be allocated using the value returned from
 *                aanc_noiseID100_dm1_bytes.
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool aanc_noiseID100_create(void *p_noiseID, uint8 *p_dm1);

/**
 * \brief  Initialize the NOISEID100 data object.
 *
 * \param  p_asf      Pointer to AANC feature handle.
 * \param  p_noiseID  Pointer to allocated NOISEID100_DMX object.
 * \param  afb_object Pointer to analysis filter bank object.
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool aanc_noiseID100_initialize(void *p_asf,
                                       void *p_noiseID,
                                       void *afb_object);

/**
 * \brief  Process data with NOISEID100.
 *
 * \param  p_asf  Pointer to AANC feature handle.
 * \param  p_noiseID  Pointer to allocated NOISEID100_DMX object.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc_noiseID100_process_data(void *p_asf, void *p_noiseID);

/**
 * \brief  Initialize Noise ID to user defined category.
 *
 * \param  p_noiseID  Pointer to allocated NOISEID100_DMX object.
 * \param  noise_id   New noise category to initialize.
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc_noiseID100_set_current_noise_id(void *p_noiseID,
                                                 uint16 noise_id);

#endif /* _NOISEID100_LIB_PUBLIC_H_ */
