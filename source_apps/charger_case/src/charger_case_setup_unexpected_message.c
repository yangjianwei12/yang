/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Unexpected message handlers.

*/

#include "logging.h"

#include "charger_case_sm.h"
#include "av.h"
#include "charger_case_setup_unexpected_message.h"

#include "unexpected_message.h"

static void chargerCase_HandleUnexpectedMessage(MessageId id)
{
#if defined(INCLUDE_AV)
    DEBUG_LOG_VERBOSE("chargerCase_HandleUnexpectedMessage, MESSAGE:0x%x, sm = %d, av = %d", id, SmGetTaskData()->state, AvGetTaskData()->bitfields.state);
#else
    UNUSED(id);
#endif
}

static void chargerCase_HandleUnexpectedSysMessage(MessageId id)
{
    DEBUG_LOG_VERBOSE("chargerCase_HandleUnexpectedSysMessage, id = MESSAGE:0x%x", id);
}



void ChargerCase_SetupUnexpectedMessage(void)
{
    UnexpectedMessage_RegisterHandler(chargerCase_HandleUnexpectedMessage);
    UnexpectedMessage_RegisterSysHandler(chargerCase_HandleUnexpectedSysMessage);
}
