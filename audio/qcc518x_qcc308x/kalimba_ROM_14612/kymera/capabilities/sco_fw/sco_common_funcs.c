/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  sco_common_funcs.c
 * \ingroup  capabilities
 *
 *  Common functions, used by NB and WB SCO capabilities.
 *  Functions "sco_common_rcv_..." are for receive capabilities (SCO_RCV & WBS_DEC).
 *  "sco_common_send_..." are for send capabilities (SCO_SEND, WBS_ENC).
 *  "sco_common_..." are for any SCO capability.
 *
 */

#include "capabilities.h"
#include "sco_common_funcs.h"
#include "ttp/ttp.h"
#include "ttp/timed_playback.h"
#include "ttp_utilities.h"

/****************************************************************************
 * Private Macro Definitions
 */

#define LAST_TAG_SAMPLES_INVALID ((unsigned)-1)

/****************************************************************************
 * Private Function Definitions
 */

static inline SCO_COMMON_RCV_OP_DATA *get_rcv_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_COMMON_RCV_OP_DATA *) base_op_get_instance_data(op_data);
}

static inline SCO_COMMON_SEND_OP_DATA *get_send_instance_data(OPERATOR_DATA *op_data)
{
    return (SCO_COMMON_SEND_OP_DATA *)base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Definitions
*/

/* Init SCO_TERMINAL_BUFFERS info on additional terminals. */
bool sco_common_init_terminals(OPERATOR_DATA *op_data,
                               SCO_TERMINAL_BUFFERS *terminals)
{
    unsigned num_inputs = base_op_get_cap_data(op_data)->max_sinks;
    unsigned num_outputs = base_op_get_cap_data(op_data)->max_sources;

    /* Not supported cases. */
    PL_ASSERT(num_inputs > 0 && num_outputs > 0);
    PL_ASSERT(num_inputs == 1 || num_outputs == 1);

    /* Must not be already initialised. */
    PL_ASSERT(terminals->others == NULL);

    if (num_inputs == 1 && num_outputs == 1)
    {
        /* No additional terminals. Nothing to do. */
        return TRUE;
    }

    unsigned num_additional = num_inputs - 1 + num_outputs - 1;

    terminals->others = xzpmalloc(sizeof(struct ISO_ADDITIONAL_TERMINALS) +
                                  num_additional * sizeof(tCbuffer*));
    if (terminals->others == NULL)
    {
        /* Not enough memory. */
        return FALSE;
    }

    terminals->others->num_additional_terminals = num_additional;
    terminals->others->terminals_are_inputs = (num_inputs > 1 );

    return TRUE;
}

/* Free any allocated memory for additional terminals. */
void sco_common_destroy_terminals(SCO_TERMINAL_BUFFERS *terminals)
{
    pfree(terminals->others);
}

/* Get buffer for input terminal with specified index.
 * If capability doesn't support multiple inputs, and index != 0, returns NULL.
 */
tCbuffer *sco_common_get_input_buffer(SCO_TERMINAL_BUFFERS *terminals,
                                      unsigned index)
{
    if (index == 0)
    {
        /* Return main input buffer. */
        return terminals->ip_buffer;
    }

    if (terminals->others == NULL || !terminals->others->terminals_are_inputs
        || index >= terminals->others->num_additional_terminals + 1)
    {
        /* Not supported. */
        return NULL;
    }

    /* Return specified additional terminal buffer. */
    return terminals->others->additional_terminals[index - 1];
}

/* Get buffer for output terminal with specified index.
 * If capability doesn't support multiple outputs, and index != 0, returns NULL.
 */
tCbuffer *sco_common_get_output_buffer(SCO_TERMINAL_BUFFERS *terminals,
                                       unsigned index)
{
    if (index == 0)
    {
        /* Return main output buffer. */
        return terminals->op_buffer;
    }

    if (terminals->others == NULL || terminals->others->terminals_are_inputs
        || index >= terminals->others->num_additional_terminals + 1)
    {
        /* Not supported. */
        return NULL;
    }

    /* Return specified additional terminal buffer. */
    return terminals->others->additional_terminals[index - 1];
}

/* Get pointer to relevant terminal buffer field in SCO_TERMINAL_BUFFERS. */
static tCbuffer ** get_terminal_buffer_ptr(SCO_TERMINAL_BUFFERS *bufs,
                                           unsigned terminal_id)
{
    unsigned index = terminal_id & TERMINAL_NUM_MASK;

    /* Handle input and output terminals separately. */
    if ((terminal_id & TERMINAL_SINK_MASK) != 0)
    {
        if (index == 0)
        {
            /* Return pointer to main input buffer */
            return &bufs->ip_buffer;
        }
        else if (bufs->others != NULL && bufs->others->terminals_are_inputs
                && index < bufs->others->num_additional_terminals + 1)
        {
            /* Return pointer to additional input buffer */
            return &bufs->others->additional_terminals[index - 1];
        }
        else
        {
            /* Not supported. */
        }
    }
    else
    {
        if (index == 0)
        {
            /* Return pointer to main output buffer */
            return &bufs->op_buffer;
        }
        else if (bufs->others != NULL && !bufs->others->terminals_are_inputs
                && index < bufs->others->num_additional_terminals + 1)
        {
            /* Return pointer to additional output buffer */
            return &bufs->others->additional_terminals[index - 1];
        }
        else
        {
            /* Not supported. */
        }
    }

    return NULL;
}

/* connect terminal to a buffer */
bool sco_common_connect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data, SCO_TERMINAL_BUFFERS *bufs,
        unsigned *terminal)
{
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    tCbuffer **buff_ptr = NULL;

    patch_fn(sco_common_connect);

    /* If the terminal number was requested then return it. */
    if (NULL != terminal)
    {
        *terminal = terminal_id;
    }
    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    buff_ptr = get_terminal_buffer_ptr(bufs, terminal_id);

    if (buff_ptr != NULL && *buff_ptr == NULL)
    {
        /* Set terminal buffer */
        *buff_ptr = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
    }
    else
    {
        /* Buffer already connected or terminal not supported */
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
    }

    return TRUE;
}

