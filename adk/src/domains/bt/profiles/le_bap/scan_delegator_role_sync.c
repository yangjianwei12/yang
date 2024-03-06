/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief      When the peer earbud connects, this component inserts itself between
                the callbacks from scan_delegator_role to le_broadcast_manager. It
                transmits some of the messages from the scan delegator to the
                secondary, maintaining sync between primary and secondary.

                On initial connection, the current state of the broadcast sources is
                sent to the secondary.
*/

#if defined(INCLUDE_MIRRORING) && defined(INCLUDE_LE_AUDIO_BROADCAST)

#include "scan_delegator_role_sync.h"
#include "scan_delegator_role_sync_marshal_typedef.h"
#include "scan_delegator_role_sync_typedef.h"

#include "peer_signalling.h"
#include "mirror_profile.h"

#include <logging.h>

static struct
{
    /*! The component's task data */
    TaskData task;
    /*! The callbacks to call in the sync chain */
    const LeBapScanDelegator_callback_interface_t * callbacks;

} scan_delegator_role_sync_task_data;

/*! Get the component's task */
#define leBapSDS_GetTask() &scan_delegator_role_sync_task_data.task
/*! Get the downstream callbacks */
#define leBapSDS_GetCallbacks() scan_delegator_role_sync_task_data.callbacks

static void leBapSDS_MessageHandler(Task task, MessageId id, Message msg);
static void leBapSDS_HandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind);
static void leBapSDS_HandleMarshalMsgTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm);
static void leBapSDS_HandleMarshalMsgRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind);

static void leBapSDS_RemoteScanningStart(void);
static void leBapSDS_RemoteScanningStop(void);
static void leBapSDS_AddSource(scan_delegator_client_add_broadcast_source_t * source);
static void leBapSDS_ModifySource(scan_delegator_client_modify_broadcast_source_t * source);
static void leBapSDS_BroadcastCode(scan_delegator_client_broadcast_code_t * code);
static void leBapSDS_RemoveSource(scan_delegator_client_remove_broadcast_source_t * source);
static void leBapSDS_PeriodicSync(scan_delegator_periodic_sync_t * sync);
static void * leBapSDS_RetrieveClientConfig(gatt_cid_t cid);
static void leBapSDS_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size);
static scan_delegator_target_sync_state_t leBapSDS_GetTargetSyncState(uint8 source_id);


static const LeBapScanDelegator_callback_interface_t bap_scan_delegator_sync_interface =
{
    .LeBapScanDelegator_RemoteScanningStart = leBapSDS_RemoteScanningStart,
    .LeBapScanDelegator_RemoteScanningStop = leBapSDS_RemoteScanningStop,
    .LeBapScanDelegator_AddSource = leBapSDS_AddSource,
    .LeBapScanDelegator_ModifySource = leBapSDS_ModifySource,
    .LeBapScanDelegator_BroadcastCode = leBapSDS_BroadcastCode,
    .LeBapScanDelegator_RemoveSource = leBapSDS_RemoveSource,
    .LeBapScanDelegator_PeriodicSync = leBapSDS_PeriodicSync,
    .LeBapScanDelegator_RetrieveClientConfig = leBapSDS_RetrieveClientConfig,
    .LeBapScanDelegator_StoreClientConfig = leBapSDS_StoreClientConfig,
    .LeBapScanDelegator_GetTargetSyncState = leBapSDS_GetTargetSyncState
};

void LeBapScanDelegatorSync_Init(void)
{
    scan_delegator_role_sync_task_data.task.handler = leBapSDS_MessageHandler;

    appPeerSigClientRegister(leBapSDS_GetTask());

    appPeerSigMarshalledMsgChannelTaskRegister(leBapSDS_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                               scan_delegator_role_sync_marshal_type_descriptors,
                                               NUMBER_OF_SCAN_DELEGATOR_ROLE_SYNC_MARSHAL_TYPES);
}

static void leBapSDS_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case PEER_SIG_CONNECTION_IND:
            leBapSDS_HandlePeerSigConnectionInd(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            leBapSDS_HandleMarshalMsgTxCfm(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            leBapSDS_HandleMarshalMsgRxInd(msg);
            break;

        default:
            break;
    }
}

static void leBapSDS_RemoteScanningStart(void)
{
    leBapSDS_GetCallbacks()->LeBapScanDelegator_RemoteScanningStart();
}

static void leBapSDS_RemoteScanningStop(void)
{
    leBapSDS_GetCallbacks()->LeBapScanDelegator_RemoteScanningStop();
}

