/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  aanc2.c
 * \ingroup aanc2
 *
 * Adaptive ANC Controller (AANC2) operator capability.
 *
 * This provides the capability wrapper for AANC2. A majority of handlers are
 * implemented in the base audio curation class, with this wrapper providing:
 *
 * * Create/destroy
 * * Custom hooks for connect/disconnect
 * * Capability process data handler
 *
 * * SET_CONTROL for operator configuration
 * * GET_STATUS for statistics
 *
 * * Event handling - initialization and processing
 * * Plant and control model handlers for the FxLMS
 * * Kalsim message support for simulation
 *
 * Algorithm processing is provided by `aanc2_proc.c`.
 *
 */

/****************************************************************************
Include Files
*/

#include "aanc2.h"

/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define AANC2_16K_CAP_ID   CAP_ID_DOWNLOAD_AANC2_16K
#else
#define AANC2_16K_CAP_ID   CAP_ID_AANC2_16K
#endif

/* Message handlers */
const handler_lookup_struct aanc2_handler_table =
{
    aanc2_create,            /* OPCMD_CREATE */
    aanc2_destroy,           /* OPCMD_DESTROY */
    aud_cur_start,           /* OPCMD_START */
    base_op_stop,            /* OPCMD_STOP */
    aud_cur_reset,           /* OPCMD_RESET */
    aud_cur_connect,         /* OPCMD_CONNECT */
    aud_cur_disconnect,      /* OPCMD_DISCONNECT */
    aud_cur_buffer_details,  /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format, /* OPCMD_DATA_FORMAT */
    aud_cur_get_sched_info   /* OPCMD_GET_SCHED_INFO */
};

/* Null-terminated operator message handler table */
const opmsg_handler_lookup_table_entry aanc2_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             aanc2_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              aanc2_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},
    {OPMSG_COMMON_ID_LINK_ANC_HW_MANAGER,     aanc2_opmsg_link_ahm},
    {OPMSG_COMMON_ID_GET_SHARED_GAIN,         aanc2_opmsg_get_shared_gain_ptr},

    {OPMSG_AANC_ID_SET_AANC_PLANT_COEFFS,     aanc2_opmsg_set_plant_model},
    {OPMSG_AANC_ID_SET_AANC_CONTROL_COEFFS,   aanc2_opmsg_set_control_model},
    {OPMSG_AANC_ID_GET_AANC_ADAPTIVE_GAIN,    aanc2_opmsg_get_adaptive_gain},
    {0, NULL}};

const CAPABILITY_DATA aanc2_16k_cap_data =
    {
        /* Capability ID */
        AANC2_16K_CAP_ID,
        /* Version information - hi and lo */
        AANC2_AANC2_16K_VERSION_MAJOR, AANC2_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        AANC2_MAX_SINKS, AANC2_MAX_SOURCES,
        /* Pointer to message handler function table */
        &aanc2_handler_table,
        /* Pointer to operator message handler function table */
        aanc2_opmsg_handler_table,
        /* Pointer to data processing function */
        aanc2_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(AANC2_OP_DATA)
    };

MAP_INSTANCE_DATA(AANC2_16K_CAP_ID, AANC2_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get AANC2 instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data AANC2_OP_DATA.
 */
static inline AANC2_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (AANC2_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief  Initialize AANC2 events.
 *
 * \param  op_data          Pointer to the operator data
 * \param  p_ext_data       Pointer to the extra op data structure
 *
 * \return  NONE
 *
 * Initialize each event in the operator extra_op_data according to its
 * event ID and duration.
 */
static void aanc2_initialize_events(OPERATOR_DATA *op_data,
                                    AANC2_OP_DATA *p_ext_data)
{
    AANC2_PARAMETERS *p_params = &p_ext_data->aanc2_cap_params;
    aanc2_initialize_event(&p_ext_data->gain_event,
                           op_data,
                           p_params->OFFSET_EVENT_GAIN_STUCK,
                           AANC2_EVENT_ID_GAIN);
    aanc2_initialize_event(&p_ext_data->ed_event,
                           op_data,
                           p_params->OFFSET_EVENT_ED_STUCK,
                           AANC2_EVENT_ID_ED);
    aanc2_initialize_event(&p_ext_data->quiet_event_detect,
                           op_data,
                           p_params->OFFSET_EVENT_QUIET_DETECT,
                           AANC2_EVENT_ID_QUIET);
    aanc2_initialize_event(&p_ext_data->quiet_event_clear,
                           op_data,
                           p_params->OFFSET_EVENT_QUIET_CLEAR,
                           AANC2_EVENT_ID_QUIET);
    aanc2_initialize_event(&p_ext_data->clip_event,
                           op_data,
                           p_params->OFFSET_EVENT_CLIP_STUCK,
                           AANC2_EVENT_ID_CLIP);
    aanc2_initialize_event(&p_ext_data->sat_event,
                           op_data,
                           p_params->OFFSET_EVENT_SAT_STUCK,
                           AANC2_EVENT_ID_SAT);
    aanc2_initialize_event(&p_ext_data->self_talk_event,
                           op_data,
                           p_params->OFFSET_EVENT_SELF_TALK,
                           AANC2_EVENT_ID_SELF_TALK);
    aanc2_initialize_event(&p_ext_data->spl_event,
                           op_data,
                           p_params->OFFSET_EVENT_SPL,
                           AANC2_EVENT_ID_SPL);
    return;
}

/**
 * \brief  Process AANC2 events.
 *
 * \param  p_ext_data       Pointer to the extra op data structure
 *
 * \return  NONE
 *
 * Calculate the event state machine for each event in the operator
 * extra_op_data.
 *
 */
static void aanc2_process_events(AANC2_OP_DATA *p_ext_data)
{
    bool cur_qm, prev_qm;
    int cur_ext;
    uint8 cur_gain;

    AANC2_EVENT *p_event, *p_detect, *p_clear;

    /* Gain stuck event. The event counter will increment every time the
     * previous gain matches the current gain, otherwise is reset. If there
     * is an ED event then this will also reset the event state, sending a
     * negative trigger unsolicited message if the gain stuck event has already
     * been sent.
     */
    p_event = &p_ext_data->gain_event;
    cur_gain = (uint8)p_ext_data->ag.p_fxlms->adaptive_gain;
    p_event->msg.payload = (uint16)cur_gain;
    /* Adaptive gain event: reset if ED detected */
    if (p_ext_data->ag.proc_flags & AANC2_ED_FLAG_MASK)
    {
        /* If we had previously sent a message then send the negative trigger */
        if (p_event->running == AANC2_EVENT_SENT)
        {
            p_event->msg.payload = 0;
            p_event->msg.type = AANC2_EVENT_MSG_NEGATIVE_TRIGGER;
            aanc2_send_event_trigger(&p_event->msg);
        }
        aanc2_clear_event(&p_ext_data->gain_event);
    }
    /* Condition holds */
    else if (cur_gain == p_ext_data->ag.prev_gain)
    {
        aanc2_process_event_detect_condition(p_event);
    }
    /* Condition cleared */
    else
    {
        aanc2_process_event_clear_condition(p_event);
    }

    /* ED stuck event. This uses the generic `process_event` function because
     * the only conditions associated with the event are the current flags
     * and previous flags.
     */
    aanc2_process_event(&p_ext_data->ed_event,
                        p_ext_data->ag.proc_flags & AANC2_ED_FLAG_MASK,
                        p_ext_data->ag.prev_flags & AANC2_ED_FLAG_MASK);

    /* Quiet condition detection event. This event has positive and negative
     * triggers dependent on the state of the detection flags.
     */
    p_detect = &p_ext_data->quiet_event_detect;
    p_clear = &p_ext_data->quiet_event_clear;
    cur_qm = p_ext_data->ag.proc_flags & AANC2_FLAGS_QUIET_MODE;
    prev_qm = p_ext_data->ag.prev_flags & AANC2_FLAGS_QUIET_MODE;
    /* Quiet condition currently detected */
    if (cur_qm > 0)
    {
        /* Quiet condition also in the previous frame. If the event has been
         * detected then decrement the frame counter. When the counter expires
         * then send the positive detection message.
         */
        if (prev_qm > 0)
        {
            if (p_detect->running == AANC2_EVENT_DETECTED)
            {
                p_detect->frame_counter -= 1;
                if (p_detect->frame_counter <= 0)
                {
                    p_detect->msg.type = AANC2_EVENT_MSG_POSITIVE_TRIGGER;
                    aanc2_send_event_trigger(&p_detect->msg);
                    p_detect->running = AANC2_EVENT_SENT;
                    p_ext_data->quiet_condition = TRUE;
                }
            }
        }
        /* Previous frame was not a quiet condition. This is the rising edge
         * for event detection, so change event state and reset the clear event
         * state.
         */
        else
        {
            p_detect->frame_counter -= 1;
            p_detect->running = AANC2_EVENT_DETECTED;
            aanc2_clear_event(p_clear);
        }
    }
    /* Quiet condition not currently detected */
    else
    {
        /* Previous frame was a quiet condition. This is the falling edge for
         * event detection, so change event state on the clear counter and
         * reset the detected event state.
         */
        if (prev_qm)
        {
            p_clear->frame_counter -= 1;
            p_clear->running = AANC2_EVENT_DETECTED;
            aanc2_clear_event(p_detect);
        }
        /* Previous frame was not a quiet condition. Steady state for clearing
         * quiet mode. Decrement the frame counter and once below the threshold
         * indicate that the quiet condition has cleared.
         */
        else
        {
            if (p_clear->running == AANC2_EVENT_DETECTED)
            {
                p_clear->frame_counter -= 1;
                if (p_clear->frame_counter <= 0)
                {
                    p_clear->msg.type = AANC2_EVENT_MSG_NEGATIVE_TRIGGER;
                    aanc2_send_event_trigger(&p_clear->msg);
                    p_clear->running = AANC2_EVENT_SENT;
                    p_ext_data->quiet_condition = FALSE;
                }
            }
        }
    }

    /* Clipping stuck event. This uses the generic `process_event` function
     * because the only conditions associated with the event are the current
     * flags and previous flags.
     */
    aanc2_process_event(&p_ext_data->clip_event,
                        p_ext_data->ag.proc_flags & AANC2_CLIPPING_FLAG_MASK,
                        p_ext_data->ag.prev_flags & AANC2_CLIPPING_FLAG_MASK);

    /* Saturation stuck event. This uses the generic `process_event` function
     * because the only conditions associated with the event are the current
     * flags and previous flags.
     */
    aanc2_process_event(&p_ext_data->sat_event,
                        p_ext_data->ag.proc_flags & AANC2_SATURATION_FLAG_MASK,
                        p_ext_data->ag.prev_flags & AANC2_SATURATION_FLAG_MASK);

    /* Self-talk stuck event. This uses the generic `process_event` function
     * because the only conditions associated with the event are the current
     * flags and previous flags.
     */
    aanc2_process_event(&p_ext_data->self_talk_event,
                        p_ext_data->ag.proc_flags & AANC2_FLAGS_SELF_SPEECH,
                        p_ext_data->ag.prev_flags & AANC2_FLAGS_SELF_SPEECH);

    /* SPL above threshold event. If the external SPL is above the threshold
     * then increment the detection counter, otherwise clear the event.
     */
    cur_ext = p_ext_data->ag.p_ed_ext->spl;
    p_event = &p_ext_data->spl_event;
    p_event->msg.payload = (uint16)(cur_ext >> 16);
    if (cur_ext > p_ext_data->aanc2_cap_params.OFFSET_EVENT_SPL_THRESHOLD)
    {
        aanc2_process_event_detect_condition(p_event);
    }
    else
    {
        aanc2_process_event_clear_condition(p_event);
    }

    /* Update previous flag states */
    aanc2_proc_update_prev_flags(&p_ext_data->ag);

    return;
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
 * This message is used in simulation to provide both a closed loop test
 * (the gain controls a filter outside of kalsim that drives the int mic input)
 * and to provide an indication of the flag status to confirm correct behavior
 * for different processing blocks within the capability.
 */
static void aanc2_send_kalsim_msg(OPERATOR_DATA *op_data,
                                  AANC2_OP_DATA *p_ext_data)
{
    unsigned msg_size = OPMSG_UNSOLICITED_AANC_INFO_WORD_SIZE;
    unsigned *trigger_message = NULL;
    OPMSG_REPLY_ID message_id;

    unsigned flags, filter_config;
    uint16 gain;

    static unsigned prev_flags = 0;
    static unsigned prev_filter_config = INT_MAX;
    static uint16 prev_gain = 0;

    /* Flags are a composite between the capability flags and the processing
     * flags from the AG block.
     */
    flags = p_ext_data->cap_flags | p_ext_data->ag.proc_flags;
    if (p_ext_data->p_ff_fine_gain != NULL)
    {
        gain = (uint16)p_ext_data->p_ff_fine_gain->gain;
    }
    else
    {
        gain = (uint16)p_ext_data->ag.p_fxlms->adaptive_gain;
    }
    filter_config = p_ext_data->filter_config;

    /* Don't send a message if there are no flag or gain changes */
    if ((flags == prev_flags) && (gain == prev_gain) &&
        (filter_config == prev_filter_config))
    {
        return;
    }

    prev_flags = flags;
    prev_filter_config = filter_config;
    prev_gain = gain;

    trigger_message = xzpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        return;
    }

    OPMSG_CREATION_FIELD_SET32(trigger_message,
                               OPMSG_UNSOLICITED_AANC_INFO,
                               FLAGS,
                               flags);

    /* Send the filter configuration */
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AANC_INFO,
                             FILTER_CONFIG,
                             filter_config);

    /* Send the FF fine gain */
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AANC_INFO,
                             FF_FINE_GAIN,
                             gain);
    OPMSG_CREATION_FIELD_SET32(trigger_message,
                               OPMSG_UNSOLICITED_AANC_INFO,
                               OPID,
                               INT_TO_EXT_OPID(op_data->id));
    message_id = OPMSG_REPLY_ID_AANC_TRIGGER;
    common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size,
                                    trigger_message);

    pdelete(trigger_message);

    return;
}
#endif /* RUNNING_ON_KALSIM */

/**
 * \brief  Communicate with the ANC HW Manager to release the shared gain(s)
 *
 * \param  op_data       Pointer to the operator data
 *
 * \return - None
 *
 */
static void aanc2_release_shared_gains(AANC2_OP_DATA *p_ext_data)
{
    AHM_SHARED_FINE_GAIN *p_gain;

    /* Release channel 0 */
    p_gain = p_ext_data->p_ff_fine_gain;

    if ((p_gain == NULL) || (p_ext_data->ahm_op_id == 0))
    {
        /* Nothing to do */
        return;
    }

    aud_cur_release_shared_fine_gain(p_gain,
                                     AHM_ANC_FILTER_FF_ID,
                                     AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                     p_ext_data->ahm_op_id,
                                     AHM_ANC_INSTANCE_ANC0_ID);
    if (p_ext_data->filter_config == AANC2_FILTER_CONFIG_DUAL)
    {
        aud_cur_release_shared_fine_gain(p_gain,
                                         AHM_ANC_FILTER_FF_ID,
                                         AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                         p_ext_data->ahm_op_id,
                                         AHM_ANC_INSTANCE_ANC1_ID);
    }
    p_ext_data->p_ff_fine_gain = NULL;

    return;
}

static bool aanc2_opmsg_link_ahm_callback(CONNECTION_LINK con_id,
                                          STATUS_KYMERA status,
                                          EXT_OP_ID op_id,
                                          unsigned num_resp_params,
                                          unsigned *resp_params)
{
    unsigned raw_ptr, raw_gain, static_gain, cur_gain;
    AANC2_OP_DATA *p_ext_data;
    AHM_SHARED_FINE_GAIN *p_gain;

    AHM_ANC_FILTER anc_filter;
    AHM_ANC_INSTANCE anc_instance;
    unsigned anc_filter_raw, anc_instance_raw;

    raw_ptr = OPMSG_CREATION_FIELD_GET32(resp_params,
                                         OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                         P_EXT_DATA);
    p_ext_data = (AANC2_OP_DATA*)raw_ptr;

    if (status != ACCMD_STATUS_OK)
    {
        L0_DBG_MSG2("OPID: %x, AANC2 link response failed: status=%d", p_ext_data->ag.opid, status);
        return FALSE;
    }
    raw_gain = OPMSG_CREATION_FIELD_GET32(resp_params,
                                          OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                          SHARED_GAIN_PTR);
    p_gain = (AHM_SHARED_FINE_GAIN*)raw_gain;

    static_gain = OPMSG_CREATION_FIELD_GET32(
        resp_params, OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP, STATIC_GAINS_PTR);


    cur_gain = OPMSG_CREATION_FIELD_GET32(
        resp_params, OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP, CURRENT_GAINS_PTR);

    anc_filter_raw = OPMSG_CREATION_FIELD_GET(resp_params,
                                              OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                              FILTER_TYPE);
    anc_filter = (AHM_ANC_FILTER) anc_filter_raw;
    anc_instance_raw = OPMSG_CREATION_FIELD_GET(resp_params,
                                                 OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                                                 CHANNEL);
    anc_instance = (AHM_ANC_INSTANCE) anc_instance_raw;

    if(anc_instance == AHM_ANC_INSTANCE_ANC0_ID && anc_filter == AHM_ANC_FILTER_FF_ID)
    {
        p_ext_data->p_ff_fine_gain = p_gain;
        p_ext_data->p_cur_gain = (AHM_GAIN_BANK*)cur_gain;
        p_ext_data->p_static_gain = (AHM_GAIN_BANK*)static_gain;
    }
    else if(anc_instance == AHM_ANC_INSTANCE_ANC1_ID && anc_filter == AHM_ANC_FILTER_FF_ID)
    {
        p_ext_data->p_ff_fine_gain1 = p_gain;
        p_ext_data->p_cur_gain1 = (AHM_GAIN_BANK*)cur_gain;
        p_ext_data->p_static_gain1 = (AHM_GAIN_BANK*)static_gain;
    }
    else
    {
        L0_DBG_MSG3("OPID: %x, AANC2 invalid ANC instance: %d or ANC filter: %d",p_ext_data->ag.opid, anc_instance, anc_filter);
    }
    return TRUE;
}

