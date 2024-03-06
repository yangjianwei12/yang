/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Implementation of the GENERIC Broadcast Scan Server volume module.
*/
#ifdef INCLUDE_GBSS

#include "generic_broadcast_scan_server_volume.h"
#include "csr_bt_gatt_client_util_lib.h"

static void genericBroadcastScanServer_OnVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume);

static const audio_source_observer_interface_t gbss_audio_source_observer_interface =
{
    .OnVolumeChange = genericBroadcastScanServer_OnVolumeChange
};

const audio_source_observer_interface_t *genericBroadcastScanServer_GetAudioSourceObserverInterface(void)
{
    return &gbss_audio_source_observer_interface;
}

void genericBroadcastScanServer_HandleReadGbssVolumeStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (connection)
    {
        uint8 config_data[GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE];

        config_data[0] = (uint8)connection->client_cfg.gbss_volume_state_ccc;
        config_data[1] = (uint8)(connection->client_cfg.gbss_volume_state_ccc >> 8);

        CsrBtGattDbReadAccessResSend(GBSS->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                     GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE,
                                     config_data);
    }
    else
    {

        CsrBtGattDbReadAccessResSend(GBSS->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     ATT_RESULT_INVALID_CID,
                                     GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE,
                                     NULL);
    }
}

void genericBroadcastScanServer_HandleWriteGbssVolumeStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    gbss_client_data_t *connectionCccData = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (!connectionCccData)
    {
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      ATT_RESULT_INVALID_CID);
        return;
    }

    if (!genericBroadcastScanServer_ValidateCccLen(GBSS, access_ind))
    {
        return;
    }

    /* Validate the input parameters - ONLY Notify*/
    if (access_ind->writeUnit[0].value[0] == CLIENT_CONFIG_NOTIFY ||
        access_ind->writeUnit[0].value[0] == CLIENT_CONFIG_NOT_SET)
    {
        /* If we are in here writeUnit[0].value[0] can only be : 0,1. No need to
        shift and read value[1] */
        connectionCccData->client_cfg.gbss_volume_state_ccc = (uint16)access_ind->writeUnit[0].value[0];

        /* Send response to the client */
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_SUCCESS);

        genericBroadcastScanServer_StoreClientConfig(access_ind->btConnId, &connectionCccData->client_cfg, sizeof(gbss_config_t));
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF);
    }
}

static uint8 *genericBroadcastScanServer_PrepareBroadcastVolumeStateValue(gbss_volume_data_t *gbss_volume_data, uint8 *len)
{
    GattBuffIterator iter;
    uint16 size;
    uint8 *report_value;

    size = sizeof(gbss_volume_data->volume_setting) + sizeof(gbss_volume_data->change_counter) + sizeof(gbss_volume_data->mute); /* calculate constant size*/


    report_value = CsrPmemZalloc(size);
    GattBuffIteratorInitialise(&iter, report_value, size);

    GattBuffIteratorWrite8(&iter, gbss_volume_data->volume_setting);
    GattBuffIteratorWrite8(&iter, gbss_volume_data->mute);
    GattBuffIteratorWrite8(&iter, gbss_volume_data->change_counter);

    /* Add the parameters based on opcode*/
    if (iter.error == TRUE)
    {
        free(report_value);
        report_value = NULL;
        *len = 0;
    }
    else
    {
        *len = size;
    }

    return report_value;
}


void genericBroadcastScanServer_HandleReadGbssVolumeState(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    uint8 report_size;
    uint8 *report_value;
    report_value = genericBroadcastScanServer_PrepareBroadcastVolumeStateValue(&GBSS->gbss_volume_data, &report_size);

    if (report_value)
    {
        CsrBtGattDbReadAccessResSend(GBSS->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                     report_size,
                                     report_value);
    }
    else
    {
        CsrBtGattDbReadAccessResSend(GBSS->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR,
                                     0,
                                     NULL);
    }
}


void GenericBroadcastScanServer_NotifyGbssVolumeState(void)
{
    uint8 client_index = 0;
    gbss_srv_data_t* gbss_server = genericBroadcastScanServer_GetInstance();

   	DEBUG_LOG("GenericBroadcastScanServer_NotifyGbssVolumeState: volume:0x%x and change_counter:0x%x",
               gbss_server->gbss_volume_data.volume_setting, gbss_server->gbss_volume_data.change_counter);

    for (client_index = 0; client_index < MAX_CONNECTIONS; client_index++)
    {
        if ((gbss_server->connected_clients[client_index].cid != 0) &&
            (gbss_server->connected_clients[client_index].client_cfg.gbss_volume_state_ccc == CLIENT_CONFIG_NOTIFY))
        {
            int16 handle = HANDLE_GENERIC_BROADCAST_VOLUME_STATE - HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + 1;
            uint8 report_size;
            uint8 *report_value;
            report_value = genericBroadcastScanServer_PrepareBroadcastVolumeStateValue(&gbss_server->gbss_volume_data, &report_size);

            if (report_value)
            {
                CsrBtGattNotificationEventReqSend(gbss_server->gattId,
                                                  gbss_server->connected_clients[client_index].cid,
                                                  handle,
                                                  report_size,
                                                  report_value);
            }
            else
            {
                DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyGbssVolumeState: Failed to generate notification report\n");
            }
        }
    }
}

