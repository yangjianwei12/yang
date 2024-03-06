/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy service functions
*/

#include "synergy_private.h"
#include "message.h"
#include "stream.h"
#include "csr_bt_td_db.h"
#include "csr_log_text_2.h"

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/

/* Maximum lifespan of unhandled Message More Data messages for a stream */
#ifndef MMD_STORAGE_TIME
#define MMD_STORAGE_TIME                 D_SEC(0.5)
#endif


/******************************************************************************
 * Global and Local Declarations
 ******************************************************************************/
SYNERGY_SERVICE_INST_T synergy_service_inst;

#if defined(CSR_LOG_ENABLE)
synergy_debug_log_level_t debug_log_level_synergy;
#endif

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! \brief Finds synergy task-id for given synergy task.

    \param synergy_task Synergy task.

    \return synergy task-id.
 */
static uint8 synergyTaskIdGet(SYNERGY_TASK_T *synergy_task)
{
     if (synergy_task < &synergy_service_inst.synergy_tasks[0] ||
         synergy_task > &synergy_service_inst.synergy_tasks[SYNERGY_TASK_ID_MAX - 1])
    {
        return SYNERGY_TASK_ID_MAX;
    }


    return synergy_task - &synergy_service_inst.synergy_tasks[0];
}

/*! \brief Adds message to synergy task.

    \param task_id      Task ID.
    \param id           Message ID.
    \param msg          Message.

    \return .
 */
static void synergyTaskMsgEnqueue(uint8 task_id, uint16 id, void *msg)
{
    SYNERGY_MSG_QUEUE_T *msgQElem;

    msgQElem = (SYNERGY_MSG_QUEUE_T *) CsrCmnListElementAddLast((CsrCmnList_t *) &synergy_service_inst.msg_q,
                                                                sizeof(SYNERGY_MSG_QUEUE_T));
    msgQElem->task_id = task_id;
    msgQElem->event_class = id;
    msgQElem->msg = msg;
}

/*! \brief Schedules synergy task.

    \param task_id      Task ID.
 */
static void synergyTaskSchedule(uint8 task_id)
{
    SYNERGY_TASK_T *synergy_task = &synergy_service_inst.synergy_tasks[task_id];
    SynergyTaskHandler[task_id](&synergy_task->inst);
}

/*! \brief Reschedules given task with trap scheduler if any messages are pending.

    \param task_id      Task ID.
 */
static void synergyTaskInspect(uint8 task_id)
{
    SYNERGY_TASK_T *synergy_task = &synergy_service_inst.synergy_tasks[task_id];

    if (SynergyMsgQSearchByTask(task_id))
    { /* Messages pending for this task, post housekeeping message */
        MessageSend(&synergy_task->task,
                    SYNERGY_MSG_RESTORE,
                    CsrMemDup(&task_id, sizeof(task_id)));
    }
}

/*! \brief Schedules synergy task.

    \param synergy_task Synergy task information.
    \param id           Message ID.
    \param msg          Message.

    \return .
 */
static void synergyTaskMsgHandler(SYNERGY_TASK_T *synergy_task, uint16 id, void *msg)
{
    synergy_service_inst.current_task_id = synergyTaskIdGet(synergy_task);

    PanicFalse(synergy_service_inst.current_task_id < SYNERGY_TASK_ID_MAX);

    synergyTaskMsgEnqueue(synergy_service_inst.current_task_id, id, msg);

    synergyTaskSchedule(synergy_service_inst.current_task_id);

    synergyTaskInspect(synergy_service_inst.current_task_id);
}

/*! \brief Schedules synergy task.

    \param synergy_task Synergy task information.
    \param p_task_id    Synergy Task ID.
 */
static void synergyMsgRestoreHandler(SYNERGY_TASK_T *synergy_task, const uint8 *p_task_id)
{
    uint8 task_id = *p_task_id;

    UNUSED(synergy_task);

    if (SynergyMsgQSearchByTask(task_id))
    { /* There are messages present in queue for this task */

        synergy_service_inst.current_task_id = task_id;
        PanicFalse(synergy_service_inst.current_task_id < SYNERGY_TASK_ID_MAX);

        synergyTaskSchedule(task_id);
        synergyTaskInspect(task_id);
    }

}

