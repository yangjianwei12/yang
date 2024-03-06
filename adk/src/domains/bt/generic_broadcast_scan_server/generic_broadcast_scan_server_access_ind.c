/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \ingroup    generic_broadcast_scan_server
    \brief      Implementation of the GENERIC Broadcast Scan Server module.
*/
#ifdef INCLUDE_GBSS

#include "generic_broadcast_scan_server_access_ind.h"
#include "generic_broadcast_scan_server_volume.h"
#include "le_broadcast_manager.h"
#include "le_broadcast_manager_self_scan.h"
#include "csr_bt_gatt_client_util_lib.h"
/* TBD broadcast maanger API */

#define BAD_CODE_LEN (16)
#define BROADCAST_CODE_LEN (16)

#define GbssBuffIteratorErrorDetected(iter)     ((iter)->error)

static bool genericBroadcastScanServer_BroadcastReceiverStateValidIdx(uint16 idx)
{

    if (idx > GBSS_RECEIVER_STATE_IDX_MAX)
    {
        DEBUG_LOG("genericBroadcastScanServer_BroadcastReceiverStateIdxFromHandle : Incorect handle");
        return FALSE;
    }

    return TRUE;
}

static void genericBroadcastScanServer_HandleReadGbssScanControlPointCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (connection)
    {
        uint8 config_data[GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE];

        config_data[0] = (uint8)connection->client_cfg.gbss_scan_cp_ccc;
        config_data[1] = (uint8)(connection->client_cfg.gbss_scan_cp_ccc >> 8);

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

static void genericBroadcastScanServer_HandleReadGbssReportCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (connection)
    {
        uint8 config_data[GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE];

        config_data[0] = (uint8)connection->client_cfg.gbss_scan_ccc;
        config_data[1] = (uint8)(connection->client_cfg.gbss_scan_ccc >> 8);

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

static void genericBroadcastScanServer_HandleReadGbssReceiverStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind, uint16 idx)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (connection && !genericBroadcastScanServer_BroadcastReceiverStateValidIdx(idx))
    {
        uint8 config_data[GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE];
        config_data[0] = (uint8)connection->client_cfg.gbss_rcv_state_ccc[idx];
        config_data[1] = (uint8)(connection->client_cfg.gbss_rcv_state_ccc[idx] >> 8);

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

static void genericBroadcastScanServer_HandleReadGbssScanControlPoint(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    uint8 report_size = 0;
    uint8 *report_value = NULL;
    gbss_client_data_t *connection = NULL;

    connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (connection)
        report_value = genericBroadcastScanServer_PrepareBroadcastScanControlPointValue(&connection->scan_cp_response, &report_size);

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

static void genericBroadcastScanServer_HandleReadGbssReport(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    uint8 report_size;
    uint8 *report_value;
    report_value = genericBroadcastScanServer_PrepareBroadcastScanReportValue(&GBSS->gbss_report, &report_size);

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

static void genericBroadcastScanServer_HandleReadGbssReceiverState(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind, uint16 idx)
{
    uint8 report_size;
    uint8 *report_value;
    report_value = genericBroadcastScanServer_PrepareBroadcastReceiverStateValue( &report_size, (uint8) idx);

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
    {/* Characteristic is empty */
        CsrBtGattDbReadAccessResSend(GBSS->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                     0,
                                     NULL);
    }
}

bool genericBroadcastScanServer_ValidateCccLen(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 size_value = 0;

    if (!access_ind->writeUnitCount)
    {
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
        return FALSE;
    }

    for (uint8 unit_index = 0; unit_index < access_ind->writeUnitCount; unit_index++)
        size_value += access_ind->writeUnit[unit_index].valueLength;
    if (size_value != GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE)
    {
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
        return FALSE;
    }

    return TRUE;
}

static void genericBroadcastScanServer_HandleWriteGbssReceiverStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind, uint16 idx)
{
    gbss_client_data_t *connectionCccData = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if(!genericBroadcastScanServer_BroadcastReceiverStateValidIdx(idx))
    {
        return;
    }

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
        connectionCccData->client_cfg.gbss_rcv_state_ccc[idx] = (uint16)access_ind->writeUnit[0].value[0];

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

static void genericBroadcastScanServer_HandleWriteGbssReportCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
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
        connectionCccData->client_cfg.gbss_scan_ccc = (uint16)access_ind->writeUnit[0].value[0];

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

static void genericBroadcastScanServer_HandleWriteGbssControlPointCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
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
        connectionCccData->client_cfg.gbss_scan_cp_ccc = (uint16)access_ind->writeUnit[0].value[0];

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

static uint16 genericBroadcastScanServer_HandleStartScan(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    self_scan_params_t params = {0};
    GattAccessIndIteratorInitialise(&iter, access_ind);

    GattAccessIndIteratorRead8(&iter); /* skip over the op code */

    params.timeout = (uint32) GattAccessIndIteratorRead16(&iter);
    params.sync_to_pa = GattAccessIndIteratorRead8(&iter);

    if (GattAccessIndIteratorRead8(&iter)) /*check if there is a filter set*/
    {
        /* TODO: parse the filter. Phase 3*/
        DEBUG_LOG("filter is set\n");
    }

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleStartScan Start Scanning operation: invalid length\n");
    }
    else
    {
        LeBroadcastManager_SelfScanStart(&GBSS->gbss_task, &params);
    }

    return result;
}

static uint16 genericBroadcastScanServer_HandleStopScan(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    UNUSED(access_ind);

    LeBroadcastManager_SelfScanStop(&GBSS->gbss_task);

    return result;
}

static void genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus(le_bm_bass_status_t bm_bass_status, gbss_bass_status_t *bass_status)
{
    switch(bm_bass_status)
    {
        case le_bm_bass_status_success:
        {
            *bass_status = GBSS_BASS_STATUS_SUCCESS;
        }
        break;

        case le_bm_bass_status_in_progress:
        {
            *bass_status = GBSS_BASS_STATUS_IN_PROGRESS;
        }
        break;

        case le_bm_bass_status_invalid_parameter:
        {
            *bass_status = GBSS_BASS_STATUS_INVALID_PARAMETER;
        }
        break;

        case le_bm_bass_status_not_allowed:
        {
            *bass_status = GBSS_BASS_STATUS_NOT_ALLOWED;
        }
        break;

        case le_bm_bass_status_failed:
        {
            *bass_status = GBSS_BASS_STATUS_FAILED;
        }
        break;

        case le_bm_bass_status_bc_source_in_sync:
        {
            *bass_status = GBSS_BASS_STATUS_BC_SOURCE_IN_SYNC;
        }
        break;

        case le_bm_bass_status_invalid_source_id:
        {
            *bass_status = GBSS_BASS_STATUS_INVALID_SOURCE_ID;
        }
        break;

        default:
            DEBUG_LOG_ERROR("genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus: staus:%d not mapped", bm_bass_status);
        break;
    }
}

static uint16 genericBroadcastScanServer_HandleAddSource(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind, gbss_client_data_t *connection)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 source_id = 0;
    le_bm_add_source_info_t source = {0};
    le_bm_bass_status_t bm_bass_status;
    GattAccessIndIteratorInitialise(&iter, access_ind);
    gbss_bass_status_t bass_status = GBSS_BASS_STATUS_SUCCESS;
    UNUSED(GBSS);

    GattAccessIndIteratorRead8(&iter); /* skip over the opcode */

    source.advertiser_address.type = GattAccessIndIteratorRead8(&iter);
    source.advertiser_address.addr.lap = (uint32)GattAccessIndIteratorRead24(&iter);
    source.advertiser_address.addr.uap = GattAccessIndIteratorRead8(&iter);
    source.advertiser_address.addr.nap = GattAccessIndIteratorRead16(&iter);
    source.advertising_sid = GattAccessIndIteratorRead8(&iter);
    source.broadcast_id = GattAccessIndIteratorRead24(&iter);
    source.pa_sync = GattAccessIndIteratorRead8(&iter);
    source.pa_interval = GattAccessIndIteratorRead16(&iter);
    source.num_subgroups = GattAccessIndIteratorRead8(&iter);
    source.subgroups = CsrPmemZalloc(source.num_subgroups * sizeof(le_bm_source_subgroup_t));
    for (uint8 subgroup = 0; subgroup < source.num_subgroups; subgroup++)
    {
        source.subgroups[subgroup].bis_sync = GattAccessIndIteratorRead32(&iter);
        source.subgroups[subgroup].metadata_length = GattAccessIndIteratorRead8(&iter);
        source.subgroups[subgroup].metadata = GattAccessIndIteratorReadMultipleOctets(&iter, source.subgroups[subgroup].metadata_length);
    }

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleAddSource: Add source: invalid length\n");
    }
    else
    {
        bm_bass_status = LeBroadcastManager_BassAddSource(&source_id, &source);
        genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus(bm_bass_status, &bass_status);
        if (connection)
        {
            connection->scan_cp_response.opcode = ADD_SOURCE_OPCODE;
            connection->scan_cp_response.status_code = bass_status;
            connection->scan_cp_response.param_len = 0x03;
            connection->scan_cp_response.params.broadcast_id = source.broadcast_id;
        }

        if(bass_status != GBSS_BASS_STATUS_SUCCESS)
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleAddSource: Failed to add source\n");
        }
    }

    for (uint8 subgroup = 0; subgroup < source.num_subgroups; subgroup++)
    {
        free(source.subgroups[subgroup].metadata);
    }
    free(source.subgroups);

    return result;
}

