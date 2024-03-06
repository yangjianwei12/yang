/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_rcv.c
 * \ingroup  operators
 *
 *  SCO send operator
 *
 */

/****************************************************************************
Include Files
*/

#include "sco_nb_private.h"
#include "sco_rcv_gen_c.h"
#include "capabilities.h"
#include "mem_utils/scratch_memory.h"

#ifdef SCO_RCV_DEBUG_FRAME
#include <stdio.h>
#endif


/****************************************************************************
Private Constant Definitions
*/

/* sizeof(SCO_RCV_PARAMETERS) in words */
#define SCO_RCV_NUM_PARAMETERS      (sizeof(SCO_RCV_PARAMETERS) >> LOG2_ADDR_PER_WORD)

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SCO_RCV_CAP_ID CAP_ID_DOWNLOAD_SCO_RCV
#else
#define SCO_RCV_CAP_ID CAP_ID_SCO_RCV
#endif

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Constant Declarations
*/
/** The SCO receive capability function handler table */
const handler_lookup_struct sco_rcv_handler_table =
{
    sco_rcv_create,           /* OPCMD_CREATE */
    sco_rcv_destroy,          /* OPCMD_DESTROY */
    sco_rcv_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    sco_rcv_reset,            /* OPCMD_RESET */
    sco_rcv_connect,          /* OPCMD_CONNECT */
    sco_rcv_disconnect,       /* OPCMD_DISCONNECT */
    sco_rcv_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    sco_rcv_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry sco_rcv_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               sco_rcv_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              sco_rcv_opmsg_disable_fadeout},
#ifdef INSTALL_PLC100
    {OPMSG_SCO_RCV_ID_FORCE_PLC_OFF,               sco_rcv_opmsg_force_plc_off},
#endif /* INSTALL_PLC100 */
    {OPMSG_SCO_RCV_ID_FRAME_COUNTS,                sco_rcv_opmsg_frame_counts},
    {OPMSG_COMMON_ID_SET_CONTROL,                  sco_rcv_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   sco_rcv_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 sco_rcv_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   sco_rcv_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   sco_rcv_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,              sco_common_rcv_opmsg_set_buffer_size},
    {OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE,     sco_common_rcv_opmsg_set_terminal_buffer_size},
    {OPMSG_COMMON_SET_TTP_LATENCY,                 sco_common_rcv_opmsg_set_ttp_latency},
    {0, NULL}};

const CAPABILITY_DATA sco_rcv_cap_data =
{
    SCO_RCV_CAP_ID ,            /* Capability ID */
    SCO_RCV_NB_VERSION_MAJOR, SCO_RCV_NB_VERSION_MINOR,  /* Version information - hi and lo parts */
    1, 1,                           /* Max number of sinks/inputs and sources/outputs */
    &sco_rcv_handler_table,         /* Pointer to message handler function table */
    sco_rcv_opmsg_handler_table,    /* Pointer to operator message handler function table */
    sco_rcv_process_data,           /* Pointer to data processing function */
    0,                              /* Reserved */
    sizeof(SCO_NB_DEC_OP_DATA)         /* Size of capability-specific per-instance data */
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SCO_RCV, SCO_NB_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SCO_RCV, SCO_NB_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Public Function Declarations
*/

/****************************************************************************
Private Function Definitions
*/
static inline SCO_NB_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_NB_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/* ******************************* Helper functions ************************************ */

/* Free the memory allocated for sco_nb */
static void sco_nb_free_mem(OPERATOR_DATA *op_data)
{
    SCO_NB_DEC_OP_DATA *x_data = get_instance_data(op_data);

#ifdef INSTALL_PLC100
    sco_plc100_free_data(x_data->plc100_struc);
    x_data->plc100_struc = NULL;
#endif /* INSTALL_PLC100 */
}

static void sco_nb_reset_working_data(OPERATOR_DATA *op_data)
{
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);

    /* now initialise specific working data */
    sco_common_rcv_reset_working_data(&sco_rcv->sco_rcv_op_data);
#ifdef INSTALL_PLC100
    sco_plc100_initialize(sco_rcv->plc100_struc);
#endif /* INSTALL_PLC100 */
}
/* ********************************** API functions ************************************* */

bool sco_rcv_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_NB_DEC_OP_DATA* sco_rcv = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_common = &sco_rcv->sco_rcv_op_data;

    sco_pio_init_pio_dec();

    sco_common->input_buffer_size = SCO_RCV_INPUT_BUFFER_SIZE;
    sco_common->output_buffer_size = SCO_RCV_OUTPUT_BUFFER_SIZE;

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

