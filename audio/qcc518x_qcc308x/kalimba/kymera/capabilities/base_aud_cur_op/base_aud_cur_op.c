/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  base_aud_cur_op.c
 * \ingroup  base_aud_cur_op
 *
 *  Audio curation base operations. This is not a complete capability, it
 *  contains common operations utilized by audio curation capabilities.
 */

/******************************************************************************
Include Files
*/
#include "base_aud_cur_op.h"

/****************************************************************************
Private Function Declarations
*/

/**
 * \brief Set the class data
 *
 * \param  op_data       Pointer to operator structure
 * \param  class_data    Pointer to the class data
 *
 * \return - NONE
 */
static inline void set_class_data(OPERATOR_DATA *op_data,
                                  AUDIO_CURATION_DEF *class_data)
{
    base_op_set_class_ext(op_data, class_data);
}

/**
 * \brief Get a particular terminal buffer pointer
 *
 * \param p_def         Terminal (sink/source) definition
 * \param num           Terminal number to get
 *
 * \return - pointer to the terminal cbuffer
 */
static inline tCbuffer *aud_cur_get_terminal(AUD_CUR_TERMINAL *p_def,
                                             uint16 num)
{
    return p_def->p_buffer_list[num];
}

/**
 * \brief  Determine whether a terminal is connected
 *
 * \param  p_def        Terminal (sink/source) definition
 * \param  num          Terminal number to test
 *
 * \return - TRUE if the terminal is connected
 */
static inline bool aud_cur_is_terminal_connected(AUD_CUR_TERMINAL *p_def,
                                                 uint16 num)
{
    return (p_def->connected & (1 << num)) > 0;
}

/**
 * \brief Connect a particular terminal number
 *
 * \param p_def        Terminal (sink/source) definition
 * \param num          Terminal number to connect
 * \param p_buffer     Pointer to the terminal cbuffer
 *
 * \return - NONE
 */
static void aud_cur_connect_terminal(AUD_CUR_TERMINAL *p_def,
                                     uint16 num,
                                     tCbuffer *p_buffer)
{
    p_def->p_buffer_list[num] = p_buffer;
    p_def->connected |= (uint16)(1 << num);
    return;
}

/**
 * \brief Disconnect a particular terminal number
 *
 * \param p_def     Terminal (sink/source) definition
 * \param num       Terminal number to disconnect
 *
 * \return - NONE
 */
static void aud_cur_disconnect_terminal(AUD_CUR_TERMINAL *p_def,
                                        uint16 num)
{
    p_def->p_buffer_list[num] = NULL;
    p_def->connected &= (uint16)(~(1 << num));
    return;
}
/**
 * \brief Update metadata buffers at connect
 *
 * \param p_metadata_list   Pointer to the metadata buffer list
 * \param num               Terminal number to connect
 * \param p_buffer          Pointer to the terminal cbuffer
 *
 * \return - NONE
 */
static void aud_cur_connect_metadata(tCbuffer **p_metadata_list,
                                     uint16 num,
                                     tCbuffer *p_buffer)
{
    unsigned idx;

    /* Select the correct metadata buffer */
    if (num == AUD_CUR_PLAYBACK_TERMINAL)
    {
        idx = AUD_CUR_METADATA_PLAYBACK;
    }
    else
    {
        idx = AUD_CUR_METADATA_MIC;
    }

    /* Populate the metadata buffer */
    if (p_metadata_list[idx] == NULL && buff_has_metadata(p_buffer))
    {
            p_metadata_list[idx] = p_buffer;
    }
}

/**
 * \brief Update metadata buffers at disconnect
 *
 * \param p_metadata_list   Pointer to the metadata buffer list
 * \param p_def             Terminal (sink/source) definition
 * \param num               Terminal number to disconnect
 *
 * \return - None
 */
