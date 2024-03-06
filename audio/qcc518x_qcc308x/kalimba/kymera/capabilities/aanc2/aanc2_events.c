/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  aanc_events.c
 * \ingroup aanc2
 *
 * AANC2 events library.
 */

#include "aanc2_events.h"

/******************************************************************************
Public Function Definitions
*/

bool aanc2_send_event_trigger(AANC2_EVENT_MSG *p_msg)
{
    unsigned msg_size;
    unsigned *trigger_message;
    unsigned message_id;

    /* Set the message ID */
    switch (p_msg->type)
    {
        case AANC2_EVENT_MSG_NEGATIVE_TRIGGER:
            message_id = OPMSG_REPLY_ID_AANC_EVENT_NEGATIVE_TRIGGER;
            break;
        default: /* includes AANC2_EVENT_MSG_POSITIVE_TRIGGER */
            message_id = OPMSG_REPLY_ID_AANC_EVENT_TRIGGER;
    }

    /* Populate the message payload */
    msg_size = OPMSG_UNSOLICITED_AANC_EVENT_TRIGGER_WORD_SIZE;
    trigger_message = xpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        L2_DBG_MSG1("OPID: %x, Failed to send AANC event message", p_msg->opid);
        return FALSE;
    }

    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AANC_EVENT_TRIGGER,
                             ID,
                             p_msg->id);
    OPMSG_CREATION_FIELD_SET(trigger_message,
                             OPMSG_UNSOLICITED_AANC_EVENT_TRIGGER,
                             PAYLOAD,
                             p_msg->payload);
    OPMSG_CREATION_FIELD_SET32(trigger_message,
                               OPMSG_UNSOLICITED_AANC_EVENT_TRIGGER,
                               OPID,
                               p_msg->opid); 
    common_send_unsolicited_message(p_msg->op_data,
                                    message_id,
                                    msg_size,
                                    trigger_message);

    pdelete(trigger_message);

    return TRUE;
}

void aanc2_process_event_clear_condition(AANC2_EVENT *p_event)
{
    switch (p_event->running)
    {
        case AANC2_EVENT_CLEAR:
            /* Clear needs to fall through so that initialization behavior
                is correct.
            */
        case AANC2_EVENT_DETECTED:
            /* Have detected but not sent message so clear */
            aanc2_clear_event(p_event);
            break;
        case AANC2_EVENT_SENT:
            /* Send a negative trigger message and clear the event */
            p_event->msg.type = AANC2_EVENT_MSG_NEGATIVE_TRIGGER;
            aanc2_send_event_trigger(&p_event->msg);
            aanc2_clear_event(p_event);
            break;
    }
}

void aanc2_process_event_detect_condition(AANC2_EVENT *p_event)
{
    switch (p_event->running)
    {
        case AANC2_EVENT_CLEAR:
            /* Start the event counter */
            p_event->frame_counter -= 1;
            p_event->running = AANC2_EVENT_DETECTED;
            break;
        case AANC2_EVENT_DETECTED:
            /* Increment the event counter */
            if (p_event->frame_counter > 0)
            {
                p_event->frame_counter -= 1;
            }
            else
            {
                /* Send the event positive trigger message */
                p_event->msg.type = AANC2_EVENT_MSG_POSITIVE_TRIGGER;
                aanc2_send_event_trigger(&p_event->msg);
                p_event->running = AANC2_EVENT_SENT;
            }
            break;
        case AANC2_EVENT_SENT:
            break;
    }
}

void aanc2_initialize_event(AANC2_EVENT *p_event,
                            OPERATOR_DATA *op_data,
                            unsigned timer_duration,
                            uint16 id)
{
    unsigned frame_rate = timer_duration * AANC2_FRAME_RATE;
    p_event->set_frames = frame_rate >> AANC2_TIMER_PARAM_SHIFT;
    p_event->msg.op_data = op_data;
    p_event->msg.payload = 0;
    p_event->msg.id = id;
    p_event->msg.opid = INT_TO_EXT_OPID(op_data->id);
    aanc2_process_event_clear_condition(p_event);
}

void aanc2_process_event(AANC2_EVENT *p_event,
                         unsigned cur_state,
                         unsigned prev_state)
{
    p_event->msg.payload = (uint16)cur_state;
    if (cur_state > 0)
    {
        /* Non-zero flags and no change starts/continues event */
        if (cur_state == prev_state)
        {
            aanc2_process_event_detect_condition(p_event);
        }
    }
    else
    {
        /* Flags reset causes event to be reset */
        if (cur_state != prev_state)
        {
            aanc2_process_event_clear_condition(p_event);
        }
    }
}
