/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Implementation for the LE Audio Broadcast BASS
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

#include "le_broadcast_manager.h"
#include "le_broadcast_manager_scan_delegator.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_sync.h"
#include "le_broadcast_manager_bass.h"

#define BROADCAST_MANAGER_BASS_LOG   DEBUG_LOG_VERBOSE

#define LE_BM_MAX_NUMBER_OF_BASS_CLIENTS 10

typedef struct
{
    le_broadcast_manager_source_state_client_if_t * callback_if;
}client_db_entry;

static client_db_entry client_db[LE_BM_MAX_NUMBER_OF_BASS_CLIENTS];

void LeBroadcastManager_BassInit(void)
{
    memset(client_db, 0, sizeof(client_db_entry) * ARRAY_DIM(client_db));
}

le_bm_bass_status_t LeBroadcastManager_BassGetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    BROADCAST_MANAGER_BASS_LOG("LeBroadcastManager_BassGetBroadcastSourceState for source id %d", source_id);

    return leBroadcastManager_GetBroadcastSourceState(source_id, source_state);
}

void LeBroadcastManager_BassFreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    BROADCAST_MANAGER_BASS_LOG("LeBroadcastManager_BassGetBroadcastSourceState source_state %p", source_state);

    leBroadcastManager_FreeBroadcastSourceState(source_state);
}

void LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(uint8 source_id)
{
    BROADCAST_MANAGER_BASS_LOG("LeBroadcastManager_BassSendSourceStateChangedNotificationToClients id %d", source_id);

    client_db_entry * item;

    if(source_id)
    {
        ARRAY_FOREACH(item, client_db)
        {
            if( item->callback_if && item->callback_if->NotifySourceStateChanged)
            {
                item->callback_if->NotifySourceStateChanged(source_id);
            }
        }
    }
}

le_bm_bass_status_t LeBroadcastManager_BassAddSource(uint8 *source_id, const le_bm_add_source_info_t *add_source_info)
{
    le_bm_bass_status_t status;

    scan_delegator_client_add_broadcast_source_t scan_delegator_source =
    {
        .pa_sync = add_source_info->pa_sync,
        .advertiser_address = add_source_info->advertiser_address,
        .broadcast_id = add_source_info->broadcast_id,
        .source_adv_sid = add_source_info->advertising_sid,
        .pa_interval = add_source_info->pa_interval,
        .assistant_address = add_source_info->assistant_address,
        .num_subgroups = add_source_info->num_subgroups,
        .subgroups = add_source_info->subgroups
    };

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassAddSource");

    status = LeBroadcastManager_AddSource(source_id, &scan_delegator_source);

    if (status == le_bm_bass_status_success)
    {
        LeBroadcastManager_SyncAddSource(&scan_delegator_source);
    }


    return status;
}


le_bm_bass_status_t LeBroadcastManager_BassModifySource(uint8 source_id, const le_bm_modify_source_info_t *modify_source_info)
{    
    le_bm_bass_status_t status;

    scan_delegator_client_modify_broadcast_source_t scan_delegator_source =
    {
        .source_id = source_id,
        .pa_sync = modify_source_info->pa_sync,
        .pa_interval = modify_source_info->pa_interval,
        .num_subgroups = modify_source_info->num_subgroups,
        .subgroups = modify_source_info->subgroups
    };

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassModifySource");

    status = LeBroadcastManager_SourceModify(&scan_delegator_source);

    if (status == le_bm_bass_status_success)
    {
        LeBroadcastManager_SyncModifySource(&scan_delegator_source);
    }

    return status;
}


le_bm_bass_status_t LeBroadcastManager_BassRemoveSource(uint8 source_id)
{
    le_bm_bass_status_t status;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassRemoveSource");

    status = LeBroadcastManager_RemoveSource(source_id);

    if (status == le_bm_bass_status_success)
    {
        LeBroadcastManager_SyncRemoveSource(source_id);
    }

    return status;
}


le_bm_bass_status_t LeBroadcastManager_BassSetBroadcastCode(uint8 source_id, uint8 *broadcast_code)
{
    le_bm_bass_status_t status;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassSetBroadcastCode");

    status = LeBapScanDelegator_SetBroadcastCode(source_id, broadcast_code);

    if (status == le_bm_bass_status_success)
    {
        LeBroadcastManager_SyncSetBroadcastCode(source_id, broadcast_code);
    }

    return status;
}

uint32 LeBroadcastManager_BassClientRegister(le_broadcast_manager_source_state_client_if_t * client_callback_if)
{
    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassClientRegister");

    uint32 client_id = INVALID_LE_BM_BASS_CLIENT_ID;

    if(client_callback_if)
    {
        for(uint32 i=0; i < ARRAY_DIM(client_db);i++)
        {
            if(client_db[i].callback_if == NULL)
            {
                client_db[i].callback_if = client_callback_if;
                client_id = i+1;
                break;
            }
        }
    }

    return client_id;
}

void LeBroadcastManager_BassClientDeregister(uint32 client_id)
{
    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_BassClientDeregister");

    if((client_id != INVALID_LE_BM_BASS_CLIENT_ID)
        && (client_id > 0)
        && (client_id <= LE_BM_MAX_NUMBER_OF_BASS_CLIENTS))
    {
        client_db[client_id-1].callback_if = NULL;
    }
}

le_bm_bass_status_t LeBroadcastManager_BassSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr)
{
    le_bm_bass_status_t status = LeBroadcastManager_SourceSetMatchAddress(source_id, taddr);

    if (status == le_bm_bass_status_success)
    {
        LeBroadcastManager_SyncSetSourceMatchAddress(source_id, taddr);
    }

    return status;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */
