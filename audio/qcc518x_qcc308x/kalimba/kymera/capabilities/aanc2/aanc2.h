/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup aanc2
 * \file  aanc2.h
 * \ingroup capabilities
 *
 * AANC Controller (AANC2) operator public header file.
 *
 */

#ifndef _AANC2_H_
#define _AANC2_H_

/******************************************************************************
Include Files
*/

#include "capabilities.h"

#include "base_aud_cur_op.h"        /* Base op functions for audio curation */
#include "anc_hw_manager_gain.h"    /* Gain types */
#include "anc_hw_manager_common.h"  /* Common AHM types */

#include "aanc2_gen_c.h"            /* AANC2 parameters/statistics/modes */
#include "aanc2_defs.h"             /* AANC2 shared definitions */
#include "aanc2_proc.h"             /* AANC2 processing types & functions */
#include "aanc2_events.h"           /* AANC2 events types & functions */

/****************************************************************************
Public Constant Definitions
*/
/** The capability data structure for AANC */
extern const CAPABILITY_DATA aanc2_16k_cap_data;

/******************************************************************************
Private Constant Definitions
*/
/* Number of statistics reported by the capability */
#define AANC2_N_STAT                (sizeof(AANC2_STATISTICS)/sizeof(ParamType))

/* Mask for the number of system modes */
#define AANC2_SYSMODE_MASK          0x3

/* Label terminals */
#define AANC2_PLAYBACK_TERMINAL_ID  0
#define AANC2_FB_MON_TERMINAL_ID    1
#define AANC2_MIC_INT_TERMINAL_ID   2
#define AANC2_MIC_EXT_TERMINAL_ID   3

/* Maximum sinks/sources */
#define AANC2_MAX_SOURCES           4
#define AANC2_MAX_SINKS             4


/* Must have at least internal and external mics connected */
#define AANC2_MIN_VALID_SINKS       ((1 << AANC2_MIC_INT_TERMINAL_ID) | \
                                     (1 << AANC2_MIC_EXT_TERMINAL_ID))

/* If we exit early then transfer/consume data on all input terminals */
#define AANC2_TRANSFER_ALL_MASK     0
/* If not exiting early then skip mic terminals that are used by the
 * algorithms
 */
#define AANC2_SKIP_TERMINALS        AANC2_MIN_VALID_SINKS

/* Capability minor version. Capability versions:
 *
 * Version | Description
 * --------|--------------------
 *   1.0   | Initial Release
 *   1.1   | Remove support for shared gain priority
 */
#define AANC2_CAP_VERSION_MINOR     1

/* Base audio curation class configuration */
#define AANC2_SUPPORTS_IN_PLACE     TRUE
#define AANC2_SUPPORTS_METADATA     TRUE
#define AANC2_DYNAMIC_BUFFERS       FALSE

/* Flag bitfields for model loading indication */
#define AANC2_FLAGS_PLANT_MODEL_LOADED     0x00020000
#define AANC2_FLAGS_CONTROL_0_MODEL_LOADED 0x00040000
#define AANC2_FLAGS_CONTROL_1_MODEL_LOADED 0x00080000

#define AANC2_DEFAULT_INIT_VALUE    128

/******************************************************************************
Public Type Declarations
*/

/* AANC operator data */
typedef struct aanc2_exop
{
    unsigned sample_rate;               /* Sample rate (fixed) */
    CAP_ID cap_id;                      /* Capability ID */

    AANC2_PARAMETERS aanc2_cap_params;  /* AANC2 parameters */

    unsigned cur_mode;                  /* Operator mode */
    unsigned host_mode;                 /* Operator mode requested by host */
    unsigned qact_mode;                 /* Operator mode requested by QACT */
    unsigned ovr_control;               /* QACT override indicator */

    unsigned cap_flags;                 /* Capability level flags */

    uint16 filter_config;               /* FxLMS filter configuration */
    bool disable_events:8;              /* Event disable flag */

    AANC2_PROC ag;                      /* Algorithm processing struct */

    AANC2_EVENT gain_event;             /* Gain stuck event */
    AANC2_EVENT ed_event;               /* ED stuck event */
    AANC2_EVENT quiet_event_detect;     /* Quiet condition detection event */
    AANC2_EVENT quiet_event_clear;      /* Quiet condition clear event */
    AANC2_EVENT clip_event;             /* Clipping stuck event */
    AANC2_EVENT sat_event;              /* Saturation stuck event */
    AANC2_EVENT self_talk_event;        /* Self-talk stuck event */
    AANC2_EVENT spl_event;              /* SPL above threshold event */

    bool quiet_condition;               /* Quiet condition flag */

    /* Shared memory for updating FF gain */
    AHM_SHARED_FINE_GAIN *p_ff_fine_gain;
    AHM_SHARED_FINE_GAIN *p_ff_fine_gain1;
    uint16 ahm_op_id;                   /* AHM operator ID */
    AHM_GAIN_BANK *p_static_gain;       /* Static gain */
    AHM_GAIN_BANK *p_static_gain1;      /* Static gain (ANC inst 1) */
    AHM_GAIN_BANK *p_cur_gain;          /* Nominal gain */
    AHM_GAIN_BANK *p_cur_gain1;         /* Nominal gain */

} AANC2_OP_DATA;

/******************************************************************************
Public Function Definitions
*/

/**
 * \brief  AANC2 operator create function.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_create(OPERATOR_DATA *op_data,
                         void *message_data,
                         unsigned *response_id,
                         void **resp_data);

/**
 * \brief  AANC2 operator destroy function.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_destroy(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *response_id,
                          void **resp_data);

/**
 * \brief  AANC2 process data function.
 *
 * \param  op_data          Pointer to operator data
 * \param  touched          Pointer to updated terminals
 *
 * \return  NONE
 */
extern void aanc2_process_data(OPERATOR_DATA *op_data,
                               TOUCHED_TERMINALS *touched);

/**
 * \brief  Connection hook for additional action(s) at start.
 *
 * \param  op_data          Pointer to the operator data
 *
 * \return  boolean indicating success or failure.
 */
bool aanc2_start_hook(OPERATOR_DATA *op_data);

/**
 * \brief  Connection hook for caching terminal pointers from the class data.
 *
 * \param  op_data          Pointer to the operator data
 * \param  terminal_id      Terminal ID for the connection
 *
 * \return  boolean indicating success or failure.
 */
bool aanc2_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);

/**
 * \brief  Disonnection hook for caching terminal pointers from the class data.
 *
 * \param  op_data          Pointer to the operator data
 * \param  terminal_id      Terminal ID for the connection
 *
 * \return  boolean indicating success or failure.
 */
bool aanc2_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id);

/**
 * \brief  AANC2 SET_CONTROL OPMSG handler for overriding operator behavior.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_opmsg_set_control(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data);
/**
 * \brief  AANC2 GET_STATUS OPMSG handler for reporting statistics.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_opmsg_get_status(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data);
/**
 * \brief  AANC2 Link AHM handler for linking to AHM gain
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_opmsg_link_ahm(OPERATOR_DATA *op_data,
                                 void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data);
/**
 * \brief  AANC2 Get shared gain message handler.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 */
extern bool aanc2_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                            void *message_data,
                                            unsigned *resp_length,
                                            OP_OPMSG_RSP_PAYLOAD **resp_data);

/****************************************************************************
Custom opmsg handlers
*/

/**
 * \brief  AANC2 OPMSG handler to set the FxLMS plant model
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool aanc2_opmsg_set_plant_model(OPERATOR_DATA *op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data);

/**
 * \brief  AANC2 OPMSG handler to set the FxLMS control model
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool aanc2_opmsg_set_control_model(OPERATOR_DATA *op_data,
                                          void *message_data,
                                          unsigned *resp_length,
                                          OP_OPMSG_RSP_PAYLOAD **resp_data);
/**
 * \brief  AANC2 OPMSG handler to get AANC adaptive gain.
 *
 * \param  op_data          Pointer to operator data
 * \param  message_data     Pointer to message data
 * \param  response_id      Pointer to response ID
 * \param  response data    Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
extern bool aanc2_opmsg_get_adaptive_gain(OPERATOR_DATA *op_data,
                                          void *message_data,
                                          unsigned *resp_length,
                                          OP_OPMSG_RSP_PAYLOAD **resp_data);

#endif /* _AANC2_H_ */
