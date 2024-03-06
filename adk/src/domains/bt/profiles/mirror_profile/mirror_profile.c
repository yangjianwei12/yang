/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    mirror_profile
    \brief      Interface to mirror ACL & eSCO connections.
*/

#include <stdlib.h>

#include <vm.h>
#include <message.h>

#include "av.h"
#include "av_instance.h"
#include "bt_device.h"
#include "connection_manager.h"
#include "device.h"
#include "device_list.h"
#include "device_properties.h"
#include "hfp_profile.h"
#include "hfp_profile_instance.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include <l2cap_psm.h>
#include "peer_signalling.h"
#include "volume_messages.h"
#include "telephony_messages.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_audio.h"
#include "audio_sync.h"
#include "voice_sources.h"
#include "qualcomm_connection_manager.h"
#include "focus_audio_source.h"
#include "focus_generic_source.h"
#include "key_sync.h"
#include "volume_utils.h"
#include "system_clock.h"

#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
#include "le_audio_messages.h"
#endif

#if defined(ENABLE_LEA_CIS_DELEGATION) && (!defined(INCLUDE_LE_AUDIO_UNICAST) || !defined(INCLUDE_MIRRORING))
#error "LEA CIS delegation enabled without LEA unicast or mirroring enabled"
#endif

#ifdef INCLUDE_MIRRORING

#include "mirror_profile.h"
#include "mirror_profile_signalling.h"
#include "mirror_profile_typedef.h"
#include "mirror_profile_marshal_typedef.h"
#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "mirror_profile_sm.h"
#include "mirror_profile_audio_source.h"
#include "mirror_profile_voice_source.h"
#include "mirror_profile_peer_audio_sync_l2cap.h"
#include "mirror_profile_peer_mode_sm.h"
#include "timestamp_event.h"

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(MIRROR_PROFILE, MIRROR_PROFILE_MESSAGE_END)

#endif

#ifdef ENABLE_LEA_CIS_DELEGATION
#define mirrorProfile_IsHandsetConnected() appDeviceIsHandsetConnected()
#else
#define mirrorProfile_IsHandsetConnected() appDeviceIsBredrHandsetConnected()
#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(mirror_profile_msg_t)
LOGGING_PRESERVE_MESSAGE_TYPE(mirror_profile_internal_msg_t)

#ifdef DISABLE_KEY_SYNC
#define mirrorProfile_IsDeviceInSync(device) (TRUE) /* No need to sync AG's info if handover is not supported */
#define mirroProfile_RegisterKeySyncListner(listener) /* do nothing */
#else
#define mirrorProfile_IsDeviceInSync(device) KeySync_IsDeviceInSync(device)
#define mirroProfile_RegisterKeySyncListner(listener) KeySync_RegisterListener(listener)
#endif


mirror_profile_task_data_t mirror_profile;
/* forward declaration */
static void mirrorProfile_AudioSyncMessageHandler(Task task, MessageId id, Message message);

/*! \brief Reset mirror SCO connection state */
void MirrorProfile_ResetEscoConnectionState(void)
{
    uint8 volume = 0;
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForDevice(MirrorProfile_GetMirroredDevice());
    if (instance != NULL)
    {
        volume = appHfpGetVolume(instance);
    }

    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    sp->esco.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    sp->esco.codec_mode = hfp_codec_mode_none;
    sp->esco.wesco = 0;
    sp->esco.volume = volume;
}

/* \brief Set the local SCO audio volume */
void MirrorProfile_SetScoVolume(voice_source_t source, uint8 volume)
{
    mirror_profile_esco_t *esco = MirrorProfile_GetScoState();

    MIRROR_LOG("mirrorProfile_SetLocalVolume enum:voice_source_t:%d vol %u old_vol %u", source, volume, esco->volume);

    assert(!MirrorProfile_IsPrimary());

    if (volume != esco->volume)
    {
        esco->volume = volume;
        Volume_SendVoiceSourceVolumeUpdateRequest(source, event_origin_peer, volume);
    }
}

/*\! brief Set the local SCO codec params */
void MirrorProfile_SetScoCodec(hfp_codec_mode_t codec_mode)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("MirrorProfile_SetScoCodec codec_mode 0x%x", codec_mode);

    /* \todo Store the params as hfp params? That may actually be the best way w.r.t.
             handover as well? */
    sp->esco.codec_mode = codec_mode;
}

/*
    External notification helpers
*/

void MirrorProfile_SendAclConnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_CONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_CONNECT_IND, ind);
}

void MirrorProfile_SendAclDisconnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_DISCONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    /* \todo propagate disconnect reason */
    ind->reason = hci_error_unspecified;
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_DISCONNECT_IND, ind);
}

void MirrorProfile_SendScoConnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_ESCO_CONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_ESCO_CONNECT_IND, ind);
}

void MirrorProfile_SendScoDisconnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_ESCO_DISCONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    /* \todo propagate disconnect reason */
    ind->reason = hci_error_unspecified;
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_ESCO_DISCONNECT_IND, ind);
}

void MirrorProfile_SendA2dpStreamActiveInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_MessageSendId(sp->client_tasks, MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND);
}

void MirrorProfile_SendA2dpStreamInactiveInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_MessageSendId(sp->client_tasks, MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND);
}

audio_source_t MirrorProfile_GetTargetA2dpSource(void)
{
    mirror_profile_state_t target_state = MirrorProfile_GetTargetState();
    
    if(MirrorProfile_IsSubStateA2dpConnected(target_state))
    {
        return DeviceProperties_GetAudioSource(MirrorProfile_GetTargetDevice());
    }
    else if(target_state == MIRROR_PROFILE_STATE_SWITCH)
    {
        audio_source_t source = DeviceProperties_GetAudioSource(MirrorProfile_GetTargetDevice());
        
        if(Focus_GetFocusForAudioSource(source) == focus_foreground)
        {
            return source;
        }
    }
    
    return audio_source_none;
}

audio_source_t MirrorProfile_GetMirroredA2dpSource(void)
{
    if(MirrorProfile_IsSubStateA2dpConnected(MirrorProfile_GetState()))
    {
        return DeviceProperties_GetAudioSource(MirrorProfile_GetMirroredDevice());
    }
    
    return audio_source_none;
}

static void mirrorProfile_RespondToPendingPrepareRequest(audio_source_t source)
{
    if(!MirrorProfile_IsAudioSyncL2capConnected())
    {
        /* No earbud connection, respond to any pending prepare requests that no sync is required */
        mirrorProfile_SendAudioSyncPrepareRes(source, audio_sync_not_required);
    }
    else
    {
        if(source == MirrorProfile_GetMirroredA2dpSource())
        {
            /* If this source is being mirrored, send success response */
            mirrorProfile_SendAudioSyncPrepareRes(source, audio_sync_success);
        }
        else if(source != MirrorProfile_GetTargetA2dpSource())
        {
            /* If this is not the target reject (even if currently being mirrored) */
            mirrorProfile_SendAudioSyncPrepareRes(source, audio_sync_rejected);
        }
    }
}

static void mirrorProfile_RespondToPendingPrepareRequests(void)
{
    mirrorProfile_RespondToPendingPrepareRequest(audio_source_a2dp_1);
    mirrorProfile_RespondToPendingPrepareRequest(audio_source_a2dp_2);
}

bool MirrorProfile_TwmLinkRequirementsAchieved(device_t handset)
{
    tp_bdaddr handset_addr;

    if (BtDevice_GetTpBdaddrForDevice(handset, &handset_addr))
    {
        return (VmGetAclRole(&handset_addr) == HCI_SLAVE);
    }

    return FALSE;
}

static bool mirrorProfile_UpdateTargetDevice(void)
{
    device_t target_device = NULL;
    mirror_profile_transport_type_t target_transport = mirror_profile_transport_bredr;
    generic_source_t focused_source = Focus_GetFocusedGenericSourceForAudioRouting();
    mirror_profile_target_device_t target;

    if (GenericSource_IsVoice(focused_source))
    {
        voice_source_t source;
        if (focused_source.u.voice == voice_source_hfp_1 || focused_source.u.voice == voice_source_hfp_2)
        {
           source = focused_source.u.voice;
           target_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_voice_source, &source, sizeof(source));
           target_device = MirrorProfile_TwmLinkRequirementsAchieved(target_device) ? target_device : NULL;
        }
#ifdef ENABLE_LEA_CIS_DELEGATION
        else if(focused_source.u.voice == voice_source_le_audio_unicast_1)
        {
            LeUnicastManager_GetLeAudioDevice(&target_device);
            target_transport = mirror_profile_transport_ble;
        }
#endif
    }
    else if (GenericSource_IsAudio(focused_source))
    {
        audio_source_t source;
        if (focused_source.u.audio == audio_source_a2dp_1 || focused_source.u.audio == audio_source_a2dp_2)
        {
           source = focused_source.u.audio;
           target_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_audio_source, &source, sizeof(source));
           target_device = MirrorProfile_TwmLinkRequirementsAchieved(target_device) ? target_device : NULL;
        }
        else if(focused_source.u.audio == audio_source_le_audio_broadcast)
        {
            target_device = BtDevice_GetMruDevice();
        }
#ifdef ENABLE_LEA_CIS_DELEGATION
        else if(focused_source.u.audio == audio_source_le_audio_unicast_1)
        {
            LeUnicastManager_GetLeAudioDevice(&target_device);
            target_transport = mirror_profile_transport_ble;
        }
