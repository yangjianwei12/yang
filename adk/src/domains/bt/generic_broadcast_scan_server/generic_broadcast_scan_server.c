/*!
    \copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \ingroup    generic_broadcast_scan_server
    \brief      Implementation of the GENERIC Broadcast Scan Server module.
*/
#ifdef INCLUDE_GBSS

#include "generic_broadcast_scan_server.h"
#include "generic_broadcast_scan_server_msg_handler.h"
#include "generic_broadcast_scan_server_access_ind.h"
#include "generic_broadcast_scan_server_volume.h"

#include "le_broadcast_manager.h"
#include "le_broadcast_manager_self_scan.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "pddu_map.h"

#include <stdio.h>

static gbss_srv_data_t gbss_server= {0};

static le_broadcast_manager_source_state_client_if_t notify_if =
{
    .NotifySourceStateChanged = GenericBroadcastScanServer_NotifyReceiverState
};

/*! Get pointer to the main GENERIC Broadcast Scan server Task */
#define GbssTask() (&gbss_server.gbss_task)

static void genericBroadcastScanServer_OnGattConnect(gatt_cid_t cid);
static void genericBroadcastScanServer_OnGattDisconnect(gatt_cid_t cid);
static void genericBroadcastScanServer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static const gatt_connect_observer_callback_t le_connect_callbacks =
{
    .OnConnection = genericBroadcastScanServer_OnGattConnect,
    .OnDisconnection = genericBroadcastScanServer_OnGattDisconnect,
    .OnEncryptionChanged = genericBroadcastScanServer_OnEncryptionChanged
};

static bool genericBroadcastScanServer_IsValidCcc(const gbss_config_t *config)
{
    if (config->gbss_scan_ccc >= CLIENT_CONFIG_INDICATE)
    {
        return FALSE;
    }

    for (int rsc = 0; rsc < GBSS_RECEIVER_STATE_IDX_MAX; rsc++)
    {
        if (config->gbss_rcv_state_ccc[rsc] >= CLIENT_CONFIG_INDICATE)
        {
            return FALSE;
        }
    }

    return TRUE;
}

gbss_client_data_t *genericBroadcastScanServer_AddConnection(connection_id_t cid)
{
    for (int con = 0; con < MAX_CONNECTIONS; con++)
    {
        if (gbss_server.connected_clients[con].cid == 0)
        {
            gbss_server.connected_clients[con].cid = cid;
            return &gbss_server.connected_clients[con];
        }
    }

    return NULL;
}

gbss_client_data_t *genericBroadcastScanServer_FindConnection(connection_id_t cid)
{
    for (int con = 0; con < MAX_CONNECTIONS; con++)
    {
        if (gbss_server.connected_clients[con].cid == cid)
        {
            return &gbss_server.connected_clients[con];
        }
    }
    return NULL;
}

gbss_srv_data_t *genericBroadcastScanServer_GetInstance(void)
{
    return &gbss_server;
}

static pdd_size_t genericBroadcastScanServer_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size;

    if (Device_GetProperty(device, device_property_generic_broadcast_scan_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }
    return config_size;
}

static void genericBroadcastScanServer_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);
    void *config = NULL;
    size_t config_size;

    if (Device_GetProperty(device, device_property_generic_broadcast_scan_config, &config, &config_size))
    {
        CsrMemCpy(buf, config, config_size);
    }
}

static void genericBroadcastScanServer_DeserialisetDeviceData(device_t device,
                                                           void *buf, pdd_size_t data_length,
                                                           pdd_size_t offset)
{
    UNUSED(offset);
    Device_SetProperty(device, device_property_generic_broadcast_scan_config, buf, data_length);
}

static gbss_config_t *genericBroadcastScanServer_RetrieveClientConfig(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void * device_config = NULL;
    size_t size_cfg;

    if(!device)
    {
        return NULL;
    }

    if (!Device_GetProperty(device, device_property_generic_broadcast_scan_config, &device_config, &size_cfg))
    {
        return NULL;
    }

    DEBUG_LOG("genericBroadcastScanServer_RetrieveClientConfig device=0x%p device_config=0x%p",
              device, device_config);

    return device_config;
}

static void genericBroadcastScanServer_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_GENERIC_BROADCAST_SCAN,
        genericBroadcastScanServer_GetDeviceDataLength,
        genericBroadcastScanServer_SerialisetDeviceData,
        genericBroadcastScanServer_DeserialisetDeviceData);
}

static void genericBroadcastScanServer_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

void genericBroadcastScanServer_StoreClientConfig(gatt_cid_t cid, void *config, uint8 config_size)
{
    device_t device = GattConnect_GetBtDevice(cid);
    if (device)
    {
        Device_SetProperty(device, device_property_generic_broadcast_scan_config, (void *)config, config_size);
        DeviceDbSerialiser_SerialiseDevice(device);
        DEBUG_LOG("genericBroadcastScanServer_StoreClientConfig device=0x%p", device);
    }
}

static void genericBroadcastScanServer_OnGattDisconnect(gatt_cid_t cid)
{
    gbss_config_t *generic_broadcast_scan_config = GenericBroadcastScanServer_RemoveConfig(cid);

    if (generic_broadcast_scan_config)
    {
        genericBroadcastScanServer_StoreClientConfig(cid, (void*)generic_broadcast_scan_config, sizeof(gbss_config_t));
    }

    LeBroadcastManager_SelfScanStop(GbssTask());
}

static void genericBroadcastScanServer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        DEBUG_LOG("genericBroadcastScanServer_OnEncryptionChanged cid 0x%4x encrypted %d", cid, encrypted);
        gbss_config_t *generic_broadcast_scan_config = (gbss_config_t *)genericBroadcastScanServer_RetrieveClientConfig(cid);

        if (GenericBroadcastScanServer_AddConfig(cid, generic_broadcast_scan_config) != CSR_BT_GATT_ACCESS_RES_SUCCESS)
        {
            DEBUG_LOG_ERROR("genericBroadcastScanServer_OnEncryptionChanged: Fail to add storted configuration\n");
        }
    }
}

static uint16 genericBroadcastScanServer_SourceIdToHandle(uint8 source_id)
{
    uint16 handle;
    switch(SOURCE_ID_TO_RECEIVER_STATE_IDX(source_id))
    {
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1)
        case GBSS_RECEIVER_STATE_IDX_1:
        {
            handle = HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1 - HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + 1;
        }
        break;
#endif
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2)
        case GBSS_RECEIVER_STATE_IDX_2:
        {
            handle = HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2 - HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + 1;
        }
        break;
#endif
    default:
        {
            handle = 0; /* maybe panic */
        }
    }
    return handle;
}

/*****************************************************************************/
bool GenericBroadcastScanServer_Init(Task init_task)
{
    uint8 index = 0;
    UNUSED(init_task);

    gbss_server.client_id = LeBroadcastManager_BassClientRegister(&notify_if);

    if(gbss_server.client_id == 0)
    {
        DEBUG_LOG_ERROR("GenericBroadcastScanServer_Init: Failed to register with BM\n");
        Panic();
    }

    if(NUM_GBSS_BRS != GBSS_RECEIVER_STATE_IDX_MAX)
    {
        DEBUG_LOG_ERROR("GenericBroadcastScanServer_Init: Incorect number of Receiver Characteristics\n");
        Panic();
    }

    genericBroadcastScanServer_RegisterAsPersistentDeviceDataUser();
    GattConnect_RegisterObserver(&le_connect_callbacks);

    memset(&gbss_server, 0, sizeof(gbss_srv_data_t));
    for (index = 0; index < MAX_CONNECTIONS; index++)
    {
        gbss_server.connected_clients[index].scan_cp_response.opcode = RESET_OPCODE;
    }

    gbss_server.gbss_task.handler = genericBroadcastScanServer_MessageHandler;
    gbss_server.gbss_volume_data.volume_setting = GBSS_DEFAULT_AUDIO_VOLUME;
    gbss_server.gbss_volume_data.step_size = GBSS_AUDIO_VOLUME_STEP_SIZE;

    GattRegisterReqSend(GbssTask(), 0);

    AudioSources_RegisterObserver(audio_source_le_audio_broadcast, genericBroadcastScanServer_GetAudioSourceObserverInterface());

#ifdef ENABLE_RDP_DEMO
    gbss_server.src_state_ntf_counter = 0;
#endif

    DEBUG_LOG("genericBroadcastScanServer_Init. Server initialised");

    return TRUE;
}

