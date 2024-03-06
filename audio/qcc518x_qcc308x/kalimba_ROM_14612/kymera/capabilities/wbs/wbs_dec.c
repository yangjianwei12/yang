/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  wbs_dec.c
 * \ingroup  operators
 *
 *  WBS_DEC operator
 *
 */

/****************************************************************************
Include Files
*/
#include "wbs_private.h"
#include "wbs_dec_gen_c.h"
#include "capabilities.h"
#include "sco_drv/sco_drv_struct.h"

/****************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Private Type Definitions
*/


/****************************************************************************
Private Constant Declarations
*/

/** The WBS decoder capability function handler table */
const handler_lookup_struct wbs_dec_handler_table =
{
    wbs_dec_create,           /* OPCMD_CREATE */
    wbs_dec_destroy,          /* OPCMD_DESTROY */
    wbs_dec_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    wbs_dec_reset,            /* OPCMD_RESET */
    wbs_dec_connect,          /* OPCMD_CONNECT */
    wbs_dec_disconnect,       /* OPCMD_DISCONNECT */
    wbs_dec_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    wbs_dec_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry wbs_dec_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               wbs_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              wbs_dec_opmsg_disable_fadeout},
#ifdef INSTALL_PLC100
    {OPMSG_WBS_DEC_ID_FORCE_PLC_OFF,               wbs_dec_opmsg_force_plc_off},
#endif /* INSTALL_PLC100 */
    {OPMSG_WBS_DEC_ID_FRAME_COUNTS,                wbs_dec_opmsg_frame_counts},
    {OPMSG_COMMON_ID_SET_CONTROL,                  wbs_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   wbs_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 wbs_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   wbs_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   wbs_dec_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,              sco_common_rcv_opmsg_set_buffer_size},
    {OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE ,    sco_common_rcv_opmsg_set_terminal_buffer_size},
    {OPMSG_COMMON_SET_TTP_LATENCY,                 sco_common_rcv_opmsg_set_ttp_latency},
    {0, NULL}};


const CAPABILITY_DATA wbs_dec_cap_data =
    {
        WBS_DEC_CAP_ID,             /* Capability ID */
        WBS_DEC_WB_VERSION_MAJOR, 1,    /* Version information - hi and lo parts */
        1, 1,                           /* Max number of sinks/inputs and sources/outputs */
        &wbs_dec_handler_table,         /* Pointer to message handler function table */
        wbs_dec_opmsg_handler_table,    /* Pointer to operator message handler function table */
        wbs_dec_process_data,           /* Pointer to data processing function */
        0,                              /* Reserved */
        sizeof(WBS_DEC_OP_DATA)         /* Size of capability-specific per-instance data */
    };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_WBS_DEC, WBS_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_WBS_DEC, WBS_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/** Memory owned by a decoder instance */
const malloc_t_entry wbs_dec_malloc_table[WBS_DEC_MALLOC_TABLE_LENGTH] =
{
    {30, MALLOC_PREFERENCE_NONE, offsetof(sbc_codec, wbs_frame_buffer_ptr)},
    {SBC_SYNTHESIS_BUFF_LENGTH, MALLOC_PREFERENCE_DM2, offsetof(sbc_codec, synthesis_vch1)}
};

const scratch_table wbs_dec_scratch_table =
{
    WBS_DM1_SCRATCH_TABLE_LENGTH,
    SBC_DEC_DM2_SCRATCH_TABLE_LENGTH,
    0,
    wbs_scratch_table_dm1,
    sbc_scratch_table_dm2,
    NULL
};

/****************************************************************************
Private Function Definitions
*/
static inline WBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (WBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);
}

/* ******************************* Helper functions ************************************ */

/* initialise various working data params of the specific operator */
static void wbs_dec_reset_working_data(OPERATOR_DATA *op_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);

    sco_common_rcv_reset_working_data(&x_data->sco_rcv_op_data);
#ifdef INSTALL_PLC100
    sco_plc100_initialize(x_data->plc100_struc);
#endif /* INSTALL_PLC100 */

    /* Now reset the decoder - re-using old but slightly massaged function in ASM */
    wbs_dec_reset_sbc_data(op_data);
}


