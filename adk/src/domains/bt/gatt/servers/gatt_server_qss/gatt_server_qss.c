/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Initializes the QSS server for Qualcomm Snapdragon Sound Service.
*/
#ifdef INCLUDE_GATT_QSS_SERVER

#include "gatt_server_qss.h"
#include "gatt_server_qss_private.h"
#include "gatt_qss_server.h"
#include "gatt_handler_db_if.h"
#include "gatt_connect.h"
#include "gatt.h"
#include <logging.h>
#include <stdio.h>
#include "gatt_handler.h"
#include "feature.h"
#include "bt_device.h"

gatt_server_qss_data_t gatt_server_qss_data;

static void gattServerQss_OnGattConnect(gatt_cid_t cid);
static void gattServerQss_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);
static void gattServerQss_OnGattDisconnect(gatt_cid_t cid);
static void gattServerQss_WriteClientConfigToStore(gatt_cid_t cid, uint16 client_config);
static void gattServerQss_ProcessQssServerMessage(Task task, MessageId id, Message message);

static const gatt_connect_observer_callback_t gattServerQss_connect_callbacks =
{
    .OnConnection = gattServerQss_OnGattConnect,
    .OnDisconnection = gattServerQss_OnGattDisconnect,
    .OnEncryptionChanged = gattServerQss_OnEncryptionChanged
};

static void gattServerQss_HandleReadQssSupportInd(GATT_QSS_SERVER_READ_IND *ind)
{
    uint8 is_qss_supported = FeatureVerifyLicense(SDS_COMPLIANCE) ?
                                 GATT_SERVER_QSS_SUPPORTED : GATT_SERVER_QSS_NOT_SUPPORTED;

    DEBUG_LOG("gattServerQss_HandleReadQssSupportInd is_qss_supported: %d", is_qss_supported);

    GattQssServerReadQssSupportResponse(GattServerQss_GetQssHandle(), ind->cid, is_qss_supported);
}

static void gattServerQss_HandleReadUserDescriptionInd(GATT_QSS_SERVER_READ_IND *ind)
{
    DEBUG_LOG("gattServerQss_HandleReadUserDescriptionInd");

    GattQssServerReadUserDescriptionResponse(GattServerQss_GetQssHandle(),
                                             ind->cid,
                                             gatt_server_qss_data.user_description_len,
                                             (uint8 *)gatt_server_qss_data.user_description);
}

static void gattServerQss_HandleReadQssLosslessAudioInd(GATT_QSS_SERVER_READ_IND *ind)
{
    DEBUG_LOG("gattServerQss_HandleReadQssLosslessAudioInd");

    GattQssServerReadLosslessAudioResponse(GattServerQss_GetQssHandle(),
                                           ind->cid,
                                           gatt_server_qss_data.lossless_audio_data);
}

void GattServerQss_SendUpdate(gatt_cid_t cid, gatt_qss_server_msg_t msg_id)
{
    switch (msg_id)
    {
        case GATT_QSS_SERVER_CONFIG_UPDATED:
        {
            /* Send a config updated message to all the registered clients */
            MESSAGE_MAKE(ind, GATT_QSS_SERVER_CONFIG_UPDATED_T);
            ind->cid = cid;
            ind->ntf_enabled = gatt_server_qss_data.ntf_enable;
            TaskList_MessageSend(gatt_server_qss_data.client_tasks, GATT_QSS_SERVER_CONFIG_UPDATED, ind);
        }
        break;

        default:
        break;
    }
}

static void gattServerQss_HandleWriteQssLosslessAudioConfigInd(GATT_QSS_SERVER_WRITE_CLIENT_CONFIG_IND *ind)
{
    DEBUG_LOG("gattServerQss_HandleWriteQssLosslessAudioConfigInd clientConfig:0x%x", ind->clientConfig);

    gatt_server_qss_data.ntf_enable = ind->clientConfig == GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_ENABLE;

    gattServerQss_WriteClientConfigToStore(ind->cid, ind->clientConfig);
    GattServerQss_SendUpdate(ind->cid, GATT_QSS_SERVER_CONFIG_UPDATED);
}

static void gattServerQss_HandleReadQssLosslessAudioConfigInd(GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND *ind)
{
    DEBUG_LOG("gattServerQss_HandleReadQssLosslessAudioConfigInd notification_enable: 0x%x", gatt_server_qss_data.ntf_enable);

    GattQssServerReadClientConfigResponse(GattServerQss_GetQssHandle(),
                                          ind->cid,
                                          gatt_server_qss_data.ntf_enable ? GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_ENABLE 
                                                                          : GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_DISABLE);
}

static void gattServerQss_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void gattServerQss_OnGattDisconnect(gatt_cid_t cid)
{
    if (cid == gatt_server_qss_data.cid)
    {
        gatt_server_qss_data.cid = INVALID_CID;
        gatt_server_qss_data.ntf_enable = FALSE;
    }
}

static void gattServerQss_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    UNUSED(encrypted);
    uint16 client_config;

    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        gatt_server_qss_data.cid = cid;

        if (GattServerQss_ReadClientConfigFromStore(cid, &client_config))
        {
            DEBUG_LOG("gattServerQss_OnEncryptionChanged client_config:0x%x", client_config);
            gatt_server_qss_data.ntf_enable = client_config == GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_ENABLE;
        }
    }
}

