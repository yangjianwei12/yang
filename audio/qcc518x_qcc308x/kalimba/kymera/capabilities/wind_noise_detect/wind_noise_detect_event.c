/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  wind_noise_detect_event.c
 * \ingroup wind_noise_detect
 *
 * Wind Noise Detect (WND) operator event processing.
 *
 */

/****************************************************************************
Include Files
*/

#include "wind_noise_detect_event.h"
#include "wind_noise_detect_gen_c.h"

/******************************************************************************
Private Function Definitions
*/

/******************************************************************************
 * Public Function Definitions
*/

/**
 * \brief  Setup the wind detection payload
 *
 * \param  p_event          Pointer to the wind event to process
 *
 * \return - TRUE if successful
 */
void wnd_setup_event_payload(WND_EVENT *p_event)
{
    AHM_EVENT_MSG *p_msg = &p_event->msg;
    int msg_power = (*p_event->p_pwr_level) & 0xFFFF0000;
    int msg_intensity = (*p_event->p_intensity) & 0x0000FFFF;

    p_msg->payload = msg_power | msg_intensity;

    switch (*p_event->p_mode)
    {
        case WIND_NOISE_DETECT_SYSMODE_1MIC:
            p_msg->id = AHM_EVENT_ID_WND_1MIC;
            break;
        case WIND_NOISE_DETECT_SYSMODE_2MIC:
            p_msg->id = AHM_EVENT_ID_WND_2MIC;
            break;
        default:
            break;
    }

    return;
}

bool wnd_process_event(WND_EVENT *p_event, EXT_OP_ID ext_op_id)
{
    bool ret_val = FALSE;
    unsigned detection = *p_event->p_detect;

    switch (p_event->state)
    {
        /* Initial state or after release message or if wind is not detected for
         * long enough during counting. Reset the counters and watch for a
         * detection to start counting.
         */
        case WND_EVENT_RELEASE:
            p_event->counter = p_event->attack_reset_count;
            if (detection > 0)
            {
                p_event->state = WND_EVENT_ATTACK_COUNT;
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of wind in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_ATTACK;
                }
            }
            break;
        /* Hold counter for a wind condition. If the counter reaches
         * 2 (counter frames + 2 for the state machine) then move to send the
         * message. Otherwise if wind is not detected return to the release
         * state.
         */
        case WND_EVENT_ATTACK_COUNT:
            if (detection > 0)
            {
                if (p_event->counter > 2)
                {
                    p_event->counter -= 1;
                }
                else
                {
                    p_event->state = WND_EVENT_ATTACK_MESSAGE;
                }
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of wind in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_ATTACK;
                }
                else
                {
                    p_event->state = WND_EVENT_RELEASE;
                }
            }
            break;
        /* Send the attack unsolicited message and move to the ATTACK state. */
        case WND_EVENT_ATTACK_MESSAGE:
            /* Final frame count check */
            if (detection > 0)
            {
                wnd_setup_event_payload(p_event);
                p_event->msg.type = WND_EVENT_TYPE_ATTACK;
                ret_val = TRUE;
                p_event->state = WND_EVENT_ATTACK;
                p_event->confirm = FALSE;
            }
            else
            {
                /* If a confirmation message is required move to ATTACK
                 * because a lack of wind in this condition will move
                 * through the release cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_ATTACK;
                }
                else
                {
                    p_event->state = WND_EVENT_RELEASE;
                }
            }
            break;
        /* Initial state or after attack message or if wind is detected during
         * release counting. Reset the counters and watch for no detection to
         * start counting.
         */
        case WND_EVENT_ATTACK:
            p_event->counter = p_event->release_reset_count;
            if (detection == 0)
            {
                p_event->state = WND_EVENT_RELEASE_COUNT;
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because wind in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_RELEASE;
                }
            }
            break;
        /* Hold counter for a no wind condition. If the counter reaches
         * 2 (counter frames + 2 for the state machine) then move to send the
         * message. Otherwise if wind is not detected return to the ATTACK
         * state.
         */
        case WND_EVENT_RELEASE_COUNT:
            if (detection == 0)
            {
                if (p_event->counter > 2)
                {
                    p_event->counter -= 1;
                }
                else
                {
                    p_event->msg.type = WND_EVENT_TYPE_RELEASE;
                    p_event->state = WND_EVENT_RELEASE_MESSAGE;
                }
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because wind in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_RELEASE;
                }
                else
                {
                    p_event->state = WND_EVENT_ATTACK;
                }
            }
            break;
        case WND_EVENT_RELEASE_MESSAGE:
            /* Final frame count check */
            if (detection == 0)
            {
                wnd_setup_event_payload(p_event);
                p_event->msg.type = WND_EVENT_TYPE_RELEASE;
                ret_val = TRUE;
                p_event->state = WND_EVENT_RELEASE;
                p_event->confirm = FALSE;
            }
            else
            {
                /* If a confirmation message is required move to RELEASE
                 * because wind in this condition will move through the attack
                 * cycle and send a message.
                 */
                if (p_event->confirm)
                {
                    p_event->state = WND_EVENT_RELEASE;
                }
                else
                {
                    p_event->state = WND_EVENT_ATTACK;
                }
            }
            break;
        default:
            L2_DBG_MSG2("OPID: %x, WND: unhandled event state %d", ext_op_id, p_event->state);
            break;
    }

    return ret_val;
}

void wnd_setup_event(WND_EVENT *p_event,
                     unsigned attack_duration,
                     unsigned release_duration)
{
    unsigned rate;

    rate = attack_duration * WND_DEFAULT_FRAME_RATE;
    p_event->attack_reset_count = rate >> WND_TIMER_PARAM_SHIFT;

    rate = release_duration * WND_DEFAULT_FRAME_RATE;
    p_event->release_reset_count = rate >> WND_TIMER_PARAM_SHIFT;

    return;
}
