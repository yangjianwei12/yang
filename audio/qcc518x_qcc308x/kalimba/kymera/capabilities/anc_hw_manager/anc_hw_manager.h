/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_hw_manager.h
 * \defgroup anc_hw_manager
 *
 * \ingroup capabilities
 *
 * ANC HW Manager (AHM) operator public header file.
 *
 */

#ifndef _ANC_HW_MANAGER_H_
#define _ANC_HW_MANAGER_H_

/******************************************************************************
Include Files
*/

#include "stdfix.h"
#include "capabilities.h"
#include "macros.h"
#include "accmd_prim.h"
#include "opmgr/opmgr_op_client_interface.h"
#include "pl_timers/pl_timers.h"
#include "base_aud_cur_op.h"

#include "anc_hw_manager_common.h"
#include "anc_hw_manager_gain.h"
#include "anc_hw_manager_ramp.h"
#include "anc_hw_manager_event.h"
#include "anc_hw_manager_list.h"
#include "anc_hw_manager_filter.h"
#include "anc_hw_manager_gen_c.h"

/****************************************************************************
Public Variable Definitions
*/
/** The capability data structure for the ANC HW Manager */


extern const CAPABILITY_DATA anc_hw_manager_cap_data;

/******************************************************************************
Private Constant Definitions
*/

/* Capability minor version. Capability versions:
 *
 * Version | Description
 * --------|--------------------
 *   1.0   | Initial Release
 *   2.0   | Support for variable target gain values
 *   3.0   | Move to timer outside of the audio graph
 *   3.1   | Remove support for shared gain priority
 */
#define AHM_CAP_VERSION_MINOR           1

#define AHM_CONTROL_MODE_OVERRIDE	    0x2000
#define AHM_OVERRIDE_MODE_MASK          (0xFFFF ^ AHM_CONTROL_MODE_OVERRIDE)

/* Mask for sysmode SET_CONTROL value */
#define AHM_SYSMODE_MASK                0x7

#define AHM_MAX_SINKS                   3
#define AHM_MAX_SOURCES                 3
/* Allow for 0 terminals to start */
#define AHM_SINK_VALID_MASK             0x00
/* No requirement on output terminal connections */
#define AHM_SOURCE_VALID_MASK           0x00
/* Don't skip any terminals when transferring data */
#define AHM_TERMINAL_SKIP_MASK          0x00

#define AHM_SUPPORTS_METADATA           TRUE
#define AHM_SUPPORTS_IN_PLACE           TRUE
#define AHM_DYNAMIC_BUFFERS             TRUE

/* FF fine gain above MAX_THRESHOLD will automatically be halved and
 * corresponding coarse gain incremented.
 */
#define AHM_FF_FINE_MAX_THRESHOLD        128
/* FF fine gain below MIN_THRESHOLD will log an error message */
#define AHM_FF_FINE_MIN_THRESHOLD        64

/* Shift amount to convert Q12.20 to Q2.30*/
#define AHM_Q12_TO_Q2_LSHIFT_AMT         10

#define AHM_N_STAT                       (sizeof(ANC_HW_MANAGER_STATISTICS)/ \
                                          sizeof(ParamType))

#define AHM_MIN_TIMER_PERIOD_US          250
#define AHM_MAX_TIMER_PERIOD_US          20000
#define AHM_DEF_TIMER_PERIOD_US          250

#define AHM_MIN_TIMER_DECIMATION         1
#define AHM_MAX_TIMER_DECIMATION         100
#define AHM_DEF_TIMER_DECIMATION         4

#define AHM_DEF_FAST_RATE                1000000 / AHM_DEF_TIMER_PERIOD_US
#define AHM_DEF_SLOW_RATE                AHM_DEF_FAST_RATE / AHM_DEF_TIMER_DECIMATION

/* Dynamic filters: FF, FB, EC. Rx mix FFa is updated separately if in ambient
 * mode.
 */

#define NUM_COEFFS_IIR_NUMERATOR         9
#define NUM_COEFFS_IIR_DENOM             8
#define TOTAL_NUM_COEFFS_IIR             (NUM_COEFFS_IIR_NUMERATOR + NUM_COEFFS_IIR_DENOM)

#define AHM_ANC_CONTROL_ZCD_ENABLE_POS   ACCMD_ANC_CONTROL_FFGAIN_ZCD_EN_MASK
#define AHM_ANC_CONTROL_ZCD_DISABLE      (AHM_ANC_CONTROL_ZCD_ENABLE_POS << 16)
#define AHM_ANC_CONTROL_ZCD_ENABLE       ((AHM_ANC_CONTROL_ZCD_ENABLE_POS << 16) | \
                                          AHM_ANC_CONTROL_ZCD_ENABLE_POS)

