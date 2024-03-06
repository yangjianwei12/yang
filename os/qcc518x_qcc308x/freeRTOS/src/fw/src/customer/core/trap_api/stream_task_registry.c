/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * For registering "listener" tasks for stream messages.
 */

#include "int/int.h"
#include "pmalloc/pmalloc.h"
#include "stream_task_registry.h"

/**
 * One entry in the stream task registry linked list.
 */
typedef struct stream_task_registry_entry_
{
    struct stream_task_registry_entry_ *next;
    Source source;
    Sink sink;
    Task task;
} stream_task_registry_entry_t;

/**
 * Linked list of tasks registered for handling streams messages.
 */
static stream_task_registry_entry_t *sink_source_registry = NULL;

/**
 * \brief Allocated and initialises a stream_task_registry_entry_t.
 *
 * \return A new entry, not linked into the list yet.
 */
static stream_task_registry_entry_t *stream_task_registry_entry_create(
    Source source, Sink sink, Task task);

/**
 * \brief Check whether an entry in the registry matches a source / sink pair.
 *
 * An entry matches if either the source or the sink is equal. If the source
 * or sink passed in are NULL then they are not matched against the entry.
 *
 * \param[in] entry   The entry in the registy to compare against.
 * \param[in] source  A source, optionally NULL.
 * \param[in] sink    A sink, optionally NULL.
 *
 * \return TRUE if \p source or \p sink are in \p entry, FALSE otherwise.
 */
static bool stream_task_registry_entry_matches(
    const stream_task_registry_entry_t *entry, Source source, Sink sink);

Task stream_task_registry_register(Source source, Sink sink, Task task)
{
    Task old_task = NULL;

    block_interrupts();
    {
        stream_task_registry_entry_t **piter;

        for(piter = &sink_source_registry; *piter != NULL; piter = &(*piter)->next)
        {
            stream_task_registry_entry_t *handler = *piter;

            if(stream_task_registry_entry_matches(handler, source, sink))
            {
                old_task = handler->task;
                if(NULL == task)
                {
                    /* Remove the entry */
                    *piter = handler->next;
                    pfree(handler);
                }
                else
                {
                    /* Replace the entry */
                    handler->source = source;
                    handler->sink = sink;
                    handler->task = task;
                }
                break;
            }
        }

        /* No existing entry, create one. */
        if(NULL == old_task && NULL != task)
        {
            *piter = stream_task_registry_entry_create(source, sink, task);
        }
    }
    unblock_interrupts();

    return old_task;
}

Task stream_task_registry_lookup(Source source, Sink sink)
{
    Task task = NULL;

    block_interrupts();
    {
        stream_task_registry_entry_t *iter;

        for(iter = sink_source_registry; NULL != iter; iter = iter->next)
        {
            if(stream_task_registry_entry_matches(iter, source, sink))
            {
                task = iter->task;
                break;
            }
        }
    }
    unblock_interrupts();

    return task;
}

static stream_task_registry_entry_t *stream_task_registry_entry_create(
    Source source, Sink sink, Task task)
{
    stream_task_registry_entry_t *entry;

    entry = pnew(stream_task_registry_entry_t);
    entry->next = NULL;
    entry->source = source;
    entry->sink = sink;
    entry->task = task;

    return entry;
}

void stream_task_registry_remove(Task task)
{
    block_interrupts();
    {
        stream_task_registry_entry_t **piter;
        for(piter = &sink_source_registry; *piter != NULL;)
        {
            stream_task_registry_entry_t *handler = *piter;
            if(task == handler->task)
            {
                *piter = handler->next;
                pfree(handler);
            }
            else
            {
                piter = &handler->next;
            }
        }
    }
    unblock_interrupts();
}

static bool stream_task_registry_entry_matches(
    const stream_task_registry_entry_t *entry, Source source, Sink sink)
{
    return ((source && entry->source == source) ||
            (sink && entry->sink == sink));
}
