/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_hw_manager_ramp.c
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) ramp functions.
 *
 */

/****************************************************************************
Include Files
*/

#include "anc_hw_manager_ramp.h"

/*****************************************************************************
Private Constant Definitions
*/

/*****************************************************************************
Public Function Implementations
*/

bool ahm_initialize_ramp(AHM_RAMP *p_ramp,
                         AHM_RAMP_CONFIG *p_cfg,
                         AHM_EVENT_MSG *p_msg,
                         unsigned sample_rate)
{
    int delta, timer_duration, delay_duration;
    uint16 *p_gain;

    bool send_message = FALSE;

    p_gain = p_ramp->p_cur;

    /* If the timer and delay are both 0 then immediately set the gain value
     * to the target value.
     */
    if (p_cfg->duration == 0 && p_cfg->delay == 0)
    {
        /* Update current gain */
        *p_gain = (uint16)p_cfg->target;
        /* Update ramp gain */
        p_ramp->p_gain->gain = (uint8)p_cfg->target;
        p_ramp->state = AHM_RAMP_FINISHED;
        return send_message;
    }

    timer_duration = ahm_calc_duration_samples(p_cfg->duration, sample_rate);
    delay_duration = ahm_calc_duration_samples(p_cfg->delay, sample_rate);

    /* Shift the current gain to Q8.24 */
    p_ramp->value = *p_gain << AHM_RAMP_PRECISION;
    p_ramp->target = p_cfg->target;
    /* Calculate the amount the ramp has to cover */
    delta = (int)(p_cfg->target << AHM_RAMP_PRECISION) - p_ramp->value;
    /* Calculate the amount to increment/decrement gain per sample */
    p_ramp->rate = delta / timer_duration;
    p_ramp->duration = timer_duration;
    if (delay_duration == 0)
    {
        p_ramp->state = AHM_RAMP_RUNNING;
        p_ramp->p_gain->valid = TRUE;
        p_msg->id = p_ramp->path;
        p_msg->type = AHM_EVENT_TYPE_TRIGGER;
        p_msg->payload = 0;
        send_message = TRUE;
    }
    else
    {
        p_ramp->state = AHM_RAMP_WAITING;
        /* Validate gain while waiting for the ramp. Apply the gain that
         * was stored at initialization which means no rounding is required
         */
        p_ramp->p_gain->valid = TRUE;
        *p_gain = (uint8)(p_ramp->value >> AHM_RAMP_PRECISION);
    }
    p_ramp->counter = p_ramp->duration + delay_duration;

    return send_message;
}

bool ahm_process_ramp(AHM_RAMP *p_ramp,
                      AHM_EVENT_MSG *p_msg,
                      unsigned samples)
{
    unsigned rounded_gain;
     uint8 *p_gain;
    bool send_message = FALSE;

    p_gain = &(p_ramp->p_gain->gain);

    switch (p_ramp->state)
    {
        case AHM_RAMP_INITIALIZED:
            p_ramp->state = AHM_RAMP_WAITING;
        case AHM_RAMP_WAITING:
            p_ramp->counter -= samples;
            if (p_ramp->counter <= p_ramp->duration)
            {
                p_ramp->state = AHM_RAMP_RUNNING;
                p_msg->id = p_ramp->path;
                p_msg->type = AHM_EVENT_TYPE_TRIGGER;
                p_msg->payload = 0;
                send_message = TRUE;
            }
            break;
        case AHM_RAMP_RUNNING:
            p_ramp->counter -= samples;
            if (p_ramp->counter <= 0)
            {
                p_ramp->state = AHM_RAMP_FINISHED;
                /* Make sure the ramp finishes */
                *p_gain = (uint8)p_ramp->target;
            }
            else
            {
                p_ramp->value += p_ramp->rate * samples;
                rounded_gain = (p_ramp->value + AHM_RAMP_ROUNDING) \
                    >> AHM_RAMP_PRECISION;
                *p_gain = (uint8)rounded_gain;
            }
            break;
        case AHM_RAMP_FINISHED:
            p_ramp->p_gain->valid = FALSE;
            p_msg->id = p_ramp->path;
            p_msg->type = AHM_EVENT_TYPE_CLEAR;
            p_msg->payload = 0;
            send_message = TRUE;
            p_ramp->state = AHM_RAMP_IDLE;
            p_ramp->p_gain->gain_delta = 0x01000000;
            break;
        default: /* Also covers AHM_RAMP_IDLE */
            break;
    }

    return send_message;
}

