/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_access.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_utils.h"

#define GATT_PACS_SERVER_INVALID_CID_INDEX  (0xFF)

static void pacsServerSendConfigChangeIndication(GPACSS_T  *const pacs_server,
    connection_id_t cid,
    bool configChangeComplete)
{
    /* Indicate the application all CCCD are written by client */
    MAKE_PACS_MESSAGE(GattPacsServerConfigChangeInd);

    message->pacsServiceHandle = pacs_server->srvc_hndl;
    message->cid = cid;
    message->id = GATT_PACS_SERVER_CONFIG_CHANGE_IND;
    message->configChangeComplete = configChangeComplete;

    PacsServerMessageSend(pacs_server->app_task, message);
}

static bool pacsServerAllClientConfigWritten(GPACSS_T  *const pacs_server,
    connection_id_t cid)
{
    uint8 i;

    for(i = 0; i < GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if(pacs_server->data.connected_clients[i].cid == cid &&
            pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg1 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg2 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg3 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg1 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg2 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg3 != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sinkAudioLocationsClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.sourceAudioLocationsClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.availableAudioContextsClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.supportedAudioContextsClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.vsAptXSinkPacClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG &&
            pacs_server->data.connected_clients[i].client_cfg.vsAptXSourcePacClientCfg != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void pacsServerSetClientConfigWrite(GPACSS_T  *const pacs_server,
    connection_id_t cid,
    uint16 handle,
    bool clientConfigChanged)
{
    bool configChangeComplete = pacsServerAllClientConfigWritten(pacs_server, cid);

    /* Except Audio Context in PACS other handles does not change values
       This needs to be revisted if others change their values too.
       We don't want to flood unncessary application for remote CCCD write on
       char which does not change their values at all.
     */
    if (clientConfigChanged &&
        (handle == HANDLE_AVAILABLE_AUDIO_CONTEXTS_CLIENT_CONFIG))
    {
        /* Inform above layer about CCCD change*/
        pacsServerSendConfigChangeIndication(pacs_server,
                                         cid,
                                         configChangeComplete);
    }
}


static bool pacsServerClientConfigChanged(uint16 clientCfg, uint8 newClientCfg)
{
    /* Check if the client config has changed, to notify above layer */
    if(((uint8)clientCfg) != newClientCfg)
        return TRUE;

    return FALSE;
}

/***************************************************************************
NAME
    sendPacsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendPacsServerAccessRsp(CsrBtGattId task,
                                    connection_id_t cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    uint8 *const value)
{
    uint8* data;

    data = (uint8*)CsrPmemZalloc(size_value);
    CsrMemCpy(data, value, size_value);

    CsrBtGattDbReadAccessResSend(
                                task,
                                cid,
                                handle,
                                result,
                                size_value,
                                data);
}

/***************************************************************************
NAME
    sendPacsServerAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT  library.
*/
static void sendPacsServerAccessErrorRsp(CsrBtGattId task,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                            uint16 error)
{
    sendPacsServerAccessRsp(task,
                            access_ind->cid,
                            access_ind->handle,
                            error,
                            0,
                            NULL);
}

/***************************************************************************
NAME
    sendPacsServerWriteAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
static void sendPacsServerWriteAccessRsp(CsrBtGattId task,
                                        connection_id_t cid,
                                        uint16 handle,
                                        uint16 result,
                                        uint16 size_value,
                                        uint8 *const value)
{
    CSR_UNUSED(size_value);
    CSR_UNUSED(value);

    CsrBtGattDbWriteAccessResSend(task,
                                  cid,
                                  handle,
                                  (CsrBtGattDbAccessRspCode)result);
}

/***************************************************************************
NAME
    sendPacsServerWriteAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*/

static void sendPacsServerWriteAccessErrorRsp(CsrBtGattId task,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                            uint16 error)
{
    sendPacsServerWriteAccessRsp(task,
                                access_ind->cid,
                                access_ind->handle,
                                error,
                                0,
                                NULL);
}



/***************************************************************************
NAME
    pacsServerServiceAccess

DESCRIPTION
    Deals with access of the HANDLE_PUBLISHED_AUDIO_CAPABILITY_SERVICE handle.
*/
static void pacsServerServiceAccess(CsrBtGattId task, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendPacsServerAccessRsp(task,
                                access_ind->cid,
                                HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                0,
                                NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Write of PACS not allowed. */
        sendPacsServerWriteAccessErrorRsp(task, access_ind, CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED);
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendPacsServerAccessErrorRsp(task, access_ind, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

/******************************************************************************/
static uint8 pacsServerGetCidIndex(const GPACSS_T *pacs_server, connection_id_t cid)
{
    uint8 index = GATT_PACS_SERVER_INVALID_CID_INDEX;
    uint8 i;

    for (i=0; i<GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if(pacs_server->data.connected_clients[i].cid == cid)
        {
            index = i;
            break;
        }
    }

    return index;
}

/***************************************************************************/
static status_t pacsServerSetCCC( GPACSS_T  *const pacs_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = pacsServerGetCidIndex(pacs_server, cid);
    bool clientConfigChanged = FALSE;

    if(index_client != GATT_PACS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_1)
        {
            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg1,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg1 = ccc[0];
        }
        else if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_2)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg2,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg2 = ccc[0];
        }
        else if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_3)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg3,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg3 = ccc[0];
        }
        else if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_VS_APTX)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSinkPacClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSinkPacClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG_1)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg1,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg1 = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG_2)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg2,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg2 = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG_3)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg3,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg3 = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG_VS_APTX)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSourcePacClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSourcePacClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SINK_AUDIO_LOCATIONS_CLIENT_CONFIG)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sinkAudioLocationsClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sinkAudioLocationsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_AUDIO_LOCATIONS_CLIENT_CONFIG)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.sourceAudioLocationsClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.sourceAudioLocationsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_AVAILABLE_AUDIO_CONTEXTS_CLIENT_CONFIG)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.availableAudioContextsClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.availableAudioContextsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SUPPORTED_AUDIO_CONTEXTS_CLIENT_CONFIG)
        {
            clientConfigChanged = pacsServerClientConfigChanged(
                  pacs_server->data.connected_clients[index_client].client_cfg.supportedAudioContextsClientCfg,
                                  ccc[0]);

            pacs_server->data.connected_clients[index_client].client_cfg.supportedAudioContextsClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_PACS_SERVER_ERROR("Invalid handle!\n");
            return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
        }

        /* Inform application for client write operation */
        pacsServerSetClientConfigWrite(pacs_server, cid, handle, clientConfigChanged);
    }
    else
    {
        /* Invalid cid */
        GATT_PACS_SERVER_ERROR("Invalid cid!\n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_PDU;
    }

    return CSR_BT_GATT_ACCESS_RES_SUCCESS;
}


