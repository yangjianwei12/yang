/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_MSG_HANDLER_H_
#define GATT_MICS_CLIENT_MSG_HANDLER_H_

#include <csrtypes.h>
#include <message.h>

/***************************************************************************
NAME
    gattMicsClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattMicsClientMsgHandler(AppTask task, MsgId id, Msg msg);

#endif
