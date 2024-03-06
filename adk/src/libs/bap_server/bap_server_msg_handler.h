/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#ifndef BAP_SERVER_MSG_HANDLER_H_
#define BAP_SERVER_MSG_HANDLER_H_

#include <message.h>
#include "gatt_ascs_server.h"

/*
NAME
    bapMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the Server role.
*/
void BapServerMsgHandler(Task task, MessageId id, Message payload);

/*!
NAME
    bapServerHandleGattAscsServerMsg
    
DESCRIPTION
    Bap server handler for Gatt Ascs Server messages.
*/
void bapServerHandleGattAscsServerMsg(BAP *bapInst, void *message);

/*!
NAME
    bapServerHandleGattAscsServerMsg
    
DESCRIPTION
    Bap server handler for Connection Library messages.
*/
void bapServerHandleClMsg(BAP *bapInst, MessageId id, void *message);

#endif
