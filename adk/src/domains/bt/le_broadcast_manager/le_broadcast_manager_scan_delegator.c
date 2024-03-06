/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      LE Broadcast Manager interface with the BAP Scan Delegator role.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#include <connection_manager.h>

#include "le_broadcast_manager_scan_delegator.h"

#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_bass.h"

#include "bt_device.h"
#include "device_properties.h"
#include "gatt_connect.h"

#include <logging.h>

#include <panic.h>


#define BROADCAST_MANAGER_SCAN_DELEGATOR_LOG   DEBUG_LOG


static void leBroadcastManager_RemoteScanningStart(void);
static void leBroadcastManager_RemoteScanningStop(void);
static void leBroadcastManager_AddSource(scan_delegator_client_add_broadcast_source_t * source);
static void leBroadcastManager_ModifySource(scan_delegator_client_modify_broadcast_source_t * source);
static void leBroadcastManager_BroadcastCode(scan_delegator_client_broadcast_code_t * code);
static void leBroadcastManager_RemoveSource(scan_delegator_client_remove_broadcast_source_t * source);
static void leBroadcastManager_PeriodicSync(scan_delegator_periodic_sync_t * sync);
static void * leBroadcastManager_RetrieveClientConfig(gatt_cid_t cid);
static void leBroadcastManager_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size);
static scan_delegator_status_t leBroadcastManager_ScanDelegatorModifySourceState(uint8 source_id, const scan_delegator_server_get_broadcast_source_state_t * new_state);
static bool leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(uint8 source_id, const scan_delegator_server_get_broadcast_source_state_t * new_state);

static const LeBapScanDelegator_callback_interface_t bap_scan_delegator_interface =
{
    .LeBapScanDelegator_RemoteScanningStart = leBroadcastManager_RemoteScanningStart,
    .LeBapScanDelegator_RemoteScanningStop = leBroadcastManager_RemoteScanningStop,
    .LeBapScanDelegator_AddSource = leBroadcastManager_AddSource,
    .LeBapScanDelegator_ModifySource = leBroadcastManager_ModifySource,
    .LeBapScanDelegator_BroadcastCode = leBroadcastManager_BroadcastCode,
    .LeBapScanDelegator_RemoveSource = leBroadcastManager_RemoveSource,
    .LeBapScanDelegator_PeriodicSync = leBroadcastManager_PeriodicSync,
    .LeBapScanDelegator_RetrieveClientConfig = leBroadcastManager_RetrieveClientConfig,
    .LeBapScanDelegator_StoreClientConfig = leBroadcastManager_StoreClientConfig,
    .LeBapScanDelegator_GetTargetSyncState = LeBroadcastManager_GetTargetSyncState
};

static inline le_bm_bass_status_t leBroadcastManager_LeBapToLeBmStatus(scan_delegator_status_t bap_status)
{
    return (le_bm_bass_status_t) bap_status;
}

static void leBroadcastManager_RemoteScanningStart(void)
{
    LeBroadcastManager_SourceSetAssistantScanningState(broadcast_manager_assistant_scan_active);
}

static void leBroadcastManager_RemoteScanningStop(void)
{
    LeBroadcastManager_SourceSetAssistantScanningState(broadcast_manager_assistant_scan_inactive);
}

static void leBroadcastManager_AddSource(scan_delegator_client_add_broadcast_source_t * source)
{
    LeBroadcastManager_SourceAdd(source);
}

static void leBroadcastManager_ModifySource(scan_delegator_client_modify_broadcast_source_t * source)
{
    LeBroadcastManager_SourceModify(source);
}

static void leBroadcastManager_BroadcastCode(scan_delegator_client_broadcast_code_t * code)
{
    LeBroadcastManager_SourceBroadcastCode(code);
}

static void leBroadcastManager_RemoveSource(scan_delegator_client_remove_broadcast_source_t * source)
{
    LeBroadcastManager_SourceRemove(source);
}

static void leBroadcastManager_PeriodicSync(scan_delegator_periodic_sync_t * sync)
{
    LeBroadcastManager_SourceSyncedToPA(sync);
}

static void * leBroadcastManager_RetrieveClientConfig(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void * device_config = NULL;
    if(device)
    {
        size_t size;
        if (!Device_GetProperty(device, device_property_le_audio_broadcast_config, &device_config, &size))
        {
            device_config = NULL;
        }
        BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("leBroadcastManager_RetrieveClientConfig device=0x%p device_config=0x%p size=%d", device, device_config, size);
    }

    return device_config;
}

