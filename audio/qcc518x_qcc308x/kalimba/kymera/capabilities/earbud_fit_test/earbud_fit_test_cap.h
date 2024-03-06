/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup Earbud Fit Test
 * \ingroup capabilities
 * \file  earbud_fit_test_cap.h
 * \ingroup Earbud Fit Test
 *
 * Earbud Fit Test operator private header file.
 *
 */

#ifndef _EARBUD_FIT_TEST_CAP_H_
#define _EARBUD_FIT_TEST_CAP_H_

/******************************************************************************
Include Files
*/

#include "capabilities.h"

/* Imports scratch memory definitions */
#include "mem_utils/scratch_memory.h"

/* Math FFT interface */
#include "math/fft_twiddle_alloc_c_stubs.h"

#include "earbud_fit_test.h"
#include "earbud_fit_test_gen_c.h"
#include "earbud_fit_test_defs.h"

/* dB60toLinearQ5(..) used in auto fit */
#include "common_conversions.h"

/* opmgr_operator_message(..) used in auto fit*/
#include "opmgr/opmgr.h"
#include "accmd_prim.h"
#include "kalimba_basic_op.h"


/******************************************************************************
Private Constant Definitions
*/
/* Number of statistics reported by the capability */
#define EFT_N_STAT               (sizeof(EARBUD_FIT_TEST_STATISTICS)/sizeof(ParamType))

/* Mask for the number of system modes */
#define EFT_SYSMODE_MASK         0x3

/* Mask for override control word */
#define EFT_OVERRIDE_MODE_MASK        (0xFFFF ^ EARBUD_FIT_TEST_CONTROL_MODE_OVERRIDE)

/* Label terminals */
#define EFT_PLAYBACK_TERMINAL_ID  0
#define EFT_MIC_INT_TERMINAL_ID   1

/* Label metadata channels */
#define EFT_METADATA_PLAYBACK_ID       0
#define EFT_METADATA_INT_ID            1
#define EFT_NUM_METADATA_CHANNELS      2

#define EFT_MAX_SINKS             2
#define EFT_MIN_VALID_SINKS      ((1 << EFT_PLAYBACK_TERMINAL_ID) | \
                                  (1 << EFT_MIC_INT_TERMINAL_ID))

/* Label in/out of ear states */
#define EFT_IN_EAR                     TRUE
#define EFT_OUT_EAR                    FALSE

/* Capability minor version
 * Version 1.1 includes the one-shot earbud characterization feature
 */
#define EARBUD_FIT_TEST_CAP_VERSION_MINOR    1

/* Timer parameter is Q12.N */
#define EFT_TIMER_PARAM_SHIFT    20

/* Event IDs */
#define EFT_EVENT_ID_FIT          0

/* Event payloads */
#define EFT_EVENT_PAYLOAD_BAD     0
#define EFT_EVENT_PAYLOAD_GOOD    1
#define EFT_EVENT_PAYLOAD_UNUSED  2
#define EFT_EVENT_PAYLOAD_CAPTURE_COMPLETE 0xFFFF

/* EFT runs on 4ms frames, so ms value to frames is divide by 4 */
#define EFT_MS_TO_FRAMES_SHIFT 2
/* Bin ratios are returned as 2x header + 2*16 words */
#define EFT_BINS_PER_SECTION 16
#define EFT_BINS_RESP_SIZE 3 + (2 * EFT_BINS_PER_SECTION)

/* AUTO FIT */
#define EFT_BIN_FREQ_SEPARATION_HZ   125        /* BIN_FREQ_SEPARATION in Hz */
#define EFT_EQU_OFFSET_BAND1_GAIN_DB 11         /* OFFSET of GAIN PARAMETERS in GEQ equalizer struct */
#define EFT_Q27_UNITY                134217728  /* 1.0 <-> 0dB in. Q5.27 */
#define EFT_AUTO_FIT_NUM_BANDS_MAX   6          /* max. num sub bands in auto fit estimation */
#define EFT_AUTO_FIT_THRSHLD_OFF     0
#define EFT_AUTO_FIT_BAND_POWER_MAX  1073741824 /* max. power measure = 2^30 */

