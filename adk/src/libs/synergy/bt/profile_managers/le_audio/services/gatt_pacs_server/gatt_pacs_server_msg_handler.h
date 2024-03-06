/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_PACS_SERVER_MSG_HANDLER_H_
#define GATT_PACS_SERVER_MSG_HANDLER_H_

#include "gatt_pacs_server_private.h"
/***************************************************************************
NAME
    pacsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void pacsServerMsgHandler(void* task, MsgId id, Msg msg);

#endif

