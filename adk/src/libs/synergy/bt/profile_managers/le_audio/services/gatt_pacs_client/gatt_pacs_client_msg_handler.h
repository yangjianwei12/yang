/******************************************************************************
 Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_PACS_CLIENT_MSG_HANDLER_H
#define GATT_PACS_CLIENT_MSG_HANDLER_H

#include "gatt_pacs_client_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      pacs_client_msg_handler
 *
 *  DESCRIPTION
 *      message handler for GATT messages
 *
 *  PARAMETERS
 *      gash:        pointer to client instance pointer
 *----------------------------------------------------------------------------*/

void PacsClientMsgHandler(void** gash);

/*----------------------------------------------------------------------------*
 *  NAME
 *      pacsClientSendInitCfm
 *
 *  DESCRIPTION
 *      Send PACS client initialization confirmation
 *
 *  PARAMETERS
 *      pacs_client:        PACS client instance
 *      status:             status code of operation
 *----------------------------------------------------------------------------*/

void pacsClientSendInitCfm(GPacsC *const pacsClient, CsrBtResultCode status);

/*----------------------------------------------------------------------------*
 *  NAME
 *      pacsClientSendTerminateCfm
 *
 *  DESCRIPTION
 *      message handler for GATT messages
 *
 *  PARAMETERS
 *      pacs_client:       PACS client instance
 *----------------------------------------------------------------------------*/

void pacsClientSendTerminateCfm(GPacsC *const pacsClient);

void pacsClientWriteRequestSend(CsrBtGattId gattId,
                                connection_id_t cid,
                                uint16 handle,
                                bool notifyEnable);

#endif /* GATT_PACS_CLIENT_MSG_HANDLER_H */

