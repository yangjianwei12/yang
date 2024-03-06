/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Interface for manipulating messages for a message_queue_t.
 */

#ifndef MESSAGE_QUEUE_MESSAGE_H_
#define MESSAGE_QUEUE_MESSAGE_H_

#include "hydra/hydra_types.h"
#include <message.h>
#include "longtimer/longtimer.h"
#include "trap_api/trap_api_private.h"

/**
 * A message for delivering via a message queue.
 *
 * These messages are reference counted so they can be placed on multiple queues
 * for delivering to many tasks to support the multicast feature.
 *
 * Note that structure members are ordered to reduce memory usage rather than
 * to logically group similar members.
 */
typedef struct message_queue_message_
{
    /**
     * The absolute time that the message it due to be delivered.
     */
    MILLITIME due_ms;

    /**
     * A pointer to the pmalloced message provided by the application.
     */
    Message app_message;

    /**
     * The address of the condition variable.
     * Must be NULL if condition_width is set to CONDITION_WIDTH_UNUSED.
     * Must be non NULL if condition_width is set CONDITION_WIDTH_16 or 32BIT.
     */
    const void *condition_addr;

    /**
     * The Message ID as provided by the application.
     */
    MessageId id;

    /**
     * The width of the condition variable.
     */
    CONDITION_WIDTH condition_width;

    /**
     * A reference count.
     *
     * Set to 1 when the message is constructed.
     * Incremented by message_queue_message_retain().
     * Decremented by message_queue_message_release().
     * The message is destroyed when this value reaches 0.
     */
    int8 refcount;
} message_queue_message_t;

/**
 * \brief Initialise a message.
 *
 * \param[in,out] message  A pointer to the message to initialise.
 * \param[in] id  The ID of the message.
 * \param[in] app_message  A pmalloc pointer to the message contents.
 * \param[in] delay_ms  The number of milliseconds to wait before delivering the
 * message.
 * \param[in] condition_addr  Optional pointer to a condition which must be zero
 * for the message to be delivered.
 * \param[in] condition_width  Whether to test the condition as a 16 or 32-bit
 * variable, or \c CONDITION_WIDTH_UNUSED if \p condition_addr is NULL.
 */
void message_queue_message_init(message_queue_message_t *message, MessageId id,
                                Message app_message, MILLITIME delay_ms,
                                const void *condition_addr,
                                CONDITION_WIDTH condition_width);

/**
 * \brief Increment a message's reference count.
 *
 * This function is thread safe.
 *
 * \param[in] message  A pointer to the message.
 */
void message_queue_message_retain(message_queue_message_t *message);

/**
 * \brief Decrement a message's reference count.
 *
 * This function is thread safe.
 *
 * Deletes the message if the reference count is decremented to 0.
 *
 * \param[in] message  A pointer to the message.
 */
void message_queue_message_release(message_queue_message_t *message);

/**
 * \brief Free the application's message pointer.
 *
 * This function is thread safe.
 *
 * \param[in] id  The message ID, some message types are free'd by P0.
 * \param[in] app_message  A pointer to the application message to free.
 */
void message_queue_message_app_free(MessageId id, Message app_message);

/**
 * \brief Return how long until the message is due, in milliseconds.
 *
 * This function is thread safe.
 *
 * \param[in] message  A pointer to the message.
 * \param[in] now_ms  The current time in milliseconds.
 *
 * \return How long until the message is due in milliseconds, possibly negative.
 */
INTERVAL message_queue_message_due_in_ms(const message_queue_message_t *message,
                                         MILLITIME now_ms);

/**
 * \brief Compares two messages and decides whether they are similar enough that
 * one could be dropped.
 *
 * \param[in] queued_message  The message already placed on the queue.
 * \param[in] new_message     The new message to compare against.
 *
 * \return TRUE if the two messages are equivalent.
 */
bool message_queue_message_similar(
    const message_queue_message_t *queued_message,
    const message_queue_message_t *new_message);

/**
 * \brief Replace one message with another.
 *
 * Used when a later message contains more up to date information than an
 * earlier one.
 *
 * \param[in,out] queued_message  The message on the queue that may be modified.
 * \param[in,out] new_message     The new message to compare against.
 *
 * \return TRUE if \p queued_message was replaced by \p new_message.
 */
bool message_queue_message_replace(
    message_queue_message_t *queued_message,
    const message_queue_message_t *new_message);

/**
 * \brief Determine whether the message's condition is satisfied.
 *
 * \return TRUE if the message's condition variable is set to 0 or if it doesn't
 *  have a condition variable.
 */
bool message_queue_message_condition_satisfied(
    const message_queue_message_t *message);

#endif /* !MESSAGE_QUEUE_MESSAGE_H_ */
