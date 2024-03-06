/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_dec.c
 * \ingroup  operators
 *
 *  SWBS_DEC operator
 *
 */

/****************************************************************************
Include Files
*/
#include "swbs_private.h"
#include "swbs_dec_gen_c.h"
#include "op_msg_utilities.h"
#include "op_msg_helpers.h"

#include "patch/patch.h"

#ifdef ADAPTIVE_R2_BUILD
#include "aptXAdaptiveTypes_r2.h"
#include "aptXAdaptiveDecode_r2.h"
#include "aptXAdaptiveDecoderMemory_r2.h"
#else
#include "aptXAdaptiveTypes.h"
#include "aptXAdaptiveDecode.h"
#include "aptXAdaptiveDecoderMemory.h"
#endif

/****************************************************************************
Private Constant Definitions
*/

/****************************************************************************
Private Type Definitions
*/


/****************************************************************************
Private Constant Declarations
*/

/** The SWBS decoder capability function handler table */
const handler_lookup_struct swbs_dec_handler_table =
{
    swbs_dec_create,           /* OPCMD_CREATE */
    swbs_dec_destroy,          /* OPCMD_DESTROY */
    swbs_dec_start,            /* OPCMD_START */
    base_op_stop,              /* OPCMD_STOP */
    swbs_dec_reset,            /* OPCMD_RESET */
    swbs_dec_connect,          /* OPCMD_CONNECT */
    swbs_dec_disconnect,       /* OPCMD_DISCONNECT */
    swbs_dec_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    swbs_dec_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info     /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry swbs_dec_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               swbs_dec_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              swbs_dec_opmsg_disable_fadeout},
    {OPMSG_SWBS_DEC_ID_FORCE_PLC_OFF,              swbs_dec_opmsg_force_plc_off},
    {OPMSG_SWBS_DEC_ID_FRAME_COUNTS,               swbs_dec_opmsg_frame_counts},
    {OPMSG_SWBS_DEC_ID_CODEC_MODE,                 swbs_dec_opmsg_codec_mode},
    {OPMSG_COMMON_ID_SET_CONTROL,                  swbs_dec_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                   swbs_dec_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                 swbs_dec_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                   swbs_dec_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                   swbs_dec_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,              sco_common_rcv_opmsg_set_buffer_size},
    {OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE,     sco_common_rcv_opmsg_set_terminal_buffer_size},
    {OPMSG_COMMON_SET_TTP_LATENCY,                 sco_common_rcv_opmsg_set_ttp_latency},
    {0, NULL}};


const CAPABILITY_DATA swbs_dec_cap_data =
    {
        SWBS_DEC_CAP_ID,             /* Capability ID */
        1,1,//SWBS_DEC_WB_VERSION_MAJOR, 1,   /* Version information - hi and lo parts */
        1, 1,                           /* Max number of sinks/inputs and sources/outputs */
        &swbs_dec_handler_table,        /* Pointer to message handler function table */
        swbs_dec_opmsg_handler_table,   /* Pointer to operator message handler function table */
        swbs_dec_process_data,          /* Pointer to data processing function */
        0,                              /* Reserved */
        sizeof(SWBS_DEC_OP_DATA)        /* Size of capability-specific per-instance data */
    };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SWBS_DEC, SWBS_DEC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SWBS_DEC, SWBS_DEC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */


/****************************************************************************
Private Function Definitions
*/
axErr_t swbs_decode_init_decoder_structs(SWBS_DEC_OP_DATA *op_data);
void OnPacketDecoded_swbs ( aptXDecodeEventType_t eType,  aptXDecodeEventUser_t* pUser, void* pvDecoderData );

static inline SWBS_DEC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_DEC_OP_DATA *) base_op_get_instance_data(op_data);\
}

/* ******************************* Helper functions ************************************ */

/* initialise various working data params of the specific operator */
static void swbs_dec_reset_working_data(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* rcvData = &x_data->sco_rcv_op_data;

    patch_fn_shared(swbs_decode);

    if(rcvData != NULL)
    {
        /* Initialise fadeout-related parameters */
        rcvData->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        rcvData->fadeout_parameters.fadeout_counter = 0;
        rcvData->fadeout_parameters.fadeout_flush_count = 0;
    }
}


/* free the memory allocated for aptX Adaptive dec (shared and non-shared) */
static void swbs_dec_free_state_data(OPERATOR_DATA* op_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);

    patch_fn_shared(swbs_decode);

    if (x_data->codec_data != NULL)
    {
        /* free aptX adaptive internal tables */
        aptXDecode_KalimbaFreeMemory((void*)x_data->codec_data[0] ,(void*)&x_data->codec_data[0], 1);

        /* now free the codec data buffers */
        pdelete(x_data->codec_data[1]);
        pdelete(x_data->codec_data[2]);
    }
}