static void leBroadcastManager_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size)
{
    device_t device = GattConnect_GetBtDevice(cid);
    if(device)
    {
        Device_SetProperty(device, device_property_le_audio_broadcast_config, config, size);
        BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("leBroadcastManager_StoreClientConfig device=0x%p size=%d", device, size);
    }
}

static bool leBroadcastManager_IsClientConnected(typed_bdaddr *client_address)
{
    bool connected = FALSE;

    if (!BdaddrTypedIsEmpty(client_address))
    {
        tp_bdaddr tpaddr;

        tpaddr.transport = TRANSPORT_BLE_ACL;
        tpaddr.taddr = *client_address;

        connected = ConManagerIsTpConnected(&tpaddr);
    }

    BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("leBroadcastManager_IsClientConnected: addr %u %04x %02x %06lx connected %u",
                                         client_address->type,
                                         client_address->addr.nap, client_address->addr.uap, client_address->addr.lap,
                                         connected);

    return connected;
}

scan_delegator_target_sync_state_t LeBroadcastManager_GetTargetSyncState(uint8 source_id)
{
    scan_delegator_target_sync_state_t target_sync_state = {0};

    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceById(source_id);
    
    if (broadcast_source != NULL)
    {
        target_sync_state.pa_sync = leBroadcastManager_GetTargetPaSyncState(broadcast_source);

        const le_broadcast_manager_bis_sync_state_t *requested_bis_sync_state = LeBroadcastManager_GetRequestedBisSyncState(broadcast_source);
        target_sync_state.num_subgroups = requested_bis_sync_state->num_subgroups;
        target_sync_state.bis_sync = requested_bis_sync_state->bis_sync;
        if (!BtDevice_GetPublicAddress(&LeBroadcastManager_GetAssistantAddress(broadcast_source), &target_sync_state.assistant_address))
        {
            BdaddrTypedSetEmpty(&target_sync_state.assistant_address);
        }
    }
    
    return target_sync_state;
}

le_bm_bass_status_t leBroadcastManager_GetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    return leBroadcastManager_LeBapToLeBmStatus(LeBapScanDelegator_GetBroadcastSourceState(source_id, source_state));
}

void leBroadcastManager_FreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    LeBapScanDelegator_FreeBroadcastSourceState(source_state);
}

static scan_delegator_status_t leBroadcastManager_ScanDelegatorModifySourceState(uint8 source_id, const scan_delegator_server_get_broadcast_source_state_t * new_state)
{
    scan_delegator_server_modify_broadcast_source_state_t source_state = {0};
    
    source_state.pa_sync_state = new_state->pa_sync_state;
    source_state.big_encryption = new_state->big_encryption;
    source_state.bad_code = new_state->bad_code;
    source_state.num_subgroups = new_state->num_subgroups;
    source_state.subgroups = new_state->subgroups;
    
    return LeBapScanDelegator_ModifyBroadcastSourceState(source_id, &source_state);
}

static bool leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(uint8 source_id, const scan_delegator_server_get_broadcast_source_state_t * new_state)
{
    scan_delegator_status_t result = leBroadcastManager_ScanDelegatorModifySourceState(source_id, new_state);

    if (scan_delegator_status_success == result)
    {
        LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(source_id);
    }
    else if (scan_delegator_status_brs_not_changed)
    {
        /* This status means the operation was successful but the values in
           the Broadcast Receive State characteristic were not changed and
           a GATT notification was not sent from the BASS server. */
        result = scan_delegator_status_success;
    }

    return (result == scan_delegator_status_success);
}

static void leBroadcastManager_ConfigurePastForSource(uint8 source_id, bool enable)
{
    broadcast_source_state_t *bss = LeBroadcastManager_GetSourceById(source_id);
    if (bss)
    {
        typed_bdaddr taddr = LeBroadcastManager_GetAssistantAddress(bss);
        if (!BdaddrTypedIsEmpty(&taddr))
        {
            LeBapScanDelegator_ConfigurePastForAddr(&taddr, enable);
        }
    }
}


void LeBroadcastManager_ScanDelegatorInit(uint8 number_broadcast_sources)
{
    LeBapScanDelegator_Init(number_broadcast_sources, &bap_scan_delegator_interface);
}

le_bm_bass_status_t LeBroadcastManager_ScanDelegatorRemoveSourceState(uint8 source_id)
{
    BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("LeBroadcastManager_ScanDelegatorRemoveSourceState id:%d", source_id);
    return leBroadcastManager_LeBapToLeBmStatus(LeBapScanDelegator_RemoveBroadcastSourceState(source_id));
}

bool LeBroadcastManager_ScanDelegatorIsBroadcastCodeSet(uint8 source_id)
{
    uint8 *code = LeBapScanDelegator_GetBroadcastCode(source_id);
    bool is_set = FALSE;
    if (code)
    {
        uint8 unset_code[SCAN_DELEGATOR_BROADCAST_CODE_SIZE];
        size_t size = SCAN_DELEGATOR_BROADCAST_CODE_SIZE;
        memset(unset_code, 0, size);
        is_set = (memcmp(unset_code, code, size) != 0);
        free(code);
    }
    return is_set;
}

bool LeBroadcastManager_ScanDelegatorIsPastAvailable(broadcast_source_state_t *broadcast_source)
{
    /* The pa_sync field allows the client to tell the server whether it supports PAST. */
    return broadcast_source &&
           (broadcast_source->target_pa_sync_state == le_bm_pa_sync_past_available) &&
           leBroadcastManager_IsClientConnected(&broadcast_source->assistant_address);
}

void LeBroadcastManager_ScanDelegatorFreeSourceAddMemory(scan_delegator_client_add_broadcast_source_t *client_add_source)
{
    for (uint8 i=0; i<client_add_source->num_subgroups; i++)
    {
        if (client_add_source->subgroups[i].metadata_length)
        {
            free(client_add_source->subgroups[i].metadata);
        }
    }
    if (client_add_source->subgroups)
    {
        free(client_add_source->subgroups);
    }
}

void LeBroadcastManager_ScanDelegatorSetPaSyncState(uint8 source_id, scan_delegator_server_pa_sync_state_t pa_sync_state)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);

    if (   (pa_sync_state == scan_delegator_server_pa_sync_state_syncinfo_request)
        && (source_state.pa_sync_state != pa_sync_state))
    {
        /* Enable PAST in the controller when entering scan_delegator_server_pa_sync_state_syncinfo_request state. */
        leBroadcastManager_ConfigurePastForSource(source_id, TRUE);
    }
    else if (   (source_state.pa_sync_state == scan_delegator_server_pa_sync_state_syncinfo_request)
             && (source_state.pa_sync_state != pa_sync_state))
    {
        /* Disable PAST in the controller when exiting scan_delegator_server_pa_sync_state_syncinfo_request state. */
        leBroadcastManager_ConfigurePastForSource(source_id, FALSE);
    }

    source_state.pa_sync_state = pa_sync_state;
    
    BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("LeBroadcastManager_ScanDelegatorSetPaSyncState source_id=0x%x pa_sync_state=0x%x", source_id, pa_sync_state);
    
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
    
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
}

scan_delegator_server_pa_sync_state_t LeBroadcastManager_ScanDelegatorGetPaSyncState(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    scan_delegator_server_pa_sync_state_t pa_sync_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    pa_sync_state = source_state.pa_sync_state;
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return pa_sync_state;
}

static inline le_broadcast_manager_bis_index_t leBroadcastManagerScanDelegator_GetAudioLocationIndexes(uint8 source_id)
{
    broadcast_source_state_t * broadcast_source = LeBroadcastManager_GetSourceById(source_id);

    le_broadcast_manager_bis_index_t base_bis_indexes = { .subgroup = 0, .index = 0 };

    if(broadcast_source)
    {
        if(broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.index)
        {
            /* If right is also present, it should be in the same subgroup */
            base_bis_indexes.subgroup = broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.subgroup;
        }
        else if(broadcast_source->bis_info[broadcast_manager_bis_location_right].base_bis_index.index)
        {
            base_bis_indexes.subgroup = broadcast_source->bis_info[broadcast_manager_bis_location_right].base_bis_index.subgroup;
        }
        base_bis_indexes.index = (broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.index
                                  | broadcast_source->bis_info[broadcast_manager_bis_location_right].base_bis_index.index);
    }
    return base_bis_indexes;
}

void LeBroadcastManager_ScanDelegatorSetBisSyncState(uint8 source_id, const le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    uint8 i;
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    
    if (bis_sync_state->num_subgroups < source_state.num_subgroups)
    {
        /* If new num_subgroups is less, free the metadata memory */
        for (i=bis_sync_state->num_subgroups; i<source_state.num_subgroups; i++)
        {
            if (source_state.subgroups[i].metadata)
            {
                free(source_state.subgroups[i].metadata);
            }
        }
    }
    
    if (bis_sync_state->num_subgroups)
    {
        if (bis_sync_state->num_subgroups != source_state.num_subgroups)
        {
            /* Amend space for different number of subgroups */
            le_bm_source_subgroup_t *buf = source_state.subgroups;
            source_state.subgroups = PanicNull(realloc(buf, sizeof(le_bm_source_subgroup_t) * bis_sync_state->num_subgroups));
            for (i=source_state.num_subgroups; i<bis_sync_state->num_subgroups; i++)
            {
                source_state.subgroups[i].bis_sync = 0;
                source_state.subgroups[i].metadata_length = 0;
                source_state.subgroups[i].metadata = NULL;
            }
        }
    }
    else
    {
        if (source_state.subgroups)
        {
            free(source_state.subgroups);
        }
        source_state.subgroups = NULL;
    }

    BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("LeBroadcastManager_ScanDelegatorSetBisSyncState source_id=0x%x num_subgroups[old:0x%x new:0x%x]", source_id, source_state.num_subgroups, bis_sync_state->num_subgroups);
    
    for (i=0; i<bis_sync_state->num_subgroups; i++)
    {
        if(bis_sync_state->bis_sync[i])
        {
            le_broadcast_manager_bis_index_t base_bis_indexes = leBroadcastManagerScanDelegator_GetAudioLocationIndexes(source_id);
            if(i == base_bis_indexes.subgroup && (bis_sync_state->bis_sync[i] & base_bis_indexes.index))
            {
                source_state.subgroups[i].bis_sync = base_bis_indexes.index;
            }
            else
            {
                source_state.subgroups[i].bis_sync = bis_sync_state->bis_sync[i];
            }
        }
        BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("  bis_sync[0x%x] reporting[0x%x]", bis_sync_state->bis_sync[i], source_state.subgroups[i].bis_sync);
    }
    
    source_state.num_subgroups = bis_sync_state->num_subgroups;
    
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
    
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
}

void LeBroadcastManager_ScanDelegatorSetBisSyncStateFailedSync(uint8 source_id)
{
    uint8 i;
    le_broadcast_manager_bis_sync_state_t bis_sync_state;
    le_broadcast_manager_bis_sync_state_t *pending_bis_sync = leBroadcastManager_GetPendingBisSyncState();
    
    bis_sync_state.num_subgroups = pending_bis_sync->num_subgroups;
    if (bis_sync_state.num_subgroups)
    {
        bis_sync_state.bis_sync = PanicUnlessMalloc(sizeof(uint32) * bis_sync_state.num_subgroups);
        for (i=0; i<bis_sync_state.num_subgroups; i++)
        {
            if (pending_bis_sync->bis_sync[i] != 0)
            {
                bis_sync_state.bis_sync[i] = SCAN_DELEGATOR_SERVER_BIS_SYNC_STATE_FAILED_SYNC_BIG;
            }
            else
            {
                bis_sync_state.bis_sync[i] = 0;
            }
        }
    }
    else
    {
        bis_sync_state.bis_sync = NULL;
    }
    LeBroadcastManager_ScanDelegatorSetBisSyncState(source_id, &bis_sync_state);
    if (bis_sync_state.bis_sync)
    {
        free(bis_sync_state.bis_sync);
    }
}

void LeBroadcastManager_ScanDelegatorSetBisSyncStateNoSync(uint8 source_id)
{
    uint8 i;
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    
    for (i=0; i<source_state.num_subgroups; i++)
    {
        source_state.subgroups[i].bis_sync = 0;
    }
    
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
    
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
}

uint32 LeBroadcastManager_ScanDelegatorGetBisSyncState(uint8 source_id)
{
    uint32 current_bis_sync_state = 0;
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);

    for (uint8 i=0; i<source_state.num_subgroups; i++)
    {
        current_bis_sync_state |= source_state.subgroups[i].bis_sync;
    }
    
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return current_bis_sync_state;
}

void LeBroadcastManager_ScanDelegatorSetBigEncryptionState(uint8 source_id, scan_delegator_server_big_encryption_t big_encryption)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    
    source_state.big_encryption = big_encryption;
    if (source_state.bad_code)
    {
        free(source_state.bad_code);
        source_state.bad_code = NULL;
    }
    
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
    
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
}