/* free the memory allocated for SBC dec (shared and non-shared) */
static void wbs_dec_free_state_data(OPERATOR_DATA* op_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);

    if (x_data->codec_data != NULL)
    {
        /* free the shared codec data */
        mem_table_free_shared((void *)(x_data->codec_data),
                            wbs_sbc_shared_malloc_table, WBS_SBC_SHARED_TABLE_LENGTH);

        /* free shared decoder data */
        mem_table_free_shared((void *)(x_data->codec_data),
                    wbs_sbc_dec_shared_malloc_table, WBS_SBC_DEC_SHARED_TABLE_LENGTH);

        /* free non-shared memory */
        mem_table_free((void *)(x_data->codec_data), wbs_dec_malloc_table,
                                                WBS_DEC_MALLOC_TABLE_LENGTH);

        /* now free the codec data object */
        pdelete(x_data->codec_data);
        x_data->codec_data = NULL;
    }

#ifdef INSTALL_PLC100
    sco_plc100_free_data(x_data->plc100_struc);
    x_data->plc100_struc = NULL;
#endif /* INSTALL_PLC100 */
}



/* ********************************** API functions ************************************* */

/* TODO: a large part of this can be re-used from SCO RCV - so may move those out into a common helper function */
bool wbs_dec_create(OPERATOR_DATA *op_data, void *message_data,
                    unsigned *response_id, void **response_data)
{
    WBS_DEC_OP_DATA* wbs_dec = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &wbs_dec->sco_rcv_op_data;

    bool new_allocation;

    sco_pio_init_pio_dec();

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    sco_data->input_buffer_size = WBS_DEC_INPUT_BUFFER_SIZE;
    sco_data->output_buffer_size = WBS_DEC_OUTPUT_BUFFER_SIZE;

#ifdef INSTALL_PLC100
    /* setup the PLC structure, zero initialise it so that we can detect if an
     * allocation has happened when we unwind */
    PLC100_STRUC* plcstruct = sco_plc100_create(SP_BUF_LEN_WB,
                                                OLA_LEN_WB,
                                                PLC100_WB_CONSTANTS);
    if (plcstruct == NULL)
    {
        /* Change the already allocated response to command failed.
         * No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    wbs_dec->plc100_struc = plcstruct;
#endif /* INSTALL_PLC100 */

    /* Initialise some of the operator data that is common between NB and WB receive. It can only be called
     * after the PLC structure is allocated (if PLC present in build) */
    sco_common_rcv_initialise(&wbs_dec->sco_rcv_op_data);

    wbs_dec->sco_rcv_op_data.sample_rate = 16000;

    /* create SBC data object */
    if((wbs_dec->codec_data = xzpnew(sbc_codec)) == NULL)
    {
        wbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Share memory with decoders. */
    if( !mem_table_zalloc_shared((void *)(wbs_dec->codec_data), wbs_sbc_dec_shared_malloc_table,
            WBS_SBC_DEC_SHARED_TABLE_LENGTH, &new_allocation))
    {
        wbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    if( !mem_table_zalloc_shared((void *)(wbs_dec->codec_data), wbs_sbc_shared_malloc_table,
            WBS_SBC_SHARED_TABLE_LENGTH, &new_allocation))
    {
        wbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* now allocate the non-shareable memory */
    if(!mem_table_zalloc((void *)(wbs_dec->codec_data), wbs_dec_malloc_table,
                                                WBS_DEC_MALLOC_TABLE_LENGTH))
    {
        wbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

#ifdef INSTALL_PLC100
    /* For future proofing, however this is one field in one block
     * (and one field in opdata structure staring at force_plc_off) */
    if(!cpsInitParameters(&wbs_dec->sco_rcv_op_data.parms_def,
                          (unsigned*)WBS_DEC_GetDefaults(base_op_get_cap_id(op_data)),
                          (unsigned*)&wbs_dec->force_plc_off,
                          sizeof(WBS_DEC_PARAMETERS)))
    {
        wbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }
#endif /* INSTALL_PLC100 */

    /* Now reserve the scratch memory */
    if (scratch_register())
    {
        if (mem_table_scratch_tbl_reserve(&wbs_dec_scratch_table))
        {
            /* Successfully allocated everything! */
            /* initialise some more WBS decoder-specific data  */
            wbs_dec_reset_working_data(op_data);
            wbsdec_init_dec_param(op_data);

            return TRUE;
        }
        /* Fail free all the scratch memory we reserved */
        scratch_deregister();
    }
    /* Clear up all the allocated memory. */
    wbs_dec_free_state_data(op_data);
   /* Change the already allocated response to command failed. No extra error info. */
    base_op_change_response_status(response_data, STATUS_CMD_FAILED);
    return TRUE;
}


bool wbs_dec_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    WBS_DEC_OP_DATA* x_data;
    x_data = get_instance_data(op_data);
    L2_DBG_MSG3("t = %08x: Destroying wbs_dec operator %04x. debug counters"
                "\n  total output samples: %d,",
                time_get_time(), base_op_get_ext_op_id(op_data),
                x_data->wbs_dec_output_samples);
#ifdef SCO_DEBUG
    L2_DBG_MSG5("\n  no space in the output buffer: %d,"
                "\n  Not enough data to process: %d,"
                "\n  No sync word was found in the frame: %d,"
                "\n  validate advanced: %d,"
                "\n  WBS validate returned an invalid result: %d,",
                x_data->wbs_dec_validate_ret.named.wbs_dec_validate_ret_0,
                x_data->wbs_dec_validate_ret.named.wbs_dec_validate_ret_1,
                x_data->wbs_dec_validate_ret.named.wbs_dec_validate_ret_2,
                x_data->wbs_dec_validate_advanced,
                x_data->wbs_dec_invalid_validate_result);
    L2_DBG_MSG2("\n  number of times the decoder aligned to sync: %d,"
                "\n  last amount which was discarded during the alignment: %d",
                x_data->align_to_sync_count,
                x_data->discarded_until_sync );
#endif

    if(base_op_destroy_lite(op_data, response_data))
    {
        /* Delete any common data. */
        sco_common_rcv_destroy(&x_data->sco_rcv_op_data);
        /* Free all the scratch memory we reserved */
        scratch_deregister();
        /* now destroy all the capability specific data */
        wbs_dec_free_state_data(op_data);
        return TRUE;
    }

    return FALSE;
}


bool wbs_dec_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    wbs_dec_reset_working_data(op_data);
    wbsdec_init_dec_param(op_data);
    return TRUE;
}


bool wbs_dec_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    WBS_DEC_OP_DATA *xdata = get_instance_data(op_data);

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
    if (   xdata->sco_rcv_op_data.buffers.ip_buffer == NULL
        || xdata->sco_rcv_op_data.buffers.op_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    L2_DBG_MSG2("t=%08x: wbs_dec operator %04x started", time_get_time(), base_op_get_ext_op_id(op_data));
    return TRUE;
}


bool wbs_dec_connect(OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **response_data)
{
    WBS_DEC_OP_DATA *x_data = get_instance_data(op_data);
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
#else
    UNUSED(terminal)
#endif /* INSTALL_PLC100 */

    return status;
}


bool wbs_dec_disconnect(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    WBS_DEC_OP_DATA *x_data = get_instance_data(op_data);
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
#else
    UNUSED(terminal)
#endif /* INSTALL_PLC100 */

    return status;
}

bool wbs_dec_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_data_format(op_data, message_data,
                                      response_id, response_data,
                                      AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP,
                                      AUDIO_DATA_FORMAT_FIXP);
}


/* ************************************* Data processing-related functions and wrappers **********************************/

void wbs_dec_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &x_data->sco_rcv_op_data;
    unsigned status;

    patch_fn(wbs_decode_process_data);

    sco_pio_set_pio_dec();

    /* We can't have started in this state, so if we lose a buffer carry on
     * and hope that it comes back or we are stopped. If not then the error
     * (lack of data) should propagate to something that cares. */
    if (sco_data->buffers.ip_buffer == NULL || sco_data->buffers.op_buffer == NULL)
    {
        sco_pio_clr_pio_dec();
        return;
    }

    /* Commit any scratch memory ideally this should be done later after the
     * decision to decode is made. */
    mem_table_scratch_tbl_commit(x_data->codec_data, &wbs_dec_scratch_table);

    /* call ASM function */
    status = wbs_dec_processing(op_data);

    /* Free the scratch memory used */
    scratch_free();

    /* Is fadeout enabled? if yes, do it on the current output data, if processing has actually produced output.
     * Now the slight migraine is that deep inside decoder, it may have decided to decode two frames - so the last guy that
     * really had to know the final outcome of how many samples were produced is PLC. Its packet size after return will be
     * the most reliable indicator of how many samples we need to process. If PLC is not installed, then WBS validate() function
     * is the only thing that tells the real story - of course, without PLC this would work in a very funny way.
     */
    if( (sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE) && status)
    {
        /* the wrapper below takes output Cbuffer and fadeout params, and the current packet size in words is from PLC */
        if(mono_cbuffer_fadeout(sco_data->buffers.op_buffer,
                                x_data->wbs_dec_output_samples,
                                &(sco_data->fadeout_parameters)))
        {
            common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
        }
    }

    touched->sources = status;

    sco_pio_clr_pio_dec();
}


/* **************************** Operator message handlers ******************************** */


bool wbs_dec_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters, RUNNING_STATE);

    return TRUE;
}

