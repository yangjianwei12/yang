/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Manager for LE Broadcast Audio Sources.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager.h"
#include "le_broadcast_manager_base.h"
#include "le_broadcast_manager_bass.h"
#include "le_broadcast_manager_config.h"
#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_link_policy.h"
#include "le_broadcast_manager_msg_handler.h"
#include "le_broadcast_manager_scan_delegator.h"
#include "le_broadcast_manager_sync.h"
#include "le_broadcast_manager_sync_typedef.h"
#include "le_broadcast_manager_broadcast_sink.h"
#include <scan_delegator_role.h>

#ifdef INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN
#include "le_broadcast_manager_periodic_scan.h"
#endif

#include "le_audio_messages.h"
#include "le_scan_manager.h"
#include "multidevice.h"
#include "bandwidth_manager.h"
#include "bt_device.h"
#include "broadcast_sink_role.h"
#include "gatt.h"
#include "gatt_connect.h"
#include "qualcomm_connection_manager.h"
#include "telephony_messages.h"

#include "av.h"
#include "hfp_profile.h"
#include "mirror_profile.h"
#include "pacs_utilities.h"

#include <connection.h>
#include <gatt_pacs_server.h>
#include <logging.h>
#include <task_list.h>

#include <timestamp_event.h>

#include <bdaddr.h>
#include <dm_prim.h>

#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#include "gatt_lib.h"
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif
#include "le_bap_profile.h"

LOGGING_PRESERVE_MESSAGE_TYPE(broadcast_manager_internal_msg_id_t)

#define BROADCAST_MANAGER_SOURCE_LOG     DEBUG_LOG

/* todo temp BIG handle, need a way to assign these? */
#define BROADCAST_MANAGER_BIG_HANDLE            0x10

#define BROADCAST_MANAGER_METADATA_OFFSET_BYTES 0x04

#define leBroadcastManager_IsClientPaSyncStateSynchronizeToPa(pa_sync_state) ((pa_sync_state == le_bm_pa_sync_past_available) || (pa_sync_state == le_bm_pa_sync_past_not_available))


le_broadcast_manager_t le_broadcast_manager;