/**
 * \brief Prime the FxLMS gain
 *
 * \param p_ext_data  Pointer to operator extra data object
 */

static void aanc2_initialize_gain(AANC2_OP_DATA *p_ext_data)
{

    uint16 gain_value, gain_value1;

    /* Check for the instance0 gain linking */
    if (p_ext_data->p_cur_gain != NULL && p_ext_data->p_static_gain != NULL)
    {
        gain_value = p_ext_data->p_cur_gain->ff.fine;
        if (gain_value == 0)
        {
            gain_value = p_ext_data->p_static_gain->ff.fine;
        }

        /* Check for the instance1 gain linking */
        if (p_ext_data->p_cur_gain1 != NULL && p_ext_data->p_static_gain1 != NULL)
        {
            gain_value1 = p_ext_data->p_cur_gain1->ff.fine;
            if (gain_value1 == 0)
            {
                gain_value1 = p_ext_data->p_static_gain1->ff.fine;
            }
        }
        else
        {
            gain_value1 = gain_value;
        }
    }
    else
    {
        /* Step 3: if the gain hasn't been linked then prime with a default value */
        gain_value = AANC2_DEFAULT_INIT_VALUE;
        gain_value1 = AANC2_DEFAULT_INIT_VALUE;
        L2_DBG_MSG2("OPID: %x, AANC2 init gain: no link, default to %d", p_ext_data->ag.opid, gain_value);
    }

    if (gain_value != p_ext_data->ag.p_fxlms->adaptive_gain ||
        gain_value1 != p_ext_data->ag.p_fxlms->adaptive_gain1)
    {
        aanc_fxlms100_update_gain(p_ext_data->ag.p_fxlms, gain_value, gain_value1);
        p_ext_data->p_ff_fine_gain->gain = (uint8)gain_value;
        p_ext_data->p_ff_fine_gain1->gain = (uint8)gain_value1;
    }
    else
    {
        L2_DBG_MSG3("OPID: %x, AANC2 init gain matches: gain_value: %d, gain_value1: %d",
                     p_ext_data->ag.opid, gain_value, gain_value1);
    }
    return;
}

/****************************************************************************
Capability API Handlers
*/

bool aanc2_create(OPERATOR_DATA *op_data,
                  void *message_data,
                  unsigned *response_id,
                  void **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    L5_DBG_MSG2("OPID: %x, AANC2 Create: p_ext_data at %p", ext_op_id, p_ext_data);

    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Capabilty ID is used to store and retrieve parameters */
    p_ext_data->cap_id = AANC2_16K_CAP_ID;

    /* Sample rate is used to configure the EDs within the capability */
    p_ext_data->sample_rate = 16000;

    /* Create the class data */
    aud_cur_create(op_data, AANC2_MAX_SOURCES, AANC2_MAX_SINKS);
    /* Setup class data with connect and disconnect hooks */
    aud_cur_set_callbacks(op_data,
                          aanc2_start_hook,
                          NULL,
                          aanc2_connect_hook,
                          aanc2_disconnect_hook,
                          NULL);
    /* Setup class data flags */
    aud_cur_set_flags(op_data,
                      AANC2_SUPPORTS_IN_PLACE,
                      AANC2_SUPPORTS_METADATA,
                      AANC2_DYNAMIC_BUFFERS);
    /* Setup class data block and buffer sizes */
    aud_cur_set_block_size(op_data, AANC2_DEFAULT_FRAME_SIZE);
    aud_cur_set_buffer_size(op_data, AANC2_DEFAULT_BUFFER_SIZE);

    /* Initialize parameters */
    p_default_params = (unsigned*)AANC2_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*)&p_ext_data->aanc2_cap_params;
    p_param_def = aud_cur_get_cps(op_data);
    if(!cpsInitParameters(p_param_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(AANC2_PARAMETERS)))
    {
       base_op_change_response_status(resp_data, STATUS_CMD_FAILED);
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = AANC2_SYSMODE_FULL;
    p_ext_data->host_mode = AANC2_SYSMODE_FULL;
    p_ext_data->qact_mode = AANC2_SYSMODE_FULL;

    /* Create any sub-block objects */
    if (!aanc2_proc_create(&p_ext_data->ag, p_ext_data->sample_rate))
    {
        L4_DBG_MSG1("OPID: %x, AANC2 failed to create AG data", ext_op_id);
        base_op_change_response_status(resp_data, STATUS_CMD_FAILED);
        return TRUE;
    }
    p_ext_data->ag.opid = ext_op_id;
    return TRUE;
}

