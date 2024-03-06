/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Synchronise the state of LE broadcast audio between peer earbuds.
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_MIRRORING)

#include "le_broadcast_manager_config.h"
#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_source.h"
#include "le_broadcast_manager_scan_delegator.h"
#include "le_broadcast_manager_sync.h"
#include "le_broadcast_manager_sync_marshal_typedef.h"
#include "le_broadcast_manager_sync_typedef.h"
#include "le_broadcast_manager.h"

#include "kymera.h"
#include "peer_signalling.h"
#include "mirror_profile.h"

#include <logging.h>
#include <panic.h>
#include <system_clock.h>


typedef struct
{
    /*! The component's task data */
    TaskData task;

    /*! Has the Secondary signalled it is ready to start. */
    uint8 secondary_ready_to_start_source_id;
} le_broadcast_manager_sync_data_t;


#define leBroadcastManager_SyncGet() (&le_broadcast_manager_sync_data)

#define leBroadcastManager_SyncGetTask() (&leBroadcastManager_SyncGet()->task)

#define leBroadcastManager_SyncSetSecondaryReadyToStartSourceId(source_id) (leBroadcastManager_SyncGet()->secondary_ready_to_start_source_id = (source_id))

#define leBroadcastManager_SyncGetSecondaryReadyToStartSourceId() (leBroadcastManager_SyncGet()->secondary_ready_to_start_source_id)


le_broadcast_manager_sync_data_t le_broadcast_manager_sync_data;


void LeBroadcastManager_SyncSendReadyToStart(uint8 source_id)
{
    le_broadcast_manager_sync_ready_to_start_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->source_id = source_id;

    DEBUG_LOG("leBroadcastManager_SyncSendReadyToStart source_id %u", source_id);

    appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                              PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                              MARSHAL_TYPE(le_broadcast_manager_sync_ready_to_start_ind_t));
    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     (msg), MARSHAL_TYPE(le_broadcast_manager_sync_ready_to_start_ind_t));
}

void LeBroadcastManager_SyncSendUnmuteInd(uint8 source_id, rtime_t time)
{
    le_broadcast_manager_sync_unmute_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->source_id = source_id;
    msg->unmute_time = time;

    DEBUG_LOG("LeBroadcastManager_SyncSendUnmuteInd source_id %u time %u", source_id, time);

    appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                              PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                              MARSHAL_TYPE(le_broadcast_manager_sync_unmute_ind_t));
    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     (msg), MARSHAL_TYPE(le_broadcast_manager_sync_unmute_ind_t));
}

static void leBroadcastManager_SyncUnmuteAudio(rtime_t unmute_time)
{
    MessageCancelFirst(LeBroadcastManager_SourceGetTask(), BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT);
    Kymera_LeAudioUnmute(unmute_time);
    leBroadcastManager_SetStartMuted(FALSE);
}

void LeBroadcastManager_OutOfEarMute(uint8 seconds)
{
    broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();

    if(broadcast_source)
    {
        rtime_t mute_time = 0;

        DEBUG_LOG("LeBroadcastManager_OutOfEarMute %d seconds", seconds);

        if(seconds)
        {
            LeBroadcastManager_SyncSendCommandInd(le_broadcast_sync_command_mute);
            Kymera_LeAudioSyncMute();
            mute_time =  rtime_add(SystemClockGetTimerTime(), (seconds * US_PER_SEC));
        }
        else
        {
            mute_time =  rtime_add(SystemClockGetTimerTime(),
                                (LeBroadcastManager_ConfigSyncStartDelayMs() * US_PER_MS));
        }

        leBroadcastManager_SyncUnmuteAudio(mute_time);
        LeBroadcastManager_SyncSendUnmuteInd(broadcast_source->source_id, mute_time);
    }
}

