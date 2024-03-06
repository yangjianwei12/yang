/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

/* 
    FILE NAME
    gatt_transmit_power_server_msg_handler.h

DESCRIPTION
    Header file for the Transmit Power Server Service Message Handler.
*/

/*!
@file   gatt_transmit_power_server_msg_handler.h
@brief  Header file for the Transmit Power Server Service Message Handler.

        This file documents the message handler API for Transmit Power Server.
*/


#ifndef GATT_TRANSMIT_POWER_SERVER_MSG_HANDLER_H_
#define GATT_TRANSMIT_POWER_SERVER_MSG_HANDLER_H_

#include "gatt_transmit_power_server_private.h"


/***************************************************************************
NAME
    tpsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void tpsServerMsgHandler(void* task, MsgId id, Msg msg);


/***************************************************************************
NAME
    sendTpsServerReadAccessRsp

DESCRIPTION
    Function for Sending Read response to a read request by the remote Transmit Power client
*/
void sendTpsServerReadAccessRsp(CsrBtGattId gattId, 
                                 connection_id_t cid,
                                 uint16 handle, 
                                 uint16 result,
                                 uint16 size_value, 
                                 uint8 *const value);

/***************************************************************************
NAME
    sendTpsServerWriteAccessRsp

DESCRIPTION
    Function for Sending Write response to a write request by the remote Transmit Power client
*/
void sendTpsServerWriteAccessRsp(CsrBtGattId gattId,
                                  connection_id_t cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 size_value,
                                  uint8*  value);



#endif /* GATT_TRANSMIT_POWER_SERVER_MSG_HANDLER_H_ */

