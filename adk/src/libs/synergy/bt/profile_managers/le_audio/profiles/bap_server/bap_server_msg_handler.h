/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_SERVER_MSG_HANDLER_H_
#define BAP_SERVER_MSG_HANDLER_H_

#include "gatt_ascs_server.h"

/*
NAME
    bapMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the Server role.
*/
void BapServerMsgHandler(void **gash);

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
    Bap server handler for Connection Manager messages.
*/
void bapServerHandleCMMsg(BAP *bapInst, void *message);

void bapServerSendConfigChangeInd(BAP *bapInst,
    ConnectionId cid,
    BapServerConfigType configType,
    bool configChangeComplete);

#endif