/* Build to only test the messaging interface */
// #define EFT_MESSAGE_TEST

/******************************************************************************
Public Type Declarations
*/

/* Represent the state of an EFT event */
typedef enum
{
    EFT_EVENT_CLEAR,
    EFT_EVENT_DETECTED,
    EFT_EVENT_SENT
} EFT_EVENT_STATE;

/* Represent EFT event messaging states */
typedef struct _EFT_EVENT
{
    unsigned frame_counter;
    unsigned set_frames;
    EFT_EVENT_STATE running;
} EFT_EVENT;

typedef struct _EFT_CAPTURE
{
    bool start;
    unsigned duration_ms;
    unsigned duration_frames;
    unsigned frame_counter;
} EFT_CAPTURE;

typedef enum
{
    EFT_BIN_SOURCE_REF = 0,
    EFT_BIN_SOURCE_MIC,
    EFT_BIN_SOURCE_MAX
} EFT_BIN_SOURCE;

typedef enum
{
    EFT_BIN_SECTION_2K = 0,
    EFT_BIN_SECTION_4K,
    EFT_BIN_SECTION_6K,
    EFT_BIN_SECTION_8K,
    EFT_BIN_SECTION_MAX
} EFT_BIN_SECTION;


typedef enum
{
    EFT_AUTO_FIT_STATE_OFF = 0,
    EFT_AUTO_FIT_STATE_SWITCHED_ON = 1,
    EFT_AUTO_FIT_STATE_ON = 2,
    EFT_AUTO_FIT_STATE_SWITCHED_OFF = 3
} eft_auto_fit_state_t;

typedef struct {
    unsigned freq_hz;              /* upper cutoff frequency of sub band power measure. frequncy increases with sub band id */
    unsigned eq_gain_offs_lin;     /* gain offset of related sub band id */
    unsigned eq_gain_lin;          /* band specific gain (lin.) */
    unsigned eq_gain_lin_smoothed; /* band specific gain (lin.) after ramping */
    unsigned eq_gain_lin_prv;      /* band specific gain (lin.), previoius value */
    unsigned eq_gain_reduction;    /* gain reduction due to clipping */
    unsigned num_clippings;        /* clipping indication of sub band */
    unsigned low_sig_flag;         /* monitor: low signal indication of sub band */
    unsigned pow_ref_avg;          /* monitor: averaged ref power of sub band */
    unsigned pow_intmic_avg;       /* monitor: averaged IntMic power of sub band */
} eft_auto_fit_band_t;

typedef struct {
    unsigned equ_op_id;                    /* operator id from equalizer, applying gain update */
    unsigned capture_interval_ms;          /* time per capture interval in ms (capture period) */
    unsigned msgs_per_capture_interval;    /* number of gain update messages per capture interval */
    unsigned sensitivity_thrshld_lin;      /* sensitivity threshold */
    unsigned clipping_thrshld_lin;         /* clipping threshold */
    unsigned gain_smooth_tc_ms;            /* time constant applied to smooth of power estimates */
    unsigned num_bands;                    /* number of bands applied for auto fitting */
    bool enable:8;                         /* automatic fit adaptation applying continuous captures */
    bool freeze:8;                         /* freeze gain update in equ of automatic fit adaptation */
    bool is_ready_to_send_msg:8;           /* flag indicates readiness to send gain update messages to equ */
    unsigned band_start_freq_bin;          /* lowest considered frequency bin */
    eft_auto_fit_band_t band[EFT_AUTO_FIT_NUM_BANDS_MAX];
    eft_auto_fit_state_t state;            /* processing state of auto fit */
    unsigned cur_mode_bak;                 /* backup of current state */
    unsigned eq_gain_smooth_coeff;         /* coeff, used for gain ramping */
    unsigned eq_gain_smooth_1_minus_coeff; /* complementary coeff, used for gain ramping */
    int frame_cnt_msg;
} auto_fit_t;