#ifdef INSTALL_PLC100
    /* operator buffer size - using the same default */
    if(!cpsInitParameters(&sco_common->parms_def,
                          (unsigned*)SCO_RCV_GetDefaults(base_op_get_cap_id(op_data)),
                          (unsigned*)&sco_rcv->force_plc_off,
                          sizeof(SCO_RCV_PARAMETERS)))
    {
        /* Change the already allocated response to command failed.
         * No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* setup the PLC structure, zero initialise it so that we can detect if an
     * allocation has happened when we unwind */
    PLC100_STRUC* plcstruct = sco_plc100_create(SP_BUF_LEN_NB,
                                                OLA_LEN_NB,
                                                PLC100_NB_CONSTANTS);
    if (plcstruct == NULL)
    {
        /* Change the already allocated response to command failed.
         * No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    sco_rcv->plc100_struc = plcstruct;
#endif /* INSTALL_PLC100 */

    /* Initialise some of the operator data that is common between NB and
     * WB receive. It can only be called after the PLC structure is allocated
     * (if PLC present in build). */
    sco_common_rcv_initialise(sco_common);

    /* initialise specific data (was allocated and pointer to it filled by
     * OpMgr. */
    sco_nb_reset_working_data(op_data);

    /* set sample rate, needed for approximate TTP generation. */
    sco_common->sample_rate = 8000;

    /* initialize scratch memory for CVSD decoder*/
#ifdef CVSD_CODEC_SOFTWARE
    if (scratch_register())
    {
        if (scratch_reserve(SCRATCH_SIZE_WORDS, MALLOC_PREFERENCE_NONE))
        {
            /* All went well. Done. */
            return TRUE;
        }
        /* It didn't go well. Try to fail nicely. */
        scratch_deregister();
    }
    /* It didn't go well. Try to fail nicely. */
    sco_nb_free_mem(op_data);
    /* Change the already allocated response to command failed.
     * No extra error info. */
    base_op_change_response_status(response_data, STATUS_CMD_FAILED);
    return TRUE;
#else
    return TRUE;
#endif /* CVSD_CODEC_SOFTWARE */
}


bool sco_rcv_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_NB_DEC_OP_DATA* x_data;
    x_data = get_instance_data(op_data);
    L2_DBG_MSG2("t = %08x: Destroying sco_rcv operator %04x",
                time_get_time(), base_op_get_ext_op_id(op_data));

    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy_lite(op_data, response_data))
    {
        return(FALSE);
    }

    /* Delete any common data. */
    sco_common_rcv_destroy(&x_data->sco_rcv_op_data);

    sco_nb_free_mem(op_data);

#ifdef CVSD_CODEC_SOFTWARE
    scratch_deregister();
#endif
    return TRUE;
}


bool sco_rcv_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    sco_nb_reset_working_data(op_data);

    return TRUE;
}


bool sco_rcv_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_common = &sco_rcv->sco_rcv_op_data;

    /* TODO - flush input buffer and zero output buffer contents? or endpoint will do it */

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        return TRUE;
    }

    /* Sanity check for buffers being connected.
     * We can't do much useful without */
    if (sco_common->buffers.ip_buffer == NULL || sco_common->buffers.op_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    L2_DBG_MSG2("t=%08x: sco_rcv operator %04x started", time_get_time(), base_op_get_ext_op_id(op_data));

    return TRUE;
}


bool sco_rcv_connect(OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **response_data)
{
    SCO_NB_DEC_OP_DATA *x_data = get_instance_data(op_data);
    bool status;
    SCO_TERMINAL_BUFFERS *terminal_buffers;
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);

    terminal_buffers = &(x_data->sco_rcv_op_data.buffers);

    status = sco_common_connect(op_data, message_data,
                                response_id, response_data,
                                terminal_buffers,
                                NULL);

#ifdef INSTALL_PLC100
    /* if connecting source terminal and previous steps were OK,
     * hook up output buffer beauty with the PLC beast */
    if(status && (terminal_id == OUTPUT_TERMINAL_ID))
    {
        x_data->plc100_struc->output = terminal_buffers->op_buffer;
    }
#endif /* INSTALL_PLC100 */

    return status;
}


bool sco_rcv_disconnect(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    SCO_NB_DEC_OP_DATA *x_data = get_instance_data(op_data);
    bool status;
    SCO_TERMINAL_BUFFERS *terminal_buffers;
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);

    terminal_buffers = &(x_data->sco_rcv_op_data.buffers);

    status = sco_common_disconnect(op_data, message_data,
                                   response_id, response_data,
                                   terminal_buffers, NULL);

#ifdef INSTALL_PLC100
    /* if disconnecting source terminal and previous steps were OK,
     * disconnect output buffer from PLC */
    if(status && (terminal_id == OUTPUT_TERMINAL_ID))
    {
        x_data->plc100_struc->output = NULL;
    }
#endif /* INSTALL_PLC100 */

    return status;
}


bool sco_rcv_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                            unsigned *response_id, void **response_data)
{
    unsigned sco_in_size = SCO_RCV_INPUT_BUFFER_SIZE;
    unsigned sco_out_size = SCO_RCV_OUTPUT_BUFFER_SIZE;
    SCO_NB_DEC_OP_DATA *sco_nb_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(sco_nb_data->sco_rcv_op_data);
    sco_in_size = MAX(sco_in_size, sco_data->input_buffer_size);
    sco_out_size = MAX(sco_out_size, sco_data->output_buffer_size);

    if(!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Currently these have the same value but this isn't guaranteed */
    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = sco_in_size;
        L4_DBG_MSG2("sco_nb_dec_buffer_details (capID=%d)  input buffer size=%d \n", base_op_get_cap_id(op_data), sco_in_size);
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = sco_out_size;
        L4_DBG_MSG2("sco_nb_dec_buffer_details (capID=%d)  output buffer size=%d \n", base_op_get_cap_id(op_data), sco_out_size);
    }

    /* supports metadata in both side  */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = 0;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;

    return TRUE;
}

