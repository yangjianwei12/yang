/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_send.c
 * \ingroup  operators
 *
 *  SCO send operator
 *
 */
/****************************************************************************
Include Files
*/
#include "sco_nb_private.h"
#include "capabilities.h"
#include "mem_utils/scratch_memory.h"

#ifdef CVSD_CODEC_SOFTWARE
#include "cvsd.h"
#include "math.h"
#endif


/****************************************************************************
Private Constant Definitions
*/

/* Minimum number of samples to process */
#define SCO_SEND_MIN_BLOCK_SIZE             (SCO_MIN_BLOCK_SIZE * SW_CVSD_RATIO)
#define SCO_SEND_DEFAULT_BLOCK_SIZE         (SCO_DEFAULT_BLOCK_SIZE * SW_CVSD_RATIO)

/** Default buffer sizes for sco send */
#define SCO_SEND_INPUT_BUFFER_SIZE          (128)
#define SCO_SEND_OUTPUT_BUFFER_SIZE         (SCO_DEFAULT_SCO_BUFFER_SIZE)

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define SCO_SEND_CAP_ID CAP_ID_DOWNLOAD_SCO_SEND
#else
#define SCO_SEND_CAP_ID CAP_ID_SCO_SEND
#endif

/****************************************************************************
Private Type Definitions
*/

/****************************************************************************
Private Constant Declarations
*/
/** The SCO send capability function handler table */
const handler_lookup_struct sco_send_handler_table =
{
    sco_send_create,          /* OPCMD_CREATE */
    sco_send_destroy,          /* OPCMD_DESTROY */
    sco_send_start,           /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    sco_send_reset,           /* OPCMD_RESET */
    sco_send_connect,         /* OPCMD_CONNECT */
    sco_send_disconnect,      /* OPCMD_DISCONNECT */
    sco_send_buffer_details,  /* OPCMD_BUFFER_DETAILS */
    sco_send_get_data_format, /* OPCMD_DATA_FORMAT */
    sco_send_get_sched_info   /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry sco_send_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,    base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_FADEOUT_ENABLE,             sco_send_opmsg_enable_fadeout},
    {OPMSG_COMMON_ID_FADEOUT_DISABLE,            sco_send_opmsg_disable_fadeout},
    {OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE ,  sco_common_send_opmsg_set_terminal_buffer_size},
    {0, NULL}};


const CAPABILITY_DATA sco_send_cap_data =
{
    SCO_SEND_CAP_ID,            /* Capability ID */
    0, 1,                           /* Version information - hi and lo parts */
    1, 1,                           /* Max number of sinks/inputs and sources/outputs */
    &sco_send_handler_table,        /* Pointer to message handler function table */
    sco_send_opmsg_handler_table,   /* Pointer to operator message handler function table */
    sco_send_process_data,          /* Pointer to data processing function */
    0,                              /* Reserved */
    sizeof(SCO_SEND_OP_DATA)        /* Size of capability-specific per-instance data */
};
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_SCO_SEND, SCO_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_SCO_SEND, SCO_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
Private Function Declarations
*/
/* ******************************* Helper functions ************************************ */
static inline SCO_SEND_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_SEND_OP_DATA *) base_op_get_instance_data(op_data);
}


/* initialise various working data params of the specific operator */
static void sco_send_reset_working_data(OPERATOR_DATA *op_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;


    if(x_data != NULL)
    {
        /* Initialise fadeout-related parameters */
        sco_data->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        sco_data->fadeout_parameters.fadeout_counter = 0;
        sco_data->fadeout_parameters.fadeout_flush_count = 0;
        sco_common_send_init_metadata(&sco_data->enc_ttp);
    }
}

/* ********************************** API functions ************************************* */