/*****************************************************************************/
status_t GenericBroadcastScanServer_AddConfig(connection_id_t cid,
                                              const gbss_config_t *config)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_AddConnection(cid);
    if (!connection)
    {
        return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    }
    /* Check config parameter:
     * If config is NULL, it indicates a default config should be used for the
     * peer device identified by the CID.
     */

    if (!config)
    {
        /* create a null filled congig */
        return CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    if (!genericBroadcastScanServer_IsValidCcc(config))
    {
        DEBUG_LOG_ERROR("Invalid Client Configuration Characteristic!\n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_PDU;
    }

    /* Save new ccc for the client */
    /* Do not notify scan report even if scan_ccc is set to CLIENT_CONFIG_NOTIFY as scan is not started at this point */
    connection->client_cfg.gbss_scan_ccc = config->gbss_scan_ccc;

    /* Do not notify scan control point response even if gbss_scan_cp_ccc is set to CLIENT_CONFIG_NOTIFY as no CP operation is started at this point */
    connection->client_cfg.gbss_scan_cp_ccc = config->gbss_scan_cp_ccc;

    /* Notify volume state */
    connection->client_cfg.gbss_volume_state_ccc = config->gbss_volume_state_ccc;
    GenericBroadcastScanServer_NotifyGbssVolumeState();

    for (uint8 brs = 0; brs < GBSS_RECEIVER_STATE_IDX_MAX; brs++)
    {
        connection->client_cfg.gbss_rcv_state_ccc[brs] = config->gbss_rcv_state_ccc[brs];
        if (config->gbss_rcv_state_ccc[brs] == CLIENT_CONFIG_NOTIFY)
        {
            GenericBroadcastScanServer_NotifyReceiverState(RECEIVER_STATE_IDX_TO_SOURCE_ID(brs));
        }
    }

    return CSR_BT_GATT_ACCESS_RES_SUCCESS;
}

/*****************************************************************************/
gbss_config_t *GenericBroadcastScanServer_RemoveConfig(connection_id_t cid)
{
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(cid);

    if (!connection)
    {
        return NULL;
    }
    connection->cid = 0;

    return &connection->client_cfg;
}

/*****************************************************************************/
void GenericBroadcastScanServer_NotifyGbssScanControlPointResponse(connection_id_t cid)
{
    uint8 client_index = 0;
    gbss_client_data_t *connection = genericBroadcastScanServer_FindConnection(cid);

	DEBUG_LOG("GenericBroadcastScanServer_NotifyGbssScanControlPointResponse: cid 0x%04x", cid);

    if (!connection)
        DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyGbssScanControlPointResponse: Invalid cid:%04x\n", cid);

    for (client_index = 0; client_index < MAX_CONNECTIONS; client_index++)
    {
        if ((gbss_server.connected_clients[client_index].cid != 0) &&
            (gbss_server.connected_clients[client_index].cid == connection->cid)  &&
            (gbss_server.connected_clients[client_index].client_cfg.gbss_scan_cp_ccc == CLIENT_CONFIG_NOTIFY))
        {
            int16 handle = HANDLE_GENERIC_BROADCAST_SCAN_CONTROL_POINT - HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + 1;
            uint8 report_size;
            uint8 *report_value;
            report_value = genericBroadcastScanServer_PrepareBroadcastScanControlPointValue(&connection->scan_cp_response, &report_size);

            if (report_value)
            {
                CsrBtGattNotificationEventReqSend(gbss_server.gattId,
                                                  gbss_server.connected_clients[client_index].cid,
                                                  handle,
                                                  report_size,
                                                  report_value);
            }
            else
            {
                DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyGbssScanControlPointResponse: Failed to generate notification report\n");
            }
        }
    }
}

/*****************************************************************************/
void GenericBroadcastScanServer_NotifyGbssScanReport(void)
{
    uint8 client_index = 0;

    for (client_index = 0; client_index < MAX_CONNECTIONS; client_index++)
    {
        if (gbss_server.connected_clients[client_index].cid != 0 &&
            gbss_server.connected_clients[client_index].client_cfg.gbss_scan_ccc == CLIENT_CONFIG_NOTIFY)
        {
            int16 handle = HANDLE_GENERIC_BROADCAST_SCAN_REPORT - HANDLE_GENERIC_BROADCAST_SCAN_SERVICE + 1;
            uint8 report_size;
            uint8 *report_value;
            report_value = genericBroadcastScanServer_PrepareBroadcastScanReportValue(&gbss_server.gbss_report, &report_size);

            if (report_value)
            {
                CsrBtGattNotificationEventReqSend(gbss_server.gattId,
                                                  gbss_server.connected_clients[client_index].cid,
                                                  handle,
                                                  report_size,
                                                  report_value);
            }
            else
            {
                DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyGbssScanReport: Failed to generate notification report\n");
            }
        }
    }
}

/*****************************************************************************/
void GenericBroadcastScanServer_NotifyReceiverState(uint8 source_id)
{
    uint8 client_index = 0;
#ifdef ENABLE_RDP_DEMO
    bool is_notified = FALSE;
#endif

    DEBUG_LOG("GenericBroadcastScanServer_NotifyReceiverState, source_id %d", source_id);

    for (client_index = 0; client_index < MAX_CONNECTIONS; client_index++)
    {
        if (gbss_server.connected_clients[client_index].cid != 0 &&
            gbss_server.connected_clients[client_index].client_cfg.gbss_scan_ccc == CLIENT_CONFIG_NOTIFY)
        {
            int16 handle = genericBroadcastScanServer_SourceIdToHandle(source_id);

            if(!handle)
            {
                DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyReceiverState: Incorect handle\n");
                return;
            }

#ifdef ENABLE_RDP_DEMO

            if (!is_notified)
            {
                ++gbss_server.src_state_ntf_counter;
            
                if (gbss_server.src_state_ntf_counter == 0)
                    ++gbss_server.src_state_ntf_counter;
            }
#endif
            uint8 report_size;
            uint8 *report_value;
            report_value = genericBroadcastScanServer_PrepareBroadcastReceiverStateValue(&report_size, SOURCE_ID_TO_RECEIVER_STATE_IDX(source_id));

            if (report_value || report_size == 0)
            {
                DEBUG_LOG("GenericBroadcastScanServer_NotifyReceiverState, notifying client with index:%d, cid:%04x",
                           client_index,gbss_server.connected_clients[client_index].cid);

                CsrBtGattNotificationEventReqSend(gbss_server.gattId,
                                                  gbss_server.connected_clients[client_index].cid,
                                                  handle,
                                                  report_size,
                                                  report_value);
#ifdef ENABLE_RDP_DEMO
                if (!is_notified) 
                {
                    if (report_size == 0)
                    {
                        --gbss_server.src_state_ntf_counter;
                
                        if (gbss_server.src_state_ntf_counter == 0)
                            --gbss_server.src_state_ntf_counter;
                    }
                    is_notified = TRUE;
                }
#endif
            }
            else
            {
                DEBUG_LOG_ERROR("GenericBroadcastScanServer_NotifyReceiverState: Failed to generate notification report\n");
            }
        }
    }
}
#endif /* INCLUDE_GBSS */
