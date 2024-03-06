/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_bass_server_common.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_db.h"

/****************************************************************************/
uint16 bassServerGetHandleBroadcastReceiveStateCharacteristic(uint8 index)
{
    uint16 handle = HANDLE_BASS_BROADCAST_RECEIVE_STATE_1;

    handle += (index *(GATT_BASS_BROADCAST_RECEIVE_STATE_DB_SIZE + GATT_BASS_CLIENT_CONFIG_VALUE_SIZE));

    return handle;
}

/******************************************************************************/
void sendBassServerAccessRsp(
        Task task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
        )
{
    if (
            !GattManagerServerAccessResponse(
                task,
                cid,
                handle,
                result,
                size_value,
                value
                )
       )
    {
        GATT_BASS_SERVER_DEBUG_PANIC((
                    "bassServerAccessRsp: Couldn't send GattManagerServerAccessRsp\n"
                    ));

    }
}

/******************************************************************************/
void gattBassServerWriteGenericResponse(
        Task task,
        connection_id_t cid,
        uint16 result,
        uint16 handle
        )
{
    if (task == NULL)
    {
        GATT_BASS_SERVER_DEBUG_PANIC((
                    "BASS: Null instance!\n"
                    ));
    }
    else if (cid == 0)
    {
        GATT_BASS_SERVER_DEBUG_PANIC((
                    "BASS: No Cid!\n"
                    ));
    }
    else
    {
        sendBassServerAccessRsp(
             task,
             cid,
             handle,
             result,
             0,
             NULL
             );
    }
}

/***************************************************************************/
void bassServerSendCharacteristicChangedNotification(
        Task task,
        connection_id_t cid,
        uint16 handle,
        uint16 size_value,
        const uint8 *value
        )
{
    if (task == NULL)
    {
        GATT_BASS_SERVER_DEBUG_PANIC((
                    "BASS: Null instance!\n"
                    ));
    }
    else if ( cid == 0 )
    {
        GATT_BASS_SERVER_DEBUG_PANIC((
                    "BASS: No Cid!\n"
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

/***************************************************************************/
void bassServerHandleReadClientConfigAccess(
        Task task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_BASS_CLIENT_CONFIG_VALUE_SIZE];

    if (task == NULL)
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS: Null instance!\n"));
    }
    else if (cid == 0)
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("BASS: Invalid cid!\n"));
    }

    config_data[0] = (uint8) client_config;
    config_data[1] = (uint8) (client_config >> 8);

    sendBassServerAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
            GATT_BASS_CLIENT_CONFIG_VALUE_SIZE,
            config_data);
}

/***************************************************************************/
void bassServerHandleWriteClientConfigAccess(
        Task task,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 *client_config)
{
    if (access_ind->size_value != GATT_BASS_CLIENT_CONFIG_VALUE_SIZE)
    {
        sendBassServerAccessErrorRsp(
                task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_invalid_length);
    }
    /* Validate the input parameters - ONLY Notify*/
    else if (access_ind->value[0] == GATT_BASS_CLIENT_CONFIG_INDICATE)
    {
        sendBassServerAccessErrorRsp(
                task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_cccd_improper_config);
    }
    else if (access_ind->value[0] == GATT_BASS_CLIENT_CONFIG_NOTIFY || access_ind->value[0] == 0)
    {
        (*client_config) = access_ind->value[0] | (((uint16) access_ind->value[1]) << 8);

        /* Send response to the client */
        gattBassServerWriteGenericResponse(
                    task,
                    access_ind->cid,
                    gatt_status_success,
                    access_ind->handle
                    );
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        gattBassServerWriteGenericResponse(
                    task,
                    access_ind->cid,
                    gatt_status_success,
                    access_ind->handle
                    );
    }
}

/***************************************************************************/
bool bassFindBroadcastSource(GBASSSS *bass_server, uint8 source_id, uint8 *index)
{
    uint8 i;

    for(i=0; i<bass_server->data.broadcast_receive_state_num; i++)
    {
        /* Check if there is an available Broadcast Receive State characteristic */
        if (bass_server->data.broadcast_source[i] &&
            bass_server->data.broadcast_source[i]->source_id == source_id)
        {
            (*index) = i;
            return TRUE;
        }
    }

    return FALSE;
}

/**************************************************************************/
bool bassIsBroadcastReceiveStateFree(GBASSSS *bass_server, uint8 index)
{
    if(bass_server && bass_server->data.broadcast_source)
    {
        if(!bass_server->data.broadcast_source[index])
            return TRUE;
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("Invalid pointer parameter!\n"));
    }

    return FALSE;
}

/***************************************************************************/
void bassServerSwapByteTransmissionOrder(uint8 *value_to_swap,
                                         uint8 len,
                                         uint8 *value)
{
    uint8 i, j;

    for(i=0, j=len-1; i<len && j>=0; i++, j--)
    {
        value[i] = value_to_swap[j];
    }
}

