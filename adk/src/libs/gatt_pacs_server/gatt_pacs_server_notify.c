/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_notify.h"
#include "gatt_pacs_server_pac_record.h"

/***************************************************************************/
static void pacsServerSendCharacteristicChangedNotification(
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        )
{
    if (task == NULL)
    {
        GATT_PACS_SERVER_DEBUG_PANIC((
                    "PACS: Null instance!\n"
                    ));
    }
    else if ( cid == 0 )
    {
        GATT_PACS_SERVER_DEBUG_PANIC((
                    "PACS: No Cid!\n"
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
                        (Task) &(pacs_server->lib_task),
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
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
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
                        (Task) &(pacs_server->lib_task),
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

void pacsServerNotifyVsPacRecordChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid)
{
    uint8 i;
    uint16 client_config;
    uint16 handle;
    uint8 *genPacRecord;
    uint8 len;
    bool notify_all = (cid != 0 ? FALSE: TRUE);

    if (audioSink)
    {
        handle = HANDLE_SINK_PAC_VS;
    }
    else
    {
        handle = HANDLE_SOURCE_PAC_VS;
    }

    genPacRecord = getGeneratedPacsRecord(audioSink, &len, handle);

    if(genPacRecord == NULL)
    {
        GATT_PACS_SERVER_DEBUG_PANIC((
                    "PACS: NULL instance!\n"
                    ));
        return;
    }

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
        {
             if (audioSink)
             {
                 client_config = 
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.vsSinkPacClientCfg);
             }
             else
             {
                 client_config = 
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.vsSourcePacClientCfg);
             }

            GATT_PACS_SERVER_DEBUG_INFO(("PACS2: pacsServerNotifyPacRecordChange 0x%x \n",client_config));

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (Task) &(pacs_server->lib_task),
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
    uint16 client_config;
    uint16 handle;
    uint16 handle1 = 0;
    uint16 client_config1 = 0;
    uint8 *genPacRecord1 = NULL;
    uint8 len1 = 0;
    uint8 *genPacRecord;
    uint8 len;
    bool notify_all = (cid != 0 ? FALSE: TRUE);

    if (audioSink)
    {
        handle = HANDLE_SINK_PAC_1;
        handle1 = HANDLE_SINK_PAC_2;
        genPacRecord1 = getGeneratedPacsRecord(audioSink, &len1, handle1);
    }
    else
    {
        handle = HANDLE_SOURCE_PAC;
    }

    genPacRecord = getGeneratedPacsRecord(audioSink, &len, handle);

    for (i=0; i< GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacs_server->data.connected_clients[i].cid != 0 &&
            (notify_all || cid == pacs_server->data.connected_clients[i].cid))
        {
             if (audioSink)
             {
                 client_config = 
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg1);

                 client_config1 = 
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg2);

             }
             else
             {
                 client_config = 
                    GET_PACS_CLIENT_CONFIG(pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg);
             }

            GATT_PACS_SERVER_DEBUG_INFO(("PACS2: pacsServerNotifyPacRecordChange 0x%x \n",client_config));

            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (client_config == GATT_PACS_SERVER_CCC_NOTIFY)
            {
                pacsServerSendCharacteristicChangedNotification(
                        (Task) &(pacs_server->lib_task),
                        pacs_server->data.connected_clients[i].cid,
                        handle,
                        len,
                        genPacRecord
                        );

                if (audioSink && client_config1 == GATT_PACS_SERVER_CCC_NOTIFY)
                {
                    pacsServerSendCharacteristicChangedNotification(
                            (Task) &(pacs_server->lib_task),
                            pacs_server->data.connected_clients[i].cid,
                            handle1,
                            len1,
                            genPacRecord1
                            );
                }

            }
            else
            {
                /* handle not configured for Notification - Nothing to do */
            }
        }
    }
}



