/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Implementation of the internal only messaging interface.
 *
 * This includes functions defined in messaging.c and messaging related
 * functions declarations in trap_api.h.
 */

#include "assert.h"
#include "hydra/hydra_macros.h"
#include "messaging.h"
#include "message_router.h"
#include "stream_task_registry.h"
#include "task_registry.h"
#include "trap_api/trap_api_private.h"
#include "message_queue_multitask_message.h"

/**
 * \brief Global list of unfreed messages delivered to the application.
 *
 * To support reference counting messages for multicast and using the
 * MessageFree(id, msg) function there needs to be some lookup from a pmalloc'd
 * message to the reference counted wrapper. This global list serves as that.
 *
 * Task messages placed on this list have their task value set to NULL as it is
 * not possible to determine which task a message is being freed from, this is
 * to prevent us reusing the task value when freeing the message, as the task
 * may not match.
 */
static message_queue_task_message_t *messages_delivered_but_not_freed = NULL;

/**
 * \brief Validate the message send parameters.
 *
 * The logic for panicking on invalid message_send inputs is complex enough
 * to warrant its own function.
 *
 * \param [in] tasks  messaging_send()'s tasks parameter.
 * \param [in] num_tasks  messaging_send()'s num_tasks parameter.
 * \param [in] condition_width  messaging_send()'s condition_width parameter.
 */
static void messaging_send_input_validation(const Task *tasks, uint8 num_tasks,
                                            CONDITION_WIDTH condition_width);

/**
 * \brief Send a message to a single task.
 *
 * \param [in] task  The VM task or MessageQueue to deliver the message to.
 * \param [in] id  The message ID.
 * \param [in] app_message  The message as provided by the application.
 * \param [in] allow_duplicates  Set to TRUE if duplicate messages should
 * be allowed. If set to FALSE messages where it's safe to discard duplicates
 * will be discarded.
 */
static void messaging_send_to_task_filtered(Task task, MessageId id,
                                            void *app_message,
                                            bool allow_duplicates);

/**
 * \brief Insert a message into the delivered list.
 *
 * \param [in] task_message  The message to insert. The task message next
 * pointer is reused for this list. Messages with NULL message pointers
 * shouldn't be inserted. If the message's ID / pointer pair already exists
 * in the queue it will have its reference count incremented. After returning
 * from this function the caller can release its reference count on the task
 * message structure.
 */
static void
messages_delivered_list_insert(message_queue_task_message_t *task_message);

/**
 * \brief Find an ID / message pointer pair in the delivered list.
 *
 * \warning Interrupts must be blocked whilst calling this function.
 *
 * \param [in] id  The ID to find.
 * \param [in] app_message  The message pointer to find. Must not be NULL.
 * \return The task message structure with the requested ID / message pointer
 * pair, or NULL if they weren't in the queue.
 */
static message_queue_task_message_t *messages_delivered_list_find(
    MessageId id, Message app_message);

void messaging_send(const Task *tasks, uint8 num_tasks, MessageId id,
                    Message app_message, MILLITIME delay_ms,
                    const void *condition_addr, CONDITION_WIDTH condition_width)
{
    message_queue_multitask_message_t *multitask_message;
    uint8 i;

    messaging_send_input_validation(tasks, num_tasks, condition_width);
    multitask_message = message_queue_multitask_message_create(
        tasks, num_tasks, id, app_message, delay_ms, condition_addr,
        condition_width);
    for (i = 0; i < num_tasks; ++i)
    {
        message_queue_t *dest = message_router_route((MessageQueue)tasks[i]);
        message_queue_send(dest, &multitask_message->task_message[i]);
    }
}

