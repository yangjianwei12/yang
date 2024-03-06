/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager.h"
#include "le_broadcast_manager_broadcast_sink.h"
#include "le_broadcast_manager_config.h"
#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_msg_handler.h"
#include "le_broadcast_manager_scan_delegator.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_sync.h"
#include "le_broadcast_manager_bass.h"
#include "le_broadcast_music_source.h"
#include "le_broadcast_media_control.h"
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
#include "le_broadcast_manager_periodic_scan.h"
#endif

#include <logging.h>
#include <timestamp_event.h>

#include "audio_sources.h"
#include "bt_device.h"
#include "device.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "gatt.h"
#include "gatt_connect.h"
#include "pddu_map.h"
#include "scan_delegator_role.h"
#include "broadcast_sink_role.h"

#ifdef ENABLE_TMAP_PROFILE
#include "tmap_profile.h"
#endif

#ifdef USE_SYNERGY
#include "tmap_server_role.h"
#endif

LOGGING_PRESERVE_MESSAGE_ENUM(le_broadcast_manager_messages)

#ifndef HOSTED_TEST_ENVIRONMENT
/*! A check that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(LE_BROADCAST_MANAGER, LE_BROADCAST_MANAGER_MESSAGE_END)
#endif

#define BROADCAST_MANAGER_LOG   DEBUG_LOG


static pdd_size_t leBroadcastManager_GetDeviceDataLength(device_t device)
{
    void * config = NULL;
    size_t config_size = 0;

    if(Device_GetProperty(device, device_property_le_audio_broadcast_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }
    return config_size;
}

static void leBroadcastManager_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);
    void * config = NULL;
    size_t config_size = 0;

    if(Device_GetProperty(device, device_property_le_audio_broadcast_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void leBroadcastManager_DeserialisetDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);
    Device_SetProperty(device, device_property_le_audio_broadcast_config, buf, data_length);
}

static void leBroadcastManager_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_BROADCAST_MANAGER,
        leBroadcastManager_GetDeviceDataLength,
        leBroadcastManager_SerialisetDeviceData,
        leBroadcastManager_DeserialisetDeviceData);
}


bool LeBroadcastManager_Init(Task init_task)
{
    BROADCAST_MANAGER_LOG("LeBroadcastManager_Init");
    UNUSED(init_task);
    leBroadcastManager_RegisterAsPersistentDeviceDataUser();
    LeBroadcastManager_ScanDelegatorInit(BROADCAST_MANAGER_MAX_BROADCAST_SOURCES);
    LeBroadcastManager_SourceInit();
    LeBroadcastManager_BroadcastSinkInit();
    AudioSources_RegisterAudioInterface(audio_source_le_audio_broadcast, LeBroadcastMusicSource_GetAudioInterface());
    AudioSources_RegisterMediaControlInterface(audio_source_le_audio_broadcast, leBroadcastManager_MediaControlGetInterface());
    LeBroadcastManager_SyncInit();
    LeBroadcastManager_BassInit();

#if defined (USE_SYNERGY) && !defined(INCLUDE_LE_AUDIO_UNICAST)
    LeTmapServer_Init();
    TmapProfile_Init();
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    leBroadcastManager_PeriodicScanInit();
#endif

    return TRUE;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */
