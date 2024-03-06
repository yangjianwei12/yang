/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_notify.h"
#include "gatt_pacs_server_access.h"

#define PACS_AUDIO_LOCATION_NOT_ALLOWED 0x00000000
#define PACS_SERVER_CHAR_CHANGES  1
static uint16 pacsServerCharChanges;

bool GattPacsServerAddAudioLocation(PacsServiceHandleType handle,
                                           PacsServerDirectionType direction,
                                           PacsAudioLocationType audioLocations)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);
    PacsAudioLocationType currentAudioLocation;

    if (pacs_server == NULL)
        return FALSE;

    /* PACS_AUDIO_LOCATION_CLEAR is not a valid location to add.
     * Audio Location 0x00000000 is currently not allowed
     */
    if (audioLocations == PACS_AUDIO_LOCATION_CLEAR ||
        audioLocations == PACS_AUDIO_LOCATION_NOT_ALLOWED)
    {
        return FALSE;
    }

    currentAudioLocation = GattPacsServerGetAudioLocation(handle, direction);

    if ((currentAudioLocation == audioLocations) ||
        (currentAudioLocation == (currentAudioLocation | audioLocations)))
    {
        return TRUE;
    }

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

static bool gattPacsServerRemoveAudioLocation(PacsAudioLocationType *currentAudioLocation,
    PacsAudioLocationType audioLocations)
{
    bool notify = TRUE;

    if(audioLocations == PACS_AUDIO_LOCATION_CLEAR)
    {
        *currentAudioLocation = PACS_AUDIO_LOCATION_NOT_ALLOWED;
    }
    else
    {
        *currentAudioLocation &= ~audioLocations;
    }

    if(*currentAudioLocation == PACS_AUDIO_LOCATION_NOT_ALLOWED)
    {
        notify = FALSE;
    }

    return notify;

}

/******************************************************************************/
bool GattPacsServerRemoveAudioLocation(PacsServiceHandleType handle,
                                            PacsServerDirectionType direction,
                                            PacsAudioLocationType audioLocations)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);
    bool notify;
    PacsAudioLocationType currentAudioLocation;

    if (pacs_server == NULL)
        return FALSE;

    /* Audio Location 0x00000000 is currently not allowed */
    if (audioLocations == PACS_AUDIO_LOCATION_NOT_ALLOWED)
    {
        return FALSE;
    }

    currentAudioLocation = GattPacsServerGetAudioLocation(handle, direction);

    if(audioLocations != PACS_AUDIO_LOCATION_CLEAR  &&
        ((currentAudioLocation & audioLocations) == PACS_AUDIO_LOCATION_NOT_ALLOWED))
    {
        return FALSE;
    }

    if (audioSink)
    {
        notify = gattPacsServerRemoveAudioLocation(
                   &pacs_server->data.sink_audio_source_location,
                   audioLocations);
    }
    else
    {
        notify = gattPacsServerRemoveAudioLocation(
                   &pacs_server->data.source_audio_source_location,
                   audioLocations);
    }

    if(notify)
    {
        pacsServerNotifyAudioLocationChange(pacs_server, audioSink, 0);
    }

    return TRUE;
}

/******************************************************************************/
PacsAudioLocationType GattPacsServerGetAudioLocation(PacsServiceHandleType handle,
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
bool GattPacsServerAddAudioContexts(PacsServiceHandleType handle,
                                       PacsServerDirectionType direction,
                                       uint16 audioContexts,
                                       PacsServerAudioContextType contexts)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    bool supportedAudioContexts = (contexts == PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS? TRUE: FALSE);
    uint16 currentAudioContexts;

    if (pacs_server == NULL)
        return FALSE;

    /* Don't allow Adding selective Audio Conext if Application
     * has Avalable Audio Context control 
     */
    if(!supportedAudioContexts && pacs_server->data.audioContextAvailabiltyControlApp)
    {
        return FALSE;
    }

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
uint16 GattPacsServerGetAudioContexts(PacsServiceHandleType handle,
                          PacsServerDirectionType direction,
                          PacsServerAudioContextType context)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint16 audio_contexts;
    bool supportedAudioContexts =
        (context == PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS? TRUE: FALSE);

    if (pacs_server == NULL)
        return 0;

    /* If Application has Available Audio Context Control then this return 0
     * if this API is called for Available Audio Context
     */
    if(!supportedAudioContexts && pacs_server->data.audioContextAvailabiltyControlApp)
    {
        return 0;
    }

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
uint16 GattPacsServerGetAudioContextsAvailability(PacsServiceHandleType handle,
                                                  PacsServerDirectionType direction)
{
   return GattPacsServerGetAudioContexts(handle,
                                         direction,
                                         PACS_SERVER_AVAILABLE_AUDIO_CONTEXTS);
}

/******************************************************************************/
bool GattPacsServerRemoveAudioContexts(PacsServiceHandleType handle,
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

    /* If Application has Available Audio Context Control then return FALSE
     * if this API is called for Available Audio Context removal
     */
    if(!supportedAudioContexts && pacs_server->data.audioContextAvailabiltyControlApp)
    {
        return FALSE;
    }

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
GattPacsServerConfigType *GattPacsServerRemoveConfig(PacsServiceHandleType handle,
                                                     ConnectionIdType cid)
{
    uint8 i;
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    GattPacsServerConfigType *client_config = NULL;

    if (pacs_server == NULL)
        return client_config;

    for(i=0; i<GATT_PACS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peer device */
        if (pacs_server->data.connected_clients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            client_config = CsrPmemZalloc(sizeof(GattPacsServerConfigType));
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
            break;
        }
    }

    return client_config;
}

/******************************************************************************/
status_t GattPacsServerAddConfig(PacsServiceHandleType handle,
                                             ConnectionIdType cid,
                                             GattPacsServerConfigType *config)
{
    uint8 i;
    status_t res = CSR_BT_GATT_RESULT_CANCELLED;

    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return CSR_BT_GATT_RESULT_INTERNAL_ERROR;

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
                    config->sinkPacClientCfg3 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sinkAudioLocationsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourcePacClientCfg1 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourcePacClientCfg2 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourcePacClientCfg3 == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->sourceAudioLocationsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->availableAudioContextsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->supportedAudioContextsClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->vsAptXSinkPacClientCfg == GATT_PACS_SERVER_CCC_INDICATE ||
                    config->vsAptXSourcePacClientCfg == GATT_PACS_SERVER_CCC_INDICATE)
                {
                    /* PACS characteristics can be only notified as per spec */
                    GATT_PACS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                    res = CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED;
                    break;
                }

                /* Save new ccc for the client */
                pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg1 = config->sinkPacClientCfg1;
                pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg2 = config->sinkPacClientCfg2;
                pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg3 = config->sinkPacClientCfg3;
                pacs_server->data.connected_clients[i].client_cfg.sinkAudioLocationsClientCfg = config->sinkAudioLocationsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg1 = config->sourcePacClientCfg1;
                pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg2 = config->sourcePacClientCfg2;
                pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg3 = config->sourcePacClientCfg3;
                pacs_server->data.connected_clients[i].client_cfg.sourceAudioLocationsClientCfg = config->sourceAudioLocationsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.availableAudioContextsClientCfg = config->availableAudioContextsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.supportedAudioContextsClientCfg = config->supportedAudioContextsClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.vsAptXSinkPacClientCfg = config->vsAptXSinkPacClientCfg;
                pacs_server->data.connected_clients[i].client_cfg.vsAptXSourcePacClientCfg = config->vsAptXSourcePacClientCfg;


                /* If Application has Available Audio Context Control then don't notify anything.
                 * Application will call GattPacsServerSetSelectiveAvailableAudioContexts()
                 * for notifying available audio context to the connected client
                 */
                if(!pacs_server->data.audioContextAvailabiltyControlApp)
                {
                    /* Notify the connected client for values for CCCD which are set and subject to change 
                    * Sink PAC, VS Sink PAC, Source PAC, Source VS PAC, Sink Audio Location,
                    * Source Audio location, Supported Audio Location is fixed so no need to notify.
                    * The client is anyways getting notified if these values change.
                    */
                    pacsServerNotifyAudioContextsChange(pacs_server, FALSE, cid);  /* Available Audio Context */
                }

                if (pacsServerCharChanges == PACS_SERVER_CHAR_CHANGES)
                {
                    /* Notify the connected client for values for CCCD which are set */
                    pacsServerNotifyPacRecordChange(pacs_server, TRUE, cid);       /* Sink PAC Record*/
                    pacsServerNotifyVsPacRecordChange(pacs_server, TRUE, cid); /* Sink VS PAC Record*/
                    pacsServerNotifyPacRecordChange(pacs_server, FALSE, cid);      /* Source PAC Record */
                    pacsServerNotifyVsPacRecordChange(pacs_server, FALSE, cid);    /* Source VS PAC Record */
                    pacsServerNotifyAudioLocationChange(pacs_server, TRUE, cid);   /* Sink Audio Location */
                    pacsServerNotifyAudioLocationChange(pacs_server, FALSE, cid);  /* Source Audio Location */
                    pacsServerNotifyAudioContextsChange(pacs_server, TRUE, cid);   /* Supported Audio Context */
                }
            }

            res = CSR_BT_GATT_RESULT_SUCCESS;
            break;
        }
    }

    if (res)
    {
        res = CSR_BT_GATT_RESULT_INSUFFICIENT_NUM_OF_HANDLES;
    }

    return res;
}