/* disconnect terminal from a buffer */
bool sco_common_disconnect(OPERATOR_DATA *op_data, void *message_data,
        unsigned *response_id, void **response_data, SCO_TERMINAL_BUFFERS *bufs,
        unsigned *terminal)
{
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    tCbuffer **buff_ptr = NULL;

    patch_fn(sco_common_disconnect);

    /* If the terminal number was requested then return it. */
    if (NULL != terminal)
    {
        *terminal = terminal_id;
    }

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, response_data))
    {
        return FALSE;
    }

    buff_ptr = get_terminal_buffer_ptr(bufs, terminal_id);

    if (buff_ptr != NULL && *buff_ptr != NULL)
    {
        /* If a terminal is disconnected the sco capabilities
         * can't do anything useful so transition to stopping state to
         * save MIPS and prevent the operator from running with
         * NULL buffers.*/
        base_op_stop_operator(op_data);

        /* Disconnect terminal. */
        *buff_ptr = NULL;
    }
    else
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
    }

    return TRUE;
}

/* initialise some common parts of SCO operator data during creation -
 * it trusts that everything referenced here was allocated before call;
 * assumes that SCO_COMMON_RCV_OP_DATA is zero-initialised on allocation
 */
void sco_common_rcv_initialise(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data)
{
    patch_fn(sco_common_rcv_initialise);

    /* No need to set fields to 0 as already zero-initialised at allocation. */

    /* output buffer size, can be configured later to a larger size */
    sco_rcv_op_data->output_buffer_size = SCO_DEFAULT_SCO_BUFFER_SIZE;
}

/* Destroy any unwanted memory allocation in the receive driver. */
void sco_common_rcv_destroy(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data)
{
    patch_fn(sco_common_rcv_destroy);

    L2_DBG_MSG2("SCO common rcv: frame counter %d, frame error counter %d",
                 sco_rcv_op_data->frame_count, sco_rcv_op_data->frame_error_count);

    /* Make sure that there will be no memory leaks because of the saved tag.
     * It doesn't matter if the disconnect fail, deleting the tag will cause
     * no harm as it only used to speed up processing.*/
    if (sco_rcv_op_data->cur_tag != NULL)
    {
        buff_metadata_delete_tag(sco_rcv_op_data->cur_tag, TRUE);
        sco_rcv_op_data->cur_tag = NULL;
    }
}


