/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/
#include "gatt_mcs_server_common.h"

#include "gatt_mcs_server_private.h"
#include "gatt_mcs_server_debug.h"


/******************************************************************************/
void sendMcsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = (uint8*)CsrPmemAlloc(sizeValue);

    if (data == NULL)
    {
        GATT_MCS_SERVER_PANIC(
            "GMCS: insufficient resources!\n"
            );
        return;
    }

    memcpy(data, value, sizeValue);

    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 data);
}

/******************************************************************************/

void gattMcsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattDbWriteAccessResSend(task,
                                      cid,
                                      handle,
                                      result);
    }
}

/***************************************************************************/
void mcsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = (uint8*)CsrPmemAlloc(sizeValue);

    if (data == NULL)
    {
        GATT_MCS_SERVER_PANIC(
            "GMCS: insufficient resources!\n"
            );
        return;
    }

    memcpy(data, value, sizeValue);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: No GattId!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattNotificationEventReqSend(task,
                                         cid,
                                         handle,
                                         sizeValue,
                                         data);
    }
}


/***************************************************************************/
void mcsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        )
{
    uint8 *configData;
    configData = (uint8*)CsrPmemAlloc(sizeof(uint8) * MCS_SERVER_CLIENT_CONFIG_VALUE_SIZE);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: Invalid GattId!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_MCS_SERVER_PANIC(
                    "GMCS: Null instance!\n"
                    );
    }

    if (configData)
    {
        configData[0] = (uint8)(clientConfig & 0xFF);
        configData[1] = (uint8)(clientConfig >> 8);

        sendMcsServerAccessRsp(
                             task,
                             cid,
                             handle,
                             CSR_BT_GATT_ACCESS_RES_SUCCESS,
                             MCS_SERVER_CLIENT_CONFIG_VALUE_SIZE,
                             configData
                             );

        CsrPmemFree(configData);
    }
}

/***************************************************************************/
bool mcsHandleWriteClientConfigAccess(
        CsrBtGattId task,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 *clientConfig
        )
{
    uint16 newClientConfig;

    if (accessInd->size_value != MCS_SERVER_CLIENT_CONFIG_VALUE_SIZE)
    {
        sendMcsServerAccessErrorRsp(
                task,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );

        return FALSE;
    }

    newClientConfig = accessInd->value[0] | (((uint16) accessInd->value[1]) << 8);

    /* Validate the input parameters - ONLY Notify*/
    if ( (newClientConfig == MCS_SERVER_NOTIFY ) ||
         (newClientConfig == 0 ))
    {
        /* Store the new client config */
        (*clientConfig) = newClientConfig;

        /* Send response to the client */
        gattMcsServerWriteGenericResponse(
                    task,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        return TRUE;
    }

    /* Send error response to the client */
    sendMcsServerAccessErrorRsp(
            task,
            accessInd->cid,
            accessInd->handle,
            CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
            );

    return FALSE;
}

/* Memory reallocation api which take pointer and frees the memory
 * and then reallocates the memory for required size*/

void* McsMemRealloc(void* ptr, uint16 *len , uint16 newLen, uint16 maxLen)
{
    void* newPtr;

    uint16 size = (newLen > maxLen ? maxLen : newLen);
    newPtr = CsrPmemAlloc(size);

    if (newPtr)
    {
        if (ptr)
            CsrPmemFree(ptr);
        *len = size;
        return newPtr;
    }
    return ptr;
}
