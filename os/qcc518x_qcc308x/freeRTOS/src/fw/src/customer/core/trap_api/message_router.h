/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * Routes a MessageQueue handle to a specific message_queue_t.
 */

#ifndef MESSAGE_ROUTER_H_
#define MESSAGE_ROUTER_H_

#include "hydra/hydra_types.h"
#include "message_queue.h"

/**
 * \brief Register a message queue with the router to get a MessageQueue handle.
 *
 * The number of MessageQueue handles available to an application is a compile
 * time constant controlled by the define MESSAGE_ROUTER_MAX_DESTINATIONS.
 *
 * \param [in] destination  A pointer to a message queue.
 * \return A MessageQueue handle suitable for passing to the application.
 * 0 if there are no handles available.
 */
MessageQueue message_router_register_destination(message_queue_t *destination);

/**
 * \brief Register the destination for unrecognised MessageQueue handles.
 *
 * Unrecognised handles are assumed to be VM Tasks and should therefore be
 * delivered to the VM task's queue.
 *
 * \param [in] destination  The default destination message queue.
 */
void message_router_register_default_destination(message_queue_t *destination);

/**
 * \brief Deregister a message queue.
 *
 * When a queue is deleted this should be called to free up handles.
 *
 * \param [in] destination  The destination to deregister. Must be a valid and
 * currently registered queue.
 */
void message_router_deregister_destination(message_queue_t *destination);

/**
 * \brief Look up the message queue for a VM Task or MessageQueue handle.
 *
 * \param [in] destination The MessageQueue or VM Task.
 * \return A pointer to the message queue associated with the handle.
 */
message_queue_t *message_router_route(MessageQueue destination);

/**
 * \brief Look up the message queue for a MessageQueue handle only.
 *
 * This variant of message_router_route() panics if the provided queue is not an
 * existing MessageQueue. i.e. panics if \p queue_handle is 0, a VM Task, or an
 * MessageQueue handle that isn't allocated.
 *
 * \param [in] queue_handle  The MessageQueue handle.
 * \return A pointer to the message queue associated with the handle.
 */
message_queue_t *message_router_route_queue(MessageQueue queue_handle);

/**
 * \brief Test whether the destination is the default message queue.
 *
 * If PANIC_ON_VM_MESSAGE_NULL_TASK_LIST is not defined then a \p destination
 * of 0 will route to the default queuue.
 *
 * \param [in] destination  The destination handle.
 * \return TRUE if \p destination routes to the default (VM) message queue,
 * FALSE otherwise.
 */
bool message_router_handle_routes_to_default(MessageQueue destination);

#endif /* !MESSAGE_ROUTER_H_ */
