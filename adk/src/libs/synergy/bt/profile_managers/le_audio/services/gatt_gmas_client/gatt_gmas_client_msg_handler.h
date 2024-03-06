/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_MSG_HANDLER_H_
#define GATT_GMAS_CLIENT_MSG_HANDLER_H_

#include <message.h>

/***************************************************************************
NAME
    gattGmasClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattGmasClientMsgHandler(Task task, MsgId id, Msg msg);

#endif