bool wbs_dec_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}

#ifdef INSTALL_PLC100
bool wbs_dec_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
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

bool wbs_dec_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_frame_counts_helper(&x_data->sco_rcv_op_data,
                                              message_data,
                                              resp_length, resp_data);

}

bool wbs_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message. Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data, NULL);
}


bool wbs_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
#ifdef INSTALL_PLC100
   WBS_DEC_OP_DATA* wbs_dec = get_instance_data(op_data);

   return cpsGetParameterMsgHandler(&wbs_dec->sco_rcv_op_data.parms_def,
                                    message_data,
                                    resp_length, resp_data);

#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool wbs_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   WBS_DEC_OP_DATA* wbs_dec = get_instance_data(op_data);

   return cpsGetDefaultsMsgHandler(&wbs_dec->sco_rcv_op_data.parms_def,
                                   message_data,
                                   resp_length, resp_data);
}

bool wbs_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Set the parameter(s). For future proofing, it is using the whole mechanism, although currently there is only one field
     * in opdata structure that is a setable parameter. If later there will be more (ever), must follow contiquously the first field,
     * as commented and instructed in the op data definition. Otherwise consider moving them into a dedicated structure.
     */
#ifdef INSTALL_PLC100
   WBS_DEC_OP_DATA* wbs_dec = get_instance_data(op_data);

   return cpsSetParameterMsgHandler(&wbs_dec->sco_rcv_op_data.parms_def,
                                    message_data,
                                    resp_length, resp_data);