/* ********************************** API functions ************************************* */

/* TODO: a large part of this can be re-used from SCO RCV - so may move those out into a common helper function */
bool swbs_dec_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data = &swbs_dec->sco_rcv_op_data;

    patch_fn_shared(swbs_decode);

    sco_pio_init_pio_dec();

    sco_data->input_buffer_size = SWBS_DEC_INPUT_BUFFER_SIZE;
    sco_data->output_buffer_size = SWBS_DEC_OUTPUT_BUFFER_SIZE;

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Initialise some of the operator data that is common between NB and WB receive. It can only be called
     * after the PLC structure is allocated (if PLC present in build) */
    sco_common_rcv_initialise(sco_data);

    sco_data->sample_rate = swbs_get_sample_rate(op_data);

    /* create aptX Adaptive data object */
    if (AX_OK != swbs_decode_init_decoder_structs(swbs_dec))
    {
        swbs_dec_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* initialise some more SWBS decoder-specific data  */
    swbs_dec_reset_working_data(op_data);
    swbsdec_init_dec_param(op_data);

    return TRUE;
}


bool swbs_dec_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA* x_data;
    x_data = get_instance_data(op_data);

    L2_DBG_MSG3("t = %08x: Destroying swbs_dec operator %04x, Debug counters:"
                "\n  total output samples: %d,",
                time_get_time(), base_op_get_ext_op_id(op_data),
                x_data->swbs_dec_output_samples);
#ifdef SCO_DEBUG
    L2_DBG_MSG4("\n  Num of kicks in period: %d,"
                "\n  num of times validate returned non zero values: %d,"
                "\n  num of times validate returned values >= 240: %d,"
                "\n  num of times validate_wbm returned values >= 240: %d,",
                x_data->swbs_dec_num_kicks,
                x_data->swbs_dec_validate_ret_nz,
                x_data->swbs_dec_validate_ret_good,
                x_data->swbs_dec_validate_ret_wbm);
#endif

    if(base_op_destroy_lite(op_data, response_data))
    {
        /* Delete any common data. */
        sco_common_rcv_destroy(&x_data->sco_rcv_op_data);
        /* now destroy all the capability specific data */
        swbs_dec_free_state_data(op_data);
        return TRUE;
    }

    return FALSE;
}


bool swbs_dec_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    swbs_dec_reset_working_data(op_data);
    swbsdec_init_dec_param(op_data);
    return TRUE;
}


bool swbs_dec_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *xdata = get_instance_data(op_data);

    patch_fn_shared(swbs_decode);

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

    L2_DBG_MSG2("t=%08x: swbs_dec operator %04x started", time_get_time(), base_op_get_ext_op_id(op_data));
    return TRUE;
}


bool swbs_dec_connect(OPERATOR_DATA *op_data, void *message_data,
                      unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *x_data = get_instance_data(op_data);
    bool status;
    SCO_TERMINAL_BUFFERS *terminal_buffers;

    terminal_buffers = &(x_data->sco_rcv_op_data.buffers);

    status = sco_common_connect(op_data, message_data,
                                response_id, response_data,
                                terminal_buffers,
                                NULL);

    return status;
}


bool swbs_dec_disconnect(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **response_data)
{
    SWBS_DEC_OP_DATA *x_data = get_instance_data(op_data);
    bool status;
    SCO_TERMINAL_BUFFERS *terminal_buffers;

    terminal_buffers = &(x_data->sco_rcv_op_data.buffers);

    status = sco_common_disconnect(op_data, message_data,
                                   response_id, response_data,
                                   terminal_buffers, NULL);

    return status;
}

bool swbs_dec_get_data_format(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **response_data)
{
    return sco_common_get_data_format(op_data, message_data,
                                      response_id, response_data,
                                      AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP,
                                      AUDIO_DATA_FORMAT_FIXP);
}


/* ************************************* Data processing-related functions and wrappers **********************************/

void swbs_dec_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA *sco_data;
    unsigned status;

    patch_fn(swbs_decode_process_data);

    sco_pio_set_pio_dec();

    sco_data = &x_data->sco_rcv_op_data;

    /* We can't have started in this state, so if we lose a buffer carry on
     * and hope that it comes back or we are stopped. If not then the error
     * (lack of data) should propagate to something that cares. */
    if (sco_data->buffers.ip_buffer == NULL || sco_data->buffers.op_buffer == NULL)
    {
        sco_pio_clr_pio_dec();
        return;
    }

    /* call ASM function */
    status = swbs_dec_processing(op_data);

    /* If fadeout is enabled, do it on the current output data, if processing
     * has actually produced output.
     */
    if( (sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE) && status)
    {
        /* the wrapper below takes output Cbuffer and fadeout params, and the current packet size in words is from PLC */
        if(mono_cbuffer_fadeout(sco_data->buffers.op_buffer,
                                x_data->swbs_dec_output_samples,
                                &(sco_data->fadeout_parameters)))
        {
            common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
        }
    }

    touched->sources = status;

    sco_pio_clr_pio_dec();
}