static void leBroadcastManager_SyncHandleReadyToStartInd(const le_broadcast_manager_sync_ready_to_start_ind_t *ind)
{
    DEBUG_LOG("leBroadcastManager_SyncHandleReadyToStartInd source_id %u role %d",
              ind->source_id, MirrorProfile_IsRolePrimary());

    if (MirrorProfile_IsRolePrimary())
    {
        /* Peer is ready to start. Need to respond to the peer to tell it to
           un-mute, but it should be synchronised with this device if possible.

           If this device has a BIS but is muted add a small delay on so that both
           devices will un-mute at the same time.

           If this device has not yet established a BIS, record that the peer is
           ready so that the un-mute command can be sent as soon as the BIS is
           established.  */
        if (LeBroadcastManager_SourceIsBisSync())
        {
            broadcast_source_state_t *broadcast_source = LeBroadcastManager_GetSourceOfActiveBis();
            
            if (broadcast_source && (broadcast_source->source_id == ind->source_id))
            {
                rtime_t sync_time = SystemClockGetTimerTime();

                /* If this device is still muted, add a delay so both devices un-mute
                   together. If this device is not muted the peer should start as soon
                   as possible. */
                if (leBroadcastManager_GetStartMuted())
                {
                    sync_time = rtime_add(sync_time, (LeBroadcastManager_ConfigSyncStartDelayMs() * US_PER_MS));
                    leBroadcastManager_SyncUnmuteAudio(sync_time);
                }

                LeBroadcastManager_SyncSendUnmuteInd(ind->source_id, sync_time);
            }
        }
        else
        {
            leBroadcastManager_SyncSetSecondaryReadyToStartSourceId(ind->source_id);
        }
    }
}

static void leBroadcastManager_SyncHandleUnmuteInd(const le_broadcast_manager_sync_unmute_ind_t *ind)
{
    DEBUG_LOG("leBroadcastManager_SyncHandleUnmuteInd unmute_time %u role %d",
              ind->unmute_time, MirrorProfile_IsRolePrimary());

    if (!MirrorProfile_IsRolePrimary())
    {
        /* Primary has told the Secondary (this device) when to unmute.
           If a BIS is established un-mute the audio chain.
           Note: If it is already un-muted this is effectively a nop.

           Otherwise ignore it and the Secondary will re-start the sync the
           next time it estabilshes a BIS and creates the audio chain.*/
        if (LeBroadcastManager_SourceIsBisSync())
        {
            leBroadcastManager_SyncUnmuteAudio(ind->unmute_time);
        }
    }
}

static void leBroadcastManager_SyncHandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    DEBUG_LOG("leBroadcastManager_SyncHandlePeerSigConnectionInd status enum:peerSigStatus:%u",
              ind->status);

    switch (ind->status)
    {
        case peerSigStatusConnected:

            break;

        case peerSigStatusDisconnected:
        case peerSigStatusLinkLoss:
            /* If an internal un-mute timeout is queued when the peer
               disconnects, immediately un-mute. */
            if (MessageCancelFirst(LeBroadcastManager_SourceGetTask(), BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT))
            {
                LeBroadcastManager_SyncHandleInternalUnmuteTimeout();
            }

            leBroadcastManager_SyncSetSecondaryReadyToStartSourceId(0);
            break;

        default:
            /* Ignore */
            break;
    }
}

static void leBroadcastManager_SyncHandleUnsyncCommand(void)
{
    LeBroadcastManager_Stop(LeBroadcastManager_SourceGetTask());

    /* Clear the paused state of the broadcast manager. */
    leBroadcastManager_ResetPausedState();

    if (le_broadcast_manager.pause_tasks)
    {
        TaskList_Destroy(le_broadcast_manager.pause_tasks);
        le_broadcast_manager.pause_tasks = NULL;
    }
}

static void leBroadcastManager_SyncHandleCommandInd(const le_broadcast_manager_sync_command_ind_t *ind)
{
    DEBUG_LOG("leBroadcastManager_SyncHandleCommandInd command enum:le_broadcast_sync_command_t:%u", ind->command);

    switch(ind->command)
    {
        case le_broadcast_sync_command_mute:
            Kymera_LeAudioSyncMute();
            break;

        case le_broadcast_sync_command_unsync:
            leBroadcastManager_SyncHandleUnsyncCommand();
            break;

        default:
            break;
    }
}

static void leBroadcastManager_SyncHandlePauseInd(const le_broadcast_manager_sync_pause_ind_t *ind)
{
    if (!MirrorProfile_IsRolePrimary())
    {
        DEBUG_LOG("leBroadcastManager_SyncHandlePauseInd source_id %u", ind->source_id);
        
        LeBroadcastManager_HandleReceivedPauseSource(ind->source_id);
    }
}

static void leBroadcastManager_SyncHandleResumeInd(const le_broadcast_manager_sync_resume_ind_t *ind)
{
    if (!MirrorProfile_IsRolePrimary())
    {
        DEBUG_LOG("leBroadcastManager_SyncHandleResumeInd source_id %u", ind->source_id);
        
        LeBroadcastManager_HandleReceivedResumeSource(ind->source_id);
    }
}

