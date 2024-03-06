/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  atr_vad.c
 * \ingroup atr_vad
 *
 * Auto Transparency VAD (ATR_VAD) capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "atr_vad.h"

/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define ATR_VAD_CAP_ID   CAP_ID_DOWNLOAD_ATR_VAD
#else
#define ATR_VAD_CAP_ID   CAP_ID_ATR_VAD
#endif

/* Message handlers */
const handler_lookup_struct atr_vad_handler_table =
{
    atr_vad_create,                /* OPCMD_CREATE */
    atr_vad_destroy,               /* OPCMD_DESTROY */
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
const opmsg_handler_lookup_table_entry atr_vad_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             atr_vad_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              atr_vad_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},

    {OPMSG_ATR_VAD_ID_ATR_SET_RELEASE_DURATION, atr_vad_opmsg_set_release_duration},
    {OPMSG_ATR_VAD_ID_ATR_GET_RELEASE_DURATION, atr_vad_opmsg_get_release_duration},
    {OPMSG_ATR_VAD_ID_ATR_SET_SENSITIVITY,      atr_vad_opmsg_set_sensitivity},
    {OPMSG_ATR_VAD_ID_ATR_GET_SENSITIVITY,      atr_vad_opmsg_get_sensitivity},
    {0, NULL}};

const CAPABILITY_DATA atr_vad_cap_data =
    {
        /* Capability ID */
        ATR_VAD_CAP_ID,
        /* Version information - hi and lo */
        ATR_VAD_ATR_VAD_VERSION_MAJOR, ATR_VAD_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        ATR_VAD_MAX_TERMINALS, ATR_VAD_MAX_TERMINALS,
        /* Pointer to message handler function table */
        &atr_vad_handler_table,
        /* Pointer to operator message handler function table */
        atr_vad_opmsg_handler_table,
        /* Pointer to data processing function */
        atr_vad_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(ATR_VAD_OP_DATA)
    };

