/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * The interface for a message queue.
 *
 * Message queues are multiple producer, single consumer, unbounded queues.
 * They are single consumer in that only one FreeRTOS task will consume the
 * messages. Each message in the queue is delivered to a single 'task'. In the
 * case of a multicast message being sent to many VM tasks the message is
 * repeated in the queue for each destination.
 *
 * Once a message's condition is satisfied and its delivery time has expired
 * it is removed from this queue, delivered to the task, and placed on a global
 * delivered list.
 *
 * The message is freed when the application calls MessageFree, where it is
 * removed the global delivered list and destroyed. Or if the Message was
 * delivered to a VM task it is automatically freed once the handler has
 * completed.
 *
 * Message queues also have an 'events' bitset, this allows the VM task to wait
 * on a queue either for an event (used to emulate background interrupts) or
 * a message. The VM task then only needs to wait on a single semaphore for both
 * delivery mechanisms.
 */

#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include "message_queue_task_message.h"

#include "FreeRTOS.h"
#include "semphr.h"

/**
 * A type for a message queue event bit. i.e. the index of bit in a bitset.
 */
typedef uint8 message_queue_event_bit_t;

/**
 * A type for a set of message queue events.
 */
typedef uint32 message_queue_events_t;

/**
 * The maximum number of events a message queue can support.
 */
#define MESSAGE_QUEUE_EVENTS_MAX (CHAR_BIT * sizeof(message_queue_events_t))

/**
 * The message queue structure.
 */
typedef struct message_queue_
{
    /**
     * Singly linked list of messages to be delivered along with the task to
     * deliver each message to. The list is sorted by delivery time, earliest
     * deadline first.
     */
    message_queue_task_message_t *queued;

    /**
     * A bit set of events, used for implementing background interrupts in the
     * VM task.
     */
    message_queue_events_t events;

    /**
     * Structure for a FreeRTOS semaphore.
     */
    StaticSemaphore_t sem_data;

    /**
     * The FreeRTOS semaphore handle for signaling/waiting on new messages or
     * events.
     */
    SemaphoreHandle_t sem;
} message_queue_t;

/**
 * \brief Create a message queue.
 *
 * This function must not be called from an interrupt handler.
 * This function may be called from multiple threads.
 *
 * \return A new message queue, returns NULL on memory allocation failure.
 */
message_queue_t *message_queue_create(void);

/**
 * \brief Destroys a message queue.
 *
 * This function must not be called from an interrupt handler.
 *
 * \param [in] queue  The message queue to destroy. May be NULL, in which case
 * the function returns immediately.
 */
void message_queue_destroy(message_queue_t *queue);

/**
 * \brief Send a message on this queue to a particular task.
 *
 * May be called from multiple tasks simultaneously for the same queue.
 *
 * Ownership of \p task_message is handed over to the message queue.
 *
 * \param [in] queue  A pointer to the queue.
 * \param [in] task_message  A pointer to a task message pair.
 */
void message_queue_send(message_queue_t *queue,
                        message_queue_task_message_t *task_message);

/**
 * \brief Send a message on this queue to a particular task whilst filtering out
 * duplicate and similar messages.
 *
 * May be called from multiple tasks simultaneously for the same queue.
 *
 * Ownership of \p task_message is handed over to the message queue.
 *
 * \param [in] queue  A pointer to the queue.
 * \param [in] task_message  A pointer to a task message pair.
 * \param [in] allow_duplicates  Set to TRUE if duplicates and similar messages
 * should be allowed in the queue, FALSE otherwise.
 */
void message_queue_send_filtered(message_queue_t *queue,
                                 message_queue_task_message_t *task_message,
                                 bool allow_duplicates);

/**
 * \brief Raise an event on the given queue.
 *
 * May be called from an interrupt or a task.
 *
 * Events are binary, so if the event is already raised it stays raised, there's
 * no count associated with it.
 *
 * \param [in] queue  A pointer to the queue to raise the event on.
 * \param [in] event_bit  The event to raise.
 */
void message_queue_event_raise(message_queue_t *queue,
                               message_queue_event_bit_t event_bit);

/**
 * \brief Wait for a message on the queue to be delivered.
 *
 * Must not be called from an interrupt.
 * Must only be called by one task for a queue.
 *
 * \warning This function will wait indefinitely so before calling it ensure
 * that a message is expected to be delivered before the task needs to do
 * something else.
 *
 * \param [in] queue  A pointer to a queue.
 * \return A pointer to the delivered message.
 */
message_queue_task_message_t *message_queue_wait_for_message(
    message_queue_t *queue);

/**
 * \brief Wait for either a message to be delivered or an event to be raised.
 *
 * \param [in] queue  A pointer to a queue.
 * \param [out] events  A pointer to memory to return the raised events in.
 * return A pointer to the delivered message, may be NULL if no message was
 * delivered.
 */
message_queue_task_message_t *message_queue_wait_for_message_or_events(
    message_queue_t *queue, message_queue_events_t *events);

/**
 * \brief Count the number of undelivered messages in the queue for a task.
 *
 * \param [in] queue  A pointer to a queue.
 * \param [in] task  The task to count undelivered messages for.
 * \param [out] first_due_in_ms  A pointer to some memory to return the time in
 * milliseconds until the first message for \p task is due.
 * \return The number of messages \p queue pending for \p task.
 */
uint16 message_queue_messages_pending_for_task(const message_queue_t *queue,
                                               Task task,
                                               int32 *first_due_in_ms);

/**
 * \brief Get the time of the first message due for a task and ID.
 *
 * \param [in] queue  A pointer to a queue.
 * \param [in] task  The task to find the first message for.
 * \param [in] id  The message ID to match.
 * \param [out] first_due_in_ms  A pointer to some memory to return the time in
 * milliseconds until the first message for \p task with \p id is due.
 * \return TRUE if a message with the given task and id was found, otherwise
 * FALSE.
 */
bool message_queue_first_pending_for_task(const message_queue_t *queue,
                                          Task task, MessageId id,
                                          int32 *first_due_in_ms);

/**
 * \brief Count messages that match a user defined function.
 *
 * \param [in] queue  A pointer to a queue.
 * \param [in] task  The task to match against.
 * \param [in] once  If this is non-zero the function will return after
 *                  the first successful match.
 * \param [in] match_fn  Function pointer to compare messages.
 *
 * \return The number of messages for task that matched \p match_fn.
 */
uint16 message_queue_pending_match(const message_queue_t *queue,
                                   Task task, bool once,
                                   MessageMatchFn match_fn);

/**
 * \brief Cancel messages for a task on a queue.
 *
 * \param [in] queue  A pointer to a queue.
 * \param [in] task  The task to cancel messages for.
 * \param [in] id  Optional pointer to an ID. If NULL all pending messages for
 * \p task are cancelled. If non-NULL message is only cancelled if both the task
 * and the value pointed to by \p id is matched.
 * \param [in] max_to_cancel  The maximum number of messages to cancel.
 * \return The number of messages that were cancelled.
 */
uint16 message_queue_cancel_messages(message_queue_t *queue, Task task,
                                     const MessageId *id, uint16 max_to_cancel);

#endif /* !MESSAGE_QUEUE_H_ */
