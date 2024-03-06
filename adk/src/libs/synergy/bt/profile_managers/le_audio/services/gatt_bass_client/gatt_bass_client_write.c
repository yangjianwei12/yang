/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client_write.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_common.h"

/***************************************************************************/
static GattBassClientMessageId bassClientSetMessageIdWriteCfm(uint16 command)
{
    GattBassClientMessageId id = GATT_BASS_CLIENT_MESSAGE_TOP;

    switch(command)
    {
        case bass_client_write_remote_scan_stop_pending:
            id = GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM;
        break;

        case bass_client_write_remote_scan_start_pending:
            id = GATT_BASS_CLIENT_REMOTE_SCAN_START_CFM;
        break;

        case bass_client_write_add_source_pending:
            id = GATT_BASS_CLIENT_ADD_SOURCE_CFM;
        break;

        case bass_client_write_modify_source_pending:
            id = GATT_BASS_CLIENT_MODIFY_SOURCE_CFM;
        break;

        case bass_client_write_set_broadcast_code_pending:
            id = GATT_BASS_CLIENT_SET_BROADCAST_CODE_CFM;
        break;

        case bass_client_write_remove_source_pending:
            id = GATT_BASS_CLIENT_REMOVE_SOURCE_CFM;
        break;

        default:
        break;
    }

    return id;
}

/***************************************************************************/
void bassClientHandleWriteValueRespCfm(GBASSC *const bass_client,
                                       const CsrBtGattWriteCfm *const write_cfm)
{
    if (bass_client != NULL)
    {
        bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

        if (write_cfm->handle == bass_client->client_data.broadcast_audio_scan_control_point_handle)
        {
            GattBassClientMessageId id = bassClientSetMessageIdWriteCfm(bass_client->pending_cmd);


            if (id < GATT_BASS_CLIENT_MESSAGE_TOP)
            {
                bassClientSendBroadcastAudioScanControlOpCfm(bass_client,
                                                             write_cfm->resultCode,
                                                             id);
            }
            bass_client->pending_cmd = bass_client_pending_none;
            return;
        }

        while(ptr)
        {
            if (write_cfm->handle == ptr->handle_ccc)
            {
                uint8 source_id = 0;

                bassClientSourceIdFromCccHandle(bass_client,
                                                write_cfm->handle,
                                                &source_id);

                bassClientSendBroadcastReceiveStateSetNtfCfm(bass_client,
                                                             write_cfm->resultCode,
                                                             source_id);
                break;
            }

            ptr = ptr->next;
        }

        bass_client->pending_cmd = bass_client_pending_none;
    }
    else
    {
        gattBassClientPanic();
    }
}


/***************************************************************************/
static void bassClientHandleBroadcastAudioScanControlPointOperation(const GBASSC *client,
                                                                    ServiceHandle clnt_hndl,
                                                                    bass_client_control_point_opcodes_t opcode,
                                                                    uint16 bass_cntrl_pnt_len,
                                                                    uint16 parameters_size,
                                                                    uint8 * parameters,
                                                                    bool no_response,
                                                                    bool longWrite)
{
    MAKE_BASS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(BASS_CLIENT_INTERNAL_MSG_WRITE,
                                      bass_cntrl_pnt_len);

    message->clnt_hndl = clnt_hndl;
    message->handle = client->client_data.broadcast_audio_scan_control_point_handle;
    message->size_value = bass_cntrl_pnt_len;

    message->value[0] = opcode;

    if (opcode != bass_client_remote_scan_stop_op &&
        opcode != bass_client_remote_scan_start_op)
    {
        /* Remote Scan Stop and Remote Scan Start operations
         * are the only ones don't have parameters */
        memcpy(&(message->value[1]), parameters, parameters_size);
    }

    message->no_response = no_response;
    message->longWrite = longWrite;

    BassMessageSendConditionally(client->lib_task,
                             BASS_CLIENT_INTERNAL_MSG_WRITE,
                             message,
                             &client->pending_cmd);
}

/****************************************************************************/
static uint16 bassClientGetCntrlPntLenFromOpcode(bass_client_control_point_opcodes_t opcode)
{

    switch(opcode)
    {
        case bass_client_remote_scan_stop_op:
        return BASS_CLIENT_BROADCAST_REMOTE_SCAN_STOP_CTRL_POINT_SIZE;

        case bass_client_remote_scan_start_op:
        return BASS_CLIENT_BROADCAST_REMOTE_SCAN_START_CTRL_POINT_SIZE;

        default:
        return 0;
    }
}

