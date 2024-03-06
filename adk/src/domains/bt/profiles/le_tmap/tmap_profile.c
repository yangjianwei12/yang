/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      Initializes the TMAS server and common placeholder for Broadcast/Unicast for TMAP
*/

#include "tmap_server_role_advertising.h"
#include "le_advertising_manager.h"
#include "gatt_tmas_server_uuids.h"
#include "gatt_tmas_server.h"
#include "tmap_profile.h"
#include "tmap_server_private.h"
#include "gatt_connect.h"
#include <gatt_handler_db_if.h>
#include "gatt_service_discovery.h"

#define TMAP_PROFILE_LOG     DEBUG_LOG

#ifdef INCLUDE_LE_AUDIO_BROADCAST
#define TMAP_BROADCAST_SINK_ROLE        TMAP_ROLE_BROADCAST_MEDIA_RECEIVER
#else
#define TMAP_BROADCAST_SINK_ROLE        0
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#define TMAP_BROADCAST_SOURCE_ROLE      TMAP_ROLE_BROADCAST_MEDIA_SENDER
#else
#define TMAP_BROADCAST_SOURCE_ROLE      0
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
#define TMAP_UNICAST_SINK_ROLE          (TMAP_ROLE_CALL_TERMINAL | TMAP_ROLE_UNICAST_MEDIA_RECEIVER)
#else
#define TMAP_UNICAST_SINK_ROLE          0
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#define TMAP_UNICAST_SOURCE_ROLE        (TMAP_ROLE_CALL_GATEWAY | TMAP_ROLE_UNICAST_MEDIA_SENDER)
#else
#define TMAP_UNICAST_SOURCE_ROLE        0
#endif

#define TMAP_PROFILE_ROLE             (TMAP_BROADCAST_SINK_ROLE |     \
                                       TMAP_BROADCAST_SOURCE_ROLE |   \
                                       TMAP_UNICAST_SINK_ROLE |       \
                                       TMAP_UNICAST_SOURCE_ROLE)

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
#define TMAP_SERVER_HANDLE_START    HANDLE_TELEPHONY_MEDIA_AUDIO_SERVICE
#define TMAP_SERVER_HANDLE_END      HANDLE_TELEPHONY_MEDIA_AUDIO_SERVICE_END
#else
#define TMAP_SERVER_HANDLE_START    0xFFFF
#define TMAP_SERVER_HANDLE_END      0xFFFF
#endif

/* Service handle initialization failure */
#define INVALID_SERVICE_HANDLE ((ServiceHandle)(0x0000))

static void tmapProfile_OnGattConnect(gatt_cid_t cid);
static void tmapProfile_OnGattDisconnect(gatt_cid_t cid);
static void tmapProfile_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static ServiceHandle tmasServiceHandle = INVALID_SERVICE_HANDLE;
static TaskData tmap_profile_task;

static const gatt_connect_observer_callback_t tmapProfile_connect_callbacks =
{
    .OnConnection = tmapProfile_OnGattConnect,
    .OnDisconnection = tmapProfile_OnGattDisconnect,
    .OnEncryptionChanged = tmapProfile_OnEncryptionChanged
};

static void tmapProfile_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void tmapProfile_OnGattDisconnect(gatt_cid_t cid)
{
    if (tmasServiceHandle != INVALID_SERVICE_HANDLE)
    {
        GattTmasServerRemoveConfig(tmasServiceHandle, cid);
    }

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    /* Add configuration to both MCS and TBS server */
    tmapServer_McsTbsServerRemoveConfig(cid);
#endif
}

static void tmapProfile_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid) && tmasServiceHandle != INVALID_SERVICE_HANDLE)
    {
        GattTmasServerAddConfig(tmasServiceHandle, cid);
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        /*Remove configuration from both MCS and TBS server */
        tmapServer_McsTbsServerAddConfig(cid);
#endif
    }
}

static void tmapProfile_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(task);
    UNUSED(message);
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    tmapServer_ProcessMcsTbsServerMessage(id, message);
#endif
}

void TmapProfile_Init(void)
{
    GattTmasInitData initData;

    tmap_profile_task.handler = tmapProfile_MessageHandler;

    if (tmasServiceHandle == INVALID_SERVICE_HANDLE)
    {
        TMAP_PROFILE_LOG("TmapProfile_Init");
        initData.role = TMAP_PROFILE_ROLE;
        tmasServiceHandle = GattTmasServerInit(TrapToOxygenTask((Task)&tmap_profile_task),
                                               TMAP_SERVER_HANDLE_START,
                                               TMAP_SERVER_HANDLE_END,
                                               &initData);

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        /* Initialise both MCS and TBS server */
        tmapServer_McsTbsServerInit((Task)&tmap_profile_task);
#endif
        GattConnect_RegisterObserver(&tmapProfile_connect_callbacks);
        GattServiceDiscovery_RegisterServiceForDiscovery(GATT_SD_TMAS_SRVC);
    }
}