bool ahm_initialize_delta_ramp(AHM_DELTA_RAMP *p_ramp,
                               AHM_RAMP_CONFIG *p_cfg,
                               AHM_EVENT_MSG *p_msg)
{
    int delay_duration;
    uint16 *p_gain;
    uint16 current_gain;
    unsigned nominal_gain, full_gain;

    bool send_message = FALSE;

    /* Initialize the gain delta to 0 to set to a known state before
     * reinitializing the ramp.
     */
    p_ramp->p_gain->gain_delta = 0;

    p_gain = p_ramp->p_cur;

    current_gain = *p_gain;
    nominal_gain = (unsigned)p_cfg->nominal_gain;

    /* initialize old normal gain to current nominal gain */
    p_ramp->prev_nominal_gain = p_cfg->nominal_gain;


    if (current_gain == 0)
    {
        p_ramp->p_gain->gain_current = 0;
    }
    else
    {
        full_gain = (current_gain << AHM_DIV_PRECISION) / nominal_gain;
        p_ramp->p_gain->gain_current = full_gain;
    }

    if (p_cfg->target == 0)
    {
        p_ramp->target = 0;
    }
    else if (p_cfg->target == nominal_gain)
    {
        p_ramp->target = AHM_NOMINAL_TARGET;
    }
    else
    {
        full_gain = (p_cfg->target << AHM_DIV_PRECISION) / nominal_gain;
        p_ramp->target =  full_gain;
    }

    p_ramp->tc = ahm_calc_ramp_tc(p_cfg->duration, p_cfg->fast_rate);
    p_ramp->p_gain->tc_attack = p_ramp->tc;
    p_ramp->p_gain->tc_release = p_ramp->tc;

    /* If the timer and delay are both 0 then immediately set the gain value
     * to the target value.
     */
    if (p_cfg->duration == 0 && p_cfg->delay == 0)
    {
        /* Update current gain */
        *p_gain = (uint16)p_cfg->target;
        /* Update ramp gain */
        p_ramp->p_gain->gain = (uint8)p_cfg->target;
        if (p_ramp->target == AHM_NOMINAL_TARGET)
        {
            p_ramp->target = AHM_DELTA_NOMINAL;
        }
        p_ramp->p_gain->gain_delta = p_ramp->target;
        p_ramp->p_gain->gain_current = p_ramp->target;
        p_ramp->state = AHM_RAMP_FINISHED;
        return send_message;
    }

    if (p_cfg->target == current_gain)
    {
        /* Nothing to be done by the ramp */
        if (p_ramp->target == AHM_NOMINAL_TARGET)
        {
            p_ramp->target = AHM_DELTA_NOMINAL;
        }
        p_ramp->p_gain->gain_delta = p_ramp->target;
        p_ramp->p_gain->gain_current = p_ramp->target;
        p_ramp->state = AHM_RAMP_FINISHED;
        return send_message;
    }

    /* Calculate the ramp delay in process calls. The process is called at the
     * base kick period.
     */
    delay_duration = (p_cfg->delay * p_cfg->slow_rate) + AHM_DELAY_ROUNDING;
    delay_duration = delay_duration >> AHM_DELAY_PRECISION;
    p_ramp->delay = delay_duration;
    p_ramp->counter = delay_duration;

    if (delay_duration == 0)
    {
        p_ramp->p_gain->gain_delta = p_ramp->target;
        p_ramp->state = AHM_RAMP_RUNNING;
        p_msg->id = p_ramp->path;
        p_msg->type = AHM_EVENT_TYPE_TRIGGER;
        p_msg->payload = 0;
        send_message = TRUE;
    }
    else
    {
        p_ramp->state = AHM_RAMP_WAITING;
        /* Don't calculate a gain delta on this path until it's ready */
         if (p_ramp->p_gain->gain_delta == AHM_DELTA_NOMINAL)
        {
            p_ramp->p_gain->gain_delta = AHM_NOMINAL_TARGET;
        }
        else
        {
            p_ramp->p_gain->gain_delta = p_ramp->p_gain->gain_current;
        }
    }

    #if defined(DEBUG_AHM_RAMP)
    {
        L2_DBG_MSG3("OPID: %x, Ramp Init @ %d microseconds: ramp_target=%d",
                     ext_op_id, time_get_time(), p_ramp->target);
        L2_DBG_MSG4("OPID: %x, current_gain=%d, ramp_current_gain=0x%08X, ramp_delta=0x%08X",
                     ext_op_id, current_gain, p_ramp->p_gain->gain_current,
                     p_ramp->p_gain->gain_delta);
        
        L2_DBG_MSG4("OPID: %x, Ramp Path @ %d: old_nominal_gain=0x%08X, ramp_tc=0x%08X",
                     ext_op_id, p_ramp->path, p_ramp->prev_nominal_gain, p_ramp->tc);
        L2_DBG_MSG3("OPID: %x, ramp_delay=0x%08X, ramp_state=0x%08X",
                     ext_op_id, p_ramp->delay, p_ramp->state);
                     p_ramp->p_gain->gain_delta);
       }
    #endif
    return send_message;
}

