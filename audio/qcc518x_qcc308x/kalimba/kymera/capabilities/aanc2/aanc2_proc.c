/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  aanc2_proc.c
 * \ingroup aanc2
 *
 * AANC2 algorithm processing library.
 *
 * This file provides create/destroy, initialize, and process_data handlers for
 * the algorithm processing in AANC2. These include:
 *
 * * Clipping detection
 * * Energy detection (ED)
 * * Self-speech
 * * FxLMS for adaptive gain calculation
 *
 */

#include "aanc2_proc.h"

/******************************************************************************
Private Function Definitions
*/

/******************************************************************************
Public Function Implementations
*/

bool aanc2_proc_create(AANC2_PROC *p_ag, unsigned sample_rate)
{

    uint16 fxlms_dmx_words, fxlms_dm_words; /* Memory required by FxLMS */
    uint16 ed_dmx_words, ed_dm1_words;      /* Memory required by EDs */
    FXLMS100_FILTER_COEFFS* coeffs;         /* Pointer to FxLMS coefficients */

    /* Load the feature handle for AANC private libraries */
    if (!load_aanc_handle(&p_ag->f_handle))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to load feature handle", p_ag->opid);
        return FALSE;
    }

    /* Allocate internal input cbuffer in DM1 */
    if (!aud_cur_create_cbuffer(&p_ag->p_tmp_int_ip,
                                AANC2_INTERNAL_BUFFER_SIZE,
                                MALLOC_PREFERENCE_DM1))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate int mic input buffer", p_ag->opid);
        return FALSE;
    }

    /* Allocate external input cbuffer in DM2 */
    if (!aud_cur_create_cbuffer(&p_ag->p_tmp_ext_ip,
                                AANC2_INTERNAL_BUFFER_SIZE,
                                MALLOC_PREFERENCE_DM1))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate ext mic input buffer", p_ag->opid);
        return FALSE;
    }

    /* Allocate int mic output cbuffer in DM2 */
    if (!aud_cur_create_cbuffer(&p_ag->p_tmp_int_op,
                                AANC2_INTERNAL_BUFFER_SIZE,
                                MALLOC_PREFERENCE_DM2))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate int mic output buffer", p_ag->opid);
        return FALSE;
    }

    /* Allocate ext mic output cbuffer in DM2 */
    if (!aud_cur_create_cbuffer(&p_ag->p_tmp_ext_op,
                                AANC2_INTERNAL_BUFFER_SIZE,
                                MALLOC_PREFERENCE_DM2))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate ext mic output buffer", p_ag->opid);
        return FALSE;
    }

    /* Create playback cbuffer */
    p_ag->p_tmp_pb_ip = cbuffer_create_with_malloc(AANC2_INTERNAL_BUFFER_SIZE,
                                                   BUF_DESC_SW_BUFFER);
    if (p_ag->p_tmp_pb_ip == NULL)
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate playback cbuffer", p_ag->opid);
        return FALSE;
    }

    /* Allocate FxLMS and ED memory using the mem_table API */
    fxlms_dmx_words = (uint16)(aanc_fxlms100_dmx_bytes()/sizeof(unsigned));
    fxlms_dm_words = (uint16)(AANC2_PROC_FXLMS_DM_BYTES/sizeof(unsigned));
    ed_dmx_words = (uint16)(aanc_ed100_dmx_bytes()/sizeof(unsigned));
    ed_dm1_words = (uint16)(aanc_ed100_dm1_bytes()/sizeof(unsigned));

    p_ag->p_table = xzpnewn(AANC2_PROC_MEM_TABLE_SIZE, malloc_t_entry);
    p_ag->p_table[0] = (malloc_t_entry){
        fxlms_dmx_words,
        MALLOC_PREFERENCE_NONE,
        offsetof(AANC2_PROC, p_fxlms)};
    p_ag->p_table[1] = (malloc_t_entry){
        fxlms_dm_words,
        MALLOC_PREFERENCE_DM1,
        offsetof(AANC2_PROC, p_fxlms_dm1)};
    p_ag->p_table[2] = (malloc_t_entry){
        fxlms_dm_words,
        MALLOC_PREFERENCE_DM2,
        offsetof(AANC2_PROC, p_fxlms_dm2)};
    p_ag->p_table[3] = (malloc_t_entry){
        ed_dmx_words,
        MALLOC_PREFERENCE_NONE,
        offsetof(AANC2_PROC, p_ed_int)};
    p_ag->p_table[4] = (malloc_t_entry){
        ed_dm1_words,
        MALLOC_PREFERENCE_DM1,
        offsetof(AANC2_PROC, p_ed_int_dm1)};
    p_ag->p_table[5] = (malloc_t_entry){
        ed_dmx_words,
        MALLOC_PREFERENCE_NONE,
        offsetof(AANC2_PROC, p_ed_ext)};
    p_ag->p_table[6] = (malloc_t_entry){
        ed_dm1_words,
        MALLOC_PREFERENCE_DM1,
        offsetof(AANC2_PROC, p_ed_ext_dm1)};
    p_ag->p_table[7] = (malloc_t_entry){
        ed_dmx_words,
        MALLOC_PREFERENCE_NONE,
        offsetof(AANC2_PROC, p_ed_pb)};
    p_ag->p_table[8] = (malloc_t_entry){
        ed_dm1_words,
        MALLOC_PREFERENCE_DM1,
        offsetof(AANC2_PROC, p_ed_pb_dm1)};

    if (!mem_table_zalloc((void *)p_ag,
                          p_ag->p_table,
                          AANC2_PROC_MEM_TABLE_SIZE))
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate memory", p_ag->opid);
        return FALSE;
    }

    /* Create shared ED cbuffer without specific bank allocation */
    p_ag->p_tmp_ed = cbuffer_create_with_malloc(ED100_DEFAULT_BUFFER_SIZE,
                                                BUF_DESC_SW_BUFFER);
    if (p_ag->p_tmp_ed == NULL)
    {
        aanc2_proc_destroy(p_ag);
        L2_DBG_MSG1("OPID: %x, AANC2_PROC failed to allocate ED cbuffer", p_ag->opid);
        return FALSE;
    }

    aanc_ed100_create(p_ag->p_ed_int, p_ag->p_ed_int_dm1, sample_rate);
    aanc_ed100_create(p_ag->p_ed_ext, p_ag->p_ed_ext_dm1, sample_rate);
    aanc_ed100_create(p_ag->p_ed_pb, p_ag->p_ed_pb_dm1, sample_rate);

    /* Initialize number of taps to allow correct buffer alignment in create */
    p_ag->p_fxlms->p_plant.num_coeffs = AANC2_PROC_NUM_TAPS_PLANT;
    p_ag->p_fxlms->p_plant.full_num_coeffs = AANC2_PROC_NUM_TAPS_PLANT;
    p_ag->p_fxlms->p_control_0.num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
    p_ag->p_fxlms->p_control_0.full_num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
    p_ag->p_fxlms->p_control_1.num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
    p_ag->p_fxlms->p_control_1.full_num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
    p_ag->p_fxlms->p_bp_int.num_coeffs = AANC2_PROC_NUM_TAPS_BP;
    p_ag->p_fxlms->p_bp_int.full_num_coeffs = AANC2_PROC_NUM_TAPS_BP;
    p_ag->p_fxlms->p_bp_ext.num_coeffs = AANC2_PROC_NUM_TAPS_BP;
    p_ag->p_fxlms->p_bp_ext.full_num_coeffs = AANC2_PROC_NUM_TAPS_BP;

    aanc_fxlms100_create(p_ag->p_fxlms, p_ag->p_fxlms_dm1, p_ag->p_fxlms_dm2);

    /* Initialize plant model as pass-through */
    coeffs = &p_ag->p_fxlms->p_plant.coeffs;
    coeffs->p_num[0] = FXLMS100_MODEL_COEFF0;
    coeffs->p_den[0] = FXLMS100_MODEL_COEFF0;

    /* Initialize control 0 model as pass-through */
    coeffs = &p_ag->p_fxlms->p_control_0.coeffs;
    coeffs->p_num[0] = FXLMS100_MODEL_COEFF0;
    coeffs->p_den[0] = FXLMS100_MODEL_COEFF0;

    return TRUE;
}

