/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \ingroup capabilities
 * \file  hcgr_cap.c
 * \ingroup hcgr
 *
 * Howling Control and Gain Recovery capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "hcgr_cap.h"
#include "opmgr/opmgr_operator_data.h"

/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define HCGR_16K_CAP_ID   CAP_ID_DOWNLOAD_HCGR_16K
#else
#define HCGR_16K_CAP_ID   CAP_ID_HCGR_16K
#endif

/* Message handlers */
const handler_lookup_struct hcgr_handler_table =
{
    hcgr_create,                   /* OPCMD_CREATE */
    hcgr_destroy,                  /* OPCMD_DESTROY */
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
const opmsg_handler_lookup_table_entry hcgr_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             hcgr_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              hcgr_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},
    {OPMSG_COMMON_ID_LINK_ANC_HW_MANAGER,     hcgr_opmsg_link_ahm},

    {OPMSG_HCGR_ID_HCGR_LINK_TARGET_GAIN_PTR, hcgr_opmsg_link_target_gain},
    {0, NULL}};

const CAPABILITY_DATA hcgr_16k_cap_data =
    {
        /* Capability ID */
        HCGR_16K_CAP_ID,
        /* Version information - hi and lo */
        HCGR_HCGR_16K_VERSION_MAJOR, HCGR_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        HCGR_MAX_TERMINALS, HCGR_MAX_TERMINALS,
        /* Pointer to message handler function table */
        &hcgr_handler_table,
        /* Pointer to operator message handler function table */
        hcgr_opmsg_handler_table,
        /* Pointer to data processing function */
        hcgr_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(HCGR_OP_DATA)
    };

