/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifndef GATT_GMAS_SERVER_PRIVATE_H
#define GATT_GMAS_SERVER_PRIVATE_H

#include "gatt_gmas_server.h"

/* Client data. This structure contains data for each connected client */
typedef struct
{
    connection_id_t      cid;
} GattGmasClientData;

/* Definition of data required for association. */
typedef struct
{
    GmasRole            role;
#if defined(ENABLE_GMAP_UGG_BGS)
    GmasUggFeatures     uggFeatures;
    GmasBgsFeatures     bgsFeatures;
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
    GmasUgtFeatures     ugtFeatures;
    GmasBgrFeatures     bgrFeatures;
#endif
    GattGmasClientData  connectedClients[GATT_GMAS_MAX_CONNECTIONS];
} GattGmasServerData;

/* The GMA service internal structure for the server role. */
typedef struct __GGMAS
{
    AppTaskData        libTask;
    AppTask            appTask;

    ServiceHandle      srvcHndl;
    uint16             startHandle;
    uint16             endHandle;

    CsrBtGattId        gattId;
    GattGmasServerData data;
} GGMAS;


#endif /* GATT_GMAS_SERVER_PRIVATE_H */