static void leBroadcastManager_SyncHandleSyncToSourceInd(const le_broadcast_manager_sync_to_source_t *ind)
{
    if (!MirrorProfile_IsRolePrimary())
    {
        DEBUG_LOG("leBroadcastManager_SyncHandleSyncToSourceInd source_id %u", ind->source_id);
        LeBroadcastManager_HandleReceivedSyncToSource(ind->source_id);
    }
}

static void leBroadcastManager_SyncPopulateDelegatorSubgroupsFromSyncMsg(uint8 num_subgroups, le_bm_source_subgroup_t **delegator_subgroups, const uint8 *sync_subgroups)
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

static void leBroadcastManager_SyncHandleAddSource(const le_broadcast_manager_sync_add_broadcast_source_t *sync)
{
    scan_delegator_client_add_broadcast_source_t source;

    DEBUG_LOG("leBroadcastManager_SyncHandleAddSource broadcast_id 0x%x", sync->broadcast_id);

    memmove(&source, sync, offsetof(scan_delegator_client_add_broadcast_source_t, subgroups));
    source.subgroups = PanicUnlessMalloc(sizeof(le_bm_source_subgroup_t) * source.num_subgroups);
    leBroadcastManager_SyncPopulateDelegatorSubgroupsFromSyncMsg(source.num_subgroups, &source.subgroups, sync->subgroups);

    LeBroadcastManager_SourceAdd(&source);

    LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(source.num_subgroups, source.subgroups);
}

static void leBroadcastManager_SyncHandleModifySource(const le_broadcast_manager_sync_modify_broadcast_source_t *sync)
{
    scan_delegator_client_modify_broadcast_source_t source;

    DEBUG_LOG_DEBUG("leBroadcastManager_SyncHandleModifySource");

    source.source_id = sync->source_id;
    source.pa_sync = (le_bm_pa_sync_t) sync->pa_sync;
    source.pa_interval = sync->pa_interval;
    source.num_subgroups = sync->num_subgroups;
    source.subgroups = PanicNull(calloc(sync->num_subgroups, sizeof (le_bm_source_subgroup_t)));

    leBroadcastManager_SyncPopulateDelegatorSubgroupsFromSyncMsg(source.num_subgroups, &source.subgroups, sync->subgroups);

    LeBroadcastManager_SourceModify(&source);

    LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(source.num_subgroups, source.subgroups);

}

static void leBroadcastManager_SyncHandleRemoveSource(const le_broadcast_manager_sync_remove_broadcast_source_t *sync)
{
    scan_delegator_client_remove_broadcast_source_t source = {0};

    source.source_id = sync->source_id;

    DEBUG_LOG_DEBUG("leBroadcastManager_SyncHandleRemoveSource: source_id %u", source.source_id);

    LeBroadcastManager_SourceRemove(&source);
}

static void leBroadcastManager_SyncHandleSetBroadcastCode(const le_broadcast_manager_sync_set_broadcast_code_t *sync)
{
    DEBUG_LOG_DEBUG("leBroadcastManager_SyncHandleSetBroadcastCode");

    LeBapScanDelegator_SetBroadcastCode(sync->source_id, (uint8 *) sync->broadcast_code);
}

static void leBroadcastManager_SyncHandleSetSourceMatchAddress(const le_broadcast_manager_sync_set_source_match_address_t *sync)
{
    DEBUG_LOG_DEBUG("leBroadcastManager_SyncHandleSetSourceMatchAddress");

    LeBroadcastManager_SourceSetMatchAddress(sync->source_id, &sync->source_match_address);
}


static void leBroadcastManager_SyncHandleMarshalMsgTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    UNUSED(cfm);
}

