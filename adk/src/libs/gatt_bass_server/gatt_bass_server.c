/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_bass_server_msg_handler.h"
#include "gatt_bass_server.h"
#include "gatt_bass_server_private.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_common.h"

/****************************************************************************/
ServiceHandle    GattBassServerInit(const Task theAppTask,
                                    uint16 startHandle,
                                    uint16 endHandle,
                                    uint8 broadcastReceiveStateNum)
{
    GBASSSS *bass_inst = NULL;
    gatt_manager_server_registration_params_t reg_params;
    ServiceHandle srvc_hndl = 0;
    uint8 i, j;

    if (theAppTask == NULL)
    {
        GATT_BASS_SERVER_PANIC(("Application Task NULL\n"));
    }

    srvc_hndl = ServiceHandleNewInstance((void **) &bass_inst, sizeof(GBASSSS));

    if (bass_inst)
    {
        /* Reset all the service library memory */
        memset(bass_inst, 0, sizeof(GBASSSS));

        /* Set up library handler for external messages */
        bass_inst->lib_task.handler = bassServerMsgHandler;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        bass_inst->app_task = theAppTask;

        /* Save the service handle */
        bass_inst->srvc_hndl = srvc_hndl;

        bass_inst->data.broadcast_receive_state_num = broadcastReceiveStateNum;

        bass_inst->data.broadcast_source = PanicUnlessMalloc(sizeof(gatt_bass_broadcast_source_info_t *) * broadcastReceiveStateNum);
        memset(bass_inst->data.broadcast_source, 0, (sizeof(gatt_bass_broadcast_source_info_t *) * broadcastReceiveStateNum));

        for(i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
        {
            bass_inst->data.connected_clients[i].client_cfg.receiveStateCccSize = bass_inst->data.broadcast_receive_state_num;
            bass_inst->data.connected_clients[i].client_cfg.receiveStateCcc = PanicUnlessMalloc(sizeof(uint16) * bass_inst->data.connected_clients[i].client_cfg.receiveStateCccSize);

            for(j=0; j<bass_inst->data.broadcast_receive_state_num; j++)
            {
                bass_inst->data.connected_clients[i].client_cfg.receiveStateCcc[j] = 0;
            }
        }

        /* Setup data required for Broadcast Audio Scan Service to be registered with the GATT Manager */
        reg_params.start_handle = startHandle;
        reg_params.end_handle = endHandle;
        reg_params.task = &bass_inst->lib_task;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterServer(&reg_params) == gatt_manager_status_success)
        {
            return srvc_hndl;
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));
            /* If the registration with GATT Manager fails and we have allocated memory
             * for the new instance successfully (service handle not zero), we have to free
             * the memory of that instance.
             */
            if (srvc_hndl)
            {
                ServiceHandleFreeInstanceData(srvc_hndl);
            }
            return 0;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("Memory alllocation of BASS Server instance failed!\n"));
        return 0;
    }
}