/* Initialise various working data params of the NB or WB SCO receive operators. */
bool sco_common_rcv_reset_working_data(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data)
{
    patch_fn(sco_common_rcv_reset_working_data);

    if(sco_rcv_op_data != NULL)
    {
        /* Initialise fadeout-related parameters */
        sco_rcv_op_data->fadeout_parameters.fadeout_state = NOT_RUNNING_STATE;
        sco_rcv_op_data->fadeout_parameters.fadeout_counter = 0;
        sco_rcv_op_data->fadeout_parameters.fadeout_flush_count = 0;
    }

    return TRUE;
}


bool sco_common_rcv_frame_counts_helper(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    patch_fn(sco_common_rcv_frame_counts_helper);

    if (OPMSG_FIELD_GET(message_data, OPMSG_SCO_COMMON_RCV_FRAME_COUNTS, FRAME_COUNTS) != 0)
    {
        /* get the counts - length is payload (4 words) plus echoed msgID/keyID */
        *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(4);
        *resp_data = (OP_OPMSG_RSP_PAYLOAD *)xpnewn(*resp_length, unsigned);
        if (*resp_data == NULL)
        {
            return FALSE;
        }

        /* echo the opmsgID/keyID - 3rd field in the message_data */
        (*resp_data)->msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

        /* get the frame counts */
        (*resp_data)->u.raw_data[0] = sco_rcv_op_data->frame_count >> 16;
        (*resp_data)->u.raw_data[1] = sco_rcv_op_data->frame_count & 0xFFFF;
        (*resp_data)->u.raw_data[2] = sco_rcv_op_data->frame_error_count >> 16;
        (*resp_data)->u.raw_data[3] = sco_rcv_op_data->frame_error_count & 0xFFFF;
    }
    else
    {
        /* set counts to zero */
        sco_rcv_op_data->frame_count = 0;
        sco_rcv_op_data->frame_error_count = 0;
    }

    return TRUE;
}


bool sco_common_get_data_format(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data,
                                AUDIO_DATA_FORMAT input_format,
                                AUDIO_DATA_FORMAT output_format)
{
    patch_fn(sco_common_get_data_format);

    if(!base_op_get_data_format(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* return the terminal's data format - since we are mono-to-mono, easy to
     * get to terminal data purely based on direction flag */
    if((OPMGR_GET_OP_DATA_FORMAT_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK) == 0)
    {
        ((OP_STD_RSP*)*response_data)->resp_data.data = output_format;
    }
    else
    {
        ((OP_STD_RSP*)*response_data)->resp_data.data = input_format;
    }

    return TRUE;
}


bool sco_common_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id,
        void **response_data, unsigned output_block_size)
{
    OP_SCHED_INFO_RSP* resp;

    patch_fn(sco_common_get_sched_info);

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);

    *response_data = resp;
 
    if (resp == NULL)
    {
        return FALSE;
    }

    /* return the output block size - since we are mono-to-mono,
       easy to get to terminal data purely based on direction flag */
    if ((OPMGR_GET_OP_SCHED_INFO_TERMINAL_ID(message_data) & TERMINAL_SINK_MASK) == 0)
    {
        resp->block_size = output_block_size;
    }

    return TRUE;
}

/**
 * sco_common_rcv_opmsg_set_ttp_latency
 * \brief sets the rcv op to generate timestamp tags instead of default toa
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sco_common_rcv_opmsg_set_ttp_latency(OPERATOR_DATA *op_data, void *message_data,
                                     unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    SCO_COMMON_RCV_OP_DATA *sco_common_rcv_data = get_rcv_instance_data(op_data);

    /* We can't change this setting while running */
    if (opmgr_op_is_running(op_data))
    {
        return FALSE;
    }

    /* configure the latency, once set it cannot go back to toa mode,
     * but the latency can change  while the operator isn't running
     */
    sco_common_rcv_data->generate_timestamp = TRUE;
    sco_common_rcv_data->timestamp_latency = ttp_get_msg_latency(message_data);

    L4_DBG_MSG2("sco rcv output ttp latency set, op=0x%04x, latency=%d",
                base_op_get_ext_op_id(op_data),
                sco_common_rcv_data->timestamp_latency);
    return TRUE;
}