MAP_INSTANCE_DATA(ATR_VAD_CAP_ID, ATR_VAD_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get ATR_VAD instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data ATR_VAD_OP_DATA.
 */
static inline ATR_VAD_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (ATR_VAD_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief  Free memory allocated during processing
 *
 * \param  p_ext_data  Address of the ATR_VAD extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool atr_vad_proc_destroy(ATR_VAD_OP_DATA *p_ext_data)
{
    pdelete(p_ext_data->p_atr_vad100);
    pdelete(p_ext_data->p_atr_vad100_dm1);

    unload_vad_handle(p_ext_data->f_handle);

    return TRUE;
}

/**
 * \brief  Sent an unsolicited message for an ATR_VAD event.
 *
 * \param  op_data      Pointer to operator data
 * \param  p_evt_msg    Pointer to the event message data to end
 *
 * \return - TRUE if successful
 */
static bool atr_vad_send_event_message(OPERATOR_DATA *op_data,
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
        L2_DBG_MSG("Failed to create ATR VAD message payload");
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

bool atr_vad_create(OPERATOR_DATA *op_data, void *message_data,
                    unsigned *response_id, void **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */
    uint16 atr_vad_bytes;           /* Number of bytes to allocate */

    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Assume the response to be command FAILED. If we reach the correct
     * termination point in create then change it to STATUS_OK.
     */
    base_op_change_response_status(resp_data, STATUS_CMD_FAILED);

    /* Multi-channel create */
    aud_cur_create(op_data, ATR_VAD_MAX_TERMINALS, ATR_VAD_MAX_TERMINALS);
    aud_cur_set_callbacks(op_data,
                          NULL,
                          NULL,
                          atr_vad_connect_hook,
                          atr_vad_disconnect_hook,
                          NULL);
    aud_cur_set_flags(op_data,
                      ATR_VAD_SUPPORTS_IN_PLACE,
                      ATR_VAD_SUPPORTS_METADATA,
                      ATR_VAD_DYNAMIC_BUFFERS);
    aud_cur_set_min_terminal_masks(op_data,
                                   ATR_VAD_MIN_VALID_SOURCES,
                                   ATR_VAD_MIN_VALID_SINKS);

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = ATR_VAD_CAP_ID;
    p_ext_data->sample_rate = ATR_VAD_DEFAULT_SAMPLE_RATE;

    aud_cur_set_buffer_size(op_data, ATR_VAD_DEFAULT_BUFFER_SIZE);
    aud_cur_set_block_size(op_data, ATR_VAD_DEFAULT_BLOCK_SIZE);
    aud_cur_set_runtime_disconnect(op_data, TRUE);

    /* Initialize parameters */
    p_default_params = (unsigned*) ATR_VAD_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->atr_vad_params;
    p_param_def = aud_cur_get_cps(op_data);
    if (!cpsInitParameters(p_param_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(ATR_VAD_PARAMETERS)))
    {
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = ATR_VAD_SYSMODE_1MIC;
    p_ext_data->host_mode = ATR_VAD_SYSMODE_1MIC;
    p_ext_data->qact_mode = ATR_VAD_SYSMODE_1MIC;

    p_ext_data->detect_event.p_detect = &p_ext_data->detect;
    p_ext_data->detect_event.p_mode = &p_ext_data->cur_mode;

    atr_vad_bytes = aanc_atr_vad100_dmx_bytes(ATR_VAD100_DEFAULT_FRAME_SIZE);
    p_ext_data->p_atr_vad100 = \
        (ATR_VAD100_DMX*) xzppmalloc(atr_vad_bytes, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_atr_vad100 == NULL)
    {
        atr_vad_proc_destroy(p_ext_data);
        L2_DBG_MSG("ATR_VAD failed to allocate atr_vad100 dmx");
        return FALSE;
    }

    p_ext_data->p_atr_vad100_dm1 = \
        (uint8*) xzppmalloc(aanc_atr_vad100_dm1_bytes(),
                            MALLOC_PREFERENCE_DM1);
    if (p_ext_data->p_atr_vad100_dm1 == NULL)
    {
        atr_vad_proc_destroy(p_ext_data);
        L2_DBG_MSG("ATR_VAD failed to allocate atr_vad100 dm1");
        return FALSE;
    }

    /* Create ATR_VAD100 data structure */
    if (!aanc_atr_vad100_create(p_ext_data->p_atr_vad100,
                                p_ext_data->p_atr_vad100_dm1,
                                p_ext_data->sample_rate,
                                ATR_VAD100_DEFAULT_FRAME_SIZE))
    {
        atr_vad_proc_destroy(p_ext_data);
        L2_DBG_MSG("ATR_VAD failed to create atr_vad100");
        return FALSE;
    }

    if (!load_vad_handle(&p_ext_data->f_handle))
    {
        atr_vad_proc_destroy(p_ext_data);
        L2_DBG_MSG("ATR_VAD failed to load feature handle");
        return FALSE;
    }

    p_ext_data->release_select = ATR_VAD_RELEASE_NORMAL;

    /* Operator creation was succesful, change respone to STATUS_OK*/
    base_op_change_response_status(resp_data, STATUS_OK);

    L4_DBG_MSG("ATR_VAD: Created");
    return TRUE;
}

bool atr_vad_destroy(OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    atr_vad_proc_destroy(p_ext_data);

    /* Release class data */
    aud_cur_destroy(op_data);

    L4_DBG_MSG("ATR_VAD: Destroyed");
    return TRUE;
}

/****************************************************************************
Hook functions
*/
bool atr_vad_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    tCbuffer * p_buffer;
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    p_buffer = aud_cur_get_sink_terminal(op_data, terminal_num);
    switch (terminal_num)
    {
        case ATR_VAD_BCM_TERMINAL:
            p_ext_data->p_bcm_mic = p_buffer;
            break;
        case ATR_VAD_FF_TERMINAL:
            p_ext_data->p_ff_mic = p_buffer;
            break;
        default:
            L2_DBG_MSG1("ATR_VAD: unhandled terminal at connect: %d",
                        terminal_num);
            return FALSE;
    }

    return TRUE;
}

bool atr_vad_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    switch (terminal_num)
    {
        case ATR_VAD_BCM_TERMINAL:
            p_ext_data->p_bcm_mic = NULL;
            break;
        case ATR_VAD_FF_TERMINAL:
            p_ext_data->p_ff_mic = NULL;
            break;
        default:
            L2_DBG_MSG1("ATR_VAD: unhandled terminal at disconnect: %d",
                        terminal_num);
            return FALSE;
    }

    return TRUE;
}

/****************************************************************************
Opmsg handlers
*/
bool atr_vad_opmsg_set_control(OPERATOR_DATA *op_data,
                               void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

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
            ctrl_value &= ATR_VAD_SYSMODE_MASK;
            if (ctrl_value >= ATR_VAD_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            /* Update current mode */
            p_ext_data->cur_mode = ctrl_value;
            /* Reset detection event */
            p_ext_data->detect_event.state = ATR_VAD_EVENT_RELEASE;
            /* Reinitialize */
            aud_cur_set_reinit(op_data, TRUE);

            if (p_ext_data->cur_mode == ATR_VAD_SYSMODE_2MIC &&
                (p_ext_data->atr_vad_params.OFFSET_ATR_VAD_CONFIG &
                 ATR_VAD_CONFIG_ATR_VAD_CONFIG_2MIC_ONLY) == 0)
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
                    p_ext_data->ovr_control |= ATR_VAD_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= ATR_VAD_OVERRIDE_MODE_MASK;
                }
            }

        }
        else
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
    }

    /* Set current operating mode based on override */
    if ((p_ext_data->ovr_control & ATR_VAD_CONTROL_MODE_OVERRIDE) != 0)
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

bool atr_vad_opmsg_get_status(OPERATOR_DATA *op_data,
                              void *message_data,
                              unsigned *resp_length,
                              OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);
    int i;

    /* Build the response */
    unsigned *resp = NULL;
    if (!common_obpm_status_helper(message_data, resp_length, resp_data,
                                   sizeof(ATR_VAD_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp)
    {
        ATR_VAD_STATISTICS stats;
        ATR_VAD_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;

        pstats->OFFSET_CUR_MODE             = p_ext_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL          = p_ext_data->ovr_control;
        pstats->OFFSET_POWER                = p_ext_data->p_atr_vad100->pwr;
        pstats->OFFSET_DETECTION            = p_ext_data->detect;
        pstats->OFFSET_EVENT_STATE          = p_ext_data->detect_event.state;

        for (i = 0; i < ATR_VAD_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((ATR_VAD_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[ATR_VAD_N_STAT-1], resp);
        }
    }

    return TRUE;
}


/****************************************************************************
Custom opmsg handlers
*/
bool atr_vad_opmsg_set_release_duration(OPERATOR_DATA *op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    p_ext_data->release_select = (ATR_VAD_RELEASE)OPMSG_FIELD_GET(
        message_data, OPMSG_ATR_VAD_SET_RELEASE_DURATION, DURATION);

    atr_vad_setup_event(&p_ext_data->detect_event,
                        p_ext_data->release_select);

    return TRUE;
}

bool atr_vad_opmsg_get_release_duration(OPERATOR_DATA *op_data,
                                        void *message_data,
                                        unsigned *resp_length,
                                        OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *p_resp;
    unsigned msg_id;

    *resp_length = OPMSG_ATR_VAD_GET_RELEASE_DURATION_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_ATR_VAD_GET_RELEASE_DURATION_RESP_WORD_SIZE,
                     unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_ATR_VAD_GET_RELEASE_DURATION_RESP,
                               MESSAGE_ID,
                               msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_ATR_VAD_GET_RELEASE_DURATION_RESP,
                             DURATION,
                             (unsigned)p_ext_data->release_select);

    return TRUE;
}

bool atr_vad_opmsg_set_sensitivity(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    p_ext_data->sensitivity_select = (uint8)OPMSG_FIELD_GET(
        message_data, OPMSG_ATR_VAD_SET_SENSITIVITY, SENSITIVITY);

    return TRUE;
}

bool atr_vad_opmsg_get_sensitivity(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *p_resp;
    unsigned msg_id;

    *resp_length = OPMSG_ATR_VAD_GET_SENSITIVITY_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_ATR_VAD_GET_SENSITIVITY_RESP_WORD_SIZE,
                     unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_ATR_VAD_GET_SENSITIVITY_RESP,
                               MESSAGE_ID,
                               msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_ATR_VAD_GET_SENSITIVITY_RESP,
                             SENSITIVITY,
                             p_ext_data->sensitivity_select);

    return TRUE;
}

