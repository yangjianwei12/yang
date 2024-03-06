/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Handles the data associated with the LE Broadcast Manager.
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST

#include "le_broadcast_manager_data.h"

#include "audio_info.h"
#include "audio_sources.h"
#include "voice_sources.h"

#include <panic.h>

void leBroadcastManager_SetTargetBisSyncState(broadcast_source_state_t *broadcast_source, uint8 num_subgroups, uint32 *bis_sync_state)
{
    uint8 i;
    
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetTargetBisSyncState source_id %u num_subgroups=[old:0x%x new:0x%x]",
                               broadcast_source->source_id, broadcast_source->target_bis_sync_state.num_subgroups, num_subgroups);
    
    if (num_subgroups != broadcast_source->target_bis_sync_state.num_subgroups)
    {
        leBroadcastManager_ResetTargetBisSyncState(broadcast_source);
        if (num_subgroups)
        {
            broadcast_source->target_bis_sync_state.bis_sync = PanicUnlessMalloc(sizeof(uint32) * num_subgroups);
        }
    }
    
    broadcast_source->target_bis_sync_state.num_subgroups = num_subgroups;
    
    for (i=0; i<num_subgroups; i++)
    {
        BROADCAST_MANAGER_DATA_LOG("  bis_sync_state=0x%x", bis_sync_state[i]);
        broadcast_source->target_bis_sync_state.bis_sync[i] = bis_sync_state[i];
    }
    
    leBroadcastManager_SetSyncToBisNoPreference(broadcast_source, FALSE);
}

void leBroadcastManager_ResetTargetBisSyncState(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_ResetTargetBisSyncState source_id %u",
                               broadcast_source->source_id);

    if (broadcast_source->target_bis_sync_state.bis_sync)
    {
        free(broadcast_source->target_bis_sync_state.bis_sync);
        broadcast_source->target_bis_sync_state.bis_sync = NULL;
    }
}

void leBroadcastManager_SetTargetBisSyncStateNoSync(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetTargetBisSyncStateNoSync source_id %u",
                               broadcast_source->source_id);

    if (broadcast_source->target_bis_sync_state.num_subgroups == 0)
    {
        broadcast_source->target_bis_sync_state.num_subgroups = 1;
        broadcast_source->target_bis_sync_state.bis_sync = PanicUnlessMalloc(sizeof(uint32));
    }
    for (uint8 i=0; i<broadcast_source->target_bis_sync_state.num_subgroups; i++)
    {
        broadcast_source->target_bis_sync_state.bis_sync[i] = 0;
    }
    leBroadcastManager_SetSyncToBisNoPreference(broadcast_source, FALSE);
}

void leBroadcastManager_SetTargetBisSyncStateNoPreference(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetTargetBisSyncStateNoPreference source_id %u",
                               broadcast_source->source_id);

    if (broadcast_source->target_bis_sync_state.num_subgroups == 0)
    {
        broadcast_source->target_bis_sync_state.num_subgroups = 1;
        broadcast_source->target_bis_sync_state.bis_sync = PanicUnlessMalloc(sizeof(uint32));
    }
    for (uint8 i=0; i<broadcast_source->target_bis_sync_state.num_subgroups; i++)
    {
        broadcast_source->target_bis_sync_state.bis_sync[i] = BIS_SYNC_NO_PREFERENCE;
    }
}

uint32 leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source_state_t *broadcast_source)
{
    uint32 target_bis_sync_state = 0;
    
    for (uint8 i=0; i<broadcast_source->target_bis_sync_state.num_subgroups; i++)
    {
        target_bis_sync_state |= broadcast_source->target_bis_sync_state.bis_sync[i];
    }

    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_GetTargetBisSyncStateValue source_id %u target_bis_sync 0x%x",
                               broadcast_source->source_id, target_bis_sync_state);
    
    return target_bis_sync_state;
}

le_broadcast_manager_bis_sync_state_t *leBroadcastManager_GetTargetBisSyncState(broadcast_source_state_t *broadcast_source)
{
    return &broadcast_source->target_bis_sync_state;
}

uint32 leBroadcastManager_GetBisSyncStateValue(const le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    uint32 bis_value = 0;

    for (uint8 i=0; i<bis_sync_state->num_subgroups; i++)
    {
        bis_value |= bis_sync_state->bis_sync[i];
    }

    return bis_value;
}

void LeBroadcastManager_ResetRequestedBisSyncState(broadcast_source_state_t *broadcast_source)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_ResetRequestedBisSyncState source_id %u",
                               broadcast_source->source_id);

    if (broadcast_source->requested_bis_sync_state.bis_sync)
    {
        free(broadcast_source->requested_bis_sync_state.bis_sync);
        broadcast_source->requested_bis_sync_state.bis_sync = NULL;
    }
}

void LeBroadcastManager_SetRequestedBisSyncState(broadcast_source_state_t *broadcast_source, const le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    uint8 i;

    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetRequestedBisSyncState source_id %u num_subgroups=[old:0x%x new:0x%x]",
                               broadcast_source->source_id,
                               broadcast_source->requested_bis_sync_state.num_subgroups, bis_sync_state->num_subgroups);

    if (bis_sync_state->num_subgroups != broadcast_source->requested_bis_sync_state.num_subgroups)
    {
        LeBroadcastManager_ResetRequestedBisSyncState(broadcast_source);
        if (bis_sync_state->num_subgroups)
        {
            broadcast_source->requested_bis_sync_state.bis_sync = PanicUnlessMalloc(sizeof(uint32) * bis_sync_state->num_subgroups);
        }
    }

    broadcast_source->requested_bis_sync_state.num_subgroups = bis_sync_state->num_subgroups;

    for (i = 0; i < bis_sync_state->num_subgroups; i++)
    {
        BROADCAST_MANAGER_DATA_LOG("  bis_sync_state=0x%x", bis_sync_state->bis_sync[i]);
        broadcast_source->requested_bis_sync_state.bis_sync[i] = bis_sync_state->bis_sync[i];
    }
}

