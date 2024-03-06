/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager.c
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) operator capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "anc_hw_manager.h"
//Macros to enable logging of events

//#define DEBUG_AHM_LOG
#ifdef DEBUG_AHM_LOG
#define DEBUG_AHM(x, a) L2_DBG_MSG2(x " - time = %d", a, time_get_time())
#else
#define DEBUG_AHM(x, a) ((void)0)
#endif


/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define ANC_HW_MANAGER_CAP_ID   CAP_ID_DOWNLOAD_ANC_HW_MANAGER
#else
#define ANC_HW_MANAGER_CAP_ID   CAP_ID_ANC_HW_MANAGER
#endif

/* Message handlers */
const handler_lookup_struct ahm_handler_table =
{
    ahm_create,              /* OPCMD_CREATE */
    ahm_destroy,             /* OPCMD_DESTROY */
    aud_cur_start,           /* OPCMD_START */
    aud_cur_stop,            /* OPCMD_STOP */
    aud_cur_reset,           /* OPCMD_RESET */
    aud_cur_connect,         /* OPCMD_CONNECT */
    aud_cur_disconnect,      /* OPCMD_DISCONNECT */
    aud_cur_buffer_details,  /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format, /* OPCMD_DATA_FORMAT */
    aud_cur_get_sched_info   /* OPCMD_GET_SCHED_INFO */
};

/* Null-terminated operator message handler table */
const opmsg_handler_lookup_table_entry ahm_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             ahm_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              aud_cur_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            aud_cur_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              aud_cur_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              ahm_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                aud_cur_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       aud_cur_opmsg_get_ps_id},
    {OPMSG_COMMON_SET_SAMPLE_RATE,            ahm_opmsg_set_sample_rate},
    {OPMSG_COMMON_ID_SET_BUFFER_SIZE,         aud_cur_opmsg_set_buffer_size},
    {OPMSG_COMMON_ID_GET_SHARED_GAIN,         ahm_opmsg_get_shared_gain_ptr},

    {OPMSG_AHM_ID_SET_AHM_STATIC_GAINS,       ahm_opmsg_set_static_gains},
    {OPMSG_AHM_ID_GET_AHM_GAINS,              ahm_opmsg_get_gains},
    {OPMSG_AHM_ID_SET_TARGET_MAKEUP_GAIN,     ahm_opmsg_set_target_makeup_gain},
    {OPMSG_AHM_ID_FREE_AHM_SHARED_GAIN_PTR,   ahm_opmsg_free_shared_gain_ptr},
    {OPMSG_AHM_ID_SET_TIMER_PERIOD,           ahm_opmsg_set_timer_period},
    {OPMSG_AHM_ID_SET_IIR_FILTER_COEFFS,      ahm_opmsg_set_iir_filter_coeffs},
    {OPMSG_AHM_ID_SET_FINE_TARGET_GAIN,       ahm_opmsg_set_fine_target_gain},
    {OPMSG_AHM_ID_SET_ZCD_DISABLE,            ahm_opmsg_set_zcd_disable}};

const CAPABILITY_DATA anc_hw_manager_cap_data =
    {
        /* Capability ID */
        ANC_HW_MANAGER_CAP_ID,
        /* Version information - hi and lo */
        ANC_HW_MANAGER_ANC_HW_MANAGER_VERSION_MAJOR, AHM_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        AHM_MAX_SINKS, AHM_MAX_SOURCES,
        /* Pointer to message handler function table */
        &ahm_handler_table,
        /* Pointer to operator message handler function table */
        ahm_opmsg_handler_table,
        /* Pointer to data processing function */
        ahm_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(AHM_OP_DATA)
    };

MAP_INSTANCE_DATA(ANC_HW_MANAGER_CAP_ID, AHM_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get AHM instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data AHM_OP_DATA.
 */
static inline AHM_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (AHM_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Private Functions
*/

/**
 * \brief  Interpret target fine gain value
 *
 * \param  p_param      Pointer to the raw parameter value
 * \param  static_gain  Static gain value
 * \param  current_gain Current gain value
 *
 * \return  Target fine gain value
 */
static uint16 interpret_fine_gain(unsigned *p_param,
                                  uint16 static_gain,
                                  uint16 current_gain,
                                  EXT_OP_ID ext_op_id)
{
    FINE_GAIN_PARAM *p_param_value = (FINE_GAIN_PARAM*)p_param;
    uint16 round_factor = 0;
    uint16 gain;
    int16 shift;
    uint8 gain_type = (uint8)(p_param_value->gain_type & AHM_GAIN_SHIFT_MASK);

    if (gain_type == AHM_GAIN_ABSOLUTE)
    {
        return (uint16)p_param_value->absolute;
    }
    else
    {
        switch (gain_type)
        {
            case AHM_GAIN_SHIFT_CURRENT:
                gain = current_gain;
                shift = p_param_value->shift_current;
                break;
            case AHM_GAIN_SHIFT_STATIC:
                gain = static_gain;
                shift = p_param_value->shift_static;
                break;
            default:
                L0_DBG_MSG2("OPID: %x, AANC gain type invalid: %u",
                            ext_op_id, p_param_value->gain_type);
                return FALSE;
        }
    }
    /* Apply rounding only for right shift */
    if (shift < 0)
    {
        /* The shift value is negative, hence negating it again */
        round_factor = (uint16)(1 << (0 - shift - 1));
    }
    return (uint16)((gain + round_factor) << shift);
}

/**
 * \brief  Function to calculate samples per timer period.
 *
 * This is provided to allow the basic ramp operation to be calculated the same
 * way regardless of whether real samples are processed (AHM in the audio
 * chain) or AHM is operating on a timer task.
 *
 * \param  sample_rate  Capability sample rate
 * \param  timer_period  Timer period duration
 * \param  decimation  Timer decimation factor
 *
 * \return unsigned  Samples per timer period.
 */
static inline unsigned ahm_calc_samples_per_period(unsigned sample_rate,
                                                   unsigned timer_period,
                                                   uint16 decimation)
{
    /* sample_rate / 1000 = samples per ms
     * timer_period / 1000 * decimation = period in ms
     * multiply together for samples per period
     */
    unsigned raw_samples = (sample_rate * timer_period) / 1000000;
    return raw_samples * (unsigned)decimation;
}


/**
 * \brief  Update the dynamic nominal gain value
 *
 * Update the nominal gain values for dynamic gain paths
 *
 * \param  pp_src_nominal  Pointer to the head of the nominal gain source list
 * \param  p_nominal  Pointer to the nominal gains to update
 * \param  cur_mode  Capability sysmode
 *
 * \return unsigned  Samples per timer period.
 */
static void update_dynamic_nominal_gain(AHM_GAIN_BANK *p_nominal,
                                        AHM_SHARED_FINE_GAIN **p_fine_nominal)
{
    AHM_GAIN *p_path;
    int i;

    p_path = &p_nominal->ff;

    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        p_path->fine = p_fine_nominal[i]->gain;
        p_path++;
    }
    return;
}

/**
 * \brief  Update the static nominal gain value
 *
 * Depending on the sysmode, update the nominal gain value.
 *
 * This function is used to keep nominal gains that are not dynamic in sync
 * with the static value
 *
 * \param  p_static  Pointer to static gains
 * \param  p_nominal  Pointer to nominal gains
 *
 * \return unsigned  Samples per timer period.
 */
static void update_static_nominal_gain(AHM_GAIN_BANK *p_static,
                                       AHM_GAIN_BANK *p_nominal)
{
    p_nominal->ff.coarse = p_static->ff.coarse;
    p_nominal->fb.coarse = p_static->fb.coarse;
    p_nominal->ec.coarse = p_static->ec.coarse;
    p_nominal->rx_ffa_mix = p_static->rx_ffa_mix;
    p_nominal->rx_ffb_mix = p_static->rx_ffb_mix;

    return;
}


/**
 * \brief  Update coarse gain for each of the paths (FF,FB, EC)
 *
 * This function is used to keep current gain in sync with static gain
 * \param  cur  Pointer to current gain
 * \param  stat Pointer to static gain

 */
static void update_coarse_gain(AHM_GAIN_BANK *cur, AHM_GAIN_BANK *stat)
{
    cur->ff.coarse  = stat->ff.coarse;
    cur->fb.coarse  = stat->fb.coarse;
    cur->ec.coarse  = stat->ec.coarse;
    cur->rx_ffa_mix.coarse = stat->rx_ffa_mix.coarse;
    cur->rx_ffb_mix.coarse = stat->rx_ffb_mix.coarse;
}

/**
 * \brief Reinitialize the nominal gain with the static gain for each of the paths
 *
 * This function reinitialize the nominal gain with the static gain
 * \param p_ext_data Pointer to AHM operator data
 */
static void reinit_nominal_gain(AHM_OP_DATA *p_ext_data)
{
    AHM_GAIN_BANK *p_stat, *p_stat1;
    AHM_GAIN *p_path, *p_path1;
    p_stat = p_ext_data->p_static_gain;
    p_stat1 = p_ext_data->p_static_gain1;
    int i;
    /* Update nominal gains */
    p_path = &p_stat->ff;
    p_path1 = &p_stat1->ff;
    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        p_ext_data->ahm_fine_nominal[i].gain = (uint8)p_path->fine;
        p_ext_data->ahm_fine_nominal1[i].gain = (uint8)p_path1->fine;
        p_path++;
        p_path1++;
    }
 
    p_path = &p_ext_data->p_nominal_gain->ff;
    p_path1 = &p_ext_data->p_nominal_gain1->ff;

    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        if (p_ext_data->ramp_required[i] || p_ext_data->coarse_gain_changed[i])
        {
            p_ext_data->p_fine_nominal[i] = &p_ext_data->ahm_fine_nominal[i];
            p_ext_data->p_fine_nominal1[i] = &p_ext_data->ahm_fine_nominal1[i];

            p_path->fine = p_ext_data->ahm_fine_nominal[i].gain;
            p_path1->fine = p_ext_data->ahm_fine_nominal1[i].gain;
        }
        p_path++;
        p_path1++;
    }

    update_static_nominal_gain(p_stat, p_ext_data->p_nominal_gain);
    update_static_nominal_gain(p_stat1, p_ext_data->p_nominal_gain1);
}

#ifndef RUNNING_ON_KALSIM
/**
 * \brief  Enable/Disable ANC Zero Crossing detect on FF gain path.
 *
 * \param  enable  Enable (true) or disable (false) ZCD
 * \param  channel ANC Instance channel (Inst0, Inst1 or both)
 *
 * \return void
 */
static void ahm_set_anc_zcd_enable(bool enable, AHM_ANC_INSTANCE channel)
{
    unsigned control_value;

    if (enable)
    {
        control_value = AHM_ANC_CONTROL_ZCD_ENABLE;
    }
    else
    {
        control_value = AHM_ANC_CONTROL_ZCD_DISABLE;
    }

    if ((channel == AHM_ANC_INSTANCE_BOTH_ID) ||
        (channel == AHM_ANC_INSTANCE_DUAL_ID))
    {
        stream_anc_set_anc_control(AHM_ANC_INSTANCE_ANC0_ID, control_value);
        stream_anc_set_anc_control(AHM_ANC_INSTANCE_ANC1_ID, control_value);
    }
    else
    {
        stream_anc_set_anc_control(channel, control_value);
    }
}
#endif

/**
 * \brief  Sent an unsolicited message for an AHM event.
 *
 * \param  op_data      Pointer to operator data
 * \param  p_evt_msg    Pointer to the event message data to end
 *
 * \return - TRUE if successful
 */
static bool ahm_send_event_message(OPERATOR_DATA *op_data,
                                   AHM_EVENT_MSG *p_evt_msg)
{
    unsigned msg_size;
    unsigned *p_msg;
    OPMSG_REPLY_ID msg_id;

    msg_id = OPMSG_REPLY_ID_AHM_EVENT_TRIGGER;
    msg_size = OPMSG_UNSOLICITED_AHM_EVENT_TRIGGER_WORD_SIZE;
    p_evt_msg->ext_op_id = INT_TO_EXT_OPID(op_data->id);
    p_msg = xzpnewn(msg_size, unsigned);
    if (p_msg == NULL)
    {
        L2_DBG_MSG1("OPID: %x, Failed to create AHM message payload", p_evt_msg->ext_op_id);
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
                               p_evt_msg->ext_op_id);
    common_send_unsolicited_message(op_data, (unsigned)msg_id, msg_size,
                                    p_msg);

    pdelete(p_msg);
    return TRUE;
}

/**
 * \brief  Initialize a delta ramp and send a notification if required
 *
 * \param  op_data  Pointer to AHM operator data
 * \param  p_ramp  Pointer to delta ramp for initialization
 * \param  p_cfg  Pointer to delta ramp configuration
 *
 * \return void
 */
static inline void ahm_init_delta_ramp_and_notify(OPERATOR_DATA *op_data,
                                                  AHM_DELTA_RAMP *p_ramp,
                                                  AHM_RAMP_CONFIG *p_cfg)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (ahm_initialize_delta_ramp(p_ramp, p_cfg, &p_ext_data->event_msg))
    {
        ahm_send_event_message(op_data, &p_ext_data->event_msg);
    }

    return;
}

/**
 * \brief  Process a delta ramp and send a notification if required
 *
 * \param  op_data  Pointer to AHM operator data
 * \param  p_ramp  Pointer to delta ramp for initialization
 *
 * \return void
 */