/***************************************************************************/
static void pacsHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_PACS_SERVER_CCC_VALUE_SIZE];

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_PACS_SERVER_PANIC(
                    "PACS: Null instance!\n"
                    );
    }
    else if (cid == CSR_BT_CONN_ID_INVALID)
    {
        GATT_PACS_SERVER_PANIC(
                    "PACS: Null instance!\n"
                    );
    }

     /* Default value of clientConfig is set as 0xFFFF. If client has not written
        any CCCD then we need to replace 0xFFFF with 0x0 (Disable) while
        responding. Default value is changed from 0 to 0xFFFF because of
        CCCD values getting lost if the device panics without these values are
        passed to application.
      */
    if(client_config != GATT_PACS_SERVER_INVALID_CLIENT_CONFIG)
    {
        config_data[0] = (uint8)(client_config & 0xFF);
        config_data[1] = (uint8)(client_config >> 8);
    }
    else
    {
        config_data[0] = 0;
        config_data[1] = 0;
    }

    sendPacsServerAccessRsp(
            task,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            GATT_PACS_SERVER_CCC_VALUE_SIZE,
            config_data
            );
}


/***************************************************************************/
static bool pacsHandleWriteClientConfigAccess(
        GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    bool res = FALSE;

    if (access_ind->size_value != GATT_PACS_SERVER_CCC_VALUE_SIZE)
    {
        sendPacsServerWriteAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if ( access_ind->value[0] == GATT_PACS_SERVER_CCC_INDICATE )
    {
        sendPacsServerWriteAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF
                );
    }
    else if ( access_ind->value[0] == GATT_PACS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0 )
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        status_t status = pacsServerSetCCC(
                                   pacs_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        sendPacsServerWriteAccessRsp(
             pacs_server->gattId,
             access_ind->cid,
             access_ind->handle,
             status,
             0,
             NULL
             );
        res = TRUE;
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        sendPacsServerWriteAccessRsp(
             pacs_server->gattId,
             access_ind->cid,
             access_ind->handle,
             CSR_BT_GATT_ACCESS_RES_SUCCESS,
             0,
             NULL
             );
    }

    return res;
}

