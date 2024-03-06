/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Implementation of a message queue.
 */

#include "assert.h"
#include "message_queue.h"
#include "pmalloc/pmalloc.h"
#include "pl_timers/pl_timers.h"

/**
 * \brief Test if a message is already in the queue.
 *
 * This function may modify the queue. If the new message is not similar to
 * a message already in the queue, but can replace it then it will be replaced.
 *
 * \param [in] queue  A pointer to a message queue, must not be NULL.
 * \param [in] task_message  A pointer to a task and message pair, must not be
 * NULL.
 *
 * \return TRUE if there is a similar message already in the queue, or a message
 *  in the queue was replaced.
 */
static bool
message_queue_already(message_queue_t *queue,
                      const message_queue_task_message_t *task_message);

/**
 * \brief Atomically set an event bit.
 *
 * \param [in,out] events  A pointer to the message queue events bitset.
 * \param [in] event_bit  The ID of the bit to set.
 */
static void message_queue_atomic_set_bit(message_queue_events_t *events,
                                         message_queue_event_bit_t event_bit);

/**
 * \brief Send a message once interrupts are blocked.
 *
 * \warning This function must be called with interrupts blocked.
 *
 * \param [in] queue  The queue to send the message on.
 * \param [in] task_message  The destination task message pair.
 *
 * \return TRUE if the message was sent, FALSE if the message wasn't sent
 * due to a similar message already being in the queue.
 */
static bool
message_queue_send_locked(message_queue_t *queue,
                          message_queue_task_message_t *task_message);

/**
 * \brief Atomically read and clear the events in a queue.
 *
 * \param [in] queue  The queue to take the events from.
 * \return The events that were set in the queue.
 */
static message_queue_events_t message_queue_take_events(message_queue_t *queue);

/**
 * \brief Atomically remove the next deliverable message from a queue.
 *
 * \param [in] queue  A queue.
 *
 * \return The removed deliverable message or NULL if no messages are
 * currently deliverable.
 */
static message_queue_task_message_t *message_queue_take_task_message(
    message_queue_t *queue);

/**
 * \brief Timed event handler called when a message deadline has elapsed.
 *
 * \param [in] data  Set to pointer to a message queue.
 */
static void message_queue_timed_event_callback_isr(void *data);

/**
 * \brief Kick the message queue's semaphore.
 *
 * Must be called from a task (not interrupt) context.
 *
 * \param [in] queue  The queue to kick.
 */
static void message_queue_kick_from_task(message_queue_t *queue);

/**
 * \brief Kick the message queue's semaphore.
 *
 * Must be called from an interrupt (not task) context.
 *
 * \param [in] queue  The queue to kick.
 */
static void message_queue_kick_from_isr(message_queue_t *queue);

/**
 * \brief Kick the message queue's semaphore at some point in the future.
 *
 * \param [in] queue  The queue to kick.
 * \param [in] delay_ms  The number of milleseconds to delay before kicking.
 */
static void message_queue_kick_in_ms(message_queue_t *queue, INTERVAL delay_ms);

/**
 * \brief Kick the message queue's semaphore.
 *
 * May be called from an interrupt or task context.
 *
 * \param [in] queue  The queue to kick.
 */
static void message_queue_kick(message_queue_t *queue);

/**
 * \brief Waits for the message queue's semaphore to be kicked.
 *
 * \warning Waits indefinetely so only call this if you're sure the semaphore
 * will be kicked before the current task needs to do anything else.
 *
 * Must not be called from an interrupt context.
 *
 * \param [in] queue  The queue to wait on.
 */
static void message_queue_wait(const message_queue_t *queue);