/* Earbud Fit Test operator data */
typedef struct eft_exop
{
    /* Input buffers: internal mic, external mic */
    tCbuffer *inputs[EFT_MAX_SINKS];

    /* Metadata input buffers:  */
    tCbuffer *metadata_ip[EFT_NUM_METADATA_CHANNELS];

    tCbuffer *p_tmp_ref_ip;              /* Pointer to temp ref mic ip */
    tCbuffer *p_tmp_int_ip;              /* Pointer to temp int mic ip */

    /* Sample rate & cap id */
    unsigned sample_rate;
    CAP_ID cap_id;

    /* Earbud Fit Test parameters */
    EARBUD_FIT_TEST_PARAMETERS eft_cap_params;

    /* Mode control */
    unsigned cur_mode;
    unsigned ovr_control;
    unsigned host_mode;
    unsigned qact_mode;

    /* In/Our of ear status */
    bool in_out_status:8;

    /* Fit quality flag */
    bool fit_quality:8;

    /* Previous Fit quality flag */
    bool prev_fit_quality:8;

    /* Reinitialization */
    bool re_init_flag:8;

    /* Perform a fixed runtime capture */
    EFT_CAPTURE one_shot;

    /* continous capture for auto fit */
    auto_fit_t auto_fit;

    /* Standard CPS object */
    CPS_PARAM_DEF params_def;

    /* Fit detect event */
    EFT_EVENT fit_event_detect;

    AANC_AFB *p_afb_ref;                 /* Pointer to reference AFB */
    AANC_AFB *p_afb_int;                 /* Pointer to internal mic AFB */
    FIT100   *p_fit;                     /* Pointer to fit object */

    void *f_handle;                      /* Pointer to feature handle */

    bool scratch_registered:8;           /* Track scratch registration */
    bool twiddle_registered:8;           /* Track FFT twiddle registration */

} EFT_OP_DATA;

/******************************************************************************
Private Function Definitions
*/

/* Standard Capability API handlers */
extern bool eft_create(OPERATOR_DATA *op_data, void *message_data,
                             unsigned *response_id, void **resp_data);
extern bool eft_destroy(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **resp_data);
extern bool eft_start(OPERATOR_DATA *op_data, void *message_data,
                            unsigned *response_id, void **resp_data);
extern bool eft_reset(OPERATOR_DATA *op_data, void *message_data,
                            unsigned *response_id, void **resp_data);
extern bool eft_connect(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **resp_data);
extern bool eft_disconnect(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *response_id, void **resp_data);
extern bool eft_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *response_id, void **resp_data);
extern bool eft_get_sched_info(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *response_id, void **resp_data);

/* Standard Opmsg handlers */
extern bool eft_opmsg_set_control(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_get_params(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_get_defaults(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_set_params(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_get_status(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_set_ucid(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool eft_opmsg_get_ps_id(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Standard data processing function */
extern void eft_process_data(OPERATOR_DATA *op_data,
                                   TOUCHED_TERMINALS *touched);

/* Standard parameter function to handle persistence */
extern bool ups_params_eft(void* instance_data, PS_KEY_TYPE key,
                                 PERSISTENCE_RANK rank, uint16 length,
                                 unsigned* data, STATUS_KYMERA status,
                                 uint16 extra_status_info);

/* Custom capability handlers */

/* Start a bin power capture */
extern bool eft_opmsg_start_capture(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Retrieve bin powers */
extern bool eft_opmsg_get_bin_power(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Set op_id of equalizer for automatic fit adaptation */
extern bool eft_opmsg_auto_fit_set_equ_op_id(OPERATOR_DATA* op_data,
                                   void* message_data,
                                   unsigned* resp_length,
                                   OP_OPMSG_RSP_PAYLOAD** resp_data);

#endif /* _EARBUD_FIT_TEST_CAP_H_ */