static void leBroadcastManager_SetCurrentPaSyncState(broadcast_source_state_t * broadcast_source, scan_delegator_server_pa_sync_state_t pa_sync_state);
static scan_delegator_server_pa_sync_state_t leBroadcastManager_GetCurrentPaSyncState(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_SetCurrentBisSyncState(broadcast_source_state_t * broadcast_source, const le_broadcast_manager_bis_sync_state_t *bis_sync_state);
static uint32 leBroadcastManager_GetCurrentBisSyncState(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_SetCurrentBigEncryptionState(broadcast_source_state_t * broadcast_source, scan_delegator_server_big_encryption_t big_encryption);
static scan_delegator_server_big_encryption_t leBroadcastManager_GetCurrentBigEncryptionState(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_SetBigEncryptionBadCode(broadcast_source_state_t *broadcast_source);
static uint8 leBroadcastManager_GetSourceAdvSid(broadcast_source_state_t * broadcast_source);
static broadcast_source_state_t * leBroadcastManager_GetFreeSourceState(void);
static bool leBroadcastManager_AnyFreeSourceId(void);
static broadcast_source_state_t * leBroadcastManager_GetSourceBySyncHandle(uint16 sync_handle);
static broadcast_source_state_t * leBroadcastManager_GetSourceByBigHandle(uint8 big_handle);
static void leBroadcastManager_StartScanForPaSource(uint8 source_id);
static void leBroadcastManager_StopSyncToPaSource(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_UpdateStartSyncToPa(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_CreateSyncToBisReq(broadcast_source_state_t * broadcast_source, uint32 bis_index);
static void leBroadcastManager_CreateSyncToBisCfm(hci_status status, uint8 numBis, uint16 *bis_handle);
static bool leBroadcastManager_TerminateSyncToPaReq(uint16 sync_handle);
static bool leBroadcastManager_TerminateSyncToBisReq(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_IsMandatoryCodecConfigurationSet(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_UpdateStartSyncToBis(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_UpdateStopSyncToPa(broadcast_source_state_t * broadcast_source);
static bool leBroadcastManager_UpdateStopSyncToBis(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_SourceModifiedEvent(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_SourceBroadcastCodeEvent(broadcast_source_state_t * broadcast_source);
static void leBroadcastManager_CompleteStop(void);
static void leBroadcastManager_AttemptToSourceRemove(broadcast_source_state_t *broadcast_source);
static void leBroadcastManager_CompleteRemoveAllSources(void);
static broadcast_source_state_t * leBroadcastManager_FindAnyRemoveSourcePending(void);
static bool leBroadcastManager_IsInitiatingSyncToPA(broadcast_source_state_t *broadcast_source);
static bool leBroadcastManager_IsInitiatingScanForPA(broadcast_source_state_t *broadcast_source);
static void leBroadcastManager_ResetPaSyncLostState(broadcast_source_state_t *broadcast_source);
static void leBroadcastManager_SetPaSyncLost(broadcast_source_state_t *broadcast_source);
static void leBroadcastManager_ResyncToLostPa(void);
static le_broadcast_manager_bis_index_t leBroadcastManagerSource_GetBisIndexToRender(const broadcast_source_state_t *boradcast_source);
static void leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource(void);
static uint16 leBroadcastManager_GetBroadcastSourceBaseSduSize(broadcast_source_state_t *broadcast_source);


static inline bool leBroadcastManager_IsCurrentBisSyncStateIdle(broadcast_source_state_t * broadcast_source)
{
    return (leBroadcastManager_GetCurrentBisSyncState(broadcast_source) == 0);
}

static void leBroadcastManager_SetCurrentPaSyncState(broadcast_source_state_t * broadcast_source, scan_delegator_server_pa_sync_state_t pa_sync_state)
{
    LeBroadcastManager_ScanDelegatorSetPaSyncState(broadcast_source->source_id, pa_sync_state);
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

/*! \brief Get the current PA sync state of a broadcast source.

    The broadcast source passed in must be a valid one otherwise this function
    will cause a panic.

    \param broadcast_source Pointer to a valid broadcast source.
*/
static scan_delegator_server_pa_sync_state_t leBroadcastManager_GetCurrentPaSyncState(broadcast_source_state_t * broadcast_source)
{
    return LeBroadcastManager_ScanDelegatorGetPaSyncState(broadcast_source->source_id);
}

static void leBroadcastManager_SetCurrentBisSyncState(broadcast_source_state_t * broadcast_source, const le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    LeBroadcastManager_ScanDelegatorSetBisSyncState(broadcast_source->source_id, bis_sync_state);
    
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

static void leBroadcastManager_SetCurrentBisSyncStateFailedSync(broadcast_source_state_t * broadcast_source)
{
    LeBroadcastManager_ScanDelegatorSetBisSyncStateFailedSync(broadcast_source->source_id);
    
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

static void leBroadcastManager_SetCurrentBisSyncStateNoSync(broadcast_source_state_t * broadcast_source)
{
    LeBroadcastManager_ScanDelegatorSetBisSyncStateNoSync(broadcast_source->source_id);
    
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

static uint32 leBroadcastManager_GetCurrentBisSyncState(broadcast_source_state_t * broadcast_source)
{
    le_broadcast_manager_bis_index_t bis_index_to_render = leBroadcastManagerSource_GetBisIndexToRender(broadcast_source);
    uint32 bis_sync_state = LeBroadcastManager_ScanDelegatorGetBisSyncState(broadcast_source->source_id);

    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_GetCurrentBisSyncState source_id %u to_render=0x%x sync state=0x%x",
                                 broadcast_source->source_id, bis_index_to_render.index, bis_sync_state);

    if(bis_index_to_render.index)
    {
        // indexes for audio locations are known, so filter out any indexes this device is not interested in
        bis_sync_state = (LeBroadcastManager_ScanDelegatorGetBisSyncState(broadcast_source->source_id) & bis_index_to_render.index);
    }
    return bis_sync_state;
}

static void leBroadcastManager_SetCurrentBigEncryptionState(broadcast_source_state_t * broadcast_source, scan_delegator_server_big_encryption_t big_encryption)
{
    LeBroadcastManager_ScanDelegatorSetBigEncryptionState(broadcast_source->source_id, big_encryption);
}

static scan_delegator_server_big_encryption_t leBroadcastManager_GetCurrentBigEncryptionState(broadcast_source_state_t * broadcast_source)
{
    return LeBroadcastManager_ScanDelegatorGetBigEncryptionState(broadcast_source->source_id);
}

static void leBroadcastManager_SetBigEncryptionBadCode(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_SetBigEncryptionBadCode source_id=0x%x", broadcast_source->source_id);
    LeBroadcastManager_ScanDelegatorSetBigEncryptionBadCode(broadcast_source->source_id);
}

typed_bdaddr leBroadcastManager_GetSourceTypedBdaddr(broadcast_source_state_t * broadcast_source)
{
    return LeBroadcastManager_ScanDelegatorGetSourceTypedBdaddr(broadcast_source->source_id);
}

static uint8 leBroadcastManager_GetSourceAdvSid(broadcast_source_state_t * broadcast_source)
{
    return LeBroadcastManager_ScanDelegatorGetSourceAdvSid(broadcast_source->source_id);
}

static broadcast_source_state_t * leBroadcastManager_GetFreeSourceState(void)
{
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        if (!leBroadcastManager_IsSourceValid(&le_broadcast_manager.broadcast_source_receive_state[i]))
        {
            return &le_broadcast_manager.broadcast_source_receive_state[i];
        }
    }
    
    return NULL;
}

static bool leBroadcastManager_AnyFreeSourceId(void)
{
    uint16 number_source_id;
    uint8 * source_ids = LeBapScanDelegator_GetBroadcastSourceIds(&number_source_id);
    bool free_id = FALSE;
    int source_index;
    
    if (source_ids)
    {
        /* Find a source_id that is not currently being used by this module */
        for (source_index = 0; source_index < number_source_id; source_index++)
        {
            if (source_ids[source_index] == SCAN_DELEGATOR_SOURCE_ID_INVALID)
            {
                free_id = TRUE;
                break;
            }
        }
        free(source_ids);
    }
    
    return free_id;
}

broadcast_source_state_t * LeBroadcastManager_GetSourceById(uint8 source_id)
{
    if (source_id != SCAN_DELEGATOR_SOURCE_ID_INVALID)
    {
        for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
        {
            if (le_broadcast_manager.broadcast_source_receive_state[i].source_id == source_id)
            {
                return &le_broadcast_manager.broadcast_source_receive_state[i];
            }
        }
    }
    
    return NULL;
}

static broadcast_source_state_t * leBroadcastManager_GetSourceBySyncHandle(uint16 sync_handle)
{
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        if (le_broadcast_manager.broadcast_source_receive_state[i].sync_handle == sync_handle)
        {
            return &le_broadcast_manager.broadcast_source_receive_state[i];
        }
    }
    
    return NULL;
}

static broadcast_source_state_t * leBroadcastManager_GetSourceByBigHandle(uint8 big_handle)
{
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        if (le_broadcast_manager.broadcast_source_receive_state[i].big_handle == big_handle)
        {
            return &le_broadcast_manager.broadcast_source_receive_state[i];
        }
    }
    
    return NULL;
}

static void leBroadcastManager_StartScanForPaSource(uint8 source_id)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_StartScanForPaSource source_id=0x%x", source_id);

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    le_bm_periodic_scan_params_t scan_params = {
        .timeout = LeBroadcastManager_GetFindTrainsTimeout(),
        .interface = leBroadcastManager_GetPeriodicScanInterface()
    };
    le_bm_periodic_scan_handle handle = 0;

    PanicFalse(leBroadcastManager_PeriodicScanStartRequest(&scan_params, &handle));

    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);
    LeBroadcastManager_SourceSetPeriodicScanHandle(broadcast_source, handle);
#else
    LeBapBroadcastSink_StartScanPaSourceRequest(LeBroadcastManager_GetFindTrainsTimeout(), NULL);
#endif
    
    leBroadcastManager_SetSyncState(source_id, broadcast_manager_sync_scanning_for_pa);
    
    TimestampEvent(TIMESTAMP_EVENT_LE_BROADCAST_START_PA_SYNC);
}

static void leBroadcastManager_StopSyncToPaSource(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_StopSyncToPaSource source_id=0x%x", broadcast_source->source_id);
    
    leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_cancelling_sync_to_pa);
    LeBapBroadcastSink_StopSyncPaSource();
}

static bool leBroadcastManager_UpdateStartSyncToPa(broadcast_source_state_t * broadcast_source)
{
    bool start_sync_to_pa = FALSE;

    DEBUG_LOG("leBroadcastManager_UpdateStartSyncToPa source_id 0x%x audio_active %d target_pa_sync_state enum:scan_delegator_client_pa_sync_t:%u current_pa_sync_state enum:scan_delegator_server_pa_sync_state_t:%u",
              broadcast_source->source_id,
              leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive(),
              leBroadcastManager_GetTargetPaSyncState(broadcast_source),
              leBroadcastManager_GetCurrentPaSyncState(broadcast_source));

    if (   !leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive()
        && leBroadcastManager_IsClientPaSyncStateSynchronizeToPa(leBroadcastManager_GetTargetPaSyncState(broadcast_source))
        && (leBroadcastManager_GetCurrentPaSyncState(broadcast_source) == scan_delegator_server_pa_sync_state_no_sync_to_pa)
        )
    {
        if (LeBroadcastManager_ScanDelegatorIsPastAvailable(broadcast_source))
        {
            /* Start timer for receiving PAST syncinfo before updating to No PAST state */
            MessageSendLater(&broadcast_source->task, BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT,
                             0,
                             LeBroadcastManager_GetPastTimeoutMs());

            TimestampEvent(TIMESTAMP_EVENT_LE_BROADCAST_START_PAST_TIMER);

            leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_syncinfo_request);

            leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_waiting_for_sync_to_pa);

            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStartSyncToPa wait for PAST");
        }
        else
        {
            /* Self scan for PA as PAST not supported */
            leBroadcastManager_StartScanForPaSource(broadcast_source->source_id);

            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStartSyncToPa scanning for PA");
        }
        start_sync_to_pa = TRUE;
    }
    
    return start_sync_to_pa;
}

static uint8 leBroadcastManager_GetBisLocation(uint8 max_bis, uint8 *bis_locations, uint32 bis_index)
{
    uint8 num_bis, bis_location;

    for (num_bis = 0, bis_location = 1; num_bis < max_bis && bis_index != 0; bis_location++)
    {
        if (bis_index & 0x1)
        {
            bis_locations[num_bis] = bis_location;
            num_bis++;
        }
    
        bis_index >>= 1;
    }

    return num_bis;
}

static bool leBroadcastManager_CreateSyncToBisReq(broadcast_source_state_t * broadcast_source, uint32 bis_index)
{
    uint8 num_bis = 0;
    uint8 bis[broadcast_manager_bis_location_max];
    uint8 big_encryption = leBroadcastManager_GetBigEncryption(broadcast_source);
    uint8 *broadcast_code = NULL;
    bool result = FALSE;
    
    if (big_encryption && !LeBroadcastManager_ScanDelegatorIsBroadcastCodeSet(broadcast_source->source_id))
    {
        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_CreateSyncToBisReq no broadcast code");
        return FALSE;
    }

    broadcast_code = calloc(1, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
    PanicNull(broadcast_code);

    num_bis = leBroadcastManager_GetBisLocation(broadcast_manager_bis_location_max, bis, bis_index);

    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_CreateSyncToBisReq. Found num_bis:0x%x big_encryption:0x%x", num_bis, big_encryption);

    if(num_bis != 0)
    {
        /* todo Need to sync to bis_index. Call into Broadcast Sink.
                    If this is BIS_SYNC_NO_PREFERENCE (No preference), can sync to any BIS in BIG.
                    Assuming some checking has been done to select certain BIS by this point.
        */
        leBroadcastManager_SetBigHandle(broadcast_source, BROADCAST_MANAGER_BIG_HANDLE);
        
        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_CreateSyncToBisReq. Creating BIG Sync with bis_index:0x%x", bis[0]);
        
        if (big_encryption)
        {
            uint8 * bass_code = LeBapScanDelegator_GetBroadcastCode(broadcast_source->source_id);
            if (bass_code)
            {
                memcpy(broadcast_code, bass_code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
                free(bass_code);
            }
        }
#ifdef USE_SYNERGY
        BapServerBroadcastBigCreateSyncReq(TrapToOxygenTask(LeBroadcastManager_SourceGetTask()),
                                            leBapScanDelegator_GetBapProfileHandle(),
                                            leBroadcastManager_GetPaSyncHandle(broadcast_source),
                                            BROADCAST_MANAGER_BIG_SYNC_TIMEOUT,
                                            BROADCAST_MANAGER_BIG_HANDLE,
                                            BROADCAST_MANAGER_BIG_SYNC_MSE,
                                            big_encryption,
                                            broadcast_code,
                                            num_bis,
                                            bis);
#else
        BapServerBroadcastBigCreateSyncReq(LeBroadcastManager_SourceGetTask(),
                                            leBapScanDelegator_GetBapProfileHandle(),
                                            leBroadcastManager_GetPaSyncHandle(broadcast_source),
                                            BROADCAST_MANAGER_BIG_SYNC_TIMEOUT,
                                            BROADCAST_MANAGER_BIG_HANDLE,
                                            BROADCAST_MANAGER_BIG_SYNC_MSE,
                                            big_encryption,
                                            broadcast_code,
                                            num_bis,
                                            bis);
#endif
        result = TRUE;
    }
                                        
    free(broadcast_code);
    
    return result;
}

static void leBroadcastManager_CreateSyncToBisCfm(hci_status status, uint8 numBis, uint16 *bis_handles)
{
    broadcast_source_state_t *broadcast_source;
    le_broadcast_manager_bis_index_t bis_index = {0};
    uint8 bis_loc;

    PanicFalse(leBroadcastManager_GetSyncState() == broadcast_manager_sync_syncing_to_bis);
    
    broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());

    if (broadcast_source != NULL)
    {
        if (status == hci_success)
        {
            bool sync_state;
            BapServerSetupDataPathReq data_params = {0};

            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_CreateSyncToBisCfm Setting up data path with bis_handle:0x%x", bis_handles[0]);

            if (leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source) == 0)
            {
                leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            }
            leBroadcastManager_SetCurrentBisSyncState(broadcast_source, leBroadcastManager_GetPendingBisSyncState());

            if (leBroadcastManager_GetBigEncryption(broadcast_source) != 0)
            {
                leBroadcastManager_SetCurrentBigEncryptionState(broadcast_source, scan_delegator_server_big_encryption_decrypting);
            }

            sync_state = leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source) != 0;

            if (sync_state)
            {
                leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_synced_to_bis_create_iso);
            }

            data_params.dataPathDirection = ISOC_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST;
            data_params.dataPathId = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
            numBis = numBis > broadcast_manager_bis_location_max ? broadcast_manager_bis_location_max : numBis;

            for (bis_loc = 0; bis_loc < numBis; bis_loc++)
            {
                leBroadcastManager_SetBisHandle(broadcast_source, bis_handles[bis_loc], bis_loc);
                if (sync_state)
                {
                    data_params.isoHandle = bis_handles[bis_loc];
                    BapServerSetupIsoDataPathReq(leBapScanDelegator_GetBapProfileHandle(), BAP_ISO_BROADCAST, &data_params);
                }
            }

            if (SCAN_DELEGATOR_SOURCE_ID_INVALID == leBroadcastManager_GetTargetBisSourceId())
            {
                leBroadcastManager_SetTargetBisSourceId(broadcast_source->source_id);
                LeBroadcastManager_SyncSendSyncToSource(broadcast_source->source_id);
            }
            leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource();
        }
        else
        {
            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_CreateSyncToBisCfm Failed hci  status 0x%x", status);
            leBroadcastManager_SetCurrentBisSyncStateFailedSync(broadcast_source);
            leBroadcastManager_SetCurrentBisSyncStateNoSync(broadcast_source);
            for (bis_loc = 0; bis_loc < broadcast_manager_bis_location_max; bis_loc++)
            {
                leBroadcastManager_SetBaseBisIndex(broadcast_source, &bis_index, bis_loc);
            }

            if (leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source) != 0)
            {
                leBroadcastManager_SetTargetBisSyncStateNoPreference(broadcast_source);
            }
            leBroadcastManager_SetBigHandle(broadcast_source, 0);
            leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            leBroadcastManager_SourceModifiedEvent(broadcast_source);
        }
        leBroadcastManager_ResetPendingBisSyncState();
    }
}

static bool leBroadcastManager_TerminateSyncToPaReq(uint16 sync_handle)
{
    if (sync_handle)
    {
#ifndef USE_SYNERGY
        PanicFalse(ConnectionUpdateTaskToSyncHandleAssociation(sync_handle, LeBroadcastManager_SourceGetTask()) == success);
#endif
        ConnectionDmBlePeriodicScanSyncTerminateReq(LeBroadcastManager_SourceGetTask(), sync_handle);
        return TRUE;
    }
    
    return FALSE;
}

static bool leBroadcastManager_TerminateSyncToBisReq(broadcast_source_state_t * broadcast_source)
{
    uint8 big_handle = leBroadcastManager_GetBigHandle(broadcast_source);
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_TerminateSyncToBisReq big_handle:0x%x", big_handle);
    if (big_handle)
    {
        BapServerBroadcastBigTerminateSyncReq(leBapScanDelegator_GetBapProfileHandle(), big_handle);
        return TRUE;
    }
    
    return FALSE;
}

static bool leBroadcastManager_IsMandatoryCodecConfigurationSet(broadcast_source_state_t * broadcast_source)
{
    if ((leBroadcastManager_GetBroadcastSourceSampleRate(broadcast_source) != 0) &&
        (leBroadcastManager_GetBroadcastSourceFrameDuration(broadcast_source) != 0) &&
        (leBroadcastManager_GetBroadcastSourceOctetsPerFrame(broadcast_source) != 0)
    )
    {
        return TRUE;
    }
    
    return FALSE;
}

/* Are we looking to sync to a different broadcast source to the currently active one?
   If so stop the sync on the current source and start syncing the new one */
static bool leBroadcastManager_ActiveSourceSwapNeeded(broadcast_source_state_t * new_broadcast_source)
{
    bool stop_needed = FALSE;
    broadcast_source_state_t *active_bss = LeBroadcastManager_GetSourceOfActiveBis();

    DEBUG_LOG("leBroadcastManager_ActiveSourceSwapNeeded new_source_id 0x%x active_source_id 0x%x stop_tasks %p",
              LeBroadcastManager_SourceGetSourceId(new_broadcast_source),
              LeBroadcastManager_SourceGetSourceId(active_bss),
              le_broadcast_manager.stop_tasks);

    /* Check there is an active BIS and we aren't in the process of stopping */
    if (active_bss && !le_broadcast_manager.stop_tasks)
    {
        uint32 new_bis_sync_state = leBroadcastManager_GetTargetBisSyncStateValue(new_broadcast_source);
        uint32 active_bis_sync_state = leBroadcastManager_GetCurrentBisSyncState(active_bss);
        uint8 target_bis_source_id = leBroadcastManager_GetTargetBisSourceId();

        /* Is the active BIS source different to the source that we're looking to start */
        if (active_bss->source_id != new_broadcast_source->source_id)
        {
            /* If the target BIS source id is the new source_id or 'none' then allow switch
               to the new source. */
            if (   (target_bis_source_id != SCAN_DELEGATOR_SOURCE_ID_INVALID)
                && (target_bis_source_id != active_bss->source_id))
            {
                /* If the active source is synced to BIS and the new one wants to replace it
                   stop the active source and start the new one */
                if ((new_bis_sync_state != 0) && (active_bis_sync_state != 0))
                {
                    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_ActiveSourceSwapNeeded stopping source_id %d, starting source_id %d",
                                                  active_bss->source_id,
                                                  new_broadcast_source->source_id);

                    /* store the source id to start after the current one is stopped */
                    le_broadcast_manager.source_to_start = new_broadcast_source->source_id;

                    /* Stop syncing the current BIS */
                    LeBroadcastManager_Stop(LeBroadcastManager_SourceGetTask());
                    stop_needed = TRUE;
                }
            }
        }
    }
    return stop_needed;
}

static bool leBroadcastManager_UpdateStartSyncToBis(broadcast_source_state_t * broadcast_source)
{
    bool start_sync_to_bis = FALSE;
    uint32 current_bis_sync_state = leBroadcastManager_GetCurrentBisSyncState(broadcast_source);
    uint32 target_bis_sync_state = leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source);
    
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStartSyncToBis. source_id %u pa_sync_state=0x%x current_bis_sync_state=0x%x target_bis_sync_state=0x%x",
                                    broadcast_source->source_id,
                                    leBroadcastManager_GetCurrentPaSyncState(broadcast_source),
                                    current_bis_sync_state, 
                                    target_bis_sync_state);
    
    if (leBroadcastManager_GetCurrentPaSyncState(broadcast_source) == scan_delegator_server_pa_sync_state_sync_to_pa)
    {
        if (!LeBroadcastManager_IsPaused() &&
            (current_bis_sync_state != target_bis_sync_state) &&
            (target_bis_sync_state != 0) &&
            (target_bis_sync_state != BIS_SYNC_NO_PREFERENCE) &&
            leBroadcastManager_IsMandatoryCodecConfigurationSet(broadcast_source)
        )
        {
            if (leBroadcastManager_CreateSyncToBisReq(broadcast_source, target_bis_sync_state))
            {
                BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStartSyncToBis. Creating sync");
                leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_syncing_to_bis);
                leBroadcastManager_SetPendingBisSyncState(leBroadcastManager_GetTargetBisSyncState(broadcast_source));
                start_sync_to_bis = TRUE;
            }
        }
    }
    
    return start_sync_to_bis;
}