#endif
    }

    if (target_device == NULL)
    {
        focused_source.type = source_type_invalid, focused_source.u.voice = voice_source_none;

        device_t current_target_device = MirrorProfile_GetTargetDevice();

        if(current_target_device != NULL && BtDevice_DeviceIsValid(current_target_device) &&
           BtDevice_IsDeviceConnectedOverBredr(current_target_device))
        {
            /* Continue mirroring the current device */
            target_device = current_target_device;
        }
        else
        {
            /* Fall back to the first connected handset that meets our link requirements */
            device_t *connected_handsets = NULL;
            unsigned count = BtDevice_GetConnectedBredrHandsets(&connected_handsets);

            for (unsigned idx = 0; idx < count; idx++)
            {
                if (MirrorProfile_TwmLinkRequirementsAchieved(connected_handsets[idx]))
                {
                    target_device = connected_handsets[idx];
                    break;
                }
            }

            free(connected_handsets);
        }
    }

    if(focused_source.type == source_type_audio)
    {
        MIRROR_LOG("mirrorProfile_UpdateTargetDevice focused_src=(enum:source_type_t:%d, enum:audio_source_t:%d) target_device=0x%p target_transport: enum:mirror_profile_transport_type_t:%d",
                   focused_source.type, focused_source.u.audio, target_device, target_transport);
    }
    else
    {
        MIRROR_LOG("mirrorProfile_UpdateTargetDevice focused_src=(enum:source_type_t:%d, enum:voice_source_t:%d) target_device=0x%p target_transport: enum:mirror_profile_transport_type_t:%d",
                   focused_source.type, focused_source.u.audio, target_device, target_transport);
    }

    target.device = target_device;
    target.transport = target_transport;
    MirrorProfile_SetTargetDeviceWithTransport(target);
    return (target_device != NULL);
}

bool MirrorProfile_IsHandsetSwitchRequired(void)
{
    device_t target_device = MirrorProfile_GetTargetDevice();
    mirror_profile_transport_type_t target_transport = MirrorProfile_GetTargetTransport();
    device_t mirrored_device = MirrorProfile_GetMirroredDevice();
    MIRROR_LOG("MirrorProfile_IsHandsetSwitchRequired target_device=0x%p target_transport=enum:mirror_profile_transport_type_t:%d mirrored_device=0x%p", target_device, target_transport, mirrored_device);

    return (MirrorProfile_IsAclConnected() &&
            (target_device != mirrored_device) &&
            (target_transport != mirror_profile_transport_ble));
}

/*
    Message handling functions
*/

/*! \brief Inspect profile and internal state and decide the target state. */
void MirrorProfile_SetTargetStateFromProfileState(void)
{
    mirror_profile_state_t target = MIRROR_PROFILE_STATE_DISCONNECTED;

    if (MirrorProfile_IsPrimary())
    {
        if (appPeerSigIsConnected() &&
            MirrorProfile_IsAudioSyncL2capConnected() &&
            mirrorProfile_IsHandsetConnected() &&
            MirrorProfile_IsQhsReady() &&
            mirrorProfile_UpdateTargetDevice() &&
            mirrorProfile_IsDeviceInSync(MirrorProfile_GetTargetDevice()))
        {
            if (MirrorProfile_IsHandsetSwitchRequired())
            {
/* This requires new support in BTSS and appsP0 */
#ifdef MIRROR_PROFILE_ACL_SWITCH
                target = MIRROR_PROFILE_STATE_SWITCH;
#endif
            }
            else
            {
                bool is_br_edr_connected = TRUE;

                if (MirrorProfile_IsAclConnected())
                {
                    device_t device = MirrorProfile_GetMirroredDevice();
                    voice_source_t voice_source = MirrorProfile_GetVoiceSource();
                    hfpInstanceTaskData *instance = HfpProfileInstance_GetInstanceForDevice(device);
                    /* SCO has higher priority than A2DP */
                    if (instance && HfpProfile_IsScoActiveForInstance(instance) &&
                        MirrorProfile_IsEscoMirroringEnabled() && MirrorProfile_IsVoiceSourceSupported(voice_source))
                    {
                        target = MIRROR_PROFILE_STATE_ESCO_CONNECTED;
                    }
                }

#ifdef ENABLE_LEA_CIS_DELEGATION
                if (target == MIRROR_PROFILE_STATE_DISCONNECTED)
                {
                    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
                    device_t lea_device;

                    /*  Initiate delegation under following conditions:
                     *  1) Peer handle is valid (OR)
                     *  2) Delegation type is "Mirror" and own handle is valid
                     */
                    if ((lea_unicast->peer_cis_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID ||
                         (lea_unicast->audio_config.mirror_type == le_um_cis_mirror_type_mirror && 
                          lea_unicast->own_cis_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)) &&
                        !MirrorProfile_IsAnyCisInDisconnectingState(lea_unicast) &&
                        LeUnicastManager_GetLeAudioDevice(&lea_device) &&
                        ((lea_device == MirrorProfile_GetTargetDevice()) || (MirrorProfile_GetTargetDevice() == NULL)))
                    {
                        target = MIRROR_PROFILE_STATE_CIS_CONNECTED;
                    }
                    else if (MirrorProfile_GetTargetDevice() != NULL)
                    {
                        bdaddr bd_addr = DeviceProperties_GetBdAddr(MirrorProfile_GetTargetDevice());

                        /* determine if BR/EDR link exist or not */
                        is_br_edr_connected = ConManagerIsConnected(&bd_addr);
                    }
                    else
                    {
                        is_br_edr_connected = FALSE;
                    }
                }
#endif

                if (target == MIRROR_PROFILE_STATE_DISCONNECTED && is_br_edr_connected)
                {
                    target = MIRROR_PROFILE_STATE_ACL_CONNECTED;
                    if (MirrorProfile_IsAclConnected() && MirrorProfile_IsA2dpMirroringEnabled())
                    {
                        if (mirrorProfile_GetMirroredAudioSyncState() == AUDIO_SYNC_STATE_READY)
                        {
                            target = MIRROR_PROFILE_STATE_A2DP_CONNECTED;
                        }
                        else if (mirrorProfile_GetMirroredAudioSyncState() == AUDIO_SYNC_STATE_ACTIVE)
                        {
                            target = MIRROR_PROFILE_STATE_A2DP_ROUTED;
                        }
                    }
                }
            }
        }

        MirrorProfile_SetTargetState(target);
        mirrorProfile_RespondToPendingPrepareRequests();
    }
}

/*!  \brief Handle an APP_HFP_CONNECTED_IND.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpConnectedInd(const APP_HFP_CONNECTED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpConnectedInd state 0x%x handset %u",
                MirrorProfile_GetState(), appDeviceIsHandset(&ind->bd_addr));

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle APP_HFP_DISCONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpDisconnectedInd(const APP_HFP_DISCONNECTED_IND_T *ind)
{
    UNUSED(ind);
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_A2DP_CONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAvA2dpConnectedInd(const AV_A2DP_CONNECTED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleAvA2dpConnectedInd state 0x%x", MirrorProfile_GetState());
    if (MirrorProfile_IsPrimary())
    {
        mirrorProfile_RegisterAudioSync(ind->av_instance);
    }

    /* Target state is updated on AUDIO_SYNC_STATE_IND */
}

/*! \brief Handle APP_HFP_VOLUME_IND

    Only Primary should receive this, because the Handset HFP must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpVolumeInd(const APP_HFP_VOLUME_IND_T *ind)
{
    if (MirrorProfile_IsPrimary())
    {
        MirrorProfile_GetScoState()->volume = ind->volume;

        MIRROR_LOG("mirrorProfile_HandleAppHfpVolumeInd volume %u", ind->volume);

        MirrorProfile_SendHfpVolumeToSecondary(ind->source, ind->volume);
    }
}

/*! \brief Handle TELEPHONY_INCOMING_CALL

    Happens when a call is incoming, but before the SCO channel has been
    created.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleTelephonyIncomingCall(void)
{
    /* Save time later by starting DSP now */
    appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_ASYNC);
}

/*! \brief Handle TELEPHONY_OUTGOING_CALL and TELEPHONY_CALL_ONGOING messages

    These messages are sent when a call state transition occurs such that we either
    have an outgoing ringing call, or an ongoing active call , but before the SCO channel
    has been created.

    For example the HFP SLC may have been connected with the call already active and
    ongoing on the handset, which may then be audio transferred to the device from the
    handset.

    In such a scenario, prepare to mirror the handset SCO as quickly as possible.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleTelephonyOutgoingAndOngoingCall(void)
{
    if (1 == BtDevice_GetNumberOfHandsetsConnectedOverBredr())
    {
        /* Prepare to mirror the SCO by exiting sniff on the peer link.
        This speeds up connecting the SCO mirror. The link is put back to sniff
        once the SCO mirror is connected or if the eSCO fails to connect. */
        mirrorProfilePeerMode_ActiveModePeriod(mirrorProfileConfig_PrepareForEscoMirrorActiveModeTimeout());
    }
    /* Save time later by starting DSP now */
    appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_ASYNC);
}

/*! \brief Handle TELEPHONY_CALL_ENDED
*/
static void mirrorProfile_HandleTelephonyCallEnded(void)
{
}

