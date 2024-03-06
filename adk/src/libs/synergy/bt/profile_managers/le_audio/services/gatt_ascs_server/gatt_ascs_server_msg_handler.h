/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_ASCS_SERVER_MSG_HANDLER_H_
#define GATT_ASCS_SERVER_MSG_HANDLER_H_

#include "gatt_ascs_server_private.h"

/***************************************************************************
NAME
    ascsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void ascsServerMsgHandler(void* task, MsgId id, Msg msg);


/***************************************************************************
NAME
    sendAscsServerReadAccessRsp

DESCRIPTION
    Handler for Sending the ASCS Read request sent to the library in the server role.
*/
void sendAscsServerReadAccessRsp(CsrBtGattId gattId, 
                                 ConnectionId cid,
                                 uint16 handle, 
                                 uint16 result,
                                 uint16 sizeValue, 
                                 uint8 *const value);

/***************************************************************************
NAME
    sendAscsServerWriteAccessRsp

DESCRIPTION
    Handler for Sending the ASCS Write request sent to the library in the server role.
*/
void sendAscsServerWriteAccessRsp(CsrBtGattId gattId,
                                  ConnectionId cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 sizeValue,
                                  uint8*  value);
#endif