static void leBroadcastManager_SyncHandleMarshalMsgRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    switch (ind->type)
    {
    case MARSHAL_TYPE(le_broadcast_manager_sync_ready_to_start_ind_t):
        {
            const le_broadcast_manager_sync_ready_to_start_ind_t *ready_ind = (const le_broadcast_manager_sync_ready_to_start_ind_t *)ind->msg;
            leBroadcastManager_SyncHandleReadyToStartInd(ready_ind);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_unmute_ind_t):
        {
            const le_broadcast_manager_sync_unmute_ind_t *unmute_ind = (const le_broadcast_manager_sync_unmute_ind_t *)ind->msg;
            leBroadcastManager_SyncHandleUnmuteInd(unmute_ind);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_command_ind_t):
        {
            const le_broadcast_manager_sync_command_ind_t *command_ind = (const le_broadcast_manager_sync_command_ind_t *)ind->msg;
            leBroadcastManager_SyncHandleCommandInd(command_ind);
        }
        break;
        
    case MARSHAL_TYPE(le_broadcast_manager_sync_pause_ind_t):
        {
            const le_broadcast_manager_sync_pause_ind_t *pause_ind = (const le_broadcast_manager_sync_pause_ind_t *)ind->msg;
            leBroadcastManager_SyncHandlePauseInd(pause_ind);
        }
        break;
        
    case MARSHAL_TYPE(le_broadcast_manager_sync_resume_ind_t):
        {
            const le_broadcast_manager_sync_resume_ind_t *resume_ind = (const le_broadcast_manager_sync_resume_ind_t *)ind->msg;
            leBroadcastManager_SyncHandleResumeInd(resume_ind);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_to_source_t):
        {
            const le_broadcast_manager_sync_to_source_t *sync_to_source_ind = (const le_broadcast_manager_sync_to_source_t *)ind->msg;
            leBroadcastManager_SyncHandleSyncToSourceInd(sync_to_source_ind);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_add_broadcast_source_t):
        {
            leBroadcastManager_SyncHandleAddSource((const le_broadcast_manager_sync_add_broadcast_source_t *)ind->msg);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_modify_broadcast_source_t):
        {
            leBroadcastManager_SyncHandleModifySource((const le_broadcast_manager_sync_modify_broadcast_source_t *)ind->msg);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_remove_broadcast_source_t):
        {
            leBroadcastManager_SyncHandleRemoveSource((const le_broadcast_manager_sync_remove_broadcast_source_t *)ind->msg);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_set_broadcast_code_t):
        {
            leBroadcastManager_SyncHandleSetBroadcastCode((const le_broadcast_manager_sync_set_broadcast_code_t *)ind->msg);
        }
        break;

    case MARSHAL_TYPE(le_broadcast_manager_sync_set_source_match_address_t):
        {
            leBroadcastManager_SyncHandleSetSourceMatchAddress((const le_broadcast_manager_sync_set_source_match_address_t *)ind->msg);
        }
        break;

    default:
        DEBUG_LOG("leBroadcastManager_SyncHandleMarshalMsgRxInd unhandled type 0x%x", ind->type);
        break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

static void leBroadcastManager_SyncMessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case PEER_SIG_CONNECTION_IND:
            leBroadcastManager_SyncHandlePeerSigConnectionInd(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            leBroadcastManager_SyncHandleMarshalMsgTxCfm(msg);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            leBroadcastManager_SyncHandleMarshalMsgRxInd(msg);
            break;

        default:
            break;
    }
}

void LeBroadcastManager_SyncInit(void)
{
    memset(&le_broadcast_manager_sync_data, 0, sizeof(le_broadcast_manager_sync_data));
    le_broadcast_manager_sync_data.task.handler = leBroadcastManager_SyncMessageHandler;
    leBroadcastManager_SyncSetSecondaryReadyToStartSourceId(0);

    appPeerSigClientRegister(leBroadcastManager_SyncGetTask());

    appPeerSigMarshalledMsgChannelTaskRegister(leBroadcastManager_SyncGetTask(),
                                               PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                               le_broadcast_manager_sync_marshal_type_descriptors,
                                               NUMBER_OF_LE_BROADCAST_MANAGER_SYNC_MARSHAL_TYPES);
}

static void leBroadcastManager_SyncStartUnMuteTimeout(void)
{
    MessageCancelFirst(LeBroadcastManager_SourceGetTask(), BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT);
    MessageSendLater(LeBroadcastManager_SourceGetTask(),
                     BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT,
                     NULL,
                     LeBroadcastManager_ConfigSyncUnMuteTimeoutMs());
}