bool aanc2_proc_destroy(AANC2_PROC *p_ag)
{
    /* Destroy EDs */
    aanc_ed100_destroy(p_ag->p_ed_int);
    aanc_ed100_destroy(p_ag->p_ed_ext);
    aanc_ed100_destroy(p_ag->p_ed_pb);

    /* Free memory table */
    if (p_ag->p_table != NULL)
    {
        mem_table_free((void *)p_ag, p_ag->p_table, AANC2_PROC_MEM_TABLE_SIZE);
        pdelete(p_ag->p_table);
    }

    /* Destroy internal cbuffers */
    cbuffer_destroy(p_ag->p_tmp_ed);

    cbuffer_destroy(p_ag->p_tmp_int_ip);
    cbuffer_destroy(p_ag->p_tmp_ext_ip);
    cbuffer_destroy(p_ag->p_tmp_pb_ip);

    cbuffer_destroy(p_ag->p_tmp_int_op);
    cbuffer_destroy(p_ag->p_tmp_ext_op);

    /* Unload the feature handle */
    unload_aanc_handle(p_ag->f_handle);

    return TRUE;
}

bool aanc2_proc_initialize(AANC2_PROC *p_ag,
                           AANC2_PARAMETERS *p_params)
{
    int i;                          /* Loop control */
    FXLMS100_DMX *p_dmx;            /* Pointer to FxLMS */
    bool disable_filter_check;      /* Disable filter check indicator */
    unsigned config, debug_config;  /* Config and debug parameter values */
    unsigned clip_disable;          /* Clip disable flag */

    /* Re-initialize flags */
    p_ag->proc_flags = 0;
    p_ag->prev_flags = 0;

    /* Initialize the FxLMS */
    p_dmx = p_ag->p_fxlms;

    /* Initialize buffer pointers */
    p_dmx->p_int_ip = p_ag->p_tmp_int_ip;
    p_dmx->p_int_op = p_ag->p_tmp_int_op;
    p_dmx->p_ext_ip = p_ag->p_tmp_ext_ip;
    p_dmx->p_ext_op = p_ag->p_tmp_ext_op;

    /* Set FxLMS parameters */
    p_dmx->target_nr = p_params->OFFSET_TARGET_NOISE_REDUCTION;
    p_dmx->mu = p_params->OFFSET_MU;
    p_dmx->gamma = AANC2_FXLMS_GAMMA;
    p_dmx->lambda = p_params->OFFSET_LAMBDA;
    p_dmx->frame_size = AANC2_DEFAULT_FRAME_SIZE;
    p_dmx->min_bound = p_params->OFFSET_FXLMS_MIN_BOUND;
    p_dmx->max_bound = p_params->OFFSET_FXLMS_MAX_BOUND;
    p_dmx->max_delta = AANC2_FXLMS_MAX_DELTA;

    /* Optimization to reduce the effective number of taps in plant and control
     * filters if there are both trailing numerator and denominator coefficients
     */
    debug_config = p_params->OFFSET_AANC2_DEBUG;
    if ((debug_config & AANC2_CONFIG_AANC2_DEBUG_DISABLE_FILTER_OPTIM) > 0)
    {
        p_dmx->p_plant.num_coeffs = AANC2_PROC_NUM_TAPS_PLANT;
        p_dmx->p_control_0.num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
        p_dmx->p_control_1.num_coeffs = AANC2_PROC_NUM_TAPS_CONTROL;
        L4_DBG_MSG1("OPID: %x, AANC_PROC filters set to default number of coefficients",
                    p_ag->opid);
    }
    else
    {
        p_dmx->p_plant.num_coeffs = aanc_fxlms100_calculate_num_coeffs(
            &p_dmx->p_plant, AANC2_PROC_NUM_TAPS_PLANT);
        p_dmx->p_control_0.num_coeffs = aanc_fxlms100_calculate_num_coeffs(
            &p_dmx->p_control_0, AANC2_PROC_NUM_TAPS_CONTROL);
        p_dmx->p_control_1.num_coeffs = aanc_fxlms100_calculate_num_coeffs(
            &p_dmx->p_control_1, AANC2_PROC_NUM_TAPS_CONTROL);

        L4_DBG_MSG4(
            "OPID: %x, AANC_PROC filter coeffs: Plant=%hu, Control 0=%hu, Control 1=%hu",
            p_ag->opid, p_dmx->p_plant.num_coeffs, p_dmx->p_control_0.num_coeffs,
            p_dmx->p_control_1.num_coeffs);
    }

    /* Initialize FxLMS bandpass model */
    int bp_num_coeffs_int[] = {
        p_params->OFFSET_BPF_NUMERATOR_COEFF_INT_0,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_INT_1,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_INT_2,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_INT_3,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_INT_4
    };

    int bp_den_coeffs_int[] = {
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_INT_0,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_INT_1,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_INT_2,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_INT_3,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_INT_4
    };

    int bp_num_coeffs_ext[] = {
        p_params->OFFSET_BPF_NUMERATOR_COEFF_EXT_0,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_EXT_1,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_EXT_2,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_EXT_3,
        p_params->OFFSET_BPF_NUMERATOR_COEFF_EXT_4
    };

    int bp_den_coeffs_ext[] = {
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_EXT_0,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_EXT_1,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_EXT_2,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_EXT_3,
        p_params->OFFSET_BPF_DENOMINATOR_COEFF_EXT_4
    };

    for (i = 0; i < p_dmx->p_bp_ext.num_coeffs; i++)
    {
        p_dmx->p_bp_ext.coeffs.p_num[i] = bp_num_coeffs_ext[i];
        p_dmx->p_bp_ext.coeffs.p_den[i] = bp_den_coeffs_ext[i];
        p_dmx->p_bp_int.coeffs.p_num[i] = bp_num_coeffs_int[i];
        p_dmx->p_bp_int.coeffs.p_den[i] = bp_den_coeffs_int[i];
    }

    /* Get control for whether the read pointer is updated or not in the FxLMS
     * library. If MUX_SEL_ALGORITHM is set then terminal output is from the
     * FxLMS filter stage. In that case the read pointer for the input buffers
     * is updated within the FxLMS because the data is not copied at a later
     * stage. Otherwise the read pointer is left untouched so that the cbuffer
     * copy routine operates correctly.
     */
    debug_config = p_params->OFFSET_AANC2_DEBUG;
    p_ag->p_fxlms->read_ptr_upd = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_MUX_SEL_ALGORITHM) > 0;

    aanc_fxlms100_initialize(p_ag->f_handle, p_ag->p_fxlms, FALSE);

    /* Set the quiet condition thresholds */
    p_ag->quiet_hi_threshold = p_params->OFFSET_QUIET_MODE_HI_THRESHOLD;
    p_ag->quiet_lo_threshold = p_params->OFFSET_QUIET_MODE_LO_THRESHOLD;

    /* Set the self-speech condition threshold */
    config = p_params->OFFSET_AANC2_CONFIG;
    p_ag->self_speech_disabled = (config & \
        AANC2_CONFIG_AANC2_CONFIG_DISABLE_SELF_SPEECH) > 0;
    p_ag->self_speech_threshold = p_params->OFFSET_SELF_SPEECH_THRESHOLD;

    p_ag->pb_ratio_disabled = (config & \
        AANC2_CONFIG_AANC2_CONFIG_DISABLE_PLAYBACK_RATIO) > 0;
    p_ag->pb_ratio_threshold = p_params->OFFSET_PLAYBACK_RATIO_THRESHOLD_CONC;

    /* Initialize the internal ED */
    p_ag->p_ed_int->p_input = p_ag->p_tmp_int_ip;
    p_ag->p_ed_int->p_tmp = p_ag->p_tmp_ed;
    p_ag->p_ed_int->disabled = (config & \
        AANC2_CONFIG_AANC2_CONFIG_DISABLE_ED_INT) > 0;
    p_ag->p_ed_int->frame_size = AANC2_DEFAULT_FRAME_SIZE;
    p_ag->p_ed_int->attack_time = AANC2_ED_ATTACK;
    p_ag->p_ed_int->decay_time = AANC2_ED_DECAY;
    p_ag->p_ed_int->envelope_time = p_params->OFFSET_ED_INT_ENVELOPE;
    p_ag->p_ed_int->init_frame_time = AANC2_ED_INIT_FRAME;
    p_ag->p_ed_int->ratio = p_params->OFFSET_ED_INT_RATIO;
    p_ag->p_ed_int->min_signal = p_params->OFFSET_ED_INT_MIN_SIGNAL;
    p_ag->p_ed_int->min_max_envelope = \
        p_params->OFFSET_ED_INT_MIN_MAX_ENVELOPE;
    p_ag->p_ed_int->delta_th = p_params->OFFSET_ED_INT_DELTA_TH;
    p_ag->p_ed_int->count_th = AANC2_ED_COUNT_TH;
    p_ag->p_ed_int->hold_frames = p_params->OFFSET_ED_INT_HOLD_FRAMES;
    p_ag->p_ed_int->e_min_threshold = \
        p_params->OFFSET_ED_INT_E_FILTER_MIN_THRESHOLD;
    p_ag->p_ed_int->e_min_counter_threshold = \
        p_params->OFFSET_ED_INT_E_FILTER_MIN_COUNTER_THRESHOLD;
    disable_filter_check = FALSE;
    disable_filter_check = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_ED_INT_E_FILTER_CHECK) > 0;
    p_ag->p_ed_int->e_min_check_disabled = disable_filter_check;
    aanc_ed100_initialize(p_ag->f_handle, p_ag->p_ed_int);

    /* Initialize the external ED */
    p_ag->p_ed_ext->p_input = p_ag->p_tmp_ext_ip;
    p_ag->p_ed_ext->p_tmp = p_ag->p_tmp_ed;
    p_ag->p_ed_ext->disabled = (config & \
        AANC2_CONFIG_AANC2_CONFIG_DISABLE_ED_EXT) > 0;
    p_ag->p_ed_ext->frame_size = AANC2_DEFAULT_FRAME_SIZE;
    p_ag->p_ed_ext->attack_time = AANC2_ED_ATTACK;
    p_ag->p_ed_ext->decay_time = AANC2_ED_DECAY;
    p_ag->p_ed_ext->envelope_time = p_params->OFFSET_ED_EXT_ENVELOPE;
    p_ag->p_ed_ext->init_frame_time = AANC2_ED_INIT_FRAME;
    p_ag->p_ed_ext->ratio = p_params->OFFSET_ED_EXT_RATIO;
    p_ag->p_ed_ext->min_signal = p_params->OFFSET_ED_EXT_MIN_SIGNAL;
    p_ag->p_ed_ext->min_max_envelope = \
        p_params->OFFSET_ED_EXT_MIN_MAX_ENVELOPE;
    p_ag->p_ed_ext->delta_th = p_params->OFFSET_ED_EXT_DELTA_TH;
    p_ag->p_ed_ext->count_th = AANC2_ED_COUNT_TH;
    p_ag->p_ed_ext->hold_frames = p_params->OFFSET_ED_EXT_HOLD_FRAMES;
    p_ag->p_ed_ext->e_min_threshold = \
        p_params->OFFSET_ED_EXT_E_FILTER_MIN_THRESHOLD;
    p_ag->p_ed_ext->e_min_counter_threshold = \
        p_params->OFFSET_ED_EXT_E_FILTER_MIN_COUNTER_THRESHOLD;
    disable_filter_check = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_ED_EXT_E_FILTER_CHECK) > 0;
    p_ag->p_ed_ext->e_min_check_disabled = disable_filter_check;
    aanc_ed100_initialize(p_ag->f_handle, p_ag->p_ed_ext);

    /* Initialize the playback ED */
    p_ag->p_ed_pb->p_input = p_ag->p_tmp_pb_ip;
    p_ag->p_ed_pb->p_tmp = p_ag->p_tmp_ed;
    p_ag->p_ed_pb->disabled = (config &
        AANC2_CONFIG_AANC2_CONFIG_DISABLE_ED_PB) > 0;
    p_ag->p_ed_pb->frame_size = AANC2_DEFAULT_FRAME_SIZE;
    p_ag->p_ed_pb->attack_time = AANC2_ED_ATTACK;
    p_ag->p_ed_pb->decay_time = AANC2_ED_DECAY;
    p_ag->p_ed_pb->envelope_time = p_params->OFFSET_ED_PB_ENVELOPE;
    p_ag->p_ed_pb->init_frame_time = AANC2_ED_INIT_FRAME;
    p_ag->p_ed_pb->ratio = p_params->OFFSET_ED_PB_RATIO;
    p_ag->p_ed_pb->min_signal = p_params->OFFSET_ED_PB_MIN_SIGNAL;
    p_ag->p_ed_pb->min_max_envelope = \
        p_params->OFFSET_ED_PB_MIN_MAX_ENVELOPE;
    p_ag->p_ed_pb->delta_th = p_params->OFFSET_ED_PB_DELTA_TH;
    p_ag->p_ed_pb->count_th = AANC2_ED_COUNT_TH;
    p_ag->p_ed_pb->hold_frames = p_params->OFFSET_ED_PB_HOLD_FRAMES;
    p_ag->p_ed_pb->e_min_threshold = \
        p_params->OFFSET_ED_PB_E_FILTER_MIN_THRESHOLD;
    p_ag->p_ed_pb->e_min_counter_threshold = \
        p_params->OFFSET_ED_PB_E_FILTER_MIN_COUNTER_THRESHOLD;
    disable_filter_check = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_ED_PB_E_FILTER_CHECK) > 0;
    p_ag->p_ed_pb->e_min_check_disabled = disable_filter_check;
    aanc_ed100_initialize(p_ag->f_handle, p_ag->p_ed_pb);

    /* Initialize clipping */
    clip_disable = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_CLIPPING_DETECT_INT) > 0;
    aanc2_clipping_initialize(&p_ag->clip_int,
                              p_ag->p_tmp_int_ip,
                              AANC2_CLIPPING_THRESHOLD,
                              p_params->OFFSET_CLIPPING_DURATION_INT,
                              clip_disable);

    clip_disable = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_CLIPPING_DETECT_EXT) > 0;
    aanc2_clipping_initialize(&p_ag->clip_ext,
                              p_ag->p_tmp_ext_ip,
                              AANC2_CLIPPING_THRESHOLD,
                              p_params->OFFSET_CLIPPING_DURATION_EXT,
                              clip_disable);

    clip_disable = (debug_config & \
        AANC2_CONFIG_AANC2_DEBUG_DISABLE_CLIPPING_DETECT_PB) > 0;
    aanc2_clipping_initialize(&p_ag->clip_pb,
                              p_ag->p_tmp_pb_ip,
                              AANC2_CLIPPING_THRESHOLD,
                              p_params->OFFSET_CLIPPING_DURATION_PB,
                              clip_disable);

    return TRUE;
}

