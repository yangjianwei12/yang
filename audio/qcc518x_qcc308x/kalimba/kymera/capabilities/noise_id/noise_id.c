/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  noise_id.c
 * \ingroup noise_id
 *
 * Noise ID (NOISE_ID) capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "noise_id.h"


/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define NOISE_ID_CAP_ID   CAP_ID_DOWNLOAD_NOISE_ID
#else
#define NOISE_ID_CAP_ID   CAP_ID_NOISE_ID
#endif

/* Message handlers */
const handler_lookup_struct noise_id_handler_table =
{
    noise_id_create,               /* OPCMD_CREATE */
    noise_id_destroy,              /* OPCMD_DESTROY */
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
const opmsg_handler_lookup_table_entry noise_id_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,     base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,                 noise_id_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                  aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                  aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                  noise_id_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                    aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,           aud_cur_opmsg_get_ps_id},
    {OPMSG_NOISEID_ID_SET_CURRENT_NOISE_ID,       noise_id_opmsg_set_current_noise_id},
    {0, NULL}};

const CAPABILITY_DATA noise_id_cap_data =
    {
        /* Capability ID */
        NOISE_ID_CAP_ID,
        /* Version information - hi and lo */
        NOISE_ID_NOISE_ID_VERSION_MAJOR, NOISE_ID_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        NOISE_ID_MAX_TERMINALS, NOISE_ID_MAX_TERMINALS,
        /* Pointer to message handler function table */
        &noise_id_handler_table,
        /* Pointer to operator message handler function table */
        noise_id_opmsg_handler_table,
        /* Pointer to data processing function */
        noise_id_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(NOISE_ID_OP_DATA)
    };