static uint16 genericBroadcastScanServer_HandleRemoveSource(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind, gbss_client_data_t *connection)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 source_id = 0;
    le_bm_bass_status_t bm_bass_status;
    gbss_bass_status_t bass_status = GBSS_BASS_STATUS_SUCCESS;

    GattAccessIndIteratorInitialise(&iter, access_ind);
    UNUSED(GBSS);

    GattAccessIndIteratorRead8(&iter); /* skip over the opcode */
    source_id = GattAccessIndIteratorRead8(&iter);

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleRemoveSource Remove Source: invalid length\n");
    }
    else
    {

        bm_bass_status = LeBroadcastManager_BassRemoveSource(source_id);
        genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus(bm_bass_status, &bass_status);
        if (connection)
        {
            connection->scan_cp_response.opcode = REMOVE_SOURCE_OPCODE;
            connection->scan_cp_response.status_code = bass_status;
            connection->scan_cp_response.param_len = 0x01;
            connection->scan_cp_response.params.source_id = source_id;
        }

        if(bass_status != GBSS_BASS_STATUS_SUCCESS)
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleRemoveSource: Failed to remove source\n");
        }
    }

    return result;
}

static uint16 genericBroadcastScanServer_HandleModifySource(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind, gbss_client_data_t *connection)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 source_id = 0;
    le_bm_modify_source_info_t modify_source_info = {0};
    le_bm_bass_status_t bm_bass_status;
    gbss_bass_status_t bass_status = GBSS_BASS_STATUS_SUCCESS;

    GattAccessIndIteratorInitialise(&iter, access_ind);
    UNUSED(GBSS);

    GattAccessIndIteratorRead8(&iter); /* skip over the opcode */
    source_id = GattAccessIndIteratorRead8(&iter);

    modify_source_info.pa_sync = GattAccessIndIteratorRead8(&iter);
    modify_source_info.pa_interval = GattAccessIndIteratorRead16(&iter);
    modify_source_info.num_subgroups = GattAccessIndIteratorRead8(&iter);
    modify_source_info.subgroups = CsrPmemZalloc(modify_source_info.num_subgroups * sizeof(le_bm_source_subgroup_t));

    for (uint8 subgroup = 0; subgroup < modify_source_info.num_subgroups; subgroup++)
    {
        modify_source_info.subgroups[subgroup].bis_sync = GattAccessIndIteratorRead32(&iter);
        modify_source_info.subgroups[subgroup].metadata_length = GattAccessIndIteratorRead8(&iter);
        modify_source_info.subgroups[subgroup].metadata = GattAccessIndIteratorReadMultipleOctets(&iter, modify_source_info.subgroups[subgroup].metadata_length);
    }

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleModifySource Remove Source: invalid length\n");
    }
    else
    {
        bm_bass_status = LeBroadcastManager_BassModifySource(source_id, &modify_source_info);
        genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus(bm_bass_status, &bass_status);
        if (connection)
        {
            connection->scan_cp_response.opcode = MODIFY_SOURCE_OPCODE;
            connection->scan_cp_response.status_code = bass_status;
            connection->scan_cp_response.param_len = 0x04;
            connection->scan_cp_response.params.source_id = source_id;
        }

        if(bass_status != GBSS_BASS_STATUS_SUCCESS)
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleModifySource: Failed to modify source\n");
        }
    }

    for (uint8 subgroup = 0; subgroup < modify_source_info.num_subgroups; subgroup++)
    {
        free(modify_source_info.subgroups[subgroup].metadata);
    }
    free(modify_source_info.subgroups);

    return result;
}