/****************************************************************************/
static bool bassServerIsValidCcc(const GattBassServerConfig *config)
{
    uint8 i;

    for (i=0; i<(config->receiveStateCccSize); i++)
    {
        if (config->receiveStateCcc[i] == GATT_BASS_CLIENT_CONFIG_INDICATE)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************/
gatt_status_t GattBassServerAddConfig(ServiceHandle srvcHndl,
                                      connection_id_t cid,
                                      const GattBassServerConfig *config)
{
    uint8 i, j;
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server)
    {
        for(i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
        {
            if(bass_server->data.connected_clients[i].cid == 0)
            {
                /* Check config parameter:
                 * If config is NULL, it indicates a default config should be used for the
                 * peer device identified by the CID.
                 * The default config is already set when the instance has been initialised.
                 */
                if (config)
                {
                    if(!bassServerIsValidCcc(config))
                    {
                        /* Broadcast Receive State characteristics can be only notified */
                        GATT_BASS_SERVER_DEBUG_INFO(("Invalid Client Configuration Characteristic!\n"));
                        return gatt_status_value_not_allowed;
                    }

                    for(j=0; j<bass_server->data.broadcast_receive_state_num; j++)
                    {
                        /* Save new ccc for the client */
                        bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[j] = config->receiveStateCcc[j];

                        if (config->receiveStateCcc[j] == GATT_BASS_CLIENT_CONFIG_NOTIFY)
                        {
                            /* If ccc is NOTIFY, we have to notify the Broadcast Receive State characteristic value */
                            if(bass_server->data.broadcast_source[j])
                            {
                                uint16 len = bassServerCalculateBroadcastReceiveStateCharacteristicLen(bass_server->data.broadcast_source[j]);
                                uint8 *value = PanicUnlessMalloc(sizeof(uint8) * len);

                                bassConvertBroadcastReceiveStateValue(bass_server, value, j);

                                bassServerSendCharacteristicChangedNotification(
                                            (Task) &bass_server->lib_task,
                                            cid,
                                            bassServerGetHandleBroadcastReceiveStateCharacteristic(j),
                                            len,
                                            value);

                                free(value);
                            }
                        }
                    }
                }

                bass_server->data.connected_clients[i].cid = cid;

                return gatt_status_success;
            }
        }
        return gatt_status_insufficient_resources;
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS Server instance is NULL!\n"));
        return gatt_status_failure;
    }
}

/******************************************************************************/
GattBassServerConfig* GattBassServerRemoveConfig(ServiceHandle srvcHndl,
                                                 connection_id_t  cid)
{
    uint8 i, j, k;
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server)
    {
        for(i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peer device */
            if (bass_server->data.connected_clients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - remove the peer device
                 * - free the server instance
                 * - return last client configuration
                 */
                GattBassServerConfig *config = PanicUnlessMalloc(sizeof(GattBassServerConfig));

                config->receiveStateCccSize = bass_server->data.broadcast_receive_state_num;

                config->receiveStateCcc = PanicUnlessMalloc(sizeof(uint16) * config->receiveStateCccSize);
                for(j=0; j<config->receiveStateCccSize; j++)
                {
                    config->receiveStateCcc[j] = bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[j];
                }

                if ((i == (BASS_SERVER_MAX_CONNECTIONS-1)) || (i == 0 && bass_server->data.connected_clients[i+1].cid == 0))
                {
                    /* The peer device is the only or the last element of the array */
                    bass_server->data.connected_clients[i].cid = 0;

                    for(j=0; j<bass_server->data.connected_clients[i].client_cfg.receiveStateCccSize; j++)
                    {
                        bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[j] = 0;
                    }
                }
                else
                {
                    /* The peer device is in the middle of the array */
                    for (j=i; j<(BASS_SERVER_MAX_CONNECTIONS - 1) && bass_server->data.connected_clients[j+1].cid != 0; j++)
                    {
                        /* Shift all the elements of the array of one position behind */
                        bass_server->data.connected_clients[j].cid = bass_server->data.connected_clients[j+1].cid;

                        for(k=0; k<bass_server->data.connected_clients[j].client_cfg.receiveStateCccSize; k++)
                        {
                            bass_server->data.connected_clients[j].client_cfg.receiveStateCcc[k] = bass_server->data.connected_clients[j+1].client_cfg.receiveStateCcc[k];
                        }
                    }

                    /* Remove the last element of the array, already shifted behind */
                    bass_server->data.connected_clients[j].cid = 0;

                    for(k=0; k<bass_server->data.connected_clients[j].client_cfg.receiveStateCccSize; k++)
                    {
                        bass_server->data.connected_clients[j].client_cfg.receiveStateCcc[k] = 0;
                    }
                }

                return config;
            }
        }

        GATT_BASS_SERVER_DEBUG_INFO(("Maximum number of clients connected!\n"));
        return NULL;
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS Server instance is NULL!\n"));
        return NULL;
    }
}

/******************************************************************************/
bool GattBassServerIsAnyClientConnected(ServiceHandle srvcHndl)
{
    uint8 i;
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server)
    {
        for (i = 0; i < BASS_SERVER_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peer device */
            if (bass_server->data.connected_clients[i].cid != 0)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/******************************************************************************/
uint8 * GattBassServerGetSourceIdsRequest(ServiceHandle srvcHndl,
                                          uint16 *sourceIdNum)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    (*sourceIdNum) = 0;

    if (bass_server && bass_server->data.broadcast_source)
    {
        uint8 *source_id = PanicUnlessMalloc(sizeof(uint8) * bass_server->data.broadcast_receive_state_num);
        uint8 i = 0;

        (*sourceIdNum) = bass_server->data.broadcast_receive_state_num;

        for (i=0; i<bass_server->data.broadcast_receive_state_num; i++)
        {
            if(bass_server->data.broadcast_source[i])
                source_id[i] = bass_server->data.broadcast_source[i]->source_id;
            else
                source_id[i] = 0;
        }
        return source_id;
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameters!\n"));
        return NULL;
    }
}

/******************************************************************************/
GattBassServerStatus GattBassServerGetBroadcastReceiveStateRequest(ServiceHandle srvcHndl,
                                                                   uint8 sourceId,
                                                                   GattBassServerReceiveState* state)
{
    uint8 index;
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server && bassServerCalcNumBroadcastReceiveStateCharacteristicsNotEmpty(bass_server))
    {
        if(bassFindBroadcastSource(bass_server,
                                   sourceId,
                                   &index))
        {
            state->bigEncryption = bass_server->data.broadcast_source[index]->broadcast_source_state.bigEncryption;
            state->paSyncState = bass_server->data.broadcast_source[index]->broadcast_source_state.paSyncState;
            state->sourceAdvSid = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAdvSid;
            state->sourceAddress = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress;
            state->broadcastId = bass_server->data.broadcast_source[index]->broadcast_source_state.broadcastId;

            state->sourceAddress.type = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type;

            state->numSubGroups = bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups;

            if(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode)
            {
                state->badCode = (uint8 *) PanicUnlessMalloc(sizeof(uint8) * GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
                memmove(state->badCode,
                        bass_server->data.broadcast_source[index]->broadcast_source_state.badCode,
                        GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
            }
            else
            {
                state->badCode = NULL;
            }

            if(bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups)
            {
                uint8 i;

                state->subGroupsData = (GattBassServerSubGroupsData *) PanicUnlessMalloc(sizeof(GattBassServerSubGroupsData) *
                                bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups);

                for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
                {
                    state->subGroupsData[i].bisSync = bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync;
                    state->subGroupsData[i].metadataLen = bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen;

                    if(state->subGroupsData[i].metadataLen)
                    {
                        state->subGroupsData[i].metadata = (uint8 *) PanicUnlessMalloc(sizeof(uint8) * state->subGroupsData[i].metadataLen);
                        memmove(state->subGroupsData[i].metadata,
                                bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata,
                                state->subGroupsData[i].metadataLen);
                    }
                    else
                    {
                        state->subGroupsData[i].metadata = NULL;
                    }
                }
            }
            return GATT_BASS_SERVER_STATUS_SUCCESS;
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Error - GattBassServerGetBroadcastReceiveStateRequest: invalid Source ID\n"));
            return GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS Server instance is NULL!\n"));
        return GATT_BASS_SERVER_STATUS_INVALID_PARAMETER;
    }
}

static void bassServerSetReceiveState(GBASSSS *bass_server,
                                      GattBassServerReceiveState *sourceInfo,
                                      uint8 index)
{
    bass_server->data.broadcast_source[index]->broadcast_source_state.bigEncryption = sourceInfo->bigEncryption;
    bass_server->data.broadcast_source[index]->broadcast_source_state.paSyncState = sourceInfo->paSyncState;
    bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAdvSid = sourceInfo->sourceAdvSid;
    bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type = sourceInfo->sourceAddress.type;
    bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr = sourceInfo->sourceAddress.addr;
    bass_server->data.broadcast_source[index]->broadcast_source_state.broadcastId = sourceInfo->broadcastId;

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode)
        free(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode);

    if(sourceInfo->badCode)
    {
        bass_server->data.broadcast_source[index]->broadcast_source_state.badCode = (uint8 *) PanicUnlessMalloc(sizeof(uint8) * GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
        memmove(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode,
                sourceInfo->badCode,
                GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
    }
    else
    {
        bass_server->data.broadcast_source[index]->broadcast_source_state.badCode = NULL;
    }

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData)
    {
        uint8 i;

        for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
        {
            if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata)
                free(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata);
        }

        free(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData);
    }

    bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups = sourceInfo->numSubGroups;

    if(sourceInfo->numSubGroups)
    {
        uint8 i;

        bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData = (GattBassServerSubGroupsData *)
                PanicUnlessMalloc(sizeof(GattBassServerSubGroupsData) * sourceInfo->numSubGroups);

        for(i=0; i<sourceInfo->numSubGroups; i++)
        {
            bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync =
                    sourceInfo->subGroupsData[i].bisSync;

            bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen =
                    sourceInfo->subGroupsData[i].metadataLen;

            if(sourceInfo->subGroupsData[i].metadataLen)
            {
                bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata =
                        (uint8 *) PanicUnlessMalloc(sizeof(uint8) * sourceInfo->subGroupsData[i].metadataLen);
                memmove(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata,
                        sourceInfo->subGroupsData[i].metadata,
                        sizeof(uint8) * sourceInfo->subGroupsData[i].metadataLen);
            }
            else
            {
                bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata = NULL;
            }

        }
    }
    else
    {
        bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData = NULL;
    }
}

/******************************************************************************/
static bool bassServerIsBadCodeChanged(GBASSSS *bass_server,
                                       uint8 *badCode,
                                       uint8 index)
{
    if((badCode && !bass_server->data.broadcast_source[index]->broadcast_source_state.badCode) ||
       ((!badCode) && bass_server->data.broadcast_source[index]->broadcast_source_state.badCode))
        return TRUE;

    if(badCode)
    {
       return memcmp(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode,
                     badCode,
                     GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
    }

    return FALSE;
}

/******************************************************************************/
static bool bassServerIsBisSyncChanged(GBASSSS *bass_server,
                                       GattBassServerSubGroupsData *subGroupsData,
                                       uint8 index)
{
    uint8 i;

    for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
    {
        if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync !=
                subGroupsData[i].bisSync)
            return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
static bool bassServerIsMetadataChanged(GBASSSS *bass_server,
                                        GattBassServerSubGroupsData *subGroupsData,
                                        uint8 index)
{
    uint8 i;

    for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
    {
        if(subGroupsData[i].metadataLen !=
                bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen)
            return TRUE;

        if(subGroupsData[i].metadataLen)
        {
            if(memcmp(subGroupsData[i].metadata,
                      bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata,
                      subGroupsData[i].metadataLen));
                return TRUE;
        }
    }

    return FALSE;
}

/******************************************************************************/
static bool bassServerIsBroadcastSourceChanged(GBASSSS *bass_server,
                                               uint8 index,
                                               GattBassServerReceiveState *source_info)

{
    if((source_info->bigEncryption != bass_server->data.broadcast_source[index]->broadcast_source_state.bigEncryption) ||
       (source_info->paSyncState  != bass_server->data.broadcast_source[index]->broadcast_source_state.paSyncState)  ||
       (source_info->sourceAdvSid != bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAdvSid) ||
       (source_info->sourceAddress.type != bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type) ||
       (!BdaddrIsSame(&(source_info->sourceAddress.addr), &(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr))) ||
       bassServerIsBadCodeChanged(bass_server, source_info->badCode, index) ||
       bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups != source_info->numSubGroups ||
       bassServerIsBisSyncChanged(bass_server, source_info->subGroupsData, index) ||
       bassServerIsMetadataChanged(bass_server, source_info->subGroupsData, index))
    {
        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
GattBassServerStatus  GattBassServerAddBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                              uint8 *sourceId,
                                                              GattBassServerReceiveState *sourceInfo)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);
    GattBassServerStatus status = bassServerCheckBroadcastSourceInfo(sourceInfo);
    bool isChanged = FALSE;

    if (bass_server && !status)
    {
        uint8 index = 0;

        if(!(*sourceId) && bassServerIsAnyBroadcastReceiveStateCharacteristicsEmpty(bass_server, &index))
        {
            status = gatt_status_success;
            isChanged = TRUE;
            (*sourceId) = index + 1;
        }
        else if(*sourceId)
        {
            if(bassFindBroadcastSource(bass_server,
                                       (*sourceId),
                                       &index))
            {
                if(bassServerIsBroadcastSourceChanged(bass_server,
                                                      index,
                                                      sourceInfo))
                {
                    isChanged = TRUE;

                    if(!bassIsBroadcastReceiveStateFree(bass_server, index))
                    {
                        if(GattBassServerRemoveBroadcastSourceRequest(srvcHndl, (*sourceId)))
                        {
                            status = GATT_BASS_SERVER_STATUS_FAILED;
                            return status;
                        }
                    }
                }
                status = gatt_status_success;
            }
            else
            {
                GATT_BASS_SERVER_DEBUG_INFO(("Invalid source_id\n"));
                status = GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID;
            }
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("No empty BRS, a source_id to be provided\n"));
            status = GATT_BASS_SERVER_STATUS_NO_EMPTY_BRS;
        }

        if (status == gatt_status_success && isChanged)
        {
            bass_server->data.broadcast_source[index] = (gatt_bass_broadcast_source_info_t *) PanicUnlessMalloc(sizeof(gatt_bass_broadcast_source_info_t));
            memset(bass_server->data.broadcast_source[index], 0, sizeof(gatt_bass_broadcast_source_info_t));

            bass_server->data.broadcast_source[index]->source_id = (*sourceId);

            bassServerSetReceiveState(bass_server, sourceInfo, index);

            /* Notify the new value of a Broadcast Receive Characteristic to al the clients have been registered
             * for notifications. */
            bassServerNotifyBroadcastReceiveStateCharacteristic(bass_server, index);
        }
    }
    else if(!bass_server)
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS Server instance is NULL!\n"));
        status = GATT_BASS_SERVER_STATUS_FAILED;
    }

    return status;
}

/******************************************************************************/
GattBassServerStatus GattBassServerRemoveBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                                uint8 sourceId)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server && bass_server->data.broadcast_source)
    {
        uint8 index;

        if(bassFindBroadcastSource(bass_server,
                                   sourceId,
                                   &index))
        {
            /* Check if we are in sync to the PA and/or to the BIS/BIG of the
             * Broadcast Source to remove*/
            if(!bassServerIsSynchronized(bass_server, index))
            {
                uint8 i;

                if(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode)
                    free(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode);

                if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData)
                {
                    for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
                    {
                        if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata)
                            free(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata);
                    }

                    free(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData);
                }

                free(bass_server->data.broadcast_source[index]);
                bass_server->data.broadcast_source[index] = NULL;

                /* Notify the new value of the Broadacast Receive State Characteristic to all the client have registered
                 * for notifications.*/
                bassServerNotifyBroadcastReceiveStateCharacteristic(bass_server, index);

                return gatt_status_success;
            }
            else
            {
                GATT_BASS_SERVER_DEBUG_INFO(("In Sync with the Broadcast Source\n"));
                return GATT_BASS_SERVER_STATUS_BC_SOURCE_IN_SYNC;
            }
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Invalid source_id\n"));
            return GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameters!\n"));
        return GATT_BASS_SERVER_STATUS_FAILED;
    }
}