/**
 * sco_common_rcv_opmsg_set_buffer_size
 * \brief message handler to set required sco rcv output buffer size
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sco_common_rcv_opmsg_set_buffer_size(OPERATOR_DATA *op_data, void *message_data,
                                          unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    SCO_COMMON_RCV_OP_DATA *sco_common_rcv_data = get_rcv_instance_data(op_data);

    /* We can't change this setting while running */
    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("sco_common_rcv_opmsg_set_buffer_size: Cannot set buffer size because the operator is running!");
        return FALSE;
    }

    if  (sco_common_rcv_data->buffers.op_buffer != NULL)
    {
        L2_DBG_MSG("sco_common_rcv_opmsg_set_buffer_size: Cannot set the buffer size for an operator with connected output!");
        return FALSE;
    }

    /* set the buffer size, it will only be used for PCM terminals */
    sco_common_rcv_data->output_buffer_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_BUFFER_SIZE, BUFFER_SIZE);

    L4_DBG_MSG2("sco rcv output buffer size, op=0x%04x, size=%d",
                base_op_get_ext_op_id(op_data),
                sco_common_rcv_data->output_buffer_size);

    return TRUE;

}

/**
 * sco_common_rcv_opmsg_set_terminal_buffer_size
 * \brief message handler for OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE message,
 *        can be used to configure required buffer size for input and output terminals.
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sco_common_rcv_opmsg_set_terminal_buffer_size(OPERATOR_DATA *op_data, void *message_data,
    unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    SCO_COMMON_RCV_OP_DATA *sco_common_rcv_data = get_rcv_instance_data(op_data);

    /* We can't change this setting while running */
    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("sco_common_rcv_opmsg_set_terminal_buffer_size: Cannot set buffer size because the operator is running!");
        return FALSE;
    }

    /* get the required buffer size */
    unsigned buffer_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE, BUFFER_SIZE);
    /* get the sink terminals that need configuration */
    unsigned sinks = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE,
        SINKS);
    /* get the source terminals that need configuration */
    unsigned sources = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE,
        SOURCES);

    /* Output buffer size is allowd to change if none of outputs are connected */
    if (sources != 0)
    {
        if (NULL != sco_common_rcv_data->buffers.op_buffer)
        {
            return FALSE;
        }
    }

    /* Input buffer size is allowd to change if none of inputs are connected,
     * Note: We allow buffer size change while the operator is running, only the
     * relevant path must be not running.
     */
    if (sinks != 0)
    {
        if (NULL != sco_common_rcv_data->buffers.ip_buffer)
        {
            return FALSE;
        }
        /* Output can change */
    }

    if (sources != 0)
    {
        /* set the output buffer size */
        sco_common_rcv_data->output_buffer_size = buffer_size;
        L2_DBG_MSG1("sco rcv: output buffer size set to %d words ", buffer_size);
    }

    if (sinks != 0)
    {
        sco_common_rcv_data->input_buffer_size = buffer_size;
        L2_DBG_MSG1("sco rcv: input buffer size set to %d words ", buffer_size);
    }

    return TRUE;
}

/**
 * sco_common_send_opmsg_set_terminal_buffer_size
 * \brief message handler for OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE message,
 *        can be used to configure required buffer size for input and output terminals.
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the start request message
 * \param resp_length pointer to location to write the response message length
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool sco_common_send_opmsg_set_terminal_buffer_size(OPERATOR_DATA *op_data, void *message_data,
    unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    SCO_COMMON_SEND_OP_DATA *sco_common_send_data = get_send_instance_data(op_data);

    /* We can't change this setting while running */
    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("sco_common_send_opmsg_set_terminal_buffer_size: Cannot set buffer size because the operator is running!");
        return FALSE;
    }

    /* get the required buffer size */
    unsigned buffer_size = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE, BUFFER_SIZE);
    /* get the sink terminals that need configuration */
    unsigned sinks = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE,
        SINKS);
    /* get the source terminals that need configuration */
    unsigned sources = OPMSG_FIELD_GET(message_data, OPMSG_COMMON_SET_TERMINAL_BUFFER_SIZE,
        SOURCES);

    /* Output buffer size is allowd to change if none of outputs are connected */
    if (sources != 0)
    {
        if (NULL != sco_common_send_data->buffers.op_buffer)
        {
            return FALSE;
        }
    }

    /* Input buffer size is allowd to change if none of inputs are connected,
     * Note: We allow buffer size change while the operator is running, only the
     * relevant path must be not running.
     */
    if (sinks != 0)
    {
        if (NULL != sco_common_send_data->buffers.ip_buffer)
        {
            return FALSE;
        }
        /* Output can change */
    }

    if (sources != 0)
    {
        /* set the output buffer size */
        sco_common_send_data->output_buffer_size = buffer_size;
        L2_DBG_MSG1("sco send: output buffer size set to %d words ", buffer_size);
    }

    if (sinks != 0)
    {
        sco_common_send_data->input_buffer_size = buffer_size;
        L2_DBG_MSG1("sco send: input buffer size set to %d words ", buffer_size);
    }

    return TRUE;

}

