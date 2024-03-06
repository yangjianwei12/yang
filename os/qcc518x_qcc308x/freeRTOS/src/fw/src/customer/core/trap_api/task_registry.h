/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * For registering "listener" tasks for the different IPC message types.
 */

#ifndef TASK_REGISTRY_H_
#define TASK_REGISTRY_H_

#include "hydra/hydra_types.h"
#include "trap_api/trap_api_private.h"
#include <message.h>

/**
 * \brief Register a task for handling a certain message type.
 *
 * \param [in] id    The type of message the task is to be passed.
 * \param [in] task  The task to receive the messages.
 *
 * \return The previous task registered for this message type.
 */
Task task_registry_register(IPC_MSG_TYPE id, Task task);

/**
 * \brief Register a task for handling IPC_MSG_TYPE_PIO.
 *
 * \param [in] task  The task to receive the messages.
 * \param [in] group  PIO group number.
 *
 * \return The previous task registered for this message type.
 */
Task pio_task_registry_register(Task task, uint16 group);

/**
 * \brief Retreive the currently registered task for a message ID.
 *
 * \param [in] _id  The message ID to retreive the task for.
 *
 * \return The task currently registered for \p id.
 */
#define task_registry_lookup(_id) registered_hdlrs[_id]

/**
 * \brief Retreive the currently registered task for a pio group.
 *
 * \param [in] _group  PIO group.
 *
 * \return The task currently registered for \p group.
 */
#define pio_task_registry_lookup(_group) registered_pio_hdlrs[_group]

/**
 * \brief Remove a task from every message ID it is registered against.
 *
 * \param [in] task  The task to remove.
 */
void task_registry_remove(Task task);

#endif /*!TASK_REGISTRY_H_ */