/*! \brief Synergy main timer handler.

    \param msg      Synergy timer callback information.
 */
static void synergyServiceMsgTimerHandler(CsrSchedTid timer_id, const SYNERGY_SERVICE_TIMER_MSG_T *msg)
{
    UNUSED(timer_id);

    SYNERGY_TASK_MSG_LOG("TimerExpiry tid=%d, fn=0x%08x, fniarg=0x%08x",
                         timer_id, msg->fn, msg->fniarg);
    msg->fn(msg->fniarg, msg->fnvarg);
}

/*! \brief Synergy task handler.

    \param task     Task Data.
    \param id       Message ID.
    \param msg      Message.
 */
static void synergyTaskHandler(Task task, MessageId id, Message msg)
{
    /* Panic if Task handler is NULL. This could happen if tasks are disabled. In which case,
     * we should not expect messages on these task ids.
     * (e.g. LEA tasks are disabled by default and are explicitly enabled by application using SynergyEnableLEATasks API)
     */
    SYNERGY_TASK_ID_T task_id = synergyTaskIdGet((SYNERGY_TASK_T *) task);
    PanicFalse((task_id < SYNERGY_TASK_ID_MAX) && (SynergyTaskHandler[task_id] != NULL));

    if (id == SYNERGY_MSG_RESTORE)
    {
        synergyMsgRestoreHandler((SYNERGY_TASK_T *) task, (const uint8 *) msg);
    }
    else
    {
        void *actual_msg = (void *) msg;

        if (id < SYNERGY_EVENT_MAX)
        {
            MessageRetain(id, msg);
        }
        else if (MESSAGE_BLUESTACK_BASE_ <= id && id <= MESSAGE_BLUESTACK_END_)
        {
            MessageRetain(id, msg);
            id -= MESSAGE_BLUESTACK_BASE_;
        }
        else if (id == MESSAGE_MORE_DATA || id == MESSAGE_MORE_SPACE)
        {
            /* The only stream messages handled by Synergy. */
            actual_msg = (MessageMoreData *) CsrMemDup(msg, sizeof(MessageMoreData));
        }
        else
        {
            SYNERGY_TASK_MSG_LOG("Ignoring id=0x%04X", id);
            actual_msg = NULL;
        }

        if (actual_msg != NULL)
        {
            synergyTaskMsgHandler((SYNERGY_TASK_T *) task, id, actual_msg);
        }
    }
}

/*! \brief Main syerngy service task handler.

    \param task     Task Data.
    \param id       Message ID.
    \param msg      Message.
 */
static void synergyServiceTaskHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    if (id >= SYNERGY_MSG_TIMER &&
        id < SYNERGY_MSG_TIMER + SYNERGY_TIMER_COUNTER_MAX)
    {
        synergyServiceMsgTimerHandler(id - SYNERGY_MSG_TIMER, (const SYNERGY_SERVICE_TIMER_MSG_T *) msg);
    }
    else
    {
        SYNERGY_TASK_MSG_LOG("Ignoring id=0x%04X", id);
    }
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/
void SynergyInit(uint16 numTdlDevices)
{
    SYNERGY_TASK_T *synergy_task;
    const schedEntryFunction_t *init_task;

#if defined(CSR_LOG_ENABLE)
    debug_log_level_synergy = SYNERGY_DEFAULT_LOG_LEVEL;
#endif

    synergy_service_inst.service_task.handler = synergyServiceTaskHandler;
    synergy_service_inst.timer_counter = SYNERGY_TIMER_COUNTER_MAX - 1;

    CsrBtTdDbInit(numTdlDevices);
    SynergyTaskBluestackRegister();

    for (synergy_task = &synergy_service_inst.synergy_tasks[0], init_task = SynergyTaskInit;
         synergy_task < &synergy_service_inst.synergy_tasks[SYNERGY_TASK_ID_MAX];
         synergy_task++, init_task++)
    {
        /*Some task require explicit enablement from application (e.g. LEA tasks are enabled
          using SynergyEnableLEATasks API). For them init_task might be NULL (if not enabled by app)
         */
        synergy_task->task.handler = synergyTaskHandler;
        synergy_service_inst.current_task_id = synergy_task - &synergy_service_inst.synergy_tasks[0];
            if(*init_task)
        (*init_task)(&synergy_task->inst);
    }
}