static bool leBroadcastManager_UpdateStopSyncToPa(broadcast_source_state_t * broadcast_source)
{
    bool stop_sync_to_pa = FALSE;
    
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStopSyncToPa. source_id %u current_pa_sync_state=enum:scan_delegator_server_pa_sync_state_t:%u target_pa_sync_state=enum:scan_delegator_server_pa_sync_state_t:%u",
                                 broadcast_source->source_id,
                                 leBroadcastManager_GetCurrentPaSyncState(broadcast_source),
                                 leBroadcastManager_GetTargetPaSyncState(broadcast_source));
    
    if (leBroadcastManager_GetTargetPaSyncState(broadcast_source) == le_bm_pa_sync_none)
    {
        switch (leBroadcastManager_GetCurrentPaSyncState(broadcast_source))
        {
            case scan_delegator_server_pa_sync_state_sync_to_pa:
                if (leBroadcastManager_TerminateSyncToPaReq(leBroadcastManager_GetPaSyncHandle(broadcast_source)))
                {
                    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStopSyncToPa. Terminating sync");
                    leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_stopping_sync_to_pa);
                    stop_sync_to_pa = TRUE;
                }
                break;
                
            case scan_delegator_server_pa_sync_state_fail_sync_to_pa:
            case scan_delegator_server_pa_sync_state_no_past:
                BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStopSyncToPa. Failed sync resetting to no sync");
                leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_no_sync_to_pa);
                stop_sync_to_pa = TRUE;
                break;

            default:
                break;
        }
    }
    
    return stop_sync_to_pa;
}

static bool leBroadcastManager_UpdateStopSyncToBis(broadcast_source_state_t * broadcast_source)
{
    bool stop_sync_to_bis = FALSE;
    uint32 current_bis_sync_state = leBroadcastManager_GetCurrentBisSyncState(broadcast_source);
    uint32 target_bis_sync_state = leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source);
    
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateStopSyncToBis source_id %u current_bis_sync_state=0x%x target_bis_sync_state=0x%x",
                                 broadcast_source->source_id, current_bis_sync_state, target_bis_sync_state);
     
    if ((target_bis_sync_state != current_bis_sync_state) && (current_bis_sync_state != 0) && (target_bis_sync_state != BIS_SYNC_NO_PREFERENCE))
    {
        if (leBroadcastManager_TerminateSyncToBisReq(broadcast_source))
        {
            stop_sync_to_bis = TRUE;
            leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_stopping_sync_to_bis);
        }
    }
    
    return stop_sync_to_bis;
}

static bool leBroadcastManager_AnyDifferentSourcesAdded(broadcast_source_state_t * broadcast_source)
{
    bool different_sources_added = FALSE;
    
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        if (leBroadcastManager_IsSourceValid(&le_broadcast_manager.broadcast_source_receive_state[i]))
        {
            if (&le_broadcast_manager.broadcast_source_receive_state[i] != broadcast_source)
            {
                different_sources_added = TRUE;
            }
        }
    }
    
    return different_sources_added;
}

static bool leBroadcastManager_UpdateSync(broadcast_source_state_t * broadcast_source)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_UpdateSync source_id 0x%x sync_state 0x%x",
                                 LeBroadcastManager_SourceGetSourceId(broadcast_source),
                                 leBroadcastManager_GetSyncState());
    
    if (leBroadcastManager_GetSyncState() != broadcast_manager_sync_none)
    {
        return TRUE;
    }

    if(leBroadcastManager_ActiveSourceSwapNeeded(broadcast_source))
    {
        return TRUE;
    }

    if (leBroadcastManager_UpdateStopSyncToBis(broadcast_source))
    {
        return TRUE;
    }
    
    if (leBroadcastManager_UpdateStopSyncToPa(broadcast_source))
    {
        return TRUE;
    }
    
    if (leBroadcastManager_UpdateStartSyncToPa(broadcast_source))
    {
        return TRUE;
    }
    
    if (leBroadcastManager_UpdateStartSyncToBis(broadcast_source))
    {
        return TRUE;
    }

    /* If the code reaches this point, it means the functions above returned
    FALSE and the manager has reached the target state and no further actions
    are required. Having reached the target state the code checks if any stop
    or remove requests can be completed. */

    leBroadcastManager_CompleteStop();
    if (broadcast_source->remove_pending)
    {
        leBroadcastManager_AttemptToSourceRemove(broadcast_source);
    }

    return FALSE;
}

static void leBroadcastManager_SourceModifiedEvent(broadcast_source_state_t * broadcast_source)
{
    DEBUG_LOG("leBroadcastManager_SourceModifiedEvent source_id 0x%x", LeBroadcastManager_SourceGetSourceId(broadcast_source));

    if (!leBroadcastManager_UpdateSync(broadcast_source))
    {
        /* If here can check other broadcast sources to see if there is any syncing to be started/stopped */
        if (leBroadcastManager_AnyDifferentSourcesAdded(broadcast_source))
        {
            LeBroadcastManager_InitiateSyncCheckAllSources();
        }
    }
}

static void leBroadcastManager_SourceBroadcastCodeEvent(broadcast_source_state_t * broadcast_source)
{
    scan_delegator_server_big_encryption_t enc_state = leBroadcastManager_GetCurrentBigEncryptionState(broadcast_source);
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_SourceBroadcastCodeEvent enum:scan_delegator_server_big_encryption_t:%d", enc_state);
    if (enc_state == scan_delegator_server_big_encryption_broadcast_code_needed)
    {
        leBroadcastManager_SetCurrentBigEncryptionState(broadcast_source, scan_delegator_server_big_encryption_decrypting);
    }
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

static void leBroadcastManager_ConvertBisIndexTypeToBisSyncStateType(uint8 num_subgroups, const le_broadcast_manager_bis_index_t *bis_index, le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    bis_sync_state->num_subgroups = num_subgroups;
    if (bis_sync_state->num_subgroups)
    {
        bis_sync_state->bis_sync = PanicUnlessMalloc(sizeof(uint32) * bis_sync_state->num_subgroups);
        for (uint8 i=0; i<bis_sync_state->num_subgroups; i++)
        {
            if (i == bis_index->subgroup)
            {
                bis_sync_state->bis_sync[i] = bis_index->index;
            }
            else
            {
                bis_sync_state->bis_sync[i] = 0;
            }
        }
    }
    else
    {
        bis_sync_state->bis_sync = NULL;
    }
}

static void leBroadcastManager_ConvertSubgroupsTypeToBisSyncStateType(uint8 num_subgroups, const le_bm_source_subgroup_t *subgroups, le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    bis_sync_state->num_subgroups = num_subgroups;
    if (bis_sync_state->num_subgroups)
    {
        bis_sync_state->bis_sync = PanicUnlessMalloc(sizeof(uint32) * bis_sync_state->num_subgroups);
        for (uint8 i=0; i<bis_sync_state->num_subgroups; i++)
        {
            bis_sync_state->bis_sync[i] = subgroups[i].bis_sync;
        }
    }
    else
    {
        bis_sync_state->bis_sync = NULL;
    }
}

static le_broadcast_manager_bis_index_t leBroadcastManagerSource_GetBisIndexToRender(const broadcast_source_state_t *broadcast_source)
{
    le_broadcast_manager_bis_index_t bis_index = {0};
    uint8 bis_loc;

    /* If the device is one side of a pair, and it has the audio location of both sides,
            only select the location for this side.
    */
    if (!Multidevice_IsDeviceStereo())
    {
        if (Multidevice_IsLeft())
        {
            bis_loc = ((broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.index != 0) ? 
                       broadcast_manager_bis_location_left_or_stereo : broadcast_manager_bis_location_right);
        }
        else
        {
            bis_loc = ((broadcast_source->bis_info[broadcast_manager_bis_location_right].base_bis_index.index != 0) ? 
                       broadcast_manager_bis_location_right : broadcast_manager_bis_location_left_or_stereo);
        }

        if (broadcast_source->bis_info[bis_loc].base_bis_index.index != 0)
        {
            bis_index.subgroup = broadcast_source->bis_info[bis_loc].base_bis_index.subgroup;
            bis_index.index = 1u << (broadcast_source->bis_info[bis_loc].base_bis_index.index - 1);
        }
    }
    else
    {
        bis_index.subgroup =  broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.subgroup;

        for (bis_loc = 0; bis_loc < broadcast_manager_bis_location_max; bis_loc++)
        {
            if (broadcast_source->bis_info[bis_loc].base_bis_index.index != 0)
            {
                bis_index.index |= (1u << ((broadcast_source->bis_info[bis_loc].base_bis_index.index) - 1));
            }
        }
    }

    return bis_index;
}

static void leBroadcastManager_SelectTargetBisIndexAndSync(broadcast_source_state_t * broadcast_source, const le_broadcast_manager_bis_index_t *bis_index_to_sync)
{
    le_broadcast_manager_bis_sync_state_t bis_sync_state = {0};
    uint8 num_subgroups = leBroadcastManager_GetBroadcastSourceNumberOfSubgroups(broadcast_source);

    leBroadcastManager_ConvertBisIndexTypeToBisSyncStateType(num_subgroups, bis_index_to_sync, &bis_sync_state);
    leBroadcastManager_SetTargetBisSyncState(broadcast_source, bis_sync_state.num_subgroups, bis_sync_state.bis_sync);
    if (bis_sync_state.bis_sync)
    {
        /* free memory created by leBroadcastManager_ConvertBisIndexTypeToBisSyncStateType() */
        free(bis_sync_state.bis_sync);
    }
    leBroadcastManager_SourceModifiedEvent(broadcast_source);
}

static void leBroadcastManager_ResetBroadcastSourceState(broadcast_source_state_t * broadcast_source)
{
    if (le_broadcast_manager.paused_bis_source_id == broadcast_source->source_id)
    {
        leBroadcastManager_ResetPausedState();
    }
    leBroadcastManager_ResetPaSyncLostState(broadcast_source);
    leBroadcastManager_ResetTargetBisSyncState(broadcast_source);
    LeBroadcastManager_ResetRequestedBisSyncState(broadcast_source);
    LeBroadcastManager_ClearCurrentSourceAddr(broadcast_source);
    memset(broadcast_source, 0, sizeof(broadcast_source_state_t));
    broadcast_source->source_id = SCAN_DELEGATOR_SOURCE_ID_INVALID;
    broadcast_source->task.handler = LeBroadcastManager_MsgHandlerLeabmMessages;
    BdaddrTypedSetEmpty(&broadcast_source->assistant_address);
    BdaddrTypedSetEmpty(&broadcast_source->source_match_address);
}

static void leBroadcastManager_QueueSourceAddAfterRemove(broadcast_source_state_t * remove_broadcast_source, scan_delegator_client_add_broadcast_source_t * add_broadcast_source)
{
    /* Process Add Source operation after the Remove Source operations are complete. */
    scan_delegator_client_add_broadcast_source_t *pending_add_source = leBroadcastManager_ScanDelegatorCopyClientAddSource(add_broadcast_source);
                
    MessageSendConditionally(LeBroadcastManager_SourceGetTask(), BROADCAST_MANAGER_INTERNAL_MSG_ADD_SOURCE, pending_add_source, &remove_broadcast_source->remove_pending);
}

static void leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource(void)
{
    broadcast_source_state_t *target_bis_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetTargetBisSourceId());
    broadcast_source_state_t * active_bis_source = LeBroadcastManager_GetSourceOfActiveBis();

    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource active source_id:%u target source_id:%u",
                                    active_bis_source ? active_bis_source->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID,
                                    target_bis_source ? target_bis_source->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID);

    if ((target_bis_source != NULL) && (active_bis_source == NULL))
    {
        /* Sync to target_bis_source. */
        BROADCAST_MANAGER_SOURCE_LOG("  Sync to target source_id %u", target_bis_source->source_id);

        /* Make sure the target PA sync state is not 'none' otherwise the
           broadcast manager will not sync to it. */
        if (leBroadcastManager_GetTargetPaSyncState(target_bis_source) == le_bm_pa_sync_none)
        {
            leBroadcastManager_SetTargetPaSyncState(target_bis_source, le_bm_pa_sync_past_not_available);
        }

        /* Set the target bis sync state for the next source. */
        const le_broadcast_manager_bis_sync_state_t *bis_sync_state = NULL;
        bis_sync_state = LeBroadcastManager_GetRequestedBisSyncState(target_bis_source);
        leBroadcastManager_SetTargetBisSyncState(target_bis_source, bis_sync_state->num_subgroups, bis_sync_state->bis_sync);

        leBroadcastManager_SourceModifiedEvent(target_bis_source);
    }
    else if ((target_bis_source == NULL) && (active_bis_source != NULL))
    {
        /* Stop syncing to active_bis_source. */
        BROADCAST_MANAGER_SOURCE_LOG("  Stop syncing to active source_id %u", active_bis_source->source_id);

        leBroadcastManager_SetTargetBisSyncStateNoSync(active_bis_source);
        leBroadcastManager_SetSyncToBisNoPreference(active_bis_source, TRUE);

        leBroadcastManager_SourceModifiedEvent(active_bis_source);
    }
    else if (   (target_bis_source != NULL)
             && (active_bis_source != NULL)
             && (target_bis_source != active_bis_source))
    {
        /* target_bis_source != NULL && active_bis_source != NULL : Stop syncing to active_bis_source and sync to target_bis_source. */
        BROADCAST_MANAGER_SOURCE_LOG("  Switch from active source_id %u to target source_id %u",
                                     active_bis_source->source_id, target_bis_source->source_id);

        /* Set the target bis sync state for the next source. */
        const le_broadcast_manager_bis_sync_state_t *bis_sync_state = NULL;
        bis_sync_state = LeBroadcastManager_GetRequestedBisSyncState(target_bis_source);
        leBroadcastManager_SetTargetBisSyncState(target_bis_source, bis_sync_state->num_subgroups, bis_sync_state->bis_sync);

        leBroadcastManager_SourceModifiedEvent(target_bis_source);
    }
    else
    {
        /* Do nothing */
        BROADCAST_MANAGER_SOURCE_LOG("  No change needed");
    }
}