/****************************************************************************/
void GattBassClientRemoteScanStopRequest(ServiceHandle clntHndl, bool noResponse)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                        client->srvcElem->service_handle,
                                        bass_client_remote_scan_stop_op,
                                        bassClientGetCntrlPntLenFromOpcode(bass_client_remote_scan_stop_op),
                                        0,
                                        NULL,
                                        noResponse,
                                        FALSE);
    }
    else
    {
        gattBassClientPanic();
    }
}

/****************************************************************************/
void GattBassClientRemoteScanStartRequest(ServiceHandle clntHndl, bool noResponse)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                        client->srvcElem->service_handle,
                                        bass_client_remote_scan_start_op,
                                        bassClientGetCntrlPntLenFromOpcode(bass_client_remote_scan_start_op),
                                        0,
                                        NULL,
                                        noResponse,
                                        FALSE);
    }
    else
    {
        gattBassClientPanic();
    }
}

/****************************************************************************/
static void bassClientPrepareDynamicFieldsControlPointParams(uint8 numSubGroups,
                                                            GattBassClientSubGroupsData *subGroupsData,
                                                            uint8 *value)
{
    uint8 i = 0;

    for(i=0; i<numSubGroups; i++)
    {
        *(++value) = (uint8) (subGroupsData[i].bisSync);
        *(++value) = (uint8) (subGroupsData[i].bisSync >> 8);
        *(++value) = (uint8) (subGroupsData[i].bisSync >> 16);
        *(++value) = (uint8) (subGroupsData[i].bisSync >> 24);

        *(++value) = subGroupsData[i].metadataLen;

        if (subGroupsData[i].metadataLen)
        {
            SynMemCpyS(++value,
                subGroupsData[i].metadataLen,
                subGroupsData[i].metadata,
                subGroupsData[i].metadataLen);

            value += (subGroupsData[i].metadataLen -1);
        }
    }
}

/****************************************************************************/
static void bassClientPrepareAddSourceOperationParameters(GattBassClientAddSourceParam *param,
                                                          uint8 *value)
{
    /* The format for the parameters of the Add Source operation is:
     * value[0] -> Advertising_Address_Type
     * value[1] -> value[6] -> Advertiser_Address
     * value[7] -> Advertising_SID
     * value[8] - value[10] -> Broadcast_ID
     * value[11] -> PA_Sync
     * value[12] - value[13] ->PA_Interval
     * value[14] -> Num_SubGroups
     *
     * For each subgroup:
     * value[x] - value[x+3] -> BIS_Sync
     * value[x+4] -> Metadata_Length
     * value[x+5] - value[x+5+Metadata_Length-1] -> Metadata
     * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
     */

    value[0] = (uint8) param->sourceAddress.type;

    value[1] = (uint8) param->sourceAddress.addr.lap;
    value[2] = (uint8) (param->sourceAddress.addr.lap >> 8);
    value[3] = (uint8) (param->sourceAddress.addr.lap >> 16);
    value[4] = param->sourceAddress.addr.uap;
    value[5] = (uint8) param->sourceAddress.addr.nap;
    value[6] = (uint8) (param->sourceAddress.addr.nap >> 8);

    value[7] = param->advSid;

    value[8] = (uint8) param->broadcastId;
    value[9] = (uint8) (param->broadcastId >> 8);
    value[10] = (uint8) (param->broadcastId >> 16);

    value[11] = (uint8) param->paSyncState;

    value[12] = (uint8) param->paInterval;
    value[13] = (uint8) (param->paInterval >> 8);

    value[14] = param->numSubGroups;

    if(param->numSubGroups)
    {
        uint8 *ptr = &value[14];

        bassClientPrepareDynamicFieldsControlPointParams(param->numSubGroups,
                                                        param->subGroupsData,
                                                        ptr);
    }
}

/****************************************************************************/
static uint16 bassClientCalculateDynamicFieldsLenCntrlPointOp(uint8 numSubGroups,
                                                              GattBassClientSubGroupsData *subGroupsData)
{
    uint16 len = 0;

    if(numSubGroups)
    {
       uint8 i;

       len += sizeof(uint32) * numSubGroups;
       len += sizeof(uint8) * numSubGroups;

       for(i=0; i<numSubGroups; i++)
       {
           len += subGroupsData[i].metadataLen;
       }
    }

    return len;
}

/****************************************************************************/
static uint16 bassClientCalculateAddSourceOpLen(GattBassClientAddSourceParam *param)
{
    uint16 len = BASS_CLIENT_BROADCAST_ADD_SOURCE_CTRL_POINT_PARAM_SIZE_MIN;

    len += bassClientCalculateDynamicFieldsLenCntrlPointOp(param->numSubGroups,
                                                           param->subGroupsData);

    return len;
}

