/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  aanc2_clipping.h
 * \ingroup aanc2
 *
 * AANC Controller (AANC2) clipping public header file.
 *
 */

#ifndef _AANC2_CLIPPING_H_
#define _AANC2_CLIPPING_H_

/******************************************************************************
Include Files
*/

#include "types.h"                      /* Kalimba type definitions */
#include "buffer/cbuffer_c.h"           /* Cbuffer management functions */

/* Imports AANC2 parameter/statistic definitions. */
#include "aanc2_defs.h"                 /* Shared AANC2 defines */
#include "aanc2_gen_c.h"                /* AANC2 parameters/statistics/modes */

/******************************************************************************
Public Constant Definitions
*/

/******************************************************************************
Public Type Declarations
*/

/* Clipping detection and signal peak calculation */
typedef struct _AANC2_CLIP_DETECT
{
    tCbuffer *p_data;     /* Pointer to the data buffer to process */
    unsigned peak_value;  /* Current peak value of the signal */
    unsigned threshold;   /* Clipping detection threshold */
    uint16 duration;      /* Duration of the clip detect counter */
    uint16 counter;       /* Clip detect counter value */
    bool frame_detect:8;  /* Clip detected in a given frame */
    bool disabled:8;      /* Clip detection enable/disable */
    bool detected:8;      /* Clip detected flag (held by the counter) */
} AANC2_CLIP_DETECT;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Initialize a clipping detection structure
 *
 * \param  p_clip  Pointer to the clip data object
 * \param  p_data  Pointer to the data cbuffer
 * \param  threshold  Threshold for clipping detection
 * \param  Duration  Duration for the clipping hold timer
 * \param  Disabled  Disable control for the detector
 *
 * \return Boolean indicating success or failure
 *
 * The result of clipping detection and peak values are stored in the data
 * structure.
 *
 */
extern bool aanc2_clipping_initialize(AANC2_CLIP_DETECT *p_clip,
                                      tCbuffer *p_data,
                                      unsigned threshold,
                                      unsigned duration,
                                      bool disabled);

/**
 * \brief  Process a clip detection.
 *
 * \param  p_clip  Pointer to the clip struct.
 *
 * This monitors the frame detection and allows a counter to hold a detection
 * for a given duration (frames).
 */
extern void aanc2_clipping_process_detection(AANC2_CLIP_DETECT *p_clip);

/**
 * \brief  ASM function to do clipping and peak detection.
 *
 * \param  p_dm1    Pointer to an AANC2_CLIP_DETECT object whose data is in DM1
 * \param  p_dm2    Pointer to an AANC2_CLIP_DETECT object whose data is in DM2
 * \param  samples  Number of samples to process for detection
 *
 * \return Boolean indicating success or failure
 *
 * The result of clipping detection and peak values are stored in the data
 * structure.
 *
 */
extern bool aanc2_clipping_peak_detect_dual(AANC2_CLIP_DETECT *p_dm1,
                                            AANC2_CLIP_DETECT *p_dm2,
                                            unsigned samples);

/**
 * \brief  ASM function to do clipping and peak detection.
 *
 * \param  p_dm1    Pointer to an AANC2_CLIP_DETECT object whose data is in DM1
 * \param  samples  Number of samples to process for detection
 *
 * \return Boolean indicating success or failure
 *
 * The result of clipping detection and peak values are stored in the data
 * structure.
 *
 */
extern bool aanc2_clipping_peak_detect_single(AANC2_CLIP_DETECT *p_dm1,
                                              unsigned samples);
#endif /* _AANC2_CLIPPING_H_*/