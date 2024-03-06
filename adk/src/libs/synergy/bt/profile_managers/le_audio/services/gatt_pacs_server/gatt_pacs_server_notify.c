/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_notify.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_utils.h"

/***************************************************************************/

static void pacsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        )
{
    uint8* data;

    data = (uint8*)CsrPmemZalloc(size_value);
    CsrMemCpy(data, value, size_value);

    if ( cid == 0 )
    {
        GATT_PACS_SERVER_PANIC(
                    "PACS: No Cid!\n"
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

void pacsServerNotifyAudioLocationChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint16 handle;
    uint8 audio_location[GATT_PACS_AUDIO_LOCATION_VALUE_SIZE];
    bool notify_all = (cid != 0 ? FALSE: TRUE);

    if (audioSink)
    {
        handle = HANDLE_SINK_AUDIO_LOCATIONS;
        audio_location[0] = (uint8)pacs_server->data.sink_audio_source_location & 0xFF;
        audio_location[1] = (uint8)(pacs_server->data.sink_audio_source_location >> 8) & 0xFF;
        audio_location[2] = (uint8)(pacs_server->data.sink_audio_source_location >> 16) & 0xFF;
        audio_location[3] = (uint8)(pacs_server->data.sink_audio_source_location >> 24) & 0xFF;
    }
    else
    {
        handle = HANDLE_SOURCE_AUDIO_LOCATIONS;
        audio_location[0] = (uint8)pacs_server->data.source_audio_source_location & 0xFF;
        audio_location[1] = (uint8)(pacs_server->data.source_audio_source_location >> 8) & 0xFF;
        audio_location[2] = (uint8)(pacs_server->data.source_audio_source_location >> 16) & 0xFF;
        audio_location[3] = (uint8)(pacs_server->data.source_audio_source_location >> 24) & 0xFF;
    }

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
        {
             if (audioSink)
             {
                 client_config = GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.sinkAudioLocationsClientCfg);
             }
             else
             {
                 client_config = GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.sourceAudioLocationsClientCfg);
             }

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (pacs_server->gattId),
                        pacs_server->data.connected_clients[i].cid,
                        handle,
                        GATT_PACS_AUDIO_LOCATION_VALUE_SIZE,
                        audio_location
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }

}

void pacsServerNotifySelectiveAudioContextsChange(GPACSS_T *pacs_server, connection_id_t cid)
{
    uint8 audioContexts[GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE];
    uint8 index, clientConfig;

    if(cid !=0 && pacsServerGetDeviceIndexFromCid(pacs_server, cid, &index) &&
        pacs_server->data.connected_clients[index].selectiveAudioContexts != 0)
    {
        audioContexts[0] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts) & 0xFF;
        audioContexts[1] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 8) & 0xFF;
        audioContexts[2] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 16) & 0xFF;
        audioContexts[3] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 24) & 0xFF;

        clientConfig = 
            GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.availableAudioContextsClientCfg);

        /* If the Client Config is 0x01 (Notify is TRUE), a notification will
         * be sent to the client */
        if (clientConfig == GATT_PACS_SERVER_CCC_NOTIFY)
        {
            pacsServerSendCharacteristicChangedNotification(
                    (pacs_server->gattId),
                    pacs_server->data.connected_clients[index].cid,
                    HANDLE_AVAILABLE_AUDIO_CONTEXTS,
                    GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE,
                    audioContexts
                    );
        }
    }
}

void pacsServerNotifyAudioContextsChange(GPACSS_T *pacs_server, bool supportedAudioContexts, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint16 handle;
    uint8 audio_contexts[GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE];
    bool notify_all = (cid != 0 ? FALSE: TRUE);

    if (supportedAudioContexts)
    {
        handle = HANDLE_SUPPORTED_AUDIO_CONTEXTS;
        audio_contexts[0] = (uint8)pacs_server->data.supported_audio_contexts & 0xFF;
        audio_contexts[1] = (uint8)(pacs_server->data.supported_audio_contexts >> 8) & 0xFF;
        audio_contexts[2] = (uint8)(pacs_server->data.supported_audio_contexts >> 16) & 0xFF;
        audio_contexts[3] = (uint8)(pacs_server->data.supported_audio_contexts >> 24) & 0xFF;
    }
    else
    {
        handle = HANDLE_AVAILABLE_AUDIO_CONTEXTS;
        audio_contexts[0] = (uint8)pacs_server->data.available_audio_contexts & 0xFF;
        audio_contexts[1] = (uint8)(pacs_server->data.available_audio_contexts >> 8) & 0xFF;
        audio_contexts[2] = (uint8)(pacs_server->data.available_audio_contexts >> 16) & 0xFF;
        audio_contexts[3] = (uint8)(pacs_server->data.available_audio_contexts >> 24) & 0xFF;
    }

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid) &&
            (pacs_server->data.connected_clients[i].selectiveAudioContexts == 0))
        {
             if (supportedAudioContexts)
             {
                 client_config = GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.supportedAudioContextsClientCfg);
             }
             else
             {
                 client_config = GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.availableAudioContextsClientCfg);
             }

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (pacs_server->gattId),
                        pacs_server->data.connected_clients[i].cid,
                        handle,
                        GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE,
                        audio_contexts
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }

}

