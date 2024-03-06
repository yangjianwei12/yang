/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Routes a MessageQueue handle to a specific message_queue_t.
 */

#include "assert.h"
#include "message_router.h"
#include "panic/panic.h"

/**
 * The maximum number of message queues that can be supported simultaneously.
 *
 * Guarded so that the maximum number of supported queues can be changed by the
 * build command if desired.
 */
#ifndef MESSAGE_ROUTER_MAX_DESTINATIONS
#define MESSAGE_ROUTER_MAX_DESTINATIONS (16)
#endif /* MESSAGE_ROUTER_MAX_DESTINATIONS */

/**
 * Structure for holding message queue routing information.
 */
typedef struct message_router_
{
    /**
     * The default destination queue to route VM Tasks to.
     */
    message_queue_t *default_destination;

    /**
     * Array of destinations for each MessageQueue handle.
     * MessageQueue handles start from 1 so the index into this array is
     * MessageQueue - 1. The maximum handle value is therefore
     * MESSAGE_ROUTER_MAX_DESTINATIONS.
     */
    message_queue_t *destination_table[MESSAGE_ROUTER_MAX_DESTINATIONS];
} message_router_t;

/**
 * Global state for the message router.
 */
static message_router_t message_router;

/**
 * \brief Maps a destination table index to a MessageQueue handle.
 *
 * \param [in] index  The index.
 * \return The MessageQueue for \p index.
 */
static MessageQueue destination_index_to_handle(uint32 index);

/**
 * \brief Maps a MessageQueue handle to a destination table index.
 *
 * \param [in] handle  The handle.
 * \return The destination table index for \p handle.
 */
static uint32 destination_handle_to_index(MessageQueue handle);

void message_router_register_default_destination(message_queue_t *destination)
{
    assert(destination);

    message_router.default_destination = destination;
}

MessageQueue message_router_register_destination(message_queue_t *destination)
{
    uint32 i;

    /* Passing a NULL destination is an internal programming error. */
    assert(destination);

    for(i = 0; i < MESSAGE_ROUTER_MAX_DESTINATIONS; ++i)
    {
        if(NULL == message_router.destination_table[i])
        {
            message_router.destination_table[i] = destination;
            return destination_index_to_handle(i);
        }
    }

    /* Return 0 if there are no more slots for message queue handles. */
    return 0;
}

void message_router_deregister_destination(message_queue_t *destination)
{
    uint32 i;

    assert(destination);

    for(i = 0; i < MESSAGE_ROUTER_MAX_DESTINATIONS; ++i)
    {
        if(destination == message_router.destination_table[i])
        {
            message_router.destination_table[i] = NULL;
            return;
        }
    }

    /* It is a programming error to pass this function a queue that isn't
       registered. */
    assert(0);
}

message_queue_t *message_router_route(MessageQueue destination)
{
    if(message_router_handle_routes_to_default(destination))
    {
        return message_router.default_destination;
    }
    else
    {
        uint32 index = destination_handle_to_index(destination);
        return message_router.destination_table[index];
    }
}

message_queue_t *message_router_route_queue(MessageQueue queue_handle)
{
    uint32 index;
    message_queue_t *queue;

    /* This function is used to validate and route message queue handles from
       the application, so should use panic codes rather than assertions. */
    if(queue_handle < 1 || queue_handle > MESSAGE_ROUTER_MAX_DESTINATIONS)
    {
        panic_diatribe(PANIC_MESSAGE_QUEUE_INVALID, queue_handle);
    }

    index = destination_handle_to_index(queue_handle);
    queue = message_router.destination_table[index];

    if(NULL == queue)
    {
        panic_diatribe(PANIC_MESSAGE_QUEUE_INVALID, queue_handle);
    }

    return queue;
}

bool message_router_handle_routes_to_default(MessageQueue destination)
{

#ifdef PANIC_ON_VM_MESSAGE_NULL_TASK_LIST
    assert(destination);
#else
    /* NULL destinations should be sent to the default queue when panic on NULL
       is disabled. */
    if(0 == destination)
    {
        return TRUE;
    }
#endif /* PANIC_ON_VM_MESSAGE_NULL_TASK_LIST */

    return destination > MESSAGE_ROUTER_MAX_DESTINATIONS;
}

static MessageQueue destination_index_to_handle(uint32 index)
{
    assert(index < MESSAGE_ROUTER_MAX_DESTINATIONS);

    return index + 1;
}

static uint32 destination_handle_to_index(MessageQueue handle)
{
    uint32 index = handle - 1;

    assert(index < MESSAGE_ROUTER_MAX_DESTINATIONS);

    return index;
}
