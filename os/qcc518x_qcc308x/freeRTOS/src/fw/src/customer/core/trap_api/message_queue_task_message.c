/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Implementation for an object associating a message with a single task.
 */

#include "assert.h"
#include "message_queue_task_message.h"
#include "message_router.h"

void message_queue_task_message_init(message_queue_task_message_t *task_message,
                                     Task task,
                                     message_queue_message_t *message)
{
    assert(task_message);
#ifdef PANIC_ON_VM_MESSAGE_NULL_TASK_LIST
    assert(task);
#endif /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */
    assert(message);

    task_message->next = NULL;
    task_message->task = task;
    task_message->message = message;
    message_queue_message_retain(message);
}

void message_queue_task_message_destroy(
    message_queue_task_message_t *task_message)
{
    if(NULL != task_message)
    {
        message_queue_task_message_log(TRAP_API_LOG_FREE, task_message);
        message_queue_message_release(task_message->message);
    }
}

bool message_queue_task_message_similar(
    const message_queue_task_message_t *queued,
    const message_queue_task_message_t *new_task_message)
{
    assert(queued);
    assert(new_task_message);

    if (queued->task != new_task_message->task)
    {
        return FALSE;
    }

    return message_queue_message_similar(queued->message,
                                         new_task_message->message);
}

bool message_queue_task_message_replace(
    message_queue_task_message_t *queued,
    const message_queue_task_message_t *new_task_message)
{
    assert(queued);
    assert(new_task_message);

    if (queued->task != new_task_message->task)
    {
        return FALSE;
    }

    return message_queue_message_replace(queued->message,
                                         new_task_message->message);
}

void message_queue_task_message_execute(
    message_queue_task_message_t *task_message)
{
    assert(task_message);

#ifdef PANIC_ON_VM_MESSAGE_NULL_TASK_LIST
    assert(task_message->task);
#else /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */
    if(task_message->task && task_message->task->handler)
#endif /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */
    {
        VALIDATE_FN_PTR(task_message->task->handler);
        task_message->task->handler(task_message->task,
                                    task_message->message->id,
                                    task_message->message->app_message);
    }
}

void message_queue_task_message_log(
    TRAP_API_LOG_ACTION action,
    const message_queue_task_message_t *task_message)
{
    trap_msg_header_t header;
    const Task task = task_message->task;
    const message_queue_message_t *message = task_message->message;

    /* Action */
    U8_OFF0_SET(header.action, action);

    /* Task */
    U32_OFF1_SET(header.task, (uint32)task);

    /* Handler */
    {
        uint32 handler = 0;
        if(NULL != task &&
           message_router_handle_routes_to_default((MessageQueue)task))
        {
            handler = (uint32)task->handler;
        }
        U32_OFF1_SET(header.handler, handler);
    }

    /* Message ID */
    {
        uint16 id = message->id;
        U16_OFF1_SET(header.id, id);
    }

    /* Condition Address */
    {
        uint32 condition_address = (uint32)message->condition_addr;
        U32_OFF1_SET(header.condition_address, condition_address);
    }

    /* Due (ms) */
    {
        uint32 due_ms = message->due_ms;
        U32_OFF1_SET(header.due_ms, due_ms);
    }

    trap_api_multitask_message_log(&header, message->app_message);
}
