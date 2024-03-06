/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  wind_noise_detect.c
 * \ingroup wind_noise_detect
 *
 * Wind Noise Detect (WND) capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "wind_noise_detect.h"

/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define WIND_NOISE_DETECT_CAP_ID   CAP_ID_DOWNLOAD_WIND_NOISE_DETECT
#else
#define WIND_NOISE_DETECT_CAP_ID   CAP_ID_WIND_NOISE_DETECT
#endif

/* Message handlers */
const handler_lookup_struct wnd_handler_table =
{
    wnd_create,                    /* OPCMD_CREATE */
    wnd_destroy,                   /* OPCMD_DESTROY */
    aud_cur_start,                 /* OPCMD_START */
    base_op_stop,                  /* OPCMD_STOP */
    aud_cur_reset,                 /* OPCMD_RESET */
    aud_cur_connect,               /* OPCMD_CONNECT */
    aud_cur_disconnect,            /* OPCMD_DISCONNECT */
    aud_cur_buffer_details,        /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,       /* OPCMD_DATA_FORMAT */
    aud_cur_get_sched_info         /* OPCMD_GET_SCHED_INFO */
};

/* Null-terminated operator message handler table */
const opmsg_handler_lookup_table_entry wnd_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             wnd_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              wnd_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},

    {OPMSG_WND_ID_WND_GET_POWER_INTENSITY,    wnd_opmsg_get_power_intensity},
    {0, NULL}};

const CAPABILITY_DATA wind_noise_detect_cap_data =
    {
        /* Capability ID */
        WIND_NOISE_DETECT_CAP_ID,
        /* Version information - hi and lo */
        WND_VERSION_MAJOR, WND_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        WND_MAX_TERMINALS, WND_MAX_TERMINALS,
        /* Pointer to message handler function table */
        &wnd_handler_table,
        /* Pointer to operator message handler function table */
        wnd_opmsg_handler_table,
        /* Pointer to data processing function */
        wnd_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(WND_OP_DATA)
    };

MAP_INSTANCE_DATA(WIND_NOISE_DETECT_CAP_ID, WND_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get WND instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data WND_OP_DATA.
 */
static inline WND_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (WND_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief  Free memory allocated during processing
 *
 * \param  p_ext_data  Address of the WND extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool wnd_proc_destroy(WND_OP_DATA *p_ext_data)
{

    cbuffer_destroy(p_ext_data->p_tmp_wnd);

    if (p_ext_data->p_wnd200_common != NULL)
    {
        aanc_wnd200_common_destroy(p_ext_data->p_wnd200_common);
    }

    if (p_ext_data->p_table != NULL)
    {
        mem_table_free((void *)p_ext_data,
                       p_ext_data->p_table,
                       WND_MEM_TABLE_SIZE);
        pdelete(p_ext_data->p_table);
    }

    unload_aanc_handle(p_ext_data->f_handle);

    return TRUE;
}

/**
 * \brief  Sent an unsolicited message for a WND event.
 *
 * \param  op_data      Pointer to operator data
 * \param  p_evt_msg    Pointer to the event message data to end
 *
 * \return - TRUE if successful
 */
static bool wnd_send_event_message(OPERATOR_DATA *op_data,
                                   AHM_EVENT_MSG *p_evt_msg)
{
    unsigned msg_size;
    unsigned *p_msg;
    OPMSG_REPLY_ID msg_id;

    msg_id = OPMSG_REPLY_ID_AHM_EVENT_TRIGGER;
    msg_size = OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER_WORD_SIZE;

    p_msg = xzpnewn(msg_size, unsigned);
    if (p_msg == NULL)
    {
        L2_DBG_MSG1("OPID: %x, Failed to create ATR VAD message payload", INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    OPMSG_CREATION_FIELD_SET(p_msg,
                             OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER,
                             ID,
                             p_evt_msg->id);
    OPMSG_CREATION_FIELD_SET(p_msg,
                             OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER,
                             TYPE,
                             p_evt_msg->type);
    OPMSG_CREATION_FIELD_SET32(p_msg,
                               OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER,
                               PAYLOAD,
                               p_evt_msg->payload);
    OPMSG_CREATION_FIELD_SET32(p_msg,
                               OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER,
                               OPID,
                               INT_TO_EXT_OPID(op_data->id));
    common_send_unsolicited_message(op_data, (unsigned)msg_id, msg_size,
                                    p_msg);

    pdelete(p_msg);
    return TRUE;
}

/****************************************************************************
Capability API Handlers
*/

bool wnd_create(OPERATOR_DATA *op_data, void *message_data,
                unsigned *response_id, void **resp_data)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    L5_DBG_MSG2("OPID: %x, WND Create: p_ext_data at %p", ext_op_id, p_ext_data);

    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* TODO: patch functions */

    /* Assume the response to be command FAILED. If we reach the correct
     * termination point in create then change it to STATUS_OK.
     */
    base_op_change_response_status(resp_data, STATUS_CMD_FAILED);

    /* Multi-channel create */
    aud_cur_create(op_data, WND_MAX_TERMINALS, WND_MAX_TERMINALS);
    aud_cur_set_callbacks(op_data,
                          NULL,
                          NULL,
                          wnd_connect_hook,
                          wnd_disconnect_hook,
                          NULL);
    aud_cur_set_flags(op_data,
                      WND_SUPPORTS_IN_PLACE,
                      WND_SUPPORTS_METADATA,
                      WND_DYNAMIC_BUFFERS_FALSE);
    aud_cur_set_min_terminal_masks(op_data,
                                   WND_MIN_VALID_SOURCES,
                                   WND_MIN_VALID_SINKS);

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = WIND_NOISE_DETECT_CAP_ID;

    p_ext_data->sample_rate = 16000;

    aud_cur_set_buffer_size(op_data, WND_DEFAULT_BUFFER_SIZE);
    aud_cur_set_block_size(op_data, WND_DEFAULT_BLOCK_SIZE);
    aud_cur_set_runtime_disconnect(op_data, TRUE);

    /* Initialize parameters */
    p_default_params = (unsigned*) WND_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->wnd_cap_params;
    p_param_def = aud_cur_get_cps(op_data);
    if (!cpsInitParameters(p_param_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(WND_PARAMETERS)))
    {
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = WIND_NOISE_DETECT_SYSMODE_1MIC;
    p_ext_data->host_mode = WIND_NOISE_DETECT_SYSMODE_1MIC;
    p_ext_data->qact_mode = WIND_NOISE_DETECT_SYSMODE_1MIC;

    p_ext_data->detect_event.p_detect = &p_ext_data->detect;
    p_ext_data->detect_event.p_pwr_level = &p_ext_data->power;
    p_ext_data->detect_event.p_intensity = (unsigned*)&p_ext_data->intensity;
    p_ext_data->detect_event.p_mode = &p_ext_data->cur_mode;

    /* Initialize memory for wnd module */
    p_ext_data->p_table = xzpnewn(WND_MEM_TABLE_SIZE, malloc_t_entry);
    if (p_ext_data->p_table == NULL)
    {
        wnd_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, WND failed to allocate memory table", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_table[0] = (malloc_t_entry){
        (uint16)(aanc_wnd200_1mic_dmx_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_1mic)};
    p_ext_data->p_table[1] = (malloc_t_entry){
        (uint16)(aanc_wnd200_1mic_dm1_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_1mic_dm1)};
    p_ext_data->p_table[2] = (malloc_t_entry){
        (uint16)(aanc_wnd200_2mic_dmx_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_2mic)};
    p_ext_data->p_table[3] = (malloc_t_entry){
        (uint16)(aanc_wnd200_2mic_dm1_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_2mic_dm1)};
    p_ext_data->p_table[4] = (malloc_t_entry){
        (uint16)(aanc_wnd200_common_dmx_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_common)};
    p_ext_data->p_table[5] = (malloc_t_entry){
        (uint16)(aanc_wnd200_common_dm1_bytes()/sizeof(unsigned)),
        MALLOC_PREFERENCE_NONE,
        offsetof(WND_OP_DATA, p_wnd200_common_dm1)};

    if (!mem_table_zalloc((void *)p_ext_data,
                          p_ext_data->p_table,
                          WND_MEM_TABLE_SIZE))
    {
        wnd_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, WND failed to allocate memory table entries", ext_op_id);
        return FALSE;
    }

    /* Create shared WND cbuffer without specific bank allocation */
    p_ext_data->p_tmp_wnd = cbuffer_create_with_malloc(
        WND200_DEFAULT_BUFFER_SIZE, BUF_DESC_SW_BUFFER);

    if (p_ext_data->p_tmp_wnd == NULL)
    {
        wnd_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, WND failed to allocate temporary cbuffer", ext_op_id);
        return FALSE;
    }

    /* Create common WND200 data structure */
    if (!aanc_wnd200_common_create(p_ext_data->p_wnd200_common,
                                   p_ext_data->p_wnd200_common_dm1))
    {
        wnd_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, WND failed to create wnd200", ext_op_id);
        return FALSE;
    }

    if (!load_aanc_handle(&p_ext_data->f_handle))
    {
        wnd_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, WND failed to load feature handle", ext_op_id);
        return FALSE;
    }

    /* Operator creation was succesful, change respone to STATUS_OK*/
    base_op_change_response_status(resp_data, STATUS_OK);

    L4_DBG_MSG1("OPID: %x, WND: Created", ext_op_id);
    return TRUE;
}

bool wnd_destroy(OPERATOR_DATA *op_data, void *message_data,
                 unsigned *response_id, void **resp_data)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* TODO: patch functions */

    wnd_proc_destroy(p_ext_data);

    /* Release class data */
    aud_cur_destroy(op_data);

    L4_DBG_MSG1("OPID: %x, WND: Destroyed", INT_TO_EXT_OPID(op_data->id));
    return TRUE;
}

/****************************************************************************
Hook functions
*/
bool wnd_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    tCbuffer * p_buffer;
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    p_buffer = aud_cur_get_sink_terminal(op_data, terminal_num);
    switch (terminal_num)
    {
        case WND_1MIC_TERMINAL:
            p_ext_data->p_ext_mic = p_buffer;
            break;
        case WND_2MIC_TERMINAL:
            p_ext_data->p_div_mic = p_buffer;
            break;
        default:
            L2_DBG_MSG2("OPID: %x, WND: unhandled terminal at connect: %d",
                        INT_TO_EXT_OPID(op_data->id), terminal_num);
            return FALSE;
    }

    return TRUE;
}

bool wnd_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    switch (terminal_num)
    {
        case WND_1MIC_TERMINAL:
            p_ext_data->p_ext_mic = NULL;
            break;
        case WND_2MIC_TERMINAL:
            p_ext_data->p_div_mic = NULL;
            break;
        default:
            L2_DBG_MSG2("OPID: %x, WND: unhandled terminal at disconnect: %d",
                        INT_TO_EXT_OPID(op_data->id), terminal_num);
            return FALSE;
    }

    return TRUE;
}

#ifdef RUNNING_ON_KALSIM
/**
 * \brief  Send an unsolicited message to the kalsim simulator.
 *
 * \param  op_data          Pointer to the operator data
 * \param  p_ext_data       Pointer to the extra op data structure
 *
 * \return  NONE
 *
 * This message is used in simulation to provide an indication of the flag
 * status to confirm correct behavior of wind noise detection.
 *
 * The message structure is shared with AANC. The second most significant nibble
 * in the FLAGS field is used by WND.
 */
static void wnd_send_kalsim_msg_on_flags_update(OPERATOR_DATA *op_data,
                                                WND_OP_DATA *p_ext_data)
{
    unsigned msg_size = OPMSG_UNSOLICITED_AANC_INFO_WORD_SIZE;
    unsigned *trigger_message = NULL;
    OPMSG_REPLY_ID message_id;
    unsigned detect;

    static unsigned previous_detect = 0;

    /* Send Kalsim msg if flags have changed */
    if (previous_detect != p_ext_data->detect)
    {
        trigger_message = xzpnewn(msg_size, unsigned);
        if (trigger_message == NULL)
        {
            return;
        }

        /* Second most significant nibble in AANC kalsim FLAGS is reserved
         * for WND
         */
        detect = (p_ext_data->detect & WND_KALSIM_FLAG_MASK) \
            << WND_KALSIM_FLAGS_SHIFT;
        OPMSG_CREATION_FIELD_SET32(trigger_message,
                                   OPMSG_UNSOLICITED_AANC_INFO,
                                   FLAGS,
                                   detect);
        OPMSG_CREATION_FIELD_SET32(trigger_message,
                                   OPMSG_UNSOLICITED_AANC_INFO,
                                   OPID,
                                   INT_TO_EXT_OPID(op_data->id));
        message_id = OPMSG_REPLY_ID_AANC_TRIGGER;
        common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size,
                                        trigger_message);

        pdelete(trigger_message);
    }

    previous_detect = p_ext_data->detect;
    return;
}
#endif /* RUNNING_ON_KALSIM */

/****************************************************************************
Opmsg handlers
*/
bool wnd_opmsg_set_control(OPERATOR_DATA *op_data,
                           void *message_data,
                           unsigned *resp_length,
                           OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);

    unsigned i;
    unsigned num_controls;

    OPMSG_RESULT_STATES result = OPMSG_RESULT_STATES_NORMAL_STATE;

    if (!cps_control_setup(message_data, resp_length, resp_data, &num_controls))
    {
       return FALSE;
    }

    /* Iterate through control messages looking for mode override messages */
    for (i = 0; i < num_controls; i++)
    {
        unsigned ctrl_value, ctrl_id;
        CPS_CONTROL_SOURCE  ctrl_src;

        ctrl_id = cps_control_get(message_data, i, &ctrl_value, &ctrl_src);

        /* Mode override */
        if (ctrl_id == OPMSG_CONTROL_MODE_ID)
        {
            /* Check for valid mode */
            ctrl_value &= WND_SYSMODE_MASK;
            if (ctrl_value >= WIND_NOISE_DETECT_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            if ((ctrl_value == WIND_NOISE_DETECT_SYSMODE_1MIC) &&
                (p_ext_data->wnd_cap_params.OFFSET_WND_CONFIG &
                 WIND_NOISE_DETECT_CONFIG_WND_CONFIG_2MIC_ONLY) > 0)
            {
                /* We are in 2-mic-only mode. Restrict 1-mic-mode */
                L0_DBG_MSG1("OPID: %x, WND: Cannot set sysmode to 1-mic when \
                             operator is configured in 2mic-only mode", ext_op_id);
                break;
            }
            /* Update current mode */
            p_ext_data->cur_mode = ctrl_value;
            /* Reset detection event */
            p_ext_data->detect_event.state = WND_EVENT_RELEASE;
            /* Reinitialize */
            aud_cur_set_reinit(op_data, TRUE);

            if (p_ext_data->cur_mode == WIND_NOISE_DETECT_SYSMODE_2MIC &&
                (p_ext_data->wnd_cap_params.OFFSET_WND_CONFIG &
                 WIND_NOISE_DETECT_CONFIG_WND_CONFIG_2MIC_ONLY) == 0)
            {
                p_ext_data->detect_event.confirm = TRUE;
            }

            /* Determine control mode source and set override flags for mode */
            if (ctrl_src == CPS_SOURCE_HOST)
            {
                p_ext_data->host_mode = ctrl_value;
            }
            else
            {
                p_ext_data->qact_mode = ctrl_value;
                /* Set or clear the QACT override flag.
                * &= is used to preserve the state of the
                * override word.
                */
                if (ctrl_src == CPS_SOURCE_OBPM_ENABLE)
                {
                    p_ext_data->ovr_control |= WIND_NOISE_DETECT_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= WND_OVERRIDE_MODE_MASK;
                }
            }

        }
        /* Enable 3 intensity categorization support */
        else if (ctrl_id == WIND_NOISE_DETECT_CONSTANT_INTENSITY_UPDATE_CTRL)
        {
            ctrl_value &= 0x1;
            p_ext_data->intensity_ctrl = ctrl_value;
        }
        else
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
    }

    /* Set current operating mode based on override */
    if ((p_ext_data->ovr_control & WIND_NOISE_DETECT_CONTROL_MODE_OVERRIDE) != 0)
    {
        p_ext_data->cur_mode = p_ext_data->qact_mode;
    }
    else
    {
        p_ext_data->cur_mode = p_ext_data->host_mode;
    }

    cps_response_set_result(resp_data, result);

    return TRUE;
}

bool wnd_opmsg_get_status(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *resp_length,
                          OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);
    int i;

    /* TODO: patch functions */

    /* Build the response */
    unsigned *resp = NULL;
    if (!common_obpm_status_helper(message_data, resp_length, resp_data,
                                   sizeof(WND_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp)
    {
        WND_STATISTICS stats;
        WND_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;

        pstats->OFFSET_CUR_MODE             = p_ext_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL          = p_ext_data->ovr_control;
        pstats->OFFSET_POWER                = p_ext_data->power;
        pstats->OFFSET_DETECTION            = p_ext_data->detect;
        pstats->OFFSET_INTENSITY            = p_ext_data->intensity;
        pstats->OFFSET_EVENT_STATE          = p_ext_data->detect_event.state;

        for (i = 0; i < WND_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((WND_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[WND_N_STAT-1], resp);
        }
    }

    return TRUE;
}


/****************************************************************************
Custom opmsg handlers
*/
bool wnd_opmsg_get_power_intensity(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_resp;
    uint16 msg_id;

    *resp_length = OPMSG_WND_GET_POWER_INTENSITY_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_WND_GET_POWER_INTENSITY_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    msg_id = (uint16)OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_WND_GET_POWER_INTENSITY_RESP,
                             MESSAGE_ID,
                             msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_WND_GET_POWER_INTENSITY_RESP,
                             POWER,
                             (uint16)(p_ext_data->power >> 16));
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_WND_GET_POWER_INTENSITY_RESP,
                             INTENSITY,
                             (uint16)p_ext_data->intensity);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}