const le_broadcast_manager_bis_sync_state_t *LeBroadcastManager_GetRequestedBisSyncState(broadcast_source_state_t *broadcast_source)
{
    return &broadcast_source->requested_bis_sync_state;
}

uint32 LeBroadcastManager_GetRequestedBisSyncStateValue(broadcast_source_state_t *broadcast_source)
{
    uint32 requested_bis_sync_state = 0;

    for (uint8 i=0; i<broadcast_source->requested_bis_sync_state.num_subgroups; i++)
    {
        requested_bis_sync_state |= broadcast_source->requested_bis_sync_state.bis_sync[i];
    }

    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_GetRequestedBisSyncState source_id %u target_bis_sync 0x%x",
                               broadcast_source->source_id, requested_bis_sync_state);

    return requested_bis_sync_state;
}

void leBroadcastManager_SetPausedState(uint8 source_id, const le_broadcast_manager_bis_sync_state_t *state)
{
    size_t size_bis_sync = sizeof(uint32) * state->num_subgroups;
    
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetPausedState source_id %u", source_id);
    
    PanicNotNull(le_broadcast_manager.paused_bis_sync_state.bis_sync);
    le_broadcast_manager.paused_bis_source_id = source_id;
    le_broadcast_manager.paused_bis_sync_state.num_subgroups = state->num_subgroups;
    if (size_bis_sync)
    {
        le_broadcast_manager.paused_bis_sync_state.bis_sync = PanicUnlessMalloc(size_bis_sync);
        memcpy(le_broadcast_manager.paused_bis_sync_state.bis_sync, state->bis_sync, size_bis_sync);
    }
    else
    {
        le_broadcast_manager.paused_bis_sync_state.bis_sync = NULL;
    }
}


void leBroadcastManager_ResetPausedState(void)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_ResetPausedState source_id=%d", le_broadcast_manager.paused_bis_source_id);
    
    le_broadcast_manager.paused_bis_source_id = 0;
    le_broadcast_manager.paused_bis_sync_state.num_subgroups = 0;
    if (le_broadcast_manager.paused_bis_sync_state.bis_sync)
    {
        free(le_broadcast_manager.paused_bis_sync_state.bis_sync);
        le_broadcast_manager.paused_bis_sync_state.bis_sync = NULL;
    }
}

void leBroadcastManager_SetSyncState(uint8 source_id, broadcast_manager_sync_state_t state)
{
    BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetSyncState source_id=%u sync_state=enum:broadcast_manager_sync_state_t:%u", source_id, state);
    le_broadcast_manager.sync_source_id = source_id;
    le_broadcast_manager.sync_state = state;
}

void leBroadcastManager_SetPendingBisSyncState(const le_broadcast_manager_bis_sync_state_t *bis_sync_state)
{
    size_t size_bis_sync = sizeof(uint32) * bis_sync_state->num_subgroups;
    if (le_broadcast_manager.pending_bis_sync_state.bis_sync)
    {
        free(le_broadcast_manager.pending_bis_sync_state.bis_sync);
    }
    le_broadcast_manager.pending_bis_sync_state.num_subgroups = bis_sync_state->num_subgroups;
    if (size_bis_sync)
    {
        le_broadcast_manager.pending_bis_sync_state.bis_sync = PanicUnlessMalloc(size_bis_sync);
        memcpy(le_broadcast_manager.pending_bis_sync_state.bis_sync, bis_sync_state->bis_sync, size_bis_sync);
    }
    else
    {
        le_broadcast_manager.pending_bis_sync_state.bis_sync = NULL;
    }
}

void leBroadcastManager_ResetPendingBisSyncState(void)
{
    PanicNull(le_broadcast_manager.pending_bis_sync_state.bis_sync);
    le_broadcast_manager.pending_bis_sync_state.num_subgroups = 0;
    free(le_broadcast_manager.pending_bis_sync_state.bis_sync);
    le_broadcast_manager.pending_bis_sync_state.bis_sync = NULL;
}

le_broadcast_manager_bis_sync_state_t *leBroadcastManager_GetPendingBisSyncState(void)
{
    return &le_broadcast_manager.pending_bis_sync_state;
}

bool leBroadcastManager_IsNonBroadcastAudioVoiceActive(void)
{
    bool non_broadcast_routed = FALSE;
    generic_source_t routed_source = AudioInfo_GetRoutedGenericSource();

    if(routed_source.type != source_type_invalid
            && !(routed_source.type == source_type_audio && routed_source.u.audio == audio_source_le_audio_broadcast))
    {
        non_broadcast_routed = TRUE;
    }

    DEBUG_LOG("leBroadcastManager_IsNonBroadcastAudioVoiceActive type: enum:source_type_t:%u source:%d",
            routed_source.type, routed_source.u.audio);

    return non_broadcast_routed;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST */
