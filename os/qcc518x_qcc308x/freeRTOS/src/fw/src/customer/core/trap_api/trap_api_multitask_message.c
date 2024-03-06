/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * This file contains the implementation for trap API messaging related
 * functions for FreeRTOS. It should be a thin layer on top of the API defined
 * in messaging.h. This file therefore defines the functions for applications
 * to call, and messaging.h contains functions called from within the
 * Qualcomm Hydra library.
 */

#include "hydra/hydra_types.h"
#define IO_DEFS_MODULE_K32_CORE
#include "io/io.h"

#include <message.h>
#include <stream.h>

#include "messaging.h"
#include "hydra/hydra_macros.h"
#include "task_registry.h"
#include "stream_task_registry.h"
#include "ipc/ipc_msg_types.h"
#include "panic/panic.h"
#include "hydra_log/hydra_log.h"

/**
 * \brief Count the number of tasks in a NULL terminated task list.
 *
 * \param[in] task_list NULL terminated array of tasks.
 *
 * \return The number of entries in \p task_list until the first NULL task.
 */
static uint8 multicast_count_tasks(const Task *task_list);

void MessageSendLater(Task task, MessageId id, void *message, uint32 delay)
{
    messaging_send(&task, /*num_tasks=*/1,
                   id, message, delay,
                   /*condition_addr=*/NULL, CONDITION_WIDTH_UNUSED);
}

/**
 * \brief Send a message to be be delivered when the corresponding uint16 is zero.
 *
 * \param t The task to deliver the message to.
 * \param id The message identifier.
 * \param m The message data.
 * \param c The condition that must be zero for the message to be delivered.
 *
 * \ingroup trapset_core
 */
void MessageSendConditionally(Task t, MessageId id, Message m, const uint16 *c)
{
    messaging_send(&t, /*num_tasks=*/1, id, m, D_IMMEDIATE, c,
                   CONDITION_WIDTH_16BIT);
}

/**
 * \brief Send a message to be be delivered when the corresponding Task is zero.
 *
 * \param t The task to deliver the message to.
 * \param id The message identifier.
 * \param m The message data.
 * \param c The task that must be zero for the message to be delivered.
 */
void MessageSendConditionallyOnTask(Task t, MessageId id, Message m,
                                    const Task *c)
{
    messaging_send(&t, /*num_tasks=*/1, id, m, D_IMMEDIATE, c,
                   CONDITION_WIDTH_32BIT);
}

Task MessageStreamTaskFromSink(Sink sink, Task task)
{
    Source source = (task != NULL) ? StreamSourceFromSink(sink) : NULL;
    return stream_task_registry_register(source, sink, task);
}

Task MessageStreamTaskFromSource(Source source, Task task)
{
    Sink sink = (task != NULL) ? StreamSinkFromSource(source) : NULL;
    return stream_task_registry_register(source, sink, task);
}

Task MessageStreamGetTaskFromSink(Sink sink)
{
    return stream_task_registry_lookup(NULL, sink);
}

Task MessageStreamGetTaskFromSource(Source source)
{
    return stream_task_registry_lookup(source, NULL);
}

void MessageSendMulticastLater(Task *task_list, MessageId id, void *message,
                               uint32 delay_ms)
{
    uint8 num_tasks = multicast_count_tasks(task_list);
    messaging_send(task_list, num_tasks,
                   id, message, delay_ms,
                   /*condition_addr=*/NULL, CONDITION_WIDTH_UNUSED);
}

/**
 *  \brief Send a message to be be delivered when the corresponding uint16 is zero.
 *
 *  \param tlist Pointer to the tasks to deliver the message to.
 *  \param id The message identifier.
 *  \param m The message data.
 *  \param c The condition that must be zero for the message to be delivered.
 *
 *  \ingroup trapset_core
 */
void MessageSendMulticastConditionally(Task *tlist, MessageId id, Message m,
                                       const uint16 *c)
{
    uint8 num_tasks = multicast_count_tasks(tlist);
    messaging_send(tlist, num_tasks, id, m, D_IMMEDIATE, c, CONDITION_WIDTH_16BIT);
}