#else
    return FALSE;
#endif /* INSTALL_PLC100 */
}

bool wbs_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WBS_DEC_OP_DATA* wbs_dec_params = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_rcv = &wbs_dec_params->sco_rcv_op_data;
    unsigned* resp;

    if(!common_obpm_status_helper(message_data,
                                  resp_length, resp_data,
                                  sizeof(WBS_DEC_STATISTICS),
                                  &resp))
    {
         return FALSE;
    }

    /* Fill the statistics as needed */
    if(resp)
    {
        resp = cpsPack2Words(0,
                             0, resp);
        resp = cpsPack2Words(sco_rcv->frame_count,
                             sco_rcv->frame_error_count, resp);
        resp = cpsPack2Words(0,
                             0, resp);
        resp = cpsPack2Words(0,
                             0, resp);
        resp = cpsPack2Words(0,
                             0,
                             resp);
        resp = cpsPack2Words(0,
                             0,
                             resp);
    }

    return TRUE;
}

bool wbs_dec_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned wbs_in_size = WBS_DEC_INPUT_BUFFER_SIZE;
    unsigned wbs_out_size = WBS_DEC_OUTPUT_BUFFER_SIZE;

    /* for decoder, the output size might have been configured by the user */
    WBS_DEC_OP_DATA* wbs_data = (WBS_DEC_OP_DATA*)base_op_get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(wbs_data->sco_rcv_op_data);
    wbs_in_size = MAX(wbs_in_size, sco_data->input_buffer_size);
    wbs_out_size = MAX(wbs_out_size, sco_data->output_buffer_size);

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Currently these have the same value but this isn't guaranteed */
    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = wbs_in_size;
        L4_DBG_MSG2("wbs_dec_buffer_details (capID=%d)  input buffer size=%d \n", base_op_get_cap_id(op_data), wbs_in_size);
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = wbs_out_size;
        L4_DBG_MSG2("wbs_dec_buffer_details (capID=%d)  output buffer size=%d \n", base_op_get_cap_id(op_data), wbs_out_size);
    }

    /* Supports metadata for both encode and decode at input and output */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = NULL;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;

    return TRUE;
}