bool sco_rcv_get_data_format(OPERATOR_DATA *op_data, void *message_data,
                             unsigned *response_id, void **response_data)
{
#ifdef CVSD_CODEC_SOFTWARE
    return sco_common_get_data_format(op_data, message_data,
                                      response_id, response_data,
                                      AUDIO_DATA_FORMAT_16_BIT,
                                      AUDIO_DATA_FORMAT_FIXP);
#else
    return sco_common_get_data_format(op_data, message_data,
                                      response_id, response_data,
                                      AUDIO_DATA_FORMAT_FIXP,
                                      AUDIO_DATA_FORMAT_FIXP);
#endif /* CVSD_CODEC_SOFTWARE */
}


/* ************************************* Data processing-related functions and wrappers **********************************/


void sco_rcv_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    unsigned status;
    SCO_NB_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &x_data->sco_rcv_op_data;
    tCbuffer temp_cbuf;

    patch_fn(sco_rcv_process_data);

    sco_pio_set_pio_dec();

    SCO_DBG_MSG("sco_rcv_process_data\n");
    /* We can't have started in this state, so if we lose a buffer carry on
     * and hope that it comes back or we are stopped. If not then the error
     * (lack of data) should propagate to something that cares. */
    if (sco_data->buffers.ip_buffer == NULL || sco_data->buffers.op_buffer == NULL)
    {
        sco_pio_clr_pio_dec();
        return;
    }

    temp_cbuf = *(sco_data->buffers.op_buffer);

#ifdef CVSD_CODEC_SOFTWARE
    x_data->ptScratch = (int*)scratch_commit(SCRATCH_SIZE_WORDS, MALLOC_PREFERENCE_NONE);
#endif

    status = sco_rcv_processing(op_data);

#ifdef CVSD_CODEC_SOFTWARE
    scratch_free();
#endif


    /* Is fadeout enabled? if yes, do it on the current output data, if processing has actually produced output */
    if( (sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE) && status)
    {
        /* the wrapper below takes output Cbuffer and fadeout params, and we'll use the current packet size in words */
        if (mono_cbuffer_fadeout(&temp_cbuf,
                                 x_data->sco_rcv_output_samples,
                                 &(sco_data->fadeout_parameters)))
        {
            common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
        }
    }

    touched->sources = status;

    sco_pio_clr_pio_dec();
}


/* **************************** Operator message handlers ******************************** */

bool sco_rcv_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &x_data->sco_rcv_op_data;
    common_set_fadeout_state(&sco_data->fadeout_parameters, RUNNING_STATE);

    return TRUE;
}


bool sco_rcv_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &x_data->sco_rcv_op_data;
    common_set_fadeout_state(&sco_data->fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}


#ifdef INSTALL_PLC100
bool sco_rcv_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA* x_data = get_instance_data(op_data);
    if(OPMSG_FIELD_GET(message_data, OPMSG_SCO_COMMON_RCV_FORCE_PLC_OFF, FORCE_PLC_OFF) != 0)
    {
        /* set force PLC off */
        x_data->force_plc_off = 1;
    }
    else
    {
        /* unset force PLC off */
        x_data->force_plc_off = 0;
    }
   return TRUE;
}
#endif /* INSTALL_PLC100 */


bool sco_rcv_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_frame_counts_helper(&x_data->sco_rcv_op_data,
                                              message_data,
                                              resp_length,
                                              resp_data);
}


bool sco_rcv_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message.
     * Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}


bool sco_rcv_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
#ifdef INSTALL_PLC100
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_rcv->sco_rcv_op_data;

   return cpsGetParameterMsgHandler(&sco_data->parms_def, message_data,
                                    resp_length,resp_data);
#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool sco_rcv_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_rcv->sco_rcv_op_data;

   return cpsGetDefaultsMsgHandler(&sco_data->parms_def, message_data,
                                   resp_length,resp_data);
}

bool sco_rcv_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiguously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */
#ifdef INSTALL_PLC100
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &sco_rcv->sco_rcv_op_data;

   return cpsSetParameterMsgHandler(&sco_data->parms_def, message_data,
                                    resp_length,resp_data);
#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool sco_rcv_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_NB_DEC_OP_DATA *sco_rcv = get_instance_data(op_data);
    unsigned* resp;

    if(!common_obpm_status_helper(message_data,
                                  resp_length, resp_data,
                                  sizeof(SCO_RCV_STATISTICS),
                                  &resp))
    {
         return FALSE;
    }

    /* Fill the statistics as needed.
     */
    if (resp)
    {
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(sco_rcv->sco_rcv_op_data.frame_count,
                             sco_rcv->sco_rcv_op_data.frame_error_count, resp);
        resp = cpsPack2Words(0, 0, resp);
        cpsPack2Words(0, 0, resp);
    }

    return TRUE;
}