static bool leBroadcastSource_GetActiveSourceColocated(bool *colocated)
{
    broadcast_source_state_t *broadcast = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast)
    {
        tp_bdaddr tpaddr;

        typed_bdaddr taddr = leBroadcastManager_GetSourceTypedBdaddr(broadcast);

        BdaddrTpFromTypedAndFlags(&tpaddr, &taddr, DM_ACL_FLAG_ULP);

        *colocated = ConManagerIsTpConnected(&tpaddr);

        return TRUE;
    }
    return FALSE;
}

bool LeBroadcastSource_IsActiveSourceColocated(void)
{
    bool colocated;

    if (leBroadcastSource_GetActiveSourceColocated(&colocated))
    {
        return colocated;
    }
    return FALSE;
}

bool LeBroadcastSource_IsActiveSourceNonColocated(void)
{
    bool colocated;

    if (leBroadcastSource_GetActiveSourceColocated(&colocated))
    {
        return !colocated;
    }
    return FALSE;
}

uint16 LeBroadcastSource_GetActiveSourceIsoInterval(void)
{
    broadcast_source_state_t *broadcast = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast)
    {
        return broadcast->iso_interval;
    }
    return 0;
}

bool LeBroadcastManager_GetActiveSourceId(uint8 *src_id)
{
    broadcast_source_state_t *broadcast = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast != NULL)
    {
        *src_id = broadcast->source_id;
        return TRUE;
    }

    return FALSE;
}

void LeBroadcastManager_SourceInit(void)
{
    broadcast_source_state_t * broadcast_source = NULL;
    
    memset(&le_broadcast_manager, 0, sizeof(le_broadcast_manager_t));
    
    le_broadcast_manager.task.handler = LeBroadcastManager_MsgHandlerLeabmMessages;
    leBroadcastManager_SetTargetBisSourceId(SCAN_DELEGATOR_SOURCE_ID_INVALID);
    
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        broadcast_source = &le_broadcast_manager.broadcast_source_receive_state[i];
        leBroadcastManager_ResetBroadcastSourceState(broadcast_source);
    }

    LeBroadcastManager_SetPastTimeoutMs(BROADCAST_MANAGER_PAST_TIMEOUT_MS);
    LeBroadcastManager_SetFindTrainsTimeout(BROADCAST_SINK_FIND_TRAINS_TIMEOUT_MS);

    /* Temporarily use the hfp_profile notifications to know when a call
       starts and ends. Use av status notifications for when A2DP media
       streaming starts and end and use LE Unicast notification when unicast streaming
       starts and ends.

       On the Secondary use mirror_profile notifications for the call and media
       start and end.

       Eventually this behaviour will be driven by the audio router pausing
       and resuming broadcast when a call is active.*/
    appAvStatusClientRegister(LeBroadcastManager_SourceGetTask());
    MirrorProfile_ClientRegister(LeBroadcastManager_SourceGetTask());
    Telephony_RegisterForMessages(LeBroadcastManager_SourceGetTask());

#ifdef INCLUDE_LE_AUDIO_UNICAST
    /* register with LE unicast to receive notifications of unicast streaming and activity */
    LeAudioMessages_ClientRegister(LeBroadcastManager_SourceGetTask());
#endif

    LeBroadcastManager_LinkPolicyInit();

    if (LeBroadcastManager_IsHighPriorityBandwidthUser())
    {
        PanicFalse(BandwidthManager_RegisterFeature(BANDWIDTH_MGR_FEATURE_LE_BROADCAST, high_bandwidth_manager_priority, NULL));
    }

}

le_bm_bass_status_t LeBroadcastManager_AddSource(uint8 *source_id, scan_delegator_client_add_broadcast_source_t *new_source)
{
    le_bm_bass_status_t status = le_bm_bass_status_failed;

    broadcast_source_state_t * broadcast_source = NULL;
    le_broadcast_manager_bis_sync_state_t bis_sync_state = {0};

    broadcast_source = leBroadcastManager_GetFreeSourceState();
    if (broadcast_source != NULL)
    {
        status = LeBroadcastManager_ScanDelegatorWriteClientAddSource(source_id, new_source);

        if (status == le_bm_bass_status_success)
        {
            broadcast_source->source_id = *source_id;

            leBroadcastManager_ConvertSubgroupsTypeToBisSyncStateType(new_source->num_subgroups, new_source->subgroups, &bis_sync_state);

            leBroadcastManager_SetTargetPaSyncState(broadcast_source, new_source->pa_sync);
            leBroadcastManager_SetTargetBisSyncState(broadcast_source, bis_sync_state.num_subgroups, bis_sync_state.bis_sync);
            LeBroadcastManager_SetRequestedBisSyncState(broadcast_source, &bis_sync_state);

            if (bis_sync_state.bis_sync)
            {
                /* free memory created by leBroadcastManager_ConvertSubgroupsTypeToBisSyncStateType() */
                free(bis_sync_state.bis_sync);
            }
            LeBroadcastManager_SetAssistantAddress(broadcast_source, new_source->assistant_address);
            BdaddrTypedSetEmpty(&broadcast_source->source_match_address);

            if (   (SCAN_DELEGATOR_SOURCE_ID_INVALID != leBroadcastManager_GetTargetBisSourceId())
                && (le_bm_pa_sync_none != new_source->pa_sync))
            {
                /* Set this source as the target bis source if the target pa sync != 'don't sync' */
                leBroadcastManager_SetTargetBisSourceId(broadcast_source->source_id);
            }

            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_AddSource: new source_id 0x%x", broadcast_source->source_id);

            leBroadcastManager_SourceModifiedEvent(broadcast_source);

            LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(broadcast_source->source_id);
        }
    }

    return status;
}

le_bm_bass_status_t LeBroadcastManager_RemoveSource(uint8 source_id)
{
    le_bm_bass_status_t status = le_bm_bass_status_invalid_source_id;

    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);

    if (broadcast_source)
    {
        status = LeBroadcastManager_ScanDelegatorRemoveSourceState(source_id);

        if (status == le_bm_bass_status_success)
        {
            LeScanManager_Stop(&le_broadcast_manager.task);

            if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_waiting_for_sync_to_pa) &&
                (leBroadcastManager_GetSyncStateSourceId() == source_id))
            {
                MessageCancelFirst(&broadcast_source->task, BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT);
                leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            }

            leBroadcastManager_ResetBroadcastSourceState(broadcast_source);
            leBroadcastManager_CompleteRemoveAllSources();

            LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(source_id);
        }
    }
    else
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_RemoveSource: no source with id %u", source_id);
    }

    return status;
}

void LeBroadcastManager_SourceAdd(scan_delegator_client_add_broadcast_source_t * new_source)
{
    broadcast_source_state_t * broadcast_source = NULL;
    
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceAdd broadcast_id 0x%x", new_source->broadcast_id);
    BROADCAST_MANAGER_SOURCE_LOG("  source_adv_sid %u", new_source->source_adv_sid);
    BROADCAST_MANAGER_SOURCE_LOG("  pa_sync enum:le_bm_pa_sync_t:%u", new_source->pa_sync);
    BROADCAST_MANAGER_SOURCE_LOG("  pa_interval %u", new_source->pa_interval);
    
    broadcast_source = leBroadcastManager_FindAnyRemoveSourcePending();
    if (broadcast_source != NULL)
    {
        leBroadcastManager_QueueSourceAddAfterRemove(broadcast_source, new_source);
    }
    else
    {
        if (leBroadcastManager_AnyFreeSourceId())
        {
            uint8 source_id = SCAN_DELEGATOR_SOURCE_ID_INVALID;
            PanicFalse(LeBroadcastManager_AddSource(&source_id, new_source) == le_bm_bass_status_success);
        }
        else
        {
            /* todo could remove an unused Source and store new one */
        }
    }
}

le_bm_bass_status_t LeBroadcastManager_SourceModify(scan_delegator_client_modify_broadcast_source_t * new_source)
{
    le_bm_bass_status_t status = le_bm_bass_status_invalid_source_id;
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(new_source->source_id);
    le_broadcast_manager_bis_sync_state_t bis_sync_state = {0};

    if (broadcast_source != NULL)
    {
        leBroadcastManager_ConvertSubgroupsTypeToBisSyncStateType(new_source->num_subgroups, new_source->subgroups, &bis_sync_state);

        /* Don't automatically resync to bis when assistant disconnected if sync 
           explicitly turned off */
        broadcast_source->dont_sync_bis = (leBroadcastManager_GetBisSyncStateValue(&bis_sync_state) == 0);

        leBroadcastManager_SetTargetPaSyncState(broadcast_source, new_source->pa_sync);
        leBroadcastManager_SetTargetBisSyncState(broadcast_source, bis_sync_state.num_subgroups, bis_sync_state.bis_sync);
        LeBroadcastManager_SetRequestedBisSyncState(broadcast_source, &bis_sync_state);
        
        /* If the broadcast is currently paused and this source was active at the time,
            update the BIS sync state to use when the broadcast is resumed */
        if (LeBroadcastManager_IsPaused() &&
            new_source->source_id == le_broadcast_manager.paused_bis_source_id)
        {
            leBroadcastManager_ResetPausedState();
            leBroadcastManager_SetPausedState(new_source->source_id, &bis_sync_state);
        }
        
        if (bis_sync_state.bis_sync)
        {
            /* free memory created by leBroadcastManager_ConvertSubgroupsTypeToBisSyncStateType() */
            free(bis_sync_state.bis_sync);
        }

        if (!BdaddrTypedIsEmpty(&new_source->assistant_address))
        {
            LeBroadcastManager_SetAssistantAddress(broadcast_source, new_source->assistant_address);
        }

        DEBUG_LOG("LeBroadcastManager_SourceModify target_bis_source_id 0x%x requested_pa_sync enum:scan_delegator_client_pa_sync_t:%u",
                  leBroadcastManager_GetTargetBisSourceId(), new_source->pa_sync);
        
        if (new_source->pa_sync == le_bm_pa_sync_none)
        {
            if (leBroadcastManager_IsInitiatingScanForPA(broadcast_source))
            {
                LeBroadcastManager_StopScanForPaSource(broadcast_source->source_id, FALSE);
            }
        
            if (leBroadcastManager_IsInitiatingSyncToPA(broadcast_source))
            {
                leBroadcastManager_StopSyncToPaSource(broadcast_source);
            }
        }
        else
        {
            leBroadcastManager_SetTargetBisSourceId(broadcast_source->source_id);
        }
    
        LeBroadcastManager_ScanDelegatorWriteClientModifySource(broadcast_source->source_id, new_source);
        
        leBroadcastManager_SourceModifiedEvent(broadcast_source);

        status = le_bm_bass_status_success;
    }

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceModify source_id=%u source=%p result=enum:le_bm_bass_status_t:%d",
                                 new_source->source_id, broadcast_source, status);

    return status;
}

