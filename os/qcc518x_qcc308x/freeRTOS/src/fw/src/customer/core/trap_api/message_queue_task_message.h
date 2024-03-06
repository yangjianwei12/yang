/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Represents a message along with a single destination task.
 */

#ifndef MESSAGE_QUEUE_TASK_MESSAGE_H_
#define MESSAGE_QUEUE_TASK_MESSAGE_H_

#include "message_queue_message.h"

/**
 * A structure for associating a message with a single task.
 */
typedef struct message_queue_task_message_
{
    /**
     * Task messages are placed on a message queue.
     * This is the link for the next task message.
     */
    struct message_queue_task_message_ *next;

    /**
     * The task this message is to be delivered to.
     * This is either the MessageQueue handle value for FreeRTOS style message
     * queues, or the Task value for a VM task destination.
     */
    Task task;

    /**
     * A pointer to the message.
     * This message is reference counted, its reference is incremented each time
     * one of these structures is created, and decremented when this structure
     * is destroyed.
     */
    message_queue_message_t *message;
} message_queue_task_message_t;

/**
 * \brief Initialise a task message pair for entering into a message queue.
 *
 * Increments the reference count of \p message.
 *
 * \param [in,out] task_message  A pointer to the task message pair to
 * initialise.
 * \param [in] task  The destination for this message.
 * \param [in] message  A pointer to the message, must be non-NULL.
 */
void message_queue_task_message_init(message_queue_task_message_t *task_message,
                                     Task task, message_queue_message_t *message);

/**
 * \brief Destroy a task message pair.
 *
 * Decrements the reference count of \p message.
 *
 * \param [in] task_message The message to destroy
 * if set to NULL the function returns immediately.
 */
void message_queue_task_message_destroy(
    message_queue_task_message_t *task_message);

/**
 * \brief Test whether these task messages are similar.
 *
 * Task messages are similar if their tasks are equal and their messages are
 * similar, \see message_queue_message_similar().
 *
 * \param [in] queued  An existing task message to compare a new message
 *  against.
 * \param [in] new_task_message  The new task message pair
 *
 * \return TRUE if the task messages are similar, FALSE otherwise.
 */
bool message_queue_task_message_similar(
    const message_queue_task_message_t *queued,
    const message_queue_task_message_t *new_task_message);

/**
 * \brief Replace a message with a new message if they match.
 *
 * Task messages match if their tasks match and their messages match, \see
 * message_queue_message_replace().
 *
 * This function does not alter any message reference counts. It is the caller's
 * responsibility to ensure the message reference count is decremented if
 * replacing means that the messaage is no longer needed.
 *
 * \param [in,out] queued  The existing task message to potentially replace.
 * \param [in] new_task_message   The new task message pair.
 *
 * \return TRUE if \p queue was replaced by \p new_task_message,
 * FALSE otherwise.
 */
bool message_queue_task_message_replace(
    message_queue_task_message_t *queued,
    const message_queue_task_message_t *new_task_message);

/**
 * \brief Execute the task handler for this VM task message.
 *
 * Must only be called for task messages whose task is a VM task, i.e. is a
 * function pointer handler rather than a message_queue_t *.
 *
 * Returns silently if the task message's task or the task message's task
 * handler is NULL.
 *
 * \param [in] task_message  A pointer to the task message to execute.
 */
void message_queue_task_message_execute(
    message_queue_task_message_t *task_message);

/**
 * \brief Log an event for a task message to the trap API message log.
 *
 * \param [in] action  The action to log.
 * \param [in] task_message  The task message to log.
 */
void message_queue_task_message_log(
    TRAP_API_LOG_ACTION action,
    const message_queue_task_message_t *task_message);

#endif /* !MESSAGE_QUEUE_TASK_MESSAGE_H_ */
