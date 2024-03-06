/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#include "gatt_telephone_bearer_server_common.h"

#include "gatt_telephone_bearer_server_private.h"
#include "gatt_telephone_bearer_server_debug.h"


/******************************************************************************/
void sendTbsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = CsrPmemZalloc(sizeValue);
    CsrMemCpy(data, value, sizeValue);
    CsrBtGattDbReadAccessResSend(
                                 task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 data);
}

/******************************************************************************/

void gattTelephoneBearerServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_TBS_SERVER_PANIC(
                    "GTBS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_TBS_SERVER_ERROR(
                    "GTBS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattDbWriteAccessResSend(
                                      task,
                                      cid,
                                      handle,
                                      result);
    }
}

/***************************************************************************/
void tbsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = CsrPmemZalloc(sizeValue);
    CsrMemCpy(data, value, sizeValue);
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_TBS_SERVER_PANIC(
                    "GTBS: No GattId!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_TBS_SERVER_ERROR(
                    "GTBS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattNotificationEventReqSend(
                                          task,
                                          cid,
                                          handle,
                                          sizeValue,
                                          data);
    }
}

/***************************************************************************/
void tbsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        )
{
    uint8 *configData;
    configData = (uint8*)CsrPmemZalloc(sizeof(uint8) * CLIENT_CONFIG_VALUE_SIZE);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_TBS_SERVER_PANIC(
                    "GTBS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_TBS_SERVER_ERROR(
                    "GTBS: No Cid!\n"
                    );
    }

    configData[0] = (uint8)(clientConfig & 0xFF);
    configData[1] = (uint8)(clientConfig >> 8);

    sendTbsServerAccessRsp(
            task,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            CLIENT_CONFIG_VALUE_SIZE,
            configData
            );
}

/***************************************************************************/
bool tbsHandleWriteClientConfigAccess(
        CsrBtGattId task,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 *clientConfig
        )
{
    uint16 newClientConfig;

    if (accessInd->size_value != CLIENT_CONFIG_VALUE_SIZE)
    {
        sendTbsServerAccessErrorRsp(
                task,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );

        return FALSE;
    }

    newClientConfig = accessInd->value[0] | (((uint16) accessInd->value[1]) << 8);

    /* Validate the input parameters - ONLY Notify*/
    if ( (newClientConfig == CLIENT_CONFIG_NOTIFY) ||
         (newClientConfig == 0 ))
    {
        /* Store the new client config */
        (*clientConfig) = newClientConfig;

        /* Send response to the client */
        gattTelephoneBearerServerWriteGenericResponse(
                    task,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        return TRUE;
    }

    /* Send error response to the client */
    sendTbsServerAccessErrorRsp(
            task,
            accessInd->cid,
            accessInd->handle,
            CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
            );

    return FALSE;
}

void TbsMessageSendLater(AppTask task, void *msg)
{
    CsrSchedMessagePut(task, TBS_SERVER_PRIM, msg);
}

/* Memory reallocation api which take pointer and frees the memory
 * and then reallocates the memory for required size*/

void *MemRealloc(void* ptr, uint16 size)
{
    void* newPtr;
    if(ptr)
        CsrPmemFree(ptr);

    newPtr = CsrPmemAlloc(size);
    if(newPtr)
    {
        return newPtr;
    }
    return NULL;
}
