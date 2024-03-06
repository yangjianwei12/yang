/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

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
                                       const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *const write_cfm)
{
    if (bass_client != NULL)
    {
        if (write_cfm->handle == bass_client->client_data.broadcast_audio_scan_control_point_handle)
        {
            GattBassClientMessageId id = bassClientSetMessageIdWriteCfm(bass_client->pending_cmd);


            if (id < GATT_BASS_CLIENT_MESSAGE_TOP)
            {
                bassClientSendBroadcastAudioScanControlOpCfm(bass_client,
                                                             write_cfm->status,
                                                             id);
            }
        }
        else
        {
            bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                if (write_cfm->handle == ptr->handle_ccc)
                {
                    uint8 source_id = 0;

                    bassClientSourceIdFromCccHandle(bass_client,
                                                    write_cfm->handle,
                                                    &source_id);

                    bassClientSendBroadcastReceiveStateSetNtfCfm(bass_client,
                                                                 write_cfm->status,
                                                                 source_id);
                    break;
                }

                ptr = ptr->next;
            }
        }

        bass_client->pending_cmd = bass_client_pending_none;
    }
    else
    {
        GATT_BASS_CLIENT_PANIC(("Null instance\n"));
    }
}

/***************************************************************************/
void bassClientHandleWriteWithoutResponseRespCfm(GBASSC *const bass_client,
                                       const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T * const write_cfm)
{
    if (bass_client != NULL)
    {
        if (write_cfm->handle == bass_client->client_data.broadcast_audio_scan_control_point_handle)
        {
            GattBassClientMessageId id = bassClientSetMessageIdWriteCfm(bass_client->pending_cmd);


            if (id < GATT_BASS_CLIENT_MESSAGE_TOP)
            {
                bassClientSendBroadcastAudioScanControlOpCfm(bass_client,
                                                             write_cfm->status,
                                                             id);
            }
        }

        bass_client->pending_cmd = bass_client_pending_none;
    }
    else
    {
        GATT_BASS_CLIENT_PANIC(("Null instance\n"));
    }
}

/***************************************************************************/
static void bassClientHandleBroadcastAudioScanControlPointOperation(const GBASSC *gatt_bass_client,
                                                                    bass_client_control_point_opcodes_t opcode,
                                                                    uint8 bass_cntrl_pnt_len,
                                                                    uint16 parameters_size,
                                                                    uint8 * parameters,
                                                                    bool no_response)
{
    MAKE_BASS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(BASS_CLIENT_INTERNAL_MSG_WRITE,
                                               bass_cntrl_pnt_len);

    message->handle = gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle;
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

    MessageSendConditionally((Task)&gatt_bass_client->lib_task,
                             BASS_CLIENT_INTERNAL_MSG_WRITE,
                             message,
                             &gatt_bass_client->pending_cmd);
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
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_remote_scan_stop_op,
                                        bassClientGetCntrlPntLenFromOpcode(bass_client_remote_scan_stop_op),
                                        0,
                                        NULL,
                                        noResponse);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
    }
}

/****************************************************************************/
void GattBassClientRemoteScanStartRequest(ServiceHandle clntHndl, bool noResponse)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_remote_scan_start_op,
                                        bassClientGetCntrlPntLenFromOpcode(bass_client_remote_scan_start_op),
                                        0,
                                        NULL,
                                        noResponse);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
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
     * value[8] -> PA_Sync
     * value[9] - value[12] -> BIS_Sync
     * value[13] -> Metadata_Length
     * value[14] - value[Metadata_Length-1] -> Metada
     * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
     */

    value[0] = param->sourceAddress.type;

    value[1] = (uint8) param->sourceAddress.addr.lap;
    value[2] = (uint8) (param->sourceAddress.addr.lap >> 8);
    value[3] = (uint8) (param->sourceAddress.addr.lap >> 16);
    value[4] = param->sourceAddress.addr.uap;
    value[5] = (uint8) param->sourceAddress.addr.nap;
    value[6] = (uint8) (param->sourceAddress.addr.nap >> 8);

    value[7] = param->advSid;
    value[8] = param->paSyncState;

    value[9]  = (uint8) (param->bisSyncState);
    value[10] = (uint8) (param->bisSyncState >> 8);
    value[11] = (uint8) (param->bisSyncState >> 16);
    value[12] = (uint8) (param->bisSyncState >> 24);

    value[13] = param->metadataLen;

    if(param->metadataLen)
    {
        bassClientSwapByteTrasmissionOrder(param->metadataValue,
                                           param->metadataLen,
                                           &value[14]);
    }
}