static void leBroadcastManager_AttemptToSourceRemove(broadcast_source_state_t * broadcast_source)
{
    /* If attempting to sync to PA, wait for this operation to complete first. */
    if (leBroadcastManager_IsInitiatingScanForPA(broadcast_source) ||
        leBroadcastManager_IsInitiatingSyncToPA(broadcast_source))
    {
        leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_none);

        if (leBroadcastManager_IsInitiatingScanForPA(broadcast_source))
        {
            LeBroadcastManager_StopScanForPaSource(broadcast_source->source_id, FALSE);
        }
        
        if (leBroadcastManager_IsInitiatingSyncToPA(broadcast_source))
        {
            leBroadcastManager_StopSyncToPaSource(broadcast_source);
        }
        
        broadcast_source->remove_pending = TRUE;
        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_AttemptToSourceRemove pending PA sync");
        
        return;
    }
    
    if ((leBroadcastManager_GetCurrentPaSyncState(broadcast_source) != scan_delegator_server_pa_sync_state_sync_to_pa) &&
        (leBroadcastManager_IsCurrentBisSyncStateIdle(broadcast_source)))
    {
        PanicFalse(LeBroadcastManager_RemoveSource(broadcast_source->source_id) == le_bm_bass_status_success);

        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_AttemptToSourceRemove complete");
    }
    else 
    {
        if (!LeBapScanDelegator_IsAnyClientConnected())
        {
            bool source_modified = FALSE;
            /* If no assistant is connected, the device can initiate the terminate of PA/BIG 
                sync before removing the source.
               If an assistant was connected, it is expected that it will terminate before trying to remove the source.
            */ 
            if (leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source) != 0)
            {
                leBroadcastManager_SetTargetBisSyncStateNoSync(broadcast_source);
                source_modified = TRUE;
            }
            if (leBroadcastManager_GetTargetPaSyncState(broadcast_source) != le_bm_pa_sync_none)
            {
                leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_none);
                source_modified = TRUE;
            }
            if (source_modified)
            {
                leBroadcastManager_SourceModifiedEvent(broadcast_source);
            }
            
            /* Not in correct state to remove the source, but PA/BIG sync is being terminated. Set the flag and try again later. */
            broadcast_source->remove_pending = TRUE;
            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_AttemptToSourceRemove pending terminate sync");
        }
    }
}

static broadcast_source_state_t * leBroadcastManager_FindAnyRemoveSourcePending(void)
{
    broadcast_source_state_t * broadcast_source = NULL;
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        broadcast_source = &le_broadcast_manager.broadcast_source_receive_state[i];
        if (leBroadcastManager_IsSourceValid(broadcast_source))
        {
            if (broadcast_source->remove_pending)
            {
                return broadcast_source;
            }
        }
    }
    
    return NULL;
}

static bool leBroadcastManager_IsInitiatingSyncToPA(broadcast_source_state_t *broadcast_source)
{
    if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_syncing_to_pa) &&
        (leBroadcastManager_GetSyncStateSourceId() == broadcast_source->source_id))
    {
        return TRUE;
    }
    
    return FALSE;
}

static bool leBroadcastManager_IsInitiatingScanForPA(broadcast_source_state_t *broadcast_source)
{
    if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_scanning_for_pa) &&
        (leBroadcastManager_GetSyncStateSourceId() == broadcast_source->source_id))
    {
        return TRUE;
    }
    
    return FALSE;
}

static void leBroadcastManager_SendMetadataNotification(uint16 data_length_adv, const uint8 *data_adv)
{
    AudioAnnouncementParserBaseData base_data;
    uint8 subgroup_count;
    uint8 metadata_len;

    AudioAnnouncementParserStatus status = AudioAnnouncementParserBasicAudioAnnouncementParsing(data_length_adv,
                                                                                                (uint8 *) data_adv,
                                                                                                &base_data);

    if (status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS)
    {
        for (subgroup_count = 0; subgroup_count < base_data.numSubgroups; subgroup_count++)
        {
            metadata_len = base_data.subGroupsData[subgroup_count].metadataLen - BROADCAST_MANAGER_METADATA_OFFSET_BYTES;

            if (base_data.subGroupsData[subgroup_count].metadata != NULL && metadata_len > 0)
            {
                LeAudioMessages_SendBroadcastMetadata(metadata_len,
                                                      base_data.subGroupsData[subgroup_count].metadata + BROADCAST_MANAGER_METADATA_OFFSET_BYTES);
            }
        }
     }

    leBroadcastManager_FreeBaseData(&base_data);
}

void LeBroadcastManager_SourceRemove(scan_delegator_client_remove_broadcast_source_t * new_source)
{
    broadcast_source_state_t * broadcast_source = LeBroadcastManager_GetSourceById(new_source->source_id);

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceRemove source_id 0x%x source %p",
                                 new_source->source_id, broadcast_source);
    
    if (broadcast_source != NULL)
    {
        leBroadcastManager_AttemptToSourceRemove(broadcast_source);
    }
}

void LeBroadcastManager_SourceSyncedToPA(scan_delegator_periodic_sync_t * sync)
{
    uint8 source_id = LeBapScanDelegator_GetServiceDataSourceId(sync->service_data);
    broadcast_source_state_t *broadcast_source = NULL;
    bool remove_sync = FALSE;
    
    if (source_id != SCAN_DELEGATOR_SOURCE_ID_INVALID)
    {
        broadcast_source = LeBroadcastManager_GetSourceById(source_id);
    }
    else
    {
        /* The source_adv_sid may not be unique for all Broadcast Sources,
           so only check it against the source we are syncing to. */
        broadcast_source_state_t *bss = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
        if (bss && (leBroadcastManager_GetSourceAdvSid(bss) == sync->source_adv_sid))
        {
            broadcast_source = bss;
        }
    }

    if(broadcast_source)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA sync_state:%d",
                                     leBroadcastManager_GetSyncState());
        if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_waiting_for_sync_to_pa)
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA PAST Time taken:%u",
                                     TimestampEvent_DeltaFrom(TIMESTAMP_EVENT_LE_BROADCAST_START_PAST_TIMER));
        }

        if ((leBroadcastManager_GetSyncStateSourceId() == broadcast_source->source_id) &&
            ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_waiting_for_sync_to_pa) ||
            (leBroadcastManager_GetSyncState() == broadcast_manager_sync_syncing_to_pa) ||
            (leBroadcastManager_GetSyncState() == broadcast_manager_sync_cancelling_sync_to_pa)))
        {
            MessageCancelFirst(&broadcast_source->task, BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT);
            
            if (sync->status == scan_delegator_status_success)
            {
                LeBroadcastManager_StoreCurrentSourceAddr(&sync->taddr_source);
            }
            
            leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            
            leBroadcastManager_ResetPaSyncLostState(broadcast_source);

            if (sync->status == scan_delegator_status_success)
            {
                leBroadcastManager_SetPaSyncHandle(broadcast_source, sync->sync_handle);
                if (leBroadcastManager_GetTargetPaSyncState(broadcast_source) == le_bm_pa_sync_none)
                {
                    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA remove_sync target is no PA sync");
                    remove_sync = TRUE;
                }
                else
                {
                    leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_sync_to_pa);
                    ConnectionDmBlePeriodicScanSyncAdvReportEnableReq(&le_broadcast_manager.task, broadcast_source->sync_handle, TRUE);
                }
            }
            else
            {
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA failed to sync to pa");
                leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_fail_sync_to_pa);
            }
        }
        else
        {
            remove_sync = TRUE;
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA remove_sync not waiting");
        }
    }
    else
    {
        remove_sync = TRUE;
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceSyncedToPA remove_sync no broadcast source source_id[0x%x]", source_id);
    }

    if (remove_sync && (sync->status == scan_delegator_status_success))
    {
        if (leBroadcastManager_TerminateSyncToPaReq(sync->sync_handle))
        {
            /* If this is unexpected, then may not be able to track the sync state if busy with another activity.
               There is a small window where sync may be active without knowing, but will try to terminate this ASAP.  */
            if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_none)
            {
                leBroadcastManager_SetSyncState(broadcast_source ? broadcast_source->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_stopping_sync_to_pa);
            }
        }
    }
}

void LeBroadcastManager_SourceFailedSyncToPA(void)
{
    broadcast_source_state_t * broadcast_source = NULL;
    
    if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_scanning_for_pa) ||
        (leBroadcastManager_GetSyncState() == broadcast_manager_sync_cancelling_scan_for_pa) ||
        (leBroadcastManager_GetSyncState() == broadcast_manager_sync_syncing_to_pa) ||
        (leBroadcastManager_GetSyncState() == broadcast_manager_sync_cancelling_sync_to_pa))
    {
        broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
        leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
        
        if (broadcast_source)
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceFailedSyncToPA source_id=0x%x", broadcast_source->source_id);
            
            leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_none);
            leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_no_sync_to_pa);
            LeBroadcastManager_ClearCurrentSourceAddr(broadcast_source);
            
            if (leBroadcastManager_PaSyncHasBeenLost(broadcast_source))
            {
                if (broadcast_source->pa_sync_lost.retries)
                {
                    broadcast_source->pa_sync_lost.retries--;
                    MessageSendLater(&broadcast_source->task,
                                        BROADCAST_MANAGER_INTERNAL_MSG_RESYNC_TO_LOST_PA,
                                        NULL,
                                        BROADCAST_MANAGER_TIME_BETWEEN_RESYNC_TO_PA_MS);
                }
                else
                {
                    leBroadcastManager_ResetPaSyncLostState(broadcast_source);
                }
            }
        }
    }
}

void LeBroadcastManager_SourceCancelledSyncToPA(bool success)
{
    broadcast_source_state_t * broadcast_source = NULL;
    
    if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_cancelling_sync_to_pa)
    {
        broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
        leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
        if (success && broadcast_source)
        {
            leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_no_sync_to_pa);
        }
    }
}

void LeBroadcastManager_SourceBroadcastCode(scan_delegator_client_broadcast_code_t * code)
{
    broadcast_source_state_t * broadcast_source = LeBroadcastManager_GetSourceById(code->source_id);

    if (broadcast_source != NULL)
    {
        leBroadcastManager_SourceBroadcastCodeEvent(broadcast_source);
    }
}

uint16 LeBroadcastManager_SourceGetBisStreamHandle(uint8 bis_loc)
{
    uint16 bis_handle = 0;
    broadcast_source_state_t * broadcast_source = NULL;
    
    /* todo Currently returns first handle found */
    for (int i = 0; i < BROADCAST_MANAGER_MAX_BROADCAST_SOURCES; i++)
    {
        broadcast_source = &le_broadcast_manager.broadcast_source_receive_state[i];
        bis_handle = leBroadcastManager_GetBisHandle(broadcast_source, bis_loc);
        if (bis_handle)
        {
            break;
        }
    }
    
    return bis_handle;
}

static bool leBroadcastManager_IsAssistantConnected(void)
{
    if (MirrorProfile_IsRolePrimary())
    {
        return LeBapScanDelegator_IsAnyClientConnected();
    }
    else
    {
        /* A secondary has a connected primary which acts as an assistant */
        return TRUE;
    }
}

