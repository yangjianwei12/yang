/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup lib_private\aanc
 *
 * \file  hc100_struct_public.h
 * \ingroup lib_private\aanc
 *
 * HC100 library header file providing public data structures.
 *
 */
#ifndef _HC100_LIB_STRUCT_PUBLIC_H_
#define _HC100_LIB_STRUCT_PUBLIC_H_

/* Imports Kalimba type definitions */
#include "types.h"
#include "opmgr/opmgr_for_ops.h"

/******************************************************************************
Public Constant Definitions
*/

/******************************************************************************
Public Variable Definitions.

There provide a means to pass data and variables into and out of the HC100
library.
*/

typedef struct _HC100_DMX
{
    /* Peak-to-threshold power ratio */
    int ptpr_threshold;
    unsigned int ptpr_threshold_bexp;
    unsigned int ptpr_threshold_dc_bexp;

    /* Peak-to-average power ratio (shift) */
    unsigned int papr_threshold_shift;

    /* Peak-to-neighboring power ratio */
    int pnpr_threshold;

    /* Intra Frame Power Ratio */
    int ifpr_growth_scale;
    int bin1_trigger_detect_count;
    int bin1_frame_reset_count;

    unsigned int counter_regular;

    /* Amplitude counter table */
    int *counter_table;  /* Pointer to internal counter table */
    int counter_limit;   /* When tone counter reaches this value, howling is detected. */

    /* Disable detection under bin # (exclusively). So for a value of 3,
     * bins 0, 1 and 2 are not considered for HC detection. */
    int no_hc_below_bin_num;

    unsigned flags;            /* Flags set during data processing */
    bool licensed;             /* Flag to indicate license status */
    int tone_detected_flag;    /* Howling has been detected */
    int tone_counter_ptr;      /* Tone detection counter */
    unsigned max_bin;          /* Index of bin with max power */
    int peak_power;            /* Peak frame power */
    int average_power_thresh;  /* Average power adjusted by threshold */
    int neighbour_peak_thresh; /* Peak power adjusted by neighbour threshold */
    int low_neighbour_pwr;     /* Power in lower frequency neighbour bin */
    int high_neighbour_pwr;    /* Power in higher frequency neighbour bin */
    int prev_det_scaled;       /* Previous frame detection scaled by IFPR */

    int *pwr_ptr;
    int *ptpr_flags_ptr;
    int *pwr_avg_diff_ptr;
    int *detect_count;
} HC100_DMX;


#endif /* _HC100_LIB_STRUCT_PUBLIC_H_ */
