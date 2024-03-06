/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * For registering "listener" tasks for stream messages.
 */

#ifndef STREAM_TASK_REGISTRY_H_
#define STREAM_TASK_REGISTRY_H_

#include "hydra/hydra_types.h"
#include <message.h>

/**
 * \brief Register a task for handling a message to the stream.
 *
 * Either source or sink, or both source and sink may be provided.
 *
 * \param[in] source The source for the stream to register the task against.
 * \param[in] sink  The sink for the stream to register the task against.
 * \param[in] task  The task to register as a handler for this stream.
 *
 * \return The previous task registered for this stream.
 */
Task stream_task_registry_register(Source source, Sink sink, Task task);

/**
 * \brief Retreive the currently registered task for this source or sink.
 *
 * Either source or sink, or both source and sink may be provided.
 *
 * \param[in] source The source for the stream to lookup the task for.
 * \param[in] sink   The sink for the stream to lookup the task for.
 *
 * \return The task currently registered for source / sink.
 */
Task stream_task_registry_lookup(Source source, Sink sink);

/**
 * \brief Remove this task from every stream that has this task as its handler.
 *
 * \param[in] task  The task to remove.
 */
void stream_task_registry_remove(Task task);

#endif /* !STREAM_TASK_REGISTRY_H_ */
