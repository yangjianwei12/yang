/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_telephone_bearer_server_common.h"
#include "gatt_telephone_bearer_server_debug.h"


/******************************************************************************/
void sendTbsServerAccessRsp(
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
        )
{
    if (
            !GattManagerServerAccessResponse(
                task,
                cid,
                handle,
                result,
                size_value,
                value
                )
       )
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_INFO((
                    "Couldn't send GattManagerServerAccessRsp\n"
                    ));

    }
}

/******************************************************************************/

void gattTelephoneBearerServerWriteGenericResponse(
        Task        task,
        uint16      cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == NULL)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: No Cid!\n"
                    ));
    }
    else
    {
        sendTbsServerAccessRsp(
             task,
             cid,
             handle,
             result,
             0,
             NULL
             );
    }
}

/***************************************************************************/
void tbsServerSendCharacteristicChangedNotification(
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        )
{
    if (task == NULL)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: Null instance!\n"
                    ));
    }
    else if ( cid == 0 )
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: No Cid!\n"
                    ));
    }
    else
    {
        GattManagerRemoteClientNotify(
                                      task,
                                      cid,
                                      handle,
                                      size_value,
                                      value);
    }
}

/***************************************************************************/
void tbsHandleReadClientConfigAccess(
        Task task,
        uint16 cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[CLIENT_CONFIG_VALUE_SIZE];

    if (task == NULL)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: Null instance!\n"
                    ));
    }

    config_data[0] = (uint8)client_config & 0xFF;
    config_data[1] = (uint8)client_config >> 8;

    sendTbsServerAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
            sizeof(config_data),
            config_data
            );
}

/***************************************************************************/
bool tbsHandleWriteClientConfigAccess(
        Task task,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 *client_config
        )
{
    uint16 newClientConfig;

    if (access_ind->size_value != CLIENT_CONFIG_VALUE_SIZE)
    {
        sendTbsServerAccessErrorRsp(
                task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_invalid_length
                );

        return FALSE;
    }

    newClientConfig = access_ind->value[0] | (((uint16) access_ind->value[1]) << 8);

    /* Validate the input parameters - ONLY Notify*/
    if ( (newClientConfig == CLIENT_CONFIG_NOTIFY) ||
         (newClientConfig == 0 ))
    {
        /* Store the new client config */
        (*client_config) = newClientConfig;

        /* Send response to the client */
        gattTelephoneBearerServerWriteGenericResponse(
                    task,
                    access_ind->cid,
                    gatt_status_success,
                    access_ind->handle
                    );

        return TRUE;
    }

    /* Send error response to the client */
    sendTbsServerAccessErrorRsp(
            task,
            access_ind->cid,
            access_ind->handle,
            gatt_status_cccd_improper_config
            );

    return FALSE;
}
