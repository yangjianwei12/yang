/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Implementation of a message for a message queue.
 */

#include <assert.h>

#include "int/int.h"
#include "ipc/ipc.h"
#include "longtimer/longtimer.h"
#include "message_queue_message.h"
#include "pmalloc/pmalloc.h"

void message_queue_message_init(message_queue_message_t *message, MessageId id,
                                Message app_message, MILLITIME delay_ms,
                                const void *condition_addr,
                                CONDITION_WIDTH condition_width)
{
    message->due_ms = time_add(delay_ms, get_milli_time());
    message->condition_addr = condition_addr;
    message->condition_width = condition_width;
    message->id = id;
    message->app_message = app_message;
    message->refcount = 1;
}

void message_queue_message_retain(message_queue_message_t *message)
{
    assert(message);

    block_interrupts();
    {
        assert(message->refcount > 0);
        assert(message->refcount < 127);
        ++message->refcount;
    }
    unblock_interrupts();
}

void message_queue_message_release(message_queue_message_t *message)
{
    bool free_message;

    assert(message);

    block_interrupts();
    {
        assert(message->refcount > 0);
        --message->refcount;
        free_message = (0 == message->refcount);
    }
    unblock_interrupts();

    if(free_message)
    {
        message_queue_message_app_free(message->id, message->app_message);
        pfree(message);
    }
}

bool message_queue_message_similar(
    const message_queue_message_t *queued_message,
    const message_queue_message_t *new_message)
{
    assert(queued_message);
    assert(new_message);

    if(queued_message->id != new_message->id)
    {
        return FALSE;
    }

    switch(queued_message->id)
    {
    case MESSAGE_MORE_DATA:
    {
        const MessageMoreData *ma =
            (const MessageMoreData *) queued_message->app_message;
        const MessageMoreData *mb =
            (const MessageMoreData *) new_message->app_message;
        return ma->source == mb->source;
    }
    case MESSAGE_MORE_SPACE:
    {
        const MessageMoreSpace *ma =
            (const MessageMoreSpace *) queued_message->app_message;
        const MessageMoreSpace *mb =
            (const MessageMoreSpace *) new_message->app_message;
        return ma->sink == mb->sink;
    }
    case MESSAGE_PSFL_FAULT:
    case MESSAGE_TX_POWER_CHANGE_EVENT:
    {
        return TRUE;
    }
    default:
        break;
    }

    return FALSE;
}

bool message_queue_message_replace(message_queue_message_t *queued_message,
                                   const message_queue_message_t *new_message)
{
    assert(queued_message);
    assert(new_message);

    switch(queued_message->id)
    {
    case MESSAGE_USB_SUSPENDED:
        if(queued_message->id == new_message->id)
        {
            MessageUsbSuspended *ma =
                MESSAGE_REMOVE_CONST(queued_message->app_message);
            const MessageUsbSuspended *mb =
                (const MessageUsbSuspended *) new_message->app_message;
            *ma = *mb;
            return TRUE;
        }
        break;

    case MESSAGE_USB_ENUMERATED:
    case MESSAGE_USB_DECONFIGURED:
        switch(new_message->id)
        {
            /* This code is much reduced by giving the deconfigured message
             * the same kind of payload as the enumerated message. If the
             * deconfigured message didn't have a payload, you would have to
             * deal separately with the different permutations. */
        case MESSAGE_USB_ENUMERATED:
        case MESSAGE_USB_DECONFIGURED:
            {
                MessageUsbConfigValue *ma =
                    MESSAGE_REMOVE_CONST(queued_message->app_message);
                const MessageUsbConfigValue *mb = new_message->app_message;
                /* copy across payload and update id */
                *ma = *mb;
                queued_message->id = new_message->id;
                return TRUE;
            }
        default:
            break;
        }
        break;

    case MESSAGE_USB_ATTACHED:
    case MESSAGE_USB_DETACHED:
        switch(new_message->id)
        {
        case MESSAGE_USB_ATTACHED:
        case MESSAGE_USB_DETACHED:
            /* No payload, just overwrite id */
            queued_message->id = new_message->id;
            return TRUE;
        default:
            break;
        }
        break;

    case MESSAGE_USB_ALT_INTERFACE:
        if(queued_message->id == new_message->id)
        {
            MessageUsbAltInterface *ma =
                MESSAGE_REMOVE_CONST(queued_message->app_message);
            const MessageUsbAltInterface *mb = new_message->app_message;

            if(ma->interface == mb->interface)
            {
                *ma = *mb;
                return TRUE;
            }
        }
        break;

    default:
        break;
    }
    return FALSE;
}

bool message_queue_message_condition_satisfied(
    const message_queue_message_t *message)
{
    assert(message);

    /* The behaviour of NULL condition addresses is undocumented but accepted
       and treated as an immediately satisfied condition in other firmware
       versions. */
    if(NULL == message->condition_addr)
    {
        return TRUE;
    }

    switch(message->condition_width)
    {
    case CONDITION_WIDTH_16BIT:
        return 0 == *(const uint16 *)message->condition_addr;
    case CONDITION_WIDTH_32BIT:
        return 0 == *(const uint32 *)message->condition_addr;
    default:
        return TRUE;
    }
}

void message_queue_message_app_free(MessageId id, Message app_message)
{
    void *nonconst_message = MESSAGE_REMOVE_CONST(app_message);
    if(MESSAGE_BLUESTACK_BASE_ <= id && id < MESSAGE_BLUESTACK_END_)
    {
        /* Notify P0's bluestack stream tracking logic that the App has seen
         * this primitive. It also gets freed at that point. */
        IPC_BLUESTACK_PRIM prim;
        prim.protocol = (uint16)(id - MESSAGE_BLUESTACK_BASE_);
        prim.prim = nonconst_message;
        ipc_send(IPC_SIGNAL_ID_BLUESTACK_PRIM_RECEIVED, &prim, sizeof(prim));
    }
    else if(id == MESSAGE_MORE_SPACE || id == MESSAGE_MORE_DATA)
    {
        /* Notify P0's bluestack stream that the App has seen this message.*/
        IPC_APP_MESSAGE_RECEIVED rcvd_msg;
        rcvd_msg.id = id;
        rcvd_msg.msg = nonconst_message;
        ipc_send(IPC_SIGNAL_ID_APP_MESSAGE_RECEIVED, &rcvd_msg,
                 sizeof(rcvd_msg));
    }
    else
    {
        pfree(nonconst_message);
    }
}

INTERVAL message_queue_message_due_in_ms(const message_queue_message_t *message,
                                         MILLITIME now_ms)
{
    assert(message);
    return time_sub(message->due_ms, now_ms);
}