static void aud_cur_disconnect_metadata(tCbuffer **p_metadata_list,
                                        AUD_CUR_TERMINAL *p_def,
                                        uint16 num)
{
    int i;
    tCbuffer *p_buffer;
    tCbuffer **p_buffer_list;

    /* Playback metadata only travels on a single terminal */
    if (num == AUD_CUR_PLAYBACK_TERMINAL)
    {
        p_metadata_list[AUD_CUR_METADATA_PLAYBACK] = NULL;
        return;
    }

    /* Look for metadata on another terminal if the terminal being used for
     * metadata is being disconnected.
     */
    p_buffer_list = p_def->p_buffer_list;
    if (p_metadata_list[AUD_CUR_METADATA_MIC] == p_buffer_list[num])
    {
        /* Set the metadata buffer to NULL and populate if a replacement is
         * available.
         */
        p_metadata_list[AUD_CUR_METADATA_MIC] = NULL;

        /* Iterate through microphone terminals */
        for (i = (AUD_CUR_PLAYBACK_TERMINAL + 1); i < p_def->max; i++)
        {
            /* Don't look at the terminal that is being disconnected */
            if (i == num)
            {
                continue;
            }
            /* Update the metadata buffer with the new buffer information */
            p_buffer = p_buffer_list[i];
            if (p_buffer != NULL && buff_has_metadata(p_buffer))
            {
                p_metadata_list[AUD_CUR_METADATA_MIC] = p_buffer;
                break;
            }
        }
    }
    return;
}

/**
 * \brief Common wrapper for terminal connect/disconnect
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 * \param  connect          Boolean indicating whether connect (TRUE) or
 *                          disconnect (FALSE)
 *
 * \return - result TRUE if response generated
 */
