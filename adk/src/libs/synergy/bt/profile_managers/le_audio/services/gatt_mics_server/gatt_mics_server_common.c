/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#include "gatt_mics_server_common.h"

#include "gatt_mics_server_private.h"
#include "gatt_mics_server_debug.h"

 /******************************************************************************/
 static void micsServerSendConfigChangeIndication(GMICS_T *mics,
                            connection_id_t cid)
{
    /* Indicate the application all CCCD are written by client */
    MAKE_MICS_MESSAGE(GattMicsServerConfigChangeInd);

    message->srvcHndl = mics->srvcHandle;
    message->cid = cid;
    message->configChangeComplete = TRUE;

    MicsMessageSend(mics->appTask, GATT_MICS_SERVER_CONFIG_CHANGE_IND, message);
}
/******************************************************************************/
void sendMicsServerAccessRsp(
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
    SynMemCpyS(data, sizeValue ,value, sizeValue);

    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 data);
}

/******************************************************************************/

void gattMicsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MICS_SERVER_PANIC(
                    "GMICS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_MICS_SERVER_PANIC(
                    "GMICS: No Cid!\n"
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
void micsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = CsrPmemZalloc(sizeValue);
    SynMemCpyS(data, sizeValue, value, sizeValue);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MICS_SERVER_PANIC(
                    "GMICS: No GattId!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_MICS_SERVER_PANIC(
                    "GMICS: No Cid!\n"
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
void micsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 const clientConfig
        )
{
    uint8 configData[MICS_SERVER_CLIENT_CONFIG_VALUE_SIZE];

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_MICS_SERVER_PANIC(
                    "GMCS: Invalid GattId!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_MICS_SERVER_PANIC(
                    "GMCS: Null instance!\n"
                    );
    }

    configData[0] = (uint8)(clientConfig & 0xFF);
    configData[1] = (uint8)(clientConfig >> 8);

    sendMicsServerAccessRsp(
                 task,
                 cid,
                 handle,
                 CSR_BT_GATT_ACCESS_RES_SUCCESS,
                 MICS_SERVER_CLIENT_CONFIG_VALUE_SIZE,
                 configData
                 );
}

/***************************************************************************/
bool micsHandleWriteClientConfigAccess(
        GMICS_T *mics,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 *clientConfig
        )
{
    uint16 newClientConfig;

    if (accessInd->size_value != MICS_SERVER_CLIENT_CONFIG_VALUE_SIZE)
    {
        sendMicsServerAccessErrorRsp(
                mics->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );

        return FALSE;
    }

    newClientConfig = accessInd->value[0] | (((uint16) accessInd->value[1]) << 8);

    /* Validate the input parameters - ONLY Notify*/
    if ( (newClientConfig == MICS_SERVER_NOTIFY ) ||
         (newClientConfig == 0 ))
    {
        /* Check if the client config has changed, to notify above layer */
        if((*clientConfig) != newClientConfig)
        {
            /* Store the new client config */
            (*clientConfig) = newClientConfig;

           /* Inform above layer all CCCD handles are written by client
            * In mics server we have just one Client config, so inform right
            * away if client config has changed
            */
            micsServerSendConfigChangeIndication(mics, accessInd->cid);
        }

        /* Send response to the client */
        gattMicsServerWriteGenericResponse(
                    mics->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    accessInd->handle
                    );

        return TRUE;
    }

    /* Send error response to the client */
    sendMicsServerAccessErrorRsp(
            mics->gattId,
            accessInd->cid,
            accessInd->handle,
            CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
            );

    return FALSE;
}

/******************************************************************************/
uint8 micsServerGetCidIndex(GMICS_T *micControlServer, connection_id_t cid)
{
    uint8 index = GATT_MICS_SERVER_INVALID_CID_INDEX;
    uint8 i;

    for (i=0; i<MICS_MAX_CONNECTIONS; i++)
    {
        if(micControlServer->data.connectedClients[i].cid == cid)
        {
            index = i;
            break;
        }
    }
    return index;
}

