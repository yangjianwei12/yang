/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * This is the interface for the multitasking messaging implementation that sits
 * on top of FreeRTOS. This is called internally by the Qualcomm Hydra VM
 * implementation and by the trap API code.
 */

#ifndef MESSAGING_H_
#define MESSAGING_H_

/**
 * The structure of the Trap API messaging module is shown below.
 *
 * - Message queues are a time ordered list of undelivered messages.
 * - Each 'Task Message' in the queue is for one specific task.
 *      - If the Task is a VM task then this value may vary depending on what
 *        the handler is.
 *      - If the Task is a MessageQueue then each 'Task Message' in a queue will
 *        have the same value.
 * - Each 'Task Message' references a 'Message'. Messages are reference counted
 *   so can be delivered to many tasks for supporting multicast.
 * - Each 'Message' references one 'App Message' that the application provided.
 *
 *                                     +-----------------------------------+
 *                                     | Multitask Message                 |
 *                                     |                                   |
 *                                     | message_queue_multitask_message.h |
 *                                     |                                   |
 *                                     | A single allocation containing a  |--.
 *                                     | message and as many task messages |  |
 *                                     | as necessary.                     |  |
 *                                     +-----------------------------------+  |
 *                                                    1                       |
 *                                                    |                       |
 *                                                   <> 1..n  has many        |
 *  +---------------------+            +------------------------------+       |
 *  | Message Queue       |            | Task Message                 |       |
 *  |                     |-queued---->|                              |       |
 *  | message_queue.h     |            | message_queue_task_message.h |       |
 *  |                     |            |                              |       |
 *  | A queue of messages |            | A message + destination task.|0..1-+ |
 *  +---------------------+            +------------------------------+     | |
 *                                   0..*             0..*       0..1       | |
 *                                   /                |           ^ next----+ |
 *                                  V                 V                       |
 *                                 1                  1                       |
 *  +--------------------------------+ +------------------------------+       |
 *  | Task                           | | Message                      |       |
 *  |                                | |                              |  is a |
 *  | message.h                      | | message_queue_message.h      |<|-----.
 *  |                                | |                              |
 *  | The application specified task.| | A reference counted message, |
 *  | This could be a pointer to VM  | | could be delivered to many   |
 *  | TaskData containing a function | | tasks.                       |
 *  | pointer to handle the message, | +------------------------------+
 *  | or a MessageQueue.             |               1
 *  +--------------------------------+               |
 *                                                   V
 *                                                   1
 *                                     +------------------------------+
 *                                     | App Message                  |
 *                                     |                              |
 *                                     | message.h                    |
 *                                     |                              |
 *                                     | The application's pmalloc    |
 *                                     | allocated message.           |
 *                                     +------------------------------+
 */

#include <message.h>

#include "hydra/hydra_types.h"
#include "message_queue.h"

/**
 * \brief Sends a message to a list of tasks.
 *
 * \param[in] tasks  An array of Tasks.
 * \param[in] num_tasks  The number of Tasks in the \p tasks array.
 * \param[in] id  The ID of the message.
 * \param[in] app_message  A pmalloc pointer to the message contents.
 * \param[in] delay_ms  The number of milliseconds to wait before delivering the
 * message.
 * \param[in] condition_addr  Optional pointer to a condition which must be zero
 * for the message to be delivered.
 * \param[in] condition_width  Whether to test the condition as a 16 or 32-bit
 * variable, or \c CONDITION_WIDTH_UNUSED if \p condition_addr is NULL.
 */
void messaging_send(const Task *tasks, uint8 num_tasks, MessageId id,
                    Message app_message, MILLITIME delay_ms,
                    const void *condition_addr,
                    CONDITION_WIDTH condition_width);

/**
 * \brief Create the message queue for the VM task.
 *
 * The VM message queue is unique in that the application delivers to it using
 * a Task rather than a message queue handle.
 *
 * This function should only be called once and during initialisation if the VM
 * task.
 *
 * This function will panic if the queue cannot be created.
 *
 * \return A pointer to the VM message queue. Never returns NULL.
 */
message_queue_t *messaging_queue_create_vm(void);

/**
 * \brief Create a message queue.
 *
 * \return A handle to the newly created message queue. This can be passed to
 * the Task parameter of message related trap API functions. Returns 0 if there
 * is a failure to allocate memory.
 */
MessageQueue messaging_queue_create(void);

/**
 * \brief Delete a message queue.
 *
 * \param[in] queue  The message queue to delete.
 */
void messaging_queue_destroy(MessageQueue queue);

