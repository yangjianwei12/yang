/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Unexpected message handlers.

*/

#include "logging.h"
#include "av.h"
#include "app_setup_unexpected_message.h"
#include "unexpected_message.h"

static void app_HandleUnexpectedMessage(MessageId id)
{
#ifdef INCLUDE_AV
    DEBUG_LOG_VERBOSE("app_HandleUnexpectedMessage, MESSAGE:0x%x, av = %d", id,  AvGetTaskData()->bitfields.state);
#else
    DEBUG_LOG_VERBOSE("app_HandleUnexpectedMessage, MESSAGE:0x%x", id);
#endif
}

static void app_HandleUnexpectedSysMessage(MessageId id)
{
    DEBUG_LOG_VERBOSE("app_HandleUnexpectedSysMessage, id = MESSAGE:0x%x", id);
}

void App_SetupUnexpectedMessage(void)
{
    UnexpectedMessage_RegisterHandler(app_HandleUnexpectedMessage);
    UnexpectedMessage_RegisterSysHandler(app_HandleUnexpectedSysMessage);
}
