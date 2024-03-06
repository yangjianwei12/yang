/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_notify.h"

#define PACS_SERVER_CHAR_CHANGES  1
static uint16 pacsServerCharChanges;

bool GattPacsServerAddAudioLocation(ServiceHandleType handle,
                                           PacsServerDirectionType direction,
                                           PacsAudioLocationType audioLocations)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);

    if (pacs_server == NULL)
        return FALSE;

    if (audioSink)
    {
        pacs_server->data.sink_audio_source_location |= audioLocations;
    }
    else
    {
        pacs_server->data.source_audio_source_location |= audioLocations;
    }

    pacsServerNotifyAudioLocationChange(pacs_server, audioSink, 0);

    return TRUE;
}

/******************************************************************************/
bool GattPacsServerRemoveAudioLocation(ServiceHandleType handle,
                                            PacsServerDirectionType direction,
                                            PacsAudioLocationType audioLocations)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);

    if (pacs_server == NULL)
        return FALSE;

    if (audioSink)
    {
        pacs_server->data.sink_audio_source_location &= ~audioLocations;
    }
    else
    {
        pacs_server->data.source_audio_source_location &= ~audioLocations;
    }

    pacsServerNotifyAudioLocationChange(pacs_server, audioSink, 0);

    return TRUE;
}

/******************************************************************************/
PacsAudioLocationType GattPacsServerGetAudioLocation(ServiceHandleType handle,
                                         PacsServerDirectionType direction)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint32 audio_locations;

    if (pacs_server == NULL)
        return 0;

    if (direction == PACS_SERVER_IS_AUDIO_SINK)
    {
        audio_locations = pacs_server->data.sink_audio_source_location;
    }
    else
    {
        audio_locations = pacs_server->data.source_audio_source_location;
    }

    return audio_locations;
}

/******************************************************************************/
bool GattPacsServerAddAudioContexts(ServiceHandleType handle,
                                       PacsServerDirectionType direction,
                                       uint16 audioContexts,
                                       PacsServerAudioContextType contexts)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool supportedAudioContexts = (contexts == PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS? TRUE: FALSE);
    uint16 currentAudioContexts;

    if (pacs_server == NULL)
        return FALSE;

    currentAudioContexts = GattPacsServerGetAudioContexts(handle, direction, contexts);

    if ((currentAudioContexts == audioContexts) ||
        (currentAudioContexts == (currentAudioContexts | audioContexts)))
        return TRUE;

    if (supportedAudioContexts)
    {
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            pacs_server->data.supported_audio_contexts |= audioContexts;
        }
        else
        {
            pacs_server->data.supported_audio_contexts |= audioContexts << 16;
        }
    }
    else
    {
        uint16 supportedContexts = GattPacsServerGetAudioContexts(handle,
                                       direction,
                                       PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS);

        uint16 validAudioContexts = supportedContexts & audioContexts;

        if (currentAudioContexts == (currentAudioContexts | validAudioContexts))
            return FALSE;

        /* Identify if the available contexts requested is already
         * supported or not.
         * Only valid available ones shall be updated
         */
        if(validAudioContexts == PACS_CONTEXT_TYPE_PROHIBITED)
            return FALSE;

        /* Assign the valid one to be updated in available audio contexts */
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            pacs_server->data.available_audio_contexts |= validAudioContexts;
        }
        else
        {
           pacs_server->data.available_audio_contexts |= validAudioContexts << 16;
        }
    }

    pacsServerNotifyAudioContextsChange(pacs_server, supportedAudioContexts, 0);

    return TRUE;
}

/******************************************************************************/
uint16 GattPacsServerGetAudioContexts(ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          PacsServerAudioContextType context)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint16 audio_contexts;
    bool supportedAudioContexts =
        (context == PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS? TRUE: FALSE);

    if (pacs_server == NULL)
        return 0;

    if (supportedAudioContexts)
    {
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            audio_contexts = (pacs_server->data.supported_audio_contexts & 0x0000FFFF);
        }
        else
        {
            audio_contexts = ((pacs_server->data.supported_audio_contexts >> 16 ) & 0x0000FFFF);
        }
    }
    else
    {
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            audio_contexts = (pacs_server->data.available_audio_contexts & 0x0000FFFF);
        }
        else
        {
            audio_contexts = ((pacs_server->data.available_audio_contexts >> 16 ) & 0x0000FFFF);
        }
    }

    return audio_contexts;
}


/******************************************************************************/
uint16 GattPacsServerGetAudioContextsAvailability(ServiceHandleType handle,
                                                  PacsServerDirectionType direction)
{
   return GattPacsServerGetAudioContexts(handle,
                                         direction,
                                         PACS_SERVER_AVAILABLE_AUDIO_CONTEXTS);
}

/******************************************************************************/
bool GattPacsServerRemoveAudioContexts(ServiceHandleType handle,
                          PacsServerDirectionType direction,
                          uint16 audioContexts,
                          PacsServerAudioContextType context)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint16 currentAudioContexts;
    bool notifyChangeInAvailableContext = FALSE;
    bool supportedAudioContexts =
        (context == PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS? TRUE: FALSE);

    if (pacs_server == NULL)
        return FALSE;

    /* a) Identify if remove one asked is already part of list or not 
     * b) If entry is removed from Supported then we may have to remove from
     *    available also. In that case we may have to notify for both
     */
    currentAudioContexts = GattPacsServerGetAudioContexts(handle, direction, context);

    if ((currentAudioContexts & audioContexts)== PACS_CONTEXT_TYPE_PROHIBITED)
        return FALSE;

    if (supportedAudioContexts)
    {
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            pacs_server->data.supported_audio_contexts &= ~audioContexts;

            /* Hide the context from available too if enabled */
            if ((pacs_server->data.available_audio_contexts & audioContexts)
                   != PACS_CONTEXT_TYPE_PROHIBITED)
            {
                pacs_server->data.available_audio_contexts &= ~audioContexts;
                notifyChangeInAvailableContext = TRUE;
            }
        }
        else
        {
            pacs_server->data.supported_audio_contexts &= ~(audioContexts << 16);

            /* Hide the context from available too if enabled */
            if ((pacs_server->data.available_audio_contexts & (audioContexts << 16))
                   != PACS_CONTEXT_TYPE_PROHIBITED)
            {
                pacs_server->data.available_audio_contexts &= ~(audioContexts << 16);
                notifyChangeInAvailableContext = TRUE;
            }
        }
    }
    else
    {
        if (direction == PACS_SERVER_IS_AUDIO_SINK)
        {
            pacs_server->data.available_audio_contexts &= ~audioContexts;
        }
        else
        {
            pacs_server->data.available_audio_contexts &= ~(audioContexts << 16);
        }
    }

    pacsServerNotifyAudioContextsChange(pacs_server, supportedAudioContexts, 0);

    if (notifyChangeInAvailableContext)
        pacsServerNotifyAudioContextsChange(pacs_server, FALSE, 0);

    return TRUE;
}