/*! \brief Handle APP_HFP_SCO_CONNECTING_SYNC_IND */
static void mirrorProfile_HandleAppHfpScoConnectingSyncInd(const APP_HFP_SCO_CONNECTING_SYNC_IND_T *ind)
{
    bool immediate_response = FALSE;
    if (MirrorProfile_IsAudioSyncL2capConnected() && MirrorProfile_IsEscoMirroringEnabled())
    {
        if (MirrorProfile_GetMirroredDevice() == ind->device)
        {
            /* Already mirroring this device so accept immediately */
            immediate_response = TRUE;
        }
        else
        {
            Task task = MirrorProfile_GetTask();
            uint16 *lock = MirrorProfile_GetScoSyncLockAddr();
            uint32 timeout = mirrorProfileConfig_ScoConnectingSyncTimeout();
            /* Mirroring another device. The mirror profile will switch to mirror
               the ACL of this device and then clear the ScoSync lock. Clearing
               this lock will cause the conditional message below to be delivered
               which calls back to HFP profile to accept the SCO connection.
               If something goes wrong during this process, the _TIMEOUT message
               will be delivered and the SCO will be accepted regardless of the
               whether the mirroring is prepared for the SCO connection. */
            MESSAGE_MAKE(msg, MIRROR_PROFILE_INTERNAL_SCO_SYNC_RSP_T);
            msg->device = ind->device;
            msg->link_type = ind->link_type;
            MessageSendConditionally(task, MIRROR_PROFILE_INTERNAL_SCO_SYNC_RSP, msg, lock);
            MessageSendLater(task, MIRROR_PROFILE_INTERNAL_SCO_SYNC_TIMEOUT, NULL, timeout);
            MirrorProfile_SetScoSyncLock();
            MirrorProfile_SetTargetStateFromProfileState();
        }
    }
    else
    {
        immediate_response = TRUE;
    }

    if (immediate_response)
    {
        HfpProfile_ScoConnectingSyncResponse(ind->device, MirrorProfile_GetTask(), TRUE, ind->link_type);
    }
}

/*! \brief Handle APP_HFP_SCO_CONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpScoConnectedInd(void)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpScoConnectedInd");

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle APP_HFP_SCO_DISCONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpScoDisconnectedInd(void)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpScoDisconnectedInd");
    /* When SCO disconnects we want to change the target state, but we don't
       need to initiate a disconnection since we expect the SCO mirror to be
       disconnected automatically by the firmware. Therefore, set the delay kick
       flag to stop the SM from initiating the disconnect immediately. A
       disconnect indication will arrive from the firmware during the delay. */
       MirrorProfile_SetDelayKick();
       MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle PEER_SIG_CONNECTION_IND

    Both Primary and Secondary will receive this when the peer signalling
    channel is connected and disconnected.
*/
static void mirrorProfile_HandlePeerSignallingConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    UNUSED(ind);
    MirrorProfile_SetTargetStateFromProfileState();
    if (ind->status != peerSigStatusConnected)
    {
        MirrorProfile_ClearStreamChangeLock();
    }
}

/*! \brief Handle AV_AVRCP_CONNECTED_IND */
static void mirrorProfile_HandleAvAvrcpConnectedInd(void)
{
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_AVRCP_DISCONNECTED_IND */
static void mirrorProfile_HandleAvAvrcpDisconnectedInd(void)
{
    MirrorProfile_SetTargetStateFromProfileState();
}

static void mirrorProfile_SendAudioSyncConnectRes(const AUDIO_SYNC_CONNECT_IND_T *ind)
{
    MESSAGE_MAKE(rsp, AUDIO_SYNC_CONNECT_RES_T);
    rsp->sync_id = ind->sync_id;
    MessageCancelAll(ind->task, AUDIO_SYNC_CONNECT_RES);
    MessageSend(ind->task, AUDIO_SYNC_CONNECT_RES, rsp);
}

/*! \brief Handle AUDIO_SYNC_CONNECT_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncConnectInd(const AUDIO_SYNC_CONNECT_IND_T *ind)
{
    MIRROR_LOG("MirrorProfile_HandleAudioSyncConnectInd for enum:audio_source_t:%d", ind->source_id);

    mirrorProfile_SetAudioSyncState(ind->source_id, AUDIO_SYNC_STATE_CONNECTED);

    /* L2CAP cid is not valid yet. Store parameters and wait to send context 
       on receipt of AUDIO_SYNC_STATE_IND with state AUDIO_SYNC_STATE_CONNECTED */
    (void)MirrorProfile_StoreAudioSourceParameters(ind->source_id);
    MirrorProfile_SetTargetStateFromProfileState();
    mirrorProfile_SendAudioSyncConnectRes(ind);
}

/*! \brief Handle AUDIO_SYNC_PREPARE_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncPrepareInd(const AUDIO_SYNC_PREPARE_IND_T *ind)
{
    MIRROR_LOG("MirrorProfile_HandleAudioSyncPrepareInd enum:audio_source_t:%d", ind->source_id);
    
    /* Send mirrored source context to secondary */
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        /* The context is sent to the secondary with the state set to
        AUDIO_SYNC_STATE_CONNECTED, not AUDIO_SYNC_STATE_READY.
        This ensures that the secondary reports the correct
        correct mirror_profile_a2dp_start_mode_t. */
        mirrorProfile_SetAudioSyncState(ind->source_id, AUDIO_SYNC_STATE_CONNECTED);
        MirrorProfile_SendA2dpStreamContextToSecondary(ind->source_id);
    }
    
    mirrorProfile_StoreAudioSyncPrepareState(ind->source_id, ind->task, ind->sync_id);
    mirrorProfile_SetAudioSyncState(ind->source_id, AUDIO_SYNC_STATE_READY);
    
    /* Prepare response is sent once target state has been decided */
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AUDIO_SYNC_ACTIVATE_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncActivateInd(const AUDIO_SYNC_ACTIVATE_IND_T *ind)
{
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncActivateInd");
    }
    else
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncActivateInd invalid audio source parameters");
    }
    
    mirrorProfile_StoreAudioSyncActivateState(ind->source_id, ind->task, ind->sync_id);
    mirrorProfile_SetAudioSyncState(ind->source_id, AUDIO_SYNC_STATE_ACTIVE);
    MirrorProfile_SetTargetStateFromProfileState();
    mirrorProfile_SendAudioSyncActivateRes(ind->source_id);
}

/*! \brief Handle AUDIO_SYNC_STATE_IND_T
    \param ind The message.

    The only state of interest here is disconnected, since other states are
    indicated in other sync messages.
*/
static void MirrorProfile_HandleAudioSyncStateInd(const AUDIO_SYNC_STATE_IND_T *ind)
{
    bool context_sent = FALSE;
    audio_sync_state_t prev_state = mirrorProfile_GetAudioSyncState(ind->source_id);
    
    MIRROR_LOG("MirrorProfile_HandleAudioSyncStateInd enum:audio_source_t:%d enum:audio_sync_state_t:%d", ind->source_id, ind->state);

    mirrorProfile_SetAudioSyncState(ind->source_id, ind->state);

    switch (ind->state)
    {
        case AUDIO_SYNC_STATE_DISCONNECTED:
            mirrorProfile_StoreAudioSyncPrepareState(ind->source_id, NULL, 0);
            mirrorProfile_StoreAudioSyncActivateState(ind->source_id, NULL, 0);
        break;
        
        case AUDIO_SYNC_STATE_CONNECTED:
            if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
            {
                if(prev_state == AUDIO_SYNC_STATE_DISCONNECTED || prev_state == AUDIO_SYNC_STATE_CONNECTED)
                {
                    /* If entering this state when media channel is first opened this will be the first time
                       we have a valid cid to send to the secondary. Block until secondary has responded to
                       the context update to ensure it has a valid cid<->context mapping before we set up
                       the mirror L2CAP connection. */
                    MirrorProfile_SendA2dpStreamContextToSecondaryBlockUntilResponse(ind->source_id);
                    context_sent = TRUE;
                }
            }
        break;
        
        case AUDIO_SYNC_STATE_ACTIVE:
            MirrorProfile_StoreAudioSourceParameters(ind->source_id);
        break;
        
        case AUDIO_SYNC_STATE_READY:
        break;
    }
    
    if(!context_sent)
    {
        MirrorProfile_SendA2dpStreamContextToSecondary(ind->source_id);
    }

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AUDIO_SYNC_CODEC_RECONFIGURED_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncReconfiguredInd(const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *ind)
{
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        MirrorProfile_SendA2dpStreamContextToSecondary(ind->source_id);
    }
    else
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncReconfiguredInd invalid audio source parameters");
    }
}

static void MirrorProfile_HandleAudioSyncDestroyInd(const AUDIO_SYNC_DESTROY_IND_T* ind)
{
    MESSAGE_MAKE(rsp, AUDIO_SYNC_DESTROY_RES_T);
    
    MIRROR_LOG("MirrorProfile_HandleAudioSyncDestroyInd enum:audio_source_t:%d", ind->source_id);
    
    /* Clear all record of reply task for prepare/activate indications to ensure it isn't used again */
    mirrorProfile_StoreAudioSyncPrepareState(ind->source_id, NULL, 0);
    mirrorProfile_StoreAudioSyncActivateState(ind->source_id, NULL, 0);
    
    rsp->sync_id = ind->sync_id;
    MessageSend(ind->task, AUDIO_SYNC_DESTROY_RES, rsp);
}