/*! \brief When sending a valid BIS index to the secondary, send "no preference".
    This allow the secondary to select the correct BIS to sync to depending on
    whether it is left or right earbud */
static uint32 leBapSDS_ConvertBis(uint8 source_id, uint32 bis_in)
{
    UNUSED(source_id);
    return bis_in ? SCAN_DELEGATOR_CLIENT_BIS_SYNC_NO_PREFERENCE : 0;
}

static uint8 leBapSDS_CalculateSubgroupsLength(uint8 num_subgroups, le_bm_source_subgroup_t *subgroups)
{
    uint8 subgroups_length = 0;
    
    for (int i=0; i<num_subgroups; i++)
    {
        subgroups_length += 5; /* Space for bis_sync (4 octets) and metadata_length (1 octet) */
        subgroups_length += subgroups[i].metadata_length;
    }
    
    return subgroups_length;
}

static void populateSubgroupsInSyncMsg(uint16 num_subgroups, uint8 **subgroups_sync, le_bm_source_subgroup_t *subGroupsData)
{
    uint8 *ptr = *subgroups_sync;
    
    for (int subgroup_count=0; subgroup_count<num_subgroups; subgroup_count++)
    {
        *ptr++ = subGroupsData[subgroup_count].bis_sync;
        *ptr++ = subGroupsData[subgroup_count].bis_sync >> 8;
        *ptr++ = subGroupsData[subgroup_count].bis_sync >> 16;
        *ptr++ = subGroupsData[subgroup_count].bis_sync >> 24;
        *ptr++ = subGroupsData[subgroup_count].metadata_length;
        if (subGroupsData[subgroup_count].metadata_length)
        {
            memmove(ptr, subGroupsData[subgroup_count].metadata, subGroupsData[subgroup_count].metadata_length);
            ptr += subGroupsData[subgroup_count].metadata_length;
        }
    }
}

static void leBapSDS_AddSource(scan_delegator_client_add_broadcast_source_t * source)
{
    uint8 subgroups_length = leBapSDS_CalculateSubgroupsLength(source->num_subgroups, source->subgroups);
    size_t size = offsetof(scan_delegator_sync_add_broadcast_source_t, subgroups) + subgroups_length;
    uint8 *ptr = NULL;
    scan_delegator_sync_add_broadcast_source_t *sync = PanicUnlessMalloc(size);
    
    memmove(sync, source, offsetof(scan_delegator_client_add_broadcast_source_t, subgroups));
    sync->subgroups_length = subgroups_length;
    ptr = sync->subgroups;
    
    populateSubgroupsInSyncMsg(sync->num_subgroups, &ptr, source->subgroups);

    appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                     sync,
                                     MARSHAL_TYPE(scan_delegator_sync_add_broadcast_source_t));
    leBapSDS_GetCallbacks()->LeBapScanDelegator_AddSource(source);
}

static void leBapSDS_ModifySource(scan_delegator_client_modify_broadcast_source_t * source)
{
    uint8 subgroups_length = leBapSDS_CalculateSubgroupsLength(source->num_subgroups, source->subgroups);
    size_t size = offsetof(scan_delegator_sync_modify_broadcast_source_t, subgroups) + subgroups_length;
    uint8 *ptr = NULL;
    scan_delegator_sync_modify_broadcast_source_t *sync = PanicUnlessMalloc(size);
    
    memmove(sync, source, offsetof(scan_delegator_client_modify_broadcast_source_t, subgroups));
    sync->subgroups_length = subgroups_length;
    ptr = sync->subgroups;
    
    populateSubgroupsInSyncMsg(sync->num_subgroups, &ptr, source->subgroups);
    
    appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                     sync,
                                     MARSHAL_TYPE(scan_delegator_sync_modify_broadcast_source_t));
    leBapSDS_GetCallbacks()->LeBapScanDelegator_ModifySource(source);
}

static void leBapSDS_BroadcastCode(scan_delegator_client_broadcast_code_t * code)
{
    scan_delegator_sync_broadcast_code_t *sync = PanicUnlessMalloc(sizeof(*sync));
    memmove(sync, code, offsetof(scan_delegator_client_broadcast_code_t, broadcast_code));
    memmove(&sync->broadcast_code, code->broadcast_code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

    appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                     sync,
                                     MARSHAL_TYPE(scan_delegator_sync_broadcast_code_t));
    leBapSDS_GetCallbacks()->LeBapScanDelegator_BroadcastCode(code);
}