static inline void ahm_proc_delta_ramp_and_notify(OPERATOR_DATA *op_data,
                                                  AHM_DELTA_RAMP *p_ramp,
                                                  uint16 nominal_gain)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (ahm_process_delta_ramp(p_ramp, &p_ext_data->event_msg, nominal_gain))
    {
        ahm_send_event_message(op_data, &p_ext_data->event_msg);
    }

    return;
}

/**
 * \brief  Configure a delta ramp for ramping up
 *
 * \param  cfg  Pointer to ramp configuration data
 * \param  op_data  Pointer to AHM operator data
 *
 * \return void
 */
static void configure_ramp_up(AHM_RAMP_CONFIG *cfg,AHM_OP_DATA *p_ext_data)
{
    AHM_GAIN *p_nominal_path;
    int i;

    cfg[AHM_ANC_FILTER_FF_ID].target = p_ext_data->ff_fine_tgt_gain;
    if(p_ext_data->set_target_flag)
    {
        p_ext_data->set_target_flag = FALSE;
    }

    cfg[AHM_ANC_FILTER_FB_ID].target =p_ext_data->fb_fine_tgt_gain;
    cfg[AHM_ANC_FILTER_EC_ID].target =p_ext_data->p_static_gain->ec.fine;

    p_nominal_path = &p_ext_data->p_nominal_gain->ff;

    for ( i=0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        cfg[i].nominal_gain = p_nominal_path->fine;
        cfg[i].slow_rate    = p_ext_data->slow_rate;
        cfg[i].fast_rate    = p_ext_data->fast_rate;
        p_nominal_path++;
    }

}

/**
 * \brief  Configure a delta ramp for ramping down
 *
 * \param  cfg  Pointer to ramp configuration data
 * \param  op_data  Pointer to AHM operator data
 *
 * \return void
 */
static void configure_ramp_down(AHM_RAMP_CONFIG *cfg, AHM_OP_DATA *p_ext_data,
                                uint16 trigger_mode)
{
    cfg->target = 0;
    cfg-> delay=  0;

    if (trigger_mode == ANC_HW_MANAGER_TRIGGER_SIMILAR)
    {
        cfg->duration = \
            p_ext_data->ahm_cap_params.OFFSET_FAST_RAMP_DOWN_DURATION;
    }
    else if (trigger_mode == ANC_HW_MANAGER_TRIGGER_DIFFERENT)
    {
        cfg->duration = \
            p_ext_data->ahm_cap_params.OFFSET_SLOW_RAMP_DOWN_DURATION;
    }
    cfg->fast_rate    =   p_ext_data->fast_rate;
    cfg->slow_rate    =   p_ext_data->slow_rate;
    cfg->nominal_gain =   p_ext_data->p_nominal_gain->ff.fine;
}

/**
 * \brief  Configure delay and duration for delta ramp for slow mode transition
 *
 * \param  cfg      Pointer to ramp configuration data
 * \param  op_data  Pointer to AHM operator data
 *
 * \return void
 */
static void configure_ramp_slow_delay_duration(AHM_RAMP_CONFIG *cfg,
                                               AHM_OP_DATA *p_ext_data)
{
    for ( int i=0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        cfg[i].delay       =  \
            p_ext_data->ahm_cap_params.OFFSET_SLOW_MODE_DELAY_DURATION;
        cfg[i].duration    =  \
            p_ext_data->ahm_cap_params.OFFSET_SLOW_RAMP_UP_DURATION;
    }
}

/**
 * \brief  Configure delay and duration for delta ramp for fast mode transition
 *
 * \param  cfg      Pointer to ramp configuration data
 * \param  op_data  Pointer to AHM operator data
 *
 * \return void
 */
static void configure_ramp_fast_delay_duration(AHM_RAMP_CONFIG *cfg,
                                               AHM_OP_DATA *p_ext_data )
{
    for ( int i=0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
            cfg[i].delay       =  \
                p_ext_data->ahm_cap_params.OFFSET_FAST_MODE_DELAY_DURATION;
            cfg[i].duration    =  \
                p_ext_data->ahm_cap_params.OFFSET_FAST_RAMP_UP_DURATION;

    }

}

#ifdef RUNNING_ON_KALSIM
static bool ahm_write_gain_generic(OPERATOR_DATA *op_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    if (p_ext_data->config.channel == AHM_ANC_INSTANCE_ANC1_ID)
    {
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain1,
                       p_ext_data->p_prev_gain1,
                       op_data);
    }

    else if (p_ext_data->config.channel == AHM_ANC_INSTANCE_BOTH_ID)
    {
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain,
                       op_data);
    }

    else if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
    {
        p_ext_data->config.channel = AHM_ANC_INSTANCE_ANC0_ID;
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain,
                       op_data);
        p_ext_data->config.channel = AHM_ANC_INSTANCE_ANC1_ID;
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain1,
                       p_ext_data->p_prev_gain1,
                       op_data);
        /* restoring the actual channel */
        p_ext_data->config.channel = AHM_ANC_INSTANCE_DUAL_ID;
    }
    else
    {
        /* default instance as 0 for backward compatinbility*/
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain,
                       op_data);
    }

    return TRUE;
}
#else
static bool ahm_write_gain_generic(OPERATOR_DATA *op_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (p_ext_data->config.channel == AHM_ANC_INSTANCE_ANC1_ID)
    {
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain1,
                       p_ext_data->p_prev_gain1);
        stream_anc_update_background_gains(p_ext_data->config.channel);
    }

    else if (p_ext_data->config.channel == AHM_ANC_INSTANCE_BOTH_ID)
    {
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain);
        stream_anc_update_background_gains(AHM_ANC_INSTANCE_ANC0_ID);
        stream_anc_update_background_gains(AHM_ANC_INSTANCE_ANC1_ID);
    }

    else if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
    { 
        p_ext_data->config.channel = AHM_ANC_INSTANCE_ANC0_ID;
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain);
        p_ext_data->config.channel = AHM_ANC_INSTANCE_ANC1_ID;
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain1,
                       p_ext_data->p_prev_gain1);
        /* restoring the actual channel */
        p_ext_data->config.channel = AHM_ANC_INSTANCE_DUAL_ID;
        
        stream_anc_update_background_gains(AHM_ANC_INSTANCE_ANC0_ID);
        stream_anc_update_background_gains(AHM_ANC_INSTANCE_ANC1_ID);
    }
    else
    {
        /* default instance as 0 for backward compatinbility*/
        ahm_write_gain(&p_ext_data->config,
                       p_ext_data->p_cur_gain,
                       p_ext_data->p_prev_gain);
        stream_anc_update_background_gains(p_ext_data->config.channel);
    }
    return TRUE;
}
#endif
/****************************************************************************
Capability API Handlers
*/

bool ahm_create(OPERATOR_DATA *op_data, void *message_data,
                 unsigned *response_id, void **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *p_default_params;     /* Pointer to default params */
    unsigned *p_cap_params;         /* Pointer to capability params */
    CPS_PARAM_DEF *p_param_def;     /* Pointer to parameter definition */
    int i;
    AHM_GAIN *p_path, *p_nom_path;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    L5_DBG_MSG2("OPID: %x, AHM Create: p_ext_data at %p",ext_op_id, p_ext_data);
    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = ANC_HW_MANAGER_CAP_ID;

    /* Multi-channel create */
    if(!aud_cur_create(op_data, AHM_MAX_SOURCES, AHM_MAX_SINKS))
    {
        return FALSE;
    }
    aud_cur_set_callbacks(op_data, ahm_start_hook, ahm_stop_hook, NULL, NULL, NULL);
    aud_cur_set_flags(op_data,
                      AHM_SUPPORTS_IN_PLACE,
                      AHM_SUPPORTS_METADATA,
                      AHM_DYNAMIC_BUFFERS);
    aud_cur_set_min_terminal_masks(op_data,
                                   AHM_SOURCE_VALID_MASK,
                                   AHM_SINK_VALID_MASK);

    /* Initialize parameters */
    p_default_params = (unsigned*) ANC_HW_MANAGER_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->ahm_cap_params;
    p_param_def = aud_cur_get_cps(op_data);
    if(!cpsInitParameters(p_param_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(ANC_HW_MANAGER_PARAMETERS)))
    {
        base_op_change_response_status(resp_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    p_ext_data->sample_rate = (unsigned)stream_if_get_system_sampling_rate();

    /* Initialize system mode. */
    p_ext_data->cur_mode = ANC_HW_MANAGER_SYSMODE_FULL;
    p_ext_data->host_mode = ANC_HW_MANAGER_SYSMODE_FULL;
    p_ext_data->qact_mode = ANC_HW_MANAGER_SYSMODE_FULL;

    p_ext_data->config.channel = AHM_ANC_INSTANCE_ANC0_ID;
    /* Default to hybrid: ff path is FFB, fb path is FFA */
    p_ext_data->config.ff_path = AHM_ANC_PATH_FFB_ID;
    p_ext_data->config.fb_path = AHM_ANC_PATH_FFA_ID;
    p_ext_data->anc_clock_check_value = AHM_HYBRID_ENABLE;
    /* Default to clock check was OK */
    p_ext_data->clock_status = TRUE;

    p_ext_data->p_cur_gain = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_cur_gain == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate current gain", ext_op_id);
    }
    p_ext_data->p_cur_gain1 = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_cur_gain1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate current gain1", ext_op_id);
    }
    p_ext_data->p_prev_gain = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_prev_gain == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate previous gain", ext_op_id);
    }
    ahm_initialize_prev_gain(p_ext_data->p_prev_gain);
    p_ext_data->p_prev_gain1 = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_prev_gain1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate previous gain1", ext_op_id);
    }
    ahm_initialize_prev_gain(p_ext_data->p_prev_gain1);
    p_ext_data->p_static_gain = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_static_gain == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate static gain", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_static_gain1 = xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_static_gain1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate inst1 static gain", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_nominal_gain = \
        xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_nominal_gain == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate nominal gain", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_nominal_gain1 = \
        xzppnew(AHM_GAIN_BANK, MALLOC_PREFERENCE_SHARED);
    if (p_ext_data->p_nominal_gain1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: failed to allocate nominal gain1", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_iir_filter_inst1 = \
        xzppnew(AHM_IIR_FILTER_BANK, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_iir_filter_inst1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: Failed to allocate IIR Filter1", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_iir_filter_inst2 = \
        xzppnew(AHM_IIR_FILTER_BANK, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_iir_filter_inst2 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM:Failed to allocate IIR Filter2", ext_op_id);
        return FALSE;

    }
    /* previous filter place holder */
    p_ext_data->p_prev_iir_filter_inst1 = \
        xzppnew(AHM_IIR_FILTER_BANK, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_prev_iir_filter_inst1 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM: Failed to allocate IIR Filter1 Place-holder", ext_op_id);
        return FALSE;
    }
    p_ext_data->p_prev_iir_filter_inst2 = \
        xzppnew(AHM_IIR_FILTER_BANK, MALLOC_PREFERENCE_NONE);
    if (p_ext_data->p_prev_iir_filter_inst2 == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM:Failed to allocate IIR Filter2 Place-holder", ext_op_id);
        return FALSE;
    }

    p_path = &p_ext_data->p_cur_gain->ff;
    p_nom_path = &p_ext_data->p_nominal_gain->ff;

    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        /* Create the fine gain ramps */
        p_ext_data->p_fine_delta_ramp[i] = \
            ahm_list_fine_gain_add(&p_ext_data->p_fine_delta_head[i], ext_op_id);
        if (p_ext_data->p_fine_delta_ramp[i] == NULL)
        {
            return FALSE;
        }
        p_ext_data->p_fine_delta_ramp[i]->gain_delta = AHM_DELTA_NOMINAL;

        /* Point the delta fine ramps to the right gains */
        p_ext_data->fine_ramp[i].p_cur = &p_path->fine;
        p_ext_data->fine_ramp[i].p_gain = p_ext_data->p_fine_delta_ramp[i];

        /* Initialize the nominal gain pointers */
        p_ext_data->p_fine_nominal[i] = &p_ext_data->ahm_fine_nominal[i];
        /* Use same static gains (i.e ahm nominal nominal) for both instances */
        p_ext_data->p_fine_nominal1[i] = &p_ext_data->ahm_fine_nominal1[i];

        p_path++;
        p_nom_path++;
    }

    p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID].path = AHM_EVENT_ID_FF_RAMP;
    p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID].path = AHM_EVENT_ID_FB_RAMP;
    p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID].path = AHM_EVENT_ID_EC_RAMP;

    p_ext_data->timer_period = AHM_DEF_TIMER_PERIOD_US;
    p_ext_data->timer_decimation = AHM_DEF_TIMER_DECIMATION;
    p_ext_data->samples_per_period = \
        ahm_calc_samples_per_period(p_ext_data->sample_rate,
                                    p_ext_data->timer_period,
                                    p_ext_data->timer_decimation);
    p_ext_data->fast_rate = AHM_DEF_FAST_RATE;
    p_ext_data->slow_rate = AHM_DEF_SLOW_RATE;



    /* Turn on double banking for ANC HW Filter coefficients for both
       ANC instances */
    #ifndef RUNNING_ON_KALSIM
       stream_anc_select_active_iir_coeffs(STREAM_ANC_INSTANCE_ANC01_MASK,
                                           STREAM_ANC_BANK_BACKGROUND);
    #endif

    /* Initialize AHM event message */
    p_ext_data->event_msg.ext_op_id = ext_op_id;
    return TRUE;
}