message_queue_t *message_queue_create(void)
{
    message_queue_t *queue;

    assert(!sched_in_interrupt());

    /* Using a non-panicking allocation allows applications to decide what to do
       in the case of a memory allocation failure. */
    queue = xpnew(message_queue_t);
    if(NULL == queue)
    {
        return NULL;
    }

    queue->queued = NULL;
    queue->events = 0;

    /* Binary semaphores created with xSemaphoreCreateBinaryStatic are intially
       in the empty state, so a "Give" must be called before a "Take" will be
       successful. */
    queue->sem = xSemaphoreCreateBinaryStatic(&queue->sem_data);

    /* Static semaphore creation should never fail. */
    assert(queue->sem);

    return queue;
}

void message_queue_destroy(message_queue_t *queue)
{
    assert(!sched_in_interrupt());

    if(NULL != queue)
    {
        vSemaphoreDelete(queue->sem);
        pfree(queue);
    }
}

void message_queue_send_filtered(message_queue_t *queue,
                                 message_queue_task_message_t *task_message,
                                 bool allow_duplicates)
{
    bool sent = FALSE;

    assert(queue);
    assert(task_message);

    message_queue_task_message_log(TRAP_API_LOG_SEND, task_message);

    block_interrupts();
    {
        if (allow_duplicates || !message_queue_already(queue, task_message))
        {
            sent = message_queue_send_locked(queue, task_message);
        }
    }
    unblock_interrupts();

    if(!sent)
    {
        message_queue_task_message_destroy(task_message);
    }
}