MAP_INSTANCE_DATA(NOISE_ID_CAP_ID, NOISE_ID_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get NOISE_ID instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data NOISE_ID_OP_DATA.
 */
static inline NOISE_ID_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (NOISE_ID_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief  Free memory allocated during processing
 *
 * \param  p_ext_data  Address of the NOISE_ID extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool noise_id_proc_destroy(NOISE_ID_OP_DATA *p_ext_data)
{
    /* Unregister FFT twiddle */
    if (p_ext_data->twiddle_registered)
    {
        math_fft_twiddle_release(AANC_FILTER_BANK_WINDOW_SIZE);
        p_ext_data->twiddle_registered = FALSE;
    }
    /* De-register scratch & free AFB */
    if (p_ext_data->scratch_registered)
    {
        scratch_deregister();
        p_ext_data->scratch_registered = FALSE;
    }
    aanc_afb_destroy(p_ext_data->p_afb);
    pfree(p_ext_data->p_afb);
    pdelete(p_ext_data->p_noise_id100);
    pdelete(p_ext_data->p_noise_id100_dm1);

    /* Destroy and free ED memory */
    aanc_ed100_destroy(p_ext_data->p_ed);
    pdelete(p_ext_data->p_ed);
    pdelete(p_ext_data->p_ed_dm1);
    cbuffer_destroy(p_ext_data->p_tmp_ed);

    unload_aanc_handle(p_ext_data->f_handle);
    return TRUE;
}

/**
 * \brief   Create Noise ID objects.
 *
 * \param   p_ext_data  Address of the NOISE_ID extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool noise_id_proc_create(NOISE_ID_OP_DATA *p_ext_data)
{
    uint16 noise_id_bytes;              /* Number of bytes to allocate */
    uint16 ed_dmx_bytes, ed_dm1_bytes;  /* Memory required by EDs */

    /* Allocate twiddle factor for AFB */
    if (!math_fft_twiddle_alloc(AANC_FILTER_BANK_WINDOW_SIZE))
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate twiddle factors");
        return FALSE;
    }
    p_ext_data->twiddle_registered = TRUE;

    /* Register scratch memory for AFB & allocate object */
    if (!scratch_register())
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to register scratch memory");
        return FALSE;
    }

    p_ext_data->scratch_registered = TRUE;

    if (!scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM1) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2))
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to reserve scratch memory");
        return FALSE;
    }

    /* Allocate AFB */
    p_ext_data->p_afb = xzpmalloc(aanc_afb_bytes());
    if (p_ext_data->p_afb == NULL)
    {
        L2_DBG_MSG("NOISE_ID failed to allocate AFB");
        noise_id_proc_destroy(p_ext_data);
        return FALSE;
    }
    aanc_afb_create(p_ext_data->p_afb);

    /* Allocate Memory */
    noise_id_bytes = aanc_noiseID100_dmx_bytes();
    p_ext_data->p_noise_id100 = \
        (NOISEID100_DMX*) xzppmalloc(noise_id_bytes, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_noise_id100 == NULL)
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate noise_id100 dmx");
        return FALSE;
    }
    p_ext_data->p_noise_id100_dm1 = \
        (uint8*) xzppmalloc(aanc_noiseID100_dm1_bytes(),
                            MALLOC_PREFERENCE_DM1);
    if (p_ext_data->p_noise_id100_dm1 == NULL)
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate noise_id100 dm1");
        return FALSE;
    }

    /* Create NoiseID100 data structure */
    if (!aanc_noiseID100_create(p_ext_data->p_noise_id100,
                                p_ext_data->p_noise_id100_dm1))
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to create noise_id100");
        return FALSE;
    }

    /* Create energy detector */
    ed_dmx_bytes = aanc_ed100_dmx_bytes();
    ed_dm1_bytes = aanc_ed100_dm1_bytes();

    p_ext_data->p_ed = \
        (ED100_DMX*) xzppmalloc(ed_dmx_bytes, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_ed == NULL)
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate ED100_DMX dmx");
        return FALSE;
    }
    p_ext_data->p_ed_dm1 = \
        (uint8*) xzppmalloc(ed_dm1_bytes,
                            MALLOC_PREFERENCE_DM1);
    if (p_ext_data->p_ed_dm1 == NULL)
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate ED100 dm1");
        return FALSE;
    }

    /* Create shared ED cbuffer without specific bank allocation */
    p_ext_data->p_tmp_ed = cbuffer_create_with_malloc(ED100_DEFAULT_BUFFER_SIZE,
                                                      BUF_DESC_SW_BUFFER);
    if (p_ext_data->p_tmp_ed == NULL)
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to allocate ED cbuffer");
        return FALSE;
    }
    aanc_ed100_create(p_ext_data->p_ed,
                      p_ext_data->p_ed_dm1,
                      p_ext_data->sample_rate);

    /* Load feature handle for licensing */
    if (!load_aanc_handle(&p_ext_data->f_handle))
    {
        noise_id_proc_destroy(p_ext_data);
        L2_DBG_MSG("NOISE_ID failed to load feature handle");
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief  Updates flags in Noise_ID extra op data.
 *
 * \param  p_ext_data  Pointer to the operator extra data.
 *
 * \return void
 */
static void noise_id_update_flags(NOISE_ID_OP_DATA *p_ext_data)
{
    /* ED detected flag */
    if (p_ext_data->p_ed->detection)
    {
        p_ext_data->flags |= NOISE_ID_FLAGS_ED;
    }
    else
    {
        p_ext_data->flags &= ~NOISE_ID_FLAGS_ED;
    }

    /* Other new flags go here */

    return;
}
/****************************************************************************
Capability API Handlers
*/

bool noise_id_create(OPERATOR_DATA *op_data, void *message_data,
                    unsigned *response_id, void **resp_data)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */

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
    aud_cur_create(op_data, NOISE_ID_MAX_TERMINALS, NOISE_ID_MAX_TERMINALS);
    aud_cur_set_callbacks(op_data,
                          NULL,
                          NULL,
                          noise_id_connect_hook,
                          noise_id_disconnect_hook,
                          NULL);
    aud_cur_set_flags(op_data,
                      NOISE_ID_SUPPORTS_IN_PLACE,
                      NOISE_ID_SUPPORTS_METADATA,
                      NOISE_ID_DYNAMIC_BUFFERS);
    aud_cur_set_min_terminal_masks(op_data,
                                   NOISE_ID_MIN_VALID_SOURCES,
                                   NOISE_ID_MIN_VALID_SINKS);

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = NOISE_ID_CAP_ID;
    p_ext_data->sample_rate = NOISE_ID_DEFAULT_SAMPLE_RATE;

    aud_cur_set_buffer_size(op_data, NOISE_ID_DEFAULT_BUFFER_SIZE);
    aud_cur_set_block_size(op_data, NOISE_ID_DEFAULT_BLOCK_SIZE);
    aud_cur_set_runtime_disconnect(op_data, TRUE);

    /* Initialize parameters */
    p_default_params = (unsigned*) NOISE_ID_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->noise_id_params;
    p_param_def = aud_cur_get_cps(op_data);
    if (!cpsInitParameters(p_param_def,
                           p_default_params,
                           p_cap_params,
                           sizeof(NOISE_ID_PARAMETERS)))
    {
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = NOISE_ID_SYSMODE_FULL;
    p_ext_data->host_mode = NOISE_ID_SYSMODE_FULL;
    p_ext_data->qact_mode = NOISE_ID_SYSMODE_FULL;

    if(!noise_id_proc_create(p_ext_data))
    {
       return TRUE;
    }

    /* Operator creation was succesful, change respone to STATUS_OK*/
    base_op_change_response_status(resp_data, STATUS_OK);

    L4_DBG_MSG("NOISE_ID: Created");
    return TRUE;
}

bool noise_id_destroy(OPERATOR_DATA *op_data, void *message_data,
                      unsigned *response_id, void **resp_data)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    noise_id_proc_destroy(p_ext_data);

    /* Release class data */
    aud_cur_destroy(op_data);

    L4_DBG_MSG("NOISE_ID: Destroyed");
    return TRUE;
}
/**
 * \brief  Sent an unsolicited message for an NOISE_ID event.
 *
 * \param  op_data      Pointer to operator data
 * \param  p_evt_msg    Pointer to the event message data to end
 *
 * \return - TRUE if successful
 **/
