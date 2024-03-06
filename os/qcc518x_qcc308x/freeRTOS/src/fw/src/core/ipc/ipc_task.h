/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 *
 * IPC task encapsulates IPC data that's specific to a single task.
 *
 * In IPC v2, used by FreeRTOS, IPC messages contain two new pieces of
 * data. An ID, to uniquely identify the task (the tag), and the priority of the
 * message.
 *
 * Tags are assigned the first time a task uses the IPC. A structure is also
 * allocated to hold the FreeRTOS task handle, expected response ID,
 * semaphore, response memory, etc.
 *
 * This module considers the main function and the interrupt handler as separate
 * tasks, as it is possible to send IPC messages from them, and receive in the
 * case of main.
 */

#ifndef IPC_TASK_H_
#define IPC_TASK_H_

#include "hydra/hydra_types.h"
#include "ipc/ipc_prim.h"

/**
 * The IPC task tag is a unique value assigned to each client that uses the IPC.
 * This includes the main function, interrupt handler and any tasks.
 */
typedef uint8 ipc_task_tag_t;

/**
 * Forward declaration of the IPC Task structure.
 * See the definition in ipc_task.c for documentation.
 */
struct ipc_task_t;
typedef struct ipc_task_ ipc_task_t;

/**
 * @brief Retreive the IPC Task structure for the running task.
 *
 * If the structure for this task doesn't already exist it is created
 * otherwise it is found and returned.
 *
 * @return A pointer to the IPC Task structure for the current task.
 *         Will not return NULL.
 */
ipc_task_t *ipc_task_get_or_create(void);

/**
 * @brief Retreive the IPC Task structure for a given tag.
 *
 * @param [in] tag  The tag, likely received in an IPC message, to get the IPC
 * task for.
 * @return The IPC Task structure for @p tag or NULL if the tag does not exist
 * in the ipc_task list.
 */
ipc_task_t *ipc_task_get_from_tag(ipc_task_tag_t tag);

/**
 * @brief Delete an IPC task structure.
 *
 * Unrecognised handles are silently ignored so that task destructors don't need
 * to know if an IPC task exists for the task that's being destroyed.
 *
 * @param [in] handle  The OS handle to the task to delete.
 */
void ipc_task_delete_from_handle(void *handle);

/**
 * @brief Retreive the tag for an IPC Task.
 * @param [in] ipc_task  The IPC task. Panics if NULL.
 * @return The tag for the IPC task.
 */
ipc_task_tag_t ipc_task_tag(const ipc_task_t *ipc_task);

/**
 * @brief Retreive the priority for an IPC Task.
 * @param [in] ipc_task  The IPC task. Panics if NULL.
 * @return The priority for the IPC task.
 */
uint8 ipc_task_priority(const ipc_task_t *ipc_task);

/**
 * @brief Hand over a response from IPC to a task.
 *
 * Called when IPC has received a blocking response for a task.
 *
 * @param [in] ipc_task  A pointer to an IPC task. Panics if NULL.
 * @param [in] id  The signal ID for the response.
 * @param [in] response  A pointer to the response. Panics if NULL.
 * @param [in] length  The number of bytes @p response points to.
 *
 * @retval TRUE If the response was successfully given to the task.
 * @retval FALSE If the response ID was not the expected one.
 */
bool ipc_task_response_give(ipc_task_t *ipc_task, IPC_SIGNAL_ID id,
                            const void *response, uint16 length);

/**
 * @brief Set the expected response signal and message memory.
 *
 * This must be called before the IPC message expecting this response is sent
 * so it's not possible for the IPC response to appear before the
 * response has been set.
 *
 * @param [in] ipc_task  A pointer to an IPC task. Panics if NULL.
 * @param [in] id  The expected signal response ID.
 * @param [in] response  Where to copy the message response. This must be large
 * enough to hold any response message for @p id.
 */
void ipc_task_response_set(ipc_task_t *ipc_task, IPC_SIGNAL_ID id,
                           void *response);

/**
 * @brief Block the task until a response is received over the IPC.
 *
 * @param [in] ipc_task  A pointer to an IPC task. This must be the IPC task for
 * task that is currently running.
 */
void ipc_task_response_wait(ipc_task_t *ipc_task);

#endif