/******************************************************************************/
GattBassServerStatus  GattBassServerModifyBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                                 uint8 sourceId,
                                                                 GattBassServerReceiveState *sourceInfo)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);
    GattBassServerStatus status = GATT_BASS_SERVER_STATUS_FAILED;

    if (bass_server)
    {
        uint8 index;
        bool isChanged = FALSE;

        if(bassFindBroadcastSource(bass_server,
                                   sourceId,
                                   &index))
        {
            status = bassServerCheckBroadcastSourceInfo(sourceInfo);

            if(status)
                return status;

            isChanged = bassServerIsBroadcastSourceChanged(bass_server,
                                                           index,
                                                           sourceInfo);

            bassServerSetReceiveState(bass_server, sourceInfo, index);

            if (isChanged)
            {
                /* If the Broadacast Receive State characteristic is changed, we need to notify its
                 * new value to all the clients that registered for it.*/
                bassServerNotifyBroadcastReceiveStateCharacteristic(bass_server,
                                                                    index);
            }

            status = gatt_status_success;
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Invalid source_id\n"));
            status = GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS Server instance is NULL!\n"));
        status = GATT_BASS_SERVER_STATUS_FAILED;
    }

    return status;
}


/******************************************************************************/
uint8* GattBassServerGetBroadcastCodeRequest(ServiceHandle srvcHndl,
                                             uint8 sourceId)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 *broadcast_code = NULL;

    if (bass_server && bass_server->data.broadcast_source)
    {
        uint8 index;

        if(bassFindBroadcastSource(bass_server,
                                   sourceId,
                                   &index))
        {
            broadcast_code = PanicUnlessMalloc(sizeof(uint8) * GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
            memcpy(broadcast_code, bass_server->data.broadcast_source[index]->broadcast_code, GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Invalid source_id\n"));
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameters!\n"));
    }

    return broadcast_code;
}

/******************************************************************************/
GattBassServerStatus GattBassServerSetBrodcastCodeRequest(ServiceHandle srvcHndl,
                                                          uint8 sourceId,
                                                          uint8 *broadcastCode)
{
    GBASSSS *bass_server = (GBASSSS *) ServiceHandleGetInstanceData(srvcHndl);

    if (bass_server && bass_server->data.broadcast_source)
    {
        uint8 index;

        if(bassFindBroadcastSource(bass_server,
                                   sourceId,
                                   &index))
        {
            memcpy(bass_server->data.broadcast_source[index]->broadcast_code, broadcastCode, GATT_BASS_SERVER_BROADCAST_CODE_SIZE);
            return GATT_BASS_SERVER_STATUS_SUCCESS;
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Invalid source_id\n"));
            return GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameters!\n"));
        return GATT_BASS_SERVER_STATUS_INVALID_PARAMETER;
    }
}
