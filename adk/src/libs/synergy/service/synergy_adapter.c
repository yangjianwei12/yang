/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy task adapter functions
*/

#include "synergy_private.h"
#include <message.h>

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/

/** Task ID mask */
#define TASK_ID_MASK                        0xFF00

/** Converts index into Task ID */
#define GET_TASK_ID(_index)                 ((_index) | TASK_ID_MASK)

/** Checks if a Task ID belongs to a Synergy Trap adapter task */
#define IS_TRAP_ADAPTER_TASK(_taskId)       (((_taskId) & TASK_ID_MASK) == TASK_ID_MASK)

/** Converts Task ID in index */
#define GET_TASK_INDEX(_taskId)             ((uint8) (_taskId) & ~TASK_ID_MASK)

/* Maximum number of Trap tasks which may communicate with Oxygen tasks */
#ifdef APP_TRAP_TASKS_MAX
#undef APP_TRAP_TASKS_MAX
#endif

#define APP_TRAP_TASKS_MAX                  96

/* NULL trap task ID */
#define APP_TRAP_TASK_NULL                  (APP_TRAP_TASKS_MAX + 1)

/******************************************************************************
 * Global and Local Declarations
 ******************************************************************************/

/* List of trap tasks communicating with Oxygen tasks */
static Task trapTaskList[APP_TRAP_TASKS_MAX];

/* Variable used to keep track of next available free slot in trap task list */
static uint8 freeIndex = 0;


/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! \brief Obtains an ID for the requested Trap task. If the requested task is already
           there in task list, its ID is returned. Otherwise,new trap task is stored 
           in the next empty slot and slot ID is returned. Once the maximum index
           in trap task list is reached, first empty slot in the trap task list is
           used to store any new trap task.If requested trap task is NULL, NULL trap 
           task ID is returned.

    \param task Destination task.

    \return queue-id for a task.
 */
static uint8 synergyTrapIndex(Task task)
{
    if (task)
    {
        uint8 i, curIndex = APP_TRAP_TASKS_MAX;

        for (i = 0; i < APP_TRAP_TASKS_MAX; i++)
        {
            if (trapTaskList[i])
            {
                if(trapTaskList[i] == task)
                    return i;
            }
            else if((curIndex ==  APP_TRAP_TASKS_MAX) && (freeIndex == APP_TRAP_TASKS_MAX))
            {
                curIndex =  i;
            }
        }

        if(freeIndex != APP_TRAP_TASKS_MAX)
        {
            curIndex = freeIndex;
            freeIndex++;
        }

        if(curIndex < APP_TRAP_TASKS_MAX)
            trapTaskList[curIndex] = task;

        return curIndex;
    }
    else
    {
        return APP_TRAP_TASK_NULL;
    }
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/

bool SynergyAdapterMessageSend(uint16 queue_id, MessageId mi, void *mv)
{
    if (IS_TRAP_ADAPTER_TASK(queue_id))
    {
        uint8 index = GET_TASK_INDEX(queue_id);

        if (index < APP_TRAP_TASKS_MAX && trapTaskList[index])
        { /* Target Trap task found, relay the message to it */
            SYNERGY_TASK_MSG_LOG("Put Msg task=0x%08x, mi=0x%04X, msgID=0x%08X",
                                 trapTaskList[index], mi, mv == NULL ? 0 : *(uint16 *) mv);
            MessageSend(trapTaskList[index],
                        (uint16) (SYNERGY_EVENT_BASE + mi),
                        mv);
        }
        else
        {
            SYNERGY_TASK_MSG_LOG("Put Msg no trap task found, mi=0x%04X, msgID=0x%04X",
                                 mi, *(uint16 *) mv);

            /* Release contents of the message */
            mv = CsrBtFreeUpstreamMessageContents(mi, mv);
            pfree(mv);

            /*PanicFalse(index == APP_TRAP_TASK_NULL);*/
        }
    }
    else
    {
        PanicFalse(FALSE);
    }

    return TRUE;
}

uint16 SynergyAdapterTaskIdGet(Task task)
{
    uint8 index = synergyTrapIndex(task);

    /*PanicFalse(index < APP_TRAP_TASKS_MAX);*/

    return (GET_TASK_ID(index));
}

Task SynergyAdapterGetTaskFromHandle(uint16 queue_id)
{
    if (IS_TRAP_ADAPTER_TASK(queue_id))
    {
        uint8 index = GET_TASK_INDEX(queue_id);

        if (index < APP_TRAP_TASKS_MAX)
        {
            return trapTaskList[index];
        }
    }

    return NULL;
}

bool SynergyAdapterTaskUnregister(Task task)
{
    if (task)
    {
        uint8 i;

        for (i = 0; i < APP_TRAP_TASKS_MAX; i++)
        {
            if (trapTaskList[i] == task)
            {
                trapTaskList[i] = NULL;
                return TRUE;
            }
        }
    }
    return FALSE;
}