bool ahm_destroy(OPERATOR_DATA *op_data, void *message_data,
                  unsigned *response_id, void **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    int i;

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }
    aud_cur_destroy(op_data);

    pdelete(p_ext_data->p_cur_gain);
    pdelete(p_ext_data->p_cur_gain1);
    pdelete(p_ext_data->p_prev_gain);
    pdelete(p_ext_data->p_prev_gain1);
    pdelete(p_ext_data->p_static_gain);
    pdelete(p_ext_data->p_static_gain1);
    pdelete(p_ext_data->p_nominal_gain);
    pdelete(p_ext_data->p_nominal_gain1);
    pdelete(p_ext_data->p_iir_filter_inst1);
    pdelete(p_ext_data->p_iir_filter_inst2);
    pdelete(p_ext_data->p_prev_iir_filter_inst1);
    pdelete(p_ext_data->p_prev_iir_filter_inst2);

    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        ahm_list_fine_gain_remove(&p_ext_data->p_fine_delta_head[i],
                                  p_ext_data->p_fine_delta_ramp[i],
                                  INT_TO_EXT_OPID(op_data->id));
        ahm_list_destroy(&p_ext_data->p_fine_delta_head[i]);
    }

    return TRUE;
}

bool ahm_start_hook(OPERATOR_DATA *op_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* Start a casual timer task if there are no connections */
    if (p_class_data->sinks.connected == 0 &&
        p_class_data->sources.connected == 0 &&
        p_ext_data->timer_period > 0)
    {
        L4_DBG_MSG1("OPID: %x, AHM starting timer", INT_TO_EXT_OPID(op_data->id));
        p_ext_data->timer_id=timer_schedule_bg_event_in(p_ext_data->timer_period,
                                                        ahm_timer_cb,
                                                        (void*)op_data);
    }


    return TRUE;
}

bool ahm_stop_hook(OPERATOR_DATA *op_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* Cancel casual timer */
    L4_DBG_MSG1("OPID: %x, AHM stopping timer", INT_TO_EXT_OPID(op_data->id));
    timer_cancel_event(p_ext_data->timer_id);

    return TRUE;
}

/****************************************************************************
Opmsg handlers
*/


/**
 * \brief  Set an AHM mode
 *
 * \param  op_data          Pointer to operator data
 * \param  p_ext_data       Pointer to the AHM extra_op_data
 * \param  ctrl_value       Mode from the opmsg_set_control payload
 *
 * \return  None
 *
 */
static void ahm_set_mode(OPERATOR_DATA *op_data,
                         AHM_OP_DATA *p_ext_data,
                         unsigned ctrl_value)
{
    /* FF, FB fine gain ramp: duration, delay, target, ramp pointers */
    unsigned fb_dur, ff_dur, fb_dly, ff_dly;
    unsigned *p_param_value;
    uint16 ff_tgt, fb_tgt;

    AHM_GAIN_BANK *p_cur, *p_cur1, *p_stat, *p_stat1;
    ANC_HW_MANAGER_PARAMETERS *p_params;
    AHM_GAIN *p_path, *p_path1, *p_stat_path;
    int i;

    AHM_RAMP_CONFIG cfg[AHM_NUM_DYNAMIC_FILTERS];
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);

    p_params = &p_ext_data->ahm_cap_params;
    if (p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_QUIET)
    {
        fb_dur = p_params->OFFSET_QUIET_FB_RAMP_DURATION;
        ff_dur = p_params->OFFSET_QUIET_FF_RAMP_DURATION;
    }
    else if(p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_WINDY)
    {
        fb_dur = p_params->OFFSET_WINDY_FB_RAMP_DURATION;
        ff_dur = p_params->OFFSET_WINDY_FF_RAMP_DURATION;
    }
    else
    {
        fb_dur = p_params->OFFSET_FB_RAMP_DURATION;
        ff_dur = p_params->OFFSET_FF_RAMP_DURATION;
    }
    fb_dly = p_ext_data->ahm_cap_params.OFFSET_FB_RAMP_DELAY_TIMER;

    p_cur = p_ext_data->p_cur_gain;
    p_cur1 = p_ext_data->p_cur_gain1;

    p_stat = p_ext_data->p_static_gain;
    p_stat1 = p_ext_data->p_static_gain1;
    ff_dly = 0;
    ff_tgt = 0;
    fb_tgt = 0;

    /* Update nominal gains */
    p_path = &p_stat->ff;
    p_path1 = &p_stat1->ff;
    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        p_ext_data->ahm_fine_nominal[i].gain = (uint8)p_path->fine;
        p_ext_data->ahm_fine_nominal1[i].gain = (uint8)p_path1->fine;
        p_path++;
        p_path1++;
    }

    update_dynamic_nominal_gain(p_ext_data->p_nominal_gain,
                                p_ext_data->p_fine_nominal);
    update_dynamic_nominal_gain(p_ext_data->p_nominal_gain1,
                                p_ext_data->p_fine_nominal1);

    update_static_nominal_gain(p_stat, p_ext_data->p_nominal_gain);
    update_static_nominal_gain(p_stat1, p_ext_data->p_nominal_gain1);

    switch (ctrl_value)
    {
        case ANC_HW_MANAGER_SYSMODE_STANDBY:
            /* No effect on gains */
            return;
        case ANC_HW_MANAGER_SYSMODE_MUTE_ANC:
            /* Ramp FF and FB fine gains to 0  */
            ff_tgt = 0;
            ff_dur = p_ext_data->ahm_cap_params.OFFSET_MUTE_RAMP_DURATION;
            fb_tgt = 0;
            ff_dly = 0;
            fb_dly = 0;
            fb_dur = p_ext_data->ahm_cap_params.OFFSET_MUTE_RAMP_DURATION;
            break;
        case ANC_HW_MANAGER_SYSMODE_FULL:
            /* Ramp FF and FB fine gains to target gain values. Set other gains
             * to static values.
             */

            p_cur->rx_ffa_mix = p_stat->rx_ffa_mix;
            p_cur->rx_ffb_mix = p_stat->rx_ffb_mix;
            p_cur->ff.coarse = p_stat->ff.coarse;
            p_cur->fb.coarse = p_stat->fb.coarse;
            p_cur->ec.coarse = p_stat->ec.coarse;

            p_cur1->rx_ffa_mix = p_stat1->rx_ffa_mix;
            p_cur1->rx_ffb_mix = p_stat1->rx_ffb_mix;
            p_cur1->ff.coarse = p_stat1->ff.coarse;
            p_cur1->fb.coarse = p_stat1->fb.coarse;
            p_cur1->ec.coarse = p_stat1->ec.coarse;

            ff_dly = p_ext_data->ahm_cap_params.OFFSET_START_MODE_DELAY_DURATION;

            if(p_ext_data->set_target_flag)
            {
                ff_tgt = p_ext_data->ff_fine_tgt_gain;
                p_ext_data->set_target_flag = FALSE;
            }
            else
            {
                ff_tgt = p_ext_data->p_nominal_gain->ff.fine;
            }

            fb_tgt = p_ext_data->fb_fine_tgt_gain;
            break;
        case ANC_HW_MANAGER_SYSMODE_STATIC:
            /* Ramp FF and FB fine gains to static values. Set other gains to
             * static values.
             */

            p_cur->rx_ffa_mix = p_stat->rx_ffa_mix;
            p_cur->rx_ffb_mix = p_stat->rx_ffb_mix;
            p_cur->ff.coarse = p_stat->ff.coarse;
            p_cur->fb.coarse = p_stat->fb.coarse;
            p_cur->ec.coarse = p_stat->ec.coarse;

            p_cur1->rx_ffa_mix = p_stat1->rx_ffa_mix;
            p_cur1->rx_ffb_mix = p_stat1->rx_ffb_mix;
            p_cur1->ff.coarse = p_stat1->ff.coarse;
            p_cur1->fb.coarse = p_stat1->fb.coarse;
            p_cur1->ec.coarse = p_stat1->ec.coarse;

            ff_dly = p_ext_data->ahm_cap_params.OFFSET_START_MODE_DELAY_DURATION;
            ff_tgt = p_stat->ff.fine;
            fb_tgt = p_stat->fb.fine;
            break;
        case ANC_HW_MANAGER_SYSMODE_QUIET:
            /* Ramp FF and FB fine gains to target values */

            p_cur->rx_ffa_mix = p_stat->rx_ffa_mix;
            p_cur->rx_ffb_mix = p_stat->rx_ffb_mix;
            p_cur->ff.coarse = p_stat->ff.coarse;
            p_cur->fb.coarse = p_stat->fb.coarse;
            p_cur->ec.coarse = p_stat->ec.coarse;

            p_cur1->rx_ffa_mix = p_stat1->rx_ffa_mix;
            p_cur1->rx_ffb_mix = p_stat1->rx_ffb_mix;
            p_cur1->ff.coarse = p_stat1->ff.coarse;
            p_cur1->fb.coarse = p_stat1->fb.coarse;
            p_cur1->ec.coarse = p_stat1->ec.coarse;


            ff_dly = 0;

            fb_dur = p_params->OFFSET_QUIET_FB_RAMP_DURATION;
            ff_dur = p_params->OFFSET_QUIET_FF_RAMP_DURATION;

            p_param_value = &p_params->OFFSET_QUIET_MODE_FF_FINE_GAIN;
            ff_tgt = interpret_fine_gain(p_param_value,
                                         p_stat->ff.fine,
                                         p_cur->ff.fine,
                                         ext_op_id);
            p_param_value = &p_params->OFFSET_QUIET_MODE_FB_FINE_GAIN;
            fb_tgt = interpret_fine_gain(p_param_value,
                                         p_stat->fb.fine,
                                         p_cur->fb.fine,
                                         ext_op_id);
            fb_dly = 0;
            break;
        case ANC_HW_MANAGER_SYSMODE_WINDY:
            /* Ramp FF and FB fine gains to target values */

            p_cur->rx_ffa_mix = p_stat->rx_ffa_mix;
            p_cur->rx_ffb_mix = p_stat->rx_ffb_mix;
            p_cur->ff.coarse = p_stat->ff.coarse;
            p_cur->fb.coarse = p_stat->fb.coarse;
            p_cur->ec.coarse = p_stat->ec.coarse;

            p_cur1->rx_ffa_mix = p_stat1->rx_ffa_mix;
            p_cur1->rx_ffb_mix = p_stat1->rx_ffb_mix;
            p_cur1->ff.coarse = p_stat1->ff.coarse;
            p_cur1->fb.coarse = p_stat1->fb.coarse;
            p_cur1->ec.coarse = p_stat1->ec.coarse;

            fb_dur = p_params->OFFSET_WINDY_FB_RAMP_DURATION;
            ff_dur = p_params->OFFSET_WINDY_FF_RAMP_DURATION;

            p_param_value = &p_params->OFFSET_WINDY_MODE_FF_FINE_GAIN;
            ff_tgt = interpret_fine_gain(p_param_value,
                                         p_stat->ff.fine,
                                         p_cur->ff.fine,
                                         ext_op_id);
            p_param_value = &p_params->OFFSET_WINDY_MODE_FB_FINE_GAIN;
            fb_tgt = interpret_fine_gain(p_param_value,
                                         p_stat->fb.fine,
                                         p_cur->fb.fine,
                                         ext_op_id);
            fb_dly = 0;
            ff_dly = 0;
            break;
        default:
            break;
    }
    /* Dummy Print to get past compiler warnings */
    L5_DBG_MSG2("OPID: %x, FF ramp delayed by %p", ext_op_id, ff_dly);
    /* Store configurations */
    cfg[AHM_ANC_FILTER_FF_ID].target = ff_tgt;
    cfg[AHM_ANC_FILTER_FF_ID].delay = ff_dly;
    cfg[AHM_ANC_FILTER_FF_ID].duration = ff_dur;


    cfg[AHM_ANC_FILTER_FB_ID].target = fb_tgt;
    cfg[AHM_ANC_FILTER_FB_ID].delay = fb_dly;
    cfg[AHM_ANC_FILTER_FB_ID].duration = fb_dur;


    cfg[AHM_ANC_FILTER_EC_ID].target = p_ext_data->p_static_gain->ec.fine;
    cfg[AHM_ANC_FILTER_EC_ID].delay = 0;
    cfg[AHM_ANC_FILTER_EC_ID].duration = 0;



     for (i=0; i < AHM_NUM_DYNAMIC_FILTERS;i++ )
    {
        cfg[i].slow_rate = p_ext_data->slow_rate;
        cfg[i].fast_rate = p_ext_data->fast_rate;
    }


    p_path = &p_ext_data->p_nominal_gain->ff;
    p_stat_path = &p_ext_data->p_static_gain->ff;

    /* Setup ramp nominal gain values and initialize ramps */
    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        if (p_path->fine == 0)
        {
            cfg[i].nominal_gain = p_stat_path->fine;
        }
        else
        {
            cfg[i].nominal_gain = p_path->fine;
        }

        ahm_init_delta_ramp_and_notify(op_data,
                                       &p_ext_data->fine_ramp[i],
                                       &cfg[i]);

        p_path++;
        p_stat_path++;
    }

    /* Update nominal gain source. Standby is included because this is what
     * is sent during a disable override from QACT.
     */
    if (ctrl_value == ANC_HW_MANAGER_SYSMODE_FULL ||
        ctrl_value == ANC_HW_MANAGER_SYSMODE_STANDBY)
    {
        for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
        {
            if (p_ext_data->ahm_nominal_client[i])
            {
                p_ext_data->p_fine_nominal[i] = \
                    &p_ext_data->client_fine_nominal[i];
            }
            else
            {
                p_ext_data->p_fine_nominal[i] = \
                    &p_ext_data->ahm_fine_nominal[i];
            }

            if (p_ext_data->ahm_nominal_client1[i])
            {
                p_ext_data->p_fine_nominal1[i] = \
                    &p_ext_data->client_fine_nominal1[i];
            }
            else
            {
                p_ext_data->p_fine_nominal1[i] = \
                    &p_ext_data->ahm_fine_nominal1[i];
            }
        }
    }
    else
    {
        for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
        {
            p_ext_data->p_fine_nominal[i] = &p_ext_data->ahm_fine_nominal[i];
            p_ext_data->p_fine_nominal1[i] = &p_ext_data->ahm_fine_nominal1[i];
        }
    }

    return;
}

bool ahm_opmsg_set_control(OPERATOR_DATA *op_data, void *message_data,
                            unsigned *resp_length,
                            OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned i;
    unsigned num_controls;

    OPMSG_RESULT_STATES result;
    CPS_CONTROL_SOURCE ctrl_src;
    unsigned ctrl_value, ctrl_id;

    unsigned sel_override, sel_bank, sel_type;
    AHM_GAIN *p_sel_bank;
    uint16 *p_override;
    AHM_RAMP_CONFIG cfg;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    if(!cps_control_setup(message_data, resp_length, resp_data, &num_controls))
    {
       return FALSE;
    }

    DEBUG_AHM("OPID: %x, ahm_opmsg_set_control Fn entry", ext_op_id);


    /* Iterate through the control messages looking for mode and gain override
     * messages */
    result = OPMSG_RESULT_STATES_NORMAL_STATE;
    for (i=0; i<num_controls; i++)
    {
        ctrl_id = cps_control_get(message_data, i, &ctrl_value, &ctrl_src);

        /* Mode override */
        if (ctrl_id == OPMSG_CONTROL_MODE_ID)
        {
            ctrl_value &= AHM_SYSMODE_MASK;
            /* Check for valid mode */
            if (ctrl_value >= ANC_HW_MANAGER_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            /* Ensure any gain updates on a mode change occur */
            ahm_initialize_prev_gain(p_ext_data->p_prev_gain);
            ahm_initialize_prev_gain(p_ext_data->p_prev_gain1);
            ahm_set_mode(op_data, p_ext_data, ctrl_value);

            /* Determine control mode source and set override flags for mode */
            if (ctrl_src == CPS_SOURCE_HOST)
            {
                p_ext_data->host_mode = ctrl_value;
            }
            else
            {
                p_ext_data->qact_mode = ctrl_value;
                /* Set or clear the QACT override flag.
                * &= is used to preserve the state of the gain bits in the
                * override word.
                */
                if (ctrl_src == CPS_SOURCE_OBPM_ENABLE)
                {
                    p_ext_data->ovr_control |= AHM_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= AHM_OVERRIDE_MODE_MASK;
                }
            }

            continue;
        }

        /* In/Out of Ear control */
        else if (ctrl_id == ANC_HW_MANAGER_CONSTANT_IN_OUT_EAR_CTRL)
        {
            ctrl_value &= 0x01;
            p_ext_data->in_out_status = ctrl_value;

            /* No override flags indicated for in/out of ear */
            continue;
        }
        /* Channel control */
        else if (ctrl_id == ANC_HW_MANAGER_CONSTANT_CHANNEL_CTRL)
        {
            ctrl_value &= 0x7;
            p_ext_data->config.channel = (AHM_ANC_INSTANCE)ctrl_value;

            /* Copy instance0's current and previous gains to instance1's */
            if(ctrl_value == AHM_ANC_INSTANCE_DUAL_ID)
            {
                p_ext_data->p_cur_gain1->ff = p_ext_data->p_cur_gain->ff;
                p_ext_data->p_cur_gain1->fb = p_ext_data->p_cur_gain->fb;
                p_ext_data->p_cur_gain1->ec = p_ext_data->p_cur_gain->ec;
                p_ext_data->p_cur_gain1->rx_ffa_mix = p_ext_data->p_cur_gain->rx_ffa_mix;
                p_ext_data->p_cur_gain1->rx_ffb_mix = p_ext_data->p_cur_gain->rx_ffb_mix;

                p_ext_data->p_prev_gain1->ff = p_ext_data->p_prev_gain->ff;
                p_ext_data->p_prev_gain1->fb = p_ext_data->p_prev_gain->fb;
                p_ext_data->p_prev_gain1->ec = p_ext_data->p_prev_gain->ec;
                p_ext_data->p_prev_gain1->rx_ffa_mix = p_ext_data->p_prev_gain->rx_ffa_mix;
                p_ext_data->p_prev_gain1->rx_ffb_mix = p_ext_data->p_prev_gain->rx_ffb_mix;
            }
            /* No override flags indicated for channel */
            continue;
        }

        /* Feedforward control */
        else if (ctrl_id == ANC_HW_MANAGER_CONSTANT_FEEDFORWARD_CTRL)
        {
            ctrl_value &= 0x1;
            if (ctrl_value == 0)
            {
                /* hybrid */
                p_ext_data->config.ff_path = AHM_ANC_PATH_FFB_ID;
                p_ext_data->config.fb_path = AHM_ANC_PATH_FFA_ID;
                p_ext_data->anc_clock_check_value = AHM_HYBRID_ENABLE;
                p_ext_data->mode = AHM_HYBRID_ENABLE;
            }
            else
            {
                /* feedforward only */
                p_ext_data->config.ff_path = AHM_ANC_PATH_FFA_ID;
                p_ext_data->config.fb_path = AHM_ANC_PATH_NONE_ID;
                p_ext_data->anc_clock_check_value = AHM_FEEDFORWARD_ENABLE;
                p_ext_data->mode = AHM_FEEDFORWARD_ENABLE;
            }
            L4_DBG_MSG3("OPID: %x, AHM feedforward override: %d - %d",
                        ext_op_id, p_ext_data->config.ff_path,
                        p_ext_data->config.fb_path);

            /* No override flags indicated for feedforward */
            continue;
        }

        /* Ambient mode control */
        else if (ctrl_id == ANC_HW_MANAGER_CONSTANT_AMBIENT_CTRL)
        {
            ctrl_value &= 0x1;
            if (ctrl_value == 0)
            {
                /* Adaptive ANC mode */
                p_ext_data->aamb_mode = ANC_HW_MANAGER_AMBIENT_CTRL_AANC;
            }
            else
            {
                /* Adaptive ambient mode */
                p_ext_data->aamb_mode = ANC_HW_MANAGER_AMBIENT_CTRL_AAMB;
            }
            L4_DBG_MSG2("OPID: %x, AHM adaptive ambient mode: %d ",
                        ext_op_id, p_ext_data->aamb_mode);

            continue;
        }
        else if (ctrl_id == ANC_HW_MANAGER_CONSTANT_TRIGGER_CTRL)
        {

            ctrl_value &= 0x3;
            switch(ctrl_value)
            {
                case ANC_HW_MANAGER_TRIGGER_START:
                    DEBUG_AHM("OPID: %x, Enabling ANC, No Mode transition ", ext_op_id);
                    p_ext_data->trigger_mode =AHM_TRIGGER_START;
                    break;

                case ANC_HW_MANAGER_TRIGGER_SIMILAR:
                    DEBUG_AHM("OPID: %x, Similar ANC Mode Transition", ext_op_id);
                    p_ext_data->trigger_mode = AHM_TRIGGER_SIMILAR;
                    /* Ramp down gains in relevant paths to zero and no delay if required */
                    configure_ramp_down(&cfg, p_ext_data, ANC_HW_MANAGER_TRIGGER_SIMILAR);

                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down in FF path for similar ANC mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                                       &cfg);
                    }
                    cfg.nominal_gain =p_ext_data->p_nominal_gain->fb.fine;
                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down  in FB path for similar ANC mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID],
                                                       &cfg);
                    }
                    cfg.nominal_gain =p_ext_data->p_nominal_gain->ec.fine;

                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down in EC path for similar ANC Mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID],
                                                       &cfg);
                    }
                    p_ext_data->ramp_status = AHM_RAMP_DOWN;
                    p_ext_data->cur_mode = ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION;
                    p_ext_data->host_mode = ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION;
                    break;

                case ANC_HW_MANAGER_TRIGGER_DIFFERENT:
                    p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID].state = AHM_RAMP_IDLE;
                    p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID].state = AHM_RAMP_IDLE;
                    p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID].state = AHM_RAMP_IDLE;

                    DEBUG_AHM("OPID: %x, Different ANC mode transition", ext_op_id);
                    p_ext_data->trigger_mode = AHM_TRIGGER_DIFFERENT;
                    aud_cur_set_reinit(op_data, FALSE);
                    /* Ramp down gains in relevant paths to zero and no delay if required */
                    configure_ramp_down(&cfg, p_ext_data, ANC_HW_MANAGER_TRIGGER_DIFFERENT );
                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down in FF path for Different ANC mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                                       &cfg);
                    }
                    cfg.nominal_gain =p_ext_data->p_nominal_gain->fb.fine;
                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down in FF path for Different ANC mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID],
                                                       &cfg);
                    }

                    cfg.nominal_gain =p_ext_data->p_nominal_gain->ec.fine;
                    if (p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID])
                    {
                        DEBUG_AHM("OPID: %x, Initialising delta ramp for ramp down in FF path for Different ANC mode!",
                                  ext_op_id);
                        ahm_init_delta_ramp_and_notify(op_data,
                                                       &p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID],
                                                       &cfg);
                    }

                    p_ext_data->ramp_status = AHM_RAMP_DOWN;
                    p_ext_data->cur_mode = ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION;
                    p_ext_data->host_mode = ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION;
                    DEBUG_AHM("OPID: %x, Filter Transition Mode Set. Ramp Down to start",
                              ext_op_id);
                    break;
              }

            continue;

        }
        else if (ctrl_id >= ANC_HW_MANAGER_CONSTANT_FF_COARSE_GAIN_CTRL &&
                  ctrl_id <= ANC_HW_MANAGER_CONSTANT_RX_FFB_MIX_FINE_GAIN_CTRL)
        {
            ahm_initialize_prev_gain(p_ext_data->p_prev_gain);

            cfg.delay = 0;
            cfg.duration = p_ext_data->ahm_cap_params.OFFSET_OVERRIDE_RAMP_DURATION;
            cfg.target = (uint16)ctrl_value;
            cfg.slow_rate = p_ext_data->slow_rate;
            cfg.fast_rate = p_ext_data->fast_rate;

            switch (ctrl_id)
            {
                /* Gain override on fine gains will ramp to the new value */
                case ANC_HW_MANAGER_CONSTANT_FF_FINE_GAIN_CTRL:
                    cfg.nominal_gain = (uint16)ctrl_value;
                    ahm_init_delta_ramp_and_notify(op_data,
                                                   &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                                   &cfg);
                break;
                case ANC_HW_MANAGER_CONSTANT_FB_FINE_GAIN_CTRL:
                    cfg.nominal_gain = (uint16)ctrl_value;
                    ahm_init_delta_ramp_and_notify(op_data,
                                                   &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID],
                                                   &cfg);
                break;
                default:
                    /* Zero-index the selected gain ID */
                    sel_override = ctrl_id - ANC_HW_MANAGER_CONSTANT_FF_COARSE_GAIN_CTRL;

                    /* Gain bank is value divided by 2 */
                    sel_bank = sel_override >> 1;
                    /* Gain type is value mod 2 */
                    sel_type = sel_override % 2;
                    /* Update pointer value */
                    p_sel_bank = (AHM_GAIN*)p_ext_data->p_cur_gain;
                    p_sel_bank += sel_bank;
                    p_override = (uint16*)p_sel_bank + (uint16)sel_type;
                    *p_override = (uint16)ctrl_value;
            }
            switch (ctrl_id)
            {
                case ANC_HW_MANAGER_CONSTANT_FF_FINE_GAIN_CTRL:
                    p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_FF_ID].gain = \
                        (uint8)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_FB_FINE_GAIN_CTRL:
                    p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_FB_ID].gain = \
                        (uint8)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_EC_FINE_GAIN_CTRL:
                    p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_EC_ID].gain = \
                        (uint8)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_FF_COARSE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->ff.coarse = (int16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_FB_COARSE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->fb.coarse = (int16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_EC_COARSE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->ec.coarse = (int16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_RX_FFA_MIX_FINE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->rx_ffa_mix.fine = (uint16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_RX_FFA_MIX_COARSE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->rx_ffa_mix.coarse = (int16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_RX_FFB_MIX_FINE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->rx_ffb_mix.fine = (uint16)ctrl_value;
                    break;
                case ANC_HW_MANAGER_CONSTANT_RX_FFB_MIX_COARSE_GAIN_CTRL:
                    p_ext_data->p_nominal_gain->rx_ffb_mix.coarse = (int16)ctrl_value;
                    break;
                default:
                    break; /* Shouldn't get here due to if filter above */
            }

            ahm_write_gain_generic(op_data);
            continue;
        }

        result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
    }

    /* Set current operating mode based on override */
    if (p_ext_data->ovr_control & AHM_CONTROL_MODE_OVERRIDE)
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

bool ahm_opmsg_get_status(OPERATOR_DATA *op_data, void *message_data,
                          unsigned *resp_length,
                          OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    int i;
    unsigned *resp;
    ANC_HW_MANAGER_STATISTICS stats;
    AHM_GAIN_BANK *p_gain;
    ParamType *pparam;

    if(!common_obpm_status_helper(message_data, resp_length, resp_data,
                                  sizeof(ANC_HW_MANAGER_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp == NULL)
    {
        return FALSE;
    }

    stats.OFFSET_CUR_MODE = p_ext_data->cur_mode;
    stats.OFFSET_OVR_CONTROL = p_ext_data->ovr_control;
    stats.OFFSET_IN_OUT_EAR_CTRL = p_ext_data->in_out_status;
    stats.OFFSET_CHANNEL = p_ext_data->config.channel;
    stats.OFFSET_FEEDFORWARD_PATH = p_ext_data->config.ff_path;
    stats.OFFSET_FLAGS = p_ext_data->clock_status;
    stats.OFFSET_AMBIENT_CTRL = p_ext_data->aamb_mode;

    p_gain = p_ext_data->p_cur_gain;
    stats.OFFSET_FF_COARSE_GAIN_CTRL = p_gain->ff.coarse;
    stats.OFFSET_FF_FINE_GAIN_CTRL = p_gain->ff.fine;
    stats.OFFSET_FF_GAIN_DB = ahm_calc_gain_db(p_gain->ff.fine,
                                               p_gain->ff.coarse);
    stats.OFFSET_FB_COARSE_GAIN_CTRL = p_gain->fb.coarse;
    stats.OFFSET_FB_FINE_GAIN_CTRL = p_gain->fb.fine;
    stats.OFFSET_FB_GAIN_DB = ahm_calc_gain_db(p_gain->fb.fine,
                                               p_gain->fb.coarse);
    stats.OFFSET_EC_COARSE_GAIN_CTRL = p_gain->ec.coarse;
    stats.OFFSET_EC_FINE_GAIN_CTRL = p_gain->ec.fine;
    stats.OFFSET_EC_GAIN_DB = ahm_calc_gain_db(p_gain->ec.fine,
                                               p_gain->ec.coarse);
    stats.OFFSET_RX_FFA_MIX_COARSE_GAIN_CTRL = p_gain->rx_ffa_mix.coarse;
    stats.OFFSET_RX_FFA_MIX_FINE_GAIN_CTRL = p_gain->rx_ffa_mix.fine;
    stats.OFFSET_RX_FFA_MIX_GAIN_DB = ahm_calc_gain_db(
        p_gain->rx_ffa_mix.fine,
        p_gain->rx_ffa_mix.coarse);
    stats.OFFSET_RX_FFB_MIX_COARSE_GAIN_CTRL = p_gain->rx_ffb_mix.coarse;
    stats.OFFSET_RX_FFB_MIX_FINE_GAIN_CTRL = p_gain->rx_ffb_mix.fine;
    stats.OFFSET_RX_FFB_MIX_GAIN_DB = ahm_calc_gain_db(
        p_gain->rx_ffb_mix.fine,
        p_gain->rx_ffb_mix.coarse);

    p_gain = p_ext_data->p_cur_gain1;
    stats.OFFSET_FF_FINE_GAIN_CTRL_INST1 = p_gain->ff.fine;

    pparam = (ParamType*)(&stats);
    for (i = 0; i < AHM_N_STAT/2; i++)
    {
        resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
    }
    if ((AHM_N_STAT % 2) == 1) /* last one */
    {
        cpsPack1Word(pparam[AHM_N_STAT-1], resp);
    }

    return TRUE;
}

bool ahm_opmsg_set_sample_rate(OPERATOR_DATA *op_data,
                               void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    DEBUG_AHM("OPID: %x, ahm_opmsg_set_sample_rate Fn entry", INT_TO_EXT_OPID(op_data->id));

    p_ext_data->sample_rate = SAMPLE_RATE_FROM_COMMON_OPMSG(message_data);
    p_ext_data->samples_per_period = \
    ahm_calc_samples_per_period(p_ext_data->sample_rate,
                                p_ext_data->timer_period,
                                p_ext_data->timer_decimation);

    return TRUE;
}

bool ahm_opmsg_set_timer_period(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    int timer_period;
    uint16 decimation;
    unsigned fast_rate;
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    DEBUG_AHM("OPID: %x, ahm_opmsg_set_timer_period Fn entry", INT_TO_EXT_OPID(op_data->id));

    /* get the timer period */
    timer_period = OPMSG_FIELD_GET(message_data,
                                   OPMSG_AHM_SET_TIMER_PERIOD,
                                   TIMER_PERIOD);

    if ((timer_period < AHM_MIN_TIMER_PERIOD_US) ||
        (timer_period > AHM_MAX_TIMER_PERIOD_US))
    {
        L0_DBG_MSG2("OPID: %x, AHM: timer period %d out of bounds",
                    INT_TO_EXT_OPID(op_data->id), timer_period);
        timer_period = AHM_DEF_TIMER_PERIOD_US;
    }

    p_ext_data->timer_period = timer_period;

    decimation = (uint16)OPMSG_FIELD_GET(message_data,
                                         OPMSG_AHM_SET_TIMER_PERIOD,
                                         DECIM_FACTOR);
    if ((decimation < AHM_MIN_TIMER_DECIMATION) ||
        (decimation > AHM_MAX_TIMER_DECIMATION))
    {
        L0_DBG_MSG2("OPID: %x, AHM: timer decimation %d out of bounds",
                    INT_TO_EXT_OPID(op_data->id), decimation);
        decimation = AHM_DEF_TIMER_DECIMATION;
    }

    p_ext_data->timer_decimation = decimation;

    p_ext_data->samples_per_period = \
        ahm_calc_samples_per_period(p_ext_data->sample_rate,
                                    p_ext_data->timer_period,
                                    p_ext_data->timer_decimation);

    fast_rate = 1000000 / p_ext_data->timer_period;
    p_ext_data->fast_rate = fast_rate;
    p_ext_data->slow_rate = fast_rate / p_ext_data->timer_decimation;

    p_ext_data->timer_counter = 0;

    return TRUE;
}

/****************************************************************************
Custom opmsg handlers
*/

/**
 * \brief  Add headroom to a gain value if it's greater than a threshold
 *
 * \param p_gain Pointer to the AHM_GAIN value to update
 *
 * \return  Nothing
 */
static void ahm_add_headroom(AHM_GAIN *p_gain, EXT_OP_ID ext_op_id)
{
    int16 coarse_gain;
    uint16 fine_gain;

    coarse_gain = p_gain->coarse;
    fine_gain = p_gain->fine;
    /* Add headroom for adaptivity. If the fine gain is too large, decrease
    * it by 6dB and increment the coarse gain to compensate.
    */
    if (fine_gain > AHM_FF_FINE_MAX_THRESHOLD)
    {
        fine_gain = (uint16)(fine_gain + 1) >> 1;
        coarse_gain++;
    }
    else if (fine_gain < AHM_FF_FINE_MIN_THRESHOLD)
    {
        L0_DBG_MSG3("OPID: %x, AHM static fine gain too low: %hu (< %hu)",
                    ext_op_id, fine_gain, AHM_FF_FINE_MIN_THRESHOLD);
    }
    p_gain->coarse = coarse_gain;
    p_gain->fine = fine_gain;

    return;
}

static void ahm_check_coarse_gain_change(unsigned instance,
                                         AHM_ANC_CONFIG *p_config,
                                         AHM_GAIN_BANK *p_static_gains,
                                         bool *cfg)
{

    uint16 ff_coarse=0, fb_coarse=0, ec_coarse=0;

#ifndef RUNNING_ON_KALSIM
    stream_anc_get_anc_coarse_gain((STREAM_ANC_INSTANCE) instance,
                                  (STREAM_ANC_PATH) p_config->ff_path,
                                  (uint16 *)&ff_coarse);
    stream_anc_get_anc_coarse_gain((STREAM_ANC_INSTANCE) instance,
                                  (STREAM_ANC_PATH) p_config->fb_path,
                                  (uint16 *)&fb_coarse);
    stream_anc_get_anc_coarse_gain((STREAM_ANC_INSTANCE) instance,
                                  (STREAM_ANC_PATH) AHM_ANC_PATH_FB_ID,
                                  (uint16 *)&ec_coarse);
#endif

    /* comparing the least significant 4 bits only */
    if((p_static_gains->ff.coarse & AHM_COARSE_GAIN_MASK) != ff_coarse)
    {
        cfg[AHM_ANC_FILTER_FF_ID] = TRUE;
    }
    if((p_static_gains->fb.coarse & AHM_COARSE_GAIN_MASK) != fb_coarse)
    {
        cfg[AHM_ANC_FILTER_FB_ID] = TRUE;
    }
    if((p_static_gains->ec.coarse & AHM_COARSE_GAIN_MASK) != ec_coarse)
    {
        cfg[AHM_ANC_FILTER_EC_ID] = TRUE;
    }
}

bool ahm_opmsg_set_static_gains(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    AHM_GAIN_BANK *p_static;
    unsigned *p_resp;
    unsigned msg_id;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    DEBUG_AHM("OPID: %x, ahm_opmsg_set_static_gains Fn entry", ext_op_id);

    unsigned int instance ;
    instance = OPMSG_FIELD_GET(message_data,
                               OPMSG_SET_AHM_STATIC_GAINS,
                               ANC_INSTANCE);
    if (instance == AHM_ANC_INSTANCE_ANC1_ID)
    {
        p_static = p_ext_data->p_static_gain1;   
    }
    else
    {
        /* default writing to the instance 0 gains*/
        p_static = p_ext_data->p_static_gain;
		/* Reset ramp required and coarse gain change flag */
        p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID] = FALSE;
        p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID] = FALSE;
        p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID] = FALSE;
        p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] = FALSE;
        p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] = FALSE;
        p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] = FALSE;
    }

    p_static->ff.coarse = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, FF_COARSE_STATIC_GAIN);
    p_static->ff.fine = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, FF_FINE_STATIC_GAIN);
    p_static->fb.coarse = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, FB_COARSE_STATIC_GAIN);
    p_static->fb.fine = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, FB_FINE_STATIC_GAIN);
    p_static->ec.coarse = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, EC_COARSE_STATIC_GAIN);
    p_static->ec.fine = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, EC_FINE_STATIC_GAIN);
    p_static->rx_ffa_mix.coarse = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, RX_FFA_MIX_COARSE_STATIC_GAIN);
    p_static->rx_ffa_mix.fine = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, RX_FFA_MIX_FINE_STATIC_GAIN);
    p_static->rx_ffb_mix.coarse = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, RX_FFB_MIX_COARSE_STATIC_GAIN);
    p_static->rx_ffb_mix.fine = OPMSG_FIELD_GET(
        message_data, OPMSG_SET_AHM_STATIC_GAINS, RX_FFB_MIX_FINE_STATIC_GAIN);

    if ((p_ext_data->ahm_cap_params.OFFSET_AHM_CONFIG & \
         ANC_HW_MANAGER_CONFIG_AHM_CONFIG_DISABLE_FF_GAIN_ADJUSTMENT) == 0)
    {
        ahm_add_headroom(&p_static->ff, ext_op_id);
        if(p_ext_data->config.ff_path == AHM_ANC_PATH_FFB_ID)
        {
           ahm_add_headroom(&p_static->rx_ffb_mix, ext_op_id);
        }
        else
        {
           ahm_add_headroom(&p_static->rx_ffa_mix, ext_op_id);
        }
    }

    L2_DBG_MSG4("OPID: %x, Instance: %d, AHM Set Static Coarse Gain: FF = %hd, FB = %hd",
                ext_op_id, instance, p_static->ff.coarse, p_static->fb.coarse);
    L2_DBG_MSG5("OPID: %x, Instance: %d, AHM Set Static Coarse Gain: EC = %hd, "
                "Rx FFa = %hd, Rx FFb = %hd", ext_op_id, instance,
                p_static->ec.coarse,
                p_static->rx_ffa_mix.coarse,
                p_static->rx_ffb_mix.coarse);
    L2_DBG_MSG4("OPID: %x, Instance: %d, AHM Set Static Fine Gain: FF = %hu, FB = %hu",
                ext_op_id, instance, p_static->ff.fine, p_static->fb.fine);

    L2_DBG_MSG5("OPID: %x, Instance: %d, AHM Set Static Fine Gain: EC = %hu, "
                "Rx FFa = %hu, Rx FFb = %hu", ext_op_id, instance,
                p_static->ec.fine,
                p_static->rx_ffa_mix.fine,
                p_static->rx_ffb_mix.fine);

    /* Override target gains when static gains are updated */
    p_ext_data->ff_fine_tgt_gain = p_static->ff.fine;
    p_ext_data->fb_fine_tgt_gain = p_static->fb.fine;

    /* Check for the change in coarse gain and update the coarse_gain_changed flags */
    ahm_check_coarse_gain_change(instance,
                                 &p_ext_data->config,
                                 p_static,
                                 p_ext_data->coarse_gain_changed);

    /* Allow a direct gain update if the sysmode is static without requiring
     * a follow-up gain override
     */
    if (p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_STATIC)
    {
        if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID && instance == AHM_ANC_INSTANCE_ANC1_ID)
        {
            *p_ext_data->p_cur_gain1 = *p_ext_data->p_static_gain1;
            p_ext_data->ahm_fine_nominal1[AHM_ANC_FILTER_FF_ID].gain = \
                (uint8)p_static->ff.fine;
            p_ext_data->ahm_fine_nominal1[AHM_ANC_FILTER_FB_ID].gain = \
                (uint8)p_static->fb.fine;
            p_ext_data->ahm_fine_nominal1[AHM_ANC_FILTER_EC_ID].gain = \
                (uint8)p_static->ec.fine;

            update_dynamic_nominal_gain(p_ext_data->p_nominal_gain1,
                                        p_ext_data->p_fine_nominal1);
            /* Update static nominal gain. Dynamic nominal gain is updated when
             * SET_CONTROL is called for a mode change.*/
            update_static_nominal_gain(p_static, p_ext_data->p_nominal_gain1);            
        }
        else
        {
            *p_ext_data->p_cur_gain = *p_ext_data->p_static_gain;
            p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_FF_ID].gain = \
                (uint8)p_static->ff.fine;
            p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_FB_ID].gain = \
                (uint8)p_static->fb.fine;
            p_ext_data->ahm_fine_nominal[AHM_ANC_FILTER_EC_ID].gain = \
                (uint8)p_static->ec.fine;

            update_dynamic_nominal_gain(p_ext_data->p_nominal_gain,
                                        p_ext_data->p_fine_nominal);
            /* Update static nominal gain. Dynamic nominal gain is updated when
             * SET_CONTROL is called for a mode change.*/
            update_static_nominal_gain(p_static, p_ext_data->p_nominal_gain);
        }
    }

    /* Echo the static gains back to the caller. This allows any adjustment
     * to be tracked upstream.
     */
    *resp_length = OPMSG_SET_AHM_STATIC_GAINS_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_SET_AHM_STATIC_GAINS_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_SET_AHM_STATIC_GAINS_RESP,
                               MESSAGE_ID,
                               msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             ANC_INSTANCE,
                             instance);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             FF_COARSE_GAIN,
                             p_static->ff.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             FF_FINE_GAIN,
                             p_static->ff.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             FB_COARSE_GAIN,
                             p_static->fb.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             FB_FINE_GAIN,
                             p_static->fb.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             EC_COARSE_GAIN,
                             p_static->ec.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             EC_FINE_GAIN,
                             p_static->ec.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             RX_FFA_MIX_COARSE_GAIN,
                             p_static->rx_ffa_mix.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             RX_FFA_MIX_FINE_GAIN,
                             p_static->rx_ffa_mix.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             RX_FFB_MIX_COARSE_GAIN,
                             p_static->rx_ffb_mix.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_SET_AHM_STATIC_GAINS_RESP,
                             RX_FFB_MIX_FINE_GAIN,
                             p_static->rx_ffb_mix.fine);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}
