/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \ingroup    mirror_profile
    \brief      Mirror profile handover interfaces

*/
#ifdef INCLUDE_MIRRORING
#include "handover_if.h"
#include "mirror_profile_private.h"
#include "mirror_profile_volume_observer.h"
#include "av.h"
#include "a2dp_profile_sync.h"
#include "av_instance.h"
#include "hfp_profile.h"
#include "focus_generic_source.h"

#include <panic.h>
#include <logging.h>
#include <bdaddr.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*!
    \brief Handle Veto check during handover
    \return If mirror profile is not in connected(ACL/ESCO/A2DP) then veto handover.
*/
bool MirrorProfile_Veto(void)
{
    int32 due;
    /* Count the number of message pending that do not cause veto */
    uint32 no_veto_msg_count = 0;
    /* List of messages that should not cause handover veto */
    MessageId no_veto_msgs[] = {MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT,
                                MIRROR_INTERNAL_DELAYED_KICK,
                                MIRROR_INTERNAL_PEER_ENTER_SNIFF};
    MessageId *mid;

    if (!MirrorProfile_IsSteadyState(MirrorProfile_GetState()) ||
        !MirrorProfile_IsSteadyState(MirrorProfile_GetSwitchState()) ||
#if !defined(ENABLE_LE_HANDOVER)
        (MirrorProfile_GetState() == MIRROR_PROFILE_STATE_DISCONNECTED) ||
#endif
        !mirrorProfilePeerMode_IsInSteadyState())
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto, vetoing the handover state 0x%x", MirrorProfile_GetState());
        return TRUE;
    }

#if defined(ENABLE_LE_HANDOVER) && defined(INCLUDE_LE_AUDIO_UNICAST)
    /* In primary, if mirror profile is in disconnected state it should veto if there exists a BREDR connection
       with handset or if it is preparing for delegation */
    if (MirrorProfile_IsPrimary() &&
        MirrorProfile_GetState() == MIRROR_PROFILE_STATE_DISCONNECTED &&
        (MirrorProfile_IsAnyCisInReadyToConnectState(MirrorProfile_GetLeaUnicastState()) ||
         appDeviceIsBredrHandsetConnected()))
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto: Delegation in progress OR connected with BREDR handset, but no mirroring ");
        return TRUE;
    }

    /* In Secondary, Veto if any pending data paths or peer MDM CIS channel(non-delegated) to be established as part LE-ACL CIS delegation*/
    if (MirrorProfile_IsSecondary() &&
        MirrorProfile_IsCisDataPathEstPending())
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto, secondary vetoing due to data path setup pending");
        return TRUE;
    }
#endif /* ENABLE_LE_HANDOVER && INCLUDE_LE_AUDIO_UNICAST */

    if (mirrorProfile_GetMirroredAudioSyncState() == AUDIO_SYNC_STATE_READY)
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto, pending audio source to connect");
        return TRUE;
    }
    else if (MirrorProfile_Get()->stream_change_lock)
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto, stream_change_lock set");
        return TRUE;
    }

    ARRAY_FOREACH(mid, no_veto_msgs)
    {
        if (MessagePendingFirst(MirrorProfile_GetTask(), *mid, NULL))
        {
            no_veto_msg_count++;
        }
    }
    if (no_veto_msg_count != MessagesPendingForTask(MirrorProfile_GetTask(), &due))
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto, vetoing the handover, message due in %dms", due);
        return TRUE;
    }

    /* Veto any handover if the hfp voice source is routed but it cannot be
       mirrored. If it cannot be mirrored then the mirror eSCO will not be
       connected */
    if (HfpProfile_IsScoActive() && !MirrorProfile_IsEscoConnected())
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto voice source active but not mirrored");
        return TRUE;
    }

    /* Veto any handover if the focused device is a2dp audio but it cannot be mirrored. 
       If it cannot be mirrored then the mirror a2dp will not be connected */
    generic_source_t routed_source = Focus_GetFocusedGenericSourceForAudioRouting();
    if ( GenericSource_IsAudio(routed_source) && AudioSource_IsA2dp(routed_source.u.audio) && 
         !MirrorProfile_IsA2dpConnected())
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto a2dp active but not mirrored");
        return TRUE;
    }

    /* If we have a mirrored A2DP Source that somehow isn't routed on either bud,
     * that can only be because it's about to be routed or torn down.
     * So, wait until that happens. */
    if (MirrorProfile_IsPrimary() && MirrorProfile_IsA2dpConnected() && !AudioSources_IsAudioRouted(MirrorProfile_GetMirroredA2dpSource()))
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto primary A2DP mirrored, but not routed");
        return TRUE;
    }
 
    if (!MirrorProfile_IsPrimary() && MirrorProfile_IsA2dpConnected() && !AudioSources_IsAudioRouted(MirrorProfile_GetA2dpState()->audio_source))
    {
        DEBUG_LOG_INFO("MirrorProfile_Veto secondary A2DP mirrored, but not routed");
        return TRUE;
    }	

    return FALSE;
}

static bool MirrorProfile_Marshal(const tp_bdaddr *tp_bd_addr,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(written);
    /* nothing to be done */
    return TRUE;
}

static bool MirrorProfile_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(consumed);
    /* nothing to be done */
    return TRUE;
}