MAP_INSTANCE_DATA(HCGR_16K_CAP_ID, HCGR_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get HCGR instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data HCGR_OP_DATA.
 */
static inline HCGR_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (HCGR_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Static functions
*/
/**
 * \brief  Updates flags in HCGR extra op data.
 *
 * \param  p_ext_data  Pointer to the operator extra data.
 *
 * \return void
 */
static void hcgr_update_flags(HCGR_OP_DATA *p_ext_data)
{
    hcgr_t * p_hcgr = &p_ext_data->hcgr;
    HC100_DMX *p_hc = p_hcgr->p_hc;

    /* Store previous flags */
    p_ext_data->previous_flags = p_ext_data->flags;

    /* Tone detected flag */
    if (p_hc->tone_detected_flag)
    {
        p_ext_data->flags |= HCGR_FLAGS_HOWLING;
    }
    else
    {
        p_ext_data->flags &= ~HCGR_FLAGS_HOWLING;
    }

    /* Recovery active */
    if (p_hcgr->ff_recovery_active)
    {
        p_ext_data->flags |= HCGR_FLAGS_FF_RECOVERY;
    }
    else
    {
        p_ext_data->flags &= ~HCGR_FLAGS_FF_RECOVERY;
    }
    if (p_hcgr->fb_recovery_active)
    {
        p_ext_data->flags |= HCGR_FLAGS_FB_RECOVERY;
    }
    else
    {
        p_ext_data->flags &= ~HCGR_FLAGS_FB_RECOVERY;
    }

    /* Other new flags go here */

    return;
}

/**
 * \brief  Free memory allocated during processing
 *
 * \param  p_ext_data  Address of the HCGR extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool hcgr_proc_destroy(HCGR_OP_DATA *p_ext_data)
{
    hcgr_t * p_hcgr;

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

    p_hcgr = &p_ext_data->hcgr;

    aanc_afb_destroy(p_hcgr->p_afb);
    pfree(p_hcgr->p_afb);
    pfree(p_hcgr->p_hc);
    pfree(p_hcgr->p_hc_dm1);
    pfree(p_hcgr->p_hc_dm2);

    unload_aanc_handle(p_ext_data->f_handle);

    return TRUE;
}

/**
 * \brief  Communicate with the ANC HW Manager to release the shared gain(s)
 *
 * \param  op_data       Pointer to the operator data
 *
 * \return - None
 *
 */
static void hcgr_release_shared_gains(HCGR_OP_DATA *p_ext_data)
{
    AHM_SHARED_FINE_GAIN *p_fb_gain, *p_ff_gain;
    /* Release */
    p_fb_gain = p_ext_data->hcgr.p_fb_fine_gain;
    p_ff_gain = p_ext_data->hcgr.p_ff_fine_gain;

    if (p_ext_data->ahm_op_id == 0)
    {
        /* Nothing to do */
        return;
    }

    if (p_fb_gain != NULL)
    {
        aud_cur_release_shared_fine_gain(p_fb_gain,
                                         AHM_ANC_FILTER_FB_ID,
                                         AHM_GAIN_CONTROL_TYPE_DELTA,
                                         p_ext_data->ahm_op_id,
                                         AHM_ANC_INSTANCE_ANC0_ID);
    }

    if (p_ff_gain != NULL)
    {
        aud_cur_release_shared_fine_gain(p_ff_gain,
                                         AHM_ANC_FILTER_FF_ID,
                                         AHM_GAIN_CONTROL_TYPE_DELTA,
                                         p_ext_data->ahm_op_id,
                                         AHM_ANC_INSTANCE_ANC0_ID);
    }

    p_ext_data->hcgr.p_fb_fine_gain = NULL;
    p_ext_data->hcgr.p_ff_fine_gain = NULL;

    return;
}

static bool hcgr_opmsg_link_ahm_callback(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         EXT_OP_ID op_id,
                                         unsigned num_resp_params,
                                         unsigned *resp_params)
{
    unsigned raw_ptr, raw_gain, cur_gain, nominal_gain, raw_filter;
    unsigned ahm_timer_period;
    HCGR_OP_DATA *p_ext_data;
    AHM_SHARED_FINE_GAIN *p_gain;
    AHM_GAIN_BANK * cur_gain_bank;
    AHM_GAIN_BANK * nominal_gain_bank;
    AHM_ANC_FILTER filter_path;

    raw_ptr = OPMSG_CREATION_FIELD_GET32(resp_params,
                                         OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                         P_EXT_DATA);
    p_ext_data = (HCGR_OP_DATA*)raw_ptr;

    EXT_OP_ID ext_op_id  = p_ext_data->hcgr.ext_op_id;
    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG2("OPID: %x, HCGR link response failed: status=%d", ext_op_id, status);
        return FALSE;
    }
    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          SHARED_GAIN_PTR);
    p_gain = (AHM_SHARED_FINE_GAIN*)raw_gain;

    cur_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          CURRENT_GAINS_PTR);
    cur_gain_bank = (AHM_GAIN_BANK*)cur_gain;

    nominal_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                             OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                             NOMINAL_GAINS_PTR);
    nominal_gain_bank = (AHM_GAIN_BANK*)nominal_gain;

    raw_filter = OPMSG_CREATION_FIELD_GET32(resp_params,
                                            OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                            FILTER_TYPE);
    filter_path = (AHM_ANC_FILTER)raw_filter;

    ahm_timer_period = \
        OPMSG_CREATION_FIELD_GET32(resp_params,
                                   OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                   AHM_TIMER_PERIOD_US);
    p_ext_data->hcgr.ahm_minimal_period_samples = \
        (p_ext_data->sample_rate * ahm_timer_period) / HCGR_MICRO_SEC_CONVERSION_FACTOR;

    switch (filter_path)
    {
        case AHM_ANC_FILTER_FF_ID:
            p_ext_data->hcgr.p_ff_fine_gain = p_gain;
            p_ext_data->hcgr.p_ff_current_gain = \
                &cur_gain_bank->ff;
            p_ext_data->hcgr.p_ff_nominal_gain = \
                &nominal_gain_bank->ff;
            break;
        case AHM_ANC_FILTER_FB_ID:
            p_ext_data->hcgr.p_fb_fine_gain = p_gain;
            p_ext_data->hcgr.p_fb_current_gain = \
                &cur_gain_bank->fb;
            p_ext_data->hcgr.p_fb_nominal_gain = \
                &nominal_gain_bank->fb;
            break;
        default:
            L2_DBG_MSG2("OPID: %x, HCGR unsupported filter path for gain control: %u",
                        ext_op_id, filter_path);
            break;
    }

    return TRUE;
}