/*! \brief Handle QHS link establishing between buds or QHS start timeout */
static void MirrorProfile_HandleQhsReadyOrFailed(void)
{
    MirrorProfile_SetQhsReady();
    MirrorProfile_SetTargetStateFromProfileState();
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT);
}

static void MirrorProfile_HandleQhsConnectedInd(const QCOM_CON_MANAGER_QHS_CONNECTED_T * message)
{
    if(appDeviceIsPeer(&message->bd_addr))
    {
        MirrorProfile_HandleQhsReadyOrFailed();
    }
}

static void MirrorProfile_HandleScoSyncRsp(const MIRROR_PROFILE_INTERNAL_SCO_SYNC_RSP_T *msg)
{
    DEBUG_LOG("MirrorProfile_HandleScoSyncRsp");
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_PROFILE_INTERNAL_SCO_SYNC_TIMEOUT);
    /* Accept SCO connection */
    HfpProfile_ScoConnectingSyncResponse(msg->device, MirrorProfile_GetTask(), TRUE, msg->link_type);
}

static void MirrorProfile_HandleScoSyncTimeout(void)
{
    DEBUG_LOG("MirrorProfile_HandleScoSyncTimeout");
    MirrorProfile_ClearScoSyncLock();
}

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! \brief Handle trigger for LE audio CIS delegate/mirror request */
static void mirrorProfile_HandleLeAudioUnicastCisConnected(const LE_AUDIO_UNICAST_CIS_CONNECTED_T *message)
{
    hci_connection_handle_t mirror_cis_handle = LE_INVALID_CIS_HANDLE;

#ifdef ENABLE_LEA_CIS_DELEGATION
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    hci_connection_handle_t cis_handle = message->cis_handle;

    MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisConnected for (cis: %d, side: %d, handle: 0x%0x), is_our_side: %d, time: %d",
               message->cis_id, message->side, cis_handle, Multidevice_IsSameAsOurSide(message->side), SystemClockGetTimerTime());

    if (MirrorProfile_IsAnyCisInDisconnectingState(lea_unicast))
    {
        LE_AUDIO_UNICAST_CIS_CONNECTED_T * const internal_msg =
            (LE_AUDIO_UNICAST_CIS_CONNECTED_T *) PanicUnlessMalloc(sizeof(*internal_msg));

        /* We need wait for disconnection to complete */
        MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisConnected mdm disconnect not completed, process later");

        internal_msg->side = message->side;
        internal_msg->cis_id = message->cis_id;
        internal_msg->cis_handle = cis_handle;
        MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_STORED_LE_AUDIO_UNICAST_CIS_CONNECTED, internal_msg);
        return;
    }

    if (message->side == multidevice_side_both)
    {
        /* In case of Mirroring single CIS belonging to both Buds it is always own CIS */
        MirrorProfile_SetOwnCisState(MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT);
        lea_unicast->own_cis_handle = cis_handle;
        lea_unicast->own_cis_dir = message->cis_dir;
        lea_unicast->own_cis_type = Multidevice_IsLeft() ? mirror_profile_cis_type_both_render_left : mirror_profile_cis_type_both_render_right;
    }
    else if (Multidevice_IsSameAsOurSide(message->side))
    {
        MirrorProfile_SetOwnCisState(MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT);
        lea_unicast->own_cis_handle = cis_handle;
        lea_unicast->own_cis_dir = message->cis_dir;
    }
    else
    {
        if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_IDLE &&
            lea_unicast->peer_cis_handle == MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)
        {
            MirrorProfile_SetPeerCisState(MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT);
        }
        lea_unicast->peer_cis_handle = cis_handle;
        lea_unicast->peer_cis_dir = message->cis_dir;
    }

    /* Prepare to mirror the CIS by exiting sniff on the peer link.this speeds up connecting the CIS mirror.*/
    mirrorProfilePeerMode_ActiveModePeriod(mirrorProfileConfig_PrepareForCisMirrorActiveModeTimeout());

    /* First always try for peer CIS */
    if (lea_unicast->peer_cis_state != MIRROR_PROFILE_CIS_SUB_STATE_IDLE ||
        lea_unicast->audio_config.mirror_type == le_um_cis_mirror_type_mirror)
    {
        MirrorProfile_SetTargetStateFromProfileState();

        if (MirrorProfile_GetTargetState() != MIRROR_PROFILE_STATE_CIS_CONNECTED &&
            MirrorProfile_GetTargetState() != MIRROR_PROFILE_STATE_SWITCH &&
            lea_unicast->own_cis_state != MIRROR_PROFILE_CIS_SUB_STATE_IDLE &&
            !MirrorProfile_GetLock())
        {
            /* Could not start CIS delegation and both CIS handles are valid, then immediately indicate failure */
            mirror_cis_handle = lea_unicast->peer_cis_handle != LE_INVALID_CIS_HANDLE ? lea_unicast->peer_cis_handle
                                                                                      : lea_unicast->own_cis_handle;
        }
    }
#else /* ENABLE_LEA_CIS_DELEGATION */
    if (Multidevice_IsSameAsPairSide(message->side))
    {
        mirror_cis_handle = message->cis_handle;
    }
#endif /* ENABLE_LEA_CIS_DELEGATION */

    if (mirror_cis_handle != LE_INVALID_CIS_HANDLE)
    {
        MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisConnected LEA mirroring not possible");
        LeUnicastManager_CisMirrorStatus(cis_handle, FALSE);
    }
}

#ifdef ENABLE_LEA_CIS_DELEGATION
/*! \brief Determines CIS next state on CIS disconnect from unicast manager.

    \param state Current state

    \return Next CIS state
*/
static mirror_profile_cis_sub_state_t mirrorProfile_GetCisNextStateOnDisconnect(mirror_profile_cis_sub_state_t state)
{
    mirror_profile_cis_sub_state_t next_state;

    switch (state)
    {
        case MIRROR_PROFILE_CIS_SUB_STATE_IDLE:
            next_state = MIRROR_PROFILE_CIS_SUB_STATE_IDLE;
            break;

        case MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT:
            next_state = MIRROR_PROFILE_CIS_SUB_STATE_IDLE;
            break;

        case MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING:
            next_state = MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECT_ON_CONNECTING;
            break;

        case MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED:
            next_state = MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECTING;
            break;

        default:
            MIRROR_LOG("mirrorProfile_GetCisNextStateOnDisconnect bad CIS state enum:mirror_profile_cis_sub_state_t:%d", state);
            assert(0);
            next_state = state;
            break;
    }

    return next_state;
}
#endif

/*! \brief Handle trigger for LE audio CIS disconnected indication */
static void mirrorProfile_HandleLeAudioUnicastCisDisconnected(const LE_AUDIO_UNICAST_CIS_DISCONNECTED_T *message)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisDisconnected for (cis: %d, side: %d, handle: 0x%0x), is_our_side: %d",
               message->cis_id, message->side, message->cis_handle, Multidevice_IsSameAsOurSide(message->side));

    if (MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_STORED_LE_AUDIO_UNICAST_CIS_CONNECTED))
    {
        MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisDisconnected connect message not processed");
        return;
    }

    MIRROR_LOG("mirrorProfile_HandleLeAudioUnicastCisDisconnected own-handle: 0x%0x, peer-handle: 0x%0x",
               lea_unicast->own_cis_handle, lea_unicast->peer_cis_handle);

    MirrorProfile_SendUnicastConfigDataClear(message->side);
    if (Multidevice_IsSameAsOurSide(message->side))
    {
        MirrorProfile_SetOwnCisState(mirrorProfile_GetCisNextStateOnDisconnect(lea_unicast->own_cis_state));
    }

    if (Multidevice_IsSameAsPairSide(message->side))
    {
        MirrorProfile_SetPeerCisState(mirrorProfile_GetCisNextStateOnDisconnect(lea_unicast->peer_cis_state));
    }

    MirrorProfile_SetTargetStateFromProfileState();

#else /* ENABLE_LEA_CIS_DELEGATION */
    UNUSED(message);
#endif /* ENABLE_LEA_CIS_DELEGATION */
}

static void mirrorProfile_HandleLeAudioUnicastMediaDataPathReady(void)
{
    MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_KICK_TARGET_STATE, NULL);
}

void MirrorProfile_StartLeUnicastAudio(void)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    if ((lea_unicast->audio_config.codec_version > 0) && (lea_unicast->audio_config.sink_codec_id == KYMERA_LE_AUDIO_CODEC_LC3))
    {
        if (lea_unicast->own_cis_handle != LE_INVALID_CIS_HANDLE)
        {
            QcomConManagerSetWBMFeature(lea_unicast->own_cis_handle, TRUE);
        }

        if (lea_unicast->peer_cis_handle != LE_INVALID_CIS_HANDLE)
        {
            QcomConManagerSetWBMFeature(lea_unicast->peer_cis_handle, TRUE);
        }
    }

    if (lea_unicast->reconfig_pending)
    {
        /* This CIS is part of reconfiguration of dynamic ASE enable/disable */
        if (MirrorProfile_GetLeAudioContext() == AUDIO_CONTEXT_TYPE_COVERSATIONAL)
        {
            MirrorProfile_ReconfigureLeVoiceGraph();
        }
        else
        {
            MirrorProfile_ReconfigureLeaAudioGraph();
        }
        lea_unicast->reconfig_pending = FALSE;
    }
    else
    {
        if (lea_unicast->audio_config.voice_source != voice_source_none)
        {
            lea_unicast->routed_source = source_type_voice;

            MirrorProfile_StartLeVoice();
        }
        else if (lea_unicast->audio_config.audio_source != audio_source_none)
        {
            lea_unicast->routed_source = source_type_audio;

            MirrorProfile_StartLeAudio();
        }
        else
        {
            Panic();
        }
    }
