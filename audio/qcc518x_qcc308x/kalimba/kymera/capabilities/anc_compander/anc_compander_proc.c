/****************************************************************************
 * Copyright (c) 2015 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_compander_proc.c
 * \ingroup  operators
 *
 *  ANC Compander processing library
 *
 */

#include "anc_compander_proc.h"

void anc_compander_proc_init(ANC_COMPANDER_OP_DATA *p_ext_data)
{
    unsigned sample_rate;
    unsigned lookahead_time;
    unsigned lookahead_samples;
    tCbuffer *p_input;
    ANC_COMPANDER_PARAMETERS * compander_params;
    t_compander_object *compander_obj = p_ext_data->compander_object;
    compander_params = &p_ext_data->compander_cap_params;

    if (compander_obj == NULL)
    {
        /* compander_obj is allocated in anc_compander_create() */
        return;
    }
    p_ext_data->lookahead_status = 0;

    sample_rate = p_ext_data->sample_rate;
    /* Lookahead time in seconds (Q12.20) */
    lookahead_time = compander_params->OFFSET_LOOKAHEAD_TIME;

    /*Actual size used is one more word than (time * rate)
     *See use of LOOKAHEAD_SAMPLES in $audio_proc.cmpd.final_gain_apply */
    lookahead_samples = ((sample_rate * lookahead_time) >> \
        ANC_COMPANDER_LOOKAHEAD_DUR_SHIFT) + 1;

    compander_obj->data_objects_ptr = (void *)p_ext_data->compander_object;
    compander_obj->sample_rate = sample_rate;
    compander_obj->num_channels = ANC_COMPANDER_NUM_COMPANDING_CHANNELS;

    if (compander_obj->lookahead_hist_buf != NULL)
    {
        pfree(compander_obj->lookahead_hist_buf);
        compander_obj->lookahead_hist_buf = NULL;
    }
    if (lookahead_samples)
    {
        compander_obj->lookahead_hist_buf = \
            (unsigned*) xzppmalloc(lookahead_samples * sizeof(unsigned),
                                   MALLOC_PREFERENCE_DM1);

        if (compander_obj->lookahead_hist_buf == NULL)
        {
            p_ext_data->lookahead_status = p_ext_data->lookahead_status | \
                ANC_COMPANDER_COMPANDING_TERMINAL_POS;
        }
    }
    anc_compander_initialize(compander_obj);

    /* Compander initialization resets linear gain to 1.0. Reset logarithmic
     * gain to the corresponding previous makeup gain to give a gradual
     * gain adjustment if the makeup gain changes. The rate is controlled by
     * the attack and release time constants.
     */
    compander_obj->gain_smooth_hist = (p_ext_data->compander_cap_params.OFFSET_MAKEUP_GAIN >>
                                       ADRC_MAKEUP_TO_GAIN_HIST_SHIFT);

    /* Reset level smoothing to the first input sample value */
    p_input = (tCbuffer*)compander_obj->channel_input_ptr;
    if (p_input != NULL)
    {
        compander_obj->level_detect_last_sample_hist = *p_input->read_ptr;
    }

}
