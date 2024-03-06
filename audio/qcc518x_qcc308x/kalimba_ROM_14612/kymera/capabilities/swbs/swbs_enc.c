/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  swbs_enc.c
 * \ingroup  operators
 *
 *  SWBS_ENC operator
 *
 */

/****************************************************************************
Include Files
*/
#include "swbs_private.h"

#include "patch/patch.h"

#ifdef ADAPTIVE_R2_BUILD
#include "aptXAdaptiveTypes_r2.h"
#include "aptXAdaptiveEncode_r2.h"
#include "aptXAdaptiveEncoderMemory_r2.h"
#else
#include "aptXAdaptiveTypes.h"
#include "aptXAdaptiveEncode.h"
#include "aptXAdaptiveEncoderMemory.h"
#endif
/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Constant Declarations
*/

/** The SWBS encoder capability function handler table */
const handler_lookup_struct swbs_enc_handler_table =
{
    swbs_enc_create,           /* OPCMD_CREATE */
    swbs_enc_destroy,          /* OPCMD_DESTROY */
    swbs_enc_start,            /* OPCMD_START */
    base_op_stop,              /* OPCMD_STOP */
    swbs_enc_reset,            /* OPCMD_RESET */
    swbs_enc_connect,          /* OPCMD_CONNECT */
    swbs_enc_disconnect,       /* OPCMD_DISCONNECT */
    swbs_enc_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    swbs_enc_get_data_format,  /* OPCMD_DATA_FORMAT */
    swbs_enc_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry swbs_enc_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,      base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,               swbs_enc_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,              swbs_enc_opmsg_disable_fadeout},
    {OPMSG_SWBS_ENC_ID_CODEC_MODE,                 swbs_enc_opmsg_codec_mode},
    {OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE ,    sco_common_send_opmsg_set_terminal_buffer_size},
    {0, NULL}};


const CAPABILITY_DATA swbs_enc_cap_data =
    {
        SWBS_ENC_CAP_ID,                /* Capability ID */
        0, 1,                           /* Version information - hi and lo parts */
        1, 1,                           /* Max number of sinks/inputs and sources/outputs */
        &swbs_enc_handler_table,        /* Pointer to message handler function table */
        swbs_enc_opmsg_handler_table,   /* Pointer to operator message handler function table */
        swbs_enc_process_data,          /* Pointer to data processing function */
        0,                              /* Reserved */
        sizeof(SWBS_ENC_OP_DATA)        /* Size of capability-specific per-instance data */
    };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SWBS_ENC, SWBS_ENC_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SWBS_ENC, SWBS_ENC_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */


/****************************************************************************
Private Function Definitions
*/
axErr_t swbs_encode_init_encoder_structs(SWBS_ENC_OP_DATA *op_data);
static inline SWBS_ENC_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SWBS_ENC_OP_DATA *) base_op_get_instance_data(op_data);
}

/* ******************************* Helper functions ************************************ */


/* initialise various working data params of the specific operator */
static void swbs_enc_reset_working_data(OPERATOR_DATA *op_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    patch_fn_shared(swbs_encode);

    if(x_data != NULL)
    {
        /* Initialise fadeout-related parameters */
        sco_data->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        sco_data->fadeout_parameters.fadeout_counter = 0;
        sco_data->fadeout_parameters.fadeout_flush_count = 0;
        sco_common_send_init_metadata(&sco_data->enc_ttp);
    }

    /* Now reset the encoder - re-using old but slightly massaged function in ASM */
    swbs_enc_reset_aptx_data(op_data);
}


/* free the memory allocated for SBC encoder (shared and non-shared) */
static void swbs_enc_free_state_data(OPERATOR_DATA* op_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    patch_fn_shared(swbs_encode);
    if (x_data->codec_data != NULL)
    {
        /* free the codec data object */
        aptXEncode_KalimbaFreeMemory((void*)x_data->codec_data[0] ,(void*)&x_data->codec_data[0], 1);

        /* now free the codec data buffers */
        pdelete(x_data->codec_data[1]);
        pdelete(x_data->codec_data[2]);
    }
}