static bool aud_cur_connect_common(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *response_id,
                                   void **response_data,
                                   bool connect)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    unsigned  terminal_id;
    uint16 terminal_num;
    uint16 terminal_pos;
    bool is_sink;
    tCbuffer **p_metadata_list;
    AUD_CUR_TERMINAL *p_terminal;

    /* Verify class data and create response */
    if ((p_class_data == NULL) ||
        (!base_op_connect(op_data, message_data, response_id, response_data)))
    {
        return FALSE;
    }

    /* Prevent runtime connection */
    if (opmgr_op_is_running(op_data))
    {
        /* Exception: allow runtime disconnection if the flag is set */
        if (connect || !p_class_data->runtime_disconnect)
        {
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }
    }

    /* Get the terminal ID, number, and determine whether sink or source */
    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    terminal_pos = (uint16)AUD_CUR_GET_TERMINAL_POS(terminal_num);
    is_sink = terminal_id & TERMINAL_SINK_MASK;

    /* Setup the selected terminal */
    if (is_sink)
    {
        p_terminal = &p_class_data->sinks;
        p_metadata_list = p_class_data->metadata_ip;
    }
    else
    {
        p_terminal = &p_class_data->sources;
        p_metadata_list = p_class_data->metadata_op;
    }

    /* Make sure the terminal is valid */
    if (terminal_num >= p_terminal->max)
    {
        L4_DBG_MSG1("base aud cur connect: terminal num %d is out \
                    of max range", terminal_num);
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /* Make sure the terminal is not marked as invalid */
    if (p_terminal->max_valid_mask &&
       !(p_terminal->max_valid_mask & terminal_pos))
    {
        L4_DBG_MSG1("base aud cur connect: invalid terminal number %d",
                    terminal_num);
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    if (connect)
    {
        if (aud_cur_is_terminal_connected(p_terminal, terminal_num))
        {
            L4_DBG_MSG1("base aud cur connect: terminal %d already connected",
                        terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        /* Connect the terminal */
        tCbuffer *p_buffer = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
        aud_cur_connect_terminal(p_terminal, terminal_num, p_buffer);
        aud_cur_connect_metadata(p_metadata_list, terminal_num, p_buffer);

        if (p_class_data->connect_fn != NULL)
        {
            p_class_data->connect_fn(op_data, terminal_id);
        }
    }
    else
    {
        if (!aud_cur_is_terminal_connected(p_terminal, terminal_num))
        {
            L4_DBG_MSG1("base aud cur connect: terminal %d not connected",
                        terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        aud_cur_disconnect_metadata(p_metadata_list, p_terminal, terminal_num);
        aud_cur_disconnect_terminal(p_terminal, terminal_num);

        if (p_class_data->disconnect_fn != NULL)
        {
            p_class_data->disconnect_fn(op_data, terminal_id);
        }
    }
    return TRUE;
}


/**
 * \brief Check that the connected terminals are valid to start
 *
 * \param  p_term           Pointer to the terminal information
 *
 * \return - result TRUE if the terminals are valid
 */
static bool aud_cur_check_valid_terminals(AUD_CUR_TERMINAL *p_term)
{
    /* No validity mask to test */
    if (p_term->min_valid_mask == 0)
    {
        return TRUE;
    }

    /* Connection mask doesn't have at least the valid mask bits */
    if ((p_term->min_valid_mask & p_term->connected) != p_term->min_valid_mask)
    {
        L4_DBG_MSG1("base aud cur start: invalid terminals %hu",
                    p_term->connected);
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
Public Function Declarations
*/

void aud_cur_set_callbacks(OPERATOR_DATA *op_data,
                           AUD_CUR_START_FN start_fn,
                           AUD_CUR_STOP_FN stop_fn,
                           AUD_CUR_CONNECT_FN connect_fn,
                           AUD_CUR_DISCONNECT_FN disconnect_fn,
                           AUD_CUR_PARAM_UPDATE_FN param_update_fn)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    p_class_data->start_fn = start_fn;
    p_class_data->stop_fn = stop_fn;
    p_class_data->connect_fn = connect_fn;
    p_class_data->disconnect_fn = disconnect_fn;
    p_class_data->param_update_fn = param_update_fn;

    return;
}

void aud_cur_set_buffer_size(OPERATOR_DATA *op_data, unsigned buffer_size)
{
   AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
   p_class_data->buffer_size  = buffer_size;
}

void aud_cur_set_block_size(OPERATOR_DATA *op_data, unsigned block_size)
{
   AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
   p_class_data->block_size  = block_size;
}

void aud_cur_set_flags(OPERATOR_DATA *op_data,
                       bool in_place,
                       bool supports_metadata,
                       bool dynamic_buffer_size)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    p_class_data->in_place_flag = in_place;
    p_class_data->supports_metadata_flag = supports_metadata;
    p_class_data->dynamic_buffer_size_flag = dynamic_buffer_size;

    return;
}

void aud_cur_set_runtime_disconnect(OPERATOR_DATA *op_data,
                                    bool allowed)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    p_class_data->runtime_disconnect = allowed;
}


void aud_cur_set_min_terminal_masks(OPERATOR_DATA *op_data,
                                    uint16 source_mask,
                                    uint16 sink_mask)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    p_class_data->sources.min_valid_mask = source_mask;
    p_class_data->sinks.min_valid_mask = sink_mask;

    return;
}

void aud_cur_set_max_terminal_masks(OPERATOR_DATA *op_data,
                                    uint16 source_mask,
                                    uint16 sink_mask)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    p_class_data->sources.max_valid_mask = source_mask;
    p_class_data->sinks.max_valid_mask = sink_mask;

    return;
}

CPS_PARAM_DEF *aud_cur_get_cps(OPERATOR_DATA *op_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return &p_class_data->param_def;
}

bool aud_cur_create(OPERATOR_DATA *op_data,
                    unsigned max_sources,
                    unsigned max_sinks)
{

    unsigned input_size, output_size;
    AUDIO_CURATION_DEF *ptr;

    /* Allocate class data including space for linked lists */
    output_size = max_sources * sizeof(tCbuffer);
    input_size = max_sinks * sizeof(tCbuffer);

    ptr = (AUDIO_CURATION_DEF*)xzpmalloc(sizeof(AUDIO_CURATION_DEF) + \
        input_size + output_size);

    if (ptr == NULL)
    {
          L4_DBG_MSG("base aud cur create: class allocation failed.");
          return FALSE;
    }

    ptr->sources.max = (uint16)max_sources;
    ptr->sinks.max = (uint16)max_sinks;

    ptr->sinks.p_buffer_list = (tCbuffer**)&ptr->buffer_data;
    ptr->sources.p_buffer_list = ptr->sinks.p_buffer_list + max_sources;

    ptr->buffer_size = 2 * AUD_CUR_DEFAULT_BLOCK_SIZE;
    ptr->block_size  = AUD_CUR_DEFAULT_BLOCK_SIZE;

    ptr->cap_id = base_op_get_cap_id(op_data);

    ptr->re_init_flag = TRUE;

    /* Save Pointer to channel definition in operator data */
    set_class_data(op_data, ptr);

    return TRUE;
}

void aud_cur_destroy(OPERATOR_DATA *op_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    if (p_class_data == NULL)
    {
        return;
    }

    pfree(p_class_data);
    set_class_data(op_data, NULL);
    return;
}

bool aud_cur_connect(OPERATOR_DATA *op_data,
                     void *message_data,
                     unsigned *response_id,
                     void **response_data)
{
    return aud_cur_connect_common(op_data,
                                  message_data,
                                  response_id,
                                  response_data,
                                  TRUE);
}

bool aud_cur_disconnect(OPERATOR_DATA *op_data,
                        void *message_data,
                        unsigned *response_id,
                        void **response_data)
{
    return aud_cur_connect_common(op_data,
                                  message_data,
                                  response_id,
                                  response_data,
                                  FALSE);
}

bool aud_cur_buffer_details(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *response_id,
                            void **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    OP_BUF_DETAILS_RSP *p_resp;
    AUD_CUR_TERMINAL *p_opposite_terminal;
    tCbuffer **p_metadata;
    unsigned terminal_id, buffer_size;
    uint16 terminal_num;
    bool is_sink;

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }

    p_resp = (OP_BUF_DETAILS_RSP*) *response_data;

    /* Make sure the buffer size is at least adequate for the capability */
    buffer_size = p_resp->b.buffer_size;
    if (buffer_size < p_class_data->buffer_size)
    {
        buffer_size = p_class_data->buffer_size;
    }

    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    is_sink = terminal_id & TERMINAL_SINK_MASK;

    /* Setup the selected terminal */
    if (is_sink)
    {
        /* Select source (opposite to the given buffer) */
        p_opposite_terminal = &p_class_data->sources;
        p_metadata = p_class_data->metadata_ip;
    }
    else
    {
        /* Select sink (opposite to the given buffer) */
        p_opposite_terminal = &p_class_data->sinks;
        p_metadata = p_class_data->metadata_op;
    }

    if (p_class_data->in_place_flag)
    {
        /* Make sure the terminal is valid */
        if (terminal_num >= p_opposite_terminal->max)
        {
            L4_DBG_MSG1("base aud cur details: invalid terminal number %d",
                        terminal_num);
            base_op_change_response_status(response_data, STATUS_CMD_FAILED);
            return TRUE;
        }

        /* Setup in-place payload */
        p_resp->runs_in_place = TRUE;
        p_resp->b.in_place_buff_params.in_place_terminal = \
            SWAP_TERMINAL_DIRECTION(terminal_id);
        p_resp->b.in_place_buff_params.size = buffer_size;
        p_resp->b.in_place_buff_params.buffer = \
            aud_cur_get_terminal(p_opposite_terminal, terminal_num);
    }
    else
    {
        p_resp->runs_in_place = FALSE;
        p_resp->b.buffer_size = buffer_size;
    }

    /* Populate metadata response */
    p_resp->supports_metadata = p_class_data->supports_metadata_flag;
    if (p_class_data->supports_metadata_flag)
    {
        if (terminal_num == AUD_CUR_PLAYBACK_TERMINAL)
        {
            p_resp->metadata_buffer = p_metadata[AUD_CUR_METADATA_PLAYBACK];
        }
        else
        {
            p_resp->metadata_buffer = p_metadata[AUD_CUR_METADATA_MIC];
        }
    }

    return TRUE;
}

bool aud_cur_start(OPERATOR_DATA *op_data,
                   void *message_data,
                   unsigned *response_id,
                   void **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    /* Create the response. If there aren't sufficient resources for this fail
     * early.
     */
    if (!base_op_start(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    /* If already running just ack */
    if (opmgr_op_is_running(op_data))
    {
       return TRUE;
    }

    /* Make sure we have valid terminal connections */
    if (!aud_cur_check_valid_terminals(&p_class_data->sinks) ||
        !aud_cur_check_valid_terminals(&p_class_data->sources))
    {
        base_op_change_response_status(response_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    if (p_class_data->start_fn != NULL)
    {
        p_class_data->start_fn(op_data);
    }

    /* Reinitialize the operator */
    p_class_data->re_init_flag = TRUE;

    return TRUE;
}

bool aud_cur_stop(OPERATOR_DATA *op_data,
                  void *message_data,
                  unsigned *response_id,
                  void **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_stop(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    if (p_class_data->stop_fn != NULL)
    {
        p_class_data->stop_fn(op_data);
    }

    return TRUE;
}

bool aud_cur_reset(OPERATOR_DATA *op_data,
                   void *message_data,
                   unsigned *response_id,
                   void **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_reset(op_data, message_data, response_id, response_data))
    {
        return FALSE;
    }

    p_class_data->re_init_flag = TRUE;

    return TRUE;
}

bool aud_cur_opmsg_set_buffer_size(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    unsigned buffer_size;

    if (opmgr_op_is_running(op_data))
    {
        L2_DBG_MSG("base aud cur set buffer size failed: operator running");
        return FALSE;
    }

    if (!p_class_data->dynamic_buffer_size_flag)
    {
        L2_DBG_MSG("base aud cur set buffer size failed: unsupported");
        return FALSE;
    }

    if ((p_class_data->sinks.connected > 0) ||
        (p_class_data->sources.connected > 0))
    {
        L2_DBG_MSG("base aud cur set buffer size failed: already connected");
        return FALSE;
    }

    buffer_size = OPMSG_FIELD_GET(message_data,
                                  OPMSG_COMMON_SET_BUFFER_SIZE,
                                  BUFFER_SIZE);
    aud_cur_set_buffer_size(op_data, buffer_size);

    return TRUE;
}

bool aud_cur_get_sched_info(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *response_id,
                            void **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED,
                                             response_data);
    }

    *response_data = resp;
    resp->block_size = p_class_data->block_size;

    return TRUE;
}

tCbuffer *aud_cur_get_source_terminal(OPERATOR_DATA *op_data, uint16 id)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return aud_cur_get_terminal(&p_class_data->sources, id);
}

tCbuffer *aud_cur_get_sink_terminal(OPERATOR_DATA *op_data, uint16 id)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return aud_cur_get_terminal(&p_class_data->sinks, id);
}

/****************************************************************************
CPS Function Declarations
*/

bool aud_cur_opmsg_get_params(OPERATOR_DATA *op_data,
                              void *message_data,
                              unsigned *resp_length,
                              OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return cpsGetParameterMsgHandler(&p_class_data->param_def, message_data,
                                     resp_length, response_data);
}

bool aud_cur_opmsg_get_defaults(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return cpsGetDefaultsMsgHandler(&p_class_data->param_def, message_data,
                                    resp_length, response_data);
}

bool aud_cur_opmsg_set_params(OPERATOR_DATA *op_data,
                              void *message_data,
                              unsigned *resp_length,
                              OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    bool success;

    success = cpsSetParameterMsgHandler(&p_class_data->param_def, message_data,
                                        resp_length, response_data);

    if (success)
    {
        if (p_class_data->param_update_fn != NULL)
        {
            p_class_data->param_update_fn(op_data);
        }
        /* Set re-initialization flag */
        p_class_data->re_init_flag = TRUE;
    }

    return success;
}

bool aud_cur_ups_params(void* instance_data,
                        PS_KEY_TYPE key,
                        PERSISTENCE_RANK rank,
                        uint16 length,
                        unsigned* data,
                        STATUS_KYMERA status,
                        uint16 extra_status_info)
{
    OPERATOR_DATA *op_data = (OPERATOR_DATA*)instance_data;
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    cpsSetParameterFromPsStore(&p_class_data->param_def, length, data, status);

    if (p_class_data->param_update_fn != NULL)
    {
        p_class_data->param_update_fn(op_data);
    }

    /* Set the re-init flag after the parameters are updated. */
    p_class_data->re_init_flag = TRUE;

    return TRUE;
}

bool aud_cur_opmsg_set_ucid(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *resp_length,
                            OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    PS_KEY_TYPE key;
    bool success;

    success = cpsSetUcidMsgHandler(&p_class_data->param_def, message_data,
                                  resp_length, response_data);
    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(p_class_data->cap_id,
                                         p_class_data->param_def.ucid,
                                         OPMSG_P_STORE_PARAMETER_SUB_ID);

    ps_entry_read((void*)op_data, key, PERSIST_ANY, aud_cur_ups_params);

    return success;
}

bool aud_cur_opmsg_get_ps_id(OPERATOR_DATA *op_data,
                             void *message_data,
                             unsigned *resp_length,
                             OP_OPMSG_RSP_PAYLOAD **response_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return cpsGetUcidMsgHandler(&p_class_data->param_def, p_class_data->cap_id,
                                message_data, resp_length, response_data);
}

/****************************************************************************
Process Data
*/

unsigned aud_cur_calc_samples(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);

    unsigned min_data, min_space, block_size, amount, i, samples;
    tCbuffer **p_inputs, **p_outputs;

    /* If no input terminal connections then do nothing */
    if (p_class_data->sinks.connected == 0)
    {
        return 0;
    }

    /* Initialize data */
    block_size = p_class_data->block_size;

    /* Find the minimum amount of data available in the input buffers */
    min_data = UINT_MAX;
    p_inputs = p_class_data->sinks.p_buffer_list;

    for (i = 0; i < p_class_data->sinks.max; i++)
    {
        if (p_inputs[i] != NULL)
        {
            amount = cbuffer_calc_amount_data_in_words(p_inputs[i]);
            if (amount < min_data)
            {
                /* Need at least one block of data available */
                if (amount < block_size)
                {
                    return 0;
                }
                min_data = amount;
            }
        }
    }

    /* Find the minimum amount of space available at the output buffers */
    min_space = UINT_MAX;
    p_outputs = p_class_data->sources.p_buffer_list;

    if (p_class_data->sources.connected > 0)
    {
        for (i = 0; i < p_class_data->sources.max; i++)
        {
            if (p_outputs[i] != NULL)
            {
                amount = cbuffer_calc_amount_space_in_words(p_outputs[i]);
                if (amount < min_space)
                {
                    /* Need at least one block of space available */
                    if (amount < block_size)
                    {
                        return 0;
                    }
                    min_space = amount;
                }
            }
        }
    }

    /* Update kick flags */
    touched->sources = p_class_data->sources.connected;

    /* Samples to process is the smaller of data or space available */
    if (min_data < min_space)
    {
        samples = min_data;
    }
    else
    {
        samples = min_space;
    }

    /* If there is less than a block left then kick backwards */
    if (min_data - samples < block_size)
    {
        touched->sinks = p_class_data->sinks.connected;
    }

    return samples;
}