void LeBroadcastManager_SyncCheckIfStartMuted(broadcast_source_state_t *broadcast_source)
{
    bool start_muted = FALSE;

    DEBUG_LOG_FN_ENTRY("LeBroadcastManager_SyncCheckIfStartMuted peer_connected %u is_primary %u",
                       appPeerSigIsConnected(),
                       MirrorProfile_IsRolePrimary());

    if (appPeerSigIsConnected())
    {
        /* If the peer earbud is connected we want to synchronise
           when the user hears the broadcast audio on both earbuds.
           This is done by starting the broadcast audio muted and
           synchronising when it is un-muted. */
        start_muted = TRUE;

        if (MirrorProfile_IsRolePrimary())
        {
            if (leBroadcastManager_SyncGetSecondaryReadyToStartSourceId() == broadcast_source->source_id)
            {
                /* The Secondary is already ready - send an unmute with a small delay.
                   Also un-mute locally at the same timestamp. */
                rtime_t sync_time = SystemClockGetTimerTime();
                sync_time = rtime_add(sync_time, (LeBroadcastManager_ConfigSyncStartDelayMs() * US_PER_MS));

                leBroadcastManager_SyncUnmuteAudio(sync_time);
                LeBroadcastManager_SyncSendUnmuteInd(broadcast_source->source_id, sync_time);
            }
            else
            {
                /* Secondary is not synced yet - wait for the ready to start ind from it. */
                leBroadcastManager_SyncStartUnMuteTimeout();
            }
        }
        else
        {
            /* Send an indication to the Primary that this earbud
               is ready to un-mute the broadcast audio. */
            LeBroadcastManager_SyncSendReadyToStart(broadcast_source->source_id);
            leBroadcastManager_SyncStartUnMuteTimeout();
        }
    }
    else
    {
        /* Always start un-muted if peer is not connected */
        start_muted = FALSE;
    }

    leBroadcastManager_SetStartMuted(start_muted);
}

void LeBroadcastManager_SyncHandleInternalUnmuteTimeout(void)
{
    DEBUG_LOG("leBroadcastManager_SourceHandleInternalUnmuteTimeout");

    rtime_t now = SystemClockGetTimerTime();
    leBroadcastManager_SyncUnmuteAudio(now);
}

void LeBroadcastManager_SyncSendCommandInd(le_broadcast_sync_command_t command)
{
    le_broadcast_manager_sync_command_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
    msg->command = command;

    DEBUG_LOG("LeBroadcastManager_SyncSendCommandInd  enum:le_broadcast_sync_command_t:%d", command);

    appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                              PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                              MARSHAL_TYPE(le_broadcast_manager_sync_command_ind_t));
    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     (msg), MARSHAL_TYPE(le_broadcast_manager_sync_command_ind_t));
}

void LeBroadcastManager_SyncSendPause(uint8 source_id)
{
    if (MirrorProfile_IsRolePrimary())
    {
        le_broadcast_manager_sync_pause_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
        msg->source_id = source_id;
        
        DEBUG_LOG("LeBroadcastManager_SyncSendPause source_id %u", source_id);

        appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                                  PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                                  MARSHAL_TYPE(le_broadcast_manager_sync_pause_ind_t));
        appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                         PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                         (msg), MARSHAL_TYPE(le_broadcast_manager_sync_pause_ind_t));
    }
}

void LeBroadcastManager_SyncSendResume(uint8 source_id)
{
    if (MirrorProfile_IsRolePrimary())
    {
        le_broadcast_manager_sync_resume_ind_t* msg = PanicUnlessMalloc(sizeof(*msg));
        msg->source_id = source_id;
        
        DEBUG_LOG("LeBroadcastManager_SyncSendResume source_id %u", source_id);

        appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                                  PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                                  MARSHAL_TYPE(le_broadcast_manager_sync_resume_ind_t));
        appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                         PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                         (msg), MARSHAL_TYPE(le_broadcast_manager_sync_resume_ind_t));
    }
}

void LeBroadcastManager_SyncSendSyncToSource(uint8 source_id)
{
    if (MirrorProfile_IsRolePrimary())
    {
        le_broadcast_manager_sync_to_source_t* msg = PanicUnlessMalloc(sizeof(*msg));
        msg->source_id = source_id;
        
        DEBUG_LOG("LeBroadcastManager_SyncSendSyncToSource source_id %u", source_id);

        appPeerSigMarshalledMsgChannelTxCancelAll(leBroadcastManager_SyncGetTask(),
                                                  PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                                  MARSHAL_TYPE(le_broadcast_manager_sync_to_source_t));
        appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                         PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                         (msg), MARSHAL_TYPE(le_broadcast_manager_sync_to_source_t));
    }
}