/******************************************************************************/
GattPacsServerConfigType *GattPacsServerRemoveConfig(ServiceHandleType handle,
                                                     ConnectionIdType cid)
{
    uint8 i;
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    GattPacsServerConfigType *client_config = NULL;

    if (pacs_server == NULL)
        return client_config;

    for(i=0; i<GATT_PACS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (pacs_server->data.connected_clients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            client_config = PanicUnlessMalloc(sizeof(GattPacsServerConfigType));
            memcpy(client_config, &(pacs_server->data.connected_clients[i].client_cfg), sizeof(GattPacsServerConfigType));
            
            if ((i == (GATT_PACS_MAX_CONNECTIONS-1)) || (i == 0 && pacs_server->data.connected_clients[i+1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(pacs_server->data.connected_clients[i]), 0, sizeof(pacs_client_data));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;
            
                for (j=i; j< (GATT_PACS_MAX_CONNECTIONS -1) && pacs_server->data.connected_clients[j+1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(pacs_server->data.connected_clients[j]),
                           &(pacs_server->data.connected_clients[j+1]),
                           sizeof(pacs_client_data));
                }
            
                /* Remove the last element of the array, already shifted behind */
                memset(&(pacs_server->data.connected_clients[j]), 0, sizeof(pacs_client_data));
            }
        }
    }

    return client_config;
}

/******************************************************************************/
gatt_status_t GattPacsServerAddConfig(ServiceHandleType handle,
                                             ConnectionIdType cid,
                                             GattPacsServerConfigType *config)
{
    uint8 i;
    gatt_status_t res = gatt_status_failure;

    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return gatt_status_invalid_phandle;

    for(i=0; i<GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if(pacs_server->data.connected_clients[i].cid == 0)
        {
            pacs_server->data.connected_clients[i].cid = cid;

            /* Check config parameter:
             * If config is NULL, it indicates a default config should be used for the
             * peer device identified by the CID.
             * The default config is already set when the instance has been initialised.
             */
            if (config)
            {
                if (config->sinkPacClientCfg1 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sinkPacClientCfg2 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->vsSinkPacClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sinkAudioLocationsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourcePacClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->vsSourcePacClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourceAudioLocationsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->availableAudioContextsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->supportedAudioContextsClientCfg == GATT_PACS_SERVER_CCC_INDICATE)
                {
                    /* PACS characteristics can be only notified as per spec */
                    GATT_PACS_SERVER_DEBUG_INFO(("Invalid Client Configuration Characteristic!\n"));
                    res = gatt_status_value_not_allowed;
                    break;
                }

                /* Save new ccc for the client */
                pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg1 = config->sinkPacClientCfg1;
                pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg2 = config->sinkPacClientCfg2;
                pacs_server->data.connected_clients[i].client_cfg.vsSinkPacClientCfg = config->vsSinkPacClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.sinkAudioLocationsClientCfg = config->sinkAudioLocationsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg = config->sourcePacClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.vsSourcePacClientCfg = config->vsSourcePacClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.sourceAudioLocationsClientCfg = config->sourceAudioLocationsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.availableAudioContextsClientCfg = config->availableAudioContextsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.supportedAudioContextsClientCfg = config->supportedAudioContextsClientCfg;

                /* Notify the connected client for values for CCCD which are set and subject to change 
                * Sink PAC, VS Sink PAC, Source PAC, Source VS PAC, Sink Audio Location,
                * Source Audio location, Supported Audio Location is fixed so no need to notify.
                * The client is anyways getting notified if these values change.
                */
                pacsServerNotifyAudioContextsChange(pacs_server, FALSE, cid);   /* Available Audio Location */

                if (pacsServerCharChanges == PACS_SERVER_CHAR_CHANGES)
                {
                    /* Notify the connected client for values for CCCD which are set */
                    if(pacs_server->data.sink_pack_record)
                        pacsServerNotifyPacRecordChange(pacs_server, TRUE, cid);       /* Sink PAC Record*/
                    if(pacs_server->data.vs_sink_pack_record)
                        pacsServerNotifyVsPacRecordChange(pacs_server, TRUE, cid);     /* Sink VS PAC Record*/
                    if(pacs_server->data.source_pack_record)
                        pacsServerNotifyPacRecordChange(pacs_server, FALSE, cid);      /* Source PAC Record */
                    if(pacs_server->data.vs_source_pack_record)
                        pacsServerNotifyVsPacRecordChange(pacs_server, FALSE, cid);    /* Source VS PAC Record */

                    pacsServerNotifyAudioLocationChange(pacs_server, TRUE, cid);   /* Sink Audio Location */
                    pacsServerNotifyAudioLocationChange(pacs_server, FALSE, cid);  /* Source Audio Location */
                    pacsServerNotifyAudioContextsChange(pacs_server, TRUE, cid);   /* Supported Audio Location */
                }
            }

            res = gatt_status_success;
            break;
        }
    }

    if (res)
    {
        res = gatt_status_insufficient_resources;
    }

    return res;
}

/************************************************************************/
void GattPacsServerCharChangesDynamically(void)
{
    pacsServerCharChanges = PACS_SERVER_CHAR_CHANGES;
}