unsigned aud_cur_mic_data_transfer(OPERATOR_DATA *op_data,
                                   unsigned amount,
                                   unsigned terminal_skip_mask)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    unsigned i, mic_amt, temp_amt;
    tCbuffer **ip_buffers, **op_buffers;

    ip_buffers = p_class_data->sinks.p_buffer_list;
    op_buffers = p_class_data->sources.p_buffer_list;

    /* Copy mic stream data */
    mic_amt = amount;

    for (i = 1; i < p_class_data->sinks.max; i++)
    {
        if(!(AUD_CUR_GET_TERMINAL_POS(i) & terminal_skip_mask))
        {
            /* Perform copy/advance if terminal number (i) is not in
             * terminal_skip_mask
             */
            temp_amt = amount;
            if (ip_buffers[i] != NULL)
            {
                if (op_buffers[i] != NULL)
                {
                    temp_amt = cbuffer_copy(op_buffers[i],
                                            ip_buffers[i],
                                            amount);
                }
                else
                {
                    cbuffer_advance_read_ptr(ip_buffers[i], amount);
                }
                if (temp_amt < mic_amt)
                {
                    mic_amt = temp_amt;
                }
            }
        }
    }

    return mic_amt;

}

unsigned aud_cur_mic_metadata_transfer(OPERATOR_DATA *op_data, unsigned amount)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    tCbuffer *p_metadata_ip, *p_metadata_op;
    /* Only copy metadata if samples were transferred */
    if (amount > 0)
    {
        p_metadata_ip = p_class_data->metadata_ip[AUD_CUR_METADATA_MIC];
        p_metadata_op = p_class_data->metadata_op[AUD_CUR_METADATA_MIC];

        metadata_strict_transport(p_metadata_ip,
                                  p_metadata_op,
                                  amount * OCTETS_PER_SAMPLE);
    }
    return amount;
}