/**
 * \brief Advances the read pointer in the input buffer by a given amount of
 *        words.
 *
 * \param sco_rcv_op_data    Pointer to the common sco operator structure.
 * \param amount_to_discard  Number of octets to consume from the input buffer.
 */
void discard_data_octets(SCO_COMMON_RCV_OP_DATA *sco_rcv_op_data,
                         unsigned amount_to_discard)
{
    cbuffer_advance_read_ptr_ex(sco_rcv_op_data->buffers.ip_buffer,
                                amount_to_discard);
}

/**
 * \brief Retrieve the status of the SCO packet data which is
 *        populated by the sco_drv
 *
 * \param mtag            cbuffer metadata tag
 * \param status          status flag extracted
 */
bool sco_common_retrieve_metadata_status(metadata_tag *mtag, metadata_status *status)
{
    unsigned length;
    SCO_PRIVATE_METADATA *priv_metadata;
    if (buff_metadata_find_private_data(mtag, META_PRIV_KEY_SCO_DATA_STATUS, &length, (void **)&priv_metadata))
    {
        /* private data key is obtained successfully */
        *status = priv_metadata->status;
        return TRUE;
    }
    /* private key not found !! */
    return FALSE;
}

/* 
 * sco_common_send_init_metadata
 */
void sco_common_send_init_metadata(SCO_ENC_TTP *enc_ttp)
{
    enc_ttp->last_tag_samples = LAST_TAG_SAMPLES_INVALID;
}

/* 
 * sco_common_send_transport_metadata
 */