void pacsServerNotifyVsPacRecordChange(GPACSS_T *pacs_server,
                                       bool audioSink,
                                       connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint16 handle;
    uint8 *genPacRecord;
    uint8 len;
    bool notify_all = (cid != 0 ? FALSE: TRUE);

    if (audioSink)
    {
        handle = HANDLE_SINK_PAC_VS_APTX;
    }
    else
    {
        handle = HANDLE_SOURCE_PAC_VS_APTX;
    }

    genPacRecord = getGeneratedPacsRecord(&len, handle);

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
        {
            if (audioSink)
            {
                client_config =
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.vsAptXSinkPacClientCfg);
            }
            else
            {
                client_config =
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.vsAptXSourcePacClientCfg);
            }
             
            GATT_PACS_SERVER_DEBUG("PACS2: pacsServerNotifyPacRecordChange 0x%x \n",client_config);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (genPacRecord && client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (pacs_server->gattId),
                        pacs_server->data.connected_clients[i].cid,
                        handle,
                        len,
                        genPacRecord
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }

}

static uint16 getClientConfigbyHandle(GPACSS_T *pacs_server,
    uint16 handle,
    uint8 index)
{
    uint16 client_config= 0;

    switch (handle)
    {
        case HANDLE_SINK_PAC_1:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sinkPacClientCfg1);
             break;
        case HANDLE_SINK_PAC_2:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sinkPacClientCfg2);
             break;
        case HANDLE_SINK_PAC_3:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sinkPacClientCfg3);
             break;

        case HANDLE_SOURCE_PAC_1:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sourcePacClientCfg1);
             break;
        case HANDLE_SOURCE_PAC_2:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sourcePacClientCfg2);
             break;
        case HANDLE_SOURCE_PAC_3:
             client_config =
                 GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[index].client_cfg.sourcePacClientCfg3);
             break;

        default:
             GATT_PACS_SERVER_PANIC("PACS: invalid handle!\n");
    }

    return client_config;
}

static void notifyPacRecordChangeByHandle(GPACSS_T *pacs_server,
    uint16 handle,
    connection_id_t cid)
{
    uint8 i;
    uint8 *genPacRecord;
    uint8 len;
    bool notify_all = (cid != 0 ? FALSE: TRUE);
    uint16 client_config = 0;

    genPacRecord = getGeneratedPacsRecord(&len, handle);
    if (genPacRecord == NULL)
    {
        GATT_PACS_SERVER_DEBUG("PACS: notifyPacRecordChangeByHandle - genPacRecord NULL for handle:%x \n", handle);
        return;
    }

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
        {
            client_config = getClientConfigbyHandle(pacs_server, handle, i);

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (pacs_server->gattId),
                        pacs_server->data.connected_clients[i].cid,
                        handle,
                        len,
                        genPacRecord
                        );
            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }

}

void pacsServerNotifyPacRecordChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid)
{
    uint8 i;
    uint16 handle = 0;

    if (audioSink)
    {
        for ( i = 0; i < NUM_SINK_PAC_RECORD_HANDLES; i++)
        {
            switch (i)
            {
                case 0:
                     handle = HANDLE_SINK_PAC_1;
                     break;
                case 1:
                     handle = HANDLE_SINK_PAC_2;
                     break;
                case 2:
                     handle = HANDLE_SINK_PAC_3;
                     break;

                default:
                     GATT_PACS_SERVER_PANIC("PACS: Invalid handle!\n");
            }

            notifyPacRecordChangeByHandle(pacs_server, handle, cid);
        }
    }
    else
    {
        for ( i = 0; i < NUM_SRC_PAC_RECORD_HANDLES; i++)
        {
            switch (i)
            {
                case 0:
                     handle = HANDLE_SOURCE_PAC_1;
                     break;

                case 1:
                     handle = HANDLE_SOURCE_PAC_2;
                     break;

                case 2:
                     handle = HANDLE_SOURCE_PAC_3;
                     break;

                default:
                     GATT_PACS_SERVER_PANIC("PACS: Invalid handle!\n");
            }

            notifyPacRecordChangeByHandle(pacs_server, handle, cid);
        }
    }
}