static void leBapSDS_RemoveSource(scan_delegator_client_remove_broadcast_source_t * source)
{
    scan_delegator_sync_remove_broadcast_source_t *sync = PanicUnlessMalloc(sizeof(*sync));
    memmove(sync, source, sizeof(*sync));
    appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                     sync,
                                     MARSHAL_TYPE(scan_delegator_sync_remove_broadcast_source_t));
    leBapSDS_GetCallbacks()->LeBapScanDelegator_RemoveSource(source);
}

static void leBapSDS_PeriodicSync(scan_delegator_periodic_sync_t * sync)
{
    /* Sync not required */
    leBapSDS_GetCallbacks()->LeBapScanDelegator_PeriodicSync(sync);
}

static void * leBapSDS_RetrieveClientConfig(gatt_cid_t cid)
{
    /* Sync not required */
    return leBapSDS_GetCallbacks()->LeBapScanDelegator_RetrieveClientConfig(cid);
}

static void leBapSDS_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size)
{
    /* Sync not required */
    leBapSDS_GetCallbacks()->LeBapScanDelegator_StoreClientConfig(cid, config, size);
}

static scan_delegator_target_sync_state_t leBapSDS_GetTargetSyncState(uint8 source_id)
{
    return leBapSDS_GetCallbacks()->LeBapScanDelegator_GetTargetSyncState(source_id);
}

static void leBapSDS_SendInitialStateToPeer(void)
{
    unsigned index;
    uint16 num_source_ids;
    uint8 *source_ids = LeBapScanDelegator_GetBroadcastSourceIds(&num_source_ids);
    
    for (index = 0; index < num_source_ids; index++)
    {
        uint8 source_id = source_ids[index];
        scan_delegator_server_get_broadcast_source_state_t source_state;
        if ((source_id != SCAN_DELEGATOR_SOURCE_ID_INVALID) && (LeBapScanDelegator_GetBroadcastSourceState(source_id, &source_state) == scan_delegator_status_success))
        {
            if (!BdaddrTypedIsEmpty(&source_state.source_address))
            {
                scan_delegator_target_sync_state_t target_sync_state = leBapSDS_GetCallbacks()->LeBapScanDelegator_GetTargetSyncState(source_id);
                uint8 subgroups_length = leBapSDS_CalculateSubgroupsLength(source_state.num_subgroups, source_state.subgroups);
                size_t size = offsetof(scan_delegator_sync_add_broadcast_source_t, subgroups) + subgroups_length;
                scan_delegator_sync_add_broadcast_source_t *add_source;
                uint8 i;
                uint8 *ptr = NULL;
                uint32 bis_sync;

                source_state.num_subgroups = target_sync_state.num_subgroups;

                DEBUG_LOG_VERBOSE("leBapSDS_SendInitialStateToPeer source_id %d num_subgroups %d", source_id, source_state.num_subgroups);

                add_source = PanicUnlessMalloc(size);
                
                add_source->pa_sync = target_sync_state.pa_sync;
                DEBUG_LOG_VERBOSE(" leBapSDS_SendInitialStateToPeer enum:scan_delegator_client_pa_sync_t:%d", add_source->pa_sync);
                
                add_source->advertiser_address = source_state.source_address;
                add_source->broadcast_id = source_state.broadcast_id;
                add_source->source_adv_sid = source_state.source_adv_sid;
                add_source->pa_interval = LE_BM_PA_INTERVAL_UNKNOWN;
                add_source->assistant_address = target_sync_state.assistant_address;
                add_source->num_subgroups = source_state.num_subgroups;
                add_source->subgroups_length = subgroups_length;
                
                for (i=0; i<add_source->num_subgroups; i++)
                {
                    if (target_sync_state.bis_sync != NULL)
                    {
                        bis_sync = leBapSDS_ConvertBis(0, target_sync_state.bis_sync[i]);
                    }
                    else
                    {
                        bis_sync = leBapSDS_ConvertBis(0, source_state.subgroups[i].bis_sync);
                    }
                    DEBUG_LOG_VERBOSE(" leBapSDS_SendInitialStateToPeer subgroup %d bis_sync 0x%x", i, bis_sync);
                    
                    source_state.subgroups[i].bis_sync = bis_sync;
                }
                
                ptr = add_source->subgroups;
                populateSubgroupsInSyncMsg(add_source->num_subgroups, &ptr, source_state.subgroups);

                appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                                PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                                add_source,
                                                MARSHAL_TYPE(scan_delegator_sync_add_broadcast_source_t));

                if (source_state.big_encryption != scan_delegator_server_big_encryption_not_encrypted)
                {
                    scan_delegator_sync_broadcast_code_t *code_sync = PanicUnlessMalloc(sizeof(*code_sync));
                    uint8 *code = PanicNull(LeBapScanDelegator_GetBroadcastCode(source_id));
                    code_sync->source_id = source_id;
                    memmove(&code_sync->broadcast_code, code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
                    free(code);

                    appPeerSigMarshalledMsgChannelTx(leBapSDS_GetTask(),
                                                    PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST,
                                                    code_sync,
                                                    MARSHAL_TYPE(scan_delegator_sync_broadcast_code_t));
                }
            }
            LeBapScanDelegator_FreeBroadcastSourceState(&source_state);
        }
    }

    if (source_ids)
    {
        free(source_ids);
    }
}

/*! \brief Unsynchronise from broadcast sources. */
static void leBapSDS_UnsyncSources(bool remove_source)
{
    unsigned index;
    uint16 num_source_ids;
    uint8 *source_ids = LeBapScanDelegator_GetBroadcastSourceIds(&num_source_ids);
    for (index = 0; index < num_source_ids; index++)
    {
        scan_delegator_server_get_broadcast_source_state_t source_state;
        if ((source_ids[index] != SCAN_DELEGATOR_SOURCE_ID_INVALID) && (LeBapScanDelegator_GetBroadcastSourceState(source_ids[index], &source_state) == scan_delegator_status_success))
        {
            if (!BdaddrTypedIsEmpty(&source_state.source_address))
            {
                scan_delegator_client_modify_broadcast_source_t source;
                source.source_id = source_ids[index];
                source.pa_sync = le_bm_pa_sync_none;
                source.pa_interval = LE_BM_PA_INTERVAL_UNKNOWN;
                BdaddrTypedSetEmpty(&source.assistant_address);
                source.num_subgroups = source_state.num_subgroups;
                source.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * source.num_subgroups);
                for (uint8 i=0; i<source.num_subgroups; i++)
                {
                    source.subgroups[i].bis_sync = 0;
                    source.subgroups[i].metadata_length = source_state.subgroups[i].metadata_length;
                    if (source.subgroups[i].metadata_length)
                    {
                        source.subgroups[i].metadata = PanicUnlessMalloc(source.subgroups[i].metadata_length);
                        memmove(source.subgroups[i].metadata, source_state.subgroups[i].metadata, source.subgroups[i].metadata_length);
                    }
                    else
                    {
                        source.subgroups[i].metadata = NULL;
                    }
                }
                leBapSDS_GetCallbacks()->LeBapScanDelegator_ModifySource(&source);
                LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(source.num_subgroups, source.subgroups);
                if (remove_source)
                {
                    scan_delegator_client_remove_broadcast_source_t remove;
                    remove.source_id = source_ids[index];
                    leBapSDS_GetCallbacks()->LeBapScanDelegator_RemoveSource(&remove);
                }
            }
            LeBapScanDelegator_FreeBroadcastSourceState(&source_state);
        }
    }

    if (source_ids)
    {
        free(source_ids);
    }
}

static inline void leBapSDS_UnsyncAndRemoveSources(void)
{
    leBapSDS_UnsyncSources(TRUE);
}

static inline void leBapSDS_UnsyncDontRemoveSources(void)
{
    leBapSDS_UnsyncSources(FALSE);
}

static void leBapSDS_HandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    switch (ind->status)
    {
        case peerSigStatusConnected:
            /* Plug into the interface to intercept the scan delegator messages */
            scan_delegator_role_sync_task_data.callbacks =
                LeBapScanDelegator_RegisterCallbacks(&bap_scan_delegator_sync_interface);

            /* The primary sends the current source state to the secondary when
               it connects. */
            if (MirrorProfile_IsRolePrimary())
            {
                leBapSDS_SendInitialStateToPeer();
            }
            else
            {
                /* Unsync and remove sources - the primary will send an updated
                   list of sources after connection. */
                leBapSDS_UnsyncAndRemoveSources();
            }
            break;

        case peerSigStatusDisconnected:
        case peerSigStatusLinkLoss:
            if (!MirrorProfile_IsRolePrimary())
            {
                /* The secondary un-synchronises from any broadcast sources when the
                   primary disconnects. It does not remove the sources as the
                   secondary may subsequently become primary - if the sources
                   were removed, the user would have to add then again. */
                leBapSDS_UnsyncDontRemoveSources();
            }

            /* Unplug from the interface */
            LeBapScanDelegator_RegisterCallbacks(scan_delegator_role_sync_task_data.callbacks);
            scan_delegator_role_sync_task_data.callbacks = NULL;

            break;

        default:
            /* Ignore */
            break;
    }
}