bool sco_send_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_SEND_OP_DATA* sco_rcv = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_common = &sco_rcv->sco_send_op_data;

    sco_pio_init_pio_enc();

    /* call base_op create, which also allocates and fills response message */
    if(!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

    /* initialise specific data (was allocated and pointer to it filled by OpMgr  */
    sco_send_reset_working_data(op_data);

    /* initialize common sco data */
    sco_common->input_buffer_size = SCO_SEND_INPUT_BUFFER_SIZE;
    sco_common->output_buffer_size = SCO_SEND_OUTPUT_BUFFER_SIZE;

#ifdef CVSD_CODEC_SOFTWARE
    if (!scratch_register())
    {
        return TRUE;
    }
    if (!scratch_reserve(SCRATCH_SIZE_WORDS, MALLOC_PREFERENCE_NONE))
    {
        return(FALSE);
    }
#endif /* CVSD_CODEC_SOFTWARE */

    return TRUE;
}

bool sco_send_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, response_data))
    {
        return(FALSE);
    }

#ifdef CVSD_CODEC_SOFTWARE
    scratch_deregister();
#endif
    return TRUE;
}


bool sco_send_reset(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    if(!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* now initialise specific working data */
    sco_send_reset_working_data(op_data);

    return TRUE;
}


bool sco_send_start(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    /* checks whether all terminals are connected, sets operator state to running, builds response message */
    if(!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    if (opmgr_op_is_running(op_data))
    {
        return TRUE;
    }

    if (!sco_data->buffers.op_buffer || !sco_data->buffers.ip_buffer)
    {
        /* The operator is not connected yet. Change the already allocated response to
         * command failed. No extra error info.*/
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* TODO flush input */

    return TRUE;
}

bool sco_send_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    unsigned sco_in_size = SCO_SEND_INPUT_BUFFER_SIZE;
    unsigned sco_out_size = SCO_SEND_OUTPUT_BUFFER_SIZE;

    /* for decoder, the output size might have been configured by the user */
    SCO_SEND_OP_DATA* sco_nb_data = (SCO_SEND_OP_DATA*)base_op_get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA* sco_data = &(sco_nb_data->sco_send_op_data);
    sco_in_size = MAX(sco_in_size, sco_data->input_buffer_size);
    sco_out_size = MAX(sco_out_size, sco_data->output_buffer_size);

    if(!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    if (OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK)
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = sco_in_size;
        L4_DBG_MSG2("sco_enc_buffer_details (capID=%d)  input buffer size=%d \n", base_op_get_cap_id(op_data), sco_in_size);
    }
    else
    {
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size = sco_out_size;
        L4_DBG_MSG2("sco_enc_buffer_details (capID=%d)  output buffer size=%d \n", base_op_get_cap_id(op_data), sco_out_size);
    }

    /* Supports metadata for both encode and decode at input and output */
    ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = NULL;
    ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;

    return TRUE;
}

bool sco_send_connect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    return sco_common_connect(op_data, message_data, response_id, response_data,
            &sco_data->buffers, NULL);
}

bool sco_send_disconnect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    return sco_common_disconnect(op_data, message_data, response_id, response_data,
            &sco_data->buffers, NULL);
}

bool sco_send_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
#ifdef CVSD_CODEC_SOFTWARE
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
        AUDIO_DATA_FORMAT_FIXP, AUDIO_DATA_FORMAT_16_BIT);
#else
    return sco_common_get_data_format(op_data, message_data, response_id, response_data,
            AUDIO_DATA_FORMAT_FIXP, AUDIO_DATA_FORMAT_FIXP);
#endif
}

bool sco_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data)
{
    return sco_common_send_get_sched_info(op_data, message_data, response_id, response_data, 0);
}

/* ************************************* Data processing-related functions and wrappers **********************************/