/******************************************************************************/
GattPacsServerConfigType* GattPacsServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid)
{
    uint8 i;
    GPACSS_T *pacs_server = (GPACSS_T*) ServiceHandleGetInstanceData(srvcHndl);
    GattPacsServerConfigType *config;

    if(pacs_server)
    {
        for(i=0; i<GATT_PACS_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peeer device */
            if (pacs_server->data.connected_clients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - return last client configuration
                 */

                config = (GattPacsServerConfigType *)CsrPmemAlloc(sizeof(GattPacsServerConfigType));
                CsrMemCpy(config, &(pacs_server->data.connected_clients[i].client_cfg), sizeof(GattPacsServerConfigType));
                return config;
            }
        }
    }
    return NULL;

}

/************************************************************************/
void GattPacsServerCharChangesDynamically(void)
{
    pacsServerCharChanges = PACS_SERVER_CHAR_CHANGES;
}

/************************************************************************/
bool GattPacsServerIsLtvTypeValuePresent(PacsServiceHandleType handle,
                                        PacsServerDirectionType direction,
                                        PacsCodecIdType codecId,
                                        PacsVendorCodecIdType vendorCodecId,
                                        PacRecordLtvType ltvType,
                                        void *value)
{
    bool isAptx = (vendorCodecId == PACS_RECORD_VENDOR_CODEC_APTX_ADAPTIVE_R3
                                             && codecId == PACS_VENDOR_CODEC_ID);
    bool isSink = (direction == PACS_SERVER_IS_AUDIO_SINK);

    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL ||
          codecId == PACS_CODEC_ID_UNKNOWN ||
              value == NULL)
        return FALSE;

     return isPacRecordWithLtvPresent(getPacRecordList(pacs_server,isAptx ,isSink),
                                   codecId,
                                   vendorCodecId,
                                   ltvType,
                                   value);

}