void LeBroadcastManager_SourcePaReportReceived(uint16 sync_handle, uint16 data_length_adv, const uint8 * data_adv)
{
    broadcast_source_state_t *broadcast_source = leBroadcastManager_GetSourceBySyncHandle(sync_handle);
    bool bis_sync_no_preference = FALSE;
    bool broadcast_source_valid_no_stream = FALSE;
    bool check_pa_report_bis_index = FALSE;
    uint32 requested_bis_sync_state = 0;
    bool assistant_connected = leBroadcastManager_IsAssistantConnected();
    
    if (broadcast_source)
    {
        requested_bis_sync_state = LeBroadcastManager_GetRequestedBisSyncStateValue(broadcast_source);
        bis_sync_no_preference = (assistant_connected && (requested_bis_sync_state == BIS_SYNC_NO_PREFERENCE));
        /* Broadcast source is valid now check streaming state */
        if (!LeBroadcastManager_SourceIsBisSync() && 
            (leBroadcastManager_GetSyncState() != broadcast_manager_sync_syncing_to_bis))
        {
            broadcast_source_valid_no_stream = TRUE;
        }
    }
    
    if (broadcast_source_valid_no_stream)
    {                          
        if (!assistant_connected || bis_sync_no_preference)
        {
            check_pa_report_bis_index = TRUE;
            if (broadcast_source->dont_sync_bis)
            {
                check_pa_report_bis_index = FALSE;
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaReportReceived. No check of BIS index dont_sync_bis set");
            }
            else if (leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive())
            {
                check_pa_report_bis_index = FALSE;
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaReportReceived. No check of BIS index other audio active");
            }
        }
        else
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaReportReceived. No check of BIS index assistant not specifying no preference");
        }
    }
    
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaReportReceived data_length_adv:0x%x sync_handle 0x%x source_id:%u assistant_connected:0x%x bis_sync_no_preference:0x%x broadcast_source_valid_no_stream:0x%x check_pa_report_bis_index:0x%x requested_bis_sync_state:0x%x",
                                 data_length_adv,
                                 sync_handle,
                                 broadcast_source ? broadcast_source->source_id : 0,
                                 assistant_connected,
                                 bis_sync_no_preference,
                                 broadcast_source_valid_no_stream,
                                 check_pa_report_bis_index,
                                 requested_bis_sync_state
                                 );

    if (le_broadcast_manager.pa_metadata_client_ref_count != 0)
    {
        leBroadcastManager_SendMetadataNotification(data_length_adv, data_adv);
    }

    if (broadcast_source_valid_no_stream)
    {
        leBroadcastManager_ParseBaseReport(broadcast_source, 
                                            data_length_adv, 
                                            data_adv,
                                            check_pa_report_bis_index,
                                            (requested_bis_sync_state && !bis_sync_no_preference) ? requested_bis_sync_state : 0
                                            );
    }
}

void LeBroadcastManager_SourceBigInfoReportReceived(uint16 sync_handle, uint16 max_sdu,
                                                    uint8 encryption, uint16 iso_interval)
{
    broadcast_source_state_t * broadcast_source = leBroadcastManager_GetSourceBySyncHandle(sync_handle);
    bool attempt_bis_sync = FALSE;

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived broadcast_source:%p source_id:%u sync_handle:0x%x encryption:0x%x  iso:%d bis_handle 0x%x sync_state 0x%x",
                    broadcast_source, 
                    broadcast_source->source_id,
                    sync_handle,
                    encryption,
                    iso_interval,
                    LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo),
                    leBroadcastManager_GetSyncState());

        broadcast_source->iso_interval = iso_interval;

        if (encryption && 
            !LeBroadcastManager_ScanDelegatorIsBroadcastCodeSet(broadcast_source->source_id) && 
            (leBroadcastManager_GetCurrentBigEncryptionState(broadcast_source) != scan_delegator_server_big_encryption_broadcast_code_needed))
        {
            leBroadcastManager_SetCurrentBigEncryptionState(broadcast_source, scan_delegator_server_big_encryption_broadcast_code_needed);
        }
        
        /* Before we evaluate other parameters, check for a mismatched SDU size between this BIGInfo report
         * and the previous PA report we've processed. If there is a mismatch we should not proceed to sync
         * to a new BIS until we have processed an up to date PA report event. */
        uint16 base_sdu_size = leBroadcastManager_GetBroadcastSourceBaseSduSize(broadcast_source);
        if ((max_sdu != base_sdu_size) && (base_sdu_size != 0))
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived BIGInfo vs PA report mismatch detected! PA SDU size 0x%x vs BIGInfo SDU size 0x%x", base_sdu_size, max_sdu);
            return;
        }
        
        if (!LeBroadcastManager_SourceIsBisSync() && (leBroadcastManager_GetSyncState() == broadcast_manager_sync_none))
        {
            uint8 target_bis_source_id = leBroadcastManager_GetTargetBisSourceId();
            attempt_bis_sync = TRUE;
            if (!leBroadcastManager_IsMandatoryCodecConfigurationSet(broadcast_source))
            {
                attempt_bis_sync = FALSE;
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived. Mandatory codec config not set");
            }
            else if (leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive())
            {
                attempt_bis_sync = FALSE;
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived. Other audio active");
            }
            else if ((target_bis_source_id != SCAN_DELEGATOR_SOURCE_ID_INVALID) &&
                     (target_bis_source_id != broadcast_source->source_id))
            {
                attempt_bis_sync = FALSE;
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived. Wrong source to sync, need source_id:%u", target_bis_source_id);
            }
        }

        if (attempt_bis_sync)
        {
            le_broadcast_manager_bis_index_t bis_index = leBroadcastManagerSource_GetBisIndexToRender(broadcast_source);

            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived Not streaming found params");
            if (bis_index.index != 0)
            {
                BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigInfoReportReceived Found bis_index[index:0x%x subgroup:0x%x] to sync to", 
                        bis_index.index, bis_index.subgroup);
                /*  BIGInfo indicates source is in streaming state. 
                    Use BIS index found in PA BASE structure to sync to BIS.
                */
                leBroadcastManager_SetBigEncryption(broadcast_source, encryption);
                leBroadcastManager_SelectTargetBisIndexAndSync(broadcast_source, &bis_index);
            }
        }
    }
}

void LeBroadcastManager_SourcePaSyncLoss(uint16 sync_handle)
{
    broadcast_source_state_t *broadcast_source = leBroadcastManager_GetSourceBySyncHandle(sync_handle);
    broadcast_source_codec_config_t codec_config = {0};
    le_broadcast_manager_bis_index_t bis_index = {0};
    uint8 bis_loc;
    
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaSyncLoss sync_handle:0x%x", sync_handle);

    if (broadcast_source)
    {
        if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_stopping_sync_to_pa)
        {
            /* A locally initiated PA sync terminate should not then try to sync back. */
            leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_none);
        }
        else if (leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive())
        {
            /* If PA sync was lost unexpectedly while another audio or voice
               source is active and has interrupted the broadcast source we
               must wait until the other source is finished before trying to
               sync to PA again. */
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaSyncLoss PA sync lost while another source active");
            leBroadcastManager_SetPaSyncLost(broadcast_source);
            leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_none);
        }
        else
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaSyncLoss Attempting to resync");
            leBroadcastManager_SetPaSyncLost(broadcast_source);
            leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_past_not_available);
        }

        leBroadcastManager_SetPaSyncHandle(broadcast_source, 0);
        leBroadcastManager_SetBroadcastSourcePresentationDelay(broadcast_source, 0);
        leBroadcastManager_SetBroadcastSourceCodecConfig(broadcast_source, &codec_config);
        for (bis_loc = 0; bis_loc < broadcast_manager_bis_location_max; bis_loc++)
        {
            leBroadcastManager_SetBaseBisIndex(broadcast_source, &bis_index, bis_loc);
        }
        leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_no_sync_to_pa);
    }
}

void LeBroadcastManager_SourceBigSyncLoss(uint8 big_handle, bool bad_code)
{
    broadcast_source_state_t *broadcast_source = leBroadcastManager_GetSourceByBigHandle(big_handle);
    le_broadcast_manager_bis_index_t bis_index = {0};
    uint8 bis_loc;

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceBigSyncLoss source_id 0x%x big_handle:0x%x source 0x%x",
                                 LeBroadcastManager_SourceGetSourceId(broadcast_source), big_handle, broadcast_source);

    if (broadcast_source)
    {
        if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_stopping_sync_to_bis) &&
            !leBroadcastManager_GetSyncToBisNoPreference(broadcast_source))
        {
            /* A locally initiated BIG sync terminate should not then try to sync back on receiving the next BIGInfo report. */
            leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            leBroadcastManager_SetTargetBisSyncStateNoSync(broadcast_source);
        }
        else 
        {
            if ((leBroadcastManager_GetSyncState() == broadcast_manager_sync_synced_to_bis_create_iso) ||
                (leBroadcastManager_GetSyncState() == broadcast_manager_sync_stopping_sync_to_bis))
            {
                leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
            }

            /* The remote end terminated, suffered sync loss, or a local termination wants to resync. 
                In these cases set the target BIS sync state to be no preference so that the the PA BASE 
                information will be parsed and BIS sync will be attempted
                on receiving the next BIGInfo report.
            */
            leBroadcastManager_SetTargetBisSyncStateNoPreference(broadcast_source);
        }

        if (broadcast_source->source_id == leBroadcastManager_GetTargetBisSourceId())
        {
            /* Clear the target BIS source_id if it is this source so that it
               does not block the broadcast manager from syncing to another
               available source. */
            leBroadcastManager_SetTargetBisSourceId(SCAN_DELEGATOR_SOURCE_ID_INVALID);
            LeBroadcastManager_SyncSendSyncToSource(SCAN_DELEGATOR_SOURCE_ID_INVALID);
        }

        leBroadcastManager_SetBigHandle(broadcast_source, 0);
        leBroadcastManager_SetBigEncryption(broadcast_source, 0);

        for (bis_loc = 0; bis_loc < broadcast_manager_bis_location_max; bis_loc++)
        {
            leBroadcastManager_SetBisHandle(broadcast_source, 0, bis_loc);
            leBroadcastManager_SetBaseBisIndex(broadcast_source, &bis_index, bis_loc);
        }

        LeAudioMessages_SendBroadcastAudioDisconnected(audio_source_le_audio_broadcast);
        if (bad_code)
        {
            leBroadcastManager_SetBigEncryptionBadCode(broadcast_source);
        }

        /* Note that this function also triggers leBroadcastManager_SourceModifiedEvent.
           This call must be the final call here as the leBroadcastManager_SourceModifiedEvent
           requires some of the states to be reset above. */
        leBroadcastManager_SetCurrentBisSyncStateNoSync(broadcast_source);
    }
}

uint16 LeBroadcastManager_SourceGetAudioStreamSampleRate(void)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceGetAudioStreamSampleRate 0x%x", leBroadcastManager_GetBroadcastSourceSampleRate(broadcast_source));
        return leBroadcastManager_GetBroadcastSourceSampleRate(broadcast_source);
    }
    
    return 0;
}

uint16 LeBroadcastManager_SourceGetAudioStreamFrameDuration(void)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceGetAudioStreamFrameDuration 0x%x", leBroadcastManager_GetBroadcastSourceFrameDuration(broadcast_source));
        return leBroadcastManager_GetBroadcastSourceFrameDuration(broadcast_source);
    }
    
    return 0;
}

uint16 LeBroadcastManager_SourceGetAudioStreamOctetsPerFrame(void)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceGetAudioStreamOctetsPerFrame 0x%x", leBroadcastManager_GetBroadcastSourceOctetsPerFrame(broadcast_source));
        return leBroadcastManager_GetBroadcastSourceOctetsPerFrame(broadcast_source);
    }
    
    return 0;
}

uint32 LeBroadcastManager_SourceGetAudioStreamPresentationDelay(void)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceGetAudioStreamPresentationDelay 0x%x", leBroadcastManager_GetBroadcastSourcePresentationDelay(broadcast_source));
        return leBroadcastManager_GetBroadcastSourcePresentationDelay(broadcast_source);
    }

    return 0;
}

uint8 LeBroadcastManager_SourceGetAudioStreamCodecFrameBlocksPerSdu(void)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();
    uint8 codec_frame_blocks_per_sdu = 0;

    if (broadcast_source != NULL)
    {
        codec_frame_blocks_per_sdu = leBroadcastManager_GetBroadcastSourceCodecFrameBlocksPerSdu(broadcast_source);
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceGetAudioStreamCodecFrameBlocksPerSdu 0x%x", codec_frame_blocks_per_sdu);
    }
    
    /* Defaults to 1 if not set */
    return (codec_frame_blocks_per_sdu == 0) ? 1 : codec_frame_blocks_per_sdu;
}

broadcast_source_state_t * LeBroadcastManager_SourceGetFirstActiveSource(void)
{
    broadcast_source_state_t *active_bss = NULL;
    broadcast_source_state_t *bss;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            if (   (leBroadcastManager_GetCurrentPaSyncState(bss) != scan_delegator_server_pa_sync_state_no_sync_to_pa)
                || (   leBroadcastManager_GetSyncStateSourceId() == bss->source_id
                    && leBroadcastManager_GetSyncState() != broadcast_manager_sync_none)
                || (!leBroadcastManager_IsCurrentBisSyncStateIdle(bss)))
            {
                active_bss = bss;
                break;
            }
        }
    }

    return active_bss;
}