static void pacsHandleClientConfigAccess(GPACSS_T *const pacs_server,
              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
              uint16 client_config)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        pacsHandleReadClientConfigAccess(
                    pacs_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    client_config
                    );
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        pacsHandleWriteClientConfigAccess(
                    pacs_server,
                    access_ind
                    );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                    pacs_server->gattId,
                    access_ind,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                    );
    }

}

/***************************************************************************
NAME
    pacsHandleAudioLocationAccess

DESCRIPTION
    Deals with access of the HANDLE_SINK_AUDIO_LOCATIONS handle and
    HANDLE_SOURCE_AUDIO_LOCATIONS handle.
*/
static void pacsHandleAudioLocationAccess(
        const GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint32 audio_location
        )
{
    uint8 audio_location_resp[GATT_PACS_AUDIO_LOCATION_VALUE_SIZE];

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        audio_location_resp[0] = (uint8)audio_location & 0xFF;
        audio_location_resp[1] = (uint8)(audio_location >> 8) & 0xFF;
        audio_location_resp[2] = (uint8)(audio_location >> 16) & 0xFF;
        audio_location_resp[3] = (uint8)(audio_location >> 24) & 0xFF;

        sendPacsServerAccessRsp(
                pacs_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_PACS_AUDIO_LOCATION_VALUE_SIZE,
                audio_location_resp
                );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    pacsHandleVsPacRecordAccess

DESCRIPTION
    Deals with access of the HANDLE_SINK_PAC_VS_APTX handle and
    HANDLE_SOURCE_PAC_VS_APTX handle.
*/
static void pacsHandleVsPacRecordAccess(
        const GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 maxRspValueLength
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint16 size_value = 0;
        uint8 *value = NULL;
        uint8 pacRecordlen = 0;

        value = getGeneratedPacsRecord(&pacRecordlen, access_ind->handle);

        if (value)
        {
            size_value = (uint16) pacRecordlen;

            if(access_ind->offset > size_value)
            {
                sendPacsServerAccessErrorRsp(
                        pacs_server->gattId,
                        access_ind,
                        CSR_BT_GATT_ACCESS_RES_INVALID_OFFSET);
            }
            else
            {
                if(access_ind->offset == size_value)
                {
                    sendPacsServerAccessRsp(pacs_server->gattId,
                                            access_ind->cid,
                                            access_ind->handle,
                                            CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                            0,
                                            NULL);
                }
                else
                {
                    uint16 sizeToSend = (size_value - access_ind->offset) > (maxRspValueLength) ?
                                (maxRspValueLength) : (size_value - access_ind->offset);
    
                    sendPacsServerAccessRsp(pacs_server->gattId,
                                            access_ind->cid,
                                            access_ind->handle,
                                            CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                            sizeToSend,
                                            &value[access_ind->offset]);
                }
            }
    
         }
        else
        {
            GATT_PACS_SERVER_DEBUG("pac_record_rsp is NULL\n");
        }
    }

    else
    {
        sendPacsServerAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    pacsHandlePacRecordAccess

DESCRIPTION
    Deals with access of the HANDLE_SINK_PAC_RECORD handle and
    HANDLE_SOURCE_PAC_RECORD handle.
*/
static void pacsHandlePacRecordAccess(
        const GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    uint8 pac_record_length;
    uint8 *pac_record_rsp;

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        pac_record_rsp = getGeneratedPacsRecord(&pac_record_length, access_ind->handle);

        if (pac_record_rsp)
        {
            sendPacsServerAccessRsp(
                pacs_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                pac_record_length,
                pac_record_rsp
                );
        }
        else
        {
            GATT_PACS_SERVER_ERROR("pac_record_rsp is NULL\n");
        }
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}


/***************************************************************************
NAME
    pacsHandleAudioLocationAccess

DESCRIPTION
    Deals with access of the HANDLE_AVAILABLE_AUDIO_CONTEXTS handle and
    HANDLE_SUPPORTED_AUDIO_CONTEXTS handle.
*/
static void pacsHandleAudioContextsAccess(
        GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint32 audio_contexts
        )
{
    uint8 audio_contexts_resp[GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE];

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 index = GATT_PACS_SERVER_INVALID_CID_INDEX;

        /* Check if higher layer has set control of Available Audio
         * Context control. If yes, send an indication to higher
         * layer for the response
         */
        if(pacs_server->data.audioContextAvailabiltyControlApp 
            && access_ind->handle == HANDLE_AVAILABLE_AUDIO_CONTEXTS)
        {
            /* Indicate the application for remote read request */
            MAKE_PACS_MESSAGE(GattPacsServerAvailableAudioContextReadInd);

            message->cid = access_ind->cid;
            message->id = GATT_PACS_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND;

            PacsServerMessageSend(pacs_server->app_task, message);

            return;
        }

        /* Check if selective available audio context is set for
         * connected client or not. If yes, we need to send
         * selective available audio context to that connected
         * client. If not, send default system wide available
         * audio context to the connected client.
         */
        if (pacsServerGetDeviceIndexFromCid(pacs_server, access_ind->cid , &index) &&
            pacs_server->data.connected_clients[index].selectiveAudioContexts != 0)
        {
            audio_contexts_resp[0] = (uint8)pacs_server->data.connected_clients[index].selectiveAudioContexts & 0xFF;
            audio_contexts_resp[1] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 8) & 0xFF;
            audio_contexts_resp[2] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 16) & 0xFF;
            audio_contexts_resp[3] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 24) & 0xFF;
        }
        else
        {
            audio_contexts_resp[0] = (uint8)audio_contexts & 0xFF;
            audio_contexts_resp[1] = (uint8)(audio_contexts >> 8) & 0xFF;
            audio_contexts_resp[2] = (uint8)(audio_contexts >> 16) & 0xFF;
            audio_contexts_resp[3] = (uint8)(audio_contexts >> 24) & 0xFF;
        }

        sendPacsServerAccessRsp(
                pacs_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE,
                audio_contexts_resp
                );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                pacs_server->gattId,
                access_ind,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************/