/* **************************** Operator message handlers ******************************** */


bool swbs_dec_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters,
                             RUNNING_STATE);

    return TRUE;
}

bool swbs_dec_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    common_set_fadeout_state(&x_data->sco_rcv_op_data.fadeout_parameters,
                             NOT_RUNNING_STATE);

    return TRUE;
}

bool swbs_dec_opmsg_force_plc_off(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
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


bool swbs_dec_opmsg_frame_counts(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* x_data = get_instance_data(op_data);
    return sco_common_rcv_frame_counts_helper(&x_data->sco_rcv_op_data,
                                              message_data,
                                              resp_length, resp_data);

}

bool swbs_dec_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_DEC_OP_DATA* swbs_dec_params = get_instance_data(op_data);
    APTX_DECODE_STATE pDecodeState = (APTX_DECODE_STATE)swbs_dec_params->codec_data[0];
    uint16 mode;

    mode = OPMSG_FIELD_GET(message_data, OPMSG_SWBS_DEC_CODEC_MODE, CODEC_MODE);
    if(mode == SWBS_CODEC_MODE4)
    {
        /* set codec mode to Mode 4 (24kHz) */
        swbs_dec_params->codecMode = SWBS_CODEC_MODE4;
    }
    else
    {
        /* set codec mode to Mode 0 (32kHz) */
        swbs_dec_params->codecMode = SWBS_CODEC_MODE0;
    }
    aptXDecode_updateSpeechMode(pDecodeState,swbs_dec_params->codecMode);

    return TRUE;
}

bool swbs_dec_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* In the case of this capability, nothing is done for control message.
     * Just follow protocol and ignore any content. */
    return cps_control_setup(message_data, resp_length, resp_data,NULL);
}


bool swbs_dec_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    return FALSE;
}

bool swbs_dec_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data,
                                      unsigned *resp_length,
                                      OP_OPMSG_RSP_PAYLOAD **resp_data)
{
   SWBS_DEC_OP_DATA* swbs_dec = get_instance_data(op_data);

   return cpsGetDefaultsMsgHandler(&swbs_dec->sco_rcv_op_data.parms_def,
                                   message_data,
                                   resp_length,
                                   resp_data);
}

bool swbs_dec_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Copied from wbs but not filled in.
     */

    return FALSE;
}

bool swbs_dec_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    unsigned* resp;

    if(!common_obpm_status_helper(message_data, resp_length, resp_data,
                                  sizeof(SWBS_DEC_STATISTICS),
                                  &resp))
    {
         return FALSE;
    }

    /* Fill the statistics as needed */
    if(resp)
    {
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(0, 0, resp);
        resp = cpsPack2Words(0, 0, resp);
    }

    return TRUE;
}

