/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    micp_server
    \brief      Initializes the MICS server for MICP
*/

#include "micp_server_private.h"
#include "micp_server.h"
#include "gatt_handler_db_if.h"
#include "pddu_map.h"
#include "device.h"
#include "bt_device.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt_connect.h"
#include "gatt_mics_server.h"
#include <stdlib.h>
#include <logging.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

micp_server_task_data_t micp_task_data;

static void micpServer_OnGattConnect(gatt_cid_t cid);
static void micpServer_OnGattDisconnect(gatt_cid_t cid);
static void micpServer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static const gatt_connect_observer_callback_t micpServer_connect_callbacks =
{
    .OnConnection = micpServer_OnGattConnect,
    .OnDisconnection = micpServer_OnGattDisconnect,
    .OnEncryptionChanged = micpServer_OnEncryptionChanged
};

static void micpServer_InformMuteStateToClients(uint8 mute_state)
{
    if (mute_state != MICS_SERVER_MUTE_DISABLED)
    {
        MESSAGE_MAKE(msg, MICP_SERVER_MUTE_IND_T);
        msg->mute_state = mute_state;
        TaskList_MessageSend(micp_task_data.client_tasks, MICP_SERVER_MUTE_IND, msg);
    }
}

/* Process the mic mute state changes */
static void micpServer_HandleMicStateSetInd(GattMicsServerMicStateSetInd *ind)
{
    DEBUG_LOG("micsServer_HandleMicStateSetInd Mute value %d", ind->muteValue);
    micpServer_InformMuteStateToClients(ind->muteValue);
    GattMicsServerSetMicStateRsp(ind->srvcHndl, ind->cid);
}

static pdd_size_t micpServer_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (!Device_GetProperty(device, device_property_le_audio_micp_config, &config, &config_size))
    {
        config_size = 0;
    }
    return config_size;
}

static void micpServer_SerialiseDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void *config = NULL;
    size_t config_size = 0;
    UNUSED(offset);

    if (Device_GetProperty(device, device_property_le_audio_micp_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void micpServer_DeserialiseDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_le_audio_micp_config, buf, data_length);
}

/*! \brief Retrieve MICS service config for the GATT Connection */
static void * micpServer_RetrieveClientConfig(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void *config_info = NULL;

    if (device)
    {
        size_t size;

        if (!Device_GetProperty(device, device_property_le_audio_micp_config, &config_info, &size))
        {
            config_info = NULL;
        }
    }

    return config_info;
}

/*! \brief Store MICS config data to NVM for the provided connection identifier */
static bool micpServer_StoreClientConfig(gatt_cid_t cid, void *config, uint8 size)
{
    bool config_stored = FALSE;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        Device_SetProperty(device, device_property_le_audio_micp_config, config, size);
        DeviceDbSerialiser_SerialiseDevice(device);
        config_stored = TRUE;
    }

    return config_stored;
}

static void micpServer_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void micpServer_OnGattDisconnect(gatt_cid_t cid)
{
    GattMicsClientConfigDataType *config;

    GattMicsServerSetMicState(micp_task_data.mics_service_handle, MICS_SERVER_NOT_MUTED);

    config = GattMicsServerRemoveConfig(micp_task_data.mics_service_handle, cid);

    if (config != NULL)
    {
        micpServer_StoreClientConfig(cid, config, sizeof(GattMicsClientConfigDataType));
        free(config);
    }
}

static void micpServer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    GattMicsClientConfigDataType *config;

    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        config = (GattMicsClientConfigDataType*) micpServer_RetrieveClientConfig(cid);
        GattMicsServerAddConfig(micp_task_data.mics_service_handle, cid, config != NULL ? config : NULL);
    }
}