/****************************************************************************
Data processing function
*/
void atr_vad_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    ATR_VAD_OP_DATA *p_ext_data = get_instance_data(op_data);

    int sample_count, samples_to_process;

    ATR_VAD100_DMX *p_atr;
    ATR_VAD_PARAMETERS *p_params;

    /*********************
     * Early exit testing
     *********************/

    samples_to_process = aud_cur_calc_samples(op_data, touched);

     /* Return early if not enough data to process */
    if (samples_to_process < ATR_VAD_DEFAULT_FRAME_SIZE)
    {
        return;
    }

    /* Don't do any processing in standby */
    if (p_ext_data->cur_mode == ATR_VAD_SYSMODE_STANDBY)
    {
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  samples_to_process,
                                  ATR_VAD_TERMINAL_SKIP_MASK);
        /*  Metadata transfer */
        aud_cur_mic_metadata_transfer(op_data, samples_to_process);

        /* Exit early */
        return;
    }

    p_atr = p_ext_data->p_atr_vad100;

    if (aud_cur_get_reinit(op_data))
    {
#ifdef USE_AANC_LICENSING
        /* Set re-init flag to FALSE if both 1-mic and 2-mic license checks
           have passed. */
        if (p_atr->licensed == TRUE)
#endif
        {
            aud_cur_set_reinit(op_data, FALSE);
        }

        p_params = &p_ext_data->atr_vad_params;

        p_atr->vad_attack_time = \
            p_params->OFFSET_VAD_ATTACK_TIME;
        p_atr->vad_decay_time = \
            p_params->OFFSET_VAD_RELEASE_TIME;
        p_atr->vad_envelope_time = \
            p_params->OFFSET_VAD_ENVELOPE_TIME;
        p_atr->vad_init_frame_time = \
            p_params->OFFSET_VAD_INIT_TIME;
        p_atr->vad_ratio = \
            p_params->OFFSET_VAD_RATIO;
        p_atr->vad_min_max_envelope = \
            p_params->OFFSET_VAD_PWR_THRESHOLD;
        p_atr->vad_min_signal = \
            p_params->OFFSET_VAD_ENV_THRESHOLD;
        p_atr->vad_delta_th = \
            p_params->OFFSET_VAD_DELTA_THRESHOLD;
        p_atr->vad_count_th = \
            p_params->OFFSET_VAD_COUNT_TH_TIME;

        p_atr->smooth_attack_time = \
            p_params->OFFSET_SMOOTH_ATTACK_TIME;
        p_atr->smooth_release_time = \
            p_params->OFFSET_SMOOTH_RELEASE_TIME;
        p_atr->hold_attack_time = \
            p_params->OFFSET_HOLD_ATTACK_TIME;
        p_atr->hold_release_time = \
            p_params->OFFSET_HOLD_RELEASE_TIME;
        p_atr->threshold = \
            p_params->OFFSET_DET_THRESHOLD;

        aanc_atr_vad100_initialize(p_ext_data->f_handle, p_atr);

        /* Update event configuration */
        p_ext_data->detect_event.config.attack_time = \
            p_params->OFFSET_MSG_ATTACK_TIME;
        p_ext_data->detect_event.config.short_release_time = \
            p_params->OFFSET_MSG_SHORT_RELEASE_TIME;
        p_ext_data->detect_event.config.normal_release_time = \
            p_params->OFFSET_MSG_NORMAL_RELEASE_TIME;
        p_ext_data->detect_event.config.long_release_time = \
            p_params->OFFSET_MSG_LONG_RELEASE_TIME;

        atr_vad_setup_event(&p_ext_data->detect_event,
                            p_ext_data->release_select);
    }

    sample_count = 0;
    while (samples_to_process >= ATR_VAD_DEFAULT_FRAME_SIZE)
    {
        switch (p_ext_data->cur_mode)
        {
            /* 1-mic mode: operate on BCM microphone */
            case ATR_VAD_SYSMODE_1MIC:
                if (p_ext_data->p_bcm_mic == NULL)
                {
                    L0_DBG_MSG("ATR_VAD: 1-mic mode no bcm");
                    break;
                }
                aanc_atr_vad100_process_data(p_ext_data->f_handle,
                                             p_atr,
                                             p_ext_data->p_bcm_mic,
                                             NULL);
                p_ext_data->detect = p_atr->detection;
                break;
            case ATR_VAD_SYSMODE_1MIC_MS:
                if (p_ext_data->p_ff_mic == NULL)
                {
                    L0_DBG_MSG("ATR_VAD: 1-mic mode no ff");
                    break;
                }
                aanc_atr_vad100_process_data(p_ext_data->f_handle,
                                             p_atr,
                                             NULL,
                                             p_ext_data->p_ff_mic);
                p_ext_data->detect = p_atr->detection;
                break;
            case ATR_VAD_SYSMODE_2MIC:
                if (p_ext_data->p_ff_mic == NULL ||
                    p_ext_data->p_bcm_mic == NULL)
                {
                    L0_DBG_MSG("ATR_VAD: 2-mic mode no bcm/ff");
                    break;
                }
                aanc_atr_vad100_process_data(p_ext_data->f_handle,
                                             p_atr,
                                             p_ext_data->p_bcm_mic,
                                             p_ext_data->p_ff_mic);
                p_ext_data->detect = p_atr->detection;
                break;
            default:
                L2_DBG_MSG1("ATR_VAD: Unsupported sysmode %d",
                            p_ext_data->cur_mode);
        }

        if (atr_vad_process_event(&p_ext_data->detect_event))
        {
            /* Send message */
            atr_vad_send_event_message(op_data, &p_ext_data->detect_event.msg);
        }

        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  ATR_VAD_DEFAULT_FRAME_SIZE,
                                  ATR_VAD_TERMINAL_SKIP_MASK);

        samples_to_process = aud_cur_calc_samples(op_data, touched);
        sample_count += ATR_VAD_DEFAULT_FRAME_SIZE;
    }

    /*  Metadata transfer */
    aud_cur_mic_metadata_transfer(op_data, sample_count);

    return;
}
