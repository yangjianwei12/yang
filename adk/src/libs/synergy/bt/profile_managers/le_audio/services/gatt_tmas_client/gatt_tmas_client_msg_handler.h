/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_MSG_HANDLER_H_
#define GATT_TMAS_CLIENT_MSG_HANDLER_H_

#include <message.h>

/***************************************************************************
NAME
    gattTmasClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void gattTmasClientMsgHandler(Task task, MsgId id, Msg msg);

#endif