void MirrorProfile_SetRoleAndSwapPeerAddress(bool is_primary)
{
    MirrorProfile_Get()->is_primary = is_primary;

    /* Set the correct peer address(if not already done) based on new role */
    if (is_primary && !appDeviceIsSecondary(&MirrorProfile_GetAudioSyncL2capState()->peer_addr))
    {
        appDeviceGetSecondaryBdAddr(&MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    }
    else if(!is_primary && !appDeviceIsPrimary(&MirrorProfile_GetAudioSyncL2capState()->peer_addr))
    {
        appDeviceGetPrimaryBdAddr(&MirrorProfile_GetAudioSyncL2capState()->peer_addr);
    }
}

static void MirrorProfile_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
#ifdef USE_SYNERGY
    if (is_primary)
    {
        if (BdaddrIsSame(MirrorProfile_GetMirroredDeviceAddress(), &tp_bd_addr->taddr.addr))
        {
            Sink sco_sink = MirrorProfile_GetScoSink();
            if (sco_sink)
            {
                DEBUG_LOG("MirrorProfile_HandoverCommit: Override SCO sink");
                CsrBtDeviceAddr deviceAddr;
                BdaddrConvertVmToBluestack(&deviceAddr, &tp_bd_addr->taddr.addr);
                HfUpdateScoHandleWithAddrReq(deviceAddr, SinkGetScoHandle(sco_sink));
            }
            else
            {
                DEBUG_LOG("MirrorProfile_HandoverCommit : No SCO Sink");
            }
        }
        else
        {
            DEBUG_LOG("MirrorProfile_HandoverCommit: No Mirrored Device");
        }
    }
#else
    UNUSED(tp_bd_addr);
    UNUSED(is_primary);
#endif
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void MirrorProfile_HandoverComplete(bool is_primary)
{
    MirrorProfile_SetRole(is_primary);
    mirror_profile_a2dp_t *a2dp = MirrorProfile_GetA2dpState();
#if defined(INCLUDE_LE_AUDIO_UNICAST)
    source_type_t lea_routed_source = source_type_invalid;
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
#endif

    if (is_primary)
    {
        avInstanceTaskData *theInst;
        av_instance_iterator_t iter;
        
        mirrorProfile_SetAudioSyncState(audio_source_a2dp_1, AUDIO_SYNC_STATE_DISCONNECTED);
        mirrorProfile_SetAudioSyncState(audio_source_a2dp_2, AUDIO_SYNC_STATE_DISCONNECTED);

        /* Register Mirror profile interface with each connected av instance for a2dp sync*/
        for_all_av_instances(theInst, &iter)
        {
            if (theInst)
            {
                audio_source_t source = Av_GetSourceForInstance(theInst);
                if (source != audio_source_none)
                {
                    mirrorProfile_SetAudioSyncState(source, appA2dpSyncGetAudioSyncState(theInst));
                }
                mirrorProfile_RegisterAudioSync(theInst);
            }
        }
        
        mirrorProfile_RegisterForMirroredSourceVolume();

#if defined(ENABLE_LE_HANDOVER)
        /* If the LE handover happened without a mirrored Link OR during a unicast streaming(with mirrored Link), the 
           mirror profile target state has to be updated correctly in the new primary, to prevent mirror profile 
           initiating incorrect state transitions later.
        */
        if (   MirrorProfile_GetState() == MIRROR_PROFILE_STATE_DISCONNECTED
#if defined(INCLUDE_LE_AUDIO_UNICAST)
            || MirrorProfile_GetState() == MIRROR_PROFILE_STATE_CIS_CONNECTED
#endif
            )
        {
            MirrorProfile_Get()->target_state = MirrorProfile_GetState();
        }
#endif
        /* The new primary kicks the state machine, in case a pending SM kick
           was cancelled on the old primary */
        MirrorProfile_SetDelayKick();
        MirrorProfile_SmKick();
    }
    else
    {
#if defined(INCLUDE_LE_AUDIO_UNICAST)
        if (lea_unicast->audio_config.voice_source != voice_source_none)
        {
            lea_routed_source = source_type_voice;
        }
        else if (lea_unicast->audio_config.audio_source != audio_source_none)
        {
            lea_routed_source = source_type_audio;
        }
#endif

        mirrorProfile_UnregisterForMirroredSourceVolume();
        /* The new secondary ignores any pending SM kicks, it is the new primaries
        responsibility to kick the SM.  */
        MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_DELAYED_KICK);
    }

#if defined(INCLUDE_LE_AUDIO_UNICAST)
    lea_unicast->routed_source = lea_routed_source;
#endif

    if (a2dp->cid != L2CA_CID_INVALID)
    {
        /* Refresh the handover policy on the new stream post-handover */
        Source media_source = StreamL2capSource(a2dp->cid);
        if (media_source)
        {
            SourceConfigure(media_source, STREAM_SOURCE_HANDOVER_POLICY, 0x1);
        }
    }

    /* Since handover completes by putting peer link into sniff mode, it is safe
       to cancel any pending enter sniff messages and set peer mode state to
       sniff. */
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_PEER_ENTER_SNIFF);
    MirrorProfilePeerMode_SetTargetStateVar(MIRROR_PROFILE_PEER_MODE_STATE_SNIFF);
}

static void MirrorProfile_HandoverAbort(void)
{
    return;
}

const handover_interface mirror_handover_if =
        MAKE_HANDOVER_IF(&MirrorProfile_Veto,
                         &MirrorProfile_Marshal,
                         &MirrorProfile_Unmarshal,
                         &MirrorProfile_HandoverCommit,
                         &MirrorProfile_HandoverComplete,
                         &MirrorProfile_HandoverAbort);

#endif /* INCLUDE_MIRRORING */