scan_delegator_server_big_encryption_t LeBroadcastManager_ScanDelegatorGetBigEncryptionState(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    scan_delegator_server_big_encryption_t big_encryption;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    big_encryption = source_state.big_encryption;
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return big_encryption;
}

void LeBroadcastManager_ScanDelegatorSetBigEncryptionBadCode(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    
    uint8 *bad_code = LeBapScanDelegator_GetBroadcastCode(source_id);
    if (bad_code)
    {
        uint8 *broadcast_code = PanicUnlessMalloc(SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

        /* Clear the stored broadcast code as it was found to be bad. */
        memset(broadcast_code, 0, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
        LeBapScanDelegator_SetBroadcastCode(source_id, broadcast_code);
        free(broadcast_code);
        
        PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
        
        /* Write the bad code state and the bad code to the Broadcast Receive State characteristic. */
        source_state.big_encryption = scan_delegator_server_big_encryption_bad_code;
        source_state.bad_code = bad_code;
        
        for(int i=0; i < source_state.num_subgroups; i++)
        {
            source_state.subgroups[i].bis_sync = 0;
        }

        PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
        
        leBroadcastManager_FreeBroadcastSourceState(&source_state);
    }
}

typed_bdaddr LeBroadcastManager_ScanDelegatorGetSourceTypedBdaddr(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    typed_bdaddr source_address;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    source_address = source_state.source_address;
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return source_address;
}

uint8 LeBroadcastManager_ScanDelegatorGetSourceAdvSid(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    uint8 source_adv_sid;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    source_adv_sid = source_state.source_adv_sid;
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return source_adv_sid;
}

void LeBroadcastManager_ScanDelegatorSetSourceAdvSid(uint8 source_id, uint8 source_adv_sid)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;

    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    source_state.source_adv_sid = source_adv_sid;
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
}

uint32 LeBroadcastManager_ScanDelegatorGetSourceBroadcastId(uint8 source_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;
    uint32 broadcast_id;
    
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    broadcast_id = source_state.broadcast_id;
    leBroadcastManager_FreeBroadcastSourceState(&source_state);
    
    return broadcast_id;
}

void LeBroadcastManager_ScanDelegatorSetSourceBroadcastId(uint8 source_id, uint32 broadcast_id)
{
    scan_delegator_server_get_broadcast_source_state_t source_state;

    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &source_state) == le_bm_bass_status_success);
    source_state.broadcast_id = broadcast_id;
    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &source_state));
}

le_bm_bass_status_t LeBroadcastManager_ScanDelegatorWriteClientAddSource(uint8 *source_id, const scan_delegator_client_add_broadcast_source_t * new_source)
{
    le_bm_bass_status_t status;
    scan_delegator_server_add_broadcast_source_state_t add_source = {0};
    
    add_source.pa_sync_state = scan_delegator_server_pa_sync_state_no_sync_to_pa;
    add_source.big_encryption = scan_delegator_server_big_encryption_not_encrypted;
    add_source.source_address = new_source->advertiser_address;
    add_source.broadcast_id = new_source->broadcast_id;
    add_source.source_adv_sid = new_source->source_adv_sid;
    add_source.bad_code = NULL;
    add_source.num_subgroups = new_source->num_subgroups;
    if (new_source->num_subgroups)
    {
        add_source.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * new_source->num_subgroups);
        for (uint8 i=0; i<new_source->num_subgroups; i++)
        {
            add_source.subgroups[i].bis_sync = 0;
            add_source.subgroups[i].metadata_length = new_source->subgroups[i].metadata_length;
            add_source.subgroups[i].metadata = new_source->subgroups[i].metadata_length ? new_source->subgroups[i].metadata : NULL;
        }
    }
    else
    {
        add_source.subgroups = NULL;
    }
    
    status = leBroadcastManager_LeBapToLeBmStatus(LeBapScanDelegator_AddBroadcastSourceState(source_id, &add_source));

    
    if (add_source.subgroups)
    {
        free(add_source.subgroups);
    }

    return status;
}