bool ahm_opmsg_set_iir_filter_coeffs(OPERATOR_DATA *op_data,
                                void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    DEBUG_AHM("OPID: %x, ahm_opmsg_set_iir_filter_coeffs Fn entry", ext_op_id);
    unsigned int instance ;

    instance = OPMSG_FIELD_GET(message_data, OPMSG_SET_IIR_FILTER_COEFFS,
                               ANC_INSTANCE);
    if (instance ==  AHM_ANC_INSTANCE_ANC0_ID)
    {
        ahm_update_filter_coeffs(p_ext_data->p_prev_iir_filter_inst1, p_ext_data->p_iir_filter_inst1,
                                 p_ext_data->ramp_required, message_data,
                                 p_ext_data->mode,
                                 ext_op_id);
    }
    else if (instance == AHM_ANC_INSTANCE_ANC1_ID)
    {
        ahm_update_filter_coeffs(p_ext_data->p_prev_iir_filter_inst2, p_ext_data->p_iir_filter_inst2,
                                 p_ext_data->ramp_required, message_data,
                                 p_ext_data->mode,
                                 ext_op_id);

    }
    else
    {
        L2_DBG_MSG1("OPID: %x, Invalid instance type", ext_op_id);
    }

    return TRUE;
}

bool ahm_opmsg_get_gains(OPERATOR_DATA *op_data,
                         void *message_data,
                         unsigned *resp_length,
                         OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned *p_resp;
    AHM_GAIN_BANK *p_gain;
    unsigned msg_id, anc_instance;

    anc_instance = OPMSG_FIELD_GET(message_data,
                                   OPMSG_GET_AHM_GAINS,
                                   ANC_INSTANCE);
    *resp_length = OPMSG_GET_AHM_GAINS_RESP_WORD_SIZE;

    p_resp = xzpnewn(OPMSG_GET_AHM_GAINS_RESP_WORD_SIZE, unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    p_gain = p_ext_data->p_cur_gain;
    if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID && anc_instance == AHM_ANC_INSTANCE_ANC1_ID)
    {
        p_gain = p_ext_data->p_cur_gain1;
    }
    
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_GET_AHM_GAINS_RESP,
                               MESSAGE_ID,
                               msg_id);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             ANC_INSTANCE,
                             anc_instance);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             FF_COARSE_GAIN,
                             p_gain->ff.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             FF_FINE_GAIN,
                             p_gain->ff.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             FB_COARSE_GAIN,
                             p_gain->fb.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             FB_FINE_GAIN,
                             p_gain->fb.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             EC_COARSE_GAIN,
                             p_gain->ec.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             EC_FINE_GAIN,
                             p_gain->ec.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             RX_FFA_MIX_COARSE_GAIN,
                             p_gain->rx_ffa_mix.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             RX_FFA_MIX_FINE_GAIN,
                             p_gain->rx_ffa_mix.fine);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             RX_FFB_MIX_COARSE_GAIN,
                             p_gain->rx_ffb_mix.coarse);
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_GET_AHM_GAINS_RESP,
                             RX_FFB_MIX_FINE_GAIN,
                             p_gain->rx_ffb_mix.fine);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    return TRUE;
}

bool ahm_opmsg_get_shared_gain_ptr(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    AHM_FINE_GAIN_NODE **p_head;
    AHM_SHARED_FINE_GAIN *p_gain;
    AHM_GAIN_BANK *p_cur_gain, *p_nominal_gain, *p_static_gain;

    AHM_GAIN *p_stat_path;

    unsigned *p_resp;
    unsigned anc_filter_data, anc_instance_data, is_coarse_data, p_recv_ext_data, msg_id;
    AHM_ANC_FILTER anc_filter;
    AHM_ANC_INSTANCE anc_instance;
    bool is_coarse;
    unsigned gc_type_data;
    AHM_GAIN_CONTROL_TYPE gc_type;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    DEBUG_AHM("OPID: %x, ahm_opmsg_get_shared_gain_ptr Fn entry", ext_op_id);

    /* Only handle fine gains */
    is_coarse_data = OPMSG_FIELD_GET(message_data,
                                     OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                     COARSE);
    is_coarse = (bool)is_coarse_data;

    if (is_coarse)
    {
        L2_DBG_MSG1("OPID: %x, AHM Shared coarse gain not supported", ext_op_id);
        return FALSE;
    }

    /* Interpret the gain control type */
    gc_type_data = OPMSG_FIELD_GET(message_data,
                                   OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                   CONTROL_TYPE);
    gc_type = (AHM_GAIN_CONTROL_TYPE)gc_type_data;

    /* Get the filter to share gains on */
    anc_filter_data = OPMSG_FIELD_GET(message_data,
                                      OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                      FILTER);
    anc_filter = (AHM_ANC_FILTER)anc_filter_data;

    /* Get the instance to share gains on */
    anc_instance_data = OPMSG_FIELD_GET(message_data,
                                        OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                        CHANNEL);
    anc_instance = (AHM_ANC_INSTANCE)anc_instance_data;


    p_stat_path = &p_ext_data->p_static_gain->ff ;
    if (anc_instance == AHM_ANC_INSTANCE_ANC0_ID)
    {
        if (anc_filter == AHM_ANC_FILTER_FF_ID)
        {
            p_stat_path = &p_ext_data->p_static_gain->ff;
        }
        else if (anc_filter == AHM_ANC_FILTER_FB_ID)
        {
            p_stat_path = &p_ext_data->p_static_gain->fb;
        }
        else if (anc_filter == AHM_ANC_FILTER_EC_ID)
        {
            p_stat_path = &p_ext_data->p_static_gain->ec;
        }
    }
    else if (anc_instance == AHM_ANC_INSTANCE_ANC1_ID)
    {
        if (anc_filter == AHM_ANC_FILTER_FF_ID)
        {
            p_stat_path = &p_ext_data->p_static_gain1->ff;
        }
    }
    else
    {
        L0_DBG_MSG2("OPID: %x, AHM invalid anc instance %d", ext_op_id, anc_instance);
    }
    p_cur_gain = p_ext_data->p_cur_gain;
    p_nominal_gain = p_ext_data->p_nominal_gain;
    p_static_gain = p_ext_data->p_static_gain;

    L2_DBG_MSG2("OPID: %x, AHM link filter block %d", ext_op_id, anc_filter);

    switch (gc_type)
    {
        case AHM_GAIN_CONTROL_TYPE_DELTA:
            p_head = &p_ext_data->p_fine_delta_head[anc_filter];
            p_gain = ahm_list_fine_gain_add(p_head, ext_op_id);
            p_gain->gain_current = AHM_DELTA_NOMINAL;
            p_gain->gain_delta = AHM_DELTA_NOMINAL;
            break;
        case AHM_GAIN_CONTROL_TYPE_NOMINAL:
            /* Hand out the client gain address */
            if (anc_instance == AHM_ANC_INSTANCE_ANC0_ID)
            {
                p_ext_data->client_fine_nominal[anc_filter].gain = (uint8) p_stat_path->fine;
                p_gain = &p_ext_data->client_fine_nominal[anc_filter];
                p_ext_data->p_fine_nominal[anc_filter] = p_gain;
                p_ext_data->ahm_nominal_client[anc_filter] = TRUE;
                p_static_gain = p_ext_data->p_static_gain;
            }
            else
            {
                p_ext_data->client_fine_nominal1[anc_filter].gain = (uint8) p_stat_path->fine;
                p_gain = &p_ext_data->client_fine_nominal1[anc_filter];
                p_ext_data->p_fine_nominal1[anc_filter] = p_gain;
                p_ext_data->ahm_nominal_client1[anc_filter] = TRUE;
                p_cur_gain = p_ext_data->p_cur_gain1;
                p_nominal_gain = p_ext_data->p_nominal_gain1;
                p_static_gain =p_ext_data->p_static_gain1;
            }
            break;
        default:
            L2_DBG_MSG2("OPID: %x, AHM invalid gain control type %d", ext_op_id, gc_type);
            return FALSE;
    }

    p_gain->gain_type = gc_type;

    p_recv_ext_data = OPMSG_FIELD_GET32(message_data,
                                        OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                        P_EXT_DATA);

    /* Send OPMSG_COMMON_ID_RECV_ANC_HW_MANAGER_PTR in response */
    p_resp = xzpnewn(OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP_WORD_SIZE,
                     unsigned);
    if (p_resp == NULL)
    {
        L0_DBG_MSG1("OPID: %x, AHM failed to create shared gain response", ext_op_id);
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
    /* Set the gains */
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               SHARED_GAIN_PTR,
                               (unsigned)p_gain);
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               CURRENT_GAINS_PTR,
                               (unsigned)p_cur_gain);
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               STATIC_GAINS_PTR,
                               (unsigned)p_static_gain);
    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                               NOMINAL_GAINS_PTR,
                               (unsigned)p_nominal_gain);
    /* Echo the filter type */
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                             FILTER_TYPE,
                             anc_filter);
    /* Echo the anc instance */
    OPMSG_CREATION_FIELD_SET(p_resp,
                             OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                             CHANNEL,
                             anc_instance);
    OPMSG_CREATION_FIELD_SET32(p_resp,
                              OPMSG_COMMON_MSG_GET_SHARED_GAIN_RESP,
                              AHM_TIMER_PERIOD_US,
                              p_ext_data->timer_period);

    *resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

    /* Set reinitialization to make sure the list gets resorted */
    aud_cur_set_reinit(op_data, TRUE);

    return TRUE;
}

bool ahm_opmsg_free_shared_gain_ptr(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    AHM_FINE_GAIN_NODE **p_head;
    AHM_SHARED_FINE_GAIN *p_gain;

    OPMSG_FREE_AHM_SHARED_GAIN_PTR *p_msg;
    unsigned anc_filter_data, is_coarse_data, p_gain_data;
    AHM_ANC_FILTER anc_filter;
    unsigned anc_instance_data;
    bool is_coarse;
    unsigned gc_type_data;
    AHM_GAIN_CONTROL_TYPE gc_type;
    AHM_ANC_INSTANCE anc_instance;

    p_msg = (OPMSG_FREE_AHM_SHARED_GAIN_PTR*)message_data;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    DEBUG_AHM("OPID: %x, ahm_opmsg_free_shared_gain_ptr Fn entry", ext_op_id);

    /* Only handle fine gains */
    is_coarse_data = OPMSG_FIELD_GET(p_msg,
                                     OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                                     COARSE);
    is_coarse = (bool)is_coarse_data;

    if (is_coarse)
    {
        L2_DBG_MSG1("OPID: %x, AHM Shared coarse gain not supported", ext_op_id);
        return FALSE;
    }

    /* Interpret the gain control type */
    gc_type_data = OPMSG_FIELD_GET(message_data,
                                   OPMSG_COMMON_MSG_GET_SHARED_GAIN,
                                   CONTROL_TYPE);
    gc_type = (AHM_GAIN_CONTROL_TYPE)gc_type_data;

    /* Get the filter to share gains on */
    anc_filter_data = OPMSG_FIELD_GET(p_msg,
                                      OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                                      FILTER);
    anc_filter = (AHM_ANC_FILTER)anc_filter_data;


    /* Get the filter to share gains on */
    anc_instance_data = OPMSG_FIELD_GET(p_msg,
                                        OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                                        CHANNEL);
    anc_instance = (AHM_ANC_INSTANCE)anc_instance_data;
    /* Dynamic control is only available on FF, FB or EC filters */

    L2_DBG_MSG2("OPID: %x, AHM unlink filter block %d", ext_op_id, anc_filter);


    p_gain_data = OPMSG_FIELD_GET(p_msg,
                                  OPMSG_FREE_AHM_SHARED_GAIN_PTR,
                                  SHARED_GAIN_PTR);

    p_gain = (AHM_SHARED_FINE_GAIN*)p_gain_data;


    switch (gc_type)
    {
        case AHM_GAIN_CONTROL_TYPE_DELTA:
            p_head = &p_ext_data->p_fine_delta_head[anc_filter];
            ahm_list_fine_gain_remove(p_head, p_gain, ext_op_id);
            break;
        case AHM_GAIN_CONTROL_TYPE_NOMINAL:
            if (anc_instance == AHM_ANC_INSTANCE_ANC0_ID)
            {
                p_gain = &p_ext_data->ahm_fine_nominal[anc_filter];
                p_ext_data->p_fine_nominal[anc_filter] = p_gain;
                p_ext_data->ahm_nominal_client[anc_filter] = FALSE;
            }
            else
            {
                p_gain = &p_ext_data->ahm_fine_nominal1[anc_filter];
                p_ext_data->p_fine_nominal1[anc_filter] = p_gain;
                p_ext_data->ahm_nominal_client1[anc_filter] = FALSE;
            }
            break;
        default:
            L2_DBG_MSG2("OPID: %x, AHM Unsupported control type %d", ext_op_id, gc_type);
            return FALSE;
    }

    return TRUE;
}

bool ahm_opmsg_set_target_makeup_gain(OPERATOR_DATA *op_data,
                                      void *message_data,
                                      unsigned *resp_length,
                                      OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    AHM_GAIN_BANK *p_stat = p_ext_data->p_static_gain;
    OPMSG_SET_TARGET_MAKEUP_GAIN *p_msg;

    int tgt_msb, tgt_lsb;

    DEBUG_AHM("OPID: %x, ahm_opmsg_set_target_makeup_gain Fn entry", INT_TO_EXT_OPID(op_data->id));

    p_msg = (OPMSG_SET_TARGET_MAKEUP_GAIN*)message_data;

    tgt_msb = OPMSG_FIELD_GET(p_msg,
                              OPMSG_SET_TARGET_MAKEUP_GAIN,
                              MAKEUP_GAIN);
    tgt_lsb = OPMSG_FIELD_GET_FROM_OFFSET(p_msg,
                                          OPMSG_SET_TARGET_MAKEUP_GAIN,
                                          MAKEUP_GAIN,
                                          1);

    p_ext_data->target_makeup_gain = (tgt_msb << 16) | tgt_lsb;

    p_ext_data->ff_fine_tgt_gain = \
        aud_cur_calc_adjusted_gain(p_stat->ff.fine, p_ext_data->target_makeup_gain);

    p_ext_data->set_target_flag = TRUE;
    return TRUE;
}

bool ahm_opmsg_set_fine_target_gain(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    uint16 filter;
    uint16 tgt_gain;
    OPMSG_SET_FINE_TARGET_GAIN * p_msg;
    DEBUG_AHM("OPID: %x, ahm_opmsg_set_fine_target_gain Fn entry", INT_TO_EXT_OPID(op_data->id));

    p_msg = (OPMSG_SET_FINE_TARGET_GAIN *)message_data;
    filter = OPMSG_FIELD_GET(p_msg,
                             OPMSG_SET_FINE_TARGET_GAIN,
                             FILTER);
    tgt_gain = OPMSG_FIELD_GET(p_msg,
                               OPMSG_SET_FINE_TARGET_GAIN,
                               TARGET_GAIN);

    switch (filter)
    {
        case AHM_ANC_FILTER_FF_ID:
            p_ext_data->ff_fine_tgt_gain = tgt_gain;
            p_ext_data->set_target_flag = TRUE;
            break;
        case AHM_ANC_FILTER_FB_ID:
            p_ext_data->fb_fine_tgt_gain = tgt_gain;
            break;
        default:
            L2_DBG_MSG2("OPID: %x, AHM Unsupported filter block %d", INT_TO_EXT_OPID(op_data->id), filter);
            return FALSE;
    }

    return TRUE;
}

bool ahm_opmsg_set_zcd_disable(OPERATOR_DATA *op_data,
                               void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);

    uint16 disable;
    OPMSG_AHM_SET_ZCD_DISABLE * p_msg;
    DEBUG_AHM("OPID: %x, ahm_opmsg_set_zcd_disable Fn entry", INT_TO_EXT_OPID(op_data->id));

    p_msg = (OPMSG_AHM_SET_ZCD_DISABLE *)message_data;
    disable = OPMSG_FIELD_GET(p_msg,
                              OPMSG_AHM_SET_ZCD_DISABLE,
                              DISABLE_ZCD);

    p_ext_data->disable_zcd = disable;

#ifndef RUNNING_ON_KALSIM
    /* Update ANC HW */
    ahm_set_anc_zcd_enable(!disable, p_ext_data->config.channel);
#endif

    return TRUE;
}
/****************************************************************************
Data processing function
*/

/**
 * \brief  Check the ANC state and initialize data structures
 *
 * \param  op_data          Pointer to operator data
 * \param  p_ext_data       Pointer to the AHM extra_op_data
 *
 * \return  TRUE if checks are valid, FALSE otherwise
 *
 */
static inline bool ahm_check_and_initialize(OPERATOR_DATA *op_data,
                                            AHM_OP_DATA *p_ext_data)
{
    AHM_GAIN_BANK *p_static_gain;
    unsigned rxmix_gain_diff;

    /* Standby mode does no gain updates or calculations */
    if (p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_STANDBY)
    {
        return FALSE;
    }

    /* Out of ear does no gain updates or calculations */
    if (!p_ext_data->in_out_status &&
        ((p_ext_data->ahm_cap_params.OFFSET_AHM_CONFIG & \
            ANC_HW_MANAGER_CONFIG_AHM_CONFIG_DISABLE_EAR_CHECK) == 0))
    {
        return FALSE;
    }

    /* Clock check failure does no gain updates or calculations */
#ifndef RUNNING_ON_KALSIM
    if (p_ext_data->ahm_cap_params.OFFSET_AHM_CONFIG & \
        ANC_HW_MANAGER_CONFIG_AHM_CONFIG_DISABLE_ANC_CLOCK_CHECK)
    {
        p_ext_data->clock_status = TRUE;
    }
    else
    {
        uint16 anc0_enable;
        uint16 anc1_enable;
        uint16 *anc_selected = &anc0_enable;

        stream_get_anc_enable(&anc0_enable, &anc1_enable);

        if (p_ext_data->config.channel == AHM_ANC_INSTANCE_ANC1_ID)
        {
            anc_selected = &anc1_enable;
        }

        if (*anc_selected == p_ext_data->anc_clock_check_value)
        {
            p_ext_data->clock_status = TRUE;
        }
        else
        {
            p_ext_data->clock_status = FALSE;
            return FALSE;
        }
    }
#endif

    /* Reinitialize picks up the latest parameter values */
    if (aud_cur_get_reinit(op_data))
    {
        aud_cur_set_reinit(op_data, FALSE);

        /* Compute feedforward rxmix static gain fine difference in dB w.r.to
            feedforward static fine gain */
        p_static_gain = p_ext_data->p_static_gain;
        /* Calculate gain diff in dB (Q12.20) */
        rxmix_gain_diff = \
            ahm_get_gain_difference_db(p_static_gain->rx_ffb_mix.fine,
                                       0,
                                       p_static_gain->ff.fine,
                                       0);
        /* Convert from dB to log2 format (Q12.20) */
        rxmix_gain_diff = ahm_convert_db_to_log2(rxmix_gain_diff);
        /* Convert to Q2.30 and cache it */
        p_ext_data->rxmix_gain_diff = \
            rxmix_gain_diff << AHM_Q12_TO_Q2_LSHIFT_AMT;
    }

    return TRUE;
}

/**
 * \brief  Calculate denormalized gain and check if it's within limit
 *
 * \param  delta_gain
 * \param  nominal gain
 *
 * \return  TRUE if denormalized gain < 10, False otherwise
 *
 */

static bool ahm_check_small_step_ramp_feasible(int delta_gain, uint16 nominal_gain)
{
    uint16 upd_gain;

    upd_gain = (uint16)(((delta_gain >> AHM_DELTA_GAIN_SHIFT) * nominal_gain) >> AHM_DELTA_PRECISION);

    if(upd_gain < 10U && (nominal_gain > 0))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * \brief  Calculate normalized gain in steps
 *
 * \param  nominal_gain
 * \param  step
 *
 * \return  normalized gain
 *
 */

static uint32 ahm_get_small_step_value(uint16 nominal_gain, uint16 step)
{
    uint32 nominal_gain_step =0;

    if(nominal_gain > 0)
    {
        nominal_gain_step = (uint32)(step << AHM_DIV_PRECISION)/(nominal_gain);
    }

    return nominal_gain_step;
}
/**
 * \brief  AHM process function
 *
 * \param  op_data          Pointer to operator data
 * \param  samples          Number of samples. Only used when calculating linear
 *                          gain ramps in an audio chain.
 * \param  run_minimal      Whether to run a minimal computation (just delta
 *                          gain calculations) or the full set.
 *
 * \return  None
 *
 */
static void ahm_process_function(OPERATOR_DATA *op_data,
                                 unsigned samples,
                                 bool run_minimal)
{
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    AHM_FINE_GAIN_NODE *p_node;
    AHM_SHARED_FINE_GAIN *p_fine_gain, *p_fine_gain1;
    AHM_GAIN_BANK *p_cur_gain, *p_nominal, *p_stat_gain;
    AHM_GAIN *p_cur_filter, *p_nominal_filter;
    AHM_GAIN_BANK *p_cur_gain1, *p_nominal1;
    AHM_GAIN *p_cur_filter1, *p_nominal_filter1;
    int tc, i;
    int cur_delta, nom_delta;
    EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(op_data->id);
    uint16 nominal_gain, upd_gain;
    int min_delta;
    bool gain_update, delta_changed, any_delta_changed = FALSE;
    AHM_RAMP_CONFIG cfg[AHM_NUM_DYNAMIC_FILTERS];
    p_nominal = p_ext_data->p_nominal_gain;
    p_nominal_filter = &p_nominal->ff;
    AHM_EVENT_MSG msg;

    if (!ahm_check_and_initialize(op_data, p_ext_data))
    {
        return;
    }
    /* Update nominal gain */
    if(p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_FULL)
    {
        update_dynamic_nominal_gain(p_ext_data->p_nominal_gain,
                                    p_ext_data->p_fine_nominal);
        if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
        {
            update_dynamic_nominal_gain(p_ext_data->p_nominal_gain1,
                                        p_ext_data->p_fine_nominal1);
        }
    }

    if ((!run_minimal) && (p_ext_data->cur_mode != ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION))
    {
        for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
        {
            /* Process the gain ramp state machine */
            if (p_ext_data->fine_ramp[i].state != AHM_RAMP_IDLE)
            {
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[i], p_nominal_filter[i].fine);
            }
        }
    }

    /* Iterate through dynamic paths */
    p_cur_gain  = p_ext_data->p_cur_gain;
    p_stat_gain = p_ext_data->p_static_gain;
    p_cur_filter = &p_cur_gain->ff;
    p_nominal = p_ext_data->p_nominal_gain;
    p_nominal_filter = &p_nominal->ff;
    gain_update = FALSE;

    /* Initialization with channel 0 gains */
    p_cur_gain1 = p_ext_data->p_cur_gain;
    p_cur_filter1 = &p_cur_gain->ff;
    p_nominal1 = p_ext_data->p_nominal_gain;
    p_nominal_filter1 = &p_nominal->ff;

    if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
    {
        p_cur_gain1 = p_ext_data->p_cur_gain1;
        p_cur_filter1 = &p_cur_gain1->ff;
        p_nominal1 = p_ext_data->p_nominal_gain1;
        p_nominal_filter1 = &p_nominal1->ff;
    }
    for (i = 0; i < AHM_NUM_DYNAMIC_FILTERS; i++)
    {
        /* First gain priority are delta gains from the timer task */
        min_delta = AHM_DELTA_MAX;
        delta_changed = FALSE;
        p_node = p_ext_data->p_fine_delta_head[i];
        while (p_node != NULL)
        {
            p_fine_gain = &p_node->data;
            tc = p_fine_gain->tc_release;

            if (p_fine_gain->gain_delta != AHM_DELTA_NOMINAL)
            {
                delta_changed = TRUE;
                any_delta_changed = TRUE;
                /* Attack is considered to be a gain reduction, i.e. nominal
                 * operating condition is 1.0 and attack would be to reduce
                 * the gain to protect against a given condition.
                 */
                if (p_fine_gain->gain_delta < p_fine_gain->gain_current)
                {
                    tc = p_fine_gain->tc_attack;
                }

                cur_delta = p_fine_gain->gain_delta - p_fine_gain->gain_current;

                //small fine steps for fine gain ramping up for FF path only
                //TODO -move this code to ahm_ramp
                if (( (p_fine_gain ==p_ext_data->p_fine_delta_ramp[AHM_ANC_FILTER_FF_ID])||
                    (p_fine_gain ==p_ext_data->p_fine_delta_ramp[AHM_ANC_FILTER_FB_ID]) ||
                    (p_fine_gain ==p_ext_data->p_fine_delta_ramp[AHM_ANC_FILTER_EC_ID])) &&
                    (cur_delta > 0) &&
                   (ahm_check_small_step_ramp_feasible(p_fine_gain->gain_current, p_nominal_filter->fine)) )
                {

                    /* postivie delta i.e., ramp up and small step ramp required */
                    p_fine_gain->gain_current += ahm_get_small_step_value(p_nominal_filter->fine, 1U); /* increment smaller steps */
                    if (p_fine_gain->gain_current < min_delta)
                        {
                           min_delta = p_fine_gain->gain_current;
                        }
                }
                else
                {
                        /* TC is Q1.31, gain delta is Q8.24 */
                        /* positive ramp and negative ramp using algorithm*/
                        p_fine_gain->gain_current += frac_mult(tc, cur_delta);
                 }

                if (p_fine_gain->gain_current < min_delta)
                {
                    min_delta = p_fine_gain->gain_current;
                }
            }
            p_node = p_node->p_next;
        }

        /* Delta gain has been calculated and is applied */
        if (delta_changed)
        {
            nominal_gain = p_nominal_filter->fine;
            /* Calculate adjusted gain */
            nom_delta = ((min_delta >> AHM_DELTA_GAIN_SHIFT) * nominal_gain);
            nom_delta += AHM_DELTA_ROUNDING;
            upd_gain = (uint16)(nom_delta >> AHM_DELTA_PRECISION);
            p_cur_filter->fine = upd_gain;

            /* Set the nominal flag on this path to FALSE */
            p_ext_data->p_fine_nominal[i]->using_nominal = FALSE;
            gain_update = TRUE;
            
            if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
            {
                nominal_gain = p_nominal_filter1->fine;
                /* Calculate adjusted gain */
                nom_delta = ((min_delta >> AHM_DELTA_GAIN_SHIFT) * nominal_gain);
                nom_delta += AHM_DELTA_ROUNDING;
                upd_gain = (uint16)(nom_delta >> AHM_DELTA_PRECISION);
                p_cur_filter1->fine = upd_gain;

                /* Set the nominal flag on this path to FALSE */
                p_ext_data->p_fine_nominal1[i]->using_nominal = FALSE;
            }
        }
        /* Minimal run of the process function exits the loop early */
        if (run_minimal)
        {
            p_nominal_filter++;
            p_cur_filter++;
            p_nominal_filter1++;
            p_cur_filter1++;
            continue;
        }

        /* Second gain priority is nominal gain */
        p_fine_gain = p_ext_data->p_fine_nominal[i];
        if (!gain_update)
        {
            p_cur_filter->fine = p_nominal_filter->fine;
            p_fine_gain->using_nominal = TRUE;
        }
        else
        {
            p_fine_gain->using_nominal = FALSE;
        }
        p_nominal_filter++;
        p_cur_filter++;

        if (p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
        {
            /* Second gain priority is nominal gain */
            p_fine_gain1 = p_ext_data->p_fine_nominal1[i];
            if (!gain_update)
            {
                p_cur_filter1->fine = p_nominal_filter1->fine;
                p_fine_gain1->using_nominal = TRUE;
            }
            else
            {
                p_fine_gain1->using_nominal = FALSE;
            }
            p_nominal_filter1++;
            p_cur_filter1++;
        }
    }

    /* Compute current FF rxmix gain corrosponding to current FF gain if
        we are in hybrid adaptive ambient mode */
    if((p_ext_data->aamb_mode == ANC_HW_MANAGER_AMBIENT_CTRL_AAMB) &&
        (p_ext_data->anc_clock_check_value == AHM_HYBRID_ENABLE))
    {
        p_cur_gain->rx_ffb_mix.fine = \
            aud_cur_calc_adjusted_gain(p_cur_gain->ff.fine,
                                       p_ext_data->rxmix_gain_diff);
    }

#ifdef RUNNING_ON_KALSIM
        /* Unused variable */
        (void)any_delta_changed;
#else
    /* Update gains */
    if (p_ext_data->disable_zcd)
    {
        /* Disable ZCD only if delta gains have changed. Else enable ZCD */
        ahm_set_anc_zcd_enable(!any_delta_changed, p_ext_data->config.channel);
    }
#endif

    ahm_write_gain_generic(op_data);

    if (p_ext_data->trigger_mode == AHM_TRIGGER_START)
    {

        #ifndef RUNNING_ON_KALSIM
            stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_ANC0_MASK);
            stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_ANC1_MASK);
        #endif
        DEBUG_AHM("OPID: %x, Copy from foreground to background IIR filters for both ANC Instances!",
                  ext_op_id);
        msg.id =AHM_EVENT_ID_FILTER_TRANSITION;
        msg.type = AHM_EVENT_TYPE_CLEAR;
        msg.payload = 0;
        msg.ext_op_id = ext_op_id;
        ahm_send_event_message(op_data,&msg);
        p_ext_data->trigger_mode = 0;

        update_coarse_gain(p_cur_gain, p_stat_gain);
        if(p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
        {
            update_coarse_gain(p_cur_gain1, p_ext_data->p_static_gain1);
        }
    }

    /* Check for a filter transition */
    if ( (p_ext_data->cur_mode == ANC_HW_MANAGER_SYSMODE_FILTER_TRANSITION) )
    {
        p_nominal = p_ext_data->p_nominal_gain;
        if (p_ext_data->ramp_status == AHM_RAMP_DOWN)
        {

            L4_DBG_MSG1("OPID: %x, Waiting for relevant ramping down to finish in Filter Transition Mode",
                        ext_op_id);
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID])
            {

                p_nominal_filter = &p_nominal->ff;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                               p_nominal_filter->fine);
            }
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID])
            {
                p_nominal_filter = &p_nominal->fb ;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID],
                                               p_nominal_filter->fine);
            }
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID])
            {
                p_nominal_filter = &p_nominal->ec ;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID],
                                                p_nominal_filter->fine);
            }
            /* wait for ramping down to complete */
            if ( (p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID]).state == AHM_RAMP_IDLE &&
                 (p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID]).state == AHM_RAMP_IDLE &&
                 (p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID]).state == AHM_RAMP_IDLE )
            {
                #ifndef RUNNING_ON_KALSIM
                    if(p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID])
                    {
                       /* Apply the ANC HW Foreground filters to the background now */
                       DEBUG_AHM("OPID: %x, Ramp down complete. Copy foreground filter to background filter for both ANC instances",
                                 ext_op_id);
                       stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_ANC0_MASK);
                       stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_ANC1_MASK);
                    }
                #endif

                reinit_nominal_gain(p_ext_data);
                update_coarse_gain(p_cur_gain, p_stat_gain);
                if(p_ext_data->config.channel == AHM_ANC_INSTANCE_DUAL_ID)
                {
                    update_coarse_gain(p_cur_gain1, p_ext_data->p_static_gain1);
                }
                aud_cur_set_reinit(op_data, TRUE);
                configure_ramp_up(cfg,p_ext_data);

                if ( p_ext_data->trigger_mode == AHM_TRIGGER_DIFFERENT)
                {
                    configure_ramp_slow_delay_duration(cfg,p_ext_data);
                }
                else if (p_ext_data->trigger_mode == AHM_TRIGGER_SIMILAR)
                {
                    configure_ramp_fast_delay_duration(cfg,p_ext_data);
                }

                if (p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID])
                {
                    ahm_init_delta_ramp_and_notify(op_data,
                                                   &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                                   &cfg[AHM_ANC_FILTER_FF_ID]);
                }
                if (p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID])
                {
                    ahm_init_delta_ramp_and_notify(op_data,
                                                   &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID],
                                                   &cfg[AHM_ANC_FILTER_FB_ID]);
                }
                if (p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID])
                {
                    ahm_init_delta_ramp_and_notify(op_data,
                                                   &p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID],
                                                   &cfg[AHM_ANC_FILTER_EC_ID]);
                }
                DEBUG_AHM("OPID: %x, Relevant ramps are initialized for ramp up in Filter Transiton Mode",
                          ext_op_id);
                p_ext_data->ramp_status = AHM_RAMP_UP;
            }
        }
        else if (p_ext_data->ramp_status == AHM_RAMP_UP)
        {
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID])
            {
                p_nominal_filter = &p_nominal->ff;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID],
                                               p_nominal_filter->fine);
            }
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID])
            {
                p_nominal_filter = &p_nominal->fb ;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID] ,
                                               p_nominal_filter->fine);
            }
            if (p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] || p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID])
            {
                p_nominal_filter = &p_nominal->ec ;
                ahm_proc_delta_ramp_and_notify(op_data,
                                               &p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID],
                                               p_nominal_filter->fine);
            }
            L4_DBG_MSG1("OPID: %x, All relevant ramps processing for ramp up started in Filter Switch Mode",
                        ext_op_id);
            if ( (p_ext_data->fine_ramp[AHM_ANC_FILTER_FF_ID]).state == AHM_RAMP_IDLE &&
                             (p_ext_data->fine_ramp[AHM_ANC_FILTER_FB_ID]).state == AHM_RAMP_IDLE &&
                             (p_ext_data->fine_ramp[AHM_ANC_FILTER_EC_ID]).state == AHM_RAMP_IDLE )
            {
                DEBUG_AHM("OPID: %x, All Relevant Ramps for Ramp up Complete in Filter Switch Mode",
                          ext_op_id);
                /* Reset ramp required and coarse gain change flag */
                p_ext_data->ramp_required[AHM_ANC_FILTER_FF_ID] = FALSE;
                p_ext_data->ramp_required[AHM_ANC_FILTER_FB_ID] = FALSE;
                p_ext_data->ramp_required[AHM_ANC_FILTER_EC_ID] = FALSE;
                p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FF_ID] = FALSE;
                p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_FB_ID] = FALSE;
                p_ext_data->coarse_gain_changed[AHM_ANC_FILTER_EC_ID] = FALSE;

                msg.id =AHM_EVENT_ID_FILTER_TRANSITION;
                msg.type = AHM_EVENT_TYPE_CLEAR;
                msg.payload = 0;
                msg.ext_op_id = ext_op_id;
                ahm_send_event_message(op_data, &msg);
                p_ext_data->ramp_status = AHM_RAMP_INIT;
            }
        }
    }
}

void ahm_timer_cb(void *p_data)
{
    OPERATOR_DATA *op_data = (OPERATOR_DATA*)p_data;
    AHM_OP_DATA *p_ext_data = get_instance_data(op_data);
    TIME next_fire_time;

    /* Run a minimal calculation every time. Every <decimation> times run the
     * full processing routine.
     */
    if (p_ext_data->timer_counter <= 0)
    {
        p_ext_data->timer_counter = (int16)(p_ext_data->timer_decimation - 1);
        ahm_process_function(op_data,
                             p_ext_data->samples_per_period,
                             FALSE);
    }
    else
    {
        p_ext_data->timer_counter -= 1;
        ahm_process_function(op_data,
                             p_ext_data->samples_per_period,
                             TRUE);
    }

        /* Next Timer Event */
        next_fire_time = p_ext_data->timer_period;
        p_ext_data->timer_id = timer_schedule_bg_event_in(next_fire_time,
                                                          ahm_timer_cb,
                                                          (void*)op_data);
}

void ahm_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    unsigned samples;

    /* Calculate samples available and updated touched terminals */
    samples = aud_cur_calc_samples(op_data, touched);

    if (samples > 0)
    {
        /* Copy any input mic data to output */
        samples = aud_cur_mic_data_transfer(op_data,
                                            samples,
                                            AHM_TERMINAL_SKIP_MASK);
        /* Reset touched terminals if no data was copied */
        if (samples == 0)
        {
            touched->sources = 0;
            touched->sinks = 0;
        }
        else
        {
            aud_cur_mic_metadata_transfer(op_data, samples);
        }

        ahm_process_function(op_data, samples, FALSE);
    }
    return;
}