/**************************************************************************/
void bassConvertBroadcastReceiveStateValue(GBASSSS *bass_server, uint8 *value, uint8 index)
{
    uint8 * ptr = NULL;
    uint8 i = 0;

    value[0] = bass_server->data.broadcast_source[index]->source_id;

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type == TYPED_BDADDR_INVALID)
    {
        value[1] = 0;
    }
    else
    {
        value[1] = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type;
    }

    value[2] = (uint8) bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap;
    value[3] = (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap >> 8);
    value[4] = (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap >> 16);
    value[5] = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.uap;
    value[6] = (uint8) bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.nap;
    value[7] = (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.nap >> 8);

    value[8] = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAdvSid;

    value[9] = bass_server->data.broadcast_source[index]->broadcast_source_state.broadcastId;
    value[10] = bass_server->data.broadcast_source[index]->broadcast_source_state.broadcastId >> 8;
    value[11] = bass_server->data.broadcast_source[index]->broadcast_source_state.broadcastId >> 16;
    value[12] = (uint8) bass_server->data.broadcast_source[index]->broadcast_source_state.paSyncState;
    value[13] = bass_server->data.broadcast_source[index]->broadcast_source_state.bigEncryption;

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode)
    {
        bassServerSwapByteTransmissionOrder(bass_server->data.broadcast_source[index]->broadcast_source_state.badCode,
                                            GATT_BASS_SERVER_BROADCAST_CODE_SIZE,
                                            &value[14]);
        ptr = (&value[14]) + GATT_BASS_SERVER_BROADCAST_CODE_SIZE;
    }
    else
    {
        ptr = (&value[14]);
    }

    *ptr = bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups;

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups)
    {
        for(i=0; i<bass_server->data.broadcast_source[index]->broadcast_source_state.numSubGroups; i++)
        {
            *(++ptr) =  (uint8) bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync;
            *(++ptr) =  (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync >> 8);
            *(++ptr) =  (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync >> 16);
            *(++ptr) =  (uint8) (bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].bisSync >> 24);

            *(++ptr) = bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen;

            if(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen)
            {
                bassServerSwapByteTransmissionOrder(bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata,
                                                    bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen,
                                                    ++ptr);

                ptr += (bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen - 1);
            }
        }
    }
}