void handlePacsServerAccessInd(GPACSS_T *pacs_server,
              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
              uint16 maxRspValueLength)
{
    uint8 index_client = pacsServerGetCidIndex(pacs_server, access_ind->cid);

    if (index_client == GATT_PACS_SERVER_INVALID_CID_INDEX)
    {
        GATT_PACS_SERVER_ERROR(" \n PACS Server: handlePacsServerAccessInd, "
                                              "Invalid CID Index \n");
        return;
    }

    switch (access_ind->handle)
    {
        case HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE:
        {
            /* Service Handle read request */
            pacsServerServiceAccess(pacs_server->gattId, access_ind);
            break;
        }

        case HANDLE_SINK_PAC_1:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SINK_PAC_2:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SINK_PAC_3:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SINK_PAC_VS_APTX:
        {
            pacsHandleVsPacRecordAccess(pacs_server, access_ind, maxRspValueLength);
            break;
        }

        case HANDLE_SOURCE_PAC_1:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SOURCE_PAC_2:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SOURCE_PAC_3:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind);
            break;
        }

        case HANDLE_SOURCE_PAC_VS_APTX:
        {
            pacsHandleVsPacRecordAccess(pacs_server, access_ind, maxRspValueLength);
            break;
        }

        case HANDLE_SINK_AUDIO_LOCATIONS:
        {
            pacsHandleAudioLocationAccess(pacs_server,
                                      access_ind,
                                      pacs_server->data.sink_audio_source_location);
            break;
        }

        case HANDLE_SOURCE_AUDIO_LOCATIONS:
        {
            pacsHandleAudioLocationAccess(pacs_server,
                                      access_ind,
                                      pacs_server->data.source_audio_source_location);
            break;
        }

        case HANDLE_AVAILABLE_AUDIO_CONTEXTS:
        {
            pacsHandleAudioContextsAccess(pacs_server,
                                      access_ind,
                                      pacs_server->data.available_audio_contexts);
            break;
        }

        case HANDLE_SUPPORTED_AUDIO_CONTEXTS:
        {
            pacsHandleAudioContextsAccess(pacs_server,
                                      access_ind,
                                      pacs_server->data.supported_audio_contexts);
            break;
        }

        case HANDLE_SINK_PAC_CLIENT_CONFIG_1:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg1);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SINK_PAC_CLIENT_CONFIG_2:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg2);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SINK_PAC_CLIENT_CONFIG_3:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg3);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SINK_PAC_CLIENT_CONFIG_VS_APTX:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSinkPacClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                       access_ind,
                                       client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG_1:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg1);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG_2:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg2);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG_3:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg3);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG_VS_APTX:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.vsAptXSourcePacClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                         access_ind,
                                         client_config);
            break;
        }

        case HANDLE_SINK_AUDIO_LOCATIONS_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sinkAudioLocationsClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_AUDIO_LOCATIONS_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sourceAudioLocationsClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_AVAILABLE_AUDIO_CONTEXTS_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.availableAudioContextsClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SUPPORTED_AUDIO_CONTEXTS_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.supportedAudioContextsClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        default:
        {
            sendPacsServerAccessErrorRsp(
                        pacs_server->gattId,
                        access_ind,
                        CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE
                        );
            break;
        }
    } /* switch */
}