/* ********************************** API functions ************************************* */

bool swbs_enc_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* swbs_enc = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &swbs_enc->sco_send_op_data;

    patch_fn_shared(swbs_encode);

    sco_pio_init_pio_enc();

    sco_data->input_buffer_size = SWBS_ENC_INPUT_BUFFER_SIZE;
    sco_data->output_buffer_size = SWBS_ENC_OUTPUT_BUFFER_SIZE;

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

      /* create aptX Adaptive data object */
	if (AX_OK != swbs_encode_init_encoder_structs(swbs_enc))
    {
        swbs_enc_free_state_data(op_data);
        /* Change the already allocated response to command failed. No extra error info. */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }


    /* initialise some more SWBS encoder-specific data  */
    swbs_enc_reset_working_data(op_data);
    swbsenc_init_encoder(op_data);
    return TRUE;
}


bool swbs_enc_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(base_op_destroy_lite(op_data, response_data))
    {
        /* now destroy all the aptX encoder and shared codec paraphernalia */
        swbs_enc_free_state_data(op_data);
        return TRUE;
    }

    return FALSE;
}


bool swbs_enc_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    swbs_enc_reset_working_data(op_data);

    return TRUE;
}

bool swbs_enc_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA *x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    patch_fn_shared(swbs_encode);

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
    if (sco_data->buffers.ip_buffer == NULL || sco_data->buffers.op_buffer == NULL)
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    L2_DBG_MSG2("t=%08x: swbs_enc operator %04x started",
    		time_get_time(), base_op_get_ext_op_id(op_data));
    return TRUE;
}


bool swbs_enc_connect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    return sco_common_connect(op_data, message_data, response_id, response_data,
            &sco_data->buffers, NULL);
}

bool swbs_enc_disconnect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    return sco_common_disconnect(op_data, message_data, response_id, response_data,
            &sco_data->buffers, NULL);
}

bool swbs_enc_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
            AUDIO_DATA_FORMAT_FIXP, AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP);
}

bool swbs_enc_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_send_get_sched_info(op_data, message_data, response_id, response_data,
             SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE);
}

/* ************************************* Data processing-related functions and wrappers **********************************/

void swbs_enc_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    unsigned available_space, samples_to_process;
    unsigned sample_rate, frame_size_samples;

    patch_fn(swbs_encode_process_data);

    sco_pio_set_pio_enc();

    /* work out amount of input data to process, based on output space and input data amount */
    available_space = cbuffer_calc_amount_space_in_words(sco_data->buffers.op_buffer);
    samples_to_process = cbuffer_calc_avail_data_words(sco_data->buffers.ip_buffer);

    while ((available_space >= SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE) &&
           (samples_to_process >= SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE))
    {
        /* Is fadeout enabled? if yes, do it on the current input data */
        if(sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE)
        {
            /* the wrapper below takes output Cbuffer and fadeout params, use input block size */
            if(mono_cbuffer_fadeout(sco_data->buffers.ip_buffer, SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE,
                                     &(sco_data->fadeout_parameters)))
            {
                common_send_simple_unsolicited_message(op_data, OPMSG_REPLY_ID_FADEOUT_DONE);
            }
        }

        PL_ASSERT(x_data->codecMode == SWBS_CODEC_MODE0);
        frame_size_samples = SWBS_MODE0_AUDIO_BLOCK_SIZE;
        sample_rate = SWBS_MODE0_SAMPLE_RATE;

        sco_common_send_transport_metadata(&sco_data->buffers, &sco_data->enc_ttp,
                                           frame_size_samples,
                                           SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES, sample_rate);

        swbsenc_process_frame(op_data);

        available_space -= SWBS_ENC_DEFAULT_OUTPUT_BLOCK_SIZE;
        samples_to_process -= SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE;

        touched->sources =  TOUCHED_SOURCE_0;
    }

    if (samples_to_process < SWBS_ENC_DEFAULT_INPUT_BLOCK_SIZE)
    {
        /* If there isn't enough data to process another frame kick backwards */
        touched->sinks = TOUCHED_SINK_0;
    }

    sco_pio_clr_pio_enc();
}