bool ahm_process_delta_ramp(AHM_DELTA_RAMP *p_ramp,
                            AHM_EVENT_MSG *p_msg, uint16 nominal_gain)
{
    int *p_delta, cur_gain;
    
    unsigned full_gain, normal_gain ;
    int denormalize_gain;

    bool send_message = FALSE;


    switch (p_ramp->state)
    {
        case AHM_RAMP_INITIALIZED:
            p_ramp->state = AHM_RAMP_WAITING;
        case AHM_RAMP_WAITING:
            p_ramp->counter -= 1;
            if (p_ramp->counter <= 0)
            {
                p_ramp->p_gain->gain_delta = p_ramp->target;
                p_ramp->state = AHM_RAMP_RUNNING;
                p_msg->id = p_ramp->path;
                p_msg->type = AHM_EVENT_TYPE_TRIGGER;
                p_msg->payload = 0;
                send_message = TRUE;
            }
            break;
        case AHM_RAMP_RUNNING:

            if (p_ramp->prev_nominal_gain != nominal_gain)
            {
               /* Recompute current gain if nominal gain has changed */
               /* update old nominal gain to new nominal gain */

                /* convert Q 8.24 ramp target to unint16 */
                #if defined(DEBUG_AHM_RAMP)
                  L2_DBG_MSG5("OPID: %x, Ramp nominal gain update @%d microseconds: \
                               old_nominal_gain =0x%08X,\
                               new_nominal_gain=0x%08X", p_msg->ext_op_id, time_get_time(),
                               p_ramp->path, p_ramp->prev_nominal_gain,
                               nominal_gain);
                #endif
                denormalize_gain = ((p_ramp->p_gain->gain_current >> AHM_DELTA_GAIN_SHIFT)*(p_ramp->prev_nominal_gain));
                denormalize_gain += AHM_DELTA_ROUNDING;
                denormalize_gain = (uint16)(denormalize_gain >> AHM_DELTA_PRECISION);
                normal_gain = (unsigned) nominal_gain;
                full_gain = (denormalize_gain << AHM_DIV_PRECISION)/normal_gain;
                p_ramp->p_gain->gain_current = full_gain;
                p_ramp->prev_nominal_gain = nominal_gain;

            }
            p_delta = &p_ramp->p_gain->gain_delta;
            cur_gain = p_ramp->p_gain->gain_current;
            #if defined(DEBUG_AHM_RAMP)
            {
                L2_DBG_MSG3("OPID: %x, Ramp Running @ %d microseconds, cur_gain=0x%08X",
                             p_msg->ext_op_id, time_get_time(), p_ramp->p_gain->gain_current);

                L2_DBG_MSG4("OPID: %x, gain_delta=0x%08X, Delta =%d path =%d",
                             p_msg->ext_op_id, p_ramp->p_gain->gain_delta,
                             pl_abs(*p_delta-cur_gain), p_ramp->path);
            }
            #endif
            if (pl_abs(*p_delta - cur_gain) < AHM_RAMP_END_THRESH)
            {
                p_ramp->p_gain->gain_current = p_ramp->target;
                p_ramp->state = AHM_RAMP_FINISHED;

                if (*p_delta == AHM_NOMINAL_TARGET)
                {
                    *p_delta = AHM_DELTA_NOMINAL;
                }


            }
            break;
        case AHM_RAMP_FINISHED:
            p_msg->id = p_ramp->path;
            p_msg->type = AHM_EVENT_TYPE_CLEAR;
            p_msg->payload = 0;
            send_message = TRUE;
            p_ramp->state = AHM_RAMP_IDLE;

            break;
        default: /* Also covers AHM_RAMP_IDLE */
            break;
    }

    return send_message;
}