/* Process the message from GATT MICS Server */
static void micpServer_ProcessMicsServerMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(id);

    GattMicsMessageId mics_msg_id = *(GattMicsMessageId*)message;

    switch (mics_msg_id)
    {
        case GATT_MICS_SERVER_MIC_STATE_SET_IND:
            micpServer_HandleMicStateSetInd((GattMicsServerMicStateSetInd*) message);
        break;

        case GATT_MICS_SERVER_CONFIG_CHANGE_IND:
        {
            GattMicsServerConfigChangeInd *ind = (GattMicsServerConfigChangeInd *)message;
            GattMicsClientConfigDataType * service_config = NULL;
            GattMicsClientConfigDataType * stored_config = NULL;

            if (ind->configChangeComplete)
            {
                DEBUG_LOG("micpServer_ProcessMicsServerMessage GATT_MICS_SERVER_CONFIG_CHANGE_IND");
                service_config = GattMicsServerGetConfig(ind->srvcHndl, ind->cid);
                stored_config = (GattMicsClientConfigDataType *)micpServer_RetrieveClientConfig(ind->cid);
                if (service_config != NULL)
                {
                    if (stored_config == NULL || service_config->micsMuteClientCfg != stored_config->micsMuteClientCfg)
                    {
                        DEBUG_LOG("micpServer_ProcessMicsServerMessage storing GATT_MICS_SERVER_CONFIG_CHANGE_IND");
                        micpServer_StoreClientConfig(ind->cid, (void *)service_config, sizeof(GattMicsClientConfigDataType));
                    }
                    pfree(service_config);
                 }
            }
        }
        break;

        default:
        break;
    }
}

void MicpServer_Init(void)
{
    GattMicsInitData initData = {0};

    memset(&micp_task_data, 0 , sizeof(micp_server_task_data_t));
    micp_task_data.client_tasks = TaskList_Create();
    micp_task_data.task_data.handler = micpServer_ProcessMicsServerMessage;
    micp_task_data.mics_service_handle = GattMicsServerInit(TrapToOxygenTask((Task)&micp_task_data.task_data),
                                                            HANDLE_MICROPHONE_CONTROL_SERVICE,
                                                            HANDLE_MICROPHONE_CONTROL_SERVICE_END,
                                                            &initData);
    PanicFalse(micp_task_data.mics_service_handle != MICS_INVALID_SERVICE_HANDLE);
    GattConnect_RegisterObserver(&micpServer_connect_callbacks);
}

void MicpServer_SetMicState(uint8 mic_state)
{
    if (micp_task_data.mics_service_handle != MICS_INVALID_SERVICE_HANDLE)
    {
        DEBUG_LOG("MicpServer_SetMicState %d", mic_state);
        micpServer_InformMuteStateToClients(mic_state);
        GattMicsServerSetMicState(micp_task_data.mics_service_handle, mic_state);
    }
}

uint8 MicpServer_GetMicState(void)
{
    uint8 mic_mute_state = FALSE;

    if (micp_task_data.mics_service_handle != MICS_INVALID_SERVICE_HANDLE)
    {
        if (!GattMicsServerReadMicState(micp_task_data.mics_service_handle, &mic_mute_state))
        {
             mic_mute_state = 0;
        }
    }

    return mic_mute_state;
}

void MicpServer_ToggleMicMute(void)
{
    uint8 mic_mute_state = MicpServer_GetMicState();

    if (mic_mute_state != MICS_SERVER_MUTE_DISABLED)
    {
        mic_mute_state = !mic_mute_state;
        MicpServer_SetMicState(mic_mute_state);
    }
}

void MicpServer_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_MICS_SERVICE,
        micpServer_GetDeviceDataLength,
        micpServer_SerialiseDeviceData,
        micpServer_DeserialiseDeviceData);
}

void MicpServer_ClientRegister(Task client_task)
{
    PanicNull((void *) client_task);
    TaskList_AddTask(micp_task_data.client_tasks, client_task);
}

void MicpServer_ClientUnregister(Task client_task)
{
    PanicNull((void *) client_task);
    TaskList_RemoveTask(micp_task_data.client_tasks, client_task);
}