#endif /*ENABLE_LEA_CIS_DELEGATION */
}

void MirrorProfile_StopLeUnicastAudio(void)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    if (lea_unicast->routed_source == source_type_voice)
    {
        MirrorProfile_StopLeVoice();
    }
    else if (lea_unicast->routed_source == source_type_audio)
    {
        MirrorProfile_StopLeAudio();
    }

    lea_unicast->routed_source = source_type_invalid;
#endif /*ENABLE_LEA_CIS_DELEGATION */
}

uint16 MirrorProfile_GetLeAudioUnicastContext(void)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
    return (!MirrorProfile_IsPrimary() &&
            MirrorProfile_GetLeUnicastConfigRcvd() ? MirrorProfile_GetLeAudioContext() : 0);
#else
    return 0;
#endif
}

void MirrorProfile_GetCisHandle(hci_connection_handle_t *own_handle,
                                hci_connection_handle_t *peer_handle)
{
    mirror_profile_lea_unicast_t *cis_ctx = MirrorProfile_GetLeaUnicastState();

    if (own_handle != NULL)
    {
        *own_handle = cis_ctx->own_cis_handle;
    }

    if (peer_handle != NULL)
    {
        *peer_handle = cis_ctx->peer_cis_handle;
    }
}

uint16 MirrorProfile_GetIsoHandleFromMirrorType(uint8 iso_direction)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    bool use_own_handle = TRUE;
    multidevice_side_t side = Multidevice_GetSide();

    switch (lea_unicast->audio_config.mirror_type)
    {
        case le_um_cis_mirror_type_mirror:
            /* Peer OR Own CIS Handle must be valid. Return whichever is valid */
            if (lea_unicast->peer_cis_handle != LE_AUDIO_INVALID_ISO_HANDLE)
            {
                use_own_handle = FALSE;
            }
            break;

        case le_um_cis_mirror_type_delegate_with_left_src_shared:
            if (iso_direction == LE_AUDIO_ISO_DIRECTION_UL && side != multidevice_side_left)
            {
                use_own_handle = FALSE;
            }
            break;

        case le_um_cis_mirror_type_delegate_with_right_src_shared:
            if (iso_direction == LE_AUDIO_ISO_DIRECTION_UL && side != multidevice_side_right)
            {
                use_own_handle = FALSE;
            }
            break;

        case le_um_cis_mirror_type_delegate_with_left_snk_shared:
            if (iso_direction == LE_AUDIO_ISO_DIRECTION_DL && side != multidevice_side_left)
            {
                use_own_handle = FALSE;
            }
            break;

        case le_um_cis_mirror_type_delegate_with_right_snk_shared:
            if (iso_direction == LE_AUDIO_ISO_DIRECTION_DL && side != multidevice_side_right)
            {
                use_own_handle = FALSE;
            }
            break;

        case le_um_cis_mirror_type_delegate:
        default:
            break;
    }

    return use_own_handle ? lea_unicast->own_cis_handle : lea_unicast->peer_cis_handle;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#ifdef USE_SYNERGY