#define AHM_RAMP_END_THRESH 0x00010000
/******************************************************************************
Public Type Declarations
*/
/* Gain interpretation types for FINE_GAIN_PARAM */
typedef enum
{
    AHM_GAIN_ABSOLUTE       = 0x00,
    AHM_GAIN_SHIFT_CURRENT  = 0x40,
    AHM_GAIN_SHIFT_STATIC   = 0x80,
    AHM_GAIN_SHIFT_MASK     = 0xC0
} GAIN_PARAM_TYPE;

/* Types of Trigger transitions for Mode (ANC HW filters) switching */
typedef enum
{
    AHM_TRIGGER_START=1,
    AHM_TRIGGER_SIMILAR=2,
    AHM_TRIGGER_DIFFERENT=3
} TRIGGER_PARAM_TYPE;

typedef enum
{
    AHM_RAMP_INIT,
    AHM_RAMP_DOWN,
    AHM_RAMP_UP
} RAMP_PARAM_TYPE;

/* Interpret the fine gain parameter value */
typedef struct _FINE_GAIN_PARAM
{
    /* 0-7: Absolute value */
    uint8 absolute;
    /* 8-15: Shift from static value */
    int8 shift_static;
    /* 16-23: Shift from current value*/
    int8 shift_current;
    /* 24-31: Gain type */
    GAIN_PARAM_TYPE gain_type:8;
} FINE_GAIN_PARAM;

typedef struct _AHM_EXOP
{
    CAP_ID cap_id;                              /* Capability ID */

    ANC_HW_MANAGER_PARAMETERS ahm_cap_params;   /* AHM parameters */

    unsigned cur_mode;                          /* Current mode */
    unsigned host_mode;                         /* Mode set by host */
    unsigned qact_mode;                         /* Mode set by QACT */
    unsigned ovr_control;                       /* Override control flags */

    unsigned sample_rate;                       /* Operator sample rate */

    AHM_GAIN_BANK *p_cur_gain;                  /* Current path gains */
    AHM_GAIN_BANK *p_cur_gain1;                 /* Current path gains */
    AHM_GAIN_BANK *p_prev_gain;                 /* Previous path gains */
    AHM_GAIN_BANK *p_prev_gain1;                /* Previous path gains */
    AHM_GAIN_BANK *p_static_gain;               /* Static path gains */
    AHM_GAIN_BANK *p_static_gain1;               /* Static path gains */
    AHM_GAIN_BANK *p_nominal_gain;              /* Nominal path gains */
    AHM_GAIN_BANK *p_nominal_gain1;             /* Nominal path gains */

    AHM_EVENT_MSG event_msg;                    /* Event message */

    /* Target makeup gain (Q2.N, log2 dB) */
    int target_makeup_gain;
    uint16 ff_fine_tgt_gain;                    /* FF fine gain target */
    uint16 fb_fine_tgt_gain;                    /* FB fine gain target */

    AHM_ANC_CONFIG config;                      /* ANC configuration */
    uint16 anc_clock_check_value;               /* ANC clock enable value */
    bool clock_status:8;                        /* ANC clock check status */
    bool in_out_status:8;                       /* In/Our of ear status */

    /* Disables ZCD for delta gain updates */
    bool disable_zcd:8;

    AHM_DELTA_RAMP fine_ramp[AHM_NUM_DYNAMIC_FILTERS]; /* Delta ramps */

    /* Pointer to shared delta fine gains */
    AHM_FINE_GAIN_NODE *p_fine_delta_head[AHM_NUM_DYNAMIC_FILTERS];
    bool delta_valid[AHM_NUM_DYNAMIC_FILTERS];  /* Delta gain valid */

    /* Pointers to shared FF fine gain values for the ramps */
    AHM_SHARED_FINE_GAIN *p_fine_delta_ramp[AHM_NUM_DYNAMIC_FILTERS];
    AHM_SHARED_FINE_GAIN *p_fine_nominal[AHM_NUM_DYNAMIC_FILTERS];
    AHM_SHARED_FINE_GAIN *p_fine_nominal1[AHM_NUM_DYNAMIC_FILTERS];

    /* AHM nominal gain values */
    AHM_SHARED_FINE_GAIN ahm_fine_nominal[AHM_NUM_DYNAMIC_FILTERS];
    AHM_SHARED_FINE_GAIN ahm_fine_nominal1[AHM_NUM_DYNAMIC_FILTERS];
    /* Cached value for nominal gain pointers */
    AHM_SHARED_FINE_GAIN client_fine_nominal[AHM_NUM_DYNAMIC_FILTERS];
    AHM_SHARED_FINE_GAIN client_fine_nominal1[AHM_NUM_DYNAMIC_FILTERS];
    bool ahm_nominal_client[AHM_NUM_DYNAMIC_FILTERS];
    bool ahm_nominal_client1[AHM_NUM_DYNAMIC_FILTERS];

    AHM_IIR_FILTER_BANK *p_iir_filter_inst1;     /* IIR filter Inst1 */
    AHM_IIR_FILTER_BANK *p_iir_filter_inst2;     /* IIR filter Inst2 */

    AHM_IIR_FILTER_BANK *p_prev_iir_filter_inst1;     /* Prev IIR filter Inst1 */
    AHM_IIR_FILTER_BANK *p_prev_iir_filter_inst2;     /* Prev IIR filter Inst2 */

    bool ramp_required[AHM_NUM_DYNAMIC_FILTERS]; /* ramp down/ramp up required for filter update */

    /* Coarse_gain_changed from previous value */
    bool coarse_gain_changed[AHM_NUM_DYNAMIC_FILTERS];

    /* dB difference between rxmix FF static gain and FF static gain (Q2.30, log2)*/
    int rxmix_gain_diff;

    unsigned samples_per_period;                /* Samples per task period */
    unsigned timer_period;                      /* Timer task period in us */
    tTimerId timer_id;                          /* Timer Task ID  */

    uint16 timer_decimation;                    /* Timer decimation factor */
    int16 timer_counter;                        /* Counter for decimation */
    unsigned fast_rate;                         /* Timer rate */
    unsigned slow_rate;                         /* Decimated timer rate */
    bool aamb_mode:8;                           /* 0: AANC mode, 1: AAMB mode */
    TRIGGER_PARAM_TYPE trigger_mode;            /* 0:Start, 1: Similar, 2:Different */
    RAMP_PARAM_TYPE ramp_status;                /* 0: Init,1: ramp down, 2 : ramp up */
    uint16 mode;                                /* Feed Forward/Hybrid mode */
    bool set_target_flag;                       /* 0: Does not enter into 
                                                   set_fine_target_gain/set_target_makeup_gain */
} AHM_OP_DATA;

/******************************************************************************
Private Function Definitions
*/

/* Standard Capability API handlers */
extern bool ahm_create(OPERATOR_DATA *op_data, void *message_data,
                       unsigned *response_id, void **resp_data);
extern bool ahm_destroy(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **resp_data);
extern bool ahm_start_hook(OPERATOR_DATA *op_data);
extern bool ahm_stop_hook(OPERATOR_DATA *op_data);

/* Standard Opmsg handlers */
extern bool ahm_opmsg_set_control(OPERATOR_DATA *op_data,
                                  void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_get_status(OPERATOR_DATA *op_data,
                                 void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_sample_rate(OPERATOR_DATA *op_data,
                                      void *message_data,
                                      unsigned *resp_length,
                                      OP_OPMSG_RSP_PAYLOAD **resp_data);
/* Custom Opmsg handlers */
extern bool ahm_opmsg_set_static_gains(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_get_gains(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_target_makeup_gain(OPERATOR_DATA *op_data,
                                             void *message_data,
                                             unsigned *resp_length,
                                             OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_fine_target_gain(OPERATOR_DATA *op_data,
                                           void *message_data,
                                           unsigned *resp_length,
                                           OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                          void *message_data,
                                          unsigned *resp_length,
                                          OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_free_shared_gain_ptr(OPERATOR_DATA *op_data,
                                           void *message_data,
                                           unsigned *resp_length,
                                           OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_timer_period(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_iir_filter_coeffs(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool ahm_opmsg_set_zcd_disable(OPERATOR_DATA *op_data,
                                      void *message_data,
                                      unsigned *resp_length,
                                      OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Standard data processing function */
extern void ahm_process_data(OPERATOR_DATA *op_data,
                             TOUCHED_TERMINALS *touched);

/* Timer task */
extern void ahm_timer_cb(void *p_data);
extern bool ahm_opmsg_set_iir_filter_coeffs(OPERATOR_DATA *op_data,
                                            void *message_data, unsigned *resp_length,
                                            OP_OPMSG_RSP_PAYLOAD **resp_data);

#endif /* _ANC_HW_MANAGER_H_ */