bool aanc2_destroy(OPERATOR_DATA *op_data,
                   void *message_data,
                   unsigned *response_id,
                   void **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* Call base op destroy to create and populate the response */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Release sub-block and shared memory */
    if (p_ext_data != NULL)
    {
        aanc2_proc_destroy(&p_ext_data->ag);
        aanc2_release_shared_gains(p_ext_data);
    }

    /* Release class data */
    aud_cur_destroy(op_data);

    return TRUE;
}

bool aanc2_start_hook(OPERATOR_DATA *op_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    aanc2_initialize_gain(p_ext_data);

    return TRUE;
}

bool aanc2_connect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    uint16 terminal_num;
    tCbuffer *p_external;
    tCbuffer **pp_internal;

    /* Copy the class data terminal buffer pointers to internal refrences
     * in the algorithm structure. This is dependent on the fact that the
     * internal references are stored in the same sequence as the terminals
     * themselves, so the buffer pointer is just incremented by the terminal
     * numberl.
     */
    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    if ((terminal_id & TERMINAL_SINK_MASK) > 0)
    {
        p_external = aud_cur_get_sink_terminal(op_data, terminal_num);
        pp_internal = &p_ext_data->ag.p_playback_ip;
        pp_internal += terminal_num;
        *pp_internal = p_external;
    }
    else
    {
        p_external = aud_cur_get_source_terminal(op_data, terminal_num);
        pp_internal = &p_ext_data->ag.p_playback_op;
        pp_internal += terminal_num;
        *pp_internal = p_external;
    }
    return TRUE;
}

bool aanc2_disconnect_hook(OPERATOR_DATA *op_data, unsigned terminal_id)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    uint16 terminal_num;
    tCbuffer **pp_internal;

    /* Set internal terminal buffer pointers to NULL */
    terminal_num = (uint16)(terminal_id & TERMINAL_NUM_MASK);
    if ((terminal_id & TERMINAL_SINK_MASK) > 0)
    {
        pp_internal = &p_ext_data->ag.p_playback_ip;
        pp_internal += terminal_num;
        *pp_internal = NULL;
    }
    else
    {
        pp_internal = &p_ext_data->ag.p_playback_op;
        pp_internal += terminal_num;
        *pp_internal = NULL;
    }
    return TRUE;
}