/****************************************************************************
Capability API Handlers
*/

bool hcgr_create(OPERATOR_DATA *op_data, void *message_data,
                 unsigned *response_id, void **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */
    EXT_OP_ID ext_op_id  = INT_TO_EXT_OPID(op_data->id);
    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    L5_DBG_MSG2("OPID: %x, HCGR Create: p_ext_data at %p", ext_op_id, p_ext_data);

    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Assume the response to be command FAILED. If we reach the correct
     * termination point in create then change it to STATUS_OK.
     */
    base_op_change_response_status(resp_data, STATUS_CMD_FAILED);

    /* Multi-channel create */
    aud_cur_create(op_data, HCGR_MAX_TERMINALS, HCGR_MAX_TERMINALS);
    aud_cur_set_callbacks(op_data,
                          NULL,
                          NULL,
                          hcgr_connect_hook,
                          hcgr_disconnect_hook,
                          hcgr_param_update_hook);
    aud_cur_set_flags(op_data,
                      HCGR_SUPPORTS_IN_PLACE,
                      HCGR_SUPPORTS_METADATA,
                      HCGR_DYNAMIC_BUFFERS_FALSE);
    aud_cur_set_min_terminal_masks(op_data,
                                   HCGR_MIN_VALID_SOURCES,
                                   HCGR_MIN_VALID_SINKS);

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = HCGR_16K_CAP_ID;

    p_ext_data->sample_rate = 16000;

    aud_cur_set_buffer_size(op_data, HCGR_DEFAULT_BUFFER_SIZE);
    aud_cur_set_block_size(op_data, HCGR_DEFAULT_BLOCK_SIZE);

    /* Initialize parameters */
    p_default_params = (unsigned*) HCGR_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->hcgr_cap_params;
    p_param_def = aud_cur_get_cps(op_data);
    if (!cpsInitParameters(p_param_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(HCGR_PARAMETERS)))
    {
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = HCGR_SYSMODE_FULL;
    p_ext_data->host_mode = HCGR_SYSMODE_FULL;
    p_ext_data->qact_mode = HCGR_SYSMODE_FULL;

    /* Allocate twiddle factor for AFB */
    if (!math_fft_twiddle_alloc(AANC_FILTER_BANK_WINDOW_SIZE))
    {
        hcgr_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, HCGR failed to allocate twiddle factors", ext_op_id);
        return FALSE;
    }
    p_ext_data->twiddle_registered = TRUE;

    /* Register scratch memory for AFB & allocate object */
    if (!scratch_register())
    {
        hcgr_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, HCGR failed to register scratch memory", ext_op_id);
        return FALSE;
    }

    p_ext_data->scratch_registered = TRUE;

    if (!scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM1) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2))
    {
        hcgr_proc_destroy(p_ext_data);
        L2_DBG_MSG1("OPID: %x, HCGR failed to reserve scratch memory", ext_op_id);
        return FALSE;
    }

    if(!hcgr_proc_create(&p_ext_data->hcgr, &p_ext_data->hcgr_cap_params, &p_ext_data->f_handle))
    {
        hcgr_proc_destroy(p_ext_data);
        return FALSE;
    }

    /* Operator creation was succesful, change respone to STATUS_OK*/
    base_op_change_response_status(resp_data, STATUS_OK);

    p_ext_data->hcgr.ext_op_id = ext_op_id;
    L4_DBG_MSG1("OPID: %x, HCGR: Created", ext_op_id);
    return TRUE;
}

bool hcgr_destroy(OPERATOR_DATA *op_data, void *message_data,
                  unsigned *response_id, void **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* TODO: patch functions */

    if (p_ext_data != NULL)
    {
        hcgr_proc_destroy(p_ext_data);
        L4_DBG_MSG1("OPID: %x, HCGR: Cleanup complete.", INT_TO_EXT_OPID(op_data->id));

        hcgr_release_shared_gains(p_ext_data);
    }

    /* Release class data */
    aud_cur_destroy(op_data);

    L4_DBG_MSG1("OPID: %x, HCGR: Destroyed", INT_TO_EXT_OPID(op_data->id));
    return TRUE;
}

/****************************************************************************
Hook functions
*/
bool hcgr_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    tCbuffer * p_buffer;
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);

    if (terminal_num == HCGR_TERMINAL)
    {
        if((terminal_id & TERMINAL_SINK_MASK) > 0)
        {
            p_buffer = aud_cur_get_sink_terminal(op_data, terminal_num);
            p_ext_data->hcgr.p_in_buf = p_buffer;
        }
    }


    return TRUE;
}

bool hcgr_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    uint16 terminal_num;
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);

    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);

    if (terminal_num == HCGR_TERMINAL)
    {
        if ((terminal_id & TERMINAL_SINK_MASK) > 0)
        {
            p_ext_data->hcgr.p_in_buf = NULL;
        }
    }

    return TRUE;
}

bool hcgr_param_update_hook(OPERATOR_DATA *op_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);
    AHM_ANC_FILTER old_filter_path, new_filter_path;

    /* Re-link AHM shared gains.
       Ignore if AHM is not linked. This can happen if SET_UCID/SET_PARAMS comes
       before linking AHM. */
    if (p_ext_data->ahm_op_id != 0)
    {
        old_filter_path = p_ext_data->hcgr.hcgr_filter_path;
        hcgr_init_filter_path(&p_ext_data->hcgr, &p_ext_data->hcgr_cap_params);
        new_filter_path = p_ext_data->hcgr.hcgr_filter_path;

        /* Re-link if filter path has changed */
        if(old_filter_path != new_filter_path)
        {
            hcgr_release_shared_gains(p_ext_data);
            aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                         AHM_ANC_FILTER_FB_ID,
                                         p_ext_data->ahm_op_id,
                                         AHM_GAIN_CONTROL_TYPE_DELTA,
                                         AHM_ANC_INSTANCE_ANC0_ID,
                                         hcgr_opmsg_link_ahm_callback);
            if (new_filter_path == AHM_ANC_FILTER_FF_AND_FB_ID)
            {
                aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                             AHM_ANC_FILTER_FF_ID,
                                             p_ext_data->ahm_op_id,
                                             AHM_GAIN_CONTROL_TYPE_DELTA,
                                             AHM_ANC_INSTANCE_ANC0_ID,
                                             hcgr_opmsg_link_ahm_callback);
            }
        }
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
 * status to confirm correct behavior of howling control and recovery blocks.
 *
 * The message structure is shared with AANC. The most significant nibble in the
 * FLAGS field is used by HCGR
 */