unsigned aud_cur_playback_data_transfer(OPERATOR_DATA *op_data, unsigned amount)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    unsigned playback_amt;

    tCbuffer **ip_buffers, **op_buffers;

    ip_buffers = p_class_data->sinks.p_buffer_list;
    op_buffers = p_class_data->sources.p_buffer_list;

    playback_amt = amount;

    /* Copy playback data. If there is no playback input then no data is
     * copied.
     */
    if (ip_buffers[AUD_CUR_PLAYBACK_TERMINAL] == NULL)
    {
        playback_amt = 0;
    }
    else
    {
        if (op_buffers[AUD_CUR_PLAYBACK_TERMINAL] != NULL)
        {
            playback_amt = cbuffer_copy(op_buffers[AUD_CUR_PLAYBACK_TERMINAL],
                                        ip_buffers[AUD_CUR_PLAYBACK_TERMINAL],
                                        amount);
        }
        else
        {
            cbuffer_advance_read_ptr(ip_buffers[AUD_CUR_PLAYBACK_TERMINAL],
                                     amount);
        }
    }

    return playback_amt;
}

unsigned aud_cur_playback_metadata_transfer(OPERATOR_DATA *op_data,
                                            unsigned amount)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    tCbuffer *p_metadata_ip, *p_metadata_op;
    /* Only copy metadata if samples were transferred */
    if (amount > 0)
    {
        p_metadata_ip = p_class_data->metadata_ip[AUD_CUR_METADATA_PLAYBACK];
        p_metadata_op = p_class_data->metadata_op[AUD_CUR_METADATA_PLAYBACK];
        metadata_strict_transport(p_metadata_ip,
                                  p_metadata_op,
                                  amount * OCTETS_PER_SAMPLE);
    }
    return amount;
}