static uint16 genericBroadcastScanServer_HandleSetBroadcastCode(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind, gbss_client_data_t *connection)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    GattAccessWriteIndIterator iter;
    uint8 source_id = 0;
    le_bm_bass_status_t bm_bass_status;
    gbss_bass_status_t bass_status = GBSS_BASS_STATUS_SUCCESS;
    uint8 *code;

    GattAccessIndIteratorInitialise(&iter, access_ind);
    UNUSED(GBSS);

    GattAccessIndIteratorRead8(&iter); /* skip over the opcode */
    source_id = GattAccessIndIteratorRead8(&iter);
    code = GattAccessIndIteratorReadMultipleOctets(&iter, BROADCAST_CODE_LEN);

    if (iter.error == TRUE) /*we atempted to read more than the contents of access ind*/
    {
        result = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleSetBroadcastCode add code invalid length\n");
    }
    else
    {
        bm_bass_status = LeBroadcastManager_BassSetBroadcastCode(source_id, code);
        genericBroadcastScanServer_ConvertLeBroadcastManagerBassStatus(bm_bass_status, &bass_status);
        if (connection)
        {
            connection->scan_cp_response.opcode = SET_BROADCAST_CODE_OPCODE;
            connection->scan_cp_response.status_code = bass_status;
            connection->scan_cp_response.param_len = 0x01;
            connection->scan_cp_response.params.source_id = source_id;
        }

        if(bass_status != GBSS_BASS_STATUS_SUCCESS)
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_HandleSetBroadcastCode failed to add code\n %d %p", source_id, code);
        }
    }

    free(code);
    return result;
}

