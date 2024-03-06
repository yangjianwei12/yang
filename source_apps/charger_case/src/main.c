/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       main.c
\brief      Main application task
*/

#include <connection.h>
#include <dfu.h>
#include <panic.h>
#include <pio.h>
#include <ps.h>
#include <string.h>
#include <boot.h>
#include <os.h>
#include <logging.h>
#include <app/message/system_message.h>

#include "app_task.h"
#include "charger_case_init.h"

#include "timestamp_event.h"
#include "unexpected_message.h"
#include "usb_device.h"

#include "system_state.h"


/* Make message enumerations (domain_message.h) available in pydbg.
    This needs to be in a C file */
PRESERVE_ENUM_FOR_DEBUGGING(message_groups)
PRESERVE_TYPE_FOR_DEBUGGING(message_base_t)

/*! Application data structure */
appTaskData globalApp;

/*! Application init start task data */
typedef struct appInitStartTaskData
{
    TaskData            task;                   /*!< Application task */
} appInitStartTaskData;

#define MSG_APP_INIT_START 0xFFF0

static void handleMessageAppInitStart(void)
{
    /* Start the application module and library initialisation sequence */

    DEBUG_LOG_INFO("handleMessageAppInitStart");
    ChargerCaseInit_StartInitialisation();
}

static void appHandleAppInit(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case MSG_APP_INIT_START:
        handleMessageAppInitStart();
        return;
    }

    UnexpectedMessage_HandleMessage(id);
}

/*! \brief Handle subsystem event report. */
static void appHandleSubsystemEventReport(MessageSubsystemEventReport *evt)
{
    UNUSED(evt);
    DEBUG_LOG("appHandleSubsystemEventReport, ss_id=%d, level=%d, id=%d, cpu=%d, occurrences=%d, time=%d",
        evt->ss_id, evt->level, evt->id, evt->cpu, evt->occurrences, evt->time);
}

/*! \brief System Message Handler

    This function is the message handler for system messages. They are
    routed to existing handlers. If execution reaches the end of the
    function then it is assumed that the message is unhandled.
*/
static void appHandleSystemMessage(Task task, MessageId id, Message message)
{
    bool handled = FALSE;

    UNUSED(task);

    switch (id)
    {
        case MESSAGE_SUBSYSTEM_EVENT_REPORT:
            appHandleSubsystemEventReport((MessageSubsystemEventReport *)message);
            return;

#ifdef INCLUDE_DFU
        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
        case MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS:
        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
            handled = Dfu_HandleSystemMessages(id, message, handled);
            break;
#endif /* INCLUDE_DFU */

        case MESSAGE_USB_ENUMERATED:
        case MESSAGE_USB_DECONFIGURED:
        case MESSAGE_USB_SUSPENDED:
        case MESSAGE_USB_ALT_INTERFACE:
        case MESSAGE_USB_ATTACHED:
        case MESSAGE_USB_DETACHED:
             UsbDevice_HandleMessage(id, message);
             return;

        default:
            break;
    }

    if (!handled)
    {
        UnexpectedMessage_HandleSysMessage(id);
    }
}

/*  Handler for the INIT_CFM message.

    Used to register the handler that decides whether to allow entry
    to low power mode, before passing the #APPS_COMMON_INIT_CFM message to
    the state machine handler.

    \param message The APPS_COMMON_INIT_CFM message received (if any).
 */
static void appHandleCommonInitCfm(void)
{
    TimestampEvent(TIMESTAMP_EVENT_INITIALISED);
    ChargerCaseInit_CompleteInitialisation();
}

/*! \brief Message Handler

    This function is the main message handler for the main application task, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
static void appHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {


        case SYSTEM_STATE_STATE_CHANGE:
        {
            SYSTEM_STATE_STATE_CHANGE_T *msg = (SYSTEM_STATE_STATE_CHANGE_T *)message;
            if(msg->old_state == system_state_initialisation && msg->new_state == system_state_initialised)
            {
                appHandleCommonInitCfm();
            }
        }
        return;
    }

    UnexpectedMessage_HandleMessage(id);
}

extern void _init(void);
void _init(void)
{
    /* Used to call Usb_TimeCriticalInit().
     *
     * This function (and it's weak copy in init_usb_early_init.c)
     * shall be removed when we're ready to delete usb_device_class library
     * and switch to the new USB device framework. */
}

/*! \brief Application entry point

    This function is the entry point for the application, it performs basic
    initialisation of the application task state machine and then sets
    the state to 'initialising' which will start the initialisation procedure.

    \returns Nothing. Only exits by powering down.
*/
int main(void)
{
    OsInit();

    TimestampEvent(TIMESTAMP_EVENT_BOOTED);
    
#if defined(SET_LOG_LEVEL) && !defined(DISABLE_DEBUG_LOG_LEVELS)
    /* Define SET_LOG_LEVEL in the project to change default log level.
       Use values defined in debug_log_level_t */
    LOG_LEVEL_CURRENT_SYMBOL = SET_LOG_LEVEL;
    debug_log_level__global = SET_LOG_LEVEL;
#endif

    /* Set up task handlers */
    appGetApp()->task.handler = appHandleMessage;
    appGetApp()->systask.handler = appHandleSystemMessage;

    appInitStartTaskData td;
    td.task.handler = appHandleAppInit;

    MessageSystemTask(appGetSysTask());

    MessageSend(&td.task, MSG_APP_INIT_START, NULL);

    /* Start the message scheduler loop */
    MessageLoop();

    /* We should never get here, keep compiler happy */
    return 0;
}