void sco_send_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    int available_output, available_input;
    int min_samples_in, min_data_out;
    int max_samples_in, max_data_out;

    patch_fn(sco_send_process_data);

    sco_pio_set_pio_enc();

    min_samples_in = SCO_SEND_MIN_BLOCK_SIZE;
    min_data_out = SCO_SEND_MIN_BLOCK_SIZE / SW_CVSD_RATIO;

    max_samples_in = SCO_SEND_DEFAULT_BLOCK_SIZE;
    max_data_out = SCO_SEND_DEFAULT_BLOCK_SIZE / SW_CVSD_RATIO;

    /* work out amount of input data to process,
     * based on output space and input data amount */
    available_output = cbuffer_calc_amount_space_in_words(sco_data->buffers.op_buffer);
    available_input = cbuffer_calc_avail_data_words(sco_data->buffers.ip_buffer);

    L4_DBG_MSG2("sco_send_process_data: Samples to process: %d, "
                "available space %d\n",
                available_input, available_output);

    /* check that we can process a minimum amount of data. */
    while ((available_output >= min_data_out) && (available_input >= min_samples_in))
    {
        int samples_in = min_samples_in;
        int data_out = min_data_out;

        /* check if we can precess more data than the minimum. */
        if ((available_output >= max_data_out) && (available_input >= max_samples_in))
        {
            samples_in = max_samples_in;
            data_out = max_data_out;
        }

        /* Is fadeout enabled? if yes, do it on the current input data */
        if(sco_data->fadeout_parameters.fadeout_state != NOT_RUNNING_STATE)
        {
            /* the wrapper below takes operator data -
             * this might be lifted out to a common function */
            if (mono_cbuffer_fadeout(sco_data->buffers.ip_buffer, samples_in,
                                     &sco_data->fadeout_parameters))
            {
                common_send_simple_unsolicited_message(op_data,
                                                       OPMSG_REPLY_ID_FADEOUT_DONE);
            }
        }

        sco_common_send_transport_metadata(&sco_data->buffers, &sco_data->enc_ttp,
                                          samples_in,
                                          data_out * cbuffer_get_usable_octets(sco_data->buffers.op_buffer),
                                          NBS_SAMPLE_RATE);

#ifdef CVSD_CODEC_SOFTWARE
        x_data->ptScratch = (int*)scratch_commit(SCRATCH_SIZE_WORDS,
                                                 MALLOC_PREFERENCE_NONE);
        cvsd_send_asm(&x_data->cvsd_struct,
                      sco_data->buffers.ip_buffer,
                      sco_data->buffers.op_buffer,
                      x_data->ptScratch,
                      samples_in);

#ifdef LOGOUTPUT
        for (int i = 0; i < samples_in * 8; i++) {
            L3_DBG_MSG2("%d %x\n", i, x_data->ptScratch[i]);
        }
#endif

        scratch_free();
#else
        /* Copy a frame's worth of data to the output buffer */
        cbuffer_copy(sco_data->buffers.op_buffer, sco_data->buffers.ip_buffer, data_out);
#endif

        available_input -= samples_in;
        available_output -= data_out;

        /* Something was produced, kick forward. */
        touched->sources =  TOUCHED_SOURCE_0;
    }

    L4_DBG_MSG1("sco_send_process_data POST: Samples left to process: %d\n",
                available_input);

    if (available_input < min_samples_in)
    {
        /* kick backwards, as we don't have data for another run */
        touched->sinks = TOUCHED_SINK_0;
    }

    sco_pio_clr_pio_enc();
}



/* **************************** Operator message handlers ******************************** */


bool sco_send_opmsg_enable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;

    common_set_fadeout_state(&sco_data->fadeout_parameters, RUNNING_STATE);

    return TRUE;
}


bool sco_send_opmsg_disable_fadeout(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_SEND_OP_DATA* x_data = get_instance_data(op_data);
    SCO_COMMON_SEND_OP_DATA *sco_data = &x_data->sco_send_op_data;
    common_set_fadeout_state(&sco_data->fadeout_parameters, NOT_RUNNING_STATE);

    return TRUE;
}

