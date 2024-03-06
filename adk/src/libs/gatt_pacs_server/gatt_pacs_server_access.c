/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <vm.h>

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_access.h"
#include "gatt_pacs_server_pac_record.h"

#define GATT_PACS_SERVER_INVALID_CID_INDEX  (0xFF)

/***************************************************************************
NAME
    sendPacsServerAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
static void sendPacsServerAccessRsp(Task task,
                                    uint16 cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    const uint8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_PACS_SERVER_PANIC(("Couldn't send GATT access response\n"));
    }
}

/***************************************************************************
NAME
    sendPacsServerAccessErrorRsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*/
static void sendPacsServerAccessErrorRsp(Task task,
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
    pacsServerServiceAccess

DESCRIPTION
    Deals with access of the HANDLE_PUBLISHED_AUDIO_CAPABILITY_SERVICE handle.
*/
static void pacsServerServiceAccess(Task task, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendPacsServerAccessRsp(task,
                                access_ind->cid,
                                HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE,
                                gatt_status_success,
                                0,
                                NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Write of PACS not allowed. */
        sendPacsServerAccessErrorRsp(task, access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        sendPacsServerAccessErrorRsp(task, access_ind, gatt_status_request_not_supported);
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
static gatt_status_t pacsServerSetCCC(const GPACSS_T *pacs_server,
                            connection_id_t cid,
                            uint16 handle,
                            uint8 *ccc)
{
    uint8 index_client = pacsServerGetCidIndex(pacs_server, cid);

    if(index_client != GATT_PACS_SERVER_INVALID_CID_INDEX)
    {
        if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_1)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg1 = ccc[0];
        }
        else if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_2)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.sinkPacClientCfg2 = ccc[0];
        }
        else if (handle == HANDLE_SINK_PAC_CLIENT_CONFIG_VS)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.vsSinkPacClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_PAC_CLIENT_CONFIG_VS)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.vsSourcePacClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SINK_AUDIO_LOCATIONS_CLIENT_CONFIG)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.sinkAudioLocationsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SOURCE_AUDIO_LOCATIONS_CLIENT_CONFIG)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.sourceAudioLocationsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_AVAILABLE_AUDIO_CONTEXTS_CLIENT_CONFIG)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.availableAudioContextsClientCfg = ccc[0];
        }
        else if (handle == HANDLE_SUPPORTED_AUDIO_CONTEXTS_CLIENT_CONFIG)
        {
            pacs_server->data.connected_clients[index_client].client_cfg.supportedAudioContextsClientCfg = ccc[0];
        }
        else
        {
            /* Invalid handle */
            GATT_PACS_SERVER_DEBUG_INFO(("Invalid handle!\n"))
            return gatt_status_invalid_handle;
        }
    }
    else
    {
        /* Invalid cid */
        GATT_PACS_SERVER_DEBUG_INFO(("Invalid cid!\n"))
        return gatt_status_invalid_cid;
    }

    return gatt_status_success;
}


/***************************************************************************/
static void pacsHandleReadClientConfigAccess(
        Task task,
        uint16 cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_PACS_SERVER_CCC_VALUE_SIZE];

    if (task == NULL)
    {
        GATT_PACS_SERVER_DEBUG_PANIC((
                    "PACS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_PACS_SERVER_DEBUG_PANIC((
                    "PACS: Null instance!\n"
                    ));
    }

    config_data[0] = (uint8)client_config & 0xFF;
    config_data[1] = (uint8)client_config >> 8;

    sendPacsServerAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
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
        sendPacsServerAccessErrorRsp(
                (Task) &pacs_server->lib_task,
                access_ind,
                gatt_status_invalid_length
                );
    }
    /* Validate the input parameters - ONLY Notify*/
    else if ( access_ind->value[0] == GATT_PACS_SERVER_CCC_INDICATE )
    {
        sendPacsServerAccessErrorRsp(
                (Task) &pacs_server->lib_task,
                access_ind,
                gatt_status_cccd_improper_config
                );
    }
    else if ( access_ind->value[0] == GATT_PACS_SERVER_CCC_NOTIFY || access_ind->value[0] == 0 )
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        gatt_status_t status = pacsServerSetCCC(
                                   pacs_server,
                                   (connection_id_t) access_ind->cid,
                                   access_ind->handle,
                                   access_ind->value);

        /* Send response to the client */
        sendPacsServerAccessRsp(
             (Task) &pacs_server->lib_task,
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
        sendPacsServerAccessRsp(
             (Task) &pacs_server->lib_task,
             access_ind->cid,
             access_ind->handle,
             gatt_status_success,
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
                    (Task) &pacs_server->lib_task,
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
                    (Task) &pacs_server->lib_task,
                    access_ind,
                    gatt_status_request_not_supported
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
                (Task)&pacs_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_PACS_AUDIO_LOCATION_VALUE_SIZE,
                audio_location_resp
                );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                (Task)&pacs_server->lib_task,
                access_ind,
                gatt_status_request_not_supported
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
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        bool audioSink
        )
{
    uint8 pac_record_length;
    uint8 *pac_record_rsp;

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        pac_record_rsp = getGeneratedPacsRecord(audioSink, &pac_record_length, access_ind->handle);

        if(pac_record_rsp == NULL)
        {
            GATT_PACS_SERVER_DEBUG_PANIC((
                        "PACS: NULL instance!\n"
                        ));
        }

        sendPacsServerAccessRsp(
                (Task)&pacs_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                pac_record_length,
                pac_record_rsp
                );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                (Task)&pacs_server->lib_task,
                access_ind,
                gatt_status_request_not_supported
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
        const GPACSS_T *pacs_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint32 audio_contexts
        )
{
    uint8 audio_contexts_resp[GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE];

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        audio_contexts_resp[0] = (uint8)audio_contexts & 0xFF;
        audio_contexts_resp[1] = (uint8)(audio_contexts >> 8) & 0xFF;
        audio_contexts_resp[2] = (uint8)(audio_contexts >> 16) & 0xFF;
        audio_contexts_resp[3] = (uint8)(audio_contexts >> 24) & 0xFF;

        sendPacsServerAccessRsp(
                (Task)&pacs_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE,
                audio_contexts_resp
                );
    }
    else
    {
        sendPacsServerAccessErrorRsp(
                (Task)&pacs_server->lib_task,
                access_ind,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************/

void handlePacsServerAccessInd(GPACSS_T *pacs_server,
              const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    uint8 index_client = pacsServerGetCidIndex(pacs_server, access_ind->cid);

    switch (access_ind->handle)
    {
        case HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE:
        {
            /* Service Handle read request */
            pacsServerServiceAccess((Task) &pacs_server->lib_task, access_ind);
            break;
        }

        case HANDLE_SINK_PAC_1:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind, TRUE);
            break;
        }

        case HANDLE_SINK_PAC_2:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind, TRUE);
            break;
        }

        case HANDLE_SINK_PAC_VS:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind, TRUE);
            break;
        }

        case HANDLE_SOURCE_PAC:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind, FALSE);
            break;
        }

        case HANDLE_SOURCE_PAC_VS:
        {
            pacsHandlePacRecordAccess(pacs_server, access_ind, FALSE);
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

        case HANDLE_SINK_PAC_CLIENT_CONFIG_VS:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.vsSinkPacClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.sourcePacClientCfg);

            pacsHandleClientConfigAccess(pacs_server,
                                access_ind,
                                client_config);
            break;
        }

        case HANDLE_SOURCE_PAC_CLIENT_CONFIG_VS:
        {
            uint16 client_config =
                GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index_client].client_cfg.vsSourcePacClientCfg);

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
                        (Task) &pacs_server->lib_task,
                         access_ind,
                         gatt_status_invalid_handle
                         );
            break;
        }
    } /* switch */
}