/****************************************************************************
Data processing function
*/
void wnd_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    WND_OP_DATA *p_ext_data = get_instance_data(op_data);

    int sample_count, samples_to_process;

    WND200_1MIC_DMX *p_1mic;
    WND200_2MIC_DMX *p_2mic;
    WND200_COMMON_DMX *p_common;
    WND_PARAMETERS *p_params;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    /*********************
     * Early exit testing
     *********************/

    samples_to_process = aud_cur_calc_samples(op_data, touched);

     /* Return early if not enough data to process */
    if (samples_to_process < WND_DEFAULT_FRAME_SIZE)
    {
        return;
    }

    /* Don't do any processing in standby */
    if (p_ext_data->cur_mode == WIND_NOISE_DETECT_SYSMODE_STANDBY)
    {
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  samples_to_process,
                                  WND_TERMINAL_SKIP_MASK);
        /*  Metadata transfer */
        aud_cur_mic_metadata_transfer(op_data, samples_to_process);

        /* Exit early */
        return;
    }

    if (aud_cur_get_reinit(op_data))
    {
        p_1mic = p_ext_data->p_wnd200_1mic;
        p_2mic = p_ext_data->p_wnd200_2mic;
        p_common = p_ext_data->p_wnd200_common;

#ifdef USE_AANC_LICENSING
        /* Set re-init flag to FALSE if both 1-mic and 2-mic license checks
           have passed. */
        if (p_1mic->licensed == TRUE && p_2mic->licensed == TRUE)
#endif
        {
            aud_cur_set_reinit(op_data, FALSE);
        }

        p_params = &p_ext_data->wnd_cap_params;

        p_common->sample_rate = p_ext_data->sample_rate;
        p_common->p_tmp = p_ext_data->p_tmp_wnd;
        p_common->p_main_ip = p_ext_data->p_ext_mic;
        p_common->pwr_threshold = p_params->OFFSET_POWER_THRESHOLD;
        p_common->high_wind_thr = p_params->OFFSET_HIGH_WIND_THRESHOLD;
        p_common->med_wind_thr = p_params->OFFSET_MED_WIND_THRESHOLD;
        p_common->high_rel_wind_thr = p_params->OFFSET_HIGH_REL_WIND_THRESHOLD;
        p_common->med_rel_wind_thr = p_params->OFFSET_MED_REL_WIND_THRESHOLD;
        p_common->envelope_time = p_params->OFFSET_POWER_ENVELOPE_TIME;
        p_common->attack_time = p_params->OFFSET_POWER_ATTACK_TIME;
        p_common->frame_size = WND_DEFAULT_FRAME_SIZE;
        p_common->intensity_ctrl = p_ext_data->intensity_ctrl;
        aanc_wnd200_common_initialize(p_ext_data->f_handle, p_common);

        p_1mic->wnd_threshold = p_params->OFFSET_1MIC_THRESHOLD;
        p_1mic->filter_decay_time = p_params->OFFSET_1MIC_FILTER_DECAY;
        p_1mic->filter_attack_time = p_params->OFFSET_1MIC_FILTER_ATTACK;
        p_1mic->wnd_block_size = WND_1MIC_BLOCK_SIZE;
        aanc_wnd200_1mic_initialize(p_ext_data->f_handle, p_1mic, p_common);

        p_2mic->p_aux_ip = p_ext_data->p_div_mic;
        p_2mic->wnd_threshold = p_params->OFFSET_2MIC_THRESHOLD;
        p_2mic->filter_decay_time = p_params->OFFSET_2MIC_FILTER_DECAY;
        p_2mic->filter_attack_time = p_params->OFFSET_2MIC_FILTER_ATTACK;
        p_2mic->wnd_block_size = WND_2MIC_BLOCK_SIZE;
        aanc_wnd200_2mic_initialize(p_ext_data->f_handle, p_2mic, p_common);

        switch (p_ext_data->cur_mode)
        {
            case WIND_NOISE_DETECT_SYSMODE_1MIC:
                wnd_setup_event(
                    &p_ext_data->detect_event,
                    p_params->OFFSET_1MIC_MESSAGE_ATTACK,
                    p_params->OFFSET_1MIC_MESSAGE_RELEASE);
                break;
            case WIND_NOISE_DETECT_SYSMODE_2MIC:
                wnd_setup_event(
                    &p_ext_data->detect_event,
                    p_params->OFFSET_2MIC_MESSAGE_ATTACK,
                    p_params->OFFSET_2MIC_MESSAGE_RELEASE);
                break;
            default:
                break;
        }
    }
    sample_count = 0;
    while (samples_to_process >= WND_DEFAULT_FRAME_SIZE)
    {
        switch (p_ext_data->cur_mode)
        {
            case WIND_NOISE_DETECT_SYSMODE_1MIC:
                if (p_ext_data->p_ext_mic == NULL)
                {
                    L0_DBG_MSG1("OPID: %x, WND: 1-mic mode missing terminal connection", ext_op_id);
                    break;
                }
                p_1mic = p_ext_data->p_wnd200_1mic;
                p_common = p_ext_data->p_wnd200_common;
                aanc_wnd200_1mic_process_data(p_ext_data->f_handle, p_1mic, p_common);
                p_ext_data->detect = (p_common->wind_flag *
                                      WIND_NOISE_DETECT_DETECTION_1MIC_WIND);
                p_ext_data->intensity = (WND_INTENSITY)p_common->wind_state;
                p_ext_data->power = p_common->main_mic_min_pwr;
                break;
            case WIND_NOISE_DETECT_SYSMODE_2MIC:
                if (p_ext_data->p_div_mic == NULL ||
                    p_ext_data->p_ext_mic == NULL)
                {
                    L0_DBG_MSG1("OPID: %x, WND: 2-mic mode missing terminal connection", ext_op_id);
                    break;
                }
                p_2mic = p_ext_data->p_wnd200_2mic;
                p_common = p_ext_data->p_wnd200_common;
                aanc_wnd200_2mic_process_data(p_ext_data->f_handle, p_2mic, p_common);
                p_ext_data->detect = (p_common->wind_flag *
                                      WIND_NOISE_DETECT_DETECTION_2MIC_WIND);
                p_ext_data->intensity = (WND_INTENSITY)p_common->wind_state;
                p_ext_data->power = p_common->main_mic_min_pwr;
                break;
            default:
                L2_DBG_MSG2("OPID: %x, WND: Unsupported sysmode %d", p_ext_data->cur_mode, ext_op_id);
        }

        if (wnd_process_event(&p_ext_data->detect_event, ext_op_id))
        {
            if(p_ext_data->intensity_ctrl)
            {
                if (p_ext_data->detect_event.msg.type == WND_EVENT_TYPE_ATTACK)
                {
                    p_ext_data->wind_confirm = TRUE;
                    p_ext_data->prev_intensity = p_ext_data->intensity;
                }
                else if (p_ext_data->detect_event.msg.type == WND_EVENT_TYPE_RELEASE)
                {
                    p_ext_data->wind_confirm = FALSE;
                }
            }
            /* Send message */
            wnd_send_event_message(op_data, &p_ext_data->detect_event.msg);
        }
        if(p_ext_data->intensity_ctrl)
        {
            if(p_ext_data->wind_confirm)
            {
                if((p_ext_data->prev_intensity != p_ext_data->intensity) && p_ext_data->detect !=0)
                {
                    wnd_setup_event_payload(&p_ext_data->detect_event);
                    p_ext_data->detect_event.msg.type = WND_EVENT_TYPE_ATTACK;
                    wnd_send_event_message(op_data, &p_ext_data->detect_event.msg);
                    p_ext_data->prev_intensity = p_ext_data->intensity;
                }
            }
        }
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  WND_DEFAULT_FRAME_SIZE,
                                  WND_TERMINAL_SKIP_MASK);

        samples_to_process = aud_cur_calc_samples(op_data, touched);
        sample_count += WND_DEFAULT_FRAME_SIZE;
    }
    /*  Metadata transfer */
    aud_cur_mic_metadata_transfer(op_data, sample_count);

#ifdef RUNNING_ON_KALSIM
    wnd_send_kalsim_msg_on_flags_update(op_data, p_ext_data);
#endif

    return;
}
