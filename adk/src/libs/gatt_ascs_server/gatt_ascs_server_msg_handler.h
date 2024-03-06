/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ASCS_SERVER_MSG_HANDLER_H_
#define GATT_ASCS_SERVER_MSG_HANDLER_H_

#include "gatt_ascs_server.h"

/***************************************************************************
NAME
    ascsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void ascsServerMsgHandler(Task task, MessageId id, Message msg);


/***************************************************************************
NAME
    sendAscsServerAccessRsp

DESCRIPTION
    Handler for Sending the ASCS Read request sent to the library in the server role.
*/
void sendAscsServerAccessRsp(Task task, uint16 cid, uint16 handle, uint16 result,
                                    uint16 size_value, const uint8 *value);
#endif

