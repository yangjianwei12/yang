/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup HCGR
 * \ingroup capabilities
 * \file  hcgr_cap.h
 *
 * Howling Control and Gain Recovery operator private header file.
 *
 */

#ifndef _HCGR_CAP_H_
#define _HCGR_CAP_H_

/******************************************************************************
Include Files
*/

#include "capabilities.h"

/* Imports scratch memory definitions */
#include "mem_utils/scratch_memory.h"

/* Math FFT interface */
#include "math/fft_twiddle_alloc_c_stubs.h"

#include "hcgr_proc.h"

/******************************************************************************
Private Constant Definitions
*/
/* Number of statistics reported by the capability */
#define HCGR_N_STAT                   (sizeof(HCGR_STATISTICS)/sizeof(ParamType))

/* Mask for the number of system modes */
#define HCGR_SYSMODE_MASK             0x3

/* Mask for override control word */
#define HCGR_OVERRIDE_MODE_MASK       (0xFFFF ^ HCGR_CONTROL_MODE_OVERRIDE)

/* Label metadata */
#define HCGR_METADATA_MIC_ID          1

#define HCGR_MIN_VALID_SINKS          ((1 << HCGR_TERMINAL))
#define HCGR_MIN_VALID_SOURCES        0

/* Terminals to skip data propagation in base class */
#define HCGR_TERMINAL_SKIP_MASK       0

#define HCGR_SUPPORTS_IN_PLACE        TRUE
#define HCGR_SUPPORTS_METADATA        TRUE
#define HCGR_DYNAMIC_BUFFERS_FALSE    FALSE

/* Capability minor version. Capability versions:
 *
 * Version | Description
 * --------|--------------------
 *   1.0   | Initial Release
 *   2.0   | Delta gain support, dual-path gain control support
 *   2.1   | Remove support for shared gain priority
 *   2.2   | Deprecate use of AHM_FRAME_SIZE parameter and link AHM period
 */
#define HCGR_CAP_VERSION_MINOR        2

#ifdef RUNNING_ON_KALSIM
#define HCGR_KALSIM_FLAG_MASK         0xF
/* Most significant nibble in AANC kalsim FLAGS is reserved for HCGR */
#define HCGR_KALSIM_FLAGS_SHIFT       28
#endif /* RUNNING_ON_KALSIM */

#define HCGR_MICRO_SEC_CONVERSION_FACTOR 1000000

/******************************************************************************
Public Enums
*/

typedef enum
{
    HCGR_PLAYBACK_TERMINAL,    /* 0 */
    HCGR_TERMINAL,             /* 1 */
    HCGR_PASSTHROUGH_TERMINAL, /* 2 */
    HCGR_MAX_TERMINALS         /* 3 */
} HCGR_TERMINALS;

/******************************************************************************
Public Type Declarations
*/

/* HCGR operator data */
typedef struct hcgr_exop
{
    /* Sample rate & cap id */
    unsigned sample_rate;
    CAP_ID cap_id;

    /* HCGR parameters */
    HCGR_PARAMETERS hcgr_cap_params;

    /* Mode control */
    unsigned cur_mode;
    unsigned ovr_control;
    unsigned host_mode;
    unsigned qact_mode;

    /* Flags */
    unsigned flags;
    unsigned previous_flags;

    /* Pointer to feature handle */
    void *f_handle;
    /* Track scratch registration */
    bool scratch_registered:8;
    /* Track FFT twiddle registration */
    bool twiddle_registered:8;

    /* HCGR */
    hcgr_t hcgr;

    /* ANC Hardware Manager operator ID */
    uint16 ahm_op_id;
} HCGR_OP_DATA;

/******************************************************************************
Private Function Definitions
*/

/* Standard Capability API handlers */
extern bool hcgr_create(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **resp_data);
extern bool hcgr_destroy(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **resp_data);

/* Standard Opmsg handlers */
extern bool hcgr_opmsg_set_control(OPERATOR_DATA *op_data,
                                  void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool hcgr_opmsg_get_status(OPERATOR_DATA *op_data,
                                  void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool hcgr_opmsg_link_ahm(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Custom opmsg handlers*/
/**
 * \brief  ANC Compander OPMSG handler to link HC Recovery target gain
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  resp_length      Length of response
 * \param  resp_data        Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool hcgr_opmsg_link_target_gain(OPERATOR_DATA *op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data);

/* Standard data processing function */
extern void hcgr_process_data(OPERATOR_DATA *op_data,
                              TOUCHED_TERMINALS *touched);

/* Capability hook functions to work with base_aud_cur_op */
bool hcgr_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);
bool hcgr_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);
bool hcgr_param_update_hook(OPERATOR_DATA *op_data);

#endif /* _HCGR_CAP_H_ */
