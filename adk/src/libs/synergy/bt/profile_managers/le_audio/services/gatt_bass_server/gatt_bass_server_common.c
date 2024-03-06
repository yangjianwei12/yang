/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_bass_server_common.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_db.h"
#include "csr_bt_util.h"

#define CLIENT_CHARACTERISTIC_CONFIG_OFFSET (1)

static void bassServerSendConfigChangeIndication(GBASSSS *bass_server,
    connection_id_t cid,
    bool configChangeComplete)
{
    MAKE_BASS_MESSAGE(GattBassServerConfigChangeInd);

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->configChangeComplete = configChangeComplete;

    BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_CONFIG_CHANGE_IND, message);
}

static bool bassServerAllClientConfigWritten(GBASSSS *bass_server,
    connection_id_t cid)
{
    uint8 i, j;

    for(i = 0; i < BASS_SERVER_MAX_CONNECTIONS; i++)
    {
        if(bass_server->data.connected_clients[i].cid == cid)
        {
            for(j=0; j<bass_server->data.broadcast_receive_state_num; j++)
            {
                if (bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[j] == GATT_BASS_SERVER_INVALID_CLIENT_CONFIG)
                    return FALSE;
            }
            return TRUE;
        }
    }

    return FALSE;
}

static void bassServerSetClientConfigWrite(GBASSSS *bass_server,
                            connection_id_t cid,
                            bool clientConfigChanged)
{
    bool configChangeComplete = bassServerAllClientConfigWritten(bass_server, cid);

    if (clientConfigChanged)
    {
        /* Inform above layer when all CCCD handles are written by client*/
        bassServerSendConfigChangeIndication(bass_server,
                                          cid,
                                          configChangeComplete);
    }
}

static bool bassServerClientConfigChanged(uint16 clientCfg, uint16 newClientCfg)
{
    /* Check if the client config has changed, to notify above layer */
    if(clientCfg != newClientCfg)
        return TRUE;

    return FALSE;
}

/****************************************************************************/
uint16 bassServerGetHandleBroadcastReceiveStateCharacteristic(uint8 index)
{
    uint16 handle = HANDLE_BASS_BROADCAST_RECEIVE_STATE_1;

    handle += (index *(GATT_BASS_BROADCAST_RECEIVE_STATE_DB_SIZE + GATT_BASS_CLIENT_CONFIG_VALUE_SIZE));

    return handle;
}

/******************************************************************************/
void sendBassServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 size_value,
        const uint8 *value
        )
{
    uint8* data;

    data = (uint8*)CsrPmemZalloc(size_value);
    CsrMemCpy(data, value, size_value);

    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 size_value,
                                 data);

}

