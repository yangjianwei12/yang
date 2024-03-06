/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_SERVER_PRIVATE_H
#define GATT_TMAS_SERVER_PRIVATE_H

#include "gatt_tmas_server.h"

/* Client data. This structure contains data for each connected client */
typedef struct
{
    connection_id_t      cid;
} gattTmasClientData;

/* Definition of data required for association. */
typedef struct
{
    uint16              role;
    gattTmasClientData  connectedClients[GATT_TMAS_MAX_CONNECTIONS];
} gattTmasServerData;

/* The TMA service internal structure for the server role. */
typedef struct __GTMAS
{
    AppTaskData        libTask;
    AppTask            appTask;

    ServiceHandle      srvcHndl;     /* Service handle provided by the service_handle lib when the server memory instance is created */
    uint16             startHandle;
    uint16             endHandle;

    CsrBtGattId        gattId;
    gattTmasServerData data;
} GTMAS;


#endif /* GATT_TMAS_SERVER_PRIVATE_H */