static void mirrorProfile_HandleCmPrim(Message message)
{
    MIRROR_LOG("mirrorProfile_HandleCmPrim CM Prim ");

    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    if (*prim == CSR_BT_CM_SDS_REGISTER_CFM)
    {
        MirrorProfile_HandleCmSdsRegisterCfm((const CsrBtCmSdsRegisterCfm *) message);
    }
    else if (CsrBtUtilSdcVerifyCmMsg(prim))
    {
        mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

        CsrBtUtilSdcCmMsgHandler(mirror_inst,
                                 mirror_inst->sdp_search_data,
                                 prim);
    }
    else
    {
        switch (*prim)
        {
            case CSR_BT_CM_L2CA_REGISTER_CFM:
                MirrorProfile_HandleCmL2caRegisterCfm((const CsrBtCmL2caRegisterCfm *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND:
                MirrorProfile_HandleCmL2caConnectAcceptInd((const CsrBtCmL2caConnectAcceptInd *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM:
                MirrorProfile_HandleCmL2caConnectAcceptCfm((const CsrBtCmL2caConnectAcceptCfm *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_CFM:
                MirrorProfile_HandleCmL2caConnectCfm((const CsrBtCmL2caConnectCfm *) message);
                break;

            case CSR_BT_CM_L2CA_DISCONNECT_IND:
                MirrorProfile_HandleCmL2caDisconnectInd((const CsrBtCmL2caDisconnectInd *) message);
                break;

            case CSR_BT_CM_MODE_CHANGE_IND:
            {
                CsrBtCmModeChangeInd *ind = (CsrBtCmModeChangeInd*)message;
                if (ind->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    mirrorProfilePeerMode_HandleDmModeChangeEvent((bdaddr*)&ind->deviceAddr, ind->mode);
                }
                break;
            }
            case CSR_BT_CM_SWITCH_ROLE_CFM:
            {
                CsrBtCmSwitchRoleCfm * cfm = (CsrBtCmSwitchRoleCfm *)message;
                mirrorProfilePeerMode_HandleDmRoleCfm((bdaddr*)&cfm->deviceAddr, cfm->role, cfm->resultCode);
                break;
            }

            case CSR_BT_CM_ROLE_CHANGE_IND:
            {
                CsrBtCmRoleChangeInd *ind = (CsrBtCmRoleChangeInd*)message;
                mirrorProfilePeerMode_HandleDmRoleInd((bdaddr*)&ind->deviceAddr, ind->role, ind->resultCode);
                break;
            }

            case CSR_BT_CM_SNIFF_SUB_RATING_IND:
                MirrorProfilePeerMode_HandleCmSniffSubRatingInd((const CsrBtCmSniffSubRatingInd *)message);
                break;

#if defined(ENABLE_LEA_CIS_DELEGATION) && defined(USE_SYNERGY)
            case CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM:
                MirrorProfile_HandleSetupIsoDataPathCfm((const CmIsocSetupIsoDataPathCfm *) message);
                break;

            case CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
                MirrorProfile_HandleRemoveIsoDataPathCfm((const CmIsocRemoveIsoDataPathCfm *) message);
                break;
#endif

            default:
                MIRROR_LOG("mirrorProfile_HandleCmPrim Unhandled CM Prim 0x%04x", *prim);
                break;
        }
    }
    CmFreeUpstreamMessageContents((void *) message);
}
#endif

static void mirrorProfile_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    /* Notifications from other bt domain modules */
    case CON_MANAGER_TP_DISCONNECT_IND:
        mirrorProfile_HandleTpConManagerDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
        break;

    case CON_MANAGER_TP_CONNECT_IND:
        mirrorProfile_HandleTpConManagerConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *)message);
        break;

    case APP_HFP_CONNECTED_IND:
        mirrorProfile_HandleAppHfpConnectedInd((const APP_HFP_CONNECTED_IND_T *)message);
        break;

    case APP_HFP_DISCONNECTED_IND:
        mirrorProfile_HandleAppHfpDisconnectedInd((const APP_HFP_DISCONNECTED_IND_T *)message);
        break;

    case APP_HFP_SCO_INCOMING_RING_IND:
        /* \todo Use this as a trigger to send a ring command to Secondary */
        break;

    case APP_HFP_SCO_INCOMING_ENDED_IND:
        /* \todo Use this as a trigger to send a stop ring command to Secondary */
        break;

    case APP_HFP_VOLUME_IND:
        mirrorProfile_HandleAppHfpVolumeInd((const APP_HFP_VOLUME_IND_T *)message);
        break;

    case APP_HFP_SCO_CONNECTING_SYNC_IND:
        mirrorProfile_HandleAppHfpScoConnectingSyncInd(message);
        break;

    case APP_HFP_SCO_CONNECTED_IND:
        mirrorProfile_HandleAppHfpScoConnectedInd();
        break;

    case APP_HFP_SCO_DISCONNECTED_IND:
        mirrorProfile_HandleAppHfpScoDisconnectedInd();
        break;

    case AV_A2DP_CONNECTED_IND:
        mirrorProfile_HandleAvA2dpConnectedInd((const AV_A2DP_CONNECTED_IND_T *)message);
        break;

    case AV_AVRCP_CONNECTED_IND:
        mirrorProfile_HandleAvAvrcpConnectedInd();
        break;

    case AV_AVRCP_DISCONNECTED_IND:
        mirrorProfile_HandleAvAvrcpDisconnectedInd();
        break;

    case TELEPHONY_INCOMING_CALL:
        mirrorProfile_HandleTelephonyIncomingCall();
        break;

    case TELEPHONY_OUTGOING_CALL:
    case TELEPHONY_CALL_ONGOING:
        mirrorProfile_HandleTelephonyOutgoingAndOngoingCall();
        break;

    case TELEPHONY_CALL_ENDED:
        mirrorProfile_HandleTelephonyCallEnded();
        break;

    /* Internal mirror_profile messages */
    case MIRROR_INTERNAL_DELAYED_KICK:
        MirrorProfile_SmKick();
        break;

    case MIRROR_INTERNAL_SET_TARGET_STATE:
        MirrorProfile_SetTargetState(((const MIRROR_INTERNAL_SET_TARGET_STATE_T *)message)->target_state);
        break;

    case MIRROR_INTERNAL_KICK_TARGET_STATE:
        MirrorProfile_SetTargetStateFromProfileState();
        break;

    case MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT:
        MirrorProfile_PeerLinkPolicyHandleIdleTimeout();
        break;
#ifdef USE_SYNERGY
    case MIRROR_INTERNAL_CLOSE_SDP:
        MirrorProfile_CloseSdp();
        break;
#endif
    case MIRROR_PROFILE_INTERNAL_SCO_SYNC_RSP:
        MirrorProfile_HandleScoSyncRsp(message);
        break;

    case MIRROR_PROFILE_INTERNAL_SCO_SYNC_TIMEOUT:
        MirrorProfile_HandleScoSyncTimeout();
        break;

    /* MDM prims from firmware */
    case MESSAGE_BLUESTACK_MDM_PRIM:
        MirrorProfile_HandleMessageBluestackMdmPrim((const MDM_UPRIM_T *)message);
        break;

    /* Peer Signalling messages */
    case PEER_SIG_CONNECTION_IND:
        mirrorProfile_HandlePeerSignallingConnectionInd((const PEER_SIG_CONNECTION_IND_T *)message);
        break;

    case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
        MirrorProfile_HandlePeerSignallingMessage((const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *)message);
        break;

    case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
        MirrorProfile_HandlePeerSignallingMessageTxConfirm((const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *)message);
        break;

#ifdef USE_SYNERGY
    case CM_PRIM:
        mirrorProfile_HandleCmPrim(message);
        break;
#else
    /* Connection library messages */
    case CL_L2CAP_REGISTER_CFM:
        MirrorProfile_HandleClL2capRegisterCfm((const CL_L2CAP_REGISTER_CFM_T *)message);
        break;

    case CL_SDP_REGISTER_CFM:
        MirrorProfile_HandleClSdpRegisterCfm((const CL_SDP_REGISTER_CFM_T *)message);
        break;

    case CL_L2CAP_CONNECT_IND:
        MirrorProfile_HandleL2capConnectInd((const CL_L2CAP_CONNECT_IND_T *)message);
        break;

    case CL_L2CAP_CONNECT_CFM:
        MirrorProfile_HandleL2capConnectCfm((const CL_L2CAP_CONNECT_CFM_T *)message);
        break;

    case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
        MirrorProfile_HandleClSdpServiceSearchAttributeCfm((const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
        break;

    case CL_L2CAP_DISCONNECT_IND:
        MirrorProfile_HandleL2capDisconnectInd((const CL_L2CAP_DISCONNECT_IND_T *)message);
        break;

    case CL_L2CAP_DISCONNECT_CFM:
        MirrorProfile_HandleL2capDisconnectCfm((const CL_L2CAP_DISCONNECT_CFM_T *)message);
        break;
#endif
    case QCOM_CON_MANAGER_QHS_CONNECTED:
        MirrorProfile_HandleQhsConnectedInd((const QCOM_CON_MANAGER_QHS_CONNECTED_T *) message);
        break;

    case MIRROR_INTERNAL_QHS_START_TIMEOUT:
         /* QHS link didn't establish */
        MirrorProfile_HandleQhsReadyOrFailed();
        break;

    case MIRROR_INTERNAL_PEER_ENTER_SNIFF:
        mirrorProfile_HandlePeerEnterSniff();
        break;

    case KEY_SYNC_DEVICE_COMPLETE_IND:
#ifdef INCLUDE_LE_AUDIO_BROADCAST
    case LE_AUDIO_BROADCAST_CONNECTED:
    case LE_AUDIO_BROADCAST_DISCONNECTED:
#endif
        MirrorProfile_SetTargetStateFromProfileState();
        break;

#ifdef INCLUDE_LE_AUDIO_UNICAST
    case LE_AUDIO_UNICAST_CIS_CONNECTED:
    case MIRROR_INTERNAL_STORED_LE_AUDIO_UNICAST_CIS_CONNECTED:
        mirrorProfile_HandleLeAudioUnicastCisConnected((const LE_AUDIO_UNICAST_CIS_CONNECTED_T *) message);
        break;

    case LE_AUDIO_UNICAST_CIS_DISCONNECTED:
        mirrorProfile_HandleLeAudioUnicastCisDisconnected((const LE_AUDIO_UNICAST_CIS_DISCONNECTED_T *) message);
        break;

    case LE_AUDIO_UNICAST_ENABLED:
        MirrorProfile_SendUnicastConfigData(((const LE_AUDIO_UNICAST_ENABLED_T *) message)->side);
        break;

    case LE_AUDIO_UNICAST_MEDIA_DATA_PATH_READY:
        mirrorProfile_HandleLeAudioUnicastMediaDataPathReady();
        break;

    case LE_AUDIO_UNICAST_VOICE_CONNECTED:
    case LE_AUDIO_UNICAST_VOICE_DISCONNECTED:
        /* Message ignored */
        break;
#endif

    default:
        MIRROR_LOG("mirrorProfile_MessageHandler: Unhandled id MESSAGE:mirror_profile_internal_msg_t:0x%x", id);
        break;
    }
}

static void mirrorProfile_AudioSyncMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    /* Audio sync messages */
    case AUDIO_SYNC_CONNECT_IND:
        MirrorProfile_HandleAudioSyncConnectInd((const AUDIO_SYNC_CONNECT_IND_T *)message);
        break;

    case AUDIO_SYNC_PREPARE_IND:
        MirrorProfile_HandleAudioSyncPrepareInd((const AUDIO_SYNC_PREPARE_IND_T *)message);
        break;

    case AUDIO_SYNC_ACTIVATE_IND:
        MirrorProfile_HandleAudioSyncActivateInd((const AUDIO_SYNC_ACTIVATE_IND_T *)message);
        break;

    case AUDIO_SYNC_STATE_IND:
        MirrorProfile_HandleAudioSyncStateInd((const AUDIO_SYNC_STATE_IND_T *)message);
        break;

    case AUDIO_SYNC_CODEC_RECONFIGURED_IND:
        MirrorProfile_HandleAudioSyncReconfiguredInd((const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *)message);
        break;
    
    case AUDIO_SYNC_DESTROY_IND:
        MirrorProfile_HandleAudioSyncDestroyInd((const AUDIO_SYNC_DESTROY_IND_T*)message);
        break;

    default:
        break;
    }
}

/*! \brief Send an audio_sync_msg_t internally.

    The audio_sync_msg_t messages do not need to be sent conditionally as the
    handling of the message can only modify the target state.
*/
static void mirrorProfile_SyncSendAudioSyncMessage(audio_sync_t *sync_inst, MessageId id, Message message)
{
    Task task = &sync_inst->task;
    PanicFalse(MessageCancelAll(task, id) <= 1);
    MessageSendConditionally(task, id, message, &MirrorProfile_GetLock());
}

bool MirrorProfile_Init(Task task)
{
    memset(&mirror_profile, 0, sizeof(mirror_profile));
    mirror_profile.task_data.handler = mirrorProfile_MessageHandler;
    mirror_profile.state = MIRROR_PROFILE_STATE_DISCONNECTED;
    mirror_profile.target_state = MIRROR_PROFILE_STATE_DISCONNECTED;
    mirror_profile.acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    mirror_profile.esco.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    mirror_profile.esco.volume = 0; //appHfpGetVolume();
    mirror_profile.init_task = task;
    mirror_profile.client_tasks = TaskList_Create();
    mirror_profile.audio_sync.local_psm = 0;
    mirror_profile.audio_sync.remote_psm = 0;
    mirror_profile.audio_sync.sdp_search_attempts = 0;
    mirror_profile.audio_sync.l2cap_state = MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE;
    mirror_profile.enable_esco_mirroring = TRUE;
    mirror_profile.enable_a2dp_mirroring = TRUE;
#ifdef ENABLE_LEA_CIS_DELEGATION
    mirror_profile.lea_unicast.own_cis_handle = mirror_profile.lea_unicast.peer_cis_handle =
        LE_INVALID_CIS_HANDLE;
    mirror_profile.lea_unicast.audio_config.audio_source = audio_source_none;
    mirror_profile.lea_unicast.audio_config.voice_source = voice_source_none;
    mirror_profile.lea_unicast.routed_source = source_type_invalid;
#endif

    /* Register a Protocol/Service Multiplexor (PSM) that will be
       used for this application. The same PSM is used at both
       ends. */
    ConnectionL2capRegisterRequest(MirrorProfile_GetTask(), L2CAP_PSM_MIRROR_PROFILE, 0);

#ifdef USE_SYNERGY
    /* Subscribe for encryption change notifications */
    CmSetEventMaskReqSend(MirrorProfile_GetTask(),
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE |
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
#endif
    /* Register for notifications when devices and/or profiles connect
       or disconnect. */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, MirrorProfile_GetTask());
    HfpProfile_RegisterStatusClient(MirrorProfile_GetTask());
    appAvStatusClientRegister(MirrorProfile_GetTask());
    Telephony_RegisterForMessages(MirrorProfile_GetTask());
    QcomConManagerRegisterClient(MirrorProfile_GetTask());

    /* Register a channel for peer signalling */
    appPeerSigMarshalledMsgChannelTaskRegister(MirrorProfile_GetTask(),
                                            PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE,
                                            mirror_profile_marshal_type_descriptors,
                                            NUMBER_OF_MIRROR_PROFILE_MARSHAL_TYPES);

    /* Register for peer signaling notifications */
    appPeerSigClientRegister(MirrorProfile_GetTask());

    HfpProfile_SetScoConnectingSyncTask(MirrorProfile_GetTask());
    mirroProfile_RegisterKeySyncListner(MirrorProfile_GetTask());
#if (defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST))
    LeAudioMessages_ClientRegister(MirrorProfile_GetTask());
#endif

    /* Now wait for MDM_REGISTER_CFM */
    return TRUE;
}

#ifdef ENABLE_LEA_CIS_DELEGATION
/*! \brief Register the required LE unicast sources interfaces */
static void mirrorProfile_RegisterLeUnicastInterfaces(void)
{
    /* Register LE Unicast audio source & control interfaces */
    AudioSources_RegisterAudioInterface(audio_source_le_audio_unicast_1, MirrorProfile_GetLeAudioInterface());
    AudioSources_RegisterMediaControlInterface(audio_source_le_audio_unicast_1, MirrorProfile_GetLeMediaControlInterface());


    /* Register LE Unicast voice source & control interfaces */
    VoiceSources_RegisterAudioInterface(voice_source_le_audio_unicast_1, MirrorProfile_GetLeVoiceInterface());
    VoiceSources_RegisterTelephonyControlInterface(voice_source_le_audio_unicast_1, MirrorProfile_GetLeTelephonyControlInterface());
}

bdaddr* MirrorProfile_GetCisMirroredDeviceAddress(void)
{
    mirror_profile_cis_sub_state_t own_cis_state = MirrorProfile_GetLeaUnicastState()->own_cis_state;

    if (own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED)
    {
        return MirrorProfile_GetLeUnicastMirrorAddr();
    }

    return NULL;
}

#endif

/* \brief Inform mirror profile of current device Primary/Secondary role.

    todo : A Primary <-> Secondary role switch should only be allowed
    when the state machine is in a stable state. This will be more important
    when the handover logic is implemented.
*/
void MirrorProfile_SetRole(bool primary)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

#ifdef INCLUDE_LE_AUDIO_UNICAST
    LeUnicastManager_RoleChangeInd(primary);
#endif

    if (!primary)
    {
        /* Take ownership of the A2DP source (mirror) when becoming secondary */
        AudioSources_RegisterAudioInterface(audio_source_a2dp_1, MirrorProfile_GetAudioInterface());
        AudioSources_RegisterAudioInterface(audio_source_a2dp_2, MirrorProfile_GetAudioInterface());

        AudioSources_RegisterMediaControlInterface(audio_source_a2dp_1, MirrorProfile_GetMediaControlInterface());
        AudioSources_RegisterMediaControlInterface(audio_source_a2dp_2, MirrorProfile_GetMediaControlInterface());

        /* Register voice source interface */
        VoiceSources_RegisterAudioInterface(voice_source_hfp_1, MirrorProfile_GetVoiceInterface());
        VoiceSources_RegisterAudioInterface(voice_source_hfp_2, MirrorProfile_GetVoiceInterface());

        VoiceSources_RegisterTelephonyControlInterface(voice_source_hfp_1, MirrorProfile_GetTelephonyControlInterface());
        VoiceSources_RegisterTelephonyControlInterface(voice_source_hfp_2, MirrorProfile_GetTelephonyControlInterface());

#ifdef ENABLE_LEA_CIS_DELEGATION
        mirrorProfile_RegisterLeUnicastInterfaces();
#endif

        /* Clear delayed kicks when becoming secondary. This avoids the state
           machine being kicked in the secondary role resulting in panic */
        MessageCancelAll(MirrorProfile_GetTask(), MIRROR_INTERNAL_DELAYED_KICK);
    }
    else
    {
        /* in-case of primary we could have a scenario where phone is already connected and A2DP profile is connected
           then mirror profile shall have missed to register for AV Sync handler, maybe PFR happened post phone connection. 
           Need to make up for that miss in-case the sync isn't registered yet */
           avInstanceTaskData *theInst;
           av_instance_iterator_t iter;

            /* Register Mirror profile interface with each connected av instance for a2dp sync*/
            for_all_av_instances(theInst, &iter)
            {
                MIRROR_LOG("MirrorProfile_SetRole - Phone is already connected before Mirror Role is determined");
                audio_source_t source = Av_GetSourceForInstance(theInst);
                if (source != audio_source_none)
                {
                    mirrorProfile_SetAudioSyncState(source, appA2dpSyncGetAudioSyncState(theInst));
                }
                mirrorProfile_RegisterAudioSync(theInst);
            }
    }

    sp->is_primary = primary;
    MIRROR_LOG("MirrorProfile_SetRole primary %u", sp->is_primary);
}

/* \brief Get the SCO sink associated with the mirror eSCO link. */
Sink MirrorProfile_GetScoSink(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return StreamScoSink(sp->esco.conn_handle);
}

void MirrorProfile_Connect(Task task, const bdaddr *peer_addr)
{
    if(peer_addr)
    {
        DEBUG_LOG("MirrorProfile_Connect - startup");

        mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
        mirror_inst->is_primary = TRUE;
        MirrorProfile_CreateAudioSyncL2capChannel(task, peer_addr);
    }
    else
    {
        DEBUG_LOG("MirrorProfile_Connect - Peer address is NULL");
        Panic();
    }
}

void MirrorProfile_Disconnect(Task task)
{
    DEBUG_LOG("MirrorProfile_Disconnect");

    MirrorProfile_CloseAudioSyncL2capChannel(task);
}

void MirrorProfile_ClientRegister(Task client_task)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_AddTask(sp->client_tasks, client_task);
}