static void hcgr_send_kalsim_msg_on_flags_update(OPERATOR_DATA *op_data,
                                                 HCGR_OP_DATA *p_ext_data)
{
    unsigned msg_size = OPMSG_UNSOLICITED_AANC_INFO_WORD_SIZE;
    unsigned *trigger_message = NULL;
    OPMSG_REPLY_ID message_id;
    unsigned flags;

    /* Send Kalsim msg if flags have changed */
    if (p_ext_data->previous_flags != p_ext_data->flags)
    {
        trigger_message = xzpnewn(msg_size, unsigned);
        if (trigger_message == NULL)
        {
            return;
        }

        /* Most significant nibble in AANC kalsim FLAGS is reserved for HCGR */
        flags = (p_ext_data->flags & HCGR_KALSIM_FLAG_MASK) \
            << HCGR_KALSIM_FLAGS_SHIFT;
        OPMSG_CREATION_FIELD_SET32(trigger_message,
                                OPMSG_UNSOLICITED_AANC_INFO,
                                FLAGS,
                                flags);
        OPMSG_CREATION_FIELD_SET32(trigger_message,
                                   OPMSG_UNSOLICITED_AANC_INFO,
                                   OPID,
                                   INT_TO_EXT_OPID(op_data->id));
        message_id = OPMSG_REPLY_ID_AANC_TRIGGER;
        common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size,
                                        trigger_message);

        pdelete(trigger_message);
    }
    return;
}
#endif /* RUNNING_ON_KALSIM */
/****************************************************************************
Opmsg handlers
*/
bool hcgr_opmsg_set_control(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *resp_length,
                            OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);

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
            ctrl_value &= HCGR_SYSMODE_MASK;
            if (ctrl_value >= HCGR_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            /* Update current mode */
            p_ext_data->cur_mode = ctrl_value;

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
                    p_ext_data->ovr_control |= HCGR_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= HCGR_OVERRIDE_MODE_MASK;
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
    if ((p_ext_data->ovr_control & HCGR_CONTROL_MODE_OVERRIDE) != 0)
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

bool hcgr_opmsg_get_status(OPERATOR_DATA *op_data,
                           void *message_data,
                           unsigned *resp_length,
                           OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);
    /* TODO: patch functions */
    int i;
    int *p_blk_exp;
    hcgr_t * p_hcgr = &p_ext_data->hcgr;          /* Pointer to HCGR */

    /* Build the response */
    unsigned *resp = NULL;
    if (!common_obpm_status_helper(message_data, resp_length, resp_data,
                                  sizeof(HCGR_STATISTICS), &resp))
    {
         return FALSE;
    }
#ifdef USE_AANC_LICENSING
    HC100_DMX *p_hc = p_hcgr->p_hc;              /* Pointer to Howling control data */
    if ((resp != 0) && (p_hc->licensed == TRUE))
#else
    if (resp)
#endif
    {
        HCGR_STATISTICS stats;
        HCGR_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;
        pstats->OFFSET_CUR_MODE             = p_ext_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL          = p_ext_data->ovr_control;
        pstats->OFFSET_FLAGS                = p_ext_data->flags;
        pstats->OFFSET_FB_HW_TARGET_GAIN    = p_hcgr->fb_hw_target_gain;
        pstats->OFFSET_FF_HW_TARGET_GAIN    = p_hcgr->ff_hw_target_gain;
        pstats->OFFSET_HCGR_FB_GAIN         = p_hcgr->p_fb_fine_gain->gain_current;
        pstats->OFFSET_HCGR_FF_GAIN         = p_hcgr->p_ff_fine_gain->gain_current;
        p_blk_exp = p_hcgr->p_afb->afb.freq_output_object_ptr->exp_ptr;
        pstats->OFFSET_BLOCK_LEVEL          = *p_blk_exp * HCGR_DB_PER_LVL;
        if (p_hcgr->latch_bexp >= 0)
        {
            pstats->OFFSET_HOWL_BLOCK_LEVEL = \
                p_hcgr->latch_bexp * HCGR_DB_PER_LVL;
            pstats->OFFSET_HOWL_FREQ        = \
                p_hcgr->latch_max_bin * HCGR_FREQ_PER_BIN;
        }
        else
        {
            pstats->OFFSET_HOWL_BLOCK_LEVEL = HCGR_CLEAR_BIN;
            pstats->OFFSET_HOWL_FREQ        = HCGR_CLEAR_FREQ;
        }

        for (i = 0; i < HCGR_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((HCGR_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[HCGR_N_STAT-1], resp);
        }
    }
#ifdef USE_AANC_LICENSING
   else if(resp != 0)
   {
        HCGR_STATISTICS stats;
        HCGR_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;
        pstats->OFFSET_CUR_MODE             = 0;
        pstats->OFFSET_OVR_CONTROL          = 0;
        pstats->OFFSET_FLAGS                = 0;
        pstats->OFFSET_FB_HW_TARGET_GAIN    = 0;
        pstats->OFFSET_FF_HW_TARGET_GAIN    = 0;
        pstats->OFFSET_HCGR_FB_GAIN         = 0;
        pstats->OFFSET_HCGR_FF_GAIN         = 0;
        pstats->OFFSET_BLOCK_LEVEL          = 0;
        pstats->OFFSET_HOWL_BLOCK_LEVEL     = 0;
        pstats->OFFSET_HOWL_FREQ            = 0;
        for (i = 0; i < HCGR_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((HCGR_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[HCGR_N_STAT-1], resp);
        }
   }   
#endif

   return TRUE;
}

bool hcgr_opmsg_link_ahm(OPERATOR_DATA *op_data,
                         void *message_data,
                         unsigned *resp_length,
                         OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned link_data;
    uint16 ahm_op_id;
    bool link;
    AHM_ANC_FILTER filter_path;

    link_data = OPMSG_FIELD_GET(message_data,
                                OPMSG_COMMON_MSG_LINK_ANC_HW_MANAGER,
                                LINK);
    ahm_op_id = OPMSG_FIELD_GET(message_data,
                                OPMSG_COMMON_MSG_LINK_ANC_HW_MANAGER,
                                AHM_OP_ID);

    p_ext_data->ahm_op_id = ahm_op_id;

    link = (bool)link_data;

    if (link)
    {
        if (p_ext_data->hcgr.p_fb_fine_gain != NULL)
        {
            L2_DBG_MSG1("OPID: %x, HCGR: link failed: FB path already linked", INT_TO_EXT_OPID(op_data->id));
            return FALSE;
        }

        /* It is assumed that UCID will be set before linking AHM gains */
        hcgr_init_filter_path(&p_ext_data->hcgr, &p_ext_data->hcgr_cap_params);

        filter_path = p_ext_data->hcgr.hcgr_filter_path;

        /* FB path is always controlled by HCGR */
        aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                     AHM_ANC_FILTER_FB_ID,
                                     p_ext_data->ahm_op_id,
                                     AHM_GAIN_CONTROL_TYPE_DELTA,
                                     AHM_ANC_INSTANCE_ANC0_ID,
                                     hcgr_opmsg_link_ahm_callback);
        if (filter_path == AHM_ANC_FILTER_FF_AND_FB_ID)
        {
            if (p_ext_data->hcgr.p_ff_fine_gain != NULL)
            {
                L2_DBG_MSG1("OPID: %x, HCGR: link failed: FF path already linked", INT_TO_EXT_OPID(op_data->id));
                return FALSE;
            }
            aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                         AHM_ANC_FILTER_FF_ID,
                                         p_ext_data->ahm_op_id,
                                         AHM_GAIN_CONTROL_TYPE_DELTA,
                                         AHM_ANC_INSTANCE_ANC0_ID,
                                         hcgr_opmsg_link_ahm_callback);
        }
    }
    else
    {
        hcgr_release_shared_gains(p_ext_data);
    }

    return TRUE;
}

