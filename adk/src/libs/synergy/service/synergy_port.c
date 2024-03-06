/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy porting functions
*/

#include "synergy_private.h"
#include "message.h"
#include "stream.h"

void SynergyStreamsSinkRegister(uint8 taskId, Sink sink);
void SynergyStreamsSourceRegister(uint8 taskId, Source src);
void bluestack_msg_free(uint16 id, void *msg);

static Task synergySchedGetTaskFromHandle(CsrSchedQid handle)
{
    return (handle < SYNERGY_TASK_ID_MAX ?
            /* Synergy task, get it from the list synergy_tasks */
            &synergy_service_inst.synergy_tasks[(uint8) handle].task :
            /* Non-synergy task, get it from the trap scheduler task list. */
            SynergyAdapterGetTaskFromHandle(handle));
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/

CsrBool CsrSchedMessageGet(uint16 *pmi, void **pmv)
{
    SYNERGY_MSG_QUEUE_T *msgQElem = (SYNERGY_MSG_QUEUE_T *) SynergyMsgQSearchByTask(synergy_service_inst.current_task_id);

    if (msgQElem)
    {
        *pmi = msgQElem->event_class;
        *pmv = msgQElem->msg;

        CsrCmnListElementRemove((CsrCmnList_t *) &synergy_service_inst.msg_q,
                                &msgQElem->iter);

        SYNERGY_TASK_MSG_LOG("Get Msg taskID=%d, mi=0x%04x, msgID=0x%08x",
                             synergy_service_inst.current_task_id, *pmi,
                             *pmv == NULL ? 0 :
                             (*pmi == MESSAGE_MORE_DATA || *pmi == MESSAGE_MORE_SPACE) ? *(uint32 *) *pmv : *(uint16 *) *pmv);
        return TRUE;
    }

    return FALSE;
}

void CsrSchedMessagePut(CsrSchedQid dst, CsrUint16 mi, void *mv)
{
    if (dst < SYNERGY_TASK_ID_MAX)
    {
        SYNERGY_TASK_MSG_LOG("Put Msg taskID=%d, mi=0x%04x, msgID=0x%08x",
                             dst, mi, mv == NULL ? 0 : *(uint16 *) mv);

        MessageSend(&synergy_service_inst.synergy_tasks[(uint8) dst].task, mi, mv);
    }
    else
    {
        SynergyAdapterMessageSend(dst, mi, mv);
    }
}

CsrSchedQid CsrSchedTaskQueueGet(void)
{
    return synergy_service_inst.current_task_id;
}

CsrSchedTid CsrSchedTimerSet(CsrTime delay, void (*fn) (CsrUint16 mi, void *mv), CsrUint16 fniarg, void *fnvarg)
{
    SYNERGY_SERVICE_TIMER_MSG_T *timer = CsrPmemAlloc(sizeof(*timer));

    timer->fn = fn;
    timer->fniarg = fniarg;
    timer->fnvarg = fnvarg;

    /* XXX: Here we are assuming that no timer will stay alive long enough for
     * timer_counter to rollover and reach same timer ID */
    synergy_service_inst.timer_counter++;
    if (synergy_service_inst.timer_counter >= SYNERGY_TIMER_COUNTER_MAX)
    {
        synergy_service_inst.timer_counter = SYNERGY_TIMER_COUNTER_INIT;
    }

    delay = TRAP_INTERVAL(delay);
    MessageSendLater(&synergy_service_inst.service_task,
                     SYNERGY_MSG_TIMER + synergy_service_inst.timer_counter,
                     timer,
                     delay);

    SYNERGY_TASK_MSG_LOG("TimerStart tid=%d, fn=0x%08x, fniarg=0x%08x, delay=0x%08x",
                         synergy_service_inst.timer_counter, fn, fniarg, delay);

    return synergy_service_inst.timer_counter;
}

CsrBool CsrSchedTimerCancel(CsrSchedTid timerId, CsrUint16 *pmi, void **pmv)
{
    UNUSED(pmi);
    UNUSED(pmv);

    if (timerId < SYNERGY_TIMER_COUNTER_MAX)
    {
        SYNERGY_TASK_MSG_LOG("TimerCancel tid=%d", timerId);

        /* ToDo: Need to extract pmi and pmv from the queue before cancelling */
        return MessageCancelFirst(&synergy_service_inst.service_task,
                                  SYNERGY_MSG_TIMER + timerId);
    }

    return FALSE;
}

CsrUint16 SynergySchedMessagePendingMatch(CsrSchedQid dst, CsrBool once, void *matchFunc)
{
    Task task = synergySchedGetTaskFromHandle(dst);

    if (task)
    {
        return MessagePendingMatch(task, once, (MessageMatchFn)matchFunc);
    }

    return 0;
}

CsrUint16 SynergySchedMessagesPendingForTask(CsrSchedQid dst, CsrInt32 *first_due)
{
    Task task = synergySchedGetTaskFromHandle(dst);

    if (task)
    {
        return MessagesPendingForTask(task, first_due);
    }

    return 0;
}

void SynergyStreamsSinkRegister(uint8 task_id, Sink sink)
{
    Task task = NULL;

    if (task_id < SYNERGY_TASK_ID_MAX)
    {
        task = &synergy_service_inst.synergy_tasks[task_id].task;
    }

    MessageStreamTaskFromSink(sink, task);
}

void SynergyStreamsSourceRegister(uint8 task_id, Source src)
{
    Task task = NULL;

    if (task_id < SYNERGY_TASK_ID_MAX)
    {
        task = &synergy_service_inst.synergy_tasks[task_id].task;
    }

    MessageStreamTaskFromSource(src, task);
}

void bluestack_msg_free(uint16 id, void *msg)
{
    if((MESSAGE_BLUESTACK_BASE_ - MESSAGE_BLUESTACK_BASE_) <= id &&
       id <= (MESSAGE_BLUESTACK_END_ - MESSAGE_BLUESTACK_BASE_))
    {
        id += MESSAGE_BLUESTACK_BASE_;
    }

    MessageFree(id, msg);
}