void MirrorProfile_ClientUnregister(Task client_task)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_RemoveTask(sp->client_tasks, client_task);
}

bool MirrorProfile_IsBredrMirroringConnected(void)
{
    return (MirrorProfile_IsAclConnected() || MirrorProfile_IsEscoConnected());
}

bool MirrorProfile_IsCisMirroringConnected(void)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
    return MirrorProfile_IsCisConnected();
#else
    return 0;
#endif
}

bool MirrorProfile_IsEscoActive(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return (SinkIsValid(StreamScoSink(sp->esco.conn_handle)));
}

bool MirrorProfile_IsA2dpActive(void)
{
    return MirrorProfile_IsA2dpConnected();
}

uint16 MirrorProfile_GetMirrorAclHandle(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return sp->acl.conn_handle;
}

/*
    Test only functions
*/
void MirrorProfile_Destroy(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_Destroy(sp->client_tasks);
}

mirror_profile_a2dp_start_mode_t MirrorProfile_GetA2dpStartMode(void)
{
    mirror_profile_a2dp_start_mode_t mode = MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED;
    bool sync_start;

    bool q2q = Kymera_IsQ2qModeEnabled();

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
        case MIRROR_PROFILE_STATE_A2DP_ROUTED:
            sync_start = TRUE;
            break;
        default:
            /* Also start synchronised if transitioning between handsets */
            sync_start = MirrorProfile_IsHandsetSwitchRequired();
            break;
    }

    if (MirrorProfile_IsPrimary())
    {
        if (sync_start)
        {
            /* If the mirrored instance is already streaming, the audio will
            be started in sync with the secondary by unmuting the audio
            stream at the same instant. The secondary sends a message to the
            primary defining the unmute instant. */
            avInstanceTaskData *av_inst = AvInstance_GetInstanceForDevice(MirrorProfile_GetMirroredDevice());
            if (av_inst && appA2dpIsStreaming(av_inst))
            {
                mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE_PRIMARY_UNMUTE):
                                (MIRROR_PROFILE_A2DP_START_PRIMARY_SYNC_UNMUTE);
            }
            else
            {
                mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE):
                                (MIRROR_PROFILE_A2DP_START_PRIMARY_SYNCHRONISED);
            }
        }
        else
        {
            mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE):
                            (MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED);
        }
    }
    else
    {
        audio_sync_state_t sync_state = mirrorProfile_GetMirroredAudioSyncState();

        switch (sync_state)
        {
            case AUDIO_SYNC_STATE_READY:
            case AUDIO_SYNC_STATE_CONNECTED:
            {
                if (sync_start)
                {
                    mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE):
                                    (MIRROR_PROFILE_A2DP_START_SECONDARY_SYNCHRONISED);
                }
                else
                {
                    mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE_SECONDARY_UNMUTE):
                                    (MIRROR_PROFILE_A2DP_START_SECONDARY_SYNC_UNMUTE);
                }
            }
            break;

            case AUDIO_SYNC_STATE_ACTIVE:
                mode = (q2q)?(MIRROR_PROFILE_A2DP_START_Q2Q_MODE_SECONDARY_UNMUTE):
                                (MIRROR_PROFILE_A2DP_START_SECONDARY_SYNC_UNMUTE);
            break;

            default:
                DEBUG_LOG_WARN("MirrorProfile_GetA2dpStartMode Unexpected a2dp state enum:audio_sync_state_t:%d",
                                sync_state);
            break;
        }
    }
    DEBUG_LOG("MirrorProfile_GetA2dpStartMode mirror mode enum:mirror_profile_a2dp_start_mode_t:%d",mode);
    return mode;
}