/******************************************************************************/
void gattBassServerWriteGenericResponse(
        CsrBtGattId     task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_BASS_SERVER_PANIC(
                    "BASS: Invalid Gatt Id!\n"
                    );
    }
    else if (cid == CSR_BT_CONN_ID_INVALID)
    {
        GATT_BASS_SERVER_PANIC(
                    "BASS: No Cid!\n"
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
void bassServerSendCharacteristicChangedNotification(
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

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_BASS_SERVER_PANIC(
                    "BASS: Invalid Gatt Id!\n"
                    );
    }
    else if ( cid == CSR_BT_CONN_ID_INVALID )
    {
        GATT_BASS_SERVER_PANIC(
                    "BASS: No Cid!\n"
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

/***************************************************************************/
void bassServerHandleReadClientConfigAccess(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        const uint16 client_config
        )
{
    uint8 config_data[GATT_BASS_CLIENT_CONFIG_VALUE_SIZE];

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_BASS_SERVER_PANIC("BASS: Invalid Gatt Id !\n");
    }
    else if (cid == CSR_BT_CONN_ID_INVALID)
    {
        GATT_BASS_SERVER_PANIC("BASS: Invalid cid!\n");
    }

     /* Default value of clientConfig is set as 0xFFFF. If client has not written
        any CCCD then we need to replace 0xFFFF with 0x0 (Disable) while
        responding. Default value is changed from 0 to 0xFFFF because of
        CCCD values getting lost if the device panics without these values are
        passed to application.
      */
    if(client_config != GATT_BASS_SERVER_INVALID_CLIENT_CONFIG)
    {
        config_data[0] = (uint8) client_config;
        config_data[1] = (uint8) (client_config >> 8);
    }
    else
    {
        config_data[0] = 0;
        config_data[1] = 0;
    }

    sendBassServerAccessRsp(
            task,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            GATT_BASS_CLIENT_CONFIG_VALUE_SIZE,
            config_data);
}

/***************************************************************************/
void bassServerHandleWriteClientConfigAccess(GBASSSS *bass_server,
                                             const CsrBtGattAccessInd *access_ind,
                                             uint16 *client_config)
{
    if(access_ind->numWriteUnits)
    {
        uint16 sizeValue = 0;
        uint8 i = 0;
        bool clientConfigChanged = FALSE;
        uint16 clientCfg;

        for(; i<access_ind->numWriteUnits; i++)
            sizeValue += access_ind->writeUnit[i].valueLength;

        if (sizeValue != GATT_BASS_CLIENT_CONFIG_VALUE_SIZE)
        {
            sendBassServerAccessErrorRsp(
                        bass_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
        }
        /* Validate the input parameters - ONLY Notify*/
        else if (access_ind->writeUnit[0].value[0] == GATT_BASS_CLIENT_CONFIG_INDICATE)
        {
            sendBassServerAccessErrorRsp(
                        bass_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF);
        }
        else if (access_ind->writeUnit[0].value[0] == GATT_BASS_CLIENT_CONFIG_NOTIFY ||
                 access_ind->writeUnit[0].value[0] == 0)
        {
            clientCfg = (*client_config);

            (*client_config) = ((uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit,CLIENT_CHARACTERISTIC_CONFIG_OFFSET)) |
                               (((uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit,(CLIENT_CHARACTERISTIC_CONFIG_OFFSET+1))) << 8);

            /* Check if the client config has changed, to notify above layer */
            clientConfigChanged = bassServerClientConfigChanged(clientCfg, (*client_config));

            /* Inform application for client write operation */
            bassServerSetClientConfigWrite(bass_server, access_ind->cid, clientConfigChanged);

            /* Send response to the client */
            gattBassServerWriteGenericResponse(
                        bass_server->gattId,
                        access_ind->cid,
                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                        access_ind->handle
                        );
        }
        else
        {
            /* Send response to the client but the value is ignored*/
            gattBassServerWriteGenericResponse(
                        bass_server->gattId,
                        access_ind->cid,
                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                        access_ind->handle
                        );
        }
    }
    else
    {
        sendBassServerAccessErrorRsp(
                    bass_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
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
        GATT_BASS_SERVER_PANIC("Invalid pointer parameter!\n");
    }

    return FALSE;
}

/***************************************************************************/
void bassServerSwapByteTransmissionOrder(uint8* value_to_swap,
                                         uint8 len,
                                         uint8* value)
{
    uint8 i, j;

    for (i = 0, j = len - 1; i < len && j >= 0; i++, j--)
    {
        value[i] = value_to_swap[j];
    }
}

/***************************************************************************/
void bassServerSwapByteWriteTransmissionOrder(uint16 numWriteUnits,
                                              CsrBtGattAttrWritePairs *writeUnit,
                                              uint16 startOffset,
                                              uint8 len,
                                              uint8* value)
{
    uint8 i=0, offset = startOffset + (len - 1);

    for (; i < len && offset >= startOffset; i++, offset--)
    {
        value[i] = bassServerGetElementBuffer(numWriteUnits, writeUnit, offset);
    }
}

/**************************************************************************/
void bassConvertBroadcastReceiveStateValue(GBASSSS *bass_server, uint8 *value, uint8 index)
{
    uint8 * ptr = NULL;
    uint8 i = 0;

    value[0] = bass_server->data.broadcast_source[index]->source_id;

    if(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type == TBDADDR_INVALID)
    {
        value[1] = 0;
    }
    else
    {
        value[1] = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.type;
    }

    value[2] = (uint8)bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap;
    value[3] = (uint8)(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap >> 8);
    value[4] = (uint8)(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.lap >> 16);
    value[5] = bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.uap;
    value[6] = (uint8)bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.nap;
    value[7] = (uint8)(bass_server->data.broadcast_source[index]->broadcast_source_state.sourceAddress.addr.nap >> 8);

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
                SynMemCpyS(++ptr, bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen,
                           bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadata,
                           bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen);

                ptr += (bass_server->data.broadcast_source[index]->broadcast_source_state.subGroupsData[i].metadataLen - 1);
            }
        }
    }
}

/**************************************************************************/
void bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient(GBASSSS *bass_server,
                                                                       uint8 index,
                                                                       uint8 clientIndex)
{
    /* If the Client Config is 0x01 (Notify is TRUE), a notification will
     * be sent to the client. */
    if (bass_server->data.connected_clients[clientIndex].client_cfg.receiveStateCcc[index] == GATT_BASS_CLIENT_CONFIG_NOTIFY)
    {
        if(bass_server->data.broadcast_source[index])
        {
            uint32 len = bassServerCalculateBroadcastReceiveStateCharacteristicLen(bass_server->data.broadcast_source[index]);
            uint8 *value = CsrPmemAlloc(sizeof(uint8) * len);

            bassConvertBroadcastReceiveStateValue(bass_server, value, index);

            bassServerSendCharacteristicChangedNotification(
                    bass_server->gattId,
                    bass_server->data.connected_clients[clientIndex].cid,
                    bassServerGetHandleBroadcastReceiveStateCharacteristic(index),
                    len,
                    value);

            CsrPmemFree(value);
        }
        else
        {
            bassServerSendCharacteristicChangedNotification(
                    bass_server->gattId,
                    bass_server->data.connected_clients[clientIndex].cid,
                    bassServerGetHandleBroadcastReceiveStateCharacteristic(index),
                    0,
                    NULL);
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
            bassServerNotifyBroadcastReceiveStateCharacteristicToSingleClient(bass_server,
                                                                              index,
                                                                              i);
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
            uint32 *bisSync = (uint32 *) CsrPmemAlloc(sizeof(uint32) * source_info->numSubGroups);
            uint32 *ptr = bisSync;

            for(i=0; i<source_info->numSubGroups; i++,ptr++)
            {
                *ptr = source_info->subGroupsData[i].bisSync;
            }

            if(bassServerIsValidBisSync(bisSync, source_info->numSubGroups))
                status = GATT_BASS_SERVER_STATUS_SUCCESS;

            CsrPmemFree(bisSync);
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

/**************************************************************************/
uint8 bassServerGetElementBuffer(uint16 numWriteUnits,
                                 CsrBtGattAttrWritePairs *writeUnit,
                                 uint16 offset)
{
    uint8 i = 0;

    for(; i<numWriteUnits; i++)
    {
        if (offset > writeUnit[i].valueLength)
            offset -= writeUnit[i].valueLength;
        else
            break;
    }

    return (writeUnit[i].value[offset-1]);
}

/**************************************************************************/
static bool bassServerIsLenDynamicFieldsControlPointOpValid(uint16 length,
                                                            uint16 minSize,
                                                            uint8 numSubGroups,
                                                            uint16 numWriteUnits,
                                                            CsrBtGattAttrWritePairs *writeUnit)
{
    uint8 i = 0;
    uint16 size = minSize;
    uint16 offset = minSize;
    uint8 value = 0;

    /* Since the characteristic has a variable length that depends on the value of
     * some of its fields, we need to calculate what is the expected length parsing
     * the characteristic value.
     * We have already checked at this point that there is a certain number of subroups
     * (numSubGroups parameters) and that the length of the characteristic is greater than
     * the minimum one.
     */

    /* We know for sure that if there is a certain number of subgroups, we have to have at least
     * 4 bytes for each subgroups for its BIS_sync field ...
     */
    size += (sizeof(uint32) * numSubGroups);

    /* ... and one byte for each metadata length of each subgroup. */
    size += (sizeof(uint8) * numSubGroups);

    /* size is now the minimum length we should have with a number of subgroups equal to
     * numSubGroups (the length of the characteristic if any of the subgroup has no metadata).
     *
     * If the length of the characteristic is less than the actual value of size, we have
     * a case of invalid length.
     *
     * Otherwise we will need:
     * - to parse each subgroup to retrieve the value of the metadata length of each subgroup
     * - to add this value to the value of size to have the new minimum size of the characteristic
     * - to check that the length of the characteristic is not less than this new value of size
     *   (otherwise we have a case of invalid length)
     *
     * If no case of invalid length has been detected before ending the parsing of all subgroups,
     * we can compare the actual length of the characteristic (length parameter) with
     * the final value of size that is the one we are expecting.
     */
    for(i=0; i<numSubGroups; i++)
    {
        if(length < size)
            return FALSE;

        offset += (sizeof(uint32) + 1);
        value = bassServerGetElementBuffer(numWriteUnits,
                                           writeUnit,
                                           offset);

        if(value)
        {
            size += value;
            offset += value;
        }
    }

    return (length == size ? TRUE : FALSE);
}

/**************************************************************************/
bool bassServerIsControlPointLengthValid(uint16 length,
                                         uint8 opcode,
                                         uint16 numWriteUnits,
                                         CsrBtGattAttrWritePairs *writeUnit)
{ 
    uint8 numSubGroups = 0;
    uint16 minSize = 0;

    if(opcode == GATT_BASS_ADD_SOURCE_OPCODE)
        minSize = GATT_BASS_ADD_SOURCE_OPCODE_SIZE;
    else if(opcode == GATT_BASS_MODIFY_SOURCE_OPCODE)
        minSize = GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE;
    else /* Invalid opcode */
        return FALSE;

    /* Invalid length in case the length of the characteristic
     * is less than the minimum one */
    if(length < minSize)
        return FALSE;

    /* Retrieve the number of subgroups */
    numSubGroups = bassServerGetElementBuffer(numWriteUnits, writeUnit, minSize);

    /* Invalid length in case:
     * - there is no subroups and the the length of the characteristic is not
     *   equal to the minimum one
     * - there is at least one subroup and the the length of the characteristic is
     *   equal to the minimum one
     */
    if((!numSubGroups && length > minSize) ||
       (numSubGroups && length == minSize))
        return FALSE;

    if(numSubGroups)
    {
        /* There is at least one subgroup and the length is greater than
         * the minimum one: we need to check the variable part of the characteristic
         */
        return bassServerIsLenDynamicFieldsControlPointOpValid(length,
                                                               minSize,
                                                               numSubGroups,
                                                               numWriteUnits,
                                                               writeUnit);
    }

    /* There is no subgroups and length is equal to the minimum one: valid case */
    return TRUE;
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
        GATT_BASS_SERVER_PANIC("NULL pointer parameter!\n");
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
        GATT_BASS_SERVER_PANIC("NULL pointer parameter!\n");
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

/******************************************************************************/
bool bassServerIsBadCodeChanged(GBASSSS *bass_server,
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
bool bassServerIsBisSyncChanged(GBASSSS *bass_server,
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
bool bassServerIsMetadataChanged(GBASSSS *bass_server,
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
                      subGroupsData[i].metadataLen))
                return TRUE;
        }
    }

    return FALSE;
}

/***************************************************************************/
bool bassServerFindCid(GBASSSS *bass_server, connection_id_t cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
    {
        if(bass_server->data.connected_clients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }

    return res;
}
