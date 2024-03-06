/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * A message structure that contains all tasks it's being delivered to.
 */

#ifndef MESSAGE_QUEUE_MULTITASK_MESSAGE_H_
#define MESSAGE_QUEUE_MULTITASK_MESSAGE_H_

#include "message_queue_message.h"
#include "message_queue_task_message.h"

/**
 * A message with multiple destinations.
 */
typedef struct message_queue_multitask_message_
{
    /** A single messaged shared for all delivery destinations. */
    message_queue_message_t message;

    /** A flexible length array of delivery destinations, set to 1 as C89
        doesn't support 0 length arrays or flexible array members, must be
        taken into account when calculating the size of the memory to allocate
        for this structure. */
    message_queue_task_message_t task_message[1];
} message_queue_multitask_message_t;

/**
 * \brief Creates a message that can be sent to multiple queues.
 *
 * Uses a single memory allocation to reduce the number of calls to pmalloc and
 * pfree when delivering messages.
 *
 * \note a multitask message is a message, the first element of the structure
 * is a message_queue_message_t. This structure is freed using
 * message_queue_message_release.
 *
 * \param [in] tasks  Pointer to an array of tasks to send the message to.
 * \param [in] num_tasks  The number of tasks in \p tasks.
 * \param [in] id  The message ID.
 * \param [in] app_message  The message as provided by the application.
 * \param [in] delay_ms  The number of milliseconds to wait before delivering
 * the message.
 * \param [in] condition_addr  Optional pointer to a condition which must be
 * zero for the message to be delivered.
 * \param [in] condition_width  Whether to test the condition as a 16 or 32-bit
 * variable, or \c CONDITION_WIDTH_UNUSED if \p condition_addr is NULL.
 *
 * \return A pointer to a newly allocated multitask message. Panics on failure.
 */
message_queue_multitask_message_t *message_queue_multitask_message_create(
    const Task *tasks, uint8 num_tasks, MessageId id, Message app_message,
    MILLITIME delay_ms, const void *condition_addr,
    CONDITION_WIDTH condition_width);

#endif /* !MESSAGE_QUEUE_MULTITASK_MESSAGE_H_ */