void sco_common_send_transport_metadata(SCO_TERMINAL_BUFFERS *buffers, SCO_ENC_TTP *enc_ttp, 
                              unsigned ip_proc_samples, unsigned out_data_octets, unsigned sample_rate)
{
    unsigned ip_proc_data_octets;
    unsigned b4idx, afteridx;
    metadata_tag *mtag_ip, *mtag_ip_list;
    metadata_tag *mtag;
    tCbuffer *src, *dst;
    unsigned new_ttp, base_ttp = 0, sample_offset = 0;
    ttp_status status;

    src = buffers->ip_buffer;
    dst = buffers->op_buffer;
    ip_proc_data_octets = ip_proc_samples * OCTETS_PER_SAMPLE;

    patch_fn_shared(sco_enc_metadata);
    /* Extract metadata tag from input */
    mtag_ip_list = buff_metadata_remove(src, ip_proc_data_octets, &b4idx, &afteridx);

    /* Find the first timestamped tag */
    mtag_ip = mtag_ip_list;
    while ((mtag_ip != NULL) && (!IS_TIME_TO_PLAY_TAG(mtag_ip)))
    {
        b4idx += mtag_ip->length;
        mtag_ip = mtag_ip->next;
    }

    if ((b4idx == 0) && (mtag_ip != NULL))
    {
        /* If the old tag is already at the start of the encoded frame,
         * Just use its timestamp directly
         */
        base_ttp = mtag_ip->timestamp;
        sample_offset = 0;
    }
    else
    {
        /* Otherwise, use the previously-stashed timestamp.
         * There had better be one ! */
        if (enc_ttp->last_tag_samples != LAST_TAG_SAMPLES_INVALID)
        {
            base_ttp = enc_ttp->last_tag_timestamp;
            sample_offset = enc_ttp->last_tag_samples;
        }
    }

    if (mtag_ip != NULL)
    {
        unsigned *err_offset_id;
        unsigned length;
        /* Save the timestamp info from the incoming metadata */
        enc_ttp->last_tag_timestamp = mtag_ip->timestamp;
        enc_ttp->last_tag_spa = mtag_ip->sp_adjust;
        enc_ttp->last_tag_samples = ip_proc_samples - (b4idx / OCTETS_PER_SAMPLE);
        if (buff_metadata_find_private_data(mtag_ip, META_PRIV_KEY_TTP_OFFSET,
                                            &length, (void **)&err_offset_id))
        {
            enc_ttp->last_tag_err_offset_id = (*err_offset_id);
        }
        else
        {
            enc_ttp->last_tag_err_offset_id = INFO_ID_INVALID;
        }
    }
    else
    {
        if (enc_ttp->last_tag_samples != LAST_TAG_SAMPLES_INVALID)
        {
            enc_ttp->last_tag_samples += ip_proc_samples;
        }
    }

    status.sp_adjustment = enc_ttp->last_tag_spa;
    status.err_offset_id = enc_ttp->last_tag_err_offset_id;
    status.stream_restart = ((mtag_ip != NULL)
                             && (METADATA_STREAM_START(mtag_ip) != 0));

    /* Free all the incoming tags */
    buff_metadata_tag_list_delete(mtag_ip_list);

    patch_fn_shared(sco_enc_metadata);
    /* Create a new tag for the output */
    mtag = buff_metadata_new_tag();

    if (mtag != NULL)
    {
        mtag->length = out_data_octets;
        METADATA_PACKET_START_SET(mtag);
        METADATA_PACKET_END_SET(mtag);

        if (enc_ttp->last_tag_samples != LAST_TAG_SAMPLES_INVALID)
        {
            /* Calculate new TTP from incoming data and sample offset */
            new_ttp = ttp_get_next_timestamp(base_ttp,
                                             sample_offset,
                                             sample_rate,
                                             enc_ttp->last_tag_spa);
            new_ttp = time_sub( new_ttp,
                                convert_samples_to_time(enc_ttp->delay_samples,
                                                        sample_rate));
            status.ttp = new_ttp;
            ttp_utils_populate_tag(mtag, &status);
        }
        else
        {
            L4_DBG_MSG("sco_common_send_transport_metadata last tag samples invalid");
        }
    }
    else
    {
        L2_DBG_MSG("sco_common_send_transport_metadata failed to allocate tag");
    }

    buff_metadata_append(dst, mtag, 0, out_data_octets);
    L4_DBG_MSG2("sco_common_send_transport_metadata: TTP:x%x Len:%d",mtag->timestamp,mtag->length);
}

AUDIO_LOG_STRING(status_str_crc, "CRC_ERROR");
AUDIO_LOG_STRING(status_str_nothing, "NOTHING_RECEIVED");
AUDIO_LOG_STRING(status_str_never, "NEVER_SCHEDULED");
AUDIO_LOG_STRING(status_str_wbm, "OK_WBM");
AUDIO_LOG_STRING(status_str_zero_packet, "ZERO_PACKET");
AUDIO_LOG_STRING(status_str_no_packet, "NO_PACKET");
AUDIO_LOG_STRING(status_str_unknown, "UNKNOWN");

/* 
 * sco_common_rcv_print_bad_status
 */
void sco_common_rcv_print_bad_status(EXT_OP_ID opid, metadata_status status, unsigned frame_count)
{
    const char *status_str;

    switch (status)
    {
        case OK:
            /* Not expected here, just ignore it */
            return;
        case CRC_ERROR:
            status_str = status_str_crc;
            break;
        case NOTHING_RECEIVED:
            status_str = status_str_nothing;
            break;
        case NEVER_SCHEDULED:
            status_str = status_str_never;
            break;
        case OK_WBM:
            status_str = status_str_wbm;
            break;
        case ZERO_PACKET:
            status_str = status_str_zero_packet;
            break;
        case NO_PACKET:
            status_str = status_str_no_packet;
            break;
        default:
            status_str = status_str_unknown;
            break;
    }

    L2_DBG_MSG4("SCO/ISO OP 0x%04X packet status = 0x%02X (%s) frame %u",
            opid, status, status_str, frame_count);
}
