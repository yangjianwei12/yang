/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/
/* 
    FILE NAME
    gatt_transport_discovery_server_msg_handler.h

DESCRIPTION
    Header file for the Transport Discovery Server Service Message Handler.
*/

/*!
@file   gatt_transport_discovery_server_msg_handler.h
@brief  Header file for the Transport Discovery Server Service Message Handler.

        This file documents the message handler API for Transport Discovery Server.
*/


#ifndef GATT_TRANSPORT_DISCOVERY_SERVER_MSG_HANDLER_H_
#define GATT_TRANSPORT_DISCOVERY_SERVER_MSG_HANDLER_H_

#include "gatt_transport_discovery_server_private.h"


/***************************************************************************
NAME
    TdsServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void TdsServerMsgHandler(void **gash);

/***************************************************************************
NAME
    tdsServerGattMsgHandler

DESCRIPTION
    Handler for GATT messages sent to the library in the server role.
*/
void tdsServerGattMsgHandler(void* task, MsgId id, Msg msg);

/***************************************************************************
NAME
    sendTdsServerClientConfigReadAccessRsp

DESCRIPTION
    Function for Sending Read response to a read request on the client config descriptor.
*/
void sendTdsServerClientConfigReadAccessRsp(const GTDS_T *tds, connection_id_t cid, uint16 clientConfig);


/***************************************************************************
NAME
    sendTdsServerControlPointIndication

DESCRIPTION
    Function for Sending Control Point characteristic indication.
*/
void sendTdsServerControlPointIndication(const GTDS_T *tds, connection_id_t cid, uint16 tdsIndSize, uint8 *tdsIndData);

/***************************************************************************
NAME
    sendTdsServerIndication

DESCRIPTION
    Sends an indication to the GATT library.
*/
void sendTdsServerIndication(CsrBtGattId gattId,
                             connection_id_t cid,
                             uint16 handle,
                             uint16 sizeValue,
                             uint8 *const value);

/***************************************************************************
NAME
    sendTdsServerReadAccessRsp

DESCRIPTION
    Function for Sending Read response to a read request by the remote Transport Discovery client
*/
void sendTdsServerReadAccessRsp(CsrBtGattId gattId, 
                                 connection_id_t cid,
                                 uint16 handle, 
                                 uint16 result,
                                 uint16 sizeValue, 
                                 uint8 *const value);

/***************************************************************************
NAME
    sendTdsServerWriteAccessRsp

DESCRIPTION
    Function for Sending Write response to a write request by the remote Transport Discovery client
*/
void sendTdsServerWriteAccessRsp(CsrBtGattId gattId,
                                  connection_id_t cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 sizeValue,
                                  uint8* value);

#endif /* GATT_TRANSPORT_DISCOVERY_SERVER_MSG_HANDLER_H_ */