static bool
message_queue_already(message_queue_t *queue,
                      const message_queue_task_message_t *task_message)
{
    message_queue_task_message_t *queued;

    assert(queue);
    assert(task_message);

    for (queued = queue->queued; NULL != queued; queued = queued->next)
    {
        if (message_queue_task_message_similar(queued, task_message) ||
            message_queue_task_message_replace(queued, task_message))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void message_queue_event_raise(message_queue_t *queue,
                               message_queue_event_bit_t event_bit)
{
    assert(queue);
    assert(event_bit < 32);

    message_queue_atomic_set_bit(&queue->events, event_bit);
    message_queue_kick(queue);
}

static void message_queue_atomic_set_bit(message_queue_events_t *events,
                                         message_queue_event_bit_t event_bit)
{
    assert(events);
    assert(event_bit < 32);

    block_interrupts();
    {
        *events |= 1 << event_bit;
    }
    unblock_interrupts();
}

void message_queue_send(message_queue_t *queue,
                        message_queue_task_message_t *task_message)
{
    bool sent;

    message_queue_task_message_log(TRAP_API_LOG_SEND, task_message);

    block_interrupts();
    {
        sent = message_queue_send_locked(queue, task_message);
    }
    unblock_interrupts();

    if(!sent)
    {
        message_queue_task_message_destroy(task_message);
    }
}

static bool
message_queue_send_locked(message_queue_t *queue,
                          message_queue_task_message_t *task_message)
{
    bool sent = FALSE;
    message_queue_task_message_t **p;
    message_queue_task_message_t *prev = NULL;

#ifdef PANIC_ON_VM_MESSAGE_NULL_TASK_LIST
    assert(task);
#endif /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */
    assert(queue);
    assert(task_message);

    p = &queue->queued;
    while (*p && time_ge(task_message->message->due_ms, (*p)->message->due_ms))
    {
        prev = *p;
        p = &prev->next;
    }

    if (NULL == prev ||
        !message_queue_message_similar(prev->message, task_message->message))
    {
        task_message->next = *p;
        *p = task_message;

        message_queue_kick(queue);
        sent = TRUE;
    }

    return sent;
}

message_queue_task_message_t *message_queue_wait_for_message(
    message_queue_t *queue)
{
    assert(queue);

    /*lint -e(716) Loop will terminate once a message is received. */
    while(1)
    {
        message_queue_task_message_t *task_message =
            message_queue_take_task_message(queue);
        if(NULL != task_message)
        {
            return task_message;
        }

        message_queue_wait(queue);
    }
}

message_queue_task_message_t *message_queue_wait_for_message_or_events(
    message_queue_t *queue, message_queue_events_t *events)
{
    assert(queue);
    assert(events);

    /*lint -e(716) Loop will terminate once an event is raised or message received. */
    while(1)
    {
        message_queue_task_message_t *task_message =
            message_queue_take_task_message(queue);
        *events = message_queue_take_events(queue);
        if(NULL != task_message || 0 != *events)
        {
            return task_message;
        }

        message_queue_wait(queue);
    }
}

static message_queue_events_t message_queue_take_events(message_queue_t *queue)
{
    message_queue_events_t events;

    assert(queue);

    block_interrupts();
    {
        events = queue->events;
        queue->events = 0;
    }
    unblock_interrupts();

    return events;
}

static message_queue_task_message_t *message_queue_take_task_message(
    message_queue_t *queue)
{
    message_queue_task_message_t **ptask_message;

    assert(queue);
    block_interrupts();
    {
        for(ptask_message = &queue->queued; NULL != *ptask_message;
            ptask_message = &(*ptask_message)->next)
        {
            message_queue_task_message_t *task_message = *ptask_message;
            message_queue_message_t *message = task_message->message;
            if(message_queue_message_condition_satisfied(message))
            {
                MILLITIME now_ms = get_milli_time();
                INTERVAL due_in_ms = message_queue_message_due_in_ms(message,
                                                                     now_ms);
                if(due_in_ms <= 0)
                {
                    *ptask_message = task_message->next;

                    /* Interrupts have been blocked a while during searching the
                       queue. Unblock interrupts before logging the message as
                       logging can take a while. */
                    unblock_interrupts();

                    message_queue_task_message_log(TRAP_API_LOG_DELIVER,
                                                   task_message);

                    /* Unlink from this queue. */
                    task_message->next = NULL;
                    return task_message;
                }

                message_queue_kick_in_ms(queue, due_in_ms);
                break;
            }
        }
    }
    unblock_interrupts();

    return NULL;
}

static void message_queue_kick_in_ms(message_queue_t *queue, INTERVAL delay_ms)
{
    assert(queue);

    timer_cancel_event_by_function(message_queue_timed_event_callback_isr,
                                   queue);

    /* Avoid setting a timer for more than 1/4 of the wrap distance. */
    delay_ms = MIN(delay_ms, 0x3fffffffl / US_PER_MS);
    (void) timer_schedule_event_in(
        /*event_time=*/delay_ms * US_PER_MS,
        /*TimerEventFunction=*/message_queue_timed_event_callback_isr,
        /*data_pointer=*/queue);
}

static void message_queue_timed_event_callback_isr(void *data)
{
    assert(data);
    message_queue_kick_from_isr(data);
}

static void message_queue_kick_from_task(message_queue_t *queue)
{
    BaseType_t result;

    assert(!sched_in_interrupt());
    assert_uses_param(queue);
    assert(queue);

    /* xSemaphoreGive can return pdPASS or errQUEUE_FULL.
       pdPASS is expected if the semaphore is currently in an empty state.
       errQUEUE_FULL is expected if a "Give" has been called without a "Take"
       having happened yet, this is expected to happen for binary semaphores. */
    result = xSemaphoreGive(queue->sem);
    UNUSED(result);
    assert((pdPASS == result) || (errQUEUE_FULL == result));
}

static void message_queue_kick_from_isr(message_queue_t *queue)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    BaseType_t result;

    assert_uses_param(queue);
    assert(queue);

    /* xSemaphoreGiveFromISR can return pdPASS or errQUEUE_FULL.
       pdPASS is expected if the semaphore is currently in an empty state.
       errQUEUE_FULL is expected if a "Give" has been called without a "Take"
       having happened yet, this is expected to happen for binary semaphores. */
    result = xSemaphoreGiveFromISR(queue->sem, &higher_priority_task_woken);
    UNUSED(result);
    assert((pdPASS == result) || (errQUEUE_FULL == result));
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void message_queue_kick(message_queue_t *queue)
{
    assert(queue);

    if(sched_in_interrupt())
    {
        message_queue_kick_from_isr(queue);
    }
    else
    {
        message_queue_kick_from_task(queue);
    }
}

static void message_queue_wait(const message_queue_t *queue)
{
    assert(!sched_in_interrupt());
    assert_uses_param(queue);
    assert(queue);

    assert_fn_ret(xSemaphoreTake(queue->sem, portMAX_DELAY), BaseType_t, pdPASS);
}

uint16 message_queue_messages_pending_for_task(const message_queue_t *queue,
                                               Task task,
                                               int32 *first_due_in_ms)
{
    uint16 count = 0;

    assert(queue);

    block_interrupts();
    {
        message_queue_task_message_t *task_message;
        for(task_message = queue->queued; NULL != task_message;
            task_message = task_message->next)
        {
            if(task_message->task == task)
            {
                message_queue_message_t *message = task_message->message;
                if(0 == count && NULL != first_due_in_ms)
                {
                    *first_due_in_ms = time_sub(message->due_ms,
                                                get_milli_time());
                }

                ++count;
            }
        }
    }
    unblock_interrupts();

    return count;
}

bool message_queue_first_pending_for_task(const message_queue_t *queue,
                                          Task task, MessageId id,
                                          int32 *first_due_in_ms)
{
    bool result = FALSE;

    assert(queue);

    block_interrupts();
    {
        message_queue_task_message_t *task_message;
        for(task_message = queue->queued; NULL != task_message;
            task_message = task_message->next)
        {
            message_queue_message_t *message = task_message->message;
            if(task_message->task == task && message->id == id)
            {
                if(NULL != first_due_in_ms)
                {
                    *first_due_in_ms = time_sub(message->due_ms,
                                                get_milli_time());
                }

                result = TRUE;
                break;
            }
        }
    }
    unblock_interrupts();

    return result;
}

uint16 message_queue_pending_match(const message_queue_t *queue,
                                   Task task, bool once,
                                   MessageMatchFn match_fn)
{
    uint16 count = 0;

    assert(queue);

    block_interrupts();
    {
        message_queue_task_message_t *task_message;
        for(task_message = queue->queued; NULL != task_message;
            task_message = task_message->next)
        {
            if(task_message->task == task)
            {
                message_queue_message_t *message = task_message->message;
                if(match_fn(task, message->id, message->app_message))
                {
                    count++;
                    if(once)
                    {
                        break;
                    }
                }
            }
        }
    }
    unblock_interrupts();

    return count;
}

uint16 message_queue_cancel_messages(message_queue_t *queue, Task task,
                                     const MessageId *id, uint16 max_to_cancel)
{
    message_queue_task_message_t *removed = NULL;
    message_queue_task_message_t **removed_end = &removed;
    uint16 count = 0;

    assert(queue);

    /* To keep the amount of time we block interrupts for low, remove the
       matching messages from the queue with interrupts blocked then do the
       expensive work of logging and destroying messages with interrupts
       unblocked. */
    block_interrupts();
    {
        /* Append to the end of the removed list to keep message destruction in
           order, although it shouldn't matter. */
        message_queue_task_message_t **ptask_message;
        for(ptask_message = &queue->queued; NULL != *ptask_message; )
        {
            message_queue_task_message_t *task_message = *ptask_message;
            message_queue_message_t *message = task_message->message;

            if(task_message->task == task && (NULL == id || message->id == *id))
            {
                /* Remove from the queue. */
                *ptask_message = task_message->next;

                /* Append to the removed list. */
                *removed_end = task_message;
                removed_end = &task_message->next;

                ++count;
                if(count >= max_to_cancel)
                {
                    break;
                }
            }
            else
            {
                ptask_message = &task_message->next;
            }
        }
    }
    unblock_interrupts();

    /* Terminate the removed list. */
    *removed_end = NULL;

    while(NULL != removed)
    {
        message_queue_task_message_t *task_message = removed;
        removed = removed->next;

        message_queue_task_message_log(TRAP_API_LOG_CANCEL, task_message);
        message_queue_task_message_destroy(task_message);
    }

    return count;
}