bool GattPacsServerSetSelectiveAvailableAudioContexts(
                               PacsServiceHandleType handle,
                               ConnectionIdType cid,
                               PacsAudioContextType sinkAudioContexts,
                               PacsAudioContextType sourceAudioContexts)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint16 supportedSinkContexts;
    uint16 supportedSourceContexts;
    uint8 index;

    if (pacs_server == NULL)
        return FALSE;

    supportedSinkContexts = GattPacsServerGetAudioContexts(handle,
                                        PACS_SERVER_IS_AUDIO_SINK,
                             PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS);

    if((supportedSinkContexts & sinkAudioContexts) == PACS_CONTEXT_TYPE_PROHIBITED)
        return FALSE;

    supportedSourceContexts = GattPacsServerGetAudioContexts(handle,
                                        PACS_SERVER_IS_AUDIO_SOURCE,
                               PACS_SERVER_SUPPORTED_AUDIO_CONTEXTS);

    if((supportedSourceContexts & sourceAudioContexts) == PACS_CONTEXT_TYPE_PROHIBITED)
        return FALSE;

    if (!pacsServerGetDeviceIndexFromCid(pacs_server, cid, &index))
        return FALSE;

    /* Clear the previous stored value */
    pacs_server->data.connected_clients[index].selectiveAudioContexts = 0;

    /* Assign the selected available audio contexts for this specific device*/
    pacs_server->data.connected_clients[index].selectiveAudioContexts = sinkAudioContexts;
    pacs_server->data.connected_clients[index].selectiveAudioContexts |= sourceAudioContexts << 16;

    /* Notify the selected cid only */
    pacsServerNotifySelectiveAudioContextsChange(pacs_server, cid);

    return TRUE;

}

bool GattPacsServerClearSelectiveAvailableAudioContexts(
                               PacsServiceHandleType handle,
                               ConnectionIdType cid)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint8 index;

    if (pacs_server == NULL)
        return FALSE;

    if (!pacsServerGetDeviceIndexFromCid(pacs_server, cid, &index))
        return FALSE;

    /* Don't allow clearing selective Audio Conext if Application
     * has Avalable Audio Context control 
     */
    if(pacs_server->data.audioContextAvailabiltyControlApp)
    {
        return FALSE;
    }

    if (pacs_server->data.connected_clients[index].selectiveAudioContexts != 0)
    {
       pacs_server->data.connected_clients[index].selectiveAudioContexts = 0;

       /* Notify the selected cid only with default available audio contexts */
       pacsServerNotifyAudioContextsChange(pacs_server, FALSE, cid);
    }

    return TRUE;
}

void GattPacsServerEnableAvailableAudioContextControl(PacsServiceHandleType handle)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return;

    /* Higher layer wants to maintain Available Audio Context Control */
    pacs_server->data.audioContextAvailabiltyControlApp = TRUE;
}

bool GattPacsServerAvailableAudioContextReadResponse(
                              PacsServiceHandleType handle,
                              ConnectionIdType cid,
                              PacsAudioContextType sinkAudioContexts,
                              PacsAudioContextType sourceAudioContexts)
{
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);
    uint8 audio_contexts_resp[GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE];
    uint8 index;

    if (pacs_server == NULL)
        return FALSE;

    if (!pacsServerGetDeviceIndexFromCid(pacs_server, cid, &index))
        return FALSE;

    /* Clear the previous stored value */
    pacs_server->data.connected_clients[index].selectiveAudioContexts = 0;

    /* Assign the selected available audio contexts for this specific device*/
    pacs_server->data.connected_clients[index].selectiveAudioContexts = sinkAudioContexts;
    pacs_server->data.connected_clients[index].selectiveAudioContexts |= sourceAudioContexts << 16;

    audio_contexts_resp[0] = (uint8)pacs_server->data.connected_clients[index].selectiveAudioContexts & 0xFF;
    audio_contexts_resp[1] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 8) & 0xFF;
    audio_contexts_resp[2] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 16) & 0xFF;
    audio_contexts_resp[3] = (uint8)(pacs_server->data.connected_clients[index].selectiveAudioContexts >> 24) & 0xFF;

    sendPacsServerAccessRsp(
            pacs_server->gattId,
            cid,
            HANDLE_AVAILABLE_AUDIO_CONTEXTS,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE,
            audio_contexts_resp
            );

    return TRUE;
}


