/****************************************************************************
 * Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  noiseid100_struct_public.h
 * \ingroup lib_private\aanc
 *
 * NoiseID100 library header file providing public data structures.
 *
 */
#ifndef _NOISEID100_LIB_STRUCT_PUBLIC_H_
#define _NOISEID100_LIB_STRUCT_PUBLIC_H_

/* Imports Kalimba type definitions */
#include "types.h"

#include "noiseid100_defs_public.h"

/******************************************************************************
Public Constant Definitions
*/
typedef enum
{
    NID_NOISE_ID0,
    NID_NOISE_ID1
} NID_NOISE_ID_TYPE;

/******************************************************************************
Public Variable Definitions.

These provide a means to pass data and variables into and out of the NOISEID100
library.
*/


/* Type definition for the NOISEID100 library. Note that this must be allocated
 * with enough space as determined by `aanc_noiseid100_dmx_bytes` to ensure the
 * private fields are allocated.
 */
typedef struct _NOISEID100_DMX
{
    unsigned int sample_rate;
    unsigned int frame_size;

    /* NOISEID100 library parameters */

    /* Smoothing filter decay time duration (Q7.N, s) */
    unsigned int filter_decay;
    /* Smoothing filter attack time duration (Q7.N, s) */
    unsigned int filter_attack;
    /* Hold timer in seconds. (Q12.N, s) */
    unsigned int timer_in_sec;
    /* Threshold used to classify noise category */
    int threshold_bin_comparison;
    /* Power ratio threshold needed to classify as NoiseID 0 */
    int id_0_threshold;
    /* Power ratio threshold needed to classify as NoiseID 1 */
    int id_1_threshold;

    /* NOISEID100 library statistics */

    /* Noise ID for current frame */
    int noiseID;
    /* Low-to-mid bin power ratio (Q3.N) - ((bin2 + bin3)/2) / ((bin4 + bin5)/2) */
    int low_to_mid_ratio;
    /* Current frame (instantaneous) noise ID */
    int cur_frame_noise_id;
    /* Pointer to bin powers array. Size NUM_BINS */
    int *bin_pwr;
    /* Flag to indicate license status */
    bool licensed;

    /* Private fields follow */
} NOISEID100_DMX;

#endif /* _NOISEID100_LIB_STRUCT_PUBLIC_H_ */