bool LeBroadcastManager_IsBroadcastReceiveActive(void)
{
    bool active = FALSE;
    if (le_broadcast_manager.sync_state != broadcast_manager_sync_none)
    {
        active = TRUE;
    }
    else
    {
        if (LeBroadcastManager_SourceGetFirstActiveSource())
        {
            active = TRUE;
        }
    }
    return active;
}

broadcast_source_state_t * LeBroadcastManager_GetSourceOfActiveBis(void)
{
    uint16 active_bis = LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo);

    if (active_bis)
    {
        broadcast_source_state_t *bss = NULL;

        ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
        {
            if (leBroadcastManager_GetBisHandle(bss, broadcast_manager_bis_location_left_or_stereo) == active_bis)
            {
                return bss;
            }
        }
    }
    return NULL;
}

static void leBroadcastManager_CompleteStop(void)
{
    DEBUG_LOG("leBroadcastManager_CompleteStop stop_tasks 0x%x active %u",
              le_broadcast_manager.stop_tasks, LeBroadcastManager_IsBroadcastReceiveActive());

    if (le_broadcast_manager.stop_tasks)
    {
        if (!LeBroadcastManager_IsBroadcastReceiveActive())
        {
            DEBUG_LOG("leBroadcastManager_CompleteStop to start source_id 0x%x",  le_broadcast_manager.source_to_start);

            /* Send CFM that stop has completed. */
            TaskList_MessageSendId(le_broadcast_manager.stop_tasks, LE_BROADCAST_MANAGER_STOP_CFM);
            TaskList_Destroy(le_broadcast_manager.stop_tasks);
            le_broadcast_manager.stop_tasks = NULL;
        }
    }
}

void LeBroadcastManager_Stop(Task requestor)
{
    broadcast_source_state_t *bss;

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_Stop requestor 0x%x", requestor);

    if (requestor)
    {
        if (!le_broadcast_manager.stop_tasks)
        {
            le_broadcast_manager.stop_tasks = TaskList_Create();
            PanicNull(le_broadcast_manager.stop_tasks);
        }
        TaskList_AddTask(le_broadcast_manager.stop_tasks, requestor);
    }

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_Stop stopping source_id 0x%x", bss->source_id);

            leBroadcastManager_SetTargetPaSyncState(bss, le_bm_pa_sync_none);
            leBroadcastManager_SetTargetBisSyncStateNoSync(bss);
            leBroadcastManager_ResetPaSyncLostState(bss);
            leBroadcastManager_SourceModifiedEvent(bss);
        }
    }
    leBroadcastManager_CompleteStop();
}

static void leBroadcastManager_AllSourcesModified(void)
{
    broadcast_source_state_t *bss;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            leBroadcastManager_SourceModifiedEvent(bss);
        }
    }
}

void LeBroadcastManager_Unsync(void)
{
    LeBroadcastManager_Stop(LeBroadcastManager_SourceGetTask());
    LeBroadcastManager_SyncSendCommandInd(le_broadcast_sync_command_unsync);

    /* Clear the paused state of the broadcast manager. */
    leBroadcastManager_ResetPausedState();

    if (le_broadcast_manager.pause_tasks)
    {
        TaskList_Destroy(le_broadcast_manager.pause_tasks);
        le_broadcast_manager.pause_tasks = NULL;
    }
}

void LeBroadcastManager_Pause(Task requestor)
{
    PanicNull(requestor);

    if (LeBroadcastManager_SourceIsBisSync())
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_Pause");

        if (!le_broadcast_manager.pause_tasks)
        {
            broadcast_source_state_t *bss = LeBroadcastManager_GetSourceOfActiveBis();
            if (bss)
            {
                leBroadcastManager_SetPausedState(bss->source_id, leBroadcastManager_GetTargetBisSyncState(bss));
                leBroadcastManager_SetTargetBisSyncStateNoSync(bss);
                LeBroadcastManager_SyncSendPause(bss->source_id);
            }
            else
            {
                leBroadcastManager_ResetPausedState();
            }

            le_broadcast_manager.pause_tasks = TaskList_Create();
            PanicNull(le_broadcast_manager.pause_tasks);
        }
        TaskList_AddTask(le_broadcast_manager.pause_tasks, requestor);

        leBroadcastManager_AllSourcesModified();
    }
}

void LeBroadcastManager_Resume(Task requestor)
{
    PanicNull(requestor);

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_Resume");

    if (le_broadcast_manager.pause_tasks)
    {
        TaskList_RemoveTask(le_broadcast_manager.pause_tasks, requestor);
        if (!TaskList_Size(le_broadcast_manager.pause_tasks))
        {
            TaskList_Destroy(le_broadcast_manager.pause_tasks);
            le_broadcast_manager.pause_tasks = NULL;

            if (le_broadcast_manager.paused_bis_source_id)
            {
                broadcast_source_state_t *bss = LeBroadcastManager_GetSourceById(le_broadcast_manager.paused_bis_source_id);
                if (bss)
                {
                    leBroadcastManager_SetTargetBisSyncState(bss, 
                                                             le_broadcast_manager.paused_bis_sync_state.num_subgroups, 
                                                             le_broadcast_manager.paused_bis_sync_state.bis_sync);
                    LeBroadcastManager_SyncSendResume(bss->source_id);
                }
                leBroadcastManager_ResetPausedState();
            }

            leBroadcastManager_AllSourcesModified();
        }
    }
}

bool LeBroadcastManager_IsPaused(void)
{
    return le_broadcast_manager.pause_tasks != NULL;
}

static void leBroadcastManager_CompleteRemoveAllSources(void)
{
    if (le_broadcast_manager.remove_tasks)
    {
        broadcast_source_state_t *bss = leBroadcastManager_FindAnyRemoveSourcePending();
        if (!bss)
        {
            TaskList_MessageSendId(le_broadcast_manager.remove_tasks, LE_BROADCAST_MANAGER_REMOVE_ALL_SOURCES_CFM);
            TaskList_Destroy(le_broadcast_manager.remove_tasks);
            le_broadcast_manager.remove_tasks = NULL;
        }

        /* Do other actions to tidy up the le_broadcast_manager state */
        leBroadcastManager_SetTargetBisSourceId(SCAN_DELEGATOR_SOURCE_ID_INVALID);
    }
}

void LeBroadcastManager_RemoveAllSources(Task requestor)
{
    broadcast_source_state_t *bss;

    if (requestor)
    {
        if (!le_broadcast_manager.remove_tasks)
        {
            le_broadcast_manager.remove_tasks = TaskList_Create();
            PanicNull(le_broadcast_manager.remove_tasks);
        }
        TaskList_AddTask(le_broadcast_manager.remove_tasks, requestor);
    }

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            leBroadcastManager_AttemptToSourceRemove(bss);
        }
    }

    leBroadcastManager_CompleteRemoveAllSources();
}

bool LeBroadcastManager_IsAnySourceSyncedToPa(void)
{
    bool is_pa_synced = FALSE;
    broadcast_source_state_t *bss;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            scan_delegator_server_pa_sync_state_t pa_sync_state = leBroadcastManager_GetCurrentPaSyncState(bss);
            if (scan_delegator_server_pa_sync_state_sync_to_pa == pa_sync_state)
            {
                is_pa_synced = TRUE;
                break;
            }
        }
    }

    return is_pa_synced;
}

bool LeBroadcastManager_IsAnySourceSyncedToBis(void)
{
    return LeBroadcastManager_SourceIsBisSync();
}

/*! \brief Store when the broadcast source has lost PA sync unexpectedly.

    This should not be set if the PA sync was terminated normally.

    \param broadcast_source Pointer to a broadcast_source_state_t to set PA sync lost.
*/
static void leBroadcastManager_SetPaSyncLost(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetPaSyncLost source_id %u", broadcast_source->source_id);
    broadcast_source->pa_sync_lost.sync_lost = TRUE;
    broadcast_source->pa_sync_lost.retries = BROADCAST_MANAGER_PA_SYNC_LOST_RETRIES;
}
            

static void leBroadcastManager_ResetPaSyncLostState(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_ResetPaSyncLostState source_id %u", broadcast_source->source_id);
    
    MessageCancelFirst(&broadcast_source->task, BROADCAST_MANAGER_INTERNAL_MSG_RESYNC_TO_LOST_PA);

    broadcast_source->pa_sync_lost.sync_lost = FALSE;
}

static void leBroadcastManager_ResyncToLostPa(void)
{
    broadcast_source_state_t *bss;
    
    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_PaSyncHasBeenLost(bss))
        {
            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_ResyncToLostPa source_id %u retries %u", bss->source_id, bss->pa_sync_lost.retries);
            leBroadcastManager_SetTargetPaSyncState(bss, le_bm_pa_sync_past_not_available);
            leBroadcastManager_SourceModifiedEvent(bss);
        }
    }
}

device_t LeBroadcastManager_GetDeviceForAudioSource(audio_source_t source)
{
    device_t device = NULL;

    if (audio_source_le_audio_broadcast == source)
    {
        /* Get the broadcast_source that is active. "Active" means
           * Is a valid source (exists in the BASS server)
           * Is syncing / synced to PA, or
           * Is syncing / synced to BIS. */
        broadcast_source_state_t *bss = LeBroadcastManager_SourceGetFirstActiveSource();
        if (bss)
        {
            /* Try to get the device that matches the Broadcast Assistant
               address for this source. */
            typed_bdaddr taddr = {0};
            if (BtDevice_GetPublicAddress(&LeBroadcastManager_GetAssistantAddress(bss), &taddr))
            {
                device = BtDevice_GetDeviceForBdAddr(&taddr.addr);
            }
        }

        if (!device)
        {
            /* Fallback to using the MRU device if no direct match was found. */
            device = BtDevice_GetMruDevice();
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_GetDeviceForAudioSource attempt to use MRU");
        }
    }

    return device;
}

void LeBroadcastManager_SourceHandleAudioOrVoiceConnectedInd(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceHandleAudioOrVoiceConnectedInd");

    /* If broadcast is currently routed, it will be paused when the audio
       router sets the state of the broadcast music source to
       source_state_disconnecting. See leBroadcastMusicSource_SetState. */
}

void LeBroadcastManager_SourceHandleAudioOrVoiceDisconnectedInd(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceHandleAudioOrVoiceDisconnectedInd");

    leBroadcastManager_ResyncToLostPa();
    
    if (!leBroadcastManager_IsIncomingCallActive() && LeBroadcastManager_IsPaused())
    {
        LeBroadcastManager_Resume(LeBroadcastManager_SourceGetTask());
    }

    /* Check if a broadcast source was added while non-broadcast audio or voice
       was active and now needs to be started. */
    leBroadcastManager_AllSourcesModified();
}

void LeBroadcastManager_SourceHandleIncomingCallStarted(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceHandleIncomingCallStarted ");
    
    leBroadcastManager_SetIncomingCallActive(TRUE);
}

void LeBroadcastManager_SourceHandleIncomingCallEnded(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourceHandleIncomingCallEnded audio_voice:%d paused:%d call:%d", leBroadcastManager_IsNonBroadcastAudioVoiceActive(), LeBroadcastManager_IsPaused(), appHfpIsCallActive());
    
    leBroadcastManager_SetIncomingCallActive(FALSE);
    
    /* If incoming call has ended without any audio interruption, then make sure Broadcast audio is resumed.
        This would happen if out of band ring tones were used during incoming call. 
        The Broadcast audio would have been paused when the audio routing changed due to incoming call. */
    if (!leBroadcastManager_IsNonBroadcastAudioVoiceActive() && !appHfpIsCallActive() && LeBroadcastManager_IsPaused())
    {
        LeBroadcastManager_Resume(LeBroadcastManager_SourceGetTask());
    }
}

void LeBroadcastManager_HandleSetupIsoDataPathCfm(const BapServerSetupDataPathCfm *cfm)
{
    broadcast_source_state_t * broadcast_source = NULL;

    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleSetupIsoDataPathCfm status=%d handle=0x%x", cfm->status, cfm->isoHandle);
    
    if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_synced_to_bis_create_iso)
    {
        broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
        leBroadcastManager_SetSyncState(SCAN_DELEGATOR_SOURCE_ID_INVALID, broadcast_manager_sync_none);
        if (broadcast_source)
        {
            if (cfm->status == hci_success)
            {
                LeBroadcastManager_SyncCheckIfStartMuted(broadcast_source);
#ifdef ENABLE_LE_AUDIO_WBM
                /* Enable WBM based on presence of valid VS LC3 license */
                if (LeBapPacsUtilities_Lc3EpcLicenseCheck())
                {
                    QcomConManagerSetWBMFeature(cfm->isoHandle, TRUE);
                }
#endif
                LeAudioMessages_SendBroadcastAudioConnected(audio_source_le_audio_broadcast);
            }
            leBroadcastManager_SourceModifiedEvent(broadcast_source);
        }
    }
}