/****************************************************************************/
void GattBassClientAddSourceRequest(ServiceHandle clntHndl,
                                    GattBassClientAddSourceParam *param,
                                    bool noResponse,
                                    bool longWrite)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if(noResponse && longWrite)
        {
            GATT_BASS_CLIENT_INFO("It's not possible to write a long characteristic without response!\n");
            return;
        }
        else
        {
            size_t size = (size_t) bassClientCalculateAddSourceOpLen(param);
            uint8* value = CsrPmemAlloc(size);

            bassClientPrepareAddSourceOperationParameters(param, value);

            bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                            client->srvcElem->service_handle,
                                            bass_client_add_source_op,
                                            (uint16) size ,
                                            (uint16)(size - 1),
                                            value,
                                            noResponse,
                                            longWrite);

            free(value);
        }
    }
    else
    {
        gattBassClientPanic();
    }
}

/****************************************************************************/
static void bassClientPrepareModifySourceOperationParameters(GattBassClientModifySourceParam *param,
                                                             uint8 *value)
{
    /* The format for the parameters of the Modify Source operation is:
     * value[0] -> Source_ID
     * value[1] -> PA_Sync
     * value[2] - value[3] -> PA_Interval
     * value[4] -> Num_Subgroups
     *
     * For each subgroup:
     * value[x] - value[x+3] -> BIS_Sync. It exists if value[10] not zero.
     * value[x+4] -> Metadata_Length.     It exists if value[10] not zero.
     * value[x+5] - value[x+5+Metadata_Length-1] -> Metada. It exists if the sum of the elements
     *                                                      of Metadata_Length is not zero.
     */

    value[0] = param->sourceId;
    value[1] = (uint8) param->paSyncState;

    value[2] = (uint8) param->paInterval;
    value[3] = (uint8) (param->paInterval >> 8);

    value[4] = param->numSubGroups;

    if(param->numSubGroups)
    {
        uint8 *ptr = &value[4];

        bassClientPrepareDynamicFieldsControlPointParams(param->numSubGroups,
                                                        param->subGroupsData,
                                                        ptr);
    }
}

/****************************************************************************/
static uint16 bassClientCalculateModifySourceOpLen(GattBassClientModifySourceParam *param)
{
    uint16 len = BASS_CLIENT_BROADCAST_MODIFY_SOURCE_CTRL_POINT_PARAM_SIZE_MIN;

    len += bassClientCalculateDynamicFieldsLenCntrlPointOp(param->numSubGroups,
                                                           param->subGroupsData);

    return len;
}

/****************************************************************************/
void GattBassClientModifySourceRequest(ServiceHandle clntHndl,
                                       GattBassClientModifySourceParam *params,
                                       bool noResponse)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        size_t size = (size_t) bassClientCalculateModifySourceOpLen(params);
        uint8 *value = CsrPmemAlloc(size);

        bassClientPrepareModifySourceOperationParameters(params, value);

        bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                        client->srvcElem->service_handle,
                                        bass_client_modify_source_op,
                                        (uint16) size,
                                        (uint16) (size-1),
                                        value,
                                        noResponse,
                                        FALSE);

        free(value);
    }
    else
    {
        gattBassClientPanic();
    }
}

/****************************************************************************/
void GattBassClientSetBroadcastCodeRequest(ServiceHandle clntHndl,
                                           uint8 sourceId,
                                           uint8 *broadcastCode,
                                           bool noResponse)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        size_t size = BASS_CLIENT_BROADCAST_CODE_SIZE + 1;
        uint8* value = CsrPmemAlloc(size);

        value[0] = sourceId;

        SynMemCpyS(&value[1], BASS_CLIENT_BROADCAST_CODE_SIZE, broadcastCode, BASS_CLIENT_BROADCAST_CODE_SIZE);
        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Set Broadacast Code operation is size + 1, because we need to include the byte
         * for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                        client->srvcElem->service_handle,
                                        bass_client_set_broadcast_code_op,
                                        (uint16)(size + 1),
                                        (uint16) size,
                                        value,
                                        noResponse,
                                        FALSE);

        free(value);
    }
    else
    {
        gattBassClientPanic();
    }
}

/****************************************************************************/
void GattBassClientRemoveSourceRequest(ServiceHandle clntHndl, uint8 sourceId, bool noResponse)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        uint8 value = sourceId;

        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Set Broadacast Code operation is BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE + 1,
         * because we need to include the byte for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(client,
                                        client->srvcElem->service_handle,
                                        bass_client_remove_source_op,
                                        BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE + 1,
                                        (uint16) BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE,
                                        &value,
                                        noResponse,
                                        FALSE);
    }
    else
    {
        gattBassClientPanic();
    }
}
