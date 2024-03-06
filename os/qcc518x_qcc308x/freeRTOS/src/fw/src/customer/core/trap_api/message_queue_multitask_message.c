/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * A message structure that contains all tasks it's being delivered to.
 */

#include "message_queue_multitask_message.h"
#include "pmalloc/pmalloc.h"

message_queue_multitask_message_t *message_queue_multitask_message_create(
    const Task *tasks, uint8 num_tasks, MessageId id, Message app_message,
    MILLITIME delay_ms, const void *condition_addr,
    CONDITION_WIDTH condition_width)
{
    uint8 i;
    message_queue_multitask_message_t *multitask_message;
    size_t allocation_size = sizeof(*multitask_message) +
                             sizeof(multitask_message->task_message[0]) * (num_tasks - 1);

    multitask_message = pmalloc(allocation_size);

    message_queue_message_init(&multitask_message->message, id, app_message,
                               delay_ms, condition_addr, condition_width);
    for (i = 0; i < num_tasks; ++i)
    {
        message_queue_task_message_init(&multitask_message->task_message[i],
                                        tasks[i], &multitask_message->message);
    }
    message_queue_message_release(&multitask_message->message);

    return multitask_message;
}