static uint8 leBroadcastManager_SyncCalculateSubgroupsLength(uint8 num_subgroups, le_bm_source_subgroup_t *subgroups)
{
    uint8 subgroups_length = 0;

    for (int i=0; i<num_subgroups; i++)
    {
        subgroups_length += 5; /* Space for bis_sync (4 octets) and metadata_length (1 octet) */
        subgroups_length += subgroups[i].metadata_length;
    }

    return subgroups_length;
}

static void LeBroadcastManager_SyncPopulateSubgroupsInSyncMsg(uint16 num_subgroups, uint8 **subgroups_sync, le_bm_source_subgroup_t *subGroupsData)
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

void LeBroadcastManager_SyncAddSource(scan_delegator_client_add_broadcast_source_t * source)
{
    uint8 subgroups_length = leBroadcastManager_SyncCalculateSubgroupsLength(source->num_subgroups, source->subgroups);
    size_t size = offsetof(le_broadcast_manager_sync_add_broadcast_source_t, subgroups) + subgroups_length;
    uint8 *ptr = NULL;
    le_broadcast_manager_sync_add_broadcast_source_t *sync = PanicUnlessMalloc(size);

    memmove(sync, source, offsetof(scan_delegator_client_add_broadcast_source_t, subgroups));
    sync->subgroups_length = subgroups_length;
    ptr = sync->subgroups;

    LeBroadcastManager_SyncPopulateSubgroupsInSyncMsg(sync->num_subgroups, &ptr, source->subgroups);

    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     sync,
                                     MARSHAL_TYPE(le_broadcast_manager_sync_add_broadcast_source_t));
}

void LeBroadcastManager_SyncModifySource(scan_delegator_client_modify_broadcast_source_t * source)
{
    uint8 subgroups_length = leBroadcastManager_SyncCalculateSubgroupsLength(source->num_subgroups, source->subgroups);
    size_t size = sizeof (le_broadcast_manager_sync_modify_broadcast_source_t) - 1 + subgroups_length;
    uint8 *ptr = NULL;
    le_broadcast_manager_sync_modify_broadcast_source_t *sync = PanicUnlessMalloc(size);

    sync->source_id = source->source_id;
    sync->pa_sync = (uint8) source->pa_sync;
    sync->pa_interval = source->pa_interval;
    sync->num_subgroups = source->num_subgroups;
    sync->subgroups_length = subgroups_length;

    ptr = sync->subgroups;

    LeBroadcastManager_SyncPopulateSubgroupsInSyncMsg(source->num_subgroups, &ptr, source->subgroups);

    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     sync,
                                     MARSHAL_TYPE(le_broadcast_manager_sync_modify_broadcast_source_t));
}

void LeBroadcastManager_SyncRemoveSource(uint8 source_id)
{
    le_broadcast_manager_sync_remove_broadcast_source_t *sync = PanicUnlessNew(le_broadcast_manager_sync_remove_broadcast_source_t);

    sync->source_id = source_id;

    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     sync,
                                     MARSHAL_TYPE(le_broadcast_manager_sync_remove_broadcast_source_t));
}

void LeBroadcastManager_SyncSetBroadcastCode(uint8 source_id, uint8 *broadcast_code)
{
    le_broadcast_manager_sync_set_broadcast_code_t *sync = PanicUnlessNew(le_broadcast_manager_sync_set_broadcast_code_t);

    sync->source_id = source_id;
    memcpy(sync->broadcast_code, broadcast_code, SCAN_DELEGATOR_BROADCAST_CODE_SIZE);

    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     sync,
                                     MARSHAL_TYPE(le_broadcast_manager_sync_set_broadcast_code_t));
}

void LeBroadcastManager_SyncSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr)
{
    le_broadcast_manager_sync_set_source_match_address_t *sync = PanicUnlessNew(le_broadcast_manager_sync_set_source_match_address_t);

    sync->source_id = source_id;
    sync->source_match_address = *taddr;

    appPeerSigMarshalledMsgChannelTx(leBroadcastManager_SyncGetTask(),
                                     PEER_SIG_MSG_CHANNEL_LE_AUDIO_BROADCAST_SYNC,
                                     sync,
                                     MARSHAL_TYPE(le_broadcast_manager_sync_set_source_match_address_t));
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_MIRRORING) */