bool MirrorProfile_ShouldEscoAudioStartSynchronously(voice_source_t source)
{
    if (MirrorProfile_IsSecondary())
    {
        return TRUE;
    }
    else if (MirrorProfile_IsAclConnected() &&
             MirrorProfile_IsEscoMirroringEnabled() &&
             (MirrorProfile_GetVoiceSource() == source))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

Sink MirrorProfile_GetA2dpAudioSyncTransportSink(void)
{
    return MirrorProfile_GetAudioSyncL2capState()->link_sink;
}

Source MirrorProfile_GetA2dpAudioSyncTransportSource(void)
{
    return MirrorProfile_GetAudioSyncL2capState()->link_source;
}

/*! \brief Request mirror_profile to Enable Mirror Esco.

    This should only be called from the Primary device.
*/
void MirrorProfile_EnableMirrorEsco(void)
{
    DEBUG_LOG("MirrorProfile_EnableMirrorEsco, State(0x%x)", MirrorProfile_GetState());
    if (!mirror_profile.enable_esco_mirroring)
    {
        mirror_profile.enable_esco_mirroring = TRUE;
        MirrorProfile_SetTargetStateFromProfileState();
    }
}

/*! \brief Request mirror_profile to Disable Mirror Esco.

    This should only be called from the Primary device.
*/
void MirrorProfile_DisableMirrorEsco(void)
{
    DEBUG_LOG("MirrorProfile_DisableMirrorEsco, State(0x%x)", MirrorProfile_GetState());
    if (mirror_profile.enable_esco_mirroring)
    {
        mirror_profile.enable_esco_mirroring = FALSE;
        MirrorProfile_SetTargetStateFromProfileState();
    }
}

void MirrorProfile_EnableMirrorA2dp(void)
{
    DEBUG_LOG("MirrorProfile_EnableMirrorA2dp, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_a2dp_mirroring = TRUE;
    MirrorProfile_SetTargetStateFromProfileState();
}

void MirrorProfile_DisableMirrorA2dp(void)
{
    DEBUG_LOG("MirrorProfile_DisableMirrorA2dp, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_a2dp_mirroring = FALSE;
    MirrorProfile_SetTargetStateFromProfileState();
}

uint16 MirrorProfile_GetMirrorState(void)
{
    return MirrorProfile_GetState();
}

uint32 MirrorProfile_GetExpectedPeerLinkTransmissionTime(void)
{
    return MirrorProfilePeerLinkPolicy_GetExpectedTransmissionTime();
}

bool MirrorProfile_IsVoiceSourceSupported(voice_source_t source)
{
    source_defined_params_t source_params = {0};
    bool mirroring_supported = TRUE;

    /* The local HFP SCO should already have been connected up to the point
       where we know the type (SCO/eSCO) and eSCO connection paramters. */
    if (VoiceSources_GetConnectParameters(source, &source_params))
    {
        voice_connect_parameters_t *voice_params = (voice_connect_parameters_t *)source_params.data;
        assert(source_params.data_length == sizeof(voice_connect_parameters_t));

        /* Mirroring is not supported for:
            SCO links (tesco == 0)
            eSCO links using HV3 packets (tesco == 6) */
        if (voice_params->tesco <= 6)
        {
            mirroring_supported = FALSE;
        }

        VoiceSources_ReleaseConnectParameters(source, &source_params);
    }

    DEBUG_LOG("MirrorProfile_IsVoiceSourceSupported supported %d", mirroring_supported);
    return mirroring_supported;
}

bool MirrorProfile_IsSameMirroredDevice(const tp_bdaddr *tpaddr)
{
    bool status = FALSE;

    /* LE ACL link will never be mirrored, only CISes are delegated/mirrored for LE-Audio.
     * Hence consider only BR/EDR ACL link which are mirrored for audio/voice streaming*/
    if (tpaddr->transport == TRANSPORT_BREDR_ACL)
    {
        /* Check if it is the same BR/EDR mirrored device address */
        status = BdaddrIsSame(&MirrorProfile_Get()->acl.bd_addr, &tpaddr->taddr.addr);
    }

    return status;
}

bdaddr * MirrorProfile_GetMirroredDeviceAddress(void)
{
    return &(MirrorProfile_Get()->acl.bd_addr);
}

void mirrorProfile_SetAudioSyncState(audio_source_t source, audio_sync_state_t state)
{
    if(source == audio_source_none)
    {
        DEBUG_LOG("mirrorProfile_SetAudioSyncState audio_source_none");
    }
    else
    {
        mirror_profile_cached_context_t* context = MirrorProfile_GetCachedContext(source);
        context->state = state;
    }
}

audio_sync_state_t mirrorProfile_GetAudioSyncState(audio_source_t source)
{
    if(source == audio_source_none)
    {
        return AUDIO_SYNC_STATE_DISCONNECTED;
    }
    else
    {
        mirror_profile_cached_context_t* context = MirrorProfile_GetCachedContext(source);
        return context->state;
    }
}

audio_sync_state_t mirrorProfile_GetMirroredAudioSyncState(void)
{
    audio_source_t asource;
    if (MirrorProfile_IsPrimary())
    {
        asource = MirrorProfile_GetAudioSource();
        if (asource != audio_source_none)
        {
            focus_t focus = Focus_GetFocusForAudioSource(asource);
            if (focus != focus_foreground)
            {
                /* The A2DP audio source is not foreground so ignore it */
                asource = audio_source_none;
            }
        }
    }
    else
    {
        asource = MirrorProfile_GetA2dpState()->audio_source;
    }

    return mirrorProfile_GetAudioSyncState(asource);
}

uint8 mirrorProfile_GetMirroredAudioVolume(audio_source_t source)
{
    if(source != audio_source_none)
    {
        return AudioSources_GetVolume(source).value;
    }

    return 0;
}

unsigned mirrorProfile_audioSourceToIndex(audio_source_t source)
{
    switch (source)
    {
        case audio_source_a2dp_1:
            return 0;
        case audio_source_a2dp_2:
            return 1;
        default:
            Panic();
            return 0;
    }
}

void mirrorProfile_StoreAudioSyncPrepareState(audio_source_t source, Task task, uint16 id)
{
    unsigned index = mirrorProfile_audioSourceToIndex(source);
    MirrorProfile_GetA2dpState()->prepare_state[index].id = id;
    MirrorProfile_GetA2dpState()->prepare_state[index].task = task;
}

void mirrorProfile_StoreAudioSyncActivateState(audio_source_t source, Task task, uint16 id)
{
    unsigned index = mirrorProfile_audioSourceToIndex(source);
    MirrorProfile_GetA2dpState()->activate_state[index].id = id;
    MirrorProfile_GetA2dpState()->activate_state[index].task = task;
}

void mirrorProfile_SendAudioSyncPrepareRes(audio_source_t source, audio_sync_reason_t reason)
{
    unsigned index = mirrorProfile_audioSourceToIndex(source);
    Task task = MirrorProfile_GetA2dpState()->prepare_state[index].task;
    if (task)
    {
        MIRROR_LOG("mirrorProfile_SendAudioSyncPrepareRes enum:audio_source_t:%d : enum:audio_sync_reason_t:%d", source, reason);
        MESSAGE_MAKE(rsp, AUDIO_SYNC_PREPARE_RES_T);
        rsp->sync_id = MirrorProfile_GetA2dpState()->prepare_state[index].id;
        rsp->reason = reason;
        MessageSend(task, AUDIO_SYNC_PREPARE_RES, rsp);
        MirrorProfile_GetA2dpState()->prepare_state[index].task = NULL;
    }
}

void mirrorProfile_SendAudioSyncActivateRes(audio_source_t source)
{
    unsigned index = mirrorProfile_audioSourceToIndex(source);
    Task task = MirrorProfile_GetA2dpState()->activate_state[index].task;
    if (task)
    {
        MESSAGE_MAKE(rsp, AUDIO_SYNC_ACTIVATE_RES_T);
        rsp->sync_id = MirrorProfile_GetA2dpState()->activate_state[index].id;
        MessageSend(task, AUDIO_SYNC_ACTIVATE_RES, rsp);
        MirrorProfile_GetA2dpState()->activate_state[index].task = NULL;
    }
}

void mirrorProfile_RegisterAudioSync(avInstanceTaskData *av_inst)
{
    audio_sync_t sync = {{mirrorProfile_AudioSyncMessageHandler},
                         mirrorProfile_SyncSendAudioSyncMessage};
    appA2dpSyncRegister(av_inst, &sync);
}

bool MirrorProfile_IsRolePrimary(void)
{
    return MirrorProfile_IsPrimary();
}

audio_source_t MirrorProfile_GetAudioSource(void)
{
    return DeviceProperties_GetAudioSource(MirrorProfile_GetAclState()->device);
}

voice_source_t MirrorProfile_GetVoiceSource(void)
{
    return DeviceProperties_GetVoiceSource(MirrorProfile_GetAclState()->device);
}

mirror_profile_cached_context_t* MirrorProfile_GetCachedContext(audio_source_t audio_source)
{
    unsigned index = mirrorProfile_audioSourceToIndex(audio_source);
    return &MirrorProfile_GetA2dpState()->cached_context[index];
}

audio_source_t MirrorProfile_GetCachedAudioSourceForCid(l2ca_cid_t cid)
{
    audio_source_t source;

    for(source = audio_source_a2dp_1; source <= audio_source_a2dp_2; source++)
    {
        mirror_profile_cached_context_t* context = MirrorProfile_GetCachedContext(source);
        
        if(cid == context->cid)
        {
            return source;
        }
    }
    
    return audio_source_none;
}

#endif /* INCLUDE_MIRRORING */