static void messaging_send_input_validation(const Task *tasks, uint8 num_tasks,
                                            CONDITION_WIDTH condition_width)
{
    assert(tasks);
    assert(num_tasks > 0);


#ifdef PANIC_ON_VM_MESSAGE_NULL_TASK_LIST
    if(NULL == tasks[0])
    {
        /* Panic if PANIC_ON_VM_MESSAGE_NULL is set */
        panic(PANIC_P1_VM_MESSAGE_NULL_TASK_LIST);
    }
#endif /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */

    /* Conditional messages can't be sent to MessageQueues. */
    if(CONDITION_WIDTH_UNUSED != condition_width)
    {
        uint8 i;
        for(i = 0; i < num_tasks; ++i)
        {
            Task task = tasks[i];
            if(NULL != task &&
               !message_router_handle_routes_to_default((MessageQueue)task))
            {
                panic_diatribe(PANIC_MESSAGE_QUEUE_RECEIVED_CONDITIONAL,
                               (MessageQueue)tasks[i]);
            }
        }
    }
}

void trap_api_send_message_to_task_filtered(Task task, MessageId id,
                                            void *app_message,
                                            bool allow_duplicates)
{
    message_queue_t *dest;
    message_queue_multitask_message_t *multitask_message;

    assert(task);

    dest = message_router_route((MessageQueue)task);
    multitask_message = message_queue_multitask_message_create(
        &task, 1, id, app_message, D_IMMEDIATE, /*condition_addr=*/NULL,
        CONDITION_WIDTH_UNUSED);
    message_queue_send_filtered(dest, &multitask_message->task_message[0],
                                allow_duplicates);
}

void trap_api_send_message_filtered(IPC_MSG_TYPE registered_task_id,
                                    MessageId id, void *app_message,
                                    bool allow_duplicates)
{
    Task t = task_registry_lookup(registered_task_id);
    messaging_send_to_task_filtered(t, id, app_message, allow_duplicates);
}

void trap_api_send_sink_source_message_filtered(uint16 stream_id, MessageId id,
                                                void *app_message,
                                                bool allow_duplicates)
{
    Sink sink = SINK_FROM_ID(stream_id);
    Source source = SOURCE_FROM_ID(stream_id);
    Task t = stream_task_registry_lookup(source, sink);
    messaging_send_to_task_filtered(t, id, app_message, allow_duplicates);
}

static void messaging_send_to_task_filtered(Task task, MessageId id,
                                            void *app_message,
                                            bool allow_duplicates)
{
    if(NULL != task)
    {
        trap_api_send_message_to_task_filtered(task, id, app_message,
                                               allow_duplicates);
    }
    else
    {
        message_queue_message_app_free(id, app_message);
    }
}

Task trap_api_register_message_task(Task task, IPC_MSG_TYPE msg_type_id)
{
    if (msg_type_id != IPC_MSG_TYPE_PIO)
    {
        return task_registry_register(msg_type_id, task);
    }
    else
    {
        return pio_task_registry_register(task, PIODEBOUNCE_NUMBER_OF_GROUPS);
    }
}

Task trap_api_register_message_group_task(Task task, uint16 group)
{
    assert(group <= PIODEBOUNCE_NUMBER_OF_GROUPS);

    /* remove PIO_MSG entry from registered_hdlrs */
    (void)task_registry_register(IPC_MSG_TYPE_PIO, NULL);

    return pio_task_registry_register(task, group);
}

void trap_api_send_pio_message_filtered(uint16 group,
                                    uint16 id,
                                    void *message,
                                    bool allow_duplicates)
{
    /* Is there a handler for this pio group? */
    Task task = pio_task_registry_lookup(group);

    trap_api_send_message_to_task_filtered(task, id, message, allow_duplicates);
}

message_queue_t *messaging_queue_create_vm(void)
{
    message_queue_t *vm_queue;

    vm_queue = message_queue_create();

    /* Panic on failure to create the VM queue. */
    if(NULL == vm_queue)
    {
        panic_diatribe(PANIC_HYDRA_PRIVATE_MEMORY_EXHAUSTION,
                       sizeof(message_queue_t));
    }

    message_router_register_default_destination(vm_queue);

    return vm_queue;
}