/* **************************** Operator message handlers ******************************** */


bool swbs_enc_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    common_set_fadeout_state(&sco_data->fadeout_parameters, RUNNING_STATE);

    return TRUE;
}

bool swbs_enc_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SWBS_ENC_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    common_set_fadeout_state(&sco_data->fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}

bool swbs_enc_opmsg_codec_mode(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
  //  patch_fn(swbs_dec_opmsg_codec_mode_helper);

    SWBS_ENC_OP_DATA* swbs_enc_params = get_instance_data(op_data);
    void* pEncodeState = (void*)swbs_enc_params->codec_data[0];

    if(OPMSG_FIELD_GET(message_data, OPMSG_SWBS_ENC_CODEC_MODE, CODEC_MODE) == SWBS_CODEC_MODE4)
    {
        /* set codec mode to Mode 4 (24kHz) */
        swbs_enc_params->codecMode = SWBS_CODEC_MODE4;
    }
    else
    {
        /* set codec mode to Mode 0 (32kHz) */
        swbs_enc_params->codecMode = SWBS_CODEC_MODE0;
    }
	aptXEncode_updateSpeechMode(pEncodeState,swbs_enc_params->codecMode);

    return TRUE;
}

axErr_t swbs_encode_init_encoder_structs(SWBS_ENC_OP_DATA *aptx_data)
{
    void * pAllocatedMemory = NULL;
    void*  pEncoderState = NULL  ;
    void * pAsmStruct = NULL;

#ifdef ADAPTIVE_R2_BUILD
    aptXParametersEx_t aptxParametersEx;
    aptXParameters_t *aptXParams = &aptxParametersEx.param;
#else
    aptXParameters_t aptXParams;
#endif

    axErr_t err          = AX_OK ;

    patch_fn_shared(swbs_encode);

    aptXChannelConfig_t channelConfig = APTX_CHANNEL_MONO;

	  //Set up the aptXParameters struct for Super Wideband Speech
#ifdef ADAPTIVE_R2_BUILD
    aptXParams->primaryProfile   = APTX_PROFILE_SPEECH       ;
    aptXParams->secondaryProfile = APTX_PROFILE_NONE        ;
    aptXParams->channelConfig    = channelConfig           ;
    aptXParams->uSampleRate      = 32000      ;                       // This should be a modifiable parameter.
    aptXParams->uBitRate         = 64000      ;                       // This should be a modifiable parameter.
    aptXParams->processMode      = APTX_PROCESS_MODE_FIXED ;
#else
    aptXParams.primaryProfile   = APTX_PROFILE_SPEECH       ;
    aptXParams.secondaryProfile = APTX_PROFILE_NONE        ;
    aptXParams.channelConfig    = channelConfig           ;
    aptXParams.uSampleRate      = 32000      ;                       // This should be a modifiable parameter.
    aptXParams.uBitRate         = 64000      ;                       // This should be a modifiable parameter.
    aptXParams.processMode      = APTX_PROCESS_MODE_FIXED ;
#endif

    pAsmStruct=(void*)&aptx_data->codec_data[0];
#ifdef ADAPTIVE_R2_BUILD
    pAllocatedMemory = aptXEncode_KalimbaAllocMemory(pAsmStruct, &aptxParametersEx, 1);
#else
    pAllocatedMemory = aptXEncode_KalimbaAllocMemory(pAsmStruct, &aptXParams, 1);
#endif
    if( NULL == pAllocatedMemory )
      return AX_ERROR ;

#ifdef ADAPTIVE_R2_BUILD
    err = aptXEncode_Create( &pEncoderState, aptXParams, NULL, aptx_data->codecMode, pAllocatedMemory ) ;
#else
    err = aptXEncode_Create( &pEncoderState, &aptXParams, aptx_data->codecMode, pAllocatedMemory ) ;
#endif
    if( AX_OK != err )
      return err ;

    axBuf_t *pInBuf  = &aptx_data->axInBuf;
    axBuf_t *pOutBuf = &aptx_data->axOutBuf;


    aptx_data->codec_data[0] = pEncoderState;
    aptx_data->codec_data[1] = xzppmalloc(SWBS_ENC_INPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM1);    // Input Buffer (read only)
    if( NULL == aptx_data->codec_data[1] )
      return AX_ERROR ;
    aptx_data->codec_data[2] = xzppmalloc(SWBS_ENC_OUTPUT_BUFFER_SIZE, MALLOC_PREFERENCE_DM2);   // Output Buffer
    if( NULL == aptx_data->codec_data[2] )
      return AX_ERROR ;

    int8_t * pBuffer1 = (int8_t*)aptx_data->codec_data[1];
    int8_t * pBuffer2 = (int8_t*)aptx_data->codec_data[2];

	//Create the encoderInput buffer
    AxBufInit( pInBuf) ;
    pInBuf->dat.pn32 = (int32_t*)pBuffer1;
    pInBuf->nRPtr    = (int32_t)pInBuf->dat.pn32;
    pInBuf->nWPtr    = (int32_t)pInBuf->dat.pn32;
    pInBuf->nFill    = (int32_t)pInBuf->dat.pn32 + SWBS_ENC_INPUT_BUFFER_SIZE ;
    pInBuf->nMax     = (int32_t)pInBuf->dat.pn32 + SWBS_ENC_INPUT_BUFFER_SIZE ;
    pInBuf->fmt.flow = AXFMT_LINEAR;
    pInBuf->fmt.type = AXFMT_NOTDEFINED;
    pInBuf->fmt.nSpan = 1;

	//Create the encoderOutput buffer
    AxBufInit( pOutBuf ) ;
    pOutBuf->dat.pn32 = (int32_t*)pBuffer2;
    pOutBuf->nFill    = (int32_t)pOutBuf->dat.pn32 + SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES ;
    pOutBuf->nMax     = (int32_t)pOutBuf->dat.pn32 + (SWBS_DEFAULT_ENCODED_BLOCK_SIZE_IN_BYTES + APTX_SPECTIVE_MAGEN) ;
    pOutBuf->fmt.flow = AXFMT_LINEAR;
    pOutBuf->fmt.type = AXFMT_STEREO_32BIT;
    pOutBuf->fmt.nSpan = 1;

    return err;
}

bool swbs_enc_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned swbs_in_size = SWBS_ENC_INPUT_BUFFER_SIZE;
    unsigned swbs_out_size = SWBS_ENC_OUTPUT_BUFFER_SIZE;

    /* for decoder, the output size might have been configured by the user */
    SWBS_ENC_OP_DATA* wbs_data = (SWBS_ENC_OP_DATA*)base_op_get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA* sco_data = &(wbs_data->sco_send_op_data);
    swbs_in_size = MAX(swbs_in_size, sco_data->input_buffer_size);
    swbs_out_size = MAX(swbs_out_size, sco_data->output_buffer_size);


    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_in_size;
        L4_DBG_MSG2("swbs_enc_buffer_details (capID=%d)  input buffer size=%d \n", base_op_get_cap_id(op_data), swbs_in_size);
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = swbs_out_size;
        L4_DBG_MSG2("swbs_enc_buffer_details (capID=%d)  output buffer size=%d \n", base_op_get_cap_id(op_data), swbs_out_size);
    }

    /* supports metadata in both side for encode and decode */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = NULL;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;

    return TRUE;
}