bool aud_cur_create_cbuffer(tCbuffer **pp_buffer,
                            unsigned size,
                            unsigned malloc_pref)
{
    /* Allocate buffer memory explicitly */
    int *ptr = xzppnewn(size, int, malloc_pref);

    if (ptr == NULL)
    {
        return FALSE;
    }

    /* Wrap allocated memory in a cbuffer */
    *pp_buffer = cbuffer_create(ptr, size, BUF_DESC_SW_BUFFER);
    if (*pp_buffer == NULL)
    {
        pdelete(ptr);
        ptr = NULL;

        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
Inter-capability messaging
*/

bool aud_cur_release_shared_gain_cback(CONNECTION_LINK con_id,
                                       STATUS_KYMERA status,
                                       EXT_OP_ID op_id,
                                       unsigned num_resp_params,
                                       unsigned *resp_params)
{
    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG2("aud_cur unlink response failed: status=%d, op_id=%d",
                    status,
                    op_id);
        return FALSE;
    }
    return TRUE;
}

void aud_cur_release_shared_fine_gain(AHM_SHARED_FINE_GAIN *p_gain,
                                      AHM_ANC_FILTER filter,
                                      AHM_GAIN_CONTROL_TYPE gain_type,
                                      uint16 ahm_op_id,
                                      AHM_ANC_INSTANCE anc_instance
                                      )
{
    unsigned msg[OPMSG_FREE_AHM_SHARED_GAIN_PTR_WORD_SIZE];
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                             MESSAGE_ID,
                             OPMSG_AHM_ID_FREE_AHM_SHARED_GAIN_PTR);
    OPMSG_CREATION_FIELD_SET32(msg,
                               OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                               SHARED_GAIN_PTR,
                               (unsigned)p_gain);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                             FILTER,
                             filter);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                             CHANNEL,
                             anc_instance);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                             COARSE,
                             FALSE);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                             CONTROL_TYPE,
                             gain_type);

    opmgr_operator_message(ADAPTOR_INTERNAL,
                           ahm_op_id,
                           OPMSG_FREE_AHM_SHARED_GAIN_PTR_WORD_SIZE,
                           (unsigned*)&msg,
                           aud_cur_release_shared_gain_cback);
}

void aud_cur_get_shared_fine_gain(void *p_ext_data,
                                  AHM_ANC_FILTER filter,
                                  unsigned op_id,
                                  AHM_GAIN_CONTROL_TYPE gain_type,
                                  AHM_ANC_INSTANCE anc_instance,
                                  OP_MSG_CBACK callback)
{
    unsigned msg[OPMSG_COMMON_MSG_GET_SHARED_GAIN_WORD_SIZE];
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                             MESSAGE_ID,
                             OPMSG_COMMON_ID_GET_SHARED_GAIN);
    OPMSG_CREATION_FIELD_SET32(msg,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                               P_EXT_DATA,
                               (unsigned)p_ext_data);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                             FILTER,
                             filter);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                             CHANNEL,
                             anc_instance);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                             COARSE,
                             FALSE);
    OPMSG_CREATION_FIELD_SET(msg,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                             CONTROL_TYPE,
                             gain_type);
    opmgr_operator_message(ADAPTOR_INTERNAL,
                           op_id,
                           OPMSG_COMMON_MSG_GET_SHARED_GAIN_WORD_SIZE,
                           (unsigned*)&msg,
                           callback);
}