/****************************************************************************
Opmsg handlers
*/
bool aanc2_opmsg_set_control(OPERATOR_DATA *op_data,
                             void *message_data,
                             unsigned *resp_length,
                             OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);
    AANC2_PROC *p_ag;
    AANC2_PARAMETERS *p_params;

    unsigned i, num_controls;
    unsigned fxlms_cur_cfg;
    unsigned *p_fxlms_cfg;

    OPMSG_RESULT_STATES result;
    CPS_CONTROL_SOURCE ctrl_src;
    unsigned ctrl_value, ctrl_id;

    if(!cps_control_setup(message_data, resp_length, resp_data, &num_controls))
    {
       return FALSE;
    }

    /* Iterate through the control messages */
    result = OPMSG_RESULT_STATES_NORMAL_STATE;
    for (i=0; i<num_controls; i++)
    {
        ctrl_id = cps_control_get(message_data, i, &ctrl_value, &ctrl_src);

        /* Mode override */
        if (ctrl_id == OPMSG_CONTROL_MODE_ID)
        {
            /* Check for valid mode */
            ctrl_value &= AANC2_SYSMODE_MASK;
            if (ctrl_value >= AANC2_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            /* Only full mode affects AANC. Switching to full mode resets the
             * events and sets the FxLMS gain to the current gain value.
             *
             * A quiet condition would be detected in FULL mode and the system
             * then is switched to FREEZE, which doesn't interrupt any of the
             * event counters for the corresponding clear condition. Only after
             * the quiet condition clears is the capability switched back to
             * FULL mode, which then resets the counters.
             */
            switch (ctrl_value)
            {
                case AANC2_SYSMODE_STANDBY:
                case AANC2_SYSMODE_FREEZE:
                    break;
                case AANC2_SYSMODE_FULL:
                case AANC2_SYSMODE_FULL_CONCURRENCY:
                    aanc2_initialize_events(op_data, p_ext_data);
                    aanc2_proc_reset_prev_flags(&p_ext_data->ag);
                    aanc2_initialize_gain(p_ext_data);
                default:
                    break;
            }

            p_ag = &p_ext_data->ag;
            p_params = &p_ext_data->aanc2_cap_params;

            switch (ctrl_value)
            {
                case AANC2_SYSMODE_STANDBY:
                case AANC2_SYSMODE_FREEZE:
                    break;
                case AANC2_SYSMODE_FULL:
                    aanc2_proc_initialize_concurrency(p_ag, p_params, FALSE);
                    break;
                case AANC2_SYSMODE_FULL_CONCURRENCY:
                    aanc2_proc_initialize_concurrency(p_ag, p_params, TRUE);
                default:
                    break;
            }

            /* Determine whether the mode change comes from the host or from
             * QACT. If it comes from QACT (OBPM) then set the override control
             * flag so that QACT displays the mode override correctly.
             */
            if (ctrl_src == CPS_SOURCE_HOST)
            {
                p_ext_data->host_mode = ctrl_value;
            }
            else
            {
                p_ext_data->qact_mode = ctrl_value;
                if (ctrl_src == CPS_SOURCE_OBPM_ENABLE)
                {
                    p_ext_data->ovr_control |= AANC2_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control = 0;
                }
            }

            continue;
        }

        /* Sample rate control */
        else if (ctrl_id == AANC2_CONSTANT_SAMPLE_RATE_CTRL)
        {
            ctrl_value &= 0x3;
            p_ext_data->ag.p_fxlms->sample_rate_config = ctrl_value;

            /* No override flags indicated for sample rate */
            continue;
        }

        /* Filter configuration control */
        else if (ctrl_id == AANC2_CONSTANT_FILTER_CONFIG_CTRL)
        {
            ctrl_value &= 0x3;
            p_fxlms_cfg = &p_ext_data->ag.p_fxlms->configuration;
            fxlms_cur_cfg = *p_fxlms_cfg & FXLMS100_CONFIG_LAYOUT_MASK_INV;

            switch (ctrl_value)
            {
                case AANC2_FILTER_CONFIG_PARALLEL:
                    *p_fxlms_cfg = fxlms_cur_cfg | FXLMS100_CONFIG_PARALLEL;
                    p_ext_data->filter_config = AANC2_FILTER_CONFIG_PARALLEL;
                    break;
                case AANC2_FILTER_CONFIG_DUAL:
                    *p_fxlms_cfg = fxlms_cur_cfg | FXLMS100_CONFIG_DUAL;
                    p_ext_data->filter_config = AANC2_FILTER_CONFIG_DUAL;
                    break;
                case AANC2_FILTER_CONFIG_SINGLE:
                default:
                    *p_fxlms_cfg = fxlms_cur_cfg | FXLMS100_CONFIG_SINGLE;
                    p_ext_data->filter_config = AANC2_FILTER_CONFIG_SINGLE;
                    break;
            }

            /* No override flags indicated for filter config */
            continue;
        }

        result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
    }

    /* Set current operating mode based on override */
    if ((p_ext_data->ovr_control & AANC2_CONTROL_MODE_OVERRIDE) > 0)
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

bool aanc2_opmsg_get_status(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *resp_length,
                            OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    int i;
    unsigned *resp;

    AANC2_STATISTICS stats;
    FXLMS100_DMX *p_fxlms;
    ParamType *pparam;
    AANC2_PROC *p_ag;

    /* Build the statistics message response */
    if(!common_obpm_status_helper(message_data,
                                  resp_length,
                                  resp_data,
                                  sizeof(AANC2_STATISTICS),
                                  &resp))
    {
         return FALSE;
    }

    if (resp != NULL)
    {
        /* Setup the data for each statistic */
        p_fxlms = p_ext_data->ag.p_fxlms;
        stats.OFFSET_CUR_MODE = p_ext_data->cur_mode;
        stats.OFFSET_OVR_CONTROL = p_ext_data->ovr_control;
        stats.OFFSET_FILTER_CONFIG = p_fxlms->configuration;
        stats.OFFSET_FILTER_SAMPLE_RATE = \
            p_fxlms->sample_rate_config;
        stats.OFFSET_FLAGS = p_ext_data->cap_flags | p_ext_data->ag.proc_flags;
        stats.OFFSET_AG_CALC = p_fxlms->adaptive_gain;

        p_ag = &p_ext_data->ag;
        stats.OFFSET_SPL_EXT = p_ag->p_ed_ext->spl;
        stats.OFFSET_SPL_INT = p_ag->p_ed_int->spl;
        stats.OFFSET_SPL_PB = p_ag->p_ed_pb->spl;
        /* Read and reset peak meters */
        stats.OFFSET_PEAK_EXT = p_ag->clip_ext.peak_value;
        p_ag->clip_ext.peak_value = 0;
        stats.OFFSET_PEAK_INT = p_ag->clip_int.peak_value;
        p_ag->clip_int.peak_value = 0;
        stats.OFFSET_PEAK_PB = p_ag->clip_pb.peak_value;
        p_ag->clip_pb.peak_value = 0;

        pparam = (ParamType*)(&stats);
        for (i=0; i<AANC2_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((AANC2_N_STAT % 2) == 1) /* last one */
        {
            cpsPack1Word(pparam[AANC2_N_STAT-1], resp);
        }
    }

    return TRUE;
}

bool aanc2_opmsg_link_ahm(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *resp_length,
                          OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned link_data;
    uint16 ahm_op_id;
    bool link;

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
        if (p_ext_data->p_ff_fine_gain != NULL)
        {
            L2_DBG_MSG1("OPID: %x, AANC2: link failed: already linked", p_ext_data->ag.opid);
            return FALSE;
        }

        aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                     AHM_ANC_FILTER_FF_ID,
                                     p_ext_data->ahm_op_id,
                                     AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                     AHM_ANC_INSTANCE_ANC0_ID,
                                     aanc2_opmsg_link_ahm_callback);
        if (p_ext_data->filter_config == AANC2_FILTER_CONFIG_DUAL)
        {
            aud_cur_get_shared_fine_gain((void*)p_ext_data,
                                         AHM_ANC_FILTER_FF_ID,
                                         p_ext_data->ahm_op_id,
                                         AHM_GAIN_CONTROL_TYPE_NOMINAL,
                                         AHM_ANC_INSTANCE_ANC1_ID,
                                         aanc2_opmsg_link_ahm_callback);
        }
    }
    else
    {
        aanc2_release_shared_gains(p_ext_data);

    }

    return TRUE;
}