static void genericBroadcastScanServer_HandleWriteGbssControlPoint(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint8 opcode = 0x00;
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(access_ind->btConnId);

    if (access_ind->writeUnit != NULL)
    {
        opcode = access_ind->writeUnit[0].value[0];
        DEBUG_LOG("genericBroadcastScanServer_HandleWriteGbssControlPoint: Generic Broadcast Scan Control Point Characteristic Opcode: 0x%x\n",
                  opcode);

        switch (opcode)
        {
            case START_SCANNING_OPCODE:
            {
                result = genericBroadcastScanServer_HandleStartScan(GBSS, access_ind);
            }
            break;

            case STOP_SCANNING_OPCODE:
            {
                result = genericBroadcastScanServer_HandleStopScan(GBSS, access_ind);
            }
            break;

            case ADD_SOURCE_OPCODE:
            {
                result = genericBroadcastScanServer_HandleAddSource(GBSS, access_ind, connection);
            }
            break;

            case MODIFY_SOURCE_OPCODE:
            {
                result = genericBroadcastScanServer_HandleModifySource(GBSS, access_ind, connection);
            }
            break;

            case SET_BROADCAST_CODE_OPCODE:
            {
                result = genericBroadcastScanServer_HandleSetBroadcastCode(GBSS, access_ind, connection);
            }
            break;

            case REMOVE_SOURCE_OPCODE:
            {
                result = genericBroadcastScanServer_HandleRemoveSource(GBSS, access_ind, connection);
            }
            break;

            case RESET_OPCODE:
            default:
            {
                result = CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED;
                DEBUG_LOG("DEBUG_LOG : Error - Broadcast Audio Scan Control Point: invalid opcode\n");
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

    if ((opcode == ADD_SOURCE_OPCODE) ||
        (opcode == MODIFY_SOURCE_OPCODE) ||
        (opcode == SET_BROADCAST_CODE_OPCODE) ||
        (opcode == REMOVE_SOURCE_OPCODE))
    {
        GenericBroadcastScanServer_NotifyGbssScanControlPointResponse(access_ind->btConnId);
    }
}

uint8 *genericBroadcastScanServer_PrepareBroadcastScanControlPointValue(gbss_scan_control_point_response_t *scan_cp_response, uint8 *len)
{
    GattBuffIterator iter;
    uint16 size;
    uint8 *report_value;

    if(scan_cp_response->param_len == 0x00)
    {
        *len = 0;
        return NULL;
    }

    size = sizeof(scan_cp_response->opcode) + sizeof(scan_cp_response->status_code) + sizeof(scan_cp_response->param_len) + scan_cp_response->param_len; /* calculate constant size*/


    report_value = CsrPmemZalloc(size);
    GattBuffIteratorInitialise(&iter, report_value, size);

    GattBuffIteratorWrite8(&iter, scan_cp_response->opcode);
    GattBuffIteratorWrite16(&iter, scan_cp_response->status_code);
    GattBuffIteratorWrite8(&iter, scan_cp_response->param_len);

    /* Add the parameters based on opcode*/
    switch (scan_cp_response->opcode)
    {
        case START_SCANNING_OPCODE:
        case STOP_SCANNING_OPCODE:
        {
            /* Nothing to add here */
        }
        break;

        case ADD_SOURCE_OPCODE:
        {
            GattBuffIteratorWrite24(&iter, scan_cp_response->params.broadcast_id);
        }
        break;

        case MODIFY_SOURCE_OPCODE:
        {
            GattBuffIteratorWrite8(&iter, scan_cp_response->params.source_id);
            GattBuffIteratorWrite24(&iter, scan_cp_response->params.broadcast_id);
        }
        break;

        case SET_BROADCAST_CODE_OPCODE:
        case REMOVE_SOURCE_OPCODE:
        {
            GattBuffIteratorWrite8(&iter, scan_cp_response->params.source_id);
        }
        break;

        default:
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_PrepareBroadcastScanControlPointValue: Invalid opcode:%d", scan_cp_response->opcode);
        }
        break;
    }

    if (GbssBuffIteratorErrorDetected(&iter))
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

uint8 *genericBroadcastScanServer_PrepareBroadcastScanReportValue(gbss_scan_report_t *gbss_report, uint8 *len)
{
    GattBuffIterator iter;
    uint16 size;
    uint8 *report_value;

    size = sizeof(gbss_scan_report_t) + gbss_report->broadcast_name_len + (gbss_report->num_subgroups * sizeof(gbss_subgroups_t)); /* calculate main struct size*/
    for (uint8 subgroup = 0; subgroup < gbss_report->num_subgroups; subgroup++)
    {
        size += gbss_report->subgroups[subgroup].metadata_length; /* add size for each subgroup metadata*/
    }

    /*  gbss_scan_active_t scan_active;
        gbss_self_scan_status_t scan_result;
        uint8 encrypted;
        int8 rssi;
        uint8 broadcast_name_len;
        uint8 *broadcast_name;
        typed_bdaddr source_address;
        uint8 source_adv_sid;
        uint32 broadcast_id;
        uint16 pa_interval;
        uint8 num_subgroups;
        gbss_subgroups_t * subgroups;*/

    report_value = CsrPmemZalloc(size);
    GattBuffIteratorInitialise(&iter, report_value, size);

    GattBuffIteratorWrite8(&iter, (uint8)gbss_report->scan_active);
    GattBuffIteratorWrite8(&iter, (uint8)gbss_report->scan_result);
    GattBuffIteratorWrite8(&iter, gbss_report->encryption_required);
    GattBuffIteratorWrite8(&iter, gbss_report->rssi);
    GattBuffIteratorWrite8(&iter, gbss_report->broadcast_name_len);
    GattBuffIteratorWriteMultipleOctets(&iter, gbss_report->broadcast_name, gbss_report->broadcast_name_len);
    GattBuffIteratorWrite8(&iter, gbss_report->source_address.type);
    GattBuffIteratorWrite24(&iter, gbss_report->source_address.addr.lap);
    GattBuffIteratorWrite8(&iter, gbss_report->source_address.addr.uap);
    GattBuffIteratorWrite16(&iter, gbss_report->source_address.addr.nap);
    GattBuffIteratorWrite8(&iter, gbss_report->source_adv_sid);
    GattBuffIteratorWrite24(&iter, gbss_report->broadcast_id);
    GattBuffIteratorWrite16(&iter, gbss_report->pa_interval);
    GattBuffIteratorWrite8(&iter, gbss_report->num_subgroups);

    for (uint8 subgroup = 0; subgroup < gbss_report->num_subgroups; subgroup++)
    {
        GattBuffIteratorWrite8(&iter, gbss_report->subgroups[subgroup].metadata_length);
        GattBuffIteratorWriteMultipleOctets(&iter, gbss_report->subgroups[subgroup].metadata, gbss_report->subgroups[subgroup].metadata_length);
    }

    if (GbssBuffIteratorErrorDetected(&iter))
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

uint8 *genericBroadcastScanServer_PrepareBroadcastReceiverStateValue(uint8 *len, uint8 idx)
{
    GattBuffIterator iter;
    uint16 size;
    uint16 bad_code_size =0;
    uint8 *report_value;
    scan_delegator_server_get_broadcast_source_state_t source_state;
#ifdef ENABLE_RDP_DEMO
    gbss_srv_data_t *gbss_inst = genericBroadcastScanServer_GetInstance();
#endif

    if(LeBroadcastManager_BassGetBroadcastSourceState(RECEIVER_STATE_IDX_TO_SOURCE_ID(idx), &source_state) != le_bm_bass_status_success)
    {
        *len = 0;
        return NULL;
    }

    if(source_state.big_encryption == GATT_BASS_BAD_CODE)
    {
      bad_code_size = BAD_CODE_LEN;
    }

    size = sizeof(scan_delegator_server_get_broadcast_source_state_t) + bad_code_size + (source_state.num_subgroups * sizeof(le_bm_source_subgroup_t)); /* calculate main struct size*/
    for (uint8 subgroup = 0; subgroup < source_state.num_subgroups; subgroup++)
    {
        size += source_state.subgroups[subgroup].metadata_length; /* add size for each subgroup metadata*/
    }
#ifdef ENABLE_RDP_DEMO
    size += sizeof(gbss_inst->src_state_ntf_counter);
#endif

    report_value = CsrPmemZalloc(size);
    GattBuffIteratorInitialise(&iter, report_value, size);

    GattBuffIteratorWrite8(&iter, RECEIVER_STATE_IDX_TO_SOURCE_ID(idx));
    GattBuffIteratorWrite8(&iter, source_state.source_address.type);
    GattBuffIteratorWrite24(&iter, source_state.source_address.addr.lap);
    GattBuffIteratorWrite8(&iter, source_state.source_address.addr.uap);
    GattBuffIteratorWrite16(&iter, source_state.source_address.addr.nap);
    GattBuffIteratorWrite8(&iter, source_state.source_adv_sid);
    GattBuffIteratorWrite24(&iter, source_state.broadcast_id);
    GattBuffIteratorWrite8(&iter, source_state.pa_sync_state);
    GattBuffIteratorWrite8(&iter, source_state.big_encryption);
    GattBuffIteratorWriteMultipleOctets(&iter, source_state.bad_code, bad_code_size);
    GattBuffIteratorWrite8(&iter, source_state.num_subgroups);

    for (uint8 subgroup = 0; subgroup < source_state.num_subgroups; subgroup++)
    {
        GattBuffIteratorWrite32(&iter, source_state.subgroups[subgroup].bis_sync);
        GattBuffIteratorWrite8(&iter, source_state.subgroups[subgroup].metadata_length);
        GattBuffIteratorWriteMultipleOctets(&iter, source_state.subgroups[subgroup].metadata, source_state.subgroups[subgroup].metadata_length);
    }

#ifdef ENABLE_RDP_DEMO
    GattBuffIteratorWrite8(&iter, gbss_inst->src_state_ntf_counter);
#endif

    if (GbssBuffIteratorErrorDetected(&iter))
    {
        free(report_value);
        report_value = NULL;
        *len = 0;
    }
    else
    {
        *len = size;
    }

    LeBroadcastManager_BassFreeBroadcastSourceState(&source_state);
    return report_value;
}

void genericBroadcastScanServer_HandleWriteAccessInd(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind)
{
    DEBUG_LOG("genericBroadcastScanServer_HandleWriteAccessInd: handle %d", access_ind->attrHandle);
    uint16 handle = HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + (access_ind->attrHandle - 1);

    if (handle == HANDLE_GENERIC_BROADCAST_SCAN_REPORT_CLIENT_CONFIG)
    {
        genericBroadcastScanServer_HandleWriteGbssReportCcc(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_SCAN_CONTROL_POINT)
    {
        genericBroadcastScanServer_HandleWriteGbssControlPoint(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_SCAN_CONTROL_POINT_CLIENT_CONFIG)
    {
        genericBroadcastScanServer_HandleWriteGbssControlPointCcc(GBSS, access_ind);
    }
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1)
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_1)
    {
           genericBroadcastScanServer_HandleWriteGbssReceiverStateCcc(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_1);
    }
#endif
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2)
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_2)
    {
           genericBroadcastScanServer_HandleWriteGbssReceiverStateCcc(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_2);
    }
#endif
    else if (handle == HANDLE_GENERIC_BROADCAST_VOLUME_STATE_CLIENT_CONFIG)
    {
        genericBroadcastScanServer_HandleWriteGbssVolumeStateCcc(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_VOLUME_CONTROL_POINT)
    {
        genericBroadcastScanServer_HandleWriteGbssVolumeControlPoint(GBSS, access_ind);
    }
    else
    {
        CsrBtGattDbWriteAccessResSend(GBSS->gattId,
                                      access_ind->btConnId,
                                      access_ind->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
    }
}

void genericBroadcastScanServer_HandleReadAccessInd(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind)
{
    DEBUG_LOG("genericBroadcastScanServer_HandleReadAccessInd: handle %d", access_ind->attrHandle);
    uint16 handle = HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + (access_ind->attrHandle - 1);

    if (handle == HANDLE_GENERIC_BROADCAST_SCAN_CONTROL_POINT_CLIENT_CONFIG)
    {
            genericBroadcastScanServer_HandleReadGbssScanControlPointCcc(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_SCAN_CONTROL_POINT)
    {
            genericBroadcastScanServer_HandleReadGbssScanControlPoint(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_SCAN_REPORT_CLIENT_CONFIG)
    {
            genericBroadcastScanServer_HandleReadGbssReportCcc(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_SCAN_REPORT)
    {
            genericBroadcastScanServer_HandleReadGbssReport(GBSS, access_ind);
    }
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1)
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_1)
    {
        genericBroadcastScanServer_HandleReadGbssReceiverStateCcc(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_1);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1)
    {
        genericBroadcastScanServer_HandleReadGbssReceiverState(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_1);
    }
#endif
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2)
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_2)
    {
        genericBroadcastScanServer_HandleReadGbssReceiverStateCcc(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_2);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2)
    {
        genericBroadcastScanServer_HandleReadGbssReceiverState(GBSS, access_ind, GBSS_RECEIVER_STATE_IDX_2);
    }
#endif
    else if (handle == HANDLE_GENERIC_BROADCAST_VOLUME_STATE)
    {
            genericBroadcastScanServer_HandleReadGbssVolumeState(GBSS, access_ind);
    }
    else if (handle == HANDLE_GENERIC_BROADCAST_VOLUME_STATE_CLIENT_CONFIG)
    {
            genericBroadcastScanServer_HandleReadGbssVolumeStateCcc(GBSS, access_ind);
    }
    else
    {
            GattDbReadAccessResSend(GBSS->gattId,
                                    access_ind->btConnId,
                                    access_ind->attrHandle,
                                    CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE,
                                    0,
                                    NULL);
    }
}
#endif /* INCLUDE_GBSS */