message_queue_events_t messaging_queue_wait_vm(message_queue_t *vm_queue)
{
    message_queue_events_t events;
    message_queue_task_message_t *task_message;

    task_message = message_queue_wait_for_message_or_events(vm_queue, &events);

    /* VM tasks support MessageRetain to prevent messages from being
       automatically freed on task completion. */
    if(NULL != task_message)
    {
        Message app_message = task_message->message->app_message;

        /* Null messages shouldn't be placed on the delivered list,
           they can't be retained and MessageFree ignores them. */
        if(NULL != app_message)
        {
            /* The message must be placed on the delivered list before execute
               is called as the handler may call MessageRetain which looks at
               the delivered list. */
            messages_delivered_list_insert(task_message);
        }

        message_queue_task_message_execute(task_message);

        if(NULL != app_message)
        {
            messaging_free(task_message->message->id,
                           task_message->message->app_message);
        }
        else
        {
            /* The delivered application message was NULL, so it isn't on the
               delivered messages list and therefore can't have been retained,
               so it's safe to destroy it directly. */
            message_queue_task_message_destroy(task_message);
        }
    }

    return events;
}

MessageQueue messaging_queue_create(void)
{
    MessageQueue handle = 0;
    message_queue_t *queue = message_queue_create();

    if(NULL != queue)
    {
        handle = message_router_register_destination(queue);

        if(0 == handle)
        {
            message_queue_destroy(queue);
        }
    }

    /* Return 0 to the application on memory allocation failure or if we're out
       of handles. */
    return handle;
}

void messaging_queue_destroy(MessageQueue queue)
{
    message_queue_t *destroy_queue;

    /* Silently return if queue is zero, this helps applications free
       MessageQueues without having to first check if it's zero or not. */
    if(0 == queue)
    {
        return;
    }

    destroy_queue = message_router_route_queue(queue);
    message_router_deregister_destination(destroy_queue);
    message_queue_destroy(destroy_queue);
}

MessageId messaging_queue_wait(MessageQueue queue, Message *app_message)
{
    MessageId id;
    message_queue_t *wait_queue;
    message_queue_task_message_t *task_message;

    wait_queue = message_router_route_queue(queue);
    task_message = message_queue_wait_for_message(wait_queue);

    id = task_message->message->id;
    if(NULL != app_message)
    {
        *app_message = task_message->message->app_message;
        if(NULL == *app_message)
        {
            /* No message pointer, destroy early. */
            message_queue_task_message_destroy(task_message);
        }
        else
        {
            /* Add the message to the global delivered list.
               The application frees this with MessageFree. */
            messages_delivered_list_insert(task_message);
        }
    }
    else
    {
        /* The app message isn't required so destroy it early. */
        message_queue_task_message_destroy(task_message);
    }

    return id;
}

static void messages_delivered_list_insert(
    message_queue_task_message_t *task_message)
{
    assert(task_message);

    /* NULL messages shouldn't be placed in the delivered list .*/
    assert(task_message->message->app_message);

    block_interrupts();
    {
        /* Only add an id/message pair into the list once. */
        message_queue_task_message_t *queued =
            messages_delivered_list_find(task_message->message->id,
                                         task_message->message->app_message);
        if(NULL == queued)
        {
            task_message->next = messages_delivered_but_not_freed;
            messages_delivered_but_not_freed = task_message;
        }
    }
    unblock_interrupts();
}

static message_queue_task_message_t *messages_delivered_list_find(
    MessageId id, Message app_message)
{
    message_queue_task_message_t *iter;

    /* NULL messages shouldn't be placed in the delivered list,
       searching for them is a waste of time. */
    assert(app_message);

    for(iter = messages_delivered_but_not_freed;
        NULL != iter; iter = iter->next)
    {
        if(iter->message->id == id && iter->message->app_message == app_message)
        {
            return iter;
        }
    }

    return NULL;
}

void messaging_retain(MessageId id, Message app_message)
{
    /* Check we're in the VM task, calling MessageRetain outside of a VM task
       is currently unsupported.

       If the message pointer is NULL there's no memory to retain. We could
       continue silently as MessageFree() ignores NULL pointers, or fault,
       as we aren't in an unrecoverable state. However, faults are largely
       ignored and retaining a NULL message is likely a bug in the
       application. */
    if(FALSE == sched_in_vm_task() || NULL == app_message)
    {
        panic(PANIC_P1_VM_MESSAGE_RETAIN_BAD_PARAMETERS);
    }

    block_interrupts();
    {
        message_queue_task_message_t *task_message;
        task_message = messages_delivered_list_find(id, app_message);
        if(NULL == task_message)
        {
            unblock_interrupts();

            /* Couldn't find this message in the delivered list */
            panic(PANIC_P1_VM_MESSAGE_RETAIN_BAD_PARAMETERS);
        }
        else
        {
            message_queue_message_retain(task_message->message);
        }
    }
    unblock_interrupts();
}

void messaging_free(MessageId id, Message app_message)
{
    /* Ignore NULL message pointers they shouldn't be on the delivered list. */
    if(NULL == app_message)
    {
        return;
    }

    block_interrupts();
    {
        /* Note that the task is not matched here, just the message ID and the
           message pointer. It's possible with multicast messages this will free
           a task message that was allocated for another task. For this reason
           the task pointer inside the task message cannot be relied on after
           delivery (it should always NULL within this function). */
        message_queue_task_message_t **ptask_message;
        for(ptask_message = &messages_delivered_but_not_freed;
            NULL != *ptask_message;
            ptask_message = &(*ptask_message)->next)
        {
            message_queue_task_message_t *task_message = *ptask_message;
            message_queue_message_t *message = task_message->message;

            if(id == message->id && app_message == message->app_message)
            {
                /* MessageRetain may have added multiple references.
                   If there's only one reference left unlink and destroy. */
                if(message->refcount > 1)
                {
                    message_queue_message_release(message);
                    unblock_interrupts();
                }
                else
                {
                    *ptask_message = task_message->next;

                    /* Unblock interrupts before destroying the message to
                       minimise the time interrupts are blocked for. */
                    unblock_interrupts();

                    message_queue_task_message_destroy(task_message);
                }
                return;
            }
        }
    }
    unblock_interrupts();

    /* If the message is not found free 'app_message'. Synergy relies on this
       behaviour. */
    pfree(MESSAGE_REMOVE_CONST(app_message));
}

uint16 messaging_task_num_pending_messages(Task task, int32 *first_due_in_ms)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return 0;
    }

    dest = message_router_route((MessageQueue)task);
    return message_queue_messages_pending_for_task(dest, task, first_due_in_ms);
}

bool messaging_task_first_pending_message(Task task, MessageId id,
                                          int32 *first_due_in_ms)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return FALSE;
    }

    dest = message_router_route((MessageQueue)task);
    return message_queue_first_pending_for_task(dest, task, id,
                                                first_due_in_ms);
}

uint16 messaging_task_pending_match(Task task, bool once,
                                    MessageMatchFn match_fn)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return 0;
    }

    dest = message_router_route((MessageQueue)task);
    return message_queue_pending_match(dest, task, once, match_fn);
}

bool messaging_cancel_first(Task task, MessageId id)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return FALSE;
    }

    dest = message_router_route((MessageQueue)task);
    if(message_queue_cancel_messages(dest, task, &id, /*max_to_cancel=*/1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

uint16 messaging_cancel_all(Task task, MessageId id)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return 0;
    }

    dest = message_router_route((MessageQueue)task);
    return message_queue_cancel_messages(dest, task, &id,
                                         /*max_to_cancel=*/UINT16_MAX);
}

uint16 messaging_task_flush(Task task)
{
    message_queue_t *dest;

    if(NULL == task)
    {
        return 0;
    }

    task_registry_remove(task);
    stream_task_registry_remove(task);

    dest = message_router_route((MessageQueue)task);
    return message_queue_cancel_messages(dest, task, /*id=*/NULL,
                                         /*max_to_cancel=*/UINT16_MAX);
}