/****************************************************************************/
void GattBassClientAddSourceRequest(ServiceHandle clntHndl,
                                    GattBassClientAddSourceParam *param,
                                    bool noResponse)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        size_t size = BASS_CLIENT_BROADCAST_ADD_SOURCE_CTRL_POINT_PARAM_SIZE_MIN + param->metadataLen;
        uint8* value = PanicUnlessMalloc(size);

        bassClientPrepareAddSourceOperationParameters(param, value);

        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Add Source operation is size + 1, because we need to include the byte
         * for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_add_source_op,
                                        size + 1,
                                        (uint16) size,
                                        value,
                                        noResponse);

        free(value);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
    }
}

/****************************************************************************/
static void bassClientPrepareModifySourceOperationParameters(GattBassClientModifySourceParam *param,
                                                             uint8 *value)
{
    /* The format for the parameters of the Modify Source operation is:
     * value[0] -> Source_ID
     * value[1] -> PA_Sync
     * value[2] - value[5] -> BIS_Sync
     * value[6] -> Metadata_Length
     * value[7] - value[Metadata_Length-1] -> Metada
     * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
     */

    value[0] = param->sourceId;
    value[1] = param->paSyncState;

    value[2] = (uint8) (param->bisSyncState);
    value[3] = (uint8) (param->bisSyncState >> 8);
    value[4] = (uint8) (param->bisSyncState >> 16);
    value[5] = (uint8) (param->bisSyncState >> 24);

    value[6] = param->metadataLen;

    if(param->metadataLen)
    {
        bassClientSwapByteTrasmissionOrder(param->metadataValue,
                                           param->metadataLen,
                                           &value[7]);
    }
}

/****************************************************************************/
void GattBassClientModifySourceRequest(ServiceHandle clntHndl,
                                       GattBassClientModifySourceParam *params,
                                       bool noResponse)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        size_t size = BASS_CLIENT_BROADCAST_MODIFY_SOURCE_CTRL_POINT_PARAM_SIZE_MIN + params->metadataLen;
        uint8* value = PanicUnlessMalloc(size);

        bassClientPrepareModifySourceOperationParameters(params, value);

        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Modify Source operation is size + 1, because we need to include the byte
         * for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_modify_source_op,
                                        size + 1,
                                        (uint16) size,
                                        value,
                                        noResponse);

        free(value);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
    }
}

/****************************************************************************/
void GattBassClientSetBroadcastCodeRequest(ServiceHandle clntHndl,
                                           uint8 sourceId,
                                           uint8 *broadcastCode,
                                           bool noResponse)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        size_t size = BASS_CLIENT_BROADCAST_CODE_SIZE + 1;
        uint8* value = PanicUnlessMalloc(size);

        value[0] = sourceId;

        bassClientSwapByteTrasmissionOrder(broadcastCode,
                                           BASS_CLIENT_BROADCAST_CODE_SIZE,
                                           &value[1]);

        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Set Broadacast Code operation is size + 1, because we need to include the byte
         * for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_set_broadcast_code_op,
                                        size + 1,
                                        (uint16) size,
                                        value,
                                        noResponse);

        free(value);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
    }
}

/****************************************************************************/
void GattBassClientRemoveSourceRequest(ServiceHandle clntHndl,
                                       uint8 sourceId,
                                       bool noResponse)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        uint8 value = sourceId;

        /* The length of the Broadcast Audio Scan Control Point characteristic in case
         * of Set Broadacast Code operation is BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE + 1,
         * because we need to include the byte for the opcode */
        bassClientHandleBroadcastAudioScanControlPointOperation(gatt_bass_client,
                                        bass_client_remove_source_op,
                                        BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE + 1,
                                        (uint16) BASS_CLIENT_BROADCAST_REMOVE_SOURCE_CTRL_POINT_PARAM_SIZE,
                                        &value,
                                        noResponse);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
    }
}