static bool noise_id_send_event_message(OPERATOR_DATA *op_data,
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
        L2_DBG_MSG("Failed to create NOISE ID message payload");
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
    common_send_unsolicited_message(op_data, (unsigned)msg_id, msg_size, p_msg);

    pdelete(p_msg);
    return TRUE;
}
/****************************************************************************
Hook functions
*/
bool noise_id_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    tCbuffer * p_buffer;
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if ((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    p_buffer = aud_cur_get_sink_terminal(op_data, terminal_num);
    switch (terminal_num)
    {
        case NOISE_ID_FF_TERMINAL:
            p_ext_data->p_ff_mic = p_buffer;
            break;
        case NOISE_ID_PASSTHROUGH_TERMINAL:
            break;
        default:
            L2_DBG_MSG1("NOISE_ID: unhandled terminal at connect: %d",
                        terminal_num);
            return FALSE;
    }

    return TRUE;
}

bool noise_id_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* No caching of terminal pointers if it isn't an input terminal */
    if ((terminal_id & TERMINAL_SINK_MASK) == 0)
    {
        return TRUE;
    }

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    switch (terminal_num)
    {
        case NOISE_ID_FF_TERMINAL:
            p_ext_data->p_ff_mic = NULL;
            break;
        case NOISE_ID_PASSTHROUGH_TERMINAL:
            break;
        default:
            L2_DBG_MSG1("NOISE_ID: unhandled terminal at disconnect: %d",
                        terminal_num);
            return FALSE;
    }

    return TRUE;
}

/****************************************************************************
Opmsg handlers
*/
bool noise_id_opmsg_set_current_noise_id(OPERATOR_DATA *op_data,
                                         void *message_data,
                                         unsigned *resp_length,
                                         OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);
    uint16  noise_id = OPMSG_FIELD_GET(message_data,
                                       OPMSG_NOISE_ID_SET_CURRENT_NOISE_ID,
                                       NOISE_ID);
#ifdef RUNNING_ON_KALSIM
    AHM_EVENT_MSG msg;
    msg.id = AHM_EVENT_ID_NOISE_ID;
    msg.type = noise_id;
    noise_id_send_event_message(op_data, &msg);
