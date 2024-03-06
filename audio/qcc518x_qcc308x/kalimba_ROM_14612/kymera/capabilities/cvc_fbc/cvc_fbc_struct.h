/*******************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
*******************************************************************************/
/**
 * \file  cvc_fbc_struct.h
 * \ingroup capabilities
 *
 *  cVc FBC apability local header
*/

#ifndef CVC_FBC_STRUCT_H
#define CVC_FBC_STRUCT_H

#include "capabilities.h"
#include "op_msg_utilities.h"
#include "cvc_fbc_gen_c.h"
#include "fbc_library_c.h"

/* port mask */
#define CVC_FBC_TERMINAL_REFIN_SINK             0
#define CVC_FBC_TERMINAL_MICIN0_SINK            1
#define CVC_FBC_TERMINAL_ERROUT0_SOURCE         0
#if !defined(CVC_ATSA)
    #define CVC_FBC_TERMINAL_REFOUT_SOURCE      8
    #define TOUCHED_SOURCE_REF   TOUCHED_SOURCE_8
#else
    #define CVC_FBC_TERMINAL_REFOUT_SOURCE      1
    #define TOUCHED_SOURCE_REF   TOUCHED_SOURCE_1
#endif

/* The maximum number of channels supported by the capability */
#define BLOCK_BASED
#ifdef BLOCK_BASED
    #define CVC_FBC_DEFAULT_BLOCK_SIZE (120)
#else
    #define CVC_FBC_DEFAULT_BLOCK_SIZE (1)
#endif
#define CVC_FBC_MAX_IO_PAIRS    (1)

#define CVC_FBC_FRAME_SIZE          (120)

#define CVC_FBC_PORT_BUFFER_SIZE    (CVC_FBC_FRAME_SIZE * 2)
#define CVC_FBC_REFDELAY_EXTRA_SIZE (MAX(CVC_FBC_FRAME_SIZE/2, (CVC_FBC_REFDLY_MAX - (CVC_FBC_PORT_BUFFER_SIZE-CVC_FBC_DEFAULT_BLOCK_SIZE))))

// seconds in Q7.25
#define MAX_FBC_FILTERN_TAIL_S      (FRACTIONAL(CVC_FBC_FILTER_LENGTH_MAX/(1<<6)))

#define CVC_FBC_CVC_FBC_VERSION_MINOR (4)

/****************************************************************************
Public Type Definitions
*/

typedef struct cvc_fbc_exop{
    /* list of channels: ref, input, output */
    tCbuffer *ip_buffer_ref;
    tCbuffer *ip_buffer_mic;
    tCbuffer *op_buffer_ref; // optional
    tCbuffer *op_buffer_mic;
    tCbuffer *op_buffer_mic_metadata;

    /* FBC data object */
    fbc_data *data_obj;

    /* control */
    int16    ref_delay;
    uint16   ovr_control;
    uint8    host_mode;
    uint8    obpm_mode;
    uint8    cur_mode;
    uint8    ReInitFlag;
    int8     lib_status;

    /* CPS */
    CPS_PARAM_DEF parms_def;
    CVC_FBC_PARAMETERS cur_params;                      /**< Pointer to cur params  */
} CVC_FBC_OP_DATA;


/** processing function */
bool cvc_fbc_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool cvc_fbc_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool cvc_fbc_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool cvc_fbc_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool cvc_fbc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
bool cvc_fbc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

#endif /* CVC_FBC_STRUCT_H */