/**
 * \brief Waits on a message queue until a message is delivered.
 *
 * Only one task should be calling this on a particular queue at any time.
 *
 * \param[in]  queue  The queue to wait on.
 * \param[out] app_message  Memory for returning the message to the application.
 *
 * \return The message ID.
 */
MessageId messaging_queue_wait(MessageQueue queue, Message *app_message);

/**
 * \brief Increment the reference count of a message delivered to the
 * application.
 *
 * \param[in] id  The message ID.
 * \param[in] app_message  A pointer to the message to refcount. Must not be
 * NULL.
 */
void messaging_retain(MessageId id, Message app_message);

/**
 * \brief Free a message that was delivered to the application.
 *
 * \param[in] id  The message ID.
 * \param[in] app_message  A pointer to the message to free.
 */
void messaging_free(MessageId id, Message app_message);

/**
 * \brief Count the number of pending messages for a task.
 *
 * If \p task is set to a MessageQueue then the number of pending messages the
 * queue is returned, otherwise the task is interpreted as a VM task and the VM
 * queue is searched for the appropriate VM task.
 *
 * \param[in] task  The VM task or MessageQueue to count the messages for.
 * \param[out] first_due_in_ms  The number of milliseconds in the future that
 * the first message for this task is due to be delivered in.
 *
 * \return The number of un-delivered messages in the queue,
 * 0 if \p task is NULL.
 */
uint16 messaging_task_num_pending_messages(Task task, int32 *first_due_in_ms);

/**
 * \brief How long until the next message is delivered for a task and ID.
 *
 * If \p task is set to a MessageQueue then the time of the first pending
 * message in the queue with the given ID is returned, otherwise the task is
 * interpreted as a VM task and the VM queue is searched for the appropriate
 * VM task and ID.
 *
 * \param[in] task  The VM task or MessageQueue.
 * \param[in] id  The ID of the message to find.
 * \param[out] first_due_in_ms  The time the first message with ID is due.
 *
 * \return FALSE if \p task is NULL or there is no task with the given ID in the
 * VM queue, TRUE if a message with the ID was found.
 */
bool messaging_task_first_pending_message(Task task, MessageId id,
                                          int32 *first_due_in_ms);

/**
 * \brief Count messages that match a user defined function.
 *
 * \param[in] task  The VM task or MessageQueue to run \p match_fn on.
 * \param[in] once  If this is non-zero the function will return after
 *                  the first successful match.
 * \param[in] match_fn  Function pointer to compare messages.
 *
 * \return The number of messages for task that matched \p match_fn.
 */
uint16 messaging_task_pending_match(Task task, bool once,
                                    MessageMatchFn match_fn);

/**
 * \brief Cancel the first message with a given ID for a task.
 *
 * If \p task is set to a MessageQueue then first pending message on the queue
 * with the matching ID is cancelled, otherwise the task is interpreted as a VM
 *  task and the VM queue is searched for the appropriate VM task and ID to cancel.
 *
 * \param[in] task  The VM task or MessageQueue.
 * \param[in] id  The ID of the message to cancel.
 *
 * \return FALSE if \p task is NULL or there are no matching IDs for \p task.
 */
bool messaging_cancel_first(Task task, MessageId id);

/**
 * \brief Cancel all messages with a given ID for a task.
 *
 * If \p task is set to a MessageQueue then all messages on the queue
 * with the matching ID are cancelled, otherwise the task is interpreted as a VM
 * task and the VM queue is searched for the appropriate VM task and ID to
 * cancel.
 *
 * \param[in] task  The VM task or MessageQueue.
 * \param[in] id  The ID of the messages to cancel.
 *
 * \return The number of messages cancelled. Always 0 if \p task is NULL.
 */
uint16 messaging_cancel_all(Task task, MessageId id);

/**
 * \brief Cancel all queued messages for a given task.
 *
 * If \p task is set to a MessageQueue then all messages on the queue with the
 * matching ID are cancelled, otherwise the task is interpreted as a VM task and
 * the VM queue is searched for the appropriate VM task and ID to cancel.
 *
 * \param[in] task  The VM task or MessageQueue.
 *
 * \return The number of messages cancelled. Always 0 if \p task is NULL.
 */
uint16 messaging_task_flush(Task task);

/**
 * \brief Wait on the VM message queue for a message or events to be delivered.
 *
 * Also executes the handler for any delivered messages.
 *
 * \param[in] vm_queue  A pointer to the VM owned message queue.
 * \return A bitset of events that were raised, this may be NULL if the queue was
 * delivered a message but no events were set.
 */
message_queue_events_t messaging_queue_wait_vm(message_queue_t *vm_queue);

#endif /* !MESSAGING_H */