static void gattServerQss_WriteClientConfigToStore(gatt_cid_t cid, uint16 client_config)
{
    bdaddr public_addr;
    bool status = FALSE;

    if (GattConnect_GetPublicAddrFromConnectionId(cid, &public_addr))
    {
        status = appDeviceSetQssServerConfig(&public_addr, client_config);
    }

    DEBUG_LOG("gattServerQss_WriteClientConfigToStore status %d", status);
}

/* Process the message from GATT QSS Server */
static void gattServerQss_ProcessQssServerMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(id);

    GattQssServerMessageInd qss_msg_id = *(GattQssServerMessageInd*)message;

    switch (qss_msg_id)
    {
        case GATT_QSS_SERVER_READ_QSS_SUPPORT_IND:
            gattServerQss_HandleReadQssSupportInd((GATT_QSS_SERVER_READ_IND*) message);
        break;

        case GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND:
            gattServerQss_HandleReadUserDescriptionInd((GATT_QSS_SERVER_READ_IND*) message);
        break;

        case GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND:
            gattServerQss_HandleReadQssLosslessAudioInd((GATT_QSS_SERVER_READ_IND*) message);
        break;

        case GATT_QSS_SERVER_WRITE_LOSSLESS_AUDIO_CLIENT_CONFIG_IND:
            gattServerQss_HandleWriteQssLosslessAudioConfigInd((GATT_QSS_SERVER_WRITE_CLIENT_CONFIG_IND*) message);
        break;

        case GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_CLIENT_CONFIG_IND:
            gattServerQss_HandleReadQssLosslessAudioConfigInd((GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND*) message);
        break;

        default:
        break;
    }
}

void GattServerQss_Init(void)
{
    memset(&gatt_server_qss_data, 0 , sizeof(gatt_server_qss_data_t));
    gatt_server_qss_data.task_data.handler = gattServerQss_ProcessQssServerMessage;
    gatt_server_qss_data.qss_service_handle = GattQssServerInit(TrapToOxygenTask((Task)&gatt_server_qss_data.task_data),
                                                                HANDLE_QUALCOMM_SNAPDRAGON_SOUND_SERVICE,
                                                                HANDLE_QUALCOMM_SNAPDRAGON_SOUND_SERVICE_END);
    gatt_server_qss_data.client_tasks = TaskList_Create();

    PanicFalse(gatt_server_qss_data.qss_service_handle != QSS_INVALID_SERVICE_HANDLE);
    GattConnect_RegisterObserver(&gattServerQss_connect_callbacks);
    memcpy(gatt_server_qss_data.user_description,
           GATT_SERVER_QSS_USER_DESCRIPTION_DEFAULT,
           GATT_SERVER_QSS_USER_DESCRIPTION_LEN_DEFAULT);
    gatt_server_qss_data.user_description_len = GATT_SERVER_QSS_USER_DESCRIPTION_LEN_DEFAULT;
}

bool GattServerQss_SetQssUserDescription(const char *description, uint8 len)
{
    bool status = FALSE;

    if (len > 0 && len <= GATT_SERVER_QSS_USER_DESCRIPTION_MAX_LEN && description != NULL)
    {
        gatt_server_qss_data.user_description_len = len;
        memcpy(gatt_server_qss_data.user_description, description, len);
        status = TRUE;
    }
    else
    {
        DEBUG_LOG("GattServerQss_SetQssUserDescription Invalid user description length %d (expected b/w 1 and %d)",
                   len, GATT_SERVER_QSS_USER_DESCRIPTION_MAX_LEN);
    }

    return status;
}

bool GattServerQss_ReadClientConfigFromStore(gatt_cid_t cid, uint16 *config)
{
    uint16 stored_client_config = 0;
    bdaddr public_addr;
    bool client_config_read = FALSE;

    if (GattConnect_GetPublicAddrFromConnectionId(cid, &public_addr))
    {
        client_config_read = appDeviceGetQssServerConfig(&public_addr, &stored_client_config);
        DEBUG_LOG("GattServerQss_ReadClientConfigFromStore read=[%d] config=[0x%x]", client_config_read, stored_client_config);

        if (client_config_read)
        {
            *config = stored_client_config;
        }
    }

    return client_config_read;
}

void GattServerQss_UpdateLosslessAudiodata(uint32 lossless_data)
{
    if (gatt_server_qss_data.lossless_audio_data == lossless_data)
    {
        return;
    }

    DEBUG_LOG("GattServerQss_UpdateLosslessAudiodata Current =[0x%x] New=[0x%x]",gatt_server_qss_data.lossless_audio_data, lossless_data);

    gatt_server_qss_data.lossless_audio_data = lossless_data;

    if (gatt_server_qss_data.ntf_enable)
    {
        GattQssServerSendLosslessAudioNotification(GattServerQss_GetQssHandle(), gatt_server_qss_data.cid, lossless_data);
    }
}

bool GattServerQss_IsNotificationEnabled(void)
{
    return gatt_server_qss_data.ntf_enable;
}

void GattServerQss_ClientRegister(Task client_task)
{
    PanicNull((void *)client_task);
    TaskList_AddTask(gatt_server_qss_data.client_tasks, client_task);
}

void GattServerQss_ClientUnregister(Task client_task)
{
    PanicNull((void *)client_task);
    TaskList_RemoveTask(gatt_server_qss_data.client_tasks, client_task);
}

#endif /* INCLUDE_GATT_QSS_SERVER */