axErr_t swbs_decode_init_decoder_structs(SWBS_DEC_OP_DATA *aptx_data)
{
    aptXConfig_t       decodeConfig ;
    aptXDecodeSetup_t  decodeSetup;

    patch_fn_shared(swbs_decode);

    void * pAllocatedMemory = NULL;
    void * pAsmStruct= NULL;
    APTX_DECODE_STATE pDecodeState = NULL;

    axBuf_t *pInBuf  = &aptx_data->axInBuf;
    axBuf_t *pOutBuf = &aptx_data->axOutBuf;

    axErr_t err;

    decodeSetup.decodeMode       = APTX_DECODE_MODE_NORMAL;
    decodeSetup.channelReference = APTX_SELECT_STEREO;
    decodeSetup.requestedProfile = APTX_PROFILE_SPEECH          ;
    decodeSetup.bDisableSyncLock = TRUE;

    aptx_data->codec_data[1] = xzppmalloc(SWBS_DEC_INPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM1);    // Input Buffer (read only)
    if (NULL == aptx_data->codec_data[1])
      return AX_ERROR;
    aptx_data->codec_data[2] = xzppmalloc(SWBS_DEC_OUTPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM2);   // Output Buffer
    if (NULL == aptx_data->codec_data[2])
      return AX_ERROR;


    aptx_data->num_out_channels = 1;

    pAsmStruct=&aptx_data->codec_data[0];
    pAllocatedMemory = aptXDecode_KalimbaAllocMemory(pAsmStruct, 1);
    if (NULL == pAllocatedMemory)
      return AX_ERROR;

    aptx_data->packet_size = 60;

    err = aptXDecode_Create( &pDecodeState, &decodeSetup, aptx_data->codecMode, pAllocatedMemory ) ;

    aptXDecode_SetEvent ( pDecodeState, APTX_DECODE_EVENT_PACKET_DECODED, OnPacketDecoded_swbs, (void *)aptx_data, NULL );

    int8_t * pBuffer1 = (int8_t*)aptx_data->codec_data[1];
    int8_t * pBuffer2 = (int8_t*)aptx_data->codec_data[2];
    aptx_data->codec_data[0] = pDecodeState;

   // Create the decoderInput axBuf buffer
    AxBufInit( pInBuf) ;
    pInBuf->dat.pn32 = (int32_t*)pBuffer1;
    pInBuf->nFill    = SWBS_DEC_INPUT_BUFFER_SIZE; // MAX_INPUT_BUFFER_SIZE ;
    pInBuf->nMax     = SWBS_DEC_INPUT_BUFFER_SIZE; // MAX_INPUT_BUFFER_SIZE ;
    pInBuf->fmt.flow = AXFMT_LINEAR;
    pInBuf->fmt.type = AXFMT_NOTDEFINED;
    pInBuf->fmt.nSpan = 1;


    //Create the decoderOutput axBuf buffer
    AxBufInit( pOutBuf ) ;
    pOutBuf->dat.pn32 = (int32_t*)pBuffer2;
    pOutBuf->nFill    = SWBS_DEC_OUTPUT_BUFFER_SIZE; // MAX_OUTPUT_BUFFER_SIZE ;
    pOutBuf->nMax     = SWBS_DEC_OUTPUT_BUFFER_SIZE; // MAX_OUTPUT_BUFFER_SIZE ;
    pOutBuf->fmt.flow = AXFMT_LINEAR;
    pOutBuf->fmt.type = AXFMT_STEREO_32BIT;
    pOutBuf->fmt.nSpan = 1;


    if (aptXDecode_GetConfig( pDecodeState, &decodeConfig ) != AX_OK)
    {
        return AX_ERROR;
    }

    return err;
}

void OnPacketDecoded_swbs ( aptXDecodeEventType_t eType,  aptXDecodeEventUser_t* pUser, void* pvDecoderData )
{
    SWBS_DEC_OP_DATA *pCodecData = (SWBS_DEC_OP_DATA *)pUser->pvContext;

    pCodecData->storedWP = pCodecData->axOutBuf.nWPtr;
    pCodecData->overlap_finished = TRUE;
}

/**
 * \brief The SWBS codec works with either 24k or 32k audio streams. This is
 *        selected after creation, setting OPMSG_SWBS_DEC_ID_CODEC_MODE.
 *        This is needed by the sco_fw to process metadata.
 *
 * \param op_data SWBS operator data.
 *
 * \return       sample rate
 */
unsigned swbs_get_sample_rate(OPERATOR_DATA *op_data)
{
    SWBS_DEC_OP_DATA* swbs_dec_params = get_instance_data(op_data);

    if (swbs_dec_params->codecMode == SWBS_CODEC_MODE4)
    {
        return 24000;
    }
    else
    {
        return 32000;
    }
}

bool swbs_dec_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned swbs_in_size = SWBS_DEC_INPUT_BUFFER_SIZE;
    unsigned swbs_out_size = SWBS_DEC_OUTPUT_BUFFER_SIZE;

    /* for decoder, the output size might have been configured by the user */
    SWBS_DEC_OP_DATA* swbs_data = (SWBS_DEC_OP_DATA*)base_op_get_instance_data(op_data);
    SCO_COMMON_RCV_OP_DATA* sco_data = &(swbs_data->sco_rcv_op_data);
    swbs_out_size = MAX(swbs_out_size, sco_data->output_buffer_size);
    swbs_in_size = MAX(swbs_in_size, sco_data->input_buffer_size);

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* Currently these have the same value but this isn't guaranteed */
    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_in_size;
        L4_DBG_MSG2("swbs_dec_buffer_details (capID=%d)  input buffer size=%d \n", base_op_get_cap_id(op_data), swbs_in_size);
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_out_size;
        L4_DBG_MSG2("swbs_dec_buffer_details (capID=%d)  output buffer size=%d \n", base_op_get_cap_id(op_data), swbs_out_size);
    }

    /* supports metadata in both side for encode and decode */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = NULL;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;

    return TRUE;
}
