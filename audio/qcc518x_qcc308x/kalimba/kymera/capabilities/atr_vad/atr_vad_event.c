/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  atr_vad_event.c
 * \ingroup atr_vad
 *
 * Auto Transparency VAD (ATR_VAD) operator event processing.
 *
 */

/****************************************************************************
Include Files
*/

#include "atr_vad_event.h"
#include "atr_vad_gen_c.h"

/******************************************************************************
Private Function Definitions
*/

/**
 * \brief  Setup the atr_vad detection payload
 *
 * \param  p_event          Pointer to the event to process
 *
 * \return - TRUE if successful
 */
static void atr_vad_setup_event_payload(ATR_VAD_EVENT *p_event)
{
    AHM_EVENT_MSG *p_msg = &p_event->msg;

    switch (*p_event->p_mode)
    {
        case ATR_VAD_SYSMODE_1MIC:
        case ATR_VAD_SYSMODE_1MIC_MS:
            p_msg->id = AHM_EVENT_ID_ATR_VAD_1MIC;
            break;
        case ATR_VAD_SYSMODE_2MIC:
            p_msg->id = AHM_EVENT_ID_ATR_VAD_2MIC;
            break;
        default:
            break;
    }

    return;
}

/******************************************************************************
Public Function Definitions
*/

bool atr_vad_process_event(ATR_VAD_EVENT *p_event)
{
    bool ret_val = FALSE;

    unsigned detection = *p_event->p_detect;

    switch (p_event->state)
    {
        /* Initial state or after release message or if speech is not detected
         * for long enough during counting. Reset the counters and watch for a
         * detection to start counting.
         */
        case ATR_VAD_EVENT_RELEASE:
            p_event->counter = p_event->attack_reset_count;
            if (detection > 0)
            {
                p_event->state = ATR_VAD_EVENT_ATTACK_COUNT;
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of speech in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK;
                }
            }
            break;
        /* Hold counter for a speech condition. If the counter reaches
         * 2 (counter frames + 2 for the state machine) then move to send the
         * message. Otherwise if speech is not detected return to the release
         * state.
         */
        case ATR_VAD_EVENT_ATTACK_COUNT:
            if (detection > 0)
            {
                if (p_event->counter > 2)
                {
                    p_event->counter -= 1;
                }
                else
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK_MESSAGE;
                }
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of speech in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK;
                }
                else
                {
                    p_event->state = ATR_VAD_EVENT_RELEASE;
                }
            }
            break;
        /* Send the attack unsolicited message and move to the ATTACK state. */
        case ATR_VAD_EVENT_ATTACK_MESSAGE:
            /* Final frame count check */
            if (detection > 0)
            {
                atr_vad_setup_event_payload(p_event);
                p_event->msg.type = ATR_VAD_EVENT_TYPE_ATTACK;
                ret_val = TRUE;
                p_event->state = ATR_VAD_EVENT_ATTACK;
                p_event->confirm = FALSE;
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of speech in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK;
                }
                else
                {
                    p_event->state = ATR_VAD_EVENT_RELEASE;
                }
            }
            break;
        /* Initial state or after attack message or if speech is detected during
         * release counting. Reset the counters and watch for no detection to
         * start counting.
         */
        case ATR_VAD_EVENT_ATTACK:
            p_event->counter = p_event->release_reset_count;
            if (detection == 0)
            {
                p_event->state = ATR_VAD_EVENT_RELEASE_COUNT;
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because speech in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_RELEASE;
                }
            }
            break;
        /* Hold counter for a no speech condition. If the counter reaches
         * 2 (counter frames + 2 for the state machine) then move to send the
         * message. Otherwise if speech is not detected return to the ATTACK
         * state.
         */
        case ATR_VAD_EVENT_RELEASE_COUNT:
            if (detection == 0)
            {
                if (p_event->counter > 2)
                {
                    p_event->counter -= 1;
                }
                else
                {
                    p_event->msg.type = ATR_VAD_EVENT_TYPE_RELEASE;
                    p_event->state = ATR_VAD_EVENT_RELEASE_MESSAGE;
                }
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because speech in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_RELEASE;
                }
                else
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK;
                }
            }
            break;
        case ATR_VAD_EVENT_RELEASE_MESSAGE:
            /* Final frame count check */
            if (detection == 0)
            {
                if (p_event->notify_release)
                {
                    atr_vad_setup_event_payload(p_event);
                    p_event->msg.type = ATR_VAD_EVENT_TYPE_RELEASE;
                    ret_val = TRUE;
                }
                p_event->state = ATR_VAD_EVENT_RELEASE;
                p_event->confirm = FALSE;
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because speech in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = ATR_VAD_EVENT_RELEASE;
                }
                else
                {
                    p_event->state = ATR_VAD_EVENT_ATTACK;
                }
            }
            break;
        default:
            L2_DBG_MSG1("ATR_VAD: unhandled event state %d", p_event->state);
            break;
    }

    return ret_val;
}

void atr_vad_setup_event(ATR_VAD_EVENT *p_event, ATR_VAD_RELEASE release_select)
{
    unsigned rate;
    ATR_VAD_EVENT_CONFIG *p_config = &p_event->config;

    rate = p_config->attack_time * ATR_VAD_DEFAULT_FRAME_RATE;
    p_event->attack_reset_count = rate >> ATR_VAD_TIMER_PARAM_SHIFT;

    /* Default to sending a notification for release */
    p_event->notify_release = TRUE;

    switch (release_select)
    {
        case ATR_VAD_RELEASE_NONE:
            /* Configure for no release message */
            p_event->notify_release = FALSE;
            rate = 0;
            break;
        case ATR_VAD_RELEASE_SHORT:
            rate = p_config->short_release_time * ATR_VAD_DEFAULT_FRAME_RATE;
            break;
        case ATR_VAD_RELEASE_LONG:
            rate = p_config->long_release_time * ATR_VAD_DEFAULT_FRAME_RATE;
            break;
        case ATR_VAD_RELEASE_NORMAL:
        default:
            rate = p_config->normal_release_time * ATR_VAD_DEFAULT_FRAME_RATE;
            break;
    }

    p_event->release_reset_count = rate >> ATR_VAD_TIMER_PARAM_SHIFT;

    return;
}