/****************************************************************************
Custom opmsg handlers
*/
/**
 * \brief  ANC Compander get shared gain callback handler
 *
 * \param  con_id          Connection ID
 * \param  status          Message status
 * \param  op_id           Operator ID
 * \param  num_resp_params Number of response parameters
 * \param  resp_params     Pointer to response data
 *
 * \return  boolean indicating success or failure.
 *
 */
static bool hcgr_opmsg_link_target_gain_callback(CONNECTION_LINK con_id,
                                                 STATUS_KYMERA status,
                                                 EXT_OP_ID op_id,
                                                 unsigned num_resp_params,
                                                 unsigned *resp_params)
{
    unsigned raw_ptr, raw_gain;
    HCGR_OP_DATA *p_ext_data;
    AHM_SHARED_FINE_GAIN *p_adrc_gain;

    /* Get p_ext_data pointer */
    raw_ptr = OPMSG_CREATION_FIELD_GET32(resp_params,
                                         OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                         P_EXT_DATA);
    p_ext_data = (HCGR_OP_DATA*)raw_ptr;
    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG2("OPID: %x, HCGR link target gain response failed: status=%d", p_ext_data->hcgr.ext_op_id, status);
        return FALSE;
    }

    /* Get shared gain pointer */
    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          SHARED_GAIN_PTR);
    p_adrc_gain = (AHM_SHARED_FINE_GAIN*)raw_gain;

    p_ext_data->hcgr.p_adrc_gain = p_adrc_gain;

    return TRUE;
}
bool hcgr_opmsg_link_target_gain(OPERATOR_DATA *op_data,
                                 void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);
    hcgr_t * p_hcgr;

    unsigned adrc_op_id;        /* ANC Compander operator ID */
    uint16 is_coarse_gain;      /* Requested gain is coarse or fine gain */

    adrc_op_id = OPMSG_FIELD_GET32(message_data,
                                   OPMSG_HCGR_LINK_TARGET_GAIN_PTR,
                                   TARGET_OP_ID);
    is_coarse_gain = OPMSG_FIELD_GET(message_data,
                                     OPMSG_HCGR_LINK_TARGET_GAIN_PTR,
                                     COARSE);

    /* Check parameter validity */
    if((adrc_op_id == 0) ||
       (is_coarse_gain == TRUE))
    {
        L2_DBG_MSG3("OPID: %x, HCGR: Link target gain: Invalid parameters - \
                    adrc_op_id 0x%x, is_coarse_gain %u",
                    INT_TO_EXT_OPID(op_data->id),
                    adrc_op_id, is_coarse_gain);
        return FALSE;
    }

    /* Check if ANC Compander gain is already linked */
    p_hcgr = &p_ext_data->hcgr;
    if(p_hcgr->p_adrc_gain != NULL)
    {
        L2_DBG_MSG1("OPID: %x, HCGR: Link target gain failed: already linked",
                    INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                 0, /* unused */
                                 adrc_op_id,
                                 0, /* unused */
                                 AHM_ANC_INSTANCE_ANC0_ID,
                                 hcgr_opmsg_link_target_gain_callback);

    return TRUE;
}
/****************************************************************************
Data processing function
*/
void hcgr_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    HCGR_OP_DATA *p_ext_data = get_instance_data(op_data);
    hcgr_t * p_hcgr;               /* Pointer to HCGR */
    HC100_DMX *p_hc;               /* Pointer to Howling control data */
    int sample_count, samples_to_process;
    AHM_SHARED_FINE_GAIN *p_shared_ff_gain;
    AHM_SHARED_FINE_GAIN *p_shared_fb_gain;
    bool bypass_hcgr;

    /*********************
     * Early exit testing
     *********************/

    samples_to_process = aud_cur_calc_samples(op_data, touched);

     /* Return early if not enough data to process */
    if (samples_to_process < HCGR_DEFAULT_FRAME_SIZE)
    {
        L5_DBG_MSG2("OPID: %x, Not enough data to process (%d)", p_ext_data->hcgr.ext_op_id, samples_to_process);
        return;
    }

    /* Default validity to FALSE so that any early exit testing not related to
     * not having enough samples to process ensures an invalid gain is not used
     * by the ANC HW Manager.
     */
    p_shared_fb_gain = p_ext_data->hcgr.p_fb_fine_gain;
    p_shared_ff_gain = p_ext_data->hcgr.p_ff_fine_gain;

    if (p_shared_fb_gain != NULL)
    {
        p_shared_fb_gain->valid = FALSE;
    }
    if (p_shared_ff_gain != NULL)
    {
        p_shared_ff_gain->valid = FALSE;
    }

    /* Don't do any processing in standby */
    if (p_ext_data->cur_mode == HCGR_SYSMODE_STANDBY)
    {
        /* Reset delta gains */
        p_shared_fb_gain->gain_delta = AHM_DELTA_NOMINAL;
        if (p_shared_ff_gain != NULL)
        {
            p_shared_ff_gain->gain_delta = AHM_DELTA_NOMINAL;
        }
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  samples_to_process,
                                  HCGR_TERMINAL_SKIP_MASK);
        /*  Metadata transfer */
        aud_cur_mic_metadata_transfer(op_data, samples_to_process);

        /* Exit early */
        return;
    }

    if (aud_cur_get_reinit(op_data))
    {
        /* Get pointers */
        p_hcgr = &p_ext_data->hcgr;
        p_hc = p_hcgr->p_hc;
#ifdef USE_AANC_LICENSING
        /* Set re-init flag to FALSE if license checks have passed. */
        if (p_hc->licensed == TRUE)
#endif
        {
        aud_cur_set_reinit(op_data, FALSE);
        }

        /* Initialize afb */
        aanc_afb_initialize(p_ext_data->f_handle, p_hcgr->p_afb);
        hcgr_proc_intialize(&p_ext_data->hcgr, &p_ext_data->hcgr_cap_params,
                            p_ext_data->f_handle, p_ext_data->sample_rate);
    }

    bypass_hcgr = (p_ext_data->hcgr_cap_params.OFFSET_HCGR_CONFIG &
                   HCGR_CONFIG_BYPASS) > 0;

    sample_count = 0;
    while (samples_to_process >= HCGR_DEFAULT_FRAME_SIZE)
    {
        if (!bypass_hcgr)
        {
            p_hcgr = &p_ext_data->hcgr;
            p_hc = p_hcgr->p_hc;

            t_fft_object *p_fft = p_hcgr->p_afb->afb.fft_object_ptr;
            p_fft->real_scratch_ptr = scratch_commit(
                AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM1);
            p_fft->imag_scratch_ptr = scratch_commit(
                AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);
            p_fft->fft_scratch_ptr = scratch_commit(
                AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);

            /* AFB process */
            aanc_afb_process_data(p_ext_data->f_handle,
                                  p_hcgr->p_afb,
                                  p_hcgr->p_in_buf);

            /* Set scratch pointers to NULL before freeing scratch */
            p_fft->real_scratch_ptr = NULL;
            p_fft->imag_scratch_ptr = NULL;
            p_fft->fft_scratch_ptr = NULL;

            scratch_free();

            /* Call howling control function */
            aanc_hc100_process_data(p_ext_data->f_handle, p_hc);

            /* Recovery process on FF & FB path */
            hcgr_proc_recovery(&p_ext_data->hcgr, &p_ext_data->hcgr_cap_params);
        }
        /* Copy or discard data on all terminals */
        aud_cur_mic_data_transfer(op_data,
                                  HCGR_DEFAULT_FRAME_SIZE,
                                  HCGR_TERMINAL_SKIP_MASK);

        samples_to_process = aud_cur_calc_samples(op_data, touched);
        sample_count += HCGR_DEFAULT_FRAME_SIZE;
    }
    /*  Metadata transfer */
    aud_cur_mic_metadata_transfer(op_data, sample_count);

    /* Update flags */
    hcgr_update_flags(p_ext_data);

#ifdef RUNNING_ON_KALSIM
    hcgr_send_kalsim_msg_on_flags_update(op_data, p_ext_data);
#endif

    /***************************
     * Update touched terminals
     ***************************/
    touched->sinks = (unsigned) HCGR_MIN_VALID_SINKS;

    L5_DBG_MSG1("OPID: %x, HCGR process data completed", p_ext_data->hcgr.ext_op_id);

    return;
}
