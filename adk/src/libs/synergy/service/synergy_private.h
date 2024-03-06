/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy internal interface
*/

#ifndef SYNERGY_PRIVATE_H_
#define SYNERGY_PRIVATE_H_

#include "synergy_tasks.h"
#include "synergy.h"
#include "csr_list.h"
#include <trapsets.h>
#include <panic.h>
#include <message.h>

#if defined(CSR_LOG_ENABLE)
#include "platform/csr_hydra_log.h"
#endif

#define TRAP_INTERVAL(_interval)        (_interval / 1000)

#define SYNERGY_EVENT_BASE              (CL_MESSAGE_BASE - CSR_BT_CPL_PRIM_BASE)
#define SYNERGY_EVENT_COUNT             CSR_BT_NUMBER_OF_CSR_BT_EVENTS
#define SYNERGY_EVENT_MAX               (SYNERGY_EVENT_BASE + SYNERGY_EVENT_COUNT)

#define SYNERGY_TIMER_COUNTER_MAX       0x7E
#define SYNERGY_TIMER_COUNTER_INIT      0x01

#define SYNERGY_INVALID_CONTEXT         ((void *) 0x0FFFFFFF)

#if defined(CSR_LOG_ENABLE)
#define SYNERGY_TASK_MSG_LOG(...)       SYNERGY_DEBUG_LOG_GENERIC(LOG_TASK, __VA_ARGS__)
#else
#define SYNERGY_TASK_MSG_LOG(...)
#endif

/* Shut lint up */
#ifndef UNUSED
#define UNUSED(x)       ((void)(/*lint -e{530}*/(x)))
#endif

/*!
    Synergy messages
 */
enum
{
    SYNERGY_MSG_TASK = SYNERGY_EVENT_MAX + 1,

    SYNERGY_MSG_RESTORE,

    SYNERGY_MSG_TIMER,

    SYNERGY_MSG_COUNT = (SYNERGY_MSG_TIMER + SYNERGY_TIMER_COUNTER_MAX)
};

/*!
    Structure for timer messages
 */
typedef struct
{
    void            (*fn)(uint16 mi, void *mv);
    uint16          fniarg;
    void            *fnvarg;
} SYNERGY_SERVICE_TIMER_MSG_T;

/*!
    Structure to save messages to synergy tasks
 */
typedef struct
{
    CsrCmnListElm_t     iter; /* Expected to be first element */

    SYNERGY_TASK_ID_T   task_id;
    uint16              event_class;
    void                *msg;
} SYNERGY_MSG_QUEUE_T;

/*!
    Structure for synergy task data
 */
typedef struct
{
    TaskData            task;
    void                *inst;
} SYNERGY_TASK_T;

/*!
    Structure to synergy service information
 */
typedef struct
{
    /*! Main synergy service task */
    TaskData            service_task;

    CsrCmnListSimple_t  msg_q;           /* List of SYNERGY_MSG_QUEUE_T */

    SYNERGY_TASK_T      synergy_tasks[SYNERGY_TASK_ID_MAX];

    SYNERGY_TASK_ID_T   current_task_id;
    uint8               timer_counter;
} SYNERGY_SERVICE_INST_T;

/*! \brief Synergy service information. */
extern SYNERGY_SERVICE_INST_T synergy_service_inst;

#if defined(CSR_LOG_ENABLE)
/*! \brief Synergy logging level control. */
extern synergy_debug_log_level_t debug_log_level_synergy;
#endif

/*! \brief Gets first message element for given task.

    \param _task_id Synergy task id.

    \return First message element if found else NULL.
 */
#define SynergyMsgQSearchByTask(_task_id)                                       \
    CsrCmnListSearchOffsetUint8((CsrCmnList_t *) &synergy_service_inst.msg_q,   \
                                offsetof(SYNERGY_MSG_QUEUE_T, task_id),         \
                                (_task_id))

/*! \brief Posts from Synergy task to other trap tasks.

    \param queue_id Destination queue-id (mapping queue-id to task is done \ref SynergyAdapterTaskIdGet).
    \param mi Message ID.
    \param mv Message.

    \return TRUE if message is posted to destination task.
 */
bool SynergyAdapterMessageSend(uint16 queue_id, MessageId mi, void *mv);

/*! \brief Gets trap queue task based on the given queue handle.

    \param queue_id Destination queue-id (mapping queue-id to task is done \ref SynergyAdapterTaskIdGet).

    \return Task for the given queue_id if present, NULL otherwise.
 */
Task SynergyAdapterGetTaskFromHandle(uint16 queue_id);

#endif /* SYNERGY_PRIVATE_H_ */