void LeBroadcastManager_HandleIsocBigCreateSyncCfm(const BapServerIsocBigCreateSyncCfm *cfm)
{
    if (cfm->status == hci_success)
    {
        PanicNull(cfm->bisHandles);
    }

    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleIsocBigCreateSyncCfm status 0x%x, num_bis:0x%x", cfm->status, cfm->numBis);

    leBroadcastManager_CreateSyncToBisCfm(cfm->status, cfm->numBis, cfm->bisHandles);
    free(cfm->bisHandles);
}

void LeBroadcastManager_SourcePaSyncTerminateCfm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM_T *cfm)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_SourcePaSyncTerminateCfm. status=0x%x", cfm->status);
    LeBroadcastManager_SourcePaSyncLoss(cfm->sync_handle);
}

void LeBroadcastManager_HandleInternalMsgPastTimeout(void)
{
    broadcast_source_state_t * broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleInternalMsgPastTimeout after %u",
                            TimestampEvent_DeltaFrom(TIMESTAMP_EVENT_LE_BROADCAST_START_PAST_TIMER));
        if (leBroadcastManager_GetSyncState() == broadcast_manager_sync_waiting_for_sync_to_pa)
        {
            /* Reset the PA sync state. This is needed to make sure PAST is
               disabled before self-scan is started. */
            leBroadcastManager_SetCurrentPaSyncState(broadcast_source, scan_delegator_server_pa_sync_state_no_past);

            /* Self scan for PA as PAST failed */
            leBroadcastManager_StartScanForPaSource(broadcast_source->source_id);

            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleInternalMsgPastTimeout scanning for PA");
        }
    }
    else
    {
        BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleInternalMsgPastTimeout. Unexpected msg");
    }
}

void LeBroadcastManager_HandleInternalMsgSourcesSyncCheck(void)
{
    broadcast_source_state_t *bss;

    ARRAY_FOREACH(bss, le_broadcast_manager.broadcast_source_receive_state)
    {
        if (leBroadcastManager_IsSourceValid(bss))
        {
            BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_HandleInternalMsgSourcesSyncCheck source_id:%d",
                                          bss->source_id);

            if (leBroadcastManager_UpdateSync(bss))
            {
                break;
            }
        }
    }
}

void LeBroadcastManager_HandleInternalMsgResyncToLostPa(void)
{
    if (!leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive())
    {
        leBroadcastManager_ResyncToLostPa();
    }
}

void LeBroadcastManager_HandleReceivedPauseSource(uint8 source_id)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_HandleReceivedPauseSource. Pausing for source_id:%d", source_id);
        LeBroadcastManager_Pause(LeBroadcastManager_SourceGetTask());
    }
}

void LeBroadcastManager_HandleReceivedResumeSource(uint8 source_id)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);

    if (broadcast_source != NULL)
    {
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_HandleReceivedResumeSource. Resuming for source_id:%d", source_id);
        LeBroadcastManager_Resume(LeBroadcastManager_SourceGetTask());
    }
}

void LeBroadcastManager_HandleReceivedSyncToSource(uint8 source_id)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_HandleReceivedSyncToSource source_id:%d", source_id);
    
    leBroadcastManager_SetTargetBisSourceId(source_id);
    
    leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource();
}

void LeBroadcastManager_HandleStopConfirm(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_HandleStopConfirm");

    /* If we've flagged that there is a source waiting to be synced once this one is 
       stopped then start it up */
    if(le_broadcast_manager.source_to_start)
    {
        broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(le_broadcast_manager.source_to_start);

        if(broadcast_source)
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_HandleStopConfirm starting source %d", le_broadcast_manager.source_to_start);
            leBroadcastManager_SetTargetPaSyncState(broadcast_source, le_bm_pa_sync_past_not_available);
            leBroadcastManager_SetTargetBisSyncStateNoPreference(broadcast_source);
            leBroadcastManager_SourceModifiedEvent(broadcast_source);
        }
        le_broadcast_manager.source_to_start = 0;
    }
}

void LeBroadcastManager_InitiateSyncCheckAllSources(void)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_InitiateSyncCheckAllSources");
    MessageSend(LeBroadcastManager_SourceGetTask(), BROADCAST_MANAGER_INTERNAL_MSG_SOURCES_SYNC_CHECK, 0);
}

bool LeBroadcastManager_DoesSidAndBroadcastIdMatchSource(broadcast_source_state_t *broadcast_source, uint8 adv_sid, uint32 broadcast_id)
{
    bool match = FALSE;

    if (broadcast_source != NULL)
    {
        uint32 source_broadcast_id = LeBroadcastManager_ScanDelegatorGetSourceBroadcastId(broadcast_source->source_id);
        uint8 source_adv_sid = LeBroadcastManager_ScanDelegatorGetSourceAdvSid(broadcast_source->source_id);

        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_DoesSidAndBroadcastIdMatchSource EA:[BrID:0x%x AdvSID:0x%x] SRC:[BrID:0x%x AdvSID:0x%x]",
                                    broadcast_id,
                                    adv_sid,
                                    source_broadcast_id,
                                    source_adv_sid);

        if ((broadcast_id == source_broadcast_id) &&
             (adv_sid == source_adv_sid))
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_DoesSidAndBroadcastIdMatchSource. Matched for source_id:%u adv_sid:0x%x broadcast_id:0x%x",
                                            broadcast_source->source_id,
                                            adv_sid,
                                            broadcast_id);
            match = TRUE;
        }
    }

    return match;
}

bool LeBroadcastManager_DoesSidAndBdaddrMatchSource(broadcast_source_state_t *broadcast_source, uint8 adv_sid, const typed_bdaddr *taddr)
{
    bool match = FALSE;

    BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_DoesSidAndBdaddrMatchSource");

    if (broadcast_source && taddr)
    {
        typed_bdaddr *source_taddr = &broadcast_source->source_match_address;
        uint8 source_adv_sid = LeBroadcastManager_ScanDelegatorGetSourceAdvSid(broadcast_source->source_id);

        BROADCAST_MANAGER_SOURCE_LOG("  EA:  Addr %u %04x:%02x:%06x sid 0x%02x",
                                     taddr->type,
                                     taddr->addr.nap, taddr->addr.uap, taddr->addr.lap,
                                     adv_sid);

        BROADCAST_MANAGER_SOURCE_LOG("  SRC: Addr %u %04x:%02x:%06x sid 0x%02x",
                                     source_taddr->type,
                                     source_taddr->addr.nap, source_taddr->addr.uap, source_taddr->addr.lap,
                                     source_adv_sid);

        if (source_adv_sid == adv_sid && BdaddrTypedIsSame(source_taddr, taddr))
        {
            BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_DoesSidAndBdaddrMatchSource: matched");
            match = TRUE;
        }
    }

    return match;
}

void LeBroadcastManager_StopScanForPaSource(uint8 source_id, bool sync_to_pa)
{
    BROADCAST_MANAGER_SOURCE_LOG("leBroadcastManager_StopScanForPaSource source_id=0x%x sync_to_pa:%u", 
                                    source_id,
                                    sync_to_pa);

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);
    leBroadcastManager_PeriodicScanStopRequest(LeBroadcastManager_SourceGetPeriodicScanHandle(broadcast_source));
#else
    LeBapBroadcastSink_StopScanPaSourceRequest();
#endif
    
    if (sync_to_pa)
    {
        leBroadcastManager_SetSyncState(source_id, broadcast_manager_sync_cancelling_scan_sync_to_pa);
    }
    else
    {
        leBroadcastManager_SetSyncState(source_id, broadcast_manager_sync_cancelling_scan_for_pa);
    }
}

void LeBroadcastManager_StartSyncToPaSource(uint8 source_id)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);
    
    if (broadcast_source != NULL)
    {
        typed_bdaddr source_taddr = LeBroadcastManager_GetCurrentSourceAddr(broadcast_source);

        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_StartSyncToPaSource source_id=0x%x addr:[%u %04x:%02x:%06x]", 
                                        broadcast_source->source_id,
                                        source_taddr.type,
                                        source_taddr.addr.nap,
                                        source_taddr.addr.uap,
                                        source_taddr.addr.lap);
                                        
        if (leBroadcastManager_GetSyncState() != broadcast_manager_sync_cancelling_scan_sync_to_pa)
        {
            TimestampEvent(TIMESTAMP_EVENT_LE_BROADCAST_START_PA_SYNC);
        }

        LeBapBroadcastSink_SyncPaSource(&source_taddr, leBroadcastManager_GetSourceAdvSid(broadcast_source));

        leBroadcastManager_SetSyncState(broadcast_source->source_id, broadcast_manager_sync_syncing_to_pa);
    }
}

void LeBroadcastManager_StoreCurrentSourceAddr(const typed_bdaddr *current_addr)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(leBroadcastManager_GetSyncStateSourceId());
    if (broadcast_source != NULL)
    {
        broadcast_source->current_taddr = *current_addr;
        BROADCAST_MANAGER_SOURCE_LOG("LeBroadcastManager_StoreCurrentSourceAddr addr:[%u %04x:%02x:%06x]", 
                                        broadcast_source->current_taddr.type,
                                        broadcast_source->current_taddr.addr.nap,
                                        broadcast_source->current_taddr.addr.uap,
                                        broadcast_source->current_taddr.addr.lap);
    }
}

typed_bdaddr LeBroadcastManager_GetCurrentSourceAddr(broadcast_source_state_t *broadcast_source)
{
    if (!BdaddrTypedIsEmpty(&broadcast_source->current_taddr))
    {
        return broadcast_source->current_taddr;
    }

    return leBroadcastManager_GetSourceTypedBdaddr(broadcast_source);
}

void LeBroadcastManager_ClearCurrentSourceAddr(broadcast_source_state_t *broadcast_source)
{
    BdaddrTypedSetEmpty(&broadcast_source->current_taddr);
}

static uint16 leBroadcastManager_GetBroadcastSourceBaseSduSize(broadcast_source_state_t *broadcast_source)
{
    uint16 num_channels_per_bis = broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.is_stereo_bis ? 2 : 1;

    return (leBroadcastManager_GetBroadcastSourceOctetsPerFrame(broadcast_source) * leBroadcastManager_GetBroadcastSourceCodecFrameBlocksPerSdu(broadcast_source) * num_channels_per_bis);
}

void LeBroadcastManager_SwitchToBroadcastSource(broadcast_source_state_t *broadcast_source)
{
    if (broadcast_source)
    {
        DEBUG_LOG("LeBroadcastManager_SwitchToBroadcastSource source_id 0x%x", broadcast_source->source_id);

        leBroadcastManager_SetTargetBisSourceId(broadcast_source->source_id);
        LeBroadcastManager_SyncSendSyncToSource(broadcast_source->source_id);

        leBroadcastManager_ResyncBisIfNotSyncedToCorrectSource();
    }
}

void LeBroadcastManager_EnableMetadataNotification(bool enable)
{
    if (enable)
    {
        le_broadcast_manager.pa_metadata_client_ref_count++;
    }
    else if (le_broadcast_manager.pa_metadata_client_ref_count)
    {
        le_broadcast_manager.pa_metadata_client_ref_count--;
    }
}

le_bm_bass_status_t LeBroadcastManager_SourceSetMatchAddress(uint8 source_id, const typed_bdaddr *taddr)
{
    le_bm_bass_status_t status = le_bm_bass_status_failed;

    if (taddr)
    {
        broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);

        if (broadcast_source)
        {
            broadcast_source->source_match_address = *taddr;
            status = le_bm_bass_status_success;

            DEBUG_LOG_DEBUG("LeBroadcastManager_SourceSetMatchAddress: source_id=%u taddr=%u %04x %02x %06x",
                            source_id,
                            taddr->type,
                            taddr->addr.nap, taddr->addr.uap, taddr->addr.lap);
        }
        else
        {
            status = le_bm_bass_status_invalid_source_id;
            DEBUG_LOG_WARN("LeBroadcastManager_SourceSetMatchAddress: no source with id %u", source_id);
        }
    }

    return status;
}


#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */
