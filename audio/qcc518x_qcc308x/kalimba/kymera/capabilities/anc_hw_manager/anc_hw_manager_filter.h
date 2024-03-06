/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \file  anc_hw_manager_filter.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) filter definitions.
 *
 */

#ifndef _ANC_HW_MANAGER_FILTER_H_
#define _ANC_HW_MANAGER_FILTER_H_

/******************************************************************************
 Include Files
 */

#include "opmgr/opmgr_for_ops.h"
#include "opmsg_prim.h"
#include "stream/stream_for_anc.h"
#include "anc_hw_manager_common.h"
#include "macros.h"

#define DENOM_COEFFS_START_OFFSET 4
#define NUM_COEFFS_IIR_NUMERATOR 9
#define NUM_COEFFS_IIR_DENOM 8
#define TOTAL_NUM_COEFFS_IIR (NUM_COEFFS_IIR_NUMERATOR + NUM_COEFFS_IIR_DENOM)

/* Macro for interpreting the set_filter messages */
#define OPMSG_AHM_SET_COEFFS32(opmsg_ahm_set_filter_msg_ptr, index)  \
    (uint32)(((opmsg_ahm_set_filter_msg_ptr)->data[index] << 16 ) |  (((opmsg_ahm_set_filter_msg_ptr)->data[index+1])))

/* Represent ANC HW Filter Coefficients */
typedef struct _AHM_IIR_FILTER {
    int p_num[NUM_COEFFS_IIR_NUMERATOR];  /* Pointer to numerator coefficient array */
    int p_den[NUM_COEFFS_IIR_DENOM];      /* Pointer to denominator coefficient array */
}AHM_IIR_FILTER;

/* Represent all the Filter Banks in ANC HW */
typedef struct _AHM_IIR_FILTER_BANK {
    AHM_IIR_FILTER ff; /* FF IIR filter */
    AHM_IIR_FILTER fb; /* FB IIR filter */
    AHM_IIR_FILTER ec; /* EC IIR filter */
}AHM_IIR_FILTER_BANK;

/*
typedef struct _AHM_IIR_FILTER_CONFIG {
	 bool ramp_required[AHM_NUM_DYNAMIC_FILTERS];
} AHM_IIR_FILTER_CONFIG;
*/

/* OPMSG_AHM_FILTER_COEFFS_MSG represents the data that is received from a
 * set iir coeff message. Leveraged from OPMSG_AANC_SET_MODEL_MSG
 */
typedef struct _OPMSG_AHM_FILTER_COEFFS_MSG {
    OPMSG_HEADER header; /* OPMSG header */
    unsigned data[]; /* message data */
}OPMSG_AHM_FILTER_COEFFS_MSG;
/* AHM operator data */
/******************************************************************************
 Private Function Definitions
 */


void ahm_update_filter_coeffs(AHM_IIR_FILTER_BANK *p_prev_iir, AHM_IIR_FILTER_BANK *p_iir, bool *cfg,
                              OPMSG_AHM_FILTER_COEFFS_MSG *p_msg,
                              uint16 mode, EXT_OP_ID ext_op_id);
bool check_filters_equal(uint32 *prev, uint32 *cur, uint32 length);
#endif /* _ANC_HW_MANAGER_FILTER_H_ */