static void genericBroadcastScanServer_OnVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume)
{
    UNUSED(origin);
    gbss_srv_data_t* gbss_server = genericBroadcastScanServer_GetInstance();

    DEBUG_LOG("genericBroadcastScanServer_OnVolumeChange source enum:audio_source_t:%u origin enum:event_origin_t:%u volume %u",
              source, origin, volume.value);

    if ( source == audio_source_le_audio_broadcast)
    {
        gbss_server->gbss_volume_data.volume_setting = volume.value;
        gbss_server->gbss_volume_data.change_counter++;

        GenericBroadcastScanServer_NotifyGbssVolumeState();
    }
}

static uint16 genericBroadcastScanServer_HandleRelativeVolumeDown(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 change_counter;
    uint8 new_volume_setting = 0;

    DEBUG_LOG("genericBroadcastScanServer_HandleRelativeVolumeDown\n");
    GattAccessIndIteratorInitialise(&iter, access_ind);

    GattAccessIndIteratorRead8(&iter); /* skip over the op code */

    change_counter = GattAccessIndIteratorRead8(&iter);

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleRelativeVolumeDown: Invalid length\n");
    }

    if (change_counter == GBSS->gbss_volume_data.change_counter)
    {
        /* Decrease the value of the Volume Setting by Step Size */
        if (GBSS->gbss_volume_data.volume_setting >= GBSS->gbss_volume_data.step_size)
        {
            new_volume_setting = GBSS->gbss_volume_data.volume_setting - GBSS->gbss_volume_data.step_size;
        }

        Volume_SendAudioSourceVolumeUpdateRequest(audio_source_le_audio_broadcast, event_origin_external, new_volume_setting);
    }
    else
    {
        result = GENERIC_BROADCAST_SCAN_SERVER_ERR_INVALID_CHANGE_COUNTER;
    }

    return result;
}

static uint16 genericBroadcastScanServer_HandleRelativeVolumeUp(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 change_counter;
    uint8 new_volume_setting = GENERIC_BROADCAST_SCAN_SERVER_MAX_VOLUME_SETTING_VALUE;

    DEBUG_LOG("genericBroadcastScanServer_HandleRelativeVolumeUp\n");
    GattAccessIndIteratorInitialise(&iter, access_ind);

    GattAccessIndIteratorRead8(&iter); /* skip over the op code */

    change_counter = GattAccessIndIteratorRead8(&iter);

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleRelativeVolumeUp: Invalid length\n");
    }

    if (change_counter == GBSS->gbss_volume_data.change_counter)
    {
        /* Increase the value of the Volume Setting by Step Size */
        if (GBSS->gbss_volume_data.step_size < (GENERIC_BROADCAST_SCAN_SERVER_MAX_VOLUME_SETTING_VALUE - GBSS->gbss_volume_data.volume_setting) )
        {
            new_volume_setting = GBSS->gbss_volume_data.volume_setting + GBSS->gbss_volume_data.step_size;
        }

        Volume_SendAudioSourceVolumeUpdateRequest(audio_source_le_audio_broadcast, event_origin_external, new_volume_setting);
    }
    else
    {
        result = GENERIC_BROADCAST_SCAN_SERVER_ERR_INVALID_CHANGE_COUNTER;
    }

    return result;
}

static uint16 genericBroadcastScanServer_HandleSetAbsoluteVolume(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 change_counter;
    uint8 new_volume_setting = 0;

    DEBUG_LOG("genericBroadcastScanServer_HandleSetAbsoluteVolume\n");
    GattAccessIndIteratorInitialise(&iter, access_ind);

    GattAccessIndIteratorRead8(&iter); /* skip over the op code */

    change_counter = GattAccessIndIteratorRead8(&iter);
    new_volume_setting = GattAccessIndIteratorRead8(&iter);

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleSetAbsoluteVolume: Invalid length\n");
    }

    if (change_counter == GBSS->gbss_volume_data.change_counter)
    {
        /* The absolute volume to set is different from the actual volume */
        if (new_volume_setting != GBSS->gbss_volume_data.volume_setting)
        {
            Volume_SendAudioSourceVolumeUpdateRequest(audio_source_le_audio_broadcast, event_origin_external, new_volume_setting);
        }
    }
    else
    {
        result = GENERIC_BROADCAST_SCAN_SERVER_ERR_INVALID_CHANGE_COUNTER;
    }

    return result;
}

void genericBroadcastScanServer_HandleWriteGbssVolumeControlPoint(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint8 opcode = 0x00;
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;

    if (access_ind->writeUnit != NULL)
    {
        opcode = access_ind->writeUnit[0].value[0];
        DEBUG_LOG("genericBroadcastScanServer_HandleWriteGbssVolumeControlPoint: Generic Broadcast Volume Control Point Characteristic Opcode: 0x%x\n",
                  opcode);

        switch (opcode)
        {
            case RELATIVE_VOLUME_DOWN_OPCODE:
            {
                result = genericBroadcastScanServer_HandleRelativeVolumeDown(GBSS, access_ind);
            }
            break;

            case RELATIVE_VOLUME_UP_OPCODE:
            {
                result = genericBroadcastScanServer_HandleRelativeVolumeUp(GBSS, access_ind);
            }
            break;

            case SET_ABSOLUTE_VOLUME_OPCODE:
            {
                result = genericBroadcastScanServer_HandleSetAbsoluteVolume(GBSS, access_ind);
            }
            break;

            default:
            {
                result = GENERIC_BROADCAST_SCAN_SERVER_ERR_OPCODE_NOT_SUPPORTED;
                DEBUG_LOG("genericBroadcastScanServer_HandleWriteGbssVolumeControlPoint : Invalid opcode\n");
            }
            break;
        }
    }
    else
    {
        result = CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED;
    }

    CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                  access_ind->btConnId,
                                  access_ind->attrHandle,
                                  result);
}
#endif /* INCLUDE_GBSS */