static void leBapSDS_HandleMarshalMsgTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);
}

static void leBapSDS_PopulateDelegatorSubgroupsFromSyncMsg(uint8 num_subgroups, le_bm_source_subgroup_t **delegator_subgroups, const uint8 *sync_subgroups)
{
    const uint8 *sync_ptr = sync_subgroups;
    le_bm_source_subgroup_t *delegator_ptr = *delegator_subgroups;
    
    for (uint8 i=0; i<num_subgroups; i++)
    {
        delegator_ptr[i].bis_sync = ((uint32) (*sync_ptr++));
        delegator_ptr[i].bis_sync |= ((uint32) (*sync_ptr++)) << 8;
        delegator_ptr[i].bis_sync |= ((uint32) (*sync_ptr++)) << 16;
        delegator_ptr[i].bis_sync |= ((uint32) (*sync_ptr++)) << 24;
        delegator_ptr[i].metadata_length = *sync_ptr++;
        if (delegator_ptr[i].metadata_length)
        {
            delegator_ptr[i].metadata = PanicUnlessMalloc(delegator_ptr[i].metadata_length);
            memmove(delegator_ptr[i].metadata, sync_ptr, delegator_ptr[i].metadata_length);
            sync_ptr += delegator_ptr[i].metadata_length;
        }
        else
        {
            delegator_ptr[i].metadata = NULL;
        }
    }
}

static void leBapSDS_HandleMarshalMsgRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE(scan_delegator_sync_add_broadcast_source_t):
        {
            scan_delegator_sync_add_broadcast_source_t *sync = ind->msg;
            scan_delegator_client_add_broadcast_source_t source;
            memmove(&source, sync, offsetof(scan_delegator_client_add_broadcast_source_t, subgroups));
            source.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * source.num_subgroups);
            
            leBapSDS_PopulateDelegatorSubgroupsFromSyncMsg(source.num_subgroups, &source.subgroups, sync->subgroups);
            
            leBapSDS_GetCallbacks()->LeBapScanDelegator_AddSource(&source);
            LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(source.num_subgroups, source.subgroups);
            break;
        }
        case MARSHAL_TYPE(scan_delegator_sync_remove_broadcast_source_t):
        {
            scan_delegator_sync_remove_broadcast_source_t *sync = ind->msg;
            scan_delegator_client_remove_broadcast_source_t source;
            memmove(&source, sync, sizeof(source));
            leBapSDS_GetCallbacks()->LeBapScanDelegator_RemoveSource(&source);
            break;
        }
        case MARSHAL_TYPE(scan_delegator_sync_modify_broadcast_source_t):
        {
            scan_delegator_sync_modify_broadcast_source_t *sync = ind->msg;
            scan_delegator_client_modify_broadcast_source_t source;
            memmove(&source, sync, offsetof(scan_delegator_client_modify_broadcast_source_t, subgroups));
            source.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * source.num_subgroups);
            
            leBapSDS_PopulateDelegatorSubgroupsFromSyncMsg(source.num_subgroups, &source.subgroups, sync->subgroups);
            
            leBapSDS_GetCallbacks()->LeBapScanDelegator_ModifySource(&source);
            LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(source.num_subgroups, source.subgroups);
            break;
        }
        case MARSHAL_TYPE(scan_delegator_sync_broadcast_code_t):
        {
            scan_delegator_sync_broadcast_code_t *sync = ind->msg;
            scan_delegator_client_broadcast_code_t code;
            uint8 *value = PanicUnlessMalloc(SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
            memmove(&code, sync, offsetof(scan_delegator_client_broadcast_code_t, broadcast_code));
            memmove(value, &sync->broadcast_code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);
            /* The LE broadcast manager doesn't update the server's code in response
               to receiving this message, so the code is manually updated in the 
               BASS server here */
            LeBapScanDelegator_SetBroadcastCode(code.source_id, value);
            code.broadcast_code = value;
            leBapSDS_GetCallbacks()->LeBapScanDelegator_BroadcastCode(&code);
            free(value);
            break;
        }
        default:
            break;
    }
    free(ind->msg);
}

#endif
