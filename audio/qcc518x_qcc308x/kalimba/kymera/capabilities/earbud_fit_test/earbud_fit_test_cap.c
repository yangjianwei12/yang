/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \ingroup capabilities
 * \file  earbud_fit_test_cap.c
 * \ingroup Earbud Fit Test
 *
 * Earbud Fit Test operator capability.
 *
 */

/****************************************************************************
Include Files
*/

#include "earbud_fit_test_cap.h"

/*****************************************************************************
Private Constant Definitions
*/
#ifdef CAPABILITY_DOWNLOAD_BUILD
#define EARBUD_FIT_TEST_16K_CAP_ID   CAP_ID_DOWNLOAD_EARBUD_FIT_TEST_16K
#else
#define EARBUD_FIT_TEST_16K_CAP_ID   CAP_ID_EARBUD_FIT_TEST_16K
#endif

/* Message handlers */
const handler_lookup_struct eft_handler_table =
{
    eft_create,                   /* OPCMD_CREATE */
    eft_destroy,                  /* OPCMD_DESTROY */
    eft_start,                    /* OPCMD_START */
    base_op_stop,                 /* OPCMD_STOP */
    eft_reset,                    /* OPCMD_RESET */
    eft_connect,                  /* OPCMD_CONNECT */
    eft_disconnect,               /* OPCMD_DISCONNECT */
    eft_buffer_details,           /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,      /* OPCMD_DATA_FORMAT */
    eft_get_sched_info            /* OPCMD_GET_SCHED_INFO */
};

/* Null-terminated operator message handler table */
const opmsg_handler_lookup_table_entry eft_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,             eft_opmsg_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,              eft_opmsg_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,            eft_opmsg_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,              eft_opmsg_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,              eft_opmsg_get_status},
    {OPMSG_COMMON_ID_SET_UCID,                eft_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,       eft_opmsg_get_ps_id},

    {OPMSG_EFT_ID_START_CAPTURE,              eft_opmsg_start_capture},
    {OPMSG_EFT_ID_GET_BIN_POWERS,             eft_opmsg_get_bin_power},
    {OPMSG_EFT_ID_AUTO_FIT_SET_EQU_OP_ID,     eft_opmsg_auto_fit_set_equ_op_id},

    {0, NULL}};

const CAPABILITY_DATA earbud_fit_test_16k_cap_data =
    {
        /* Capability ID */
        EARBUD_FIT_TEST_16K_CAP_ID,
        /* Version information - hi and lo */
        EARBUD_FIT_TEST_EARBUD_FIT_TEST_16K_VERSION_MAJOR, EARBUD_FIT_TEST_CAP_VERSION_MINOR,
        /* Max number of sinks/inputs and sources/outputs */
        2, 0,
        /* Pointer to message handler function table */
        &eft_handler_table,
        /* Pointer to operator message handler function table */
        eft_opmsg_handler_table,
        /* Pointer to data processing function */
        eft_process_data,
        /* Reserved */
        0,
        /* Size of capability-specific per-instance data */
        sizeof(EFT_OP_DATA)
    };

MAP_INSTANCE_DATA(EARBUD_FIT_TEST_16K_CAP_ID, EFT_OP_DATA)

/****************************************************************************
Inline Functions
*/

/**
 * \brief  Get EFT instance data.
 *
 * \param  op_data  Pointer to the operator data.
 *
 * \return  Pointer to extra operator data EFT_OP_DATA.
 */
static inline EFT_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (EFT_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief  Calculate the number of samples to process
 *
 * \param  p_ext_data  Pointer to capability data
 *
 * \return  Number of samples to process
 *
 * If there is less data or space than the default frame size then only that
 * number of samples will be returned.
 *
 */
static int eft_calc_samples_to_process(EFT_OP_DATA *p_ext_data)
{
    int i, amt, min_data;

    /* Return if playback and int mic input terminals are not connected */
    if (p_ext_data->inputs[EFT_PLAYBACK_TERMINAL_ID] == NULL ||
        p_ext_data->inputs[EFT_MIC_INT_TERMINAL_ID] == NULL)
    {
        return INT_MAX;
    }

    min_data = EFT_DEFAULT_FRAME_SIZE;
    /* Calculate the amount of data available */
    for (i = EFT_PLAYBACK_TERMINAL_ID; i <= EFT_MIC_INT_TERMINAL_ID; i++)
    {
        if (p_ext_data->inputs[i] != NULL)
        {
            amt = cbuffer_calc_amount_data_in_words(p_ext_data->inputs[i]);
            if (amt < min_data)
            {
                min_data = amt;
            }
        }
    }

    /* Samples to process determined as minimum data available */
    return min_data;
}

static void eft_clear_event(EFT_EVENT *p_event)
{
       p_event->frame_counter =p_event->set_frames;
       p_event->running = EFT_EVENT_CLEAR;
}

/**
 * \brief  Sent an event trigger message.
 *
 * \param op_data  Address of the EFT operator data.
 * \param  id  ID for the event message
 * \param  payload Payload for the event message
 *
 * \return  bool indicating success
 */
static bool eft_send_event_trigger(OPERATOR_DATA *op_data,
                                    uint16 id, uint16 payload)
{
    unsigned msg_size;
    unsigned *trigger_message = NULL;

    msg_size = OPMSG_UNSOLICITED_EFT_EVENT_TRIGGER_WORD_SIZE;
    trigger_message = xpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        L2_DBG_MSG("Failed to send EFT event message");
        return FALSE;
    }

    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_EFT_EVENT_TRIGGER,
                             ID,
                             id);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_EFT_EVENT_TRIGGER,
                             PAYLOAD,
                             payload);

    L2_DBG_MSG2("EFT Event Sent: [%u, %u]", trigger_message[0],
                trigger_message[1]);
    common_send_unsolicited_message(op_data,
                                    (unsigned)OPMSG_REPLY_ID_EFT_EVENT_TRIGGER,
                                    msg_size,
                                    trigger_message);

    pdelete(trigger_message);

    return TRUE;
}

/**
 * \brief  Initialize events for messaging.
 *
 * \param  op_data  Address of the operator data
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  void.
 */
static void eft_initialize_events(OPERATOR_DATA *op_data, EFT_OP_DATA *p_ext_data)
{
    EARBUD_FIT_TEST_PARAMETERS *p_params = &p_ext_data->eft_cap_params;
    unsigned set_frames;

    set_frames = (p_params->OFFSET_EVENT_GOOD_FIT * EFT_FRAME_RATE);
    set_frames = set_frames >> EFT_TIMER_PARAM_SHIFT;
    L4_DBG_MSG1("EFT Fit Detect Event Initialized at %u frames", set_frames);
    p_ext_data->fit_event_detect.set_frames = set_frames;
    eft_clear_event(&p_ext_data->fit_event_detect);

}


/**
 * \brief  Calcs smoothing coeffs for auto fit.
 *
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool eft_auto_fit_calc_smooth_coeff(EFT_OP_DATA* p_ext_data)
{
    /********************
     * set filter coeff for gain ramping
     ********************/
    if (p_ext_data->auto_fit.gain_smooth_tc_ms == 0) {
        p_ext_data->auto_fit.eq_gain_smooth_coeff = 0; /* contribution of prv.input */
    }
    else if (p_ext_data->auto_fit.gain_smooth_tc_ms * EFT_FRAME_RATE <= 1000) {
        /* Note: inst_ptr->params.eft_config.gain_smooth_tc_ms = 1000 / EFT_FRAME_RATE;
         *       actual lower limit for smoothing (tc_ms>0)
         *       eq_gain_smooth_coeff = 1000 / (gain_smooth_tc_ms * EFT_FRAME_RATE) = 0
         */
        p_ext_data->auto_fit.eq_gain_smooth_coeff = 0;
    }
    else {
        unsigned mant;
        int16_t qexp;
        /* Note: eq_gain_smooth_coeff = 1000 / (gain_smooth_tc_ms * EFT_FRAME_RATE)
         *       result of kal_s32_div_s32_s32_normalized(1000 / den) in Q15
         *       -> For result in Q27: (2^12*1000/den)
         */
        mant = kal_s32_div_s32_s32_normalized(4096000, kal_s32_mult_s16_s16(
            (uint16_t)p_ext_data->auto_fit.gain_smooth_tc_ms, EFT_FRAME_RATE), &qexp);
        p_ext_data->auto_fit.eq_gain_smooth_coeff = EFT_Q27_UNITY - (mant << qexp); /* in Q5.27 */

    }
    /* 1_minus_coeff = (1 -1000 / (gain_smooth_tc_ms * EFT_FRAME_RATE)) considers contribution of current input */
    p_ext_data->auto_fit.eq_gain_smooth_1_minus_coeff =
        EFT_Q27_UNITY - p_ext_data->auto_fit.eq_gain_smooth_coeff;

    L4_DBG_MSG3("EFT: auto fit, smoothing coeffs calculated in Q27 format for gain_smooth_tc_ms=%d: coeff=%d, 1-coeff=%d",
        p_ext_data->auto_fit.gain_smooth_tc_ms,
        p_ext_data->auto_fit.eq_gain_smooth_coeff,
        p_ext_data->auto_fit.eq_gain_smooth_1_minus_coeff
    );

    return TRUE;
}


/**
 * \brief  Set configuration for auto fit.
 *
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool eft_auto_fit_set_config(EFT_OP_DATA* p_ext_data)
{
    unsigned band_id;
    int value, k;
    EFT_CAPTURE* p_shot = &p_ext_data->one_shot;

    /* capture_interval_ms: time per capture interval in ms (capture period) */
    p_ext_data->auto_fit.capture_interval_ms = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_CAPTURE_INTERVAL_MS;

    /* msgs_per_capture_interval: number of gain update messages per capture interval */
    p_ext_data->auto_fit.msgs_per_capture_interval = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_MSGS_PER_CAPTURE_INTERVAL;

    /* gain_smooth_tc_ms: time constant applied to smooth of power estimates */
    p_ext_data->auto_fit.gain_smooth_tc_ms = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_GAIN_SMOOTH_TC_MS;

    /* sensitivity_thrshld_db: min. required REF power level for power ratio estimate */
    if (p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_SENSITIVITY_THRSHLD_DB == EFT_AUTO_FIT_THRSHLD_OFF) {
        p_ext_data->auto_fit.sensitivity_thrshld_lin = 0;
    }
    else {
        /*
         * CONVERT from 1/60dB to LIN Q31 and from Q31 to Q27(+24dB): 60 * 20 * log(16) = 1445,
         * as gain_linear2dB60(..) expects Q31-argument, but gets arg in Q5.27
         */
        value = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_SENSITIVITY_THRSHLD_DB;
        p_ext_data->auto_fit.sensitivity_thrshld_lin = dB60toLinearQ5(60 * value + 1445);
    }

    /* clipping threshold: max. allowed REF power level. Reduce gain when exceeduing this level */
    if (p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_CLIPPING_THRSHLD_DB == EFT_AUTO_FIT_THRSHLD_OFF) {
        p_ext_data->auto_fit.clipping_thrshld_lin = INT32_MAX;
    }
    else {
        /*
         * CONVERT from 1/60dB to LIN Q31 and from Q31 to Q27(+24dB): 60 * 20 * log(16) = 1445,
         * as gain_linear2dB60(..) expects Q31-argument, but gets arg in Q5.27
         */
        value = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_CLIPPING_THRSHLD_DB;
        p_ext_data->auto_fit.clipping_thrshld_lin = dB60toLinearQ5(60 * value + 1445);
    }

    /* num_bands: number of bands applied for auto fitting power measure */
    p_ext_data->auto_fit.num_bands = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_NUM_BANDS;

    /*********
     * band specific settings
     *********/
    /* lowest cutoff frequency of sub band power measure */
    value = p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND1_START_FREQ_HZ;
    p_ext_data->auto_fit.band_start_freq_bin = (value + (EFT_BIN_FREQ_SEPARATION_HZ >> 1)) / EFT_BIN_FREQ_SEPARATION_HZ;

    ParamType* pparam_freq = (ParamType*)(&p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND1_FREQ_HZ);
    ParamType* pparam_gain_offs = (ParamType*)(&p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND1_EQ_GAIN_OFFS_DB);
    for (band_id = 0; band_id < p_ext_data->auto_fit.num_bands; band_id++) {
        p_ext_data->auto_fit.band[band_id].freq_hz = (unsigned) pparam_freq[band_id];
        value = (int) pparam_gain_offs[band_id];
        /* CONVERT from 1/60dB to LIN Q5.27 */
        p_ext_data->auto_fit.band[band_id].eq_gain_offs_lin = dB60toLinearQ5(value);

        /* settings to log output */
        L4_DBG_MSG4("EFT: auto fit, set parameters for Band %d of %d: upper band edge freq=%dHz, "
            "gain offset=%d (Q5.27, lin.)",
            band_id + 1,
            p_ext_data->auto_fit.num_bands,
            p_ext_data->auto_fit.band[band_id].freq_hz,
            p_ext_data->auto_fit.band[band_id].eq_gain_offs_lin);
    }

    /* settings to log output */
    L4_DBG_MSG5("EFT: auto fit, set parameters: capture_interval_ms=%d, "
        "msgs_per_capture_interval=%d, gain_smooth_tc_ms=%d "
        "sensitivity_thrshld_lin=%d, clipping_thrshld_lin=%d ",
        p_ext_data->auto_fit.capture_interval_ms,
        p_ext_data->auto_fit.msgs_per_capture_interval,
        p_ext_data->auto_fit.gain_smooth_tc_ms,
        p_ext_data->auto_fit.sensitivity_thrshld_lin,
        p_ext_data->auto_fit.clipping_thrshld_lin
        );

    /* derivations */
    p_shot->duration_ms = p_ext_data->auto_fit.capture_interval_ms;
    p_shot->duration_frames = p_shot->duration_ms >> EFT_MS_TO_FRAMES_SHIFT;
    if (p_shot->duration_frames == 0)
    {
        L2_DBG_MSG("EFT: auto fit capture interval is 0");
        return FALSE;
    }
    p_ext_data->auto_fit.frame_cnt_msg = 0;
    L4_DBG_MSG4("EFT: auto fit initialized to accumulate over %dms, "
        "means %d frames x %d samples/frame / %d samples/s",
        p_shot->duration_ms, p_shot->duration_frames, EFT_DEFAULT_FRAME_SIZE,
        p_ext_data->sample_rate);

    /* set filter coeff for gain ramping */
    eft_auto_fit_calc_smooth_coeff(p_ext_data);

    /* init bandwise parameters */
    for (k = 0; k < p_ext_data->auto_fit.num_bands; k++) {
        p_ext_data->auto_fit.band[k].eq_gain_lin_prv = EFT_Q27_UNITY; /* 0dB */
        p_ext_data->auto_fit.band[k].num_clippings = 0; /* indicates no clipping */
        p_ext_data->auto_fit.band[k].low_sig_flag = 0;
        p_ext_data->auto_fit.band[k].eq_gain_reduction = EFT_Q27_UNITY; /* no reduction */
        p_ext_data->auto_fit.band[k].pow_ref_avg = 0;
        p_ext_data->auto_fit.band[k].pow_intmic_avg = 0;
    }

    return TRUE;
}


/**
 * \brief  State machine of auto fit.
 *
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool eft_opmsg_auto_fit_set_state(EFT_OP_DATA* p_ext_data)
{
    p_ext_data->auto_fit.freeze = FALSE; /* default */
    p_ext_data->auto_fit.enable = FALSE;
    if (p_ext_data->cur_mode >= EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_ON) {
        p_ext_data->auto_fit.enable = TRUE;
        if (p_ext_data->cur_mode == EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_FROZEN) {
            p_ext_data->auto_fit.freeze = TRUE;
        }
    }

    /* state transitions */
    switch (p_ext_data->auto_fit.state)
    {
    case EFT_AUTO_FIT_STATE_OFF:
        if (p_ext_data->auto_fit.enable == TRUE) {
            p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_SWITCHED_ON;
            p_ext_data->auto_fit.is_ready_to_send_msg = FALSE;
            L4_DBG_MSG("EFT: auto fit, state set to SWITCHED_ON");
        }
        break;
    case EFT_AUTO_FIT_STATE_SWITCHED_ON:
        if (p_ext_data->auto_fit.enable == FALSE) {
            p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_OFF;
            p_ext_data->auto_fit.is_ready_to_send_msg = FALSE;
            L4_DBG_MSG("EFT: auto fit, state set to OFF");
        }
        break;
    case EFT_AUTO_FIT_STATE_ON:
        if (p_ext_data->auto_fit.enable == FALSE) {
            p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_SWITCHED_OFF;
            /* backup new state, keep AUTO_FIT running till last msg is sent, then switch to new state */
            p_ext_data->auto_fit.cur_mode_bak = p_ext_data->cur_mode;
            /* overwrite mode till last msg is sent */
            p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_ON;
            p_ext_data->auto_fit.freeze = FALSE;
            L4_DBG_MSG("EFT: auto fit, state set to SWITCHED_OFF");
        }
        else { // set to SWITCHED_ON to trigger auto fit re-init
            p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_SWITCHED_ON;
            p_ext_data->auto_fit.is_ready_to_send_msg = FALSE;
            L4_DBG_MSG("EFT: auto fit, state set to SWITCHED_ON");
        }
        break;
    case EFT_AUTO_FIT_STATE_SWITCHED_OFF:
        if (p_ext_data->auto_fit.enable == TRUE) {
            p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_SWITCHED_ON;
            p_ext_data->auto_fit.is_ready_to_send_msg = FALSE;
            L4_DBG_MSG("EFT: auto fit, state set to SWITCHED_ON");
        }
        break;
    }

    /* Set re-initialization flag for capability */
    p_ext_data->re_init_flag = TRUE;

    return TRUE;
}


/**
 * \brief  Calculate events for messaging.
 *
 * \param op_data  Address of the EFT operator data.
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */
static bool eft_process_events(OPERATOR_DATA *op_data,
                               EFT_OP_DATA *p_ext_data)
{
    /* Current and previous fit quality */
    bool cur_fit = (p_ext_data->fit_quality == 1);
    bool prev_fit = (p_ext_data->prev_fit_quality == 1);
    EFT_EVENT* fit_event = &p_ext_data->fit_event_detect;
    uint16 payload = EFT_EVENT_PAYLOAD_UNUSED;

    if (cur_fit)
    {
        if (prev_fit) /* Steady state for fit detect event */
        {
            if (fit_event->running == EFT_EVENT_DETECTED)
            {
                fit_event->frame_counter -= 1;
                if (fit_event->frame_counter <= 0)
                {
                    /* Payload 1 indicates good fit */
                    payload = EFT_EVENT_PAYLOAD_GOOD;
                    fit_event->running = EFT_EVENT_SENT;
                }
            }
            else if (fit_event->running == EFT_EVENT_CLEAR)
            {
                fit_event->running == EFT_EVENT_DETECTED;
            }
        }
        else
        {
            fit_event->frame_counter -= 1;
            fit_event->running = EFT_EVENT_DETECTED;
        }
    }
    else
    {
        if (prev_fit) /* Check if good fit message has been sent */
        {
            if (fit_event->running == EFT_EVENT_SENT)
            {
                /* if good fit message previously sent, send bad fit message
                    Payload 0 indicates bad fit */
                payload = EFT_EVENT_PAYLOAD_BAD;
            }
            eft_clear_event(fit_event);
        }
    }

    if (payload != EFT_EVENT_PAYLOAD_UNUSED)
    {
        eft_send_event_trigger(op_data,
                               EFT_EVENT_ID_FIT,
                               payload);
    }
    return TRUE;
}

/**
 * \brief  Free memory allocated during processing
 *
 * \param  p_ext_data  Address of the EFT extra_op_data.
 *
 * \return  boolean indicating success or failure.
 */

static bool eft_proc_destroy(EFT_OP_DATA *p_ext_data)
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

    aanc_afb_destroy(p_ext_data->p_afb_ref);
    pfree(p_ext_data->p_afb_ref);
    aanc_afb_destroy(p_ext_data->p_afb_int);
    pfree(p_ext_data->p_afb_int);

    aanc_fit100_destroy(p_ext_data->p_fit);
    pfree(p_ext_data->p_fit);

    cbuffer_destroy(p_ext_data->p_tmp_ref_ip);
    cbuffer_destroy(p_ext_data->p_tmp_int_ip);

    unload_aanc_handle(p_ext_data->f_handle);

    return TRUE;
}


/****************************************************************************
Capability API Handlers
*/

bool eft_create(OPERATOR_DATA *op_data, void *message_data,
                      unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    int i;
    unsigned *p_default_params; /* Pointer to default params */
    unsigned *p_cap_params;     /* Pointer to capability params */

    /* NB: create is passed a zero-initialized structure so any fields not
     * explicitly initialized are 0.
     */

    L5_DBG_MSG1("EFT Create: p_ext_data at %p", p_ext_data);

    if (!base_op_create_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* TODO: patch functions */

    /* Assume the response to be command FAILED. If we reach the correct
     * termination point in create then change it to STATUS_OK.
     */
    base_op_change_response_status(resp_data, STATUS_CMD_FAILED);

    /* Initialize buffers */
    for (i = 0; i < EFT_MAX_SINKS; i++)
    {
        p_ext_data->inputs[i] = NULL;
    }

    for (i = 0; i < EFT_NUM_METADATA_CHANNELS; i++)
    {
        p_ext_data->metadata_ip[i] = NULL;
    }

    /* Initialize capid and sample rate fields */
    p_ext_data->cap_id = EARBUD_FIT_TEST_16K_CAP_ID;

    p_ext_data->sample_rate = 16000;
    /* Initialize parameters */
    p_default_params = (unsigned*) EARBUD_FIT_TEST_GetDefaults(p_ext_data->cap_id);
    p_cap_params = (unsigned*) &p_ext_data->eft_cap_params;
    if(!cpsInitParameters(&p_ext_data->params_def,
                          p_default_params,
                          p_cap_params,
                          sizeof(EARBUD_FIT_TEST_PARAMETERS)))
    {
       return TRUE;
    }

    /* Initialize system mode */
    p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_FULL;
    p_ext_data->host_mode = EARBUD_FIT_TEST_SYSMODE_FULL;
    p_ext_data->qact_mode = EARBUD_FIT_TEST_SYSMODE_FULL;

    /* Trigger re-initialization at start */
    p_ext_data->re_init_flag = TRUE;

    p_ext_data->p_tmp_ref_ip = cbuffer_create_with_malloc(
                                EFT_INTERNAL_BUFFER_SIZE, BUF_DESC_SW_BUFFER);
    if (p_ext_data->p_tmp_ref_ip == NULL)
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to allocate reference input buffer");
        return FALSE;
    }

    p_ext_data->p_tmp_int_ip = cbuffer_create_with_malloc(
                                EFT_INTERNAL_BUFFER_SIZE, BUF_DESC_SW_BUFFER);
    if (p_ext_data->p_tmp_int_ip == NULL)
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to allocate int mic input buffer");
        return FALSE;
    }

    /* Allocate twiddle factor for AFB */
    if (!math_fft_twiddle_alloc(AANC_FILTER_BANK_WINDOW_SIZE))
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to allocate twiddle factors");
        return FALSE;
    }
    p_ext_data->twiddle_registered = TRUE;

    /* Register scratch memory for AFB & allocate object */
    if (!scratch_register())
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to register scratch memory");
        return FALSE;
    }

    p_ext_data->scratch_registered = TRUE;

    if (!scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM1) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2) ||
        !scratch_reserve(AANC_AFB_SCRATCH_MEMORY, MALLOC_PREFERENCE_DM2))
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to reserve scratch memory");
        return FALSE;
    }

    p_ext_data->p_afb_ref = xzpmalloc(aanc_afb_bytes());
    if (p_ext_data->p_afb_ref == NULL)
    {
        L2_DBG_MSG("EFT failed to allocate AFB ref");
        eft_proc_destroy(p_ext_data);
    }
    aanc_afb_create(p_ext_data->p_afb_ref);

    p_ext_data->p_afb_int = xzpmalloc(aanc_afb_bytes());
    if (p_ext_data->p_afb_int == NULL)
    {
        L2_DBG_MSG("EFT failed to allocate AFB int");
        eft_proc_destroy(p_ext_data);
    }
    aanc_afb_create(p_ext_data->p_afb_int);

    p_ext_data->p_fit = xzpmalloc(aanc_fit100_bytes());
    if (p_ext_data->p_fit == NULL)
    {
        L2_DBG_MSG("EFT failed to allocate fit100");
        eft_proc_destroy(p_ext_data);
    }
    aanc_fit100_create(p_ext_data->p_fit);

    if (!load_aanc_handle(&p_ext_data->f_handle))
    {
        eft_proc_destroy(p_ext_data);
        L2_DBG_MSG("EFT failed to load feature handle");
        return FALSE;
    }

    /* Operator creation was succesful, change respone to STATUS_OK*/
    base_op_change_response_status(resp_data, STATUS_OK);

    L4_DBG_MSG("EFT: Created");
    return TRUE;
}

bool eft_destroy(OPERATOR_DATA *op_data, void *message_data,
                  unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if (!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* TODO: patch functions */

    if (p_ext_data != NULL)
    {
        eft_proc_destroy(p_ext_data);
        L4_DBG_MSG("EFT: Cleanup complete.");
    }

    L4_DBG_MSG("EFT: Destroyed");
    return TRUE;
}

bool eft_start(OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    /* TODO: patch functions */

    /* Start with the assumption that we fail and change later if we succeed */
    if (!base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, resp_data))
    {
        return FALSE;
    }

    /* Check that we have a minimum number of terminals connected */
    if (p_ext_data->inputs[EFT_PLAYBACK_TERMINAL_ID] == NULL ||
        p_ext_data->inputs[EFT_MIC_INT_TERMINAL_ID] == NULL)
    {
        L4_DBG_MSG("EFT start failure: inputs not connected");
        return TRUE;
    }

    /* Set reinitialization flags to ensure first run behavior */
    p_ext_data->re_init_flag = TRUE;

    /* All good */
    base_op_change_response_status(resp_data, STATUS_OK);

    L4_DBG_MSG("EFT Started");
    return TRUE;
}

bool eft_reset(OPERATOR_DATA *op_data, void *message_data,
                     unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);

    if (!base_op_reset(op_data, message_data, response_id, resp_data))
    {
        return FALSE;
    }

    p_ext_data->re_init_flag = TRUE;

    L4_DBG_MSG("EFT: Reset");
    return TRUE;
}

bool eft_connect(OPERATOR_DATA *op_data, void *message_data,
                       unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned terminal_id, terminal_num;
    tCbuffer* pterminal_buf;

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, resp_data))
    {
        return FALSE;
    }

    /* Only sink terminal can be connected */
    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;
    L4_DBG_MSG1("EFT connect: sink terminal %u", terminal_num);

    /* Can't use invalid ID */
    if (terminal_num >= EFT_MAX_SINKS)
    {
        /* invalid terminal id */
        L4_DBG_MSG1("EFT connect failed: invalid terminal %u", terminal_num);
        base_op_change_response_status(resp_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    /* Can't connect if already connected */
    if (p_ext_data->inputs[terminal_num] != NULL)
    {
        L4_DBG_MSG1("EFT connect failed: terminal %u already connected",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    pterminal_buf = OPMGR_GET_OP_CONNECT_BUFFER(message_data);
    p_ext_data->inputs[terminal_num] = pterminal_buf;

    if (p_ext_data->metadata_ip[terminal_num] == NULL &&
        buff_has_metadata(pterminal_buf))
    {
        p_ext_data->metadata_ip[terminal_num] = pterminal_buf;
    }

    return TRUE;
}

bool eft_disconnect(OPERATOR_DATA *op_data, void *message_data,
                          unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned terminal_id, terminal_num;

    /* Create the response. If there aren't sufficient resources for this fail
     * early. */
    if (!base_op_build_std_response_ex(op_data, STATUS_OK, resp_data))
    {
        return FALSE;
    }

    /* Only sink terminal can be disconnected */
    terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    terminal_num = terminal_id & TERMINAL_NUM_MASK;
    L4_DBG_MSG1("EFT disconnect: sink terminal %u", terminal_num);

    /* Can't use invalid ID */
    if (terminal_num >= EFT_MAX_SINKS)
    {
        /* invalid terminal id */
        L4_DBG_MSG1("EFT disconnect failed: invalid terminal %u",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    /* Can't disconnect if not connected */
    if (p_ext_data->inputs[terminal_num] == NULL)
    {
        L4_DBG_MSG1("EFT disconnect failed: terminal %u not connected",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_CMD_FAILED);
        return TRUE;
    }

    /*  Disconnect the existing metadata and input channel. */
    p_ext_data->metadata_ip[terminal_num] = NULL;
    p_ext_data->inputs[terminal_num] = NULL;

    return TRUE;
}

bool eft_buffer_details(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);
    /* Response pointer */
    OP_BUF_DETAILS_RSP *p_resp;

#ifndef DISABLE_IN_PLACE
    unsigned terminal_num = terminal_id & TERMINAL_NUM_MASK;
#endif

    if (!base_op_buffer_details_lite(op_data, resp_data))
    {
        return FALSE;
    }

    /* Response pointer */
    p_resp = (OP_BUF_DETAILS_RSP*) *resp_data;

#ifdef DISABLE_IN_PLACE
    p_resp->runs_in_place = FALSE;
    p_resp->b.buffer_size = EFT_DEFAULT_BUFFER_SIZE;
#else

    /* Can't use invalid ID */
    if (terminal_num >= EFT_MAX_SINKS)
    {
        /* invalid terminal id */
        L4_DBG_MSG1("EFT buffer details failed: invalid terminal %d",
                    terminal_num);
        base_op_change_response_status(resp_data, STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }
    /* Operator does not run in place */
    p_resp->runs_in_place = FALSE;
    p_resp->b.buffer_size = EFT_DEFAULT_BUFFER_SIZE;
    p_resp->supports_metadata = TRUE;

    if (terminal_num == EFT_PLAYBACK_TERMINAL_ID)
    {
        p_resp->metadata_buffer = p_ext_data->metadata_ip[EFT_METADATA_PLAYBACK_ID];
    }
    else
    {
        p_resp->metadata_buffer = p_ext_data->metadata_ip[EFT_METADATA_INT_ID];
    }
#endif /* DISABLE_IN_PLACE */
    return TRUE;
}

bool eft_get_sched_info(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *response_id, void **resp_data)
{
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED,
                                             resp_data);
    }

    *resp_data = resp;
    resp->block_size = EFT_DEFAULT_BLOCK_SIZE;

    return TRUE;
}

/****************************************************************************
Opmsg handlers
*/
bool eft_opmsg_set_control(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *resp_length,
                                 OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned i;
    unsigned num_controls;
    bool mode_update = FALSE;

    OPMSG_RESULT_STATES result = OPMSG_RESULT_STATES_NORMAL_STATE;

    if(!cps_control_setup(message_data, resp_length, resp_data, &num_controls))
    {
       return FALSE;
    }

    /* Iterate through the control messages looking for mode and gain override
     * messages */
    for (i=0; i<num_controls; i++)
    {
        unsigned ctrl_value, ctrl_id;
        CPS_CONTROL_SOURCE  ctrl_src;

        ctrl_id = cps_control_get(message_data, i, &ctrl_value, &ctrl_src);

        /* Mode override */
        if (ctrl_id == OPMSG_CONTROL_MODE_ID)
        {
            /* Check for valid mode */
            ctrl_value &= EFT_SYSMODE_MASK;
            if (ctrl_value >= EARBUD_FIT_TEST_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            eft_initialize_events(op_data, p_ext_data);

            /* Gain update logic */
            switch (ctrl_value)
            {
                case EARBUD_FIT_TEST_SYSMODE_STANDBY:
                    /* Set current mode to Standby */
                    p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_STANDBY;
                    break;
                case EARBUD_FIT_TEST_SYSMODE_FULL:
                    /* Set current mode to Full */
                    p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_FULL;
                    break;

                case EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_ON:
                    /* Set current mode to AUTO_FIT_ON */
                    p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_ON;
                    break;

                case EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_FROZEN:
                    /* Set current mode to AUTO_FIT_FROZEN */
                    p_ext_data->cur_mode = EARBUD_FIT_TEST_SYSMODE_AUTO_FIT_FROZEN;
                    break;

                default:
                    /* Handled by early exit above */
                    break;
            }

            mode_update = TRUE; /* indicate that mode has changed */

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
                    p_ext_data->ovr_control |= EARBUD_FIT_TEST_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    p_ext_data->ovr_control &= EFT_OVERRIDE_MODE_MASK;
                }
            }

        }
        /* In/Out of Ear control */
        else if (ctrl_id == EARBUD_FIT_TEST_CONSTANT_IN_OUT_EAR_CTRL)
        {
            ctrl_value &= 0x01;
            p_ext_data->in_out_status = ctrl_value;
        }
        else
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
    }

    /* Set current operating mode based on override */
    if ((p_ext_data->ovr_control & EARBUD_FIT_TEST_CONTROL_MODE_OVERRIDE) != 0)
    {
        p_ext_data->cur_mode = p_ext_data->qact_mode;
    }
    else
    {
        p_ext_data->cur_mode = p_ext_data->host_mode;
    }

    /* set auto fit state */
    if (mode_update == TRUE) {
        eft_opmsg_auto_fit_set_state(p_ext_data);
    }

    cps_response_set_result(resp_data, result);

    return TRUE;
}

bool eft_opmsg_get_params(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    return cpsGetParameterMsgHandler(&p_ext_data->params_def, message_data,
                                     resp_length, resp_data);
}

bool eft_opmsg_get_defaults(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *resp_length,
                                  OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    return cpsGetDefaultsMsgHandler(&p_ext_data->params_def, message_data,
                                    resp_length, resp_data);
}

bool eft_opmsg_set_params(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    bool success;
    /* patch_fn TODO */

    success = cpsSetParameterMsgHandler(&p_ext_data->params_def, message_data,
                                       resp_length, resp_data);

    if (success)
    {
        /* Set re-initialization flag for capability */
        p_ext_data->re_init_flag = TRUE;
    }
    else
    {
        L2_DBG_MSG("EFT Set Parameters Failed");
    }

    return success;
}

bool eft_opmsg_get_status(OPERATOR_DATA *op_data, void *message_data,
                                unsigned *resp_length,
                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    /* TODO: patch functions */
    int i;

    /* Build the response */
    unsigned *resp = NULL;
    if(!common_obpm_status_helper(message_data, resp_length, resp_data,
                                  sizeof(EARBUD_FIT_TEST_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp)
    {
        EARBUD_FIT_TEST_STATISTICS stats;
        EARBUD_FIT_TEST_STATISTICS *pstats = &stats;
        ParamType *pparam = (ParamType*)pstats;

        pstats->OFFSET_CUR_MODE         = p_ext_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL      = p_ext_data->ovr_control;
        pstats->OFFSET_IN_OUT_EAR_CTRL  = p_ext_data->in_out_status;

        pstats->OFFSET_FIT_QUALITY_FLAG = p_ext_data->fit_quality;

        pstats->OFFSET_FIT_EVENT        = p_ext_data->fit_event_detect.running;
        pstats->OFFSET_FIT_TIMER        = (p_ext_data->fit_event_detect.frame_counter
                                           << EFT_TIMER_PARAM_SHIFT)/EFT_FRAME_RATE;
        pstats->OFFSET_POWER_REF        = p_ext_data->p_fit->pwr_reference;
        pstats->OFFSET_POWER_INT_MIC    = p_ext_data->p_fit->pwr_internal;
        pstats->OFFSET_POWER_RATIO      = p_ext_data->p_fit->pwr_ratio;

        for (i=0; i<EFT_N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if ((EFT_N_STAT % 2) == 1) // last one
        {
            cpsPack1Word(pparam[EFT_N_STAT-1], resp);
        }
    }

    return TRUE;
}

bool ups_params_eft(void* instance_data, PS_KEY_TYPE key,
                          PERSISTENCE_RANK rank, uint16 length,
                          unsigned* data, STATUS_KYMERA status,
                          uint16 extra_status_info)
{
    OPERATOR_DATA *op_data = (OPERATOR_DATA*) instance_data;
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);

    cpsSetParameterFromPsStore(&p_ext_data->params_def, length, data, status);

    /* Set the reinitialization flag after setting the parameters */
    p_ext_data->re_init_flag = TRUE;

    return TRUE;
}

bool eft_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data,
                              unsigned *resp_length,
                              OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    PS_KEY_TYPE key;
    bool retval;

    retval = cpsSetUcidMsgHandler(&p_ext_data->params_def, message_data,
                                  resp_length, resp_data);
    L5_DBG_MSG1("EFT cpsSetUcidMsgHandler Return Value %u", retval);
    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(p_ext_data->cap_id,
                                         p_ext_data->params_def.ucid,
                                         OPMSG_P_STORE_PARAMETER_SUB_ID);

    ps_entry_read((void*)op_data, key, PERSIST_ANY, ups_params_eft);

    L5_DBG_MSG1("EFT UCID Set to %u", p_ext_data->params_def.ucid);

    p_ext_data->re_init_flag = TRUE;

    return retval;
}

bool eft_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data,
                               unsigned *resp_length,
                               OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    return cpsGetUcidMsgHandler(&p_ext_data->params_def,
                                p_ext_data->cap_id,
                                message_data,
                                resp_length,
                                resp_data);
}

/****************************************************************************
Custom opmsg handlers
*/

bool eft_opmsg_start_capture(OPERATOR_DATA *op_data,
                             void *message_data,
                             unsigned *resp_length,
                             OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    EFT_CAPTURE *p_shot = &p_ext_data->one_shot;

    *resp_length = sizeof(OP_STD_RSP)/sizeof(int);

    if (p_shot->start)
    {
        L2_DBG_MSG("EFT: capture failed to start - already running");
        return FALSE;
    }

    p_shot->duration_ms = OPMSG_FIELD_GET(
        message_data,
        OPMSG_EFT_START_CAPTURE,
        DURATION_MS
    );

    p_shot->duration_frames = p_shot->duration_ms >> EFT_MS_TO_FRAMES_SHIFT;

    if (p_shot->duration_frames == 0)
    {
        L2_DBG_MSG("EFT: capture failed to start - 0 frames");
        return FALSE;
    }

    p_shot->frame_counter = p_shot->duration_frames;
    p_shot->start = TRUE;

    return TRUE;
}

bool eft_opmsg_get_bin_power(OPERATOR_DATA *op_data,
                             void *message_data,
                             unsigned *resp_length,
                             OP_OPMSG_RSP_PAYLOAD **resp_data)
{
#ifdef EFT_MESSAGE_TEST
    unsigned debug_shift;
#else
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    int *p_bin_source;
#endif
    unsigned *p_resp;
    unsigned msg_id, value, offset;
    EFT_BIN_SECTION section;
    EFT_BIN_SOURCE source;
    int i;

    /* Allocate response payload */
    *resp_length = EFT_BINS_RESP_SIZE;

    p_resp = xzpnewn(EFT_BINS_RESP_SIZE, unsigned);
    if (p_resp == NULL)
    {
        return FALSE;
    }

    /* Echo message ID and section */
    msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    OPMSG_CREATION_FIELD_SET32(p_resp,
                               OPMSG_GET_AHM_GAINS_RESP,
                               MESSAGE_ID,
                               msg_id);

    section = (EFT_BIN_SECTION)OPMSG_FIELD_GET(
        message_data,
        OPMSG_EFT_GET_BIN_POWER,
        SECTION
    );

    OPMSG_CREATION_FIELD_SET(
        p_resp,
        OPMSG_EFT_GET_BIN_POWER_RESP,
        SECTION,
        (uint16)section
    );

    source = (EFT_BIN_SOURCE)OPMSG_FIELD_GET(
        message_data,
        OPMSG_EFT_GET_BIN_POWER,
        SOURCE
    );

    OPMSG_CREATION_FIELD_SET(
        p_resp,
        OPMSG_EFT_GET_BIN_POWER_RESP,
        SOURCE,
        (uint16)source
    );

#ifdef EFT_MESSAGE_TEST
    switch (source)
    {
        case EFT_BIN_SOURCE_REF:
            debug_shift = 20;
            break;
        case EFT_BIN_SOURCE_MIC:
        default:
            debug_shift = 24;
            break;
    }
#else
switch (source)
{
case EFT_BIN_SOURCE_REF:
    p_bin_source = p_ext_data->p_fit->pwr_ref_bins;
    break;
case EFT_BIN_SOURCE_MIC:
default:
    p_bin_source = p_ext_data->p_fit->pwr_mic_bins;
    break;
}
#endif

/* Populate the return payload */
for (i = 0; i < EFT_BINS_PER_SECTION; i++)
{
    offset = i + EFT_BINS_PER_SECTION * section;
#ifdef EFT_MESSAGE_TEST
    value = ((offset << debug_shift)) + offset;
#else
    value = p_bin_source[offset];
#endif
    OPMSG_CREATION_FIELD_SET_FROM_OFFSET(
        p_resp,
        OPMSG_EFT_GET_BIN_POWER_RESP,
        POWERS,
        2 * i,
        value >> 16
    );
    OPMSG_CREATION_FIELD_SET_FROM_OFFSET(
        p_resp,
        OPMSG_EFT_GET_BIN_POWER_RESP,
        POWERS,
        2 * i + 1,
        value & 0xFFFF
    );
}

*resp_data = (OP_OPMSG_RSP_PAYLOAD*)p_resp;

return TRUE;
}


bool eft_opmsg_auto_fit_set_equ_op_id(OPERATOR_DATA* op_data,
    void* message_data,
    unsigned* resp_length,
    OP_OPMSG_RSP_PAYLOAD** resp_data)
{
    EFT_OP_DATA* p_ext_data = get_instance_data(op_data);
    *resp_length = sizeof(OP_STD_RSP) / sizeof(int);

    p_ext_data->auto_fit.equ_op_id = OPMSG_FIELD_GET(
        message_data,
        OPMSG_EFT_AUTO_FIT_SET_EQU,
        OP_ID
    );
    L4_DBG_MSG1("EFT: auto fit, equalizer operator id set to 0x%x)",
        p_ext_data->auto_fit.equ_op_id);

    return TRUE;
}


static bool auto_fit_send_gain_update_to_equ_callback(CONNECTION_LINK con_id,
    STATUS_KYMERA status,
    EXT_OP_ID op_id,
    unsigned num_resp_params,
    unsigned* resp_params)
{
    if (status != ACCMD_STATUS_OK)
    {
        L2_DBG_MSG1("EFT: auto_fit, send_gain_update to equ failed: status=%d", status);
        return FALSE;
    }

    return TRUE;
}


static bool auto_fit_send_gain_update_to_equ(EFT_OP_DATA* p_ext_data)
{
    unsigned k;
    int32_t new_gain_db60[EFT_AUTO_FIT_NUM_BANDS_MAX];

    /* send gain update message to equ
     * (4 + NUM_BANDS*2) WORD16: 4 WORD16 HEADER + 2 WORD16 per BAND
     */
    unsigned params[4 + 2 * EFT_AUTO_FIT_NUM_BANDS_MAX] = { 0 };

    /* do not send message if gain update is frozen */
    if (p_ext_data->auto_fit.freeze == TRUE) {
        return TRUE;
    }

    /* msg header */
    params[0] = OPMSG_COMMON_ID_SET_PARAMS;
    params[1] = (1<<12) | 0x1; // Number of blocks: 1
    params[2] = EFT_EQU_OFFSET_BAND1_GAIN_DB; /* OFFS. 1st gain parameter(s) in GEQ capability */
    params[3] = p_ext_data->auto_fit.num_bands; /* PAYLOAD LENGTH in num of WORD32(2xWORD16) */
    L4_DBG_MSG4("EFT: auto_fit, send gain update message (header) to equ: "
        "msgid=%d, num_blocks=%d, offset=%d, len_payload=%d",
        params[0], params[1], params[2], params[3]);
    /* payload */
    for (k = 0; k < p_ext_data->auto_fit.num_bands; k++) {
        /*
         * result_dB60 = gain_linear2dB60(inp_Q31),
         * means 2^31-1<->0db60<->0dB, 2^30<->-360dB60<->-6dB
         */
        if (p_ext_data->auto_fit.eq_gain_smooth_coeff > 0) {
            new_gain_db60[k] = gain_linear2dB60(p_ext_data->auto_fit.band[k].eq_gain_lin_smoothed);
        }
        else {
            new_gain_db60[k] = gain_linear2dB60(p_ext_data->auto_fit.band[k].eq_gain_lin);
        }

        /*
         * Convert from Q31 to Q27(+24dB): 60 * 20 * log(16) = 1445,
         * as gain_linear2dB60(..) expects Q31-argument, but gets arg in Q5.27
         */
        new_gain_db60[k] += 1445;

        /* limit gain output */
        if (new_gain_db60[k] < ((int)p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND_GAIN_MIN_DB))
        {
            new_gain_db60[k] = (int)p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND_GAIN_MIN_DB;
        }
        if (new_gain_db60[k] > ((int)p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND_GAIN_MAX_DB))
        {
            new_gain_db60[k] = (int)p_ext_data->eft_cap_params.OFFSET_AUTO_FIT_BAND_GAIN_MAX_DB;
        }

        params[2 * k + 4] = (unsigned)((new_gain_db60[k]) >> 16);     // MSB16
        params[2 * k + 5] = (unsigned)((new_gain_db60[k]) & 0xffff);  // LSB16
        L4_DBG_MSG3("EFT: auto_fit, send gain update message (payload) to equ: "
            "band%d: gain_db60_msb16=%d, gain_db60_lsb16=%d",
            k, params[2*k + 4], params[2*k + 5]);
    }
    opmgr_operator_message(ADAPTOR_INTERNAL,
        p_ext_data->auto_fit.equ_op_id,
        4 + 2*p_ext_data->auto_fit.num_bands,
        params,
        auto_fit_send_gain_update_to_equ_callback);

    return TRUE;
}


static bool auto_fit_calc_gain_update(EFT_OP_DATA* p_ext_data)
{
    unsigned freq_bin, band, norm_shift, num_bin;
    int mant, gain_lin;
    int16_t qexp;
    int power_ref[EFT_AUTO_FIT_NUM_BANDS_MAX] = { 0 };
    int power_intmic[EFT_AUTO_FIT_NUM_BANDS_MAX] = { 0 };
    int* power_ref_all_bins = (int*) p_ext_data->p_fit->pwr_ref_bins;
    int* power_intmic_all_bins = (int*) p_ext_data->p_fit->pwr_mic_bins;

    /*
     *  64 freq bins: 0..8kHz
     *-----------------------------------------------------------------------------------
     * freq_bins                  0      1      2      3      4      5      6      7   ...
     * center frequency [Hz]      0     125    250    375    500    625    750    875  ...
     *-----------------------------------------------------------------------------------
     */
    /* only debugging purposes
    for (freq_bin = 0; freq_bin < EFT_DEFAULT_FRAME_SIZE; freq_bin++)
    {
        L0_DBG_MSG4("DEBUG: [bin_%2d], %4dHz: power_ref_all_bins=%10d, power_intmic_all_bins=%10d",
            freq_bin,
            EFT_BIN_FREQ_SEPARATION_HZ * freq_bin,
            power_ref_all_bins[freq_bin],
            power_intmic_all_bins[freq_bin]);
    }
    */

    //--------------
    // mapping: eft bins to equalizer bands
    //--------------
    // Example (4-Band, upper band_freq_hz=250,500,990,1900):
    //      BAND_0, avg:  125Hz, 250Hz
    //      BAND_1, avg:  250Hz, 375Hz,   500Hz
    //      BAND_2, avg:  500Hz, 625Hz,   750Hz,  875Hz
    //      BAND_3: avg: 1000Hz, 1125Hz, 1250Hz, 1375Hz, 1500Hz, 1625Hz, 1750Hz, 1875Hz

    freq_bin = p_ext_data->auto_fit.band_start_freq_bin; /* ignore DC bin or multiple low freq bins */
    for (band = 0; band < p_ext_data->auto_fit.num_bands; band++)
    {
        /* only debugging purposes
        L0_DBG_MSG2("EFT: auto fit capture result band%d, upper edge freq=%dHz",
            band, p_ext_data->auto_fit.band[band].freq_hz);
        */
        norm_shift = 0;
        num_bin = 0;
        while (freq_bin * EFT_BIN_FREQ_SEPARATION_HZ <= p_ext_data->auto_fit.band[band].freq_hz)
        {
            power_ref[band] += power_ref_all_bins[freq_bin]>>norm_shift;
            power_intmic[band] += power_intmic_all_bins[freq_bin]>>norm_shift;

            if ((power_ref[band] > EFT_AUTO_FIT_BAND_POWER_MAX)
                || (power_intmic[band] > EFT_AUTO_FIT_BAND_POWER_MAX))
            {
                norm_shift++;
                power_ref[band] >>= 1;
                power_intmic[band] >>= 1;
            }

            num_bin++;
            /* if bin freq. == band edge freq. -> consider bin for power of next band as well */
            if ((freq_bin * EFT_BIN_FREQ_SEPARATION_HZ) == p_ext_data->auto_fit.band[band].freq_hz) {
                break;
            }
            freq_bin++;
        }

        /* averaged bin power per band */
        p_ext_data->auto_fit.band[band].pow_ref_avg = kal_s32_saturate_s64((((int64_t) power_ref[band]) << norm_shift)/num_bin);
        p_ext_data->auto_fit.band[band].pow_intmic_avg = kal_s32_saturate_s64((((int64_t)power_intmic[band]) << norm_shift)/num_bin);

        /* no signal or refpower below sensitivity in current band */
        if ((p_ext_data->auto_fit.band[band].pow_ref_avg < p_ext_data->auto_fit.sensitivity_thrshld_lin)
            || (power_intmic[band] == 0)) {
            p_ext_data->auto_fit.band[band].low_sig_flag = 1;
            continue; /* don't calculate power ratio for this band and proceed with next band */
        }
        p_ext_data->auto_fit.band[band].low_sig_flag = 0;

        mant = kal_s32_div_s32_s32_normalized(power_ref[band], power_intmic[band], &qexp);
        gain_lin = mant << qexp;
        gain_lin = kal_s32_shl_s32_sat(gain_lin, 12);  /* convert Q15 --> Q27 */

        /* band specific equ gain offset : eq_gain_lin *= eq_gain_offs_lin, result in Q5.27 */
        p_ext_data->auto_fit.band[band].eq_gain_lin = kal_s32_saturate_s64(kal_s64_mult_s32_s32_shift(
            gain_lin,                                         /* in Q27 */
            p_ext_data->auto_fit.band[band].eq_gain_offs_lin, /* in Q27 */
            5));                                              /* to get result in Q27: 32-27=5 */

        /* only debugging purposes
        L0_DBG_MSG5("EFT: auto fit capture result for power in eft sub bands (1..4): "
            "power_ref=%10d / power_intmic=%10d = gain_lin=%10d (Q27) --offs--> eq_gain_lin=%10d (Q27); (eq_gain_offs_lin=%d (Q27))",
            power_ref[band],
            power_intmic[band],
            gain_lin,
            p_ext_data->auto_fit.band[band].eq_gain_lin,
            p_ext_data->auto_fit.band[band].eq_gain_offs_lin);
        */
    }

    return TRUE;
}


static bool auto_fit_one_frame_evaluation(EFT_OP_DATA* p_ext_data)
{
    unsigned freq_bin, band, norm_shift, num_bin;
    FIT100* p_fit = (FIT100*)p_ext_data->p_fit;
    int* power_ref_all_bin_abs = (int*) p_fit->autofit_one_frame.bin_abs;
    int power_ref;
    int pow_ref_band_avg;

    /****************
     * bandwise signal levale evaluation
     ****************/
    freq_bin = p_ext_data->auto_fit.band_start_freq_bin; /* ignore DC bin or multiple low freq bins */
    for (band = 0; band < p_ext_data->auto_fit.num_bands; band++)
    {
        /* only debugging purposes
        L0_DBG_MSG2("EFT: auto fit capture result band%d, upper edge freq=%dHz",
            band, p_ext_data->auto_fit.band[band].freq_hz);
        */
        power_ref = 0;
        norm_shift = 0;
        num_bin = 0;
        while (freq_bin * EFT_BIN_FREQ_SEPARATION_HZ <= p_ext_data->auto_fit.band[band].freq_hz)
        {
            power_ref += power_ref_all_bin_abs[freq_bin] >> norm_shift;

            if (power_ref > EFT_AUTO_FIT_BAND_POWER_MAX)
            {
                norm_shift++;
                power_ref >>= 1;
            }

            num_bin++;
            /* if bin freq. == band edge freq. -> consider bin for power of next band as well */
            if ((freq_bin * EFT_BIN_FREQ_SEPARATION_HZ) == p_ext_data->auto_fit.band[band].freq_hz) {
                break;
            }
            freq_bin++;
        }

        /* averaged bin power per band */
        pow_ref_band_avg = kal_s32_saturate_s64((((int64_t)power_ref) << norm_shift) / num_bin);

        /* ref power exceeds clipping threshold in current band */
        if (pow_ref_band_avg > p_ext_data->auto_fit.clipping_thrshld_lin) {
            /* Potential clipping in GEQ: reduce related sub band gain on GEQ with upcoming message */
            L4_DBG_MSG3("EFT: auto fit, clipping detected for current frame in band%d: pow_ref_band_avg=%d > clipping_thrshld_lin=%d",
                band, pow_ref_band_avg, p_ext_data->auto_fit.clipping_thrshld_lin);

            p_ext_data->auto_fit.band[band].num_clippings++;
        }
    }

    return TRUE;
}


static bool auto_fit_clipping_detection(EFT_OP_DATA* p_ext_data)
{
    unsigned band;
    unsigned gain_reduction_min = (EFT_Q27_UNITY >> 3); /* -18dB */
    int gain_lin, delta;
    int tc_shift = 6; /* time constant */

    /* gain reduction factor due to clipping */
    for (band = 0; band < p_ext_data->auto_fit.num_bands; band++)
    {
        /* reduce gain_lin in case of clipping within frames since last message */
        if (p_ext_data->auto_fit.band[band].num_clippings > 0)
        { /* clipping detected since last message -> decrease gain */
            delta = (p_ext_data->auto_fit.band[band].eq_gain_reduction / p_ext_data->auto_fit.msgs_per_capture_interval) >> tc_shift;
            p_ext_data->auto_fit.band[band].eq_gain_reduction -= p_ext_data->auto_fit.band[band].num_clippings*delta;
            /* Note: "num_clippings" applied to incr. stepsize */

            /* lower reduction limit */
            if (p_ext_data->auto_fit.band[band].eq_gain_reduction < gain_reduction_min) {
                p_ext_data->auto_fit.band[band].eq_gain_reduction = gain_reduction_min;
            }
            L4_DBG_MSG2("EFT: auto fit, gain reduction in band%d due to clipping. clipping counter=%d",
                band, p_ext_data->auto_fit.band[band].num_clippings);
        }
        else { /* no clipping detected since last message */
            /* increase gain as it was reduced before */
            if (p_ext_data->auto_fit.band[band].eq_gain_reduction < EFT_Q27_UNITY) {
                delta = (p_ext_data->auto_fit.band[band].eq_gain_reduction / p_ext_data->auto_fit.msgs_per_capture_interval) >> tc_shift;
                p_ext_data->auto_fit.band[band].eq_gain_reduction += delta;

                /* upper reduction limit (= no reduction) */
                if (p_ext_data->auto_fit.band[band].eq_gain_reduction > EFT_Q27_UNITY) {
                    p_ext_data->auto_fit.band[band].eq_gain_reduction = EFT_Q27_UNITY;
                }
            }
        }
        /* reset clipping indicator */
        p_ext_data->auto_fit.band[band].num_clippings = 0;

        /* apply gain reduction */
        if (p_ext_data->auto_fit.band[band].eq_gain_reduction < EFT_Q27_UNITY) {
            if (p_ext_data->auto_fit.eq_gain_smooth_coeff > 0) {
                gain_lin = p_ext_data->auto_fit.band[band].eq_gain_lin_smoothed;
                /* band specific equ gain reduction: eq_gain_lin *= eq_gain_reduction, result in Q5.27 */
                p_ext_data->auto_fit.band[band].eq_gain_lin_smoothed = kal_s32_saturate_s64(kal_s64_mult_s32_s32_shift(
                    gain_lin,                                          /* in Q27 */
                    p_ext_data->auto_fit.band[band].eq_gain_reduction, /* in Q27 */
                    5));
            }
            else {
                gain_lin = p_ext_data->auto_fit.band[band].eq_gain_lin;
                /* band specific equ gain reduction: eq_gain_lin *= eq_gain_reduction, result in Q5.27 */
                p_ext_data->auto_fit.band[band].eq_gain_lin = kal_s32_saturate_s64(kal_s64_mult_s32_s32_shift(
                    gain_lin,                                          /* in Q27 */
                    p_ext_data->auto_fit.band[band].eq_gain_reduction, /* in Q27 */
                    5));
            }
        }
    }

    return TRUE;
}


static bool auto_fit_gain_ramping(EFT_OP_DATA* p_ext_data)
{
    unsigned k;
    /* Note: LP filter: 1st order
     *    out = (1.0f - coeff) * in + z1;
     *     z1 = coeff * out;
     */
    for (k = 0; k < p_ext_data->auto_fit.num_bands; k++) {
        p_ext_data->auto_fit.band[k].eq_gain_lin_smoothed = kal_s32_saturate_s64(kal_s64_mult_s32_s32_shift(
            p_ext_data->auto_fit.band[k].eq_gain_lin, p_ext_data->auto_fit.eq_gain_smooth_1_minus_coeff, 5)
            + p_ext_data->auto_fit.band[k].eq_gain_lin_prv);

        p_ext_data->auto_fit.band[k].eq_gain_lin_prv = kal_s32_saturate_s64(kal_s64_mult_s32_s32_shift(
            p_ext_data->auto_fit.band[k].eq_gain_lin_smoothed, /* in Q27 */
            p_ext_data->auto_fit.eq_gain_smooth_coeff,         /* in Q27 */
            5));                                               /* to get result in Q27: 32-27=5 */
    }

    return TRUE;
}


static bool eft_auto_fit_ctrl(EFT_OP_DATA* p_ext_data)
{
    EFT_CAPTURE* p_shot = &p_ext_data->one_shot;
    int k;

    /**********
     * Called on capture rate
     * availability of new power estimate: triggers ready flag (to send gain update messages to equalizer)
     **********/
    if ((p_ext_data->auto_fit.state > EFT_AUTO_FIT_STATE_SWITCHED_ON) && (p_shot->start == FALSE))
    {
        L4_DBG_MSG("EFT: auto fit, new power estimate available...");
        p_ext_data->auto_fit.is_ready_to_send_msg = TRUE;
        auto_fit_calc_gain_update(p_ext_data);
    }

    /**********
    * Called on frame rate
    **********/
    /* framewise measures from fit 100 library, call upon each frame */
    // compute another rms to consider rms for single frame -> for fast attack
    aanc_fit100_autofit_one_shot_process(
        p_ext_data->f_handle,
        p_ext_data->p_fit
    );
    /* framewise evaluation of framewise measures */
    auto_fit_one_frame_evaluation(p_ext_data);

    if (p_ext_data->auto_fit.is_ready_to_send_msg == TRUE)
    {
        /* gain smoothing on frame rate, start as as soon as 1st measure is available */
        if (p_ext_data->auto_fit.eq_gain_smooth_coeff > 0) {
            auto_fit_gain_ramping(p_ext_data);
        }

        /**********
        * Called on message rate
        **********/
       /* check whether its time to sent new gain update message on multiple of capture rate */
       if ((p_ext_data->auto_fit.frame_cnt_msg <= 0)
           || ((p_ext_data->auto_fit.state == EFT_AUTO_FIT_STATE_SWITCHED_OFF)
               && (p_ext_data->auto_fit.msgs_per_capture_interval == 0))) {
           /* last message before disabling of auto_fit: set equ gain to 0dB */
           if (p_ext_data->auto_fit.state == EFT_AUTO_FIT_STATE_SWITCHED_OFF) {
               for (k = 0; k < p_ext_data->auto_fit.num_bands; k++) {
                   p_ext_data->auto_fit.band[k].eq_gain_lin = EFT_Q27_UNITY;
                   p_ext_data->auto_fit.band[k].eq_gain_lin_smoothed = EFT_Q27_UNITY;
                   p_ext_data->auto_fit.band[k].eq_gain_lin_prv = EFT_Q27_UNITY;
                   /* monitor signals */
                   p_ext_data->auto_fit.band[k].pow_ref_avg = 0;
                   p_ext_data->auto_fit.band[k].pow_intmic_avg = 0;
                   p_ext_data->auto_fit.band[k].low_sig_flag = 0;
                   p_ext_data->auto_fit.band[k].num_clippings = 0;
               }
               /* disable sent message flag and set state to OFF */
               p_ext_data->auto_fit.is_ready_to_send_msg = FALSE;
               p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_OFF;
               p_ext_data->one_shot.start = FALSE;
               auto_fit_send_gain_update_to_equ(p_ext_data); /* last message when SWITCHED_OFF */
               L4_DBG_MSG("EFT: auto fit, state set to OFF (transition from SWITCHED_OFF to OFF)");
               /* after last msg is sent, switch to new state */
               p_ext_data->cur_mode = p_ext_data->auto_fit.cur_mode_bak;
               return TRUE;
           }
           if (p_ext_data->auto_fit.msgs_per_capture_interval > 0)
           {
               /* check occurence of clipping since last message */
               auto_fit_clipping_detection(p_ext_data);
               /* send gain update message */
               auto_fit_send_gain_update_to_equ(p_ext_data);
           }
           p_ext_data->auto_fit.frame_cnt_msg += p_shot->duration_frames;
       }
       p_ext_data->auto_fit.frame_cnt_msg -= p_ext_data->auto_fit.msgs_per_capture_interval;
    }

    /* check if previous capture is still active */
    if (p_shot->start)
    {
        return TRUE;
    }

    /**********
     * Called on capture rate
     **********/
    L4_DBG_MSG("EFT: auto fit, start new capture");
    /* initialize auto_fit and launch 1st capture */
    if (p_ext_data->auto_fit.state == EFT_AUTO_FIT_STATE_SWITCHED_ON) {
        p_ext_data->auto_fit.state = EFT_AUTO_FIT_STATE_ON;
        L4_DBG_MSG("EFT: auto fit, state set to ON (transition from SWITCHED_ON to ON)");
    }
    p_shot->frame_counter = p_shot->duration_frames;
    p_shot->start = TRUE;

    return TRUE;
}


/****************************************************************************
Data processing function
*/
void eft_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    EFT_OP_DATA *p_ext_data = get_instance_data(op_data);
    /* Reference the capability parameters */
    EARBUD_FIT_TEST_PARAMETERS *p_params;
    int i, sample_count, samples_to_process;

    /* Certain conditions require an "early exit" that will just discard any
     * data in the input buffers and not do any other processing
     */
    bool exit_early, discard_data;
    unsigned b4idx, afteridx;
    metadata_tag *mtag_ip_list;

    EFT_CAPTURE *p_shot = &p_ext_data->one_shot;

    /*********************
     * Early exit testing
     *********************/

    /* Without adequate data we can just return */
    samples_to_process = INT_MAX;
    samples_to_process = eft_calc_samples_to_process(p_ext_data);

    /* Return early if playback and int mic input terminals are not connected */
    if (samples_to_process == INT_MAX)
    {
        L5_DBG_MSG("Minimum number of ports (ref and int mic) not connected");
        return;
    }

     /* Return early if not enough data to process */
    if (samples_to_process < EFT_DEFAULT_FRAME_SIZE)
    {
        L5_DBG_MSG1("Not enough data to process (%d)", samples_to_process);
        return;
    }

    /* Other conditions that are invalid for running EFT need to discard
     * input data if it exists.
     */

    exit_early = FALSE;
    /* Don't do any processing in standby */
    if (p_ext_data->cur_mode == EARBUD_FIT_TEST_SYSMODE_STANDBY)
    {
        exit_early = TRUE;
    }

    if (p_ext_data->in_out_status != EFT_IN_EAR)
    {
        exit_early = TRUE;
    }

    sample_count = 0;
    if (exit_early)
    {
        discard_data = TRUE;

        /* There is at least 1 frame to process */
        do {
            sample_count += EFT_DEFAULT_FRAME_SIZE;
            /* Iterate through all sinks */
            for (i = 0; i < EFT_MAX_SINKS; i++)
            {
                if (p_ext_data->inputs[i] != NULL)
                {
                    /* Discard a frame of data */
                    cbuffer_discard_data(p_ext_data->inputs[i],
                                         EFT_DEFAULT_FRAME_SIZE);

                    /* If there isn't a frame worth of data left then don't
                     * iterate through the input terminals again.
                     */
                    samples_to_process = cbuffer_calc_amount_data_in_words(
                        p_ext_data->inputs[i]);

                    if (samples_to_process < EFT_DEFAULT_FRAME_SIZE)
                    {
                        discard_data = FALSE;
                    }
                }
            }
        } while (discard_data);

        for (i=0; i < EFT_NUM_METADATA_CHANNELS; i++)
        {
            /* Extract metadata tag from input */
            mtag_ip_list = buff_metadata_remove(p_ext_data->metadata_ip[i],
                            sample_count * OCTETS_PER_SAMPLE, &b4idx, &afteridx);

            /* Free all the incoming tags */
            buff_metadata_tag_list_delete(mtag_ip_list);
        }

        /* Exit early */
        return;
    }

    if (p_ext_data->re_init_flag == TRUE)
    {
        p_ext_data->re_init_flag = FALSE;

        /* Initialize events*/
        eft_initialize_events(op_data, p_ext_data);

        /* Initialize afb and fit100 */
        aanc_afb_initialize(p_ext_data->f_handle,
                            p_ext_data->p_afb_ref);
        aanc_afb_initialize(p_ext_data->f_handle,
                            p_ext_data->p_afb_int);

        p_params = &p_ext_data->eft_cap_params;
        p_ext_data->p_fit->bin_select = p_params->OFFSET_BIN_SELECT;
        p_ext_data->p_fit->power_smooth_time = p_params->OFFSET_POWER_SMOOTH_TIME;
        p_ext_data->p_fit->threshold = p_params->OFFSET_FIT_THRESHOLD;
        p_ext_data->p_fit->bexp_offset = 0;

        p_ext_data->fit_quality = 0;
        p_ext_data->prev_fit_quality = 0;

        aanc_fit100_initialize(p_ext_data->f_handle,
                               p_ext_data->p_fit,
                               p_ext_data->p_afb_int,
                               p_ext_data->p_afb_ref);

        /* initialize auto fit */
        eft_auto_fit_set_config(p_ext_data);
    }

    sample_count = 0;
    while (samples_to_process >= EFT_DEFAULT_FRAME_SIZE)
    {

        /* Copy input data to internal data buffers */
        cbuffer_copy(p_ext_data->p_tmp_ref_ip,
                     p_ext_data->inputs[EFT_PLAYBACK_TERMINAL_ID],
                     EFT_DEFAULT_FRAME_SIZE);
        cbuffer_copy(p_ext_data->p_tmp_int_ip,
                     p_ext_data->inputs[EFT_MIC_INT_TERMINAL_ID],
                     EFT_DEFAULT_FRAME_SIZE);

        t_fft_object *p_fft_ref = p_ext_data->p_afb_ref->afb.fft_object_ptr;
        p_fft_ref->real_scratch_ptr = scratch_commit(
            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM1);
        p_fft_ref->imag_scratch_ptr = scratch_commit(
            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);
        p_fft_ref->fft_scratch_ptr = scratch_commit(
            AANC_FILTER_BANK_NUM_BINS*sizeof(int), MALLOC_PREFERENCE_DM2);

        /* AFB process on reference */
        aanc_afb_process_data(p_ext_data->f_handle, p_ext_data->p_afb_ref,
                              p_ext_data->p_tmp_ref_ip);

        /* Second AFB call re-uses scratch memory from the first */
        t_fft_object *p_fft_int = p_ext_data->p_afb_int->afb.fft_object_ptr;
        p_fft_int->real_scratch_ptr = p_fft_ref->real_scratch_ptr;
        p_fft_int->imag_scratch_ptr = p_fft_ref->imag_scratch_ptr;
        p_fft_int->fft_scratch_ptr = p_fft_ref->fft_scratch_ptr;

        /* AFB process on int mic */
        aanc_afb_process_data(p_ext_data->f_handle, p_ext_data->p_afb_int,
                              p_ext_data->p_tmp_int_ip);

        /* Set scratch pointers to NULL before freeing scratch */
        p_fft_ref->real_scratch_ptr = NULL;
        p_fft_ref->imag_scratch_ptr = NULL;
        p_fft_ref->fft_scratch_ptr = NULL;
        p_fft_int->real_scratch_ptr = NULL;
        p_fft_int->imag_scratch_ptr = NULL;
        p_fft_int->fft_scratch_ptr = NULL;

        scratch_free();

        /* FIT100 processing */
        aanc_fit100_process_data(p_ext_data->f_handle, p_ext_data->p_fit);

        p_ext_data->fit_quality = p_ext_data->p_fit->fit_flag;

        /* One-shot processing */
        if (p_shot->start)
        {
            if (p_shot->frame_counter == p_shot->duration_frames)
            {
                aanc_fit100_one_shot_initialize(
                    p_ext_data->f_handle,
                    p_ext_data->p_fit,
                    p_ext_data->one_shot.duration_frames
                );
            }
            p_ext_data->one_shot.frame_counter -= 1;
            aanc_fit100_one_shot_process(
                p_ext_data->f_handle,
                p_ext_data->p_fit);
            if (p_ext_data->one_shot.frame_counter == 0)
            {
                p_ext_data->one_shot.start = FALSE;
                if (p_ext_data->auto_fit.state == EFT_AUTO_FIT_STATE_OFF)
                {   /* raise only if AUTO_FIT is OFF */
                    eft_send_event_trigger(op_data,
                        EFT_EVENT_ID_FIT,
                        EFT_EVENT_PAYLOAD_CAPTURE_COMPLETE);
                }
            }
        }

        /* Process and send significant event, if any */
        eft_process_events(op_data, p_ext_data);

        /* Update prev fit flag after event processing */
        p_ext_data->prev_fit_quality = p_ext_data->fit_quality;

        /* auto fit */
        if (p_ext_data->auto_fit.state > EFT_AUTO_FIT_STATE_OFF)
        {
            eft_auto_fit_ctrl(p_ext_data);
        }

        cbuffer_discard_data(p_ext_data->p_tmp_ref_ip,
                                EFT_DEFAULT_FRAME_SIZE);
        cbuffer_discard_data(p_ext_data->p_tmp_int_ip,
                                EFT_DEFAULT_FRAME_SIZE);

        samples_to_process = eft_calc_samples_to_process(p_ext_data);
        sample_count += EFT_DEFAULT_FRAME_SIZE;
    }

    for (i=0; i < EFT_NUM_METADATA_CHANNELS; i++)
    {
        /* Extract metadata tag from input */
        mtag_ip_list = buff_metadata_remove(p_ext_data->metadata_ip[i],
                        sample_count * OCTETS_PER_SAMPLE, &b4idx, &afteridx);

        /* Free all the incoming tags */
        buff_metadata_tag_list_delete(mtag_ip_list);
    }
    /***************************
     * Update touched terminals
     ***************************/
    touched->sinks = (unsigned) EFT_MIN_VALID_SINKS;

    L5_DBG_MSG("EFT process channel data completed");

    return;
}
