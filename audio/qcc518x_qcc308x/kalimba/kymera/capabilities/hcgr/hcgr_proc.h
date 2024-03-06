/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \ingroup capabilities
 * \file  hcgr_proc.h
 *
 * Howling Control and Gain Recovery operator processing library header file.
 *
 */

#ifndef _HCGR_PROC_H_
#define _HCGR_PROC_H_

/******************************************************************************
Include Files
*/

#include "hcgr_gen_c.h"
#include "hcgr_defs.h"
#include "aanc_utils_public.h"
#include "base_aud_cur_op.h"            /* Shared audio curation functions */

/******************************************************************************
Private Constant Definitions
*/

/* Default HC recovery target gain */
#define HCGR_DEFAULT_TARGET_GAIN      128

#ifdef RUNNING_ON_KALSIM
#define HCGR_KALSIM_FLAG_MASK         0xF
/* Most significant nibble in AANC kalsim FLAGS is reserved for HCGR */
#define HCGR_KALSIM_FLAGS_SHIFT       28
#endif /* RUNNING_ON_KALSIM */

#define HCGR_DB_PER_LVL              -6
#define HCGR_FREQ_PER_BIN             125
#define HCGR_CLEAR_BIN                0
#define HCGR_CLEAR_FREQ              -1
#define HCGR_RESET_VALUE             -1

#define HCGR_DELTA_GAIN_PRECISION     24
#define HCGR_DIV_PRECISION            20
#define HCGR_DIV_TO_DELTA_SHIFT       (HCGR_DELTA_GAIN_PRECISION - HCGR_DIV_PRECISION)

/* Delta gain time constant for gain release is set to 1.0 as we want
 *    exponential recovery. The rate of recovery is tunable by parameters */
#define HCGR_GAIN_RELEASE_TC          0x7fffffff /* 1.0 in 1.N format */
/* HCGR is created with default AHM frame size (effective) for minimal
   processing loop. The actual frame size is updated when AHM is linked */
#define HCGR_DEFAULT_AHM_MINIMAL_PERIOD_SAMPLES 16

/******************************************************************************
Public Type Declarations
*/

/* HCGR Process data structure */
typedef struct hcgr
{
    int ff_hw_target_gain;                 /* Hardware target FF gain value */
    int fb_hw_target_gain;                 /* Hardware target FF gain value */
    AANC_AFB *p_afb;                       /* Pointer to AFB */
    HC100_DMX *p_hc;                       /* Pointer to HC data */
    uint8 *p_hc_dm1;                       /* Pointer to HC memory in DM1 */
    uint8 *p_hc_dm2;                       /* Pointer to HC memory in DM2 */
    bool recovery_active;                  /* Recovery is active */
    bool ff_recovery_active;               /* HCGR recovery is active on FF path */
    bool fb_recovery_active;               /* HCGR recovery is active on FB path */
    bool ff_release_active;                /* FF Gain release active after first mitigation */
    bool fb_release_active;                /* FB Gain release active after first mitigation */
    bool ff_slowest_recovery;              /* Slowest recovery set on successive recovery */
    bool fb_slowest_recovery;              /* Slowest recovery set on successive recovery */
    tCbuffer * p_in_buf;                   /* Pointer to input cBuffer */
    AHM_SHARED_FINE_GAIN *p_ff_fine_gain;  /* Pointer to FF gain to control */
    AHM_SHARED_FINE_GAIN *p_fb_fine_gain;  /* Pointer to FB gain to control */
    AHM_SHARED_FINE_GAIN *p_adrc_gain;     /* Pointer to ADRC shared fine gain */
    AHM_GAIN *p_ff_current_gain;           /* Pointer to FF or FB current gain */
    AHM_GAIN *p_ff_nominal_gain;           /* Pointer to FF or FB nominal gain */
    AHM_GAIN *p_fb_current_gain;           /* Pointer to FF or FB current gain */
    AHM_GAIN *p_fb_nominal_gain;           /* Pointer to FF or FB nominal gain */
    AHM_ANC_FILTER hcgr_filter_path;       /* Filter path used for gain control */
    int latch_max_bin;                     /* Max bin for a given howling event */
    int latch_bexp;                        /* Bexp for a given howling event */
    int latch_peak_power;                  /* Peak frame power */
    int latch_average_power_thresh;        /* Average power adjusted by threshold */
    int latch_neighbour_peak_thresh;       /* Peak power adjusted by neighbour threshold */
    int latch_low_neighbour_pwr;           /* Power in lower frequency neighbour bin */
    int latch_high_neighbour_pwr;          /* Power in higher frequency neighbour bin */
    int latch_prev_det_scaled;             /* Scaled previous frame detection power */

    int min_ff_delta_gain;                 /* Minimum delta gain to target on howling (8.N) */
    int min_fb_delta_gain;                 /* Minimum delta gain to target on howling (8.N) */
    int attack_tc;                         /* Gain attack time constant */
    unsigned ahm_minimal_period_samples;   /* AHM timer period (ms) for minimal run */
    int gain_recovery_rate;                /* Gain recovery rate per frame */
    unsigned int minimum_ff_gain;          /* Minimum FF fine gain on howling detect */
    unsigned int minimum_fb_gain;          /* Minimum FB fine gain on howling detect */
    unsigned int slow_recovery_thresh;     /* Slow recovery gain threshold (8.N) */
    EXT_OP_ID ext_op_id;                   /* OperatorID*/

} hcgr_t;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  Initializing the filter path.
 *
 * \param  p_hcgr           Pointer to the hcgr data.
 * \param  p_params         pointer to the hcgr parameters
 *
 * \return void
 */
extern void hcgr_init_filter_path(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params);

/**
 * \brief  Initializing the HCGR data object.
 *
 * \param  p_hcgr           Pointer to the hcgr data.
 * \param  p_params         pointer to the hcgr parameters
 * \param  f_handle         pointer to hcgr feature handle
 * \param  sample_rate      sampling rate
 *
 * \return void
 */
extern void hcgr_proc_intialize(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params,
                                void *f_handle, unsigned sample_rate);
/**
 * \brief   Create HCGR objects.
 *
 * \param  p_hcgr           Pointer to the hcgr data.
 * \param  f_handle         pointer to hcgr feature handle
 *
 * \return  boolean indicating success or failure.
 */
extern bool hcgr_proc_create(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params,
                             void **f_handle);

/**
 * \brief  Recovery process on FF & FB path.
 *
 * \param  p_hcgr           Pointer to the hcgr data.
 * \param  p_params         pointer to the hcgr parameters
 *
 * \return  boolean indicating success or failure.
 */
extern void hcgr_proc_recovery(hcgr_t *p_hcgr, HCGR_PARAMETERS *p_params);

#endif /* _HCGR_PROC_H_ */