void LeBroadcastManager_ScanDelegatorWriteClientModifySource(uint8 source_id, const scan_delegator_client_modify_broadcast_source_t * new_source)
{
    scan_delegator_server_get_broadcast_source_state_t current_brs = {0};
    scan_delegator_server_get_broadcast_source_state_t new_brs = {0};
    uint8 number_of_subgroups = 0;
        
    PanicFalse(leBroadcastManager_GetBroadcastSourceState(source_id, &current_brs) == le_bm_bass_status_success);

    new_brs.pa_sync_state = current_brs.pa_sync_state;
    new_brs.big_encryption = current_brs.big_encryption;
    new_brs.source_adv_sid = current_brs.source_adv_sid;
    new_brs.bad_code = current_brs.bad_code;
    number_of_subgroups = new_source->num_subgroups;
    
    if (new_source->num_subgroups < current_brs.num_subgroups)
    {
        for (uint8 i=new_source->num_subgroups; i<current_brs.num_subgroups; i++)
        {
            if (current_brs.subgroups[i].bis_sync != 0)
            {
                /* If currently BIS sync on a subgroup that is to be removed, need to make sure this sync is terminated first.
                    Keep hold of the current number of subgroups while the sync is still active. 
                */
                number_of_subgroups = current_brs.num_subgroups;
            }
        }
    }
    
    new_brs.num_subgroups = number_of_subgroups;
    
    BROADCAST_MANAGER_SCAN_DELEGATOR_LOG("LeBroadcastManager_SourceModify modify_num_sub:%d current_num_sub:%d new_num_sub:%d", new_source->num_subgroups, current_brs.num_subgroups, new_brs.num_subgroups);
    
    if (new_brs.num_subgroups)
    {            
        new_brs.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * new_brs.num_subgroups);
        
        for (uint8 i=0; i<new_brs.num_subgroups; i++)
        {
            if (i < current_brs.num_subgroups)
            {
                new_brs.subgroups[i].bis_sync = current_brs.subgroups[i].bis_sync;
                if ((i < new_source->num_subgroups) && new_source->subgroups[i].metadata_length)
                {
                    new_brs.subgroups[i].metadata_length = new_source->subgroups[i].metadata_length;
                    new_brs.subgroups[i].metadata = new_source->subgroups[i].metadata;
                }
                else
                {
                    new_brs.subgroups[i].metadata_length = current_brs.subgroups[i].metadata_length;
                    new_brs.subgroups[i].metadata = current_brs.subgroups[i].metadata_length ? current_brs.subgroups[i].metadata : NULL;
                }
            }
            else
            {
                new_brs.subgroups[i].bis_sync = 0;
                if (new_source->subgroups[i].metadata_length)
                {
                    new_brs.subgroups[i].metadata_length = new_source->subgroups[i].metadata_length;
                    new_brs.subgroups[i].metadata = new_source->subgroups[i].metadata;
                }
                else
                {
                    new_brs.subgroups[i].metadata_length = 0;
                    new_brs.subgroups[i].metadata = NULL;
                }
            }
        }
    }
    else
    {
        new_brs.subgroups = NULL;
    }

    PanicFalse(leBroadcastManager_ScanDelegatorModifySourceStateAndNotifyClients(source_id, &new_brs));

    if (new_brs.subgroups)
    {
        free(new_brs.subgroups);
    }
    
    leBroadcastManager_FreeBroadcastSourceState(&current_brs);
}

scan_delegator_client_add_broadcast_source_t * leBroadcastManager_ScanDelegatorCopyClientAddSource(const scan_delegator_client_add_broadcast_source_t * add_broadcast_source)
{
    /* Process Add Source operation after the Remove Source operations are complete. */
    scan_delegator_client_add_broadcast_source_t *pending_add_source = PanicUnlessMalloc(sizeof(scan_delegator_client_add_broadcast_source_t));
    memcpy(pending_add_source, add_broadcast_source, sizeof(scan_delegator_client_add_broadcast_source_t));
    
    if (pending_add_source->num_subgroups)
    {
        pending_add_source->subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * pending_add_source->num_subgroups);
        for (uint8 i=0; i<pending_add_source->num_subgroups; i++)
        {
            pending_add_source->subgroups[i].bis_sync = add_broadcast_source->subgroups[i].bis_sync;
            pending_add_source->subgroups[i].metadata_length = add_broadcast_source->subgroups[i].metadata_length;
            if (pending_add_source->subgroups[i].metadata_length)
            {
                pending_add_source->subgroups[i].metadata = PanicUnlessMalloc(add_broadcast_source->subgroups[i].metadata_length);
                memcpy(pending_add_source->subgroups[i].metadata, add_broadcast_source->subgroups[i].metadata, add_broadcast_source->subgroups[i].metadata_length);
            }
            else
            {
                pending_add_source->subgroups[i].metadata = NULL;
            }
        }
    }
    else
    {
        pending_add_source->subgroups = NULL;
    }
    
    return pending_add_source;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) */