bool aanc2_proc_initialize_concurrency(AANC2_PROC *p_ag,
                                       AANC2_PARAMETERS *p_params,
                                       bool concurrency)
{
    if (concurrency)
    {
        p_ag->self_speech_threshold = p_params->OFFSET_SELF_SPEECH_THRESHOLD_CONC;
        p_ag->p_ed_int->envelope_time = p_params->OFFSET_ED_INT_ENVELOPE_CONC;
        p_ag->p_ed_int->min_signal = p_params->OFFSET_ED_INT_MIN_SIGNAL_CONC;
    }
    else {
        p_ag->self_speech_threshold = p_params->OFFSET_SELF_SPEECH_THRESHOLD;
        p_ag->p_ed_int->envelope_time = p_params->OFFSET_ED_INT_ENVELOPE;
        p_ag->p_ed_int->min_signal = p_params->OFFSET_ED_INT_MIN_SIGNAL;
    }

    return TRUE;
}

/**
 * \brief  Copy or discard cbuffer data
 *
 * \param  p_dest       Destination cbuffer
 * \param  p_src        Source cbuffer
 * \param  samples      Number of samples to consume
 *
 * \return  NONE
 *
 * If the destination buffer exists then the data is copied otherwise the data
 * is discarded.
 */
static inline void aanc2_proc_consume_cbuffer(tCbuffer *p_dest,
                                              tCbuffer *p_src,
                                              unsigned samples)
{
    if (p_dest != NULL)
    {
        cbuffer_copy(p_dest, p_src, samples);
    }
    else
    {
        cbuffer_discard_data(p_src, samples);
    }

    return;
}

/**
 * \brief  Pass data through the internal cbuffers
 *
 * \param  p_ag         Pointer to the AANC2_PROC data object
 * \param  samples      Number of samples to pass through
 * \param  use_output   Boolean indicating whether to use the temporary input
 *                      buffers or temporary output buffers to copy data to the
 *                      capability source terminals
 *
 * \return  boolean indicating success or failure.
 */
static void aanc2_proc_pass_data(AANC2_PROC *p_ag,
                                 unsigned samples,
                                 bool use_output)
{
    tCbuffer *p_int_working_buffer, *p_ext_working_buffer;

    if (use_output)
    {
        p_int_working_buffer = p_ag->p_tmp_int_op;
        p_ext_working_buffer = p_ag->p_tmp_ext_op;

        /* The internal output buffers have been populated by the FxLMS
         * processing routine. To make the buffer alignment work we need to
         * advance the respective read and write pointers.
         */
        cbuffer_advance_write_ptr(p_ag->p_tmp_ext_op, samples);
        cbuffer_advance_write_ptr(p_ag->p_tmp_int_op, samples);
        cbuffer_advance_read_ptr(p_ag->p_tmp_ext_ip, samples);
        cbuffer_advance_read_ptr(p_ag->p_tmp_int_ip, samples);
    }
    else
    {
        p_int_working_buffer = p_ag->p_tmp_int_ip;
        p_ext_working_buffer = p_ag->p_tmp_ext_ip;
    }

    /* Copy terminal data on internal and external microphones */
    aanc2_proc_consume_cbuffer(p_ag->p_mic_int_op,
                               p_int_working_buffer,
                               samples);
    aanc2_proc_consume_cbuffer(p_ag->p_mic_ext_op,
                               p_ext_working_buffer,
                               samples);

    /* Copy or discard data on the playback stream */
    if (p_ag->p_playback_ip != NULL) {
        aanc2_proc_consume_cbuffer(p_ag->p_playback_op,
                                   p_ag->p_tmp_pb_ip,
                                   samples);
    }
}

bool aanc2_proc_process_data(AANC2_PROC *p_ag,
                             unsigned samples,
                             bool calculate_gain)
{
    bool self_speech, copy_output, high_pb_ratio;
    unsigned flags, clip_det;

    /* Copy input data to internal data buffers */
    cbuffer_copy(p_ag->p_tmp_int_ip, p_ag->p_mic_int_ip, samples);
    cbuffer_copy(p_ag->p_tmp_ext_ip, p_ag->p_mic_ext_ip, samples);

    /* Copy playback data to internal data buffers if connected */
    if (p_ag->p_playback_ip != NULL)
    {
        cbuffer_copy(p_ag->p_tmp_pb_ip, p_ag->p_playback_ip, samples);
    }

    /* Clear all flags connected with processing data but persist quiet mode */
    flags = p_ag->proc_flags & AANC2_FLAGS_QUIET_MODE;

    /* Calculate peak values and clipping status */
    if (!p_ag->clip_ext.disabled && !p_ag->clip_int.disabled)
    {
        aanc2_clipping_peak_detect_dual(&p_ag->clip_int,
                                        &p_ag->clip_ext,
                                        samples);
        aanc2_clipping_process_detection(&p_ag->clip_ext);
        aanc2_clipping_process_detection(&p_ag->clip_int);
    }
    if (!p_ag->clip_pb.disabled && p_ag->p_playback_ip != NULL)
    {
        aanc2_clipping_peak_detect_single(&p_ag->clip_pb, samples);
        aanc2_clipping_process_detection(&p_ag->clip_pb);
    }

    /* If clipping is detected pass data through and exit early */
    clip_det = p_ag->clip_ext.detected * AANC2_FLAGS_CLIPPING_EXT;
    clip_det |= p_ag->clip_int.detected * AANC2_FLAGS_CLIPPING_INT;
    clip_det |= p_ag->clip_pb.detected * AANC2_FLAGS_CLIPPING_PLAYBACK;

    if (clip_det > 0)
    {
        aanc2_proc_pass_data(p_ag, samples, FALSE);
        flags |= clip_det;
        p_ag->proc_flags = flags;
        return FALSE;
    }

    /* ED process ext mic */
    if (!p_ag->p_ed_ext->disabled)
    {
        aanc_ed100_process_data(p_ag->f_handle, p_ag->p_ed_ext);

        /* Catch external ED detection */
        if (p_ag->p_ed_ext->detection)
        {
            flags |= AANC2_FLAGS_ED_EXT;
        }

        /* Threshold detect on external ED */
        if (p_ag->p_ed_ext->spl < p_ag->quiet_lo_threshold)
        {
            /* Set quiet mode flag */
            flags |= AANC2_FLAGS_QUIET_MODE;
        }
        else if (p_ag->p_ed_ext->spl > p_ag->quiet_hi_threshold)
        {
            /* Reset quiet mode flag */
            flags &= AANC2_PROC_QUIET_MODE_RESET_FLAG;
        }
    }

    /* ED process int mic */
    if (!p_ag->p_ed_int->disabled)
    {
        aanc_ed100_process_data(p_ag->f_handle, p_ag->p_ed_int);
        if (p_ag->p_ed_int->detection)
        {
            flags |= AANC2_FLAGS_ED_INT;
        }
    }

    /* ED process self-speech */
    self_speech = FALSE;
    if (!p_ag->self_speech_disabled)
    {
        self_speech = aanc_ed100_self_speech_detect(
            p_ag->p_ed_int, p_ag->p_ed_ext,
            p_ag->self_speech_threshold);
        if (self_speech)
        {
            flags |= AANC2_FLAGS_SELF_SPEECH;
        }
    }

    /* ED process playback */
    high_pb_ratio = FALSE;
    if (p_ag->p_playback_ip != NULL && !p_ag->p_ed_pb->disabled)
    {
        aanc_ed100_process_data(p_ag->f_handle, p_ag->p_ed_pb);
        if (p_ag->p_ed_pb->detection)
        {
            flags |= AANC2_FLAGS_ED_PLAYBACK;
        }

        /* Compute playback ratio */
        if (!p_ag->pb_ratio_disabled && !p_ag->p_ed_ext->disabled)
        {
            high_pb_ratio = aanc_ed100_self_speech_detect(
                p_ag->p_ed_pb, p_ag->p_ed_ext,
                p_ag->pb_ratio_threshold);
            if (high_pb_ratio)
            {
                flags |= AANC2_FLAGS_HIGH_PB_RATIO;
            }
        }
    }


    /* Call adaptive ANC function */
    if (!p_ag->p_ed_ext->detection && !p_ag->p_ed_int->detection &&
        !p_ag->p_ed_pb->detection  && !self_speech && calculate_gain &&
        !high_pb_ratio)
    {
        p_ag->prev_gain = (uint8)p_ag->p_fxlms->adaptive_gain;
        if (aanc_fxlms100_process_data(p_ag->f_handle, p_ag->p_fxlms))
        {
            flags |= p_ag->p_fxlms->flags;
        }
    }

    /* Update flags */
    p_ag->proc_flags = flags;

    /* Copy data from internal to external buffers */
    copy_output = p_ag->p_fxlms->read_ptr_upd > 0;
    aanc2_proc_pass_data(p_ag, samples, copy_output);

    return TRUE;
}