bool aanc2_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                     void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *p_resp;
    unsigned is_coarse_data, p_recv_ext_data, msg_id;
    bool is_coarse;

    p_recv_ext_data = OPMSG_FIELD_GET32(message_data,
                                        OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                        P_EXT_DATA);
    is_coarse_data = OPMSG_FIELD_GET(message_data,
                                     OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                     COARSE);
    is_coarse = (bool)is_coarse_data;

    /* Only handle fine gains */
    if (is_coarse)
    {
        L2_DBG_MSG1("OPID: %x, AANC2: Shared coarse gain not supported",
                     INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    p_resp = xzpnewn(OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        L0_DBG_MSG1("OPID: %x, ANC2: Failed to create shared gain response",
                     INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }
    *resp_length = OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP_WORD_SIZE;
    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    /* Set the message ID */
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                             MESSAGE_ID,
                             msg_id);
    /* Set the extra operator data pointer */
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               P_EXT_DATA,
                               p_recv_ext_data);
    /* Set shared gain pointer */
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               SHARED_GAIN_PTR,
                               (unsigned)p_ext_data->p_ff_fine_gain);


    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}
/****************************************************************************
Custom opmsg handlers
*/
bool aanc2_opmsg_set_plant_model(OPERATOR_DATA *op_data,
                                 void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (!aanc_fxlms100_set_plant_model(p_ext_data->ag.p_fxlms, message_data))
    {
        L4_DBG_MSG1("OPID: %x, AANC set plant coefficients failed", INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    /* Indicate the model loading state and trigger a reinitialization */
    p_ext_data->cap_flags |= AANC2_FLAGS_PLANT_MODEL_LOADED;
    aud_cur_set_reinit(op_data, TRUE);

    return TRUE;
}

bool aanc2_opmsg_set_control_model(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    int destination;

    if (!aanc_fxlms100_set_control_model(p_ext_data->ag.p_fxlms, message_data,
                                         &destination))
    {
        L4_DBG_MSG1("OPID: %x, AANC set control coefficients failed", INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    /* Indicate the model loading state and trigger a reinitialization */
    if (destination)
    {
        p_ext_data->cap_flags |= AANC2_FLAGS_CONTROL_1_MODEL_LOADED;
    }
    else
    {
        p_ext_data->cap_flags |= AANC2_FLAGS_CONTROL_0_MODEL_LOADED;
    }
    aud_cur_set_reinit(op_data, TRUE);

    return TRUE;
}

bool aanc2_opmsg_get_adaptive_gain(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_resp;
    uint16 adaptive_gain;
    uint16 msg_id;

    *resp_length = OPMSG_GET_AANC_ADAPTIVE_GAIN_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_GET_AANC_ADAPTIVE_GAIN_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        L2_DBG_MSG1("OPID: %x, AANC: get_adaptive_gain - Failed to allocate response msg",
                     INT_TO_EXT_OPID(op_data->id));
        return FALSE;
    }

    adaptive_gain = (uint16)p_ext_data->ag.p_fxlms->adaptive_gain;
    msg_id = (uint16)OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AANC_ADAPTIVE_GAIN_RESP,
                             MESSAGE_ID,
                             msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AANC_ADAPTIVE_GAIN_RESP,
                             ADAPTIVE_GAIN,
                             adaptive_gain);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}

/****************************************************************************
Data processing function
*/

void aanc2_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    AANC2_OP_DATA *p_ext_data = get_instance_data(op_data);

    int sample_count;                   /* Total number of samples processed */
    int samples;                        /* Number of samples to process */
    unsigned mode_after_flags;          /* Mode behavior based on flags */
    AANC2_PARAMETERS *p_params;         /* Pointer to capability parameters */
    bool calculate_gain;                /* Gain calculation flag */
    bool run_processing;                /* Run processing flag */
    unsigned *p_gain_calc;              /* Pointer to calculated gain */
    AHM_SHARED_FINE_GAIN *p_ff_gain;    /* Pointer to shared gain to update */
    bool concurrency;                   /* Calculate concurrency status */

    /* Calculate amount of data/space available and update the touched terminal
     * state based on the assumption that if there is enough data (at least one
     * frame) and enough space then that data will be consumed/produced.
     */
    samples = aud_cur_calc_samples(op_data, touched);

     /* Return early if no data or not enough space to process */
    if (samples < AANC2_DEFAULT_FRAME_SIZE)
    {
        return;
    }

    /* Default the gain validity to FALSE to prevent incorrect gains being
     * preserved if the mode is changed from FULL.
     */
    p_ff_gain = p_ext_data->p_ff_fine_gain;
    if (p_ff_gain != NULL)
    {
        p_ff_gain->valid = FALSE;
    }

    run_processing = p_ext_data->cur_mode != AANC2_SYSMODE_STANDBY;

    /* Reinitialize events and the algorithm */
    if (run_processing && aud_cur_get_reinit(op_data))
    {
        /* Clear reinitialization flag */
        aud_cur_set_reinit(op_data, FALSE);

        aanc2_initialize_events(op_data, p_ext_data);
        p_params = &p_ext_data->aanc2_cap_params;
        p_ext_data->disable_events = \
            (p_params->OFFSET_AANC2_DEBUG &
             AANC2_CONFIG_AANC2_DEBUG_DISABLE_EVENT_MESSAGING);

        AANC2_PROC *p_ag = &p_ext_data->ag;

        aanc2_proc_initialize(p_ag, p_params);
        concurrency = p_ext_data->cur_mode == AANC2_SYSMODE_FULL_CONCURRENCY;
        aanc2_proc_initialize_concurrency(p_ag, p_params, concurrency);
    }

    /* Only full mode does gain adaptation */
    calculate_gain = FALSE;
    if ((p_ext_data->cur_mode == AANC2_SYSMODE_FULL) ||
        (p_ext_data->cur_mode == AANC2_SYSMODE_FULL_CONCURRENCY))
    {
        if (p_ff_gain != NULL)
        {
            if (p_ff_gain->using_nominal)
            {
                calculate_gain = TRUE;
            }
        }
        else
        {
            calculate_gain = TRUE;
        }
    }

    sample_count = 0;

    /* Consume all the data in the input buffer, or until there isn't space
     * available in blocks of the frame size.
     */
    do
    {
        if (run_processing)
        {
            if (p_ext_data->quiet_condition)
            {
                calculate_gain = FALSE;
            }
            aanc2_proc_process_data(&p_ext_data->ag,
                                    AANC2_DEFAULT_FRAME_SIZE,
                                    calculate_gain);

            /* Update the mode behavior based on the flags */
            mode_after_flags = p_ext_data->cur_mode;
            if ((p_ext_data->ag.proc_flags & AANC2_ED_FLAG_MASK) > 0)
            {
                L5_DBG_MSG2("OPID: %x, AANC ED detected: %u",
                             INT_TO_EXT_OPID(op_data->id), p_ext_data->ag.proc_flags & AANC2_ED_FLAG_MASK);
                mode_after_flags = AANC2_SYSMODE_FREEZE;
            }

            if ((p_ext_data->ag.proc_flags & AANC2_CLIPPING_FLAG_MASK) > 0)
            {
                L5_DBG_MSG2("OPID: %x, AANC Clipping detected: %u",
                             INT_TO_EXT_OPID(op_data->id), p_ext_data->ag.proc_flags & \
                             AANC2_CLIPPING_FLAG_MASK);
                mode_after_flags = AANC2_SYSMODE_FREEZE;
            }

            if ((p_ext_data->ag.proc_flags & AANC2_SATURATION_FLAG_MASK) > 0)
            {
                L5_DBG_MSG2("OPID: %x, AANC Saturation detected: %u",
                             INT_TO_EXT_OPID(op_data->id), p_ext_data->ag.proc_flags & \
                             AANC2_SATURATION_FLAG_MASK);
                mode_after_flags = AANC2_SYSMODE_FREEZE;
            }

            if (p_ext_data->quiet_condition)
            {
                L5_DBG_MSG1("OPID: %x, AANC Quiet Condition detected",
                             INT_TO_EXT_OPID(op_data->id));
                mode_after_flags = AANC2_SYSMODE_FREEZE;
            }

            /* Update the gain if in FULL mode. Otherwise ensure the shared gain
            * value is marked as invalid so that it isn't accidentally taken.
            */
            p_gain_calc = &p_ext_data->ag.p_fxlms->adaptive_gain;
            p_ff_gain = p_ext_data->p_ff_fine_gain;

            if (((mode_after_flags == AANC2_SYSMODE_FULL) ||
                 (mode_after_flags == AANC2_SYSMODE_FULL_CONCURRENCY)) &&
                p_ff_gain != NULL)
            {
                p_ff_gain->gain = (uint8)*p_gain_calc;
                p_ff_gain->valid = TRUE;
            }

            if (p_ext_data->filter_config == AANC2_FILTER_CONFIG_DUAL)
            {
                /* Update instance 1 */
                p_gain_calc = &p_ext_data->ag.p_fxlms->adaptive_gain1;
                p_ff_gain = p_ext_data->p_ff_fine_gain1;

                if (((mode_after_flags == AANC2_SYSMODE_FULL) ||
                    (mode_after_flags == AANC2_SYSMODE_FULL_CONCURRENCY)) &&
                    p_ff_gain != NULL)
                {
                    p_ff_gain->gain = (uint8)*p_gain_calc;
                    p_ff_gain->valid = TRUE;
                }
            }
            /* Evaluate event messaging criteria */
            if (!p_ext_data->disable_events)
            {
                aanc2_process_events(p_ext_data);
            }

            /* Simulation update */
        #ifdef RUNNING_ON_KALSIM
            aanc2_send_kalsim_msg(op_data, p_ext_data);
        #endif

            /* Transfer data on unused terminals */
            if (p_ext_data->ag.p_fbmon_ip != NULL)
            {
                aud_cur_mic_data_transfer(op_data,
                                          AANC2_DEFAULT_FRAME_SIZE,
                                          AANC2_SKIP_TERMINALS);
            }
        }
        else
        {
            /* Consume all input data when not running the processing
             * algorithms.
             */
            if (p_ext_data->ag.p_playback_ip != NULL)
            {
                aud_cur_playback_data_transfer(op_data,
                                               AANC2_DEFAULT_FRAME_SIZE);
            }
            aud_cur_mic_data_transfer(op_data,
                                      AANC2_DEFAULT_FRAME_SIZE,
                                      AANC2_TRANSFER_ALL_MASK);
        }

        /* Update sample count */
        sample_count += AANC2_DEFAULT_FRAME_SIZE;
        samples = aud_cur_calc_samples(op_data, touched);
    } while (samples >= AANC2_DEFAULT_FRAME_SIZE);

    /* Transfer as much metadata as data has been processed */
    if (p_ext_data->ag.p_playback_ip != NULL)
    {
        aud_cur_playback_metadata_transfer(op_data, sample_count);
    }
    aud_cur_mic_metadata_transfer(op_data, sample_count);

    return;
}