/**************************************************************************/
void bassServerNotifyBroadcastReceiveStateCharacteristic(GBASSSS *bass_server, uint8 index)
{
    /* If the ccc is NOTIFY, we have to notify the Broadcast Receive State characteristic value */
    uint8 i;

    for (i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
    {
        if (bass_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client. */
            if (bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[index] == GATT_BASS_CLIENT_CONFIG_NOTIFY)
            {
                if(bass_server->data.broadcast_source[index])
                {
                    uint32 len = bassServerCalculateBroadcastReceiveStateCharacteristicLen(bass_server->data.broadcast_source[index]);
                    uint8 *value = PanicUnlessMalloc(sizeof(uint8) * len);

                    bassConvertBroadcastReceiveStateValue(bass_server, value, index);

                    bassServerSendCharacteristicChangedNotification(
                                (Task) &bass_server->lib_task,
                                bass_server->data.connected_clients[i].cid,
                                bassServerGetHandleBroadcastReceiveStateCharacteristic(index),
                                len,
                                value);

                    free(value);
                }
                else
                {
                    bassServerSendCharacteristicChangedNotification(
                            (Task) &bass_server->lib_task,
                            bass_server->data.connected_clients[i].cid,
                            bassServerGetHandleBroadcastReceiveStateCharacteristic(index),
                            0,
                            NULL);
                }
            }
        }
    }
}

/**************************************************************************/
static bool bassServerIsMetadataValid(GattBassServerReceiveState *source_info)
{
    uint8 i;

    for(i=0; i<source_info->numSubGroups; i++)
    {
        if((!source_info->subGroupsData[i].metadataLen && source_info->subGroupsData[i].metadata) ||
           (source_info->subGroupsData[i].metadataLen  && !source_info->subGroupsData[i].metadata))
                return FALSE;
    }

    return TRUE;
}

/**************************************************************************/
GattBassServerStatus bassServerCheckBroadcastSourceInfo(GattBassServerReceiveState *source_info)
{
    GattBassServerStatus status = GATT_BASS_SERVER_STATUS_INVALID_PARAMETER;

    if (VALID_BIG_ENCRYPTION(source_info->bigEncryption)             &&
        VALID_PA_SYNC_STATE(source_info->paSyncState)                &&
        source_info->sourceAdvSid <= BASS_SERVER_ADVERTISING_SID_MAX &&
        VALID_ADVERTISE_ADDRESS_TYPE(source_info->sourceAddress.type)  &&
        source_info->broadcastId <= BROADCAST_ID_MAX_VALUE             &&
        bassServerIsValidBadCodeValue(source_info->badCode, source_info->bigEncryption))
    {
        if(source_info->numSubGroups  &&
           source_info->subGroupsData &&
           bassServerIsMetadataValid(source_info))
        {
            uint8 i;
            uint32 *bisSync = (uint32 *) PanicUnlessMalloc(sizeof(uint32) * source_info->numSubGroups);
            uint32 *ptr = bisSync;

            for(i=0; i<source_info->numSubGroups; i++,ptr++)
            {
                *ptr = source_info->subGroupsData[i].bisSync;
            }

            if(bassServerIsValidBisSync(bisSync, source_info->numSubGroups))
                status = GATT_BASS_SERVER_STATUS_SUCCESS;

            free(bisSync);
        }
        else
        {
            if(!(source_info->subGroupsData))
                status = GATT_BASS_SERVER_STATUS_SUCCESS;
        }
    }

    return status;
}

/**************************************************************************/
static bool bassServerIsBisSynchronized(GBASSSS * bass_server, uint8 source_index)
{
    if(bass_server->data.broadcast_source[source_index]->broadcast_source_state.subGroupsData)
    {
        uint8 i = 0;

        for(i=0; i<bass_server->data.broadcast_source[source_index]->broadcast_source_state.numSubGroups; i++)
        {
            if(bass_server->data.broadcast_source[source_index]->broadcast_source_state.subGroupsData[i].bisSync &&
               bass_server->data.broadcast_source[source_index]->broadcast_source_state.subGroupsData[i].bisSync  != BASS_SERVER_FAILED_SYNC_BIG)
                return TRUE;
        }
    }

    return FALSE;
}

/**************************************************************************/
bool bassServerIsSynchronized(GBASSSS * bass_server, uint8 source_index)
{
    if (!bassServerIsBisSynchronized(bass_server, source_index) &&
        bass_server->data.broadcast_source[source_index]->broadcast_source_state.paSyncState != GATT_BASS_SERVER_SYNCHRONIZED)
    {
        return FALSE;
    }

    return TRUE;
}

static void bassServerCalculateLenDynamicFieldsControlPointOp(uint8 numSubGroups, uint16 *size, uint8 *value)
{
    uint8 i;

    /* Add 4 bytes for each BIS_sync - The number of BIS_sync is given by Num_Subgroups */
    (*size) += (sizeof(uint32) * numSubGroups);

    /* Add numSubGroups bytes for the metadata length field
       (there is one byte for each metadata length of each subgroup.
    */
    (*size) += (sizeof(uint8) * numSubGroups);

    /* Add the metadata size (sum of all metadata lengths) */
    for(i=0; i<numSubGroups; i++)
    {
        value += (sizeof(uint32) + 1);

        if(*value)
        {
            (*size) += (*value);
            value += (*value);
        }
    }
}

/**************************************************************************/
uint16 bassServerCalculateAddSourceOpLength(uint8 *value)
{
    /* The format for the Add Source operation is:
     * value[0] -> 0x02 = Add Source operation
     * value[1] -> Advertising_Address_Type
     * value[2] -> value[7] -> Advertiser_Address
     * value[8] -> Advertising_SID
     * value[9] - value[11] -> Broadcast_ID
     * value[12] -> PA_Sync
     * value[13] - value[14] -> PA_Interval
     * value[15] -> Num_Subgroups
     * value[16] -> BIS_Sync[0]
     * ...
     * value[16+Num_Subgroups-1] -> BIS_Sync[Num_Subgroups-1]
     * value[16+Num_Subgroups] -> Metadata_Length[0]
     * ...
     * value[16+Num_Subgroups+Num_Subgroups-1] -> Metadata_Length[Num_Subgroups-1]
     * value[16+Num_Subgroups+Num_Subgroups] -
                  value[16+Num_Subgroups+Num_Subgroups+Metadata_Length[0]-1]-> Metada[0]
     * ...
     * value[...] - value[...]-> Metada[Num_Subgroups-1]
     * NOTE: Metadata exists only if the Metadata_Length parameter value is 0x00.
     */
    uint16 size = GATT_BASS_ADD_SOURCE_OPCODE_SIZE;

    if(value[15])
    {
       uint8 *ptr = (value + (GATT_BASS_ADD_SOURCE_OPCODE_SIZE - 1));
       bassServerCalculateLenDynamicFieldsControlPointOp(value[15], &size, ptr);
    }

    return size;
}

/**************************************************************************/
uint16 bassServerCalculateModifySourceOpLength(uint8 *value)
{
    /* The format for the Modify Source operation is:
     * value[0] -> 0x03 = Modify Source operation
     * value[1] -> Source_ID
     * value[2] -> PA_Sync
     * value[3] - value[4] -> PA_Interval
     * value[5] -> Num_Subgroups
     * value[6] -> BIS_Sync[0]
     * ...
     * value[6+Num_Subgroups-1] -> BIS_Sync[Num_Subgroups-1]
     * value[6+Num_Subgroups] -> Metadata_Length[0]
     * ...
     * value[6+Num_Subgroups+Num_Subgroups-1] -> Metadata_Length[Num_Subgroups-1]
     * value[6+Num_Subgroups+Num_Subgroups] -
                  value[6+Num_Subgroups+Num_Subgroups+Metadata_Length[0]-1]-> Metada[0]
     * ...
     * value[...] - value[...]-> Metada[Num_Subgroups-1]
     * NOTE: Metadata exists only if the Metadata_Length parameter value is 0x00.
     */
    uint16 size = GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE;

    if(value[5])
    {
        uint8 *ptr = (value + (GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE - 1));
        bassServerCalculateLenDynamicFieldsControlPointOp(value[5], &size, ptr);
    }

    return size;
}

/**************************************************************************/
bool bassServerIsValidBisSync(uint32 *bisSync, uint8 numSubGroups)
{
    uint8 i = 0;
    uint32 tmp = 0;

    if(!bisSync)
        return FALSE;

    /* The values in bisSync are valid if a bit set to 1
     * in one of them, is not set to in any of the others */
    for(i=0; i<numSubGroups; i++)
    {
        if(bisSync[i] == GATT_BASS_SERVER_BIS_STATE_NO_PREFERENCE)
            continue;

        if(tmp & bisSync[i])
            return FALSE;

        tmp = tmp | bisSync[i];
    }

    return TRUE;
}

/**************************************************************************/
uint16 bassServerCalculateBroadcastReceiveStateCharacteristicLen(gatt_bass_broadcast_source_info_t *sourceInfo)
{
    uint32 len = GATT_BASS_BROADCAST_RECEIVE_STATE_SIZE_MIN;
    uint8 i;

    /* Check if there is the Bad_Code field */
    if(sourceInfo->broadcast_source_state.bigEncryption == GATT_BASS_BAD_CODE)
    {
        len += GATT_BASS_SERVER_BROADCAST_CODE_SIZE;
    }

    /* Add the length of the BIS_Sync_State field */
    len += (sizeof(uint32) * sourceInfo->broadcast_source_state.numSubGroups);

    /* Add the length of Metadata_length  and Metadata fields */
    len += sourceInfo->broadcast_source_state.numSubGroups;

    for(i=0; i<sourceInfo->broadcast_source_state.numSubGroups; i++)
    {
        len += sourceInfo->broadcast_source_state.subGroupsData[i].metadataLen;
    }

    return len;
}

/**************************************************************************/
bool bassServerIsValidBadCodeValue(uint8 *badCode, GattBassServerBroadcastBigEncryption bigEncryption)
{
    if((bigEncryption == GATT_BASS_BAD_CODE && badCode) ||
       (bigEncryption != GATT_BASS_BAD_CODE && !badCode))
        return TRUE;

    return FALSE;
}

/**************************************************************************/
uint8 bassServerCalcNumBroadcastReceiveStateCharacteristicsNotEmpty(GBASSSS * bassServer)
{
    uint8 notEmptyBrsNum = 0;

    if(bassServer && bassServer->data.broadcast_source)
    {
        uint8 i = 0;

        for (i=0; i<bassServer->data.broadcast_receive_state_num; i++)
        {
            if(bassServer->data.broadcast_source[i])
                notEmptyBrsNum++;
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameter!\n"));
    }

    return notEmptyBrsNum;
}

/**************************************************************************/
bool bassServerIsAnyBroadcastReceiveStateCharacteristicsEmpty(GBASSSS * bassServer, uint8 *index)
{
    (*index) = 0;

    if(bassServer && bassServer->data.broadcast_source)
    {
        uint8 i = 0;

        for(i=0; i<bassServer->data.broadcast_receive_state_num; i++)
        {
            if(!bassServer->data.broadcast_source[i])
            {
                (*index) = i;
                return TRUE;
            }
        }
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_PANIC(("NULL pointer parameter!\n"));
    }

    return FALSE;
}

/**************************************************************************/
bool bassServerIsValidSourceId(GBASSSS *bassServer, uint8 sourceId)
{
    if(bassServer && bassServer->data.broadcast_source)
    {
        uint8 i = 0;

        for(i=0; i<bassServer->data.broadcast_receive_state_num; i++)
        {
            if(bassServer->data.broadcast_source[i] &&
               bassServer->data.broadcast_source[i]->source_id == sourceId)
                return TRUE;
        }
    }

    return FALSE;
}