#endif

    /* Set current noise id and reset the counter */
    aanc_noiseID100_set_current_noise_id(p_ext_data->p_noise_id100, noise_id);
    p_ext_data->previous_nid = noise_id;
    L4_DBG_MSG1("NOISE_ID: set current noise id : %d",noise_id);
    return TRUE;
}
bool noise_id_opmsg_set_control(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);

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
            ctrl_value &= NOISE_ID_SYSMODE_MASK;
            if (ctrl_value >= NOISE_ID_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

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
                    p_ext_data->ovr_control |= NOISE_ID_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= NOISE_ID_OVERRIDE_MODE_MASK;
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
    if ((p_ext_data->ovr_control & NOISE_ID_CONTROL_MODE_OVERRIDE) != 0)
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

bool noise_id_opmsg_get_status(OPERATOR_DATA *op_data,
                               void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);
    int i;
    NOISEID100_DMX *p_nid;
    p_nid = p_ext_data->p_noise_id100;

    /* Build the response */
    unsigned *resp = NULL;
    if (!common_obpm_status_helper(message_data, resp_length, resp_data,
                                   sizeof(NOISE_ID_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp)
    {
        NOISE_ID_STATISTICS stats;
        NOISE_ID_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;

        pstats->OFFSET_CUR_MODE             = p_ext_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL          = p_ext_data->ovr_control;
        pstats->OFFSET_NOISE_ID             = p_nid->noiseID;
        pstats->OFFSET_LOW_TO_MID_RATIO     = p_nid->low_to_mid_ratio;
        pstats->OFFSET_FLAGS                = p_ext_data->flags;

        for (i = 0; i < NOISE_ID_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((NOISE_ID_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[NOISE_ID_N_STAT-1], resp);
        }
    }
    return TRUE;
}

/****************************************************************************
Data processing function
*/
void noise_id_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    NOISE_ID_OP_DATA *p_ext_data = get_instance_data(op_data);

    int sample_count, samples_to_process;
    NOISEID100_DMX *p_nid;
    NOISE_ID_PARAMETERS *p_params;
    AHM_EVENT_MSG msg;
    ED100_DMX *p_ed;
    bool disable_filter_check;              /* Disable filter check indicator */
    unsigned config;
    bool bypass_noiseid;
    /*********************
     * Early exit testing
     *********************/

    samples_to_process = aud_cur_calc_samples(op_data, touched);

    /* Return early if not enough data to process */
    if (samples_to_process < NOISE_ID_DEFAULT_FRAME_SIZE)
    {
        return;
    }

    /* Don't do any processing in standby */
    if (p_ext_data->cur_mode == NOISE_ID_SYSMODE_STANDBY)
    {
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  samples_to_process,
                                  NOISE_ID_TERMINAL_SKIP_MASK);
        /*  Metadata transfer */
        aud_cur_mic_metadata_transfer(op_data, samples_to_process);

        /* Exit early */
        return;
    }
    p_nid = p_ext_data->p_noise_id100;
    p_ed = p_ext_data->p_ed;

    if (aud_cur_get_reinit(op_data))
    {
#ifdef USE_AANC_LICENSING
        /* Set re-init flag to FALSE if license checks have passed. */
        if (p_nid->licensed == TRUE)
#endif
        {
            aud_cur_set_reinit(op_data, FALSE);
        }
        p_params = &p_ext_data->noise_id_params;

        /* Initialize afb */
        aanc_afb_initialize(p_ext_data->f_handle, p_ext_data->p_afb);

        p_nid->sample_rate = NOISE_ID_DEFAULT_SAMPLE_RATE;
        p_nid->frame_size = NOISE_ID_DEFAULT_FRAME_SIZE;
        p_nid->filter_decay = \
            p_params->OFFSET_FILTER_DECAY_TIME;
        p_nid->filter_attack = \
            p_params->OFFSET_FILTER_ATTACK_TIME;
        p_nid->timer_in_sec = \
            p_params->OFFSET_NID_HOLD_TIMER;

        p_nid->id_0_threshold = \
            p_params->OFFSET_POWER_RATIO_ID0_THRESHOLD;
        p_nid->id_1_threshold = \
            p_params->OFFSET_POWER_RATIO_ID1_THRESHOLD;

        aanc_noiseID100_initialize(p_ext_data->f_handle,
                                   p_ext_data->p_noise_id100,
                                   p_ext_data->p_afb);

        /* Initialize the ED */
        config = p_params->OFFSET_NOISE_ID_CONFIG;

        p_ed->p_input = p_ext_data->p_ff_mic;
        p_ed->p_tmp = p_ext_data->p_tmp_ed;
        p_ed->disabled = (config & \
            NOISE_ID_CONFIG_NOISE_ID_CONFIG_DISABLE_ED) > 0;
        p_ed->frame_size = NOISE_ID_DEFAULT_FRAME_SIZE;
        p_ed->attack_time = NOISE_ID_ED_ATTACK;
        p_ed->decay_time = NOISE_ID_ED_DECAY;
        p_ed->envelope_time = p_params->OFFSET_ED_ENVELOPE;
        p_ed->init_frame_time = NOISE_ID_ED_INIT_FRAME;
        p_ed->ratio = p_params->OFFSET_ED_RATIO;
        p_ed->min_signal = p_params->OFFSET_ED_MIN_SIGNAL;
        p_ed->min_max_envelope = \
            p_params->OFFSET_ED_MIN_MAX_ENVELOPE;
        p_ed->delta_th = p_params->OFFSET_ED_DELTA_TH;
        p_ed->count_th = NOISE_ID_ED_COUNT_TH;
        p_ed->hold_frames = p_params->OFFSET_ED_HOLD_FRAMES;
        p_ed->e_min_threshold = \
            p_params->OFFSET_ED_E_FILTER_MIN_THRESHOLD;
        p_ed->e_min_counter_threshold = \
            p_params->OFFSET_ED_E_FILTER_MIN_COUNTER_THRESHOLD;
        disable_filter_check = (config & \
            NOISE_ID_CONFIG_NOISE_ID_CONFIG_DISABLE_ED_E_FILTER_CHECK) > 0;
        p_ed->e_min_check_disabled = disable_filter_check;

        aanc_ed100_initialize(p_ext_data->f_handle, p_ext_data->p_ed);
    }

    bypass_noiseid = (p_ext_data->noise_id_params.OFFSET_NOISE_ID_CONFIG &
                      NOISE_ID_CONFIG_NOISE_ID_CONFIG_BYPASS) > 0;

    sample_count = 0;
    while (samples_to_process >= NOISE_ID_DEFAULT_FRAME_SIZE)
    {
        switch (p_ext_data->cur_mode)
        {
            case NOISE_ID_SYSMODE_FULL:
                if (p_ext_data->p_ff_mic == NULL)
                {
                    L0_DBG_MSG("NOISE ID: Full mode no ff mic");
                    break;
                }
                if (!bypass_noiseid)
                {
                    /* ED process */
                    /* Clear the ED detection in case there was a config update to
                    disable ED while ED was detected. */
                    p_ed->detection = FALSE;
                    if (!p_ed->disabled)
                    {
                        aanc_ed100_process_data(p_ext_data->f_handle, p_ed);
                    }

                    if (!p_ed->detection)
                    {
                        t_fft_object *p_fft = p_ext_data->p_afb->afb.fft_object_ptr;
                        p_fft->real_scratch_ptr = scratch_commit(
                            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM1);
                        p_fft->imag_scratch_ptr = scratch_commit(
                            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);
                        p_fft->fft_scratch_ptr = scratch_commit(
                            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);

                        /* AFB process */
                        aanc_afb_process_data(p_ext_data->f_handle,
                                              p_ext_data->p_afb,
                                              p_ext_data->p_ff_mic);

                        /* Set scratch pointers to NULL before freeing scratch */
                        p_fft->real_scratch_ptr = NULL;
                        p_fft->imag_scratch_ptr = NULL;
                        p_fft->fft_scratch_ptr = NULL;

                        scratch_free();

                        /* Call noise id process function */
                        aanc_noiseID100_process_data(p_ext_data->f_handle,
                                                     p_ext_data->p_noise_id100);
                        if (p_ext_data->previous_nid != p_nid->noiseID)
                        {
                            msg.id = AHM_EVENT_ID_NOISE_ID;
                            msg.type = (uint16)p_nid->noiseID;
                            noise_id_send_event_message(op_data, &msg);
                            p_ext_data->previous_nid = p_nid->noiseID;
                            L4_DBG_MSG1("NOISE_ID: noise id changed : %d",p_nid->noiseID);
                        }
                    }
                }
                break;
            default:
                L2_DBG_MSG1("NOISE_ID: Unsupported sysmode %d",
                            p_ext_data->cur_mode);
        }
        noise_id_update_flags(p_ext_data);

        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  NOISE_ID_DEFAULT_FRAME_SIZE,
                                  NOISE_ID_TERMINAL_SKIP_MASK);

        samples_to_process = aud_cur_calc_samples(op_data, touched);
        sample_count += NOISE_ID_DEFAULT_FRAME_SIZE;
    }

    /* Metadata transfer */
    aud_cur_mic_metadata_transfer(op_data, sample_count);
    return;
}
