/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_ASCS_CLIENT_MSG_HANDLER_H
#define GATT_ASCS_CLIENT_MSG_HANDLER_H

#include "gatt_ascs_client.h"

/***************************************************************************
NAME
    AscsClientMsgHandler

DESCRIPTION
    Handler for messages sent to the library and internal messages. 
    Expects notifications and indications
*/
void AscsClientMsgHandler(void **gash);

/*----------------------------------------------------------------------------*
 *  NAME
 *      ascsClientSendInitCfm
 *
 *  DESCRIPTION
 *      Send ASCS client initialization confirmation
 *
 *  PARAMETERS
 *      ascsClient:        ASCS client instance
 *      status:             status code of operation
 *----------------------------------------------------------------------------*/

void ascsClientSendInitCfm(GAscsC *const ascsClient, CsrBtResultCode status);

/*----------------------------------------------------------------------------*
 *  NAME
 *      ascsClientSendTerminateCfm
 *
 *  DESCRIPTION
 *      Send ASCS Client termination confirmation
 *
 *  PARAMETERS
 *      ascsClient:       ASCS client instance
 *----------------------------------------------------------------------------*/

void ascsClientSendTerminateCfm(GAscsC *const ascsClient, GattAscsClientStatus status);


#endif /* GATT_ASCS_CLIENT_MSG_HANDLER_H */