/**
 * \brief Send a message to be be delivered when the corresponding Task is zero.
 *
 * \param tlist The task to deliver the message to.
 * \param id The message identifier.
 * \param m The message data.
 * \param c The task that must be zero for the message to be delivered.
 *
 * \ingroup trapset_core
 */
void MessageSendMulticastConditionallyOnTask(Task *tlist, MessageId id,
                                             Message m, const Task *c)
{
    uint8 num_tasks = multicast_count_tasks(tlist);
    messaging_send(tlist, num_tasks, id, m, D_IMMEDIATE, c,
                   CONDITION_WIDTH_32BIT);
}

static uint8 multicast_count_tasks(const Task *task_list)
{
    size_t num_tasks = 0;
    const Task *iter;

    if((NULL == task_list) || (NULL == task_list[0]))
    {
        /* We can't tolerate no list or an empty list for multicast. */
        panic(PANIC_P1_VM_MESSAGE_NULL_TASK_LIST);
    }

    for(iter = task_list; NULL != *iter; ++iter)
    {
        ++num_tasks;
    }

    if(num_tasks > MAX_MULTICAST_RECIPIENTS)
    {
        /* Match Oxygen's behaviour by panicking if there are too many multicast
           recipients. The Oxygen VM messaging code panics because a copy of
           task_list is stored on the stack during logging, so the maximum
           size of the list needs to be known at compile time. This messaging
           code doesn't use that array as the message is sent to the log one
           task at a time.
        */
        panic(PANIC_P1_VM_MESSAGE_TOO_MANY_RECIPIENTS);
    }

    return (uint8) num_tasks;
}

bool MessageCancelFirst(Task task, MessageId id)
{
    return messaging_cancel_first(task, id);
}

uint16 MessageCancelAll(Task task, MessageId id)
{
    return messaging_cancel_all(task, id);
}

uint16 MessageFlushTask(Task task)
{
    return messaging_task_flush(task);
}

Task MessageSystemTask(Task task)
{
    return task_registry_register(IPC_MSG_TYPE_SYSTEM, task);
}

#if TRAPSET_NFC
Task MessageNfcTask(Task task)
{
    return task_registry_register(IPC_MSG_TYPE_NFC, task);
}
#endif /* TRAPSET_NFC */

uint16 MessagesPendingForTask(Task task, int32 *first_due)
{
    return messaging_task_num_pending_messages(task, first_due);
}

bool MessagePendingFirst(Task task, MessageId id, int32 *first_due)
{
    return messaging_task_first_pending_message(task, id, first_due);
}

uint16 MessagePendingMatch(Task task, bool once, MessageMatchFn match_fn)
{
    return messaging_task_pending_match(task, once, match_fn);
}

void MessageRetain(MessageId id, Message m)
{
    messaging_retain(id, m);
}

void MessageFree(MessageId id, Message data)
{
    messaging_free(id, data);
}

#if TRAPSET_OSAL
MessageQueue MessageQueueCreate(void)
{
    return messaging_queue_create();
}

void MessageQueueDestroy(MessageQueue queue)
{
    messaging_queue_destroy(queue);
}

MessageId MessageQueueWait(MessageQueue queue, Message *msg)
{
    return messaging_queue_wait(queue, msg);
}
#endif /* TRAPSET_OSAL */

/**
 * This isn't needed because our MessageLoop is the real scheduler.  However,
 * some customers might want to write their own MessageLoop, so we might need to
 * implement this in due course.
 */
void MessageWait(void * m)
{
    UNUSED(m);
}

void MessageLoop(void)
{
#ifndef DESKTOP_TEST_BUILD
    /* Do not insert anything between here and the scheduler call */
    L2_DBG_MSG3("MAIN Boot took %d instructions, %d stalls, %d clocks",
                hal_get_reg_num_instrs(), hal_get_reg_num_core_stalls(),
                hal_get_reg_num_run_clks());
#endif
    sched();
}
