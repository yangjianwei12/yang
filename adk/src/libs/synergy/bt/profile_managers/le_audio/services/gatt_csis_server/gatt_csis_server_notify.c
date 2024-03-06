/******************************************************************************
 Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_notify.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_debug.h"

/***************************************************************************/
void csisServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        uint8  *const value
        )
{

    uint8* data;

    data = (uint8*)CsrPmemZalloc(size_value);
    CsrMemCpy(data, value, size_value);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_CSIS_SERVER_PANIC(
                    "CSIS: Invalid GattId!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_CSIS_SERVER_PANIC(
                    "CSIS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattNotificationEventReqSend(task,
                                          cid,
                                          handle,
                                          size_value,
                                          data);
    }
}

void csisServerNotifySirkChange(GCSISS_T *csis_server, connection_id_t cid)
{
    uint8 i, j;
    uint16 client_config;
    uint8 *value = (uint8*)CsrPmemAlloc(GATT_CSIS_SERVER_SIRK_SIZE);
    bool notify_all = (cid != 0 ? FALSE : TRUE);

    if ((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_OOB_ONLY) == GATT_CSIS_SHARE_SIRK_OOB_ONLY)
    {
        /* Server does not want to notify in such cases even if SIRK changes locally*/
        return;
    }

    if ((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_ENCRYPTED) == GATT_CSIS_SHARE_SIRK_ENCRYPTED)
    {
        /* server will notify after generating encrypted sirk in csisServerhandleSirkOperation
         */
        return;
    }
    else
    {
        /* default plain text */
        value[0] = GATT_CSIS_PLAIN_TEXT_SIRK;

        for (i = 0, j = GATT_CSIS_SERVER_SIRK_SIZE - 1; i < GATT_CSIS_SERVER_SIRK_SIZE && j >= 0; i++, j--)
        {
            value[1 + i] = csis_server->data.sirk[j];
        }
    }

    for (i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if (csis_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == csis_server->data.connected_clients[i].cid))
        {
            client_config = GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_CSIS_SERVER_CCC_NOTIFY)
            {
                csisServerSendCharacteristicChangedNotification(
                        (csis_server->gattId),
                        csis_server->data.connected_clients[i].cid,
                        HANDLE_SIRK,
                        GATT_CSIS_SERVER_SIRK_SIZE,
                        value
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }
}


void csisServerNotifySizeChange(GCSISS_T *csis_server, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint8 value;
    bool notify_all = (cid != 0 ? FALSE : TRUE);

    value = csis_server->data.cs_size;

    for (i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if (csis_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == csis_server->data.connected_clients[i].cid))
        {
            client_config = GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[i].client_cfg.sizeValueClientCfg);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_CSIS_SERVER_CCC_NOTIFY)
            {
                csisServerSendCharacteristicChangedNotification(
                        (csis_server->gattId),
                        csis_server->data.connected_clients[i].cid,
                        HANDLE_SIZE,
                        GATT_CSIS_SERVER_SIZE_CHARACTERISTCIS_SIZE,
                        &value
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }
}

void csisServerNotifyLockChange(GCSISS_T *csis_server, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint8 value;

    bool notify_all = (cid != 0 ? FALSE : TRUE);

    /* Get the Lock value */
    value = (uint8)csisServerGetLockState();

    for (i = 0; i < GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if (csis_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == csis_server->data.connected_clients[i].cid))
        {
            client_config = GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_CSIS_SERVER_CCC_NOTIFY)
            {
                csisServerSendCharacteristicChangedNotification(
                    (csis_server->gattId),
                    csis_server->data.connected_clients[i].cid,
                    HANDLE_LOCK,
                    GATT_CSIS_SERVER_LOCK_SIZE,
                    &value
                );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }
}

void csisServerNotifyLockChangeOtherClients(GCSISS_T* csis_server, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint8 value;

    /* Get the Lock value */
    value = (uint8) csisServerGetLockState();

    for (i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if (csis_server->data.connected_clients[i].cid != 0 &&
            csis_server->data.connected_clients[i].cid != cid)
        {
            client_config = GET_CSIS_CLIENT_CONFIG(csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_CSIS_SERVER_CCC_NOTIFY)
            {
                csisServerSendCharacteristicChangedNotification(
                        (csis_server->gattId),
                        csis_server->data.connected_clients[i].cid,
                        HANDLE_LOCK,
                        GATT_CSIS_SERVER_LOCK_SIZE,
                        &value
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }
}

