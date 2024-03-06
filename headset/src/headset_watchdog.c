/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_watchdog.c
\brief      Provide watchdog support
*/

#ifdef INCLUDE_WATCHDOG
#include "headset_watchdog.h"
#include "headset_config.h"

#include <watchdog.h>
#include <logging.h>
#include <message.h>
#include <panic.h>

#define WD_KICK_EVENT   0x1000
#define WD_STOP_EVENT   0x1001

CREATE_WATCHDOG(HeadsetWatchdog);

static void appWatchdog_HandleMessage(Task task, MessageId id, Message message);

/*! headset watchdog task data */
typedef struct
{
   TaskData task;
   uint16 kick_time_ms;
   uint8 timeout_sec;
   bool is_initialized:1;
   bool is_started:1;
}appWatchdogTaskData;

#define appWatchdog_GetTaskData()       (&app_watchdog_taskdata)
#define appWatchdog_GetTask()           (&app_watchdog_taskdata.task)
#define appWatchdog_IsInitialized()     (app_watchdog_taskdata.is_initialized)
#define appWatchdog_IsStarted()         (app_watchdog_taskdata.is_started)

/*! Initialise the task data */
static appWatchdogTaskData app_watchdog_taskdata;

/*! \brief Send event message. */
static bool appWatchdog_Trigger(uint16 event_id)
{
    if(appWatchdog_IsInitialized())
    {
        MessageSend(appWatchdog_GetTask(), event_id, NULL);
        return TRUE;
    }
    else
    {
        DEBUG_LOG_WARN("appWatchdog_Trigger Failed, event %d", event_id);
        return FALSE;
    }
}

bool AppWatchdog_Start(void)
{
    appWatchdogTaskData *wd_data = appWatchdog_GetTaskData();
    bool status;

    status = appWatchdog_IsStarted() ? FALSE : appWatchdog_Trigger(WD_KICK_EVENT);
    DEBUG_LOG_INFO("AppWatchdog_Start kick_time_ms %d, timeout_sec %d, status: %d", wd_data->kick_time_ms, wd_data->timeout_sec, status);
    return status;
}

bool AppWatchdog_Stop(void)
{
    bool status;

    status = appWatchdog_IsStarted() ? appWatchdog_Trigger(WD_STOP_EVENT) : FALSE;
    DEBUG_LOG_WARN("AppWatchdog_Stop: status: %d", status);
    return status;
}

/*! \brief Kick the watchdog. */
static void appWatchdog_Kick(void)
{
    DEBUG_LOG_VERBOSE("appWatchdog_Kick");
    appWatchdogTaskData *wd_data = appWatchdog_GetTaskData();

    if (wd_data->timeout_sec && wd_data->kick_time_ms)
    {
        Watchdog_Kick(&HeadsetWatchdog, wd_data->timeout_sec );
        MessageCancelAll(appWatchdog_GetTask(), WD_KICK_EVENT);
        MessageSendLater(appWatchdog_GetTask(), WD_KICK_EVENT, NULL, wd_data->kick_time_ms);
        wd_data->is_started = TRUE;
    }
}

/*! \brief Stop the watchdog. */
static void appWatchdog_Stop(void)
{
    DEBUG_LOG_VERBOSE("appWatchdog_Stop");
    MessageCancelAll(appWatchdog_GetTask(), WD_KICK_EVENT);
    Watchdog_Stop(&HeadsetWatchdog);
    appWatchdog_GetTaskData()->is_started = FALSE;
}

static void appWatchdog_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    DEBUG_LOG_V_VERBOSE("appWatchdog_HandleMessage %d", id);

    switch(id)
    {
        case WD_KICK_EVENT:
            appWatchdog_Kick();
        break;
        case WD_STOP_EVENT:
            appWatchdog_Stop();
        break;
        default:
        DEBUG_LOG_WARN("appWatchdog_HandleMessage unhandled %d", id);
        break;
    }
}

bool AppWatchdog_Init(Task init_task)
{
    UNUSED(init_task);
    appWatchdogTaskData *wd_data = appWatchdog_GetTaskData();
    wd_data->task.handler = appWatchdog_HandleMessage;
    wd_data->is_initialized = FALSE;
    wd_data->is_started = FALSE;
    wd_data->kick_time_ms = appConfigWatchdogKickTimeMs();
    wd_data->timeout_sec = appConfigWatchdogTimeout();

    Watchdog_Init();

    wd_data->is_initialized = TRUE;
    return TRUE;
}

#endif /* INCLUDE_WATCHDOG */
