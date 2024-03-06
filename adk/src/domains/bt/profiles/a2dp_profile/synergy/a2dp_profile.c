/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_profile.c
\brief      A2DP State Machine
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include "a2dp_profile.h"

#include "av.h"
#include "av_config.h"
#include <av_instance.h>
#include "a2dp_profile_audio.h"
#include "a2dp_profile_caps.h"
#include "a2dp_profile_config.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_protected.h"
#include "audio_sources.h"
#include "audio_sources_list.h"
#include "audio_sync.h"
#include <device.h>
#include <device_list.h>
#include <device_properties.h>
#include "kymera.h"
#include "kymera_adaptation.h"
#include "kymera_config.h"
#include "kymera_latency_manager.h"
#include "latency_config.h"
#include "power_manager.h"
#include <profile_manager.h>
#include "ui.h"
#include "timestamp_event.h"

#include <connection_manager.h>

#include <a2dp.h>
#include <avrcp.h>
#include <panic.h>
#include <connection.h>
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <ps.h>
#include <string.h>
#include <stdlib.h>
#include <stream.h>
#include <av_lib.h>
#include <avrcp_lib.h>
#include "csr_sbc_api.h"
#include "link_policy.h"
#include "a2dp_profile_codec_handler.h"
#include "a2dp_profile_data_block.h"
#include "a2dp_profile_caps_parse.h"
#include "a2dp_profile_caps_aptx_adaptive.h"
#include "a2dp_profile_codec_aptx_adaptive.h"

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)


/*! Macro for simplifying creating messages */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

#include "adk_log.h"

#define A2dpProfile_IsSourceConnected(av_instance) \
    ((A2dpProfile_GetSourceState(av_instance) == source_state_connected) || \
    (A2dpProfile_GetSourceState(av_instance) == source_state_connecting))

static bool pts_mode_enabled = FALSE;

/* Local Function Prototypes */
static void appA2dpAbortStreamConnect(avInstanceTaskData *theInst);
static bool appA2dpCapabilitiesDataValidated(avInstanceTaskData *theInst,
                                             const CsrBtAvReconfigureInd *ind, uint8 *errorSvcCat,
                                             uint8 *errorCode, uint8 sepIndex);
static void appA2dpStreamReset(avInstanceTaskData *theInst);
static bool appA2dpBuildPreferredList(avInstanceTaskData *theInst, const uint8 *seid_list, uint8 seid_list_size);
static void appA2dpIssueStartRes(avInstanceTaskData *theInst, CsrBtAvResult result, uint8 strHdl);
static uint8 appA2dpGetSepIndexBySeid(avInstanceTaskData *theInst, uint8 seid);
static void appA2dpUpdateConfiguredServiceCaps(avInstanceTaskData *theInst, uint8 local_sep_index,
                                               const uint8 *new_service_caps, uint16 new_service_caps_size);

static a2dp_codec_settings *appA2dpGetCodecSettings(const avInstanceTaskData *theInst)
{
    a2dp_codec_settings *codec_settings = appA2dpGetCodecAudioParams(theInst);

    /* If pointer is valid and is_twsp_mode is set then change SEID to virtual
     * SEID AV_SEID_APTX_ADAPTIVE_TWS_SNK */
    if (codec_settings && codec_settings->codecData.aptx_ad_params.is_twsp_mode)
        codec_settings->seid = AV_SEID_APTX_ADAPTIVE_TWS_SNK;

    return codec_settings;
}


/*! \brief Update UI to show streaming state

    This function updates the UI when streaming is active, the current SEID
    is checked to differentiate between the codec type streaming.
*/
static void appA2dpStreamingActiveUi(avInstanceTaskData *theInst)
{
    /* Call appropriate UI function */
    switch (theInst->a2dp.current_seid)
    {
        case AV_SEID_SBC_SNK:
        case AV_SEID_AAC_SNK:
        case AV_SEID_SBC_MONO_TWS_SNK:
        case AV_SEID_AAC_STEREO_TWS_SNK:
            appAvSendUiMessageId(AV_STREAMING_ACTIVE);
            return;

        case AV_SEID_APTX_SNK:
        case AV_SEID_APTXHD_SNK:
        case AV_SEID_APTX_ADAPTIVE_SNK:
        case AV_SEID_APTX_MONO_TWS_SNK:
        case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
            appAvSendUiMessageId(AV_STREAMING_ACTIVE_APTX);
            return;
    }
}

/*! \brief Update UI to show streaming inactive

    This function updates the UI when streaming becomes inactive.
    If just calls the appropriate UI module function.
*/
static void appA2dpStreamingInactiveUi(avInstanceTaskData *theInst)
{
    if (appA2dpIsSinkCodec(theInst) || appA2dpIsSourceCodec(theInst))
    {
        appAvSendUiMessageId(AV_STREAMING_INACTIVE);
    }
}

/*! \brief Stop audio. */
static void appA2dpStopAudio(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpStopAudio(%p), device_id=%d, stream_id=%d", (void *)theInst, theInst->a2dp.device_id, theInst->a2dp.stream_id);
    device_t device = Av_FindDeviceFromInstance(theInst);

    /* Allow Audio disconnect. However set A2DP Stop lock only if a2dp_source is still connected. It may so happen that because of audio_router
        priority algo it might have already disconnect A2DP chain. So, there is no point in setting the AUDIO_STOP_LOCK.
        We need to just inform audio router that a2dp is no more available so that it can remove from its database */
    if ((BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET) && A2dpProfile_IsSourceConnected(theInst))
    {
        /* Use the lock to prevent a call to appAvInstanceDestroy until cleared */
        appA2dpSetAudioStopLockBit(theInst);
        DEBUG_LOG("appA2dpStopAudio(%p) %p %u", (void *)theInst, &appA2dpGetLock(theInst), appA2dpGetLock(theInst));
    }
    else
    {
        DEBUG_LOG("appA2dpStopAudio(%p) Dont set lock, because of a2dp source_state: %d", (void *)theInst, A2dpProfile_GetSourceState(theInst));
    }

    AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_DISCONNECTED);
    /* Tell clients we are not streaming */
    AvSendStreamingStatusMessage(theInst, AV_STREAMING_INACTIVE_IND);
}

/*! \brief Called on exiting a '_SYNC' state.
           The _SYNC state can be exited for reasons other than receiving
           a #AV_INTERNAL_A2DP_INST_SYNC_RES, for example, receiving a disconnect
           message from the A2DP library. In this case, when the
           #AV_INTERNAL_A2DP_INST_SYNC_RES is finally received, the handler
           (#appA2dpHandleInternalA2dpInstSyncResponse) needs to know the message
           should be ignored. Incrementing the a2dp_sync_counter achieves this.
*/
static void appA2dpInstSyncExit(avInstanceTaskData *theInst)
{
    theInst->a2dp.sync_counter++;
}

void A2dpProfile_SendMediaConnectReq(avInstanceTaskData *av_instance, uint8 seid, uint16 delay_ms, bool send_later)
{
    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);
    message->seid = seid;
    MessageCancelFirst(&av_instance->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);

    if(send_later)
    {
        message->delay_ms = 0;
        MessageSendLater(&av_instance->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ, message, delay_ms);
    }
    else
    {
        message->delay_ms = delay_ms;
        MessageSendConditionally(&av_instance->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ, message,
                                    &appA2dpGetLock(av_instance));
    }
}

/*! \brief Enter A2DP_STATE_CONNECTING_LOCAL

    The A2DP state machine has entered 'connecting local' state, set the
    lock to serialise connect attempts and block and other operations on this
    instance.
*/
static void appA2dpEnterConnectingLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectingLocal(%p)", (void *)theInst);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);

    /* Clear detach pending flag */
    theInst->detach_pending = FALSE;

    /* Set locally initiated flag */
    theInst->a2dp.bitfields.local_initiated = TRUE;
}

/*! \brief Exit A2DP_STATE_CONNECTING_LOCAL

    The A2DP state machine has exited 'connecting local' state, clear the
    lock to allow pending connection attempts
    and any pending operations on this instance to proceed.
*/
static void appA2dpExitConnectingLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectingLocal(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);

    /* We have finished (successfully or not) attempting to connect, so
     * we can relinquish our lock on the ACL.  Bluestack will then close
     * the ACL when there are no more L2CAP connections */
    ConManagerReleaseAcl(&theInst->bd_addr);
}

/*! \brief Determine if device is PTS tester.

    \param bd_addr Pointer to read-only device BT address to check.
    \return bool TRUE if device is PTES tester, FALSE if it isn't.
*/
static bool a2dpProfile_DeviceIsPts(const bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        uint16 flags = 0;
        Device_GetPropertyU16(device, device_property_flags, &flags);
        return !!(flags & DEVICE_FLAGS_IS_PTS);
    }
    else
        return FALSE;
}

/*! \brief  Enter A2DP_STATE_CONNECTED_SIGNALLING

    The A2DP state machine has entered 'connected signalling' state, this means
    that the A2DP signalling channel has been established.

    Kick the link policy manager to make sure this link is configured correctly
    and to maintain the correct link topology.

    Check if we need to create media channel immediately, either because this
    is a locally initiated connection with SEIDs specified or because there
    is already another A2DP sink with media channel established.
*/
static void appA2dpEnterConnectedSignalling(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedSignalling(%p)(PARENT)", (void *)theInst);

    /* Set PTS test mode if device is PTS tester */
    if (a2dpProfile_DeviceIsPts(&theInst->bd_addr))
    {
        A2dpProfile_SetPtsMode(TRUE);
    }

    /* Clear current SEID */
    theInst->a2dp.current_seid = AV_SEID_INVALID;
    theInst->a2dp.stream_id = CSR_BT_AV_MAX_NUM_STREAMS;

    DEBUG_LOG("appA2dpEnterConnectedSignalling connect_flag %d", appA2dpIsConnectMediaFlagSet(theInst));

    /* Request A2DP media connect:
       When sending the media connect internal msg:
       * If A2DP_CONNECT_MEDIA is set don't use a delay (locally initiated A2DP)
       * Otherwise use a delay (remotely initiated A2DP)
    */
    A2dpProfile_SendMediaConnectReq(theInst,
                                    AV_SEID_INVALID,
                                    appA2dpIsConnectMediaFlagSet(theInst) ? 0 : appConfigA2dpMediaConnectDelayAfterLocalA2dpConnectMs(),
                                    FALSE);

    /* Play connected UI. */
    appAvSendUiMessageId(AV_CONNECTED);

    /* Update AV instance now that A2DP is connected */
    appAvInstanceA2dpConnected(theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTED_SIGNALLING

    The A2DP state machine has exited 'connected signalling' state, this means
    that the A2DP signalling channel has closed.
*/
static void appA2dpExitConnectedSignalling(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedSignalling(%p)(PARENT)", (void *)theInst);

    /* Cancel any pending media connect request */
    MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);

    /* Clear current SEID */
    theInst->a2dp.current_seid = AV_SEID_INVALID;

    /* Clear PTS test mode */
    A2dpProfile_SetPtsMode(FALSE);
}

/*! \brief Enter A2DP_STATE_CONNECTING_MEDIA_LOCAL

    The A2DP state machine has entered 'connecting media local' state, this means
    that the A2DP media channel is required.  Set the
    operation lock to block any other operations on this instance and initiate
    opening media channel.

*/
static void appA2dpEnterConnectingMediaLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectingMediaLocal(%p) current_seid (%d)", (void *)theInst, theInst->a2dp.current_seid);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);
    const uint8* seid_list_array;
    uint16 seid_list_size;

    if (theInst->a2dp.current_seid != AV_SEID_INVALID)
    {
        /* Open media channel to peer device */
        seid_list_array = &theInst->a2dp.current_seid;
        seid_list_size = 1;
    }
    else
    {
        seid_list_size = theInst->av_callbacks->GetMediaChannelSeids(&seid_list_array);
    }

    if (!appA2dpBuildPreferredList(theInst, seid_list_array, seid_list_size))
    {
        appA2dpAbortStreamConnect(theInst);
        return;
    }
    AvDiscoverReqSend(theInst->a2dp.device_id, A2DP_ASSIGN_TLABEL(theInst));
}

/*! \brief Exit A2DP_STATE_CONNECTING_MEDIA_LOCAL

    The A2DP state machine has exited 'connecting media' state, clear the
    operation lock to allow any pending operations on this instance to proceed.
*/
static void appA2dpExitConnectingMediaLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectingMediaLocal(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTING_MEDIA_REMOTE

    Remote Media connection phase involves handling of remote:
    Discovery indication
    Get capabilities indication
    Set configuration indication
 */
static void appA2dpEnterConnectingMediaRemote(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectingMediaRemote: current_seid(%d)",
                  theInst->a2dp.current_seid);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTING_MEDIA_REMOTE

    The A2DP state machine has exited 'connecting media remote' state, clear the
    operation lock to allow any pending operations on this instance to proceed.
*/
static void appA2dpExitConnectingMediaRemote(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectingMediaRemote(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC

    The A2DP state machine has entered 'connecting media remote' state, this means
    that the A2DP media channel is being opened by the remote device.  Set the
    operation lock to block any other operations on this instance and attempt
    to synchronise the other instance.
*/
static void appA2dpEnterConnectingMediaRemoteSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectingMediaRemoteSync(%p), seid %d", (void *)theInst, theInst->a2dp.current_seid);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);
    
    /* Send a request for any registered audio_sync instance to also become
       'connected' and then wait for the response. */
    a2dpProfileSync_SendConnectIndication(theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC

    The A2DP state machine has exited 'connecting media remote' state, clear the
    operation lock to allow any pending operations on this instance to proceed.
*/
static void appA2dpExitConnectingMediaRemoteSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectingMediaRemoteSync(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);

    appA2dpInstSyncExit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA

    The A2DP state machine has entered 'connected media' state, this means
    that the A2DP media channel has been established.
*/
static void appA2dpEnterConnectedMedia(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMedia(%p)", (void *)theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA

    The A2DP state machine has exited 'connected media' state, this means
    that the A2DP media channel has closed.
*/
static void appA2dpExitConnectedMedia(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMedia(%p)(PARENT)", (void *)theInst);

    theInst->a2dp.suspend_state &= ~(AV_SUSPEND_REASON_LOCAL | AV_SUSPEND_REASON_REMOTE);

    /* Tell the registered audio_sync object that we are now in the 'disconnected' state. */
    a2dpProfileSync_SendStateIndication(theInst, AUDIO_SYNC_STATE_DISCONNECTED);
}

/*! \brief Enter A2DP_STATE_DISCONNECTING_MEDIA

    The A2DP state machine has entered 'disconnecting media' state, this means
    that we have initiated disconnecting the A2DP media channel.

    Set the operation lock to block any other operations, call
    AvCloseReqSend() to actually request closing of the media channel.
*/
static void appA2dpEnterDisconnectingMedia(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterDisconnectingMedia(%p)(PARENT)", (void *)theInst);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);

    /* Close media channel to peer device */
    AvCloseReqSend(theInst->a2dp.stream_id, theInst->a2dp.tLabel);
}

/*! \brief Exit A2DP_STATE_DISCONNECTING_MEDIA

    The A2DP state machine has exited 'disconnecting media' state, this means
    that we have completed disconnecting the A2DP media channel.

    Clear the operation lock to allow any pending operations on this instance
    to proceed.
*/
static void appA2dpExitDisconnectingMedia(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitDisconnectingMedia(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_STREAMING

    The A2DP state machine has entered 'connected media streaming' state, this means
    that the A2DP media channel is now streaming audio.
*/
static void appA2dpEnterConnectedMediaStreaming(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaStreaming(%p)", (void *)theInst);

    /* Call appropriate UI function */
    appA2dpStreamingActiveUi(theInst);

    /* Tell a registered audio_sync object we are 'active' */
    a2dpProfileSync_SendStateIndication(theInst, appA2dpSyncGetAudioSyncState(theInst));

    /* Tell clients we are streaming */
    AvSendStreamingStatusMessage(theInst, AV_STREAMING_ACTIVE_IND);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_STREAMING

    The A2DP state machine has exited 'connected media streaming' state, this means
    that the A2DP media channel has stopped streaming audio state.
*/
static void appA2dpExitConnectedMediaStreaming(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaStreaming(%p)", (void *)theInst);

#ifndef INCLUDE_AV_SOURCE
    /* Reset play state so the context reverts to streaming when we next start */
    Av_ResetPlayStatus(theInst);
#endif
    /* Stop UI indication */
    appA2dpStreamingInactiveUi(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED

    The A2DP state machine has entered 'connected media streaming muted' state, this means
    that the headset has failed to suspend the audio.
*/
static void appA2dpEnterConnectedMediaStreamingMuted(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaStreamingMuted(%p)", (void *)theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED

    The A2DP state machine has exited 'connected media streaming muted' state, this means
    that either about to start streaming again or we're disconnecting.
*/
static void appA2dpExitConnectedMediaStreamingMuted(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaStreamingMuted(%p)", (void *)theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL

    The A2DP state machine has entered 'connected media suspending' state, this means
    that the A2DP media channel needs to be suspended.

    SeF the operation lock to block any other operations, call
    AvSuspendReqSend() to actually request suspension of the stream.
*/
static void appA2dpEnterConnectedMediaSuspendingLocal(avInstanceTaskData *theInst)
{
    uint8 *sHandle = (uint8 *)PanicNull(malloc(sizeof(uint8)));

    DEBUG_LOG("appA2dpEnterConnectedMediaSuspendingLocal(%p)", (void *)theInst);

    *sHandle = theInst->a2dp.stream_id;

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);

    /* Suspend A2DP streaming */
    AvSuspendReqSend(A2DP_ASSIGN_TLABEL(theInst), sHandle, 1);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL

    The A2DP state machine has exited 'connected media suspending' state, this could
    be for a number of reasons.	 Clear the operation lock to allow other operations to
    proceed.
*/
static void appA2dpExitConnectedMediaSuspendingLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaSuspendingLocal(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_SUSPENDED

    The A2DP state machine has entered 'connected media suspended' state, this means
    the audio streaming has now actually suspended.
*/
static void appA2dpEnterConnectedMediaSuspended(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaSuspended(%p)", (void *)theInst);
    
    /* If the A2DP source is already connected, wait for it to disconnect before moving
       audio sync back to the 'connected' state */
    if(theInst->a2dp.source_state != source_state_connected)
    {
        a2dpProfileSync_SendStateIndication(theInst, AUDIO_SYNC_STATE_CONNECTED);
    }

    appAvInstanceStartMediaPlayback(theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_SUSPENDED

    The A2DP state machine has exited 'connected media suspended' state, this could
    be for a number of reasons.
*/
static void appA2dpExitConnectedMediaSuspended(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaSuspended(%p)", (void *)theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING

    This means a codec reconfiguration is in progress.
*/
static void appA2dpEnterConnectedMediaReconfiguring(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaReconfiguring(%p)", (void *)theInst);
    appA2dpSetTransitionLockBit(theInst);
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING

    This means a codec reconfiguration completed.
*/
static void appA2dpExitConnectedMediaReconfiguring(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaReconfiguring(%p)", (void *)theInst);
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC

    The A2DP state machine has entered 'connected media starting local sync' state,
    this means we should synchronise with the slave.
*/
static void appA2dpEnterConnectedMediaStartingLocalSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaStartingLocalSync(%p)", (void *)theInst);
    
#ifdef INCLUDE_AV_SOURCE
    /* Always assume sync required for source application */
    bool a2dp_sync_already_prepared = FALSE;
    theInst->a2dp.bitfields.local_media_start_audio_sync_complete = FALSE;
#else
    bool a2dp_sync_already_prepared = theInst->a2dp.bitfields.local_media_start_audio_sync_complete = a2dpIsSyncFlagSet(theInst, A2DP_SYNC_PREPARED);
#endif

    appDeviceUpdateMruDevice(&theInst->bd_addr);
    
    /* Start as quickly as possible */
    appPowerPerformanceProfileRequest();

    /* Set operation locks */
    appA2dpSetTransitionLockBit(theInst);

    DEBUG_LOG("appA2dpEnterConnectedMediaStartingLocalSync: local_media_start_audio_sync_complete %d", theInst->a2dp.bitfields.local_media_start_audio_sync_complete);

    /* Send a request for any registered audio_sync instance to also become
       'active' and then wait for the response. */
    a2dpClearAllSyncFlags(theInst);
    a2dpSetSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);
    
    if(a2dp_sync_already_prepared)
    {
        a2dpSetSyncFlag(theInst, A2DP_SYNC_PREPARED);
        a2dpProfileSync_SendMediaStartRequest(theInst);
        AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_CONNECTED);
    }
    else
    {
        a2dpProfileSync_SendPrepareIndication(theInst);
        AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_CONNECTING);
    }  
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC

    The A2DP state machine has exited 'connected media starting local sync' state,
    this means we have either sucessfully synchronised the slave or we
    failed for some reason.
*/
static void appA2dpExitConnectedMediaStartingLocalSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaStartingLocalSync(%p)", (void *)theInst);
    
    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);

    appA2dpInstSyncExit(theInst);

    appPowerPerformanceProfileRelinquish();
}

/*! \brief Enter A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC

    The A2DP state machine has entered 'connected media starting remote' state,
    this means the remote device has requested to start streaming.

    We sync the slave and get a message back that triggers the A2dpStartResponse().

    The operation lock is set to that any other operations are blocked until we
    have exited this state.
*/
static void appA2dpEnterConnectedMediaStartingRemoteSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterConnectedMediaStartingRemoteSync(%p)", (void *)theInst);
    
#ifdef INCLUDE_AV_SOURCE
    /* Always assume sync required for source application. */
    bool a2dp_sync_already_prepared = FALSE;
#else
    bool a2dp_sync_already_prepared = a2dpIsSyncFlagSet(theInst, A2DP_SYNC_PREPARED);
#endif

    appDeviceUpdateMruDevice(&theInst->bd_addr);

    /* Start as quickly as possible */
    appPowerPerformanceProfileRequest();

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);

    /* Send a request for any registered audio_sync instance to also become
       'active' and then wait for the response. */
    a2dpClearAllSyncFlags(theInst);
    a2dpSetSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);

    if(a2dp_sync_already_prepared)
    {
        a2dpSetSyncFlag(theInst, A2DP_SYNC_PREPARED);
        a2dpProfileSync_SendMediaStartResponse(theInst);
        a2dpClearSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);
        AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_CONNECTED);
        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING);     
    }
    else
    {
        a2dpProfileSync_SendPrepareIndication(theInst);
        AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_CONNECTING);
    }
}

/*! \brief Exit A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC

    The A2DP state machine has exited 'connected media starting remote' state,
    this means we have either sucessfully started streaming or we failed for some
    reason.

    As we are exiting this state we can clear the operation lock to allow any other
    blocked operations to proceed.
*/
static void appA2dpExitConnectedMediaStartingRemoteSync(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitConnectedMediaStartingRemoteSync(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);

    appA2dpInstSyncExit(theInst);

    appPowerPerformanceProfileRelinquish();
}

/*! \brief Enter A2DP_STATE_DISCONNECTING

    The A2DP state machine has entered 'disconnecting' state, this means that
    we have initiated a disconnect.  Set the operation lock to prevent any other
    operations occuring and call A2dpDisconnectAll() to start the disconnection.
*/
static void appA2dpEnterDisconnecting(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterDisconnecting(%p)", (void *)theInst);

    /* Set operation lock */
    appA2dpSetTransitionLockBit(theInst);

    /* Make sure AVRCP isn't doing something important, send internal message blocked on
       AVRCP lock */
    MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_UNLOCK_IND, NULL, &appAvrcpGetLock(theInst));
}

/*! \brief Exit A2DP_STATE_DISCONNECTING

    The A2DP state machine has exited 'disconnect' state, this means we have
    completed the disconnect, clear the operation lock so that any blocked
    operations can proceed.
*/
static void appA2dpExitDisconnecting(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitDisconnecting(%p)", (void *)theInst);

    /* Clear operation lock */
    appA2dpClearTransitionLockBit(theInst);
}

/*! \brief Enter A2DP_STATE_DISCONNECTED

    The A2DP state machine has entered 'disconnected' state, this means we
    have completely disconnected.  Generally after entering the
    'disconnected' state we'll received a AV_INTERNAL_A2DP_DESTROY_REQ message
    which will destroy this instance.
*/
static void appA2dpEnterDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpEnterDisconnected(%p) %p %u", (void *)theInst, &appA2dpGetLock(theInst), appA2dpGetLock(theInst));

    /* A2DP is now disconnected, clear pending A2DP disconnect requests */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_UNLOCK_IND);

    /* We may need the AV instance for the cleanup of other modules, so cancel any pending destroy requests */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);

    /* Clear the Configured SEP */
    appA2dpStreamReset(theInst);

    /* Clear A2DP device ID and stream handle */
    theInst->a2dp.device_id = INVALID_DEVICE_ID;
    theInst->a2dp.stream_id = CSR_BT_AV_MAX_NUM_STREAMS;

    /* Tell clients we have disconnected */
    appAvInstanceA2dpDisconnected(theInst);
    
    /* Tell sync client we intend to destroy the AV instance */
    a2dpProfileSync_SendDestroyIndication(theInst);
    
    /* Send ourselves a destroy message so that AV clients are notified first and
       any other messages waiting on the operation lock can be handled */
    MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ, NULL, &appA2dpGetLock(theInst));
}

/*! \brief Exiting A2DP_STATE_DISCONNECTED

    The A2DP state machine has entered 'disconnected' state, this means we
    are about to connect to the peer device, either for a new connection or
    on a reconnect attempt.
*/
static void appA2dpExitDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpExitDisconnected(%p)", (void *)theInst);

    /* Reset disconnect reason */
    theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_INVALID_REASON;

    /* Clear any queued AV_INTERNAL_A2DP_DESTROY_REQ messages, as we are exiting the
       'destroyed' state, probably due to a incoming connection */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
}

/*! \brief Set A2DP state

    Called to change state.  Handles calling the state entry and exit functions.
    Note: The entry and exit functions will be called regardless of whether or not
    the state actually changes value.
*/
void appA2dpSetState(avInstanceTaskData *theInst, avA2dpState a2dp_state)
{
    avA2dpState a2dp_old_state = theInst->a2dp.state;
    DEBUG_LOG("appA2dpSetState(%p) old state: enum:avA2dpState:%d, new state: enum:avA2dpState:%d",
                (void *)theInst, a2dp_old_state, a2dp_state);

    /* Handle state exit functions */
    switch (a2dp_old_state)
    {
        case A2DP_STATE_CONNECTING_LOCAL:
            appA2dpExitConnectingLocal(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            appA2dpExitConnectingMediaLocal(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            appA2dpExitConnectingMediaRemote(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
            appA2dpExitConnectingMediaRemoteSync(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            appA2dpExitConnectedMediaStreaming(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            appA2dpExitConnectedMediaStreamingMuted(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            appA2dpExitConnectedMediaSuspendingLocal(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            appA2dpExitConnectedMediaSuspended(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING:
            appA2dpExitConnectedMediaReconfiguring(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            appA2dpExitConnectedMediaStartingLocalSync(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
            appA2dpExitConnectedMediaStartingRemoteSync(theInst);
            break;
        case A2DP_STATE_DISCONNECTING_MEDIA:
            appA2dpExitDisconnectingMedia(theInst);
            break;
        case A2DP_STATE_DISCONNECTING:
            appA2dpExitDisconnecting(theInst);
            break;
        case A2DP_STATE_DISCONNECTED:
            appA2dpExitDisconnected(theInst);
            break;
        default:
            break;
    }

    /* Check if exiting 'kymera on' state */
    if (appA2dpIsKymeraOnInState(a2dp_old_state) && !appA2dpIsKymeraOnInState(a2dp_state))
        appA2dpStopAudio(theInst);

    /* Check if exiting 'connected media' parent state */
    if (appA2dpIsStateConnectedMedia(a2dp_old_state) && !appA2dpIsStateConnectedMedia(a2dp_state))
        appA2dpExitConnectedMedia(theInst);

    /* Check if exiting 'connected signalling' parent state */
    if (appA2dpIsStateConnectedSignalling(a2dp_old_state) && !appA2dpIsStateConnectedSignalling(a2dp_state))
        appA2dpExitConnectedSignalling(theInst);

    /* Set new state */
    theInst->a2dp.state = a2dp_state;

    /* Check if entering 'connected signalling' parent state */
    if (!appA2dpIsStateConnectedSignalling(a2dp_old_state) && appA2dpIsStateConnectedSignalling(a2dp_state))
        appA2dpEnterConnectedSignalling(theInst);

    /* Check if entering 'connected media' parent state */
    if (!appA2dpIsStateConnectedMedia(a2dp_old_state) && appA2dpIsStateConnectedMedia(a2dp_state))
        appA2dpEnterConnectedMedia(theInst);

    /* Handle state entry functions */
    switch (a2dp_state)
    {
        case A2DP_STATE_CONNECTING_LOCAL:
            appA2dpEnterConnectingLocal(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            appA2dpEnterConnectingMediaLocal(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            appA2dpEnterConnectingMediaRemote(theInst);
            break;
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
            appA2dpEnterConnectingMediaRemoteSync(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            appA2dpEnterConnectedMediaStreaming(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            appA2dpEnterConnectedMediaStreamingMuted(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            appA2dpEnterConnectedMediaSuspendingLocal(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            appA2dpEnterConnectedMediaSuspended(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING:
            appA2dpEnterConnectedMediaReconfiguring(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            appA2dpEnterConnectedMediaStartingLocalSync(theInst);
            break;
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
            appA2dpEnterConnectedMediaStartingRemoteSync(theInst);
            break;
        case A2DP_STATE_DISCONNECTING_MEDIA:
            appA2dpEnterDisconnectingMedia(theInst);
            break;
        case A2DP_STATE_DISCONNECTING:
            appA2dpEnterDisconnecting(theInst);
            break;
        case A2DP_STATE_DISCONNECTED:
            appA2dpEnterDisconnected(theInst);
            break;
        default:
            break;
    }

    /* Update link policy following change in state */
    appLinkPolicyUpdatePowerTable(&theInst->bd_addr);

    if(appDeviceIsPeerConnected())
    {
        bdaddr peer_bd_addr;
        appDeviceGetPeerBdAddr(&peer_bd_addr);
        appLinkPolicyUpdatePowerTable(&peer_bd_addr);
    }
}

/*! \brief Get A2DP state

    \param  theAv   The AV instance for this A2DP link

    \return The current A2DP state.
*/
avA2dpState appA2dpGetState(avInstanceTaskData *theAv)
{
    return theAv->a2dp.state;
}

/*! \brief Handle A2DP error

    Some error occurred in the A2DP state machine.

    To avoid the state machine getting stuck, if instance is connected then
    drop connection and move to 'disconnecting' state.
*/
void appA2dpError(avInstanceTaskData *theInst, MessageId id, Message message)
{
    UNUSED(message); UNUSED(id);

#if defined(AV_DEBUG) || defined(AV_DEBUG_PANIC)
    DEBUG_LOG("appA2dpError(%p), state enum:avA2dpState:0x%x, MESSAGE:0x%x",
                (void *)theInst, theInst->a2dp.state, id);
#endif

    /* Check if we are connected */
    if (appA2dpIsStateConnectedSignalling(appA2dpGetState(theInst)))
    {
        /* Move to 'disconnecting' state */
        appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING);
    }

    /* Destroy ourselves */
    MessageSend(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ, NULL);
}

/*! \brief Request outgoing A2DP connection

    Handle A2DP connect request from AV parent task, store connection
    parameters and move into the 'connecting local' state.  The state machine
    will handle creating the connection.  If we are not in the 'disconnected'
    state then just ignore the request as it was probably due to a
    race-condition, this can happen as the AV_INTERNAL_A2DP_CONNECT_REQ can be
    blocked by the ACL lock not the operation lock.
*/
static void appA2dpHandleInternalA2dpConnectRequest(avInstanceTaskData *theInst,
                                                    const AV_INTERNAL_A2DP_CONNECT_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpConnectRequest(%p), addr[%04x,%02x,%06lx]",
              (void *)theInst, theInst->bd_addr.nap, theInst->bd_addr.uap, theInst->bd_addr.lap);

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_DISCONNECTED:
        {
            /* Check ACL is connected */
            if (ConManagerIsConnected(&theInst->bd_addr))
            {
                CsrBtAvRole local_role = theInst->av_callbacks->GetA2dpLocalRole();
                CsrBtAvRole remote_role = (local_role == CSR_BT_AV_AUDIO_SINK)? CSR_BT_AV_AUDIO_SOURCE : CSR_BT_AV_AUDIO_SINK;
                CsrBtDeviceAddr deviceAddr;

                DEBUG_LOG("appA2dpHandleInternalA2dpConnectRequest(%p), ACL connected", theInst);

                /* Store connection parameters */
                theInst->a2dp.bitfields.flags = req->flags;
                theInst->a2dp.bitfields.connect_retries = req->num_retries;

                /* Request outgoing connection */
                BdaddrConvertVmToBluestack(&deviceAddr, &theInst->bd_addr);
                AvConnectReqSend(&(AvGetTaskData()->task), deviceAddr, remote_role);

                /* Move to 'connecting local' state */
                appA2dpSetState(theInst, A2DP_STATE_CONNECTING_LOCAL);
            }
            else
            {
                /* Check if we should retry */
                if (req->num_retries)
                {
                    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_REQ);

                    DEBUG_LOG("appA2dpHandleInternalA2dpConnectRequest(%p), ACL not connected, retrying", theInst);

                    /* Send message to retry connecting this AV instance */
                    message->num_retries = req->num_retries - 1;
                    message->flags = req->flags;
                    MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ);
                    MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ, message,
                                             ConManagerCreateAcl(&theInst->bd_addr));

                    /* Move to 'disconnected' state */
                    appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                    MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
                    return;
                }
                else
                {
                    DEBUG_LOG("appA2dpHandleInternalA2dpConnectRequest(%p), ACL not connected", theInst);

                    /* Set disconnect reason */
                    theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_CONNECT_FAILED;

                    /* Move to 'disconnected' state. This will also notify the
                       result of the connect request to requestors. */
                    appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                }
            }
        }
        return;

        case A2DP_STATE_DISCONNECTING:
        {
            /* Send AV_INTERNAL_A2DP_CONNECT_REQ to start A2DP connection */
            MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_REQ);

            DEBUG_LOG("appA2dpHandleInternalA2dpConnectRequest(%p) repost connect request", (void*)theInst);

            /* Send message to newly created AV instance to connect A2DP */
            message->num_retries = req->num_retries;
            message->flags = req->flags;
            MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ);
            MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ, message,
                                     &appA2dpGetLock(theInst));
        }
        return;

        default:
            return;
    }
}

static inline void a2dpProfile_StoreSeid(avInstanceTaskData *theInst, uint8 seid)
{
    theInst->a2dp.current_seid = seid;
    theInst->a2dp.last_known_seid = seid;
}

/*! \brief Request A2DP media channel

    Handle A2DP open media channel request from AV parent task, or self.
    Only valid in the 'connected signalling' state, should never be received in any of the
    transition states as the operation lock will block the request.
*/
static void appA2dpHandleInternalA2dpConnectMediaRequest(avInstanceTaskData *theInst,
                                                         const AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpConnectMediaRequest(%p) delay_ms %u", (void *)theInst, req->delay_ms);

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_SIGNALLING:
        {
            if (req->delay_ms)
            {
                /* Connect media channel */
                A2dpProfile_SendMediaConnectReq(theInst, req->seid, req->delay_ms, TRUE);
                return;
            }

            a2dpProfile_StoreSeid(theInst, req->seid);

            /* Move to 'local connecting media' state */
            appA2dpSetState(theInst, A2DP_STATE_CONNECTING_MEDIA_LOCAL);
        }
        return;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        case A2DP_STATE_DISCONNECTED:
            return;

        case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
        {
            /* Remote has already started Media connection */
            MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);
        }
        return;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ, NULL);
            return;
    }
}

/*! \brief Disconnect A2DP media channel

    Handle A2DP close media channel request from AV parent task, or self.
    Only valid in the 'connected media' states, should never be received in any of the
    transition states as the operation lock will block the request.
*/
static void appA2dpHandleInternalA2dpDisconnectMediaRequest(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpDisconnectMediaRequest(%p)", (void *)theInst);

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        {
            /* Move to 'local disconnecting media' state */
            appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING_MEDIA);
        }
        return;

        case A2DP_STATE_CONNECTED_SIGNALLING:
        case A2DP_STATE_DISCONNECTED:
            return;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_DISCONNECT_MEDIA_REQ, NULL);
            return;
    }
}

/*! \brief Request A2DP disconnection

    Handle A2DP disconnect request from AV parent task.  Move into the
    'disconnecting' state, this will initiate the disconnect.
*/
static void appA2dpHandleInternalA2dpDisconnectRequest(avInstanceTaskData *theInst,
                                                       const AV_INTERNAL_A2DP_DISCONNECT_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpDisconnectRequest(%p)", (void *)theInst);

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_SIGNALLING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        {
            /* Store flags */
            theInst->a2dp.bitfields.flags |= req->flags;

            /* Move to 'disconnecting' state */
            appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING);
        }
        return;

        case A2DP_STATE_DISCONNECTED:
            /* Ignore as instance already disconnected */
            return;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_DISCONNECT_REQ, req);
            return;
    }
}

/*! \brief Request suspend A2DP streaming

    Handle A2DP suspend request from AV parent task, move into the
    'suspending local' state, this will initate the suspend.

    Record the suspend reason, to prevent resuming if there are outstanding
    suspend reasons.
*/
static void appA2dpHandleInternalA2dpSuspendRequest(avInstanceTaskData *theInst,
                                                    const AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpSuspendRequest(%p) suspend_state(0x%x) reason(0x%x)",
                (void *)theInst, theInst->a2dp.suspend_state, req->reason);

    /* Record suspend reason */
    theInst->a2dp.suspend_state |= req->reason;

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTING_LOCAL:
        case A2DP_STATE_CONNECTED_SIGNALLING:
        case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        case A2DP_STATE_DISCONNECTING_MEDIA:
        case A2DP_STATE_DISCONNECTING:
        case A2DP_STATE_DISCONNECTED:
            return;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        {
            /* Suspend always:
             * Sinks always want to suspend, same goes for non-TWS Sources
             * which are the only Sources using this app now */
            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL);
        }
        return;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ, req);
            return;
    }
}

/*! \brief Request start A2DP streaming

    Handle A2DP resume request from AV parent task.  Clear the suspend reason,
    if there are no suspend reasons left then we can attempt to initiate A2DP
    streaming.
*/
static void appA2dpHandleInternalA2dpResumeRequest(avInstanceTaskData *theInst,
                                                   const AV_INTERNAL_A2DP_RESUME_MEDIA_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpResumeRequest(%p) enum:avSuspendReason(0x%x) reason(0x%x)",
                (void *)theInst, theInst->a2dp.suspend_state, req->reason);

    /* Clear suspend reason */
    theInst->a2dp.suspend_state &= ~req->reason;

    /* Immediately return if suspend is not cleared. */
    if (theInst->a2dp.suspend_state)
    {
        return;
    }

    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        {
            /* Resume if this instance is an AV sink, or if AV source and the
               other instance sync sent the resume request */

            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC);

        }
        return;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        {
            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING);
        }
        return;

        case A2DP_STATE_CONNECTING_LOCAL:
        case A2DP_STATE_CONNECTED_SIGNALLING:
        case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
        case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        case A2DP_STATE_DISCONNECTING_MEDIA:
        case A2DP_STATE_DISCONNECTING:
        case A2DP_STATE_DISCONNECTED:
            return;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ, req);
            return;
    }
}

static void appA2dpInitiateReconfig(avInstanceTaskData *theInst,
                                    uint8 *configured_media_caps,
                                    uint16 configured_media_caps_size)
{
    uint8 *tobe_configured_caps;

    DEBUG_LOG_INFO("appA2dpInitiateReconfig: Reconfiguring media channel...");

    tobe_configured_caps = (uint8 *)PanicUnlessMalloc(configured_media_caps_size);
    memcpy(tobe_configured_caps, configured_media_caps, configured_media_caps_size);
    AvReconfigReqSend(theInst->a2dp.stream_id,
                      A2DP_ASSIGN_TLABEL(theInst),
                      configured_media_caps_size, tobe_configured_caps);

    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING);
}

/*! \brief Request to reconfigure the A2DP media channel

    Handle request to reconfigure the negotiated A2DP codec parameters.
    An A2DP reconfigure can only be issued when A2DP is suspended. So the normal
    procedure is to initiate an A2DP suspend, then queue up an A2DP reconfigure
    for after the suspend has completed.
*/
static void appA2dpHandleInternalA2dpReconfigureRequest(avInstanceTaskData *theInst,
                                                        const AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ_T *req)
{
    DEBUG_LOG("appA2dpHandleInternalA2dpReconfigureRequest(%p)", (void *)theInst);

    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        {
            appA2dpInitiateReconfig(theInst, req->new_media_caps, req->new_media_caps_size);
        }
        break;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        case A2DP_STATE_DISCONNECTING_MEDIA:
        case A2DP_STATE_DISCONNECTING:
        case A2DP_STATE_DISCONNECTED:
            DEBUG_LOG("appA2dpHandleInternalA2dpReconfigureRequest, wrong state - ignoring");
            break;

        default:
            appA2dpError(theInst, AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ, req);
            break;
    }

    /* Free the memory allocated for the media caps, regardless of outcome. */
    free(req->new_media_caps);
}

/*! \brief Handle AV Activate Confirmation.
    \param theAv The AV.
    \param cfm   The AV Confirmation message of Activate Request.

    AV Library has sent confirmation for the AV activation request.
*/
static void appA2dpHandleAvActivateCfm(const CsrBtAvActivateCfm *cfm)
{
    appAvHandleA2dpInitConfirm(cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS);
}

/*! \brief Handle incoming A2DP connection.
    \param theAv The AV.
    \param ind The A2DP signalling connect indication.

    A2DP Library has indicated an incoming established A2DP connection.
*/
static void appA2dpHandleAvConnectInd(const CsrBtAvConnectInd *ind)
{
    avInstanceTaskData *av_inst;
    bdaddr bd_addr = {0};

    /* Check there isn't already an A2DP instance for this device */
    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);
    av_inst = appAvInstanceFindFromBdAddr(&bd_addr);
    if (av_inst == NULL)
        av_inst = appAvInstanceCreate(&bd_addr, &av_plugin_interface);
    else
    {
        /* Make sure there's no pending destroy message */
        MessageCancelAll(&av_inst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
        MessageCancelAll(&av_inst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);
    }

    DEBUG_LOG("appA2dpHandleAvConnectInd(%p) addr(%04x,%02x,%06lx) connectionId(%d)",
                av_inst, bd_addr.nap, bd_addr.uap, bd_addr.lap, ind->connectionId);

    if (av_inst != NULL)
    {
        /* Make sure instance isn't about to be destroyed */
        MessageCancelAll(&av_inst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);

        av_inst->a2dp.btConnId  = ind->btConnId;
        /* Consider connection id as device id */
        av_inst->a2dp.device_id = ind->connectionId;
        /* Clear locally initiated flag */
        av_inst->a2dp.bitfields.local_initiated = FALSE;
        /* Clear locally configured flag */
        av_inst->a2dp.bitfields.local_configured = FALSE;

        switch (appA2dpGetState(av_inst))
        {
            case A2DP_STATE_DISCONNECTED:
            case A2DP_STATE_CONNECTING_LOCAL:
            {
                    DEBUG_LOG("appA2dpHandleAvConnectInd: Connected remote");

                    /* Reset suspend reasons */
                    av_inst->a2dp.suspend_state &= ~(AV_SUSPEND_REASON_LOCAL | AV_SUSPEND_REASON_REMOTE);

                    /* Clear detach pending flag */
                    av_inst->detach_pending = FALSE;

                    /* Everything is good and we are connected, hence move to connected state */
                    appA2dpSetState(av_inst, A2DP_STATE_CONNECTED_SIGNALLING);
                    MessageCancelAll(&av_inst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ);
            }
            break;

            default:
                appA2dpError(av_inst, CSR_BT_AV_CONNECT_IND, ind);
                break;
        }
    }
    else
    {
        /* Reject incoming connection, either there is an existing instance, or we failed to create a new instance */
        DEBUG_LOG("appA2dpHandleAvConnectInd, rejecting");
        AvDisconnectReqSend(ind->connectionId);
    }
}

/*! \brief A2DP signalling channel confirmation

    A2DP library has confirmed signalling channel connect request.
    First of all check if the request was successful, if it was then we should
    store the pointer to the newly created A2DP instance, also obtain the
    address of the remote device from the Sink.  After this move into the
    'connect signalling' state as we now have an active A2DP signalling channel.

    If the request was unsuccessful, move back to the 'disconnected' state and
    play an error tone if this connection request was silent.  Note: Moving to
    the 'disconnected' state may result in this AV instance being free'd.
*/
static void appA2dpHandleAvConnectCfm(const CsrBtAvConnectCfm *cfm)
{
    avInstanceTaskData *theInst;
    bdaddr bd_addr = {0};

    BdaddrConvertBluestackToVm(&bd_addr, &cfm->deviceAddr);
    theInst = appAvInstanceFindFromBdAddr(&bd_addr);

    DEBUG_LOG("appA2dpHandleAvConnectCfm(%p) addr(%04x,%02x,%06lx) result(0x%04x) supplier(0x%04x) connectionId(%d)",
                theInst, bd_addr.nap, bd_addr.uap, bd_addr.lap,
                cfm->avResultCode, cfm->avResultSupplier, cfm->connectionId);

    /* Since it is possible in cross over scenario that remote
     * is already connected and outgoing conn is outstanding for
     * same device. Just return irrespective of connection result
     * if instance is already connected remotely */
    if (theInst && !appA2dpIsConnected(theInst))
    {
        DEBUG_LOG("appA2dpHandleAvConnectCfm: detach pending %d", theInst->detach_pending);
        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_LOCAL:
            {
                /* Check if signalling channel created successfully */
                if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                    cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                {
                    /* Consider connection id as device id */
                    theInst->a2dp.device_id = cfm->connectionId;
                    theInst->a2dp.btConnId  = cfm->btConnId;

                    /* Reset suspend reasons */
                    theInst->a2dp.suspend_state &= ~(AV_SUSPEND_REASON_LOCAL | AV_SUSPEND_REASON_REMOTE);

                    /* Move to 'connected signalling' state */
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
                }
                else if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_ALREADY_CONNECTED &&
                         cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                {
                    /* Already connected with remote device, just complete the connect request */
                    ProfileManager_GenericConnectCfm(profile_manager_a2dp_profile,
                                                     Av_GetDeviceForInstance(theInst),
                                                     TRUE);
                }
                else
                {
                    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SDC_SEARCH_FAILED &&
                        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    {
                         BtDevice_RemoveSupportedProfiles(&bd_addr, DEVICE_PROFILE_A2DP);
                    }

                    /* Set disconnect reason */
                    theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_CONNECT_FAILED;

                    /* Check if we should retry
                       - Don't retry if the ACL has already been disconnected.
                    */
                    if (   !theInst->detach_pending
                        && theInst->a2dp.bitfields.connect_retries)
                    {
                        MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_REQ);

                        /* Send message to retry connecting this AV instance */
                        message->num_retries = theInst->a2dp.bitfields.connect_retries - 1;
                        message->flags = theInst->a2dp.bitfields.flags;
                        MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ);
                        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ, message,
                                                 ConManagerCreateAcl(&theInst->bd_addr));

                        /* Move to 'disconnected' state */
                        appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                        MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
                        return;
                    }
                    else
                    {
                        appAvSendUiMessageId(AV_ERROR);

                        /* Move to 'disconnected' state */
                        appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                    }
                }
            }
            return;

            default:
                appA2dpError(theInst, CSR_BT_AV_CONNECT_CFM, cfm);
                return;
        }
    }
    else
    {
        DEBUG_LOG("appA2dpHandleAvConnectCfm: Av instance not found or already connected remotely ");
    }
}

/*! \brief A2DP connection disconnected

    A2DP Library has indicated that the signalling channel for A2DP
    has been disconnected, move to the 'disconnected' state, this will
    result in this AV instance being destroyed.
*/
static void appA2dpHandleAvDisconnectInd(const CsrBtAvDisconnectInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvDisconnectInd(%p): reason(0x%04x) supplier(0x%04x) device_id(%d)",
                (void *)theInst, ind->reasonCode, ind->reasonSupplier, ind->connectionId);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.device_id == ind->connectionId);

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_LOCAL:
            case A2DP_STATE_CONNECTED_SIGNALLING:
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                {
                    /* Check if disconnected due to link-loss */
                    if ((ind->reasonCode == L2CA_DISCONNECT_LINK_LOSS ||
                                ind->reasonCode == CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT) &&
                            !theInst->detach_pending)
                    {
                        appAvSendUiMessageId(AV_LINK_LOSS);

                        /* Set disconnect reason */
                        theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_LINKLOSS;
                    }
                    else if (ind->reasonCode == CSR_BT_RESULT_CODE_AV_SUCCESS)
                    {
                        /* Play disconnected UI if not the peer */
                        if (!appDeviceIsPeer(&theInst->bd_addr))
                        {
                            appAvSendUiMessageId(AV_DISCONNECTED);
                        }

                        /* Set disconnect reason */
                        theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_NORMAL;
                    }
                    else
                    {
                        /* Play disconnected UI if not the peer */
                        if (!appDeviceIsPeer(&theInst->bd_addr))
                        {
                            appAvSendUiMessageId(AV_DISCONNECTED);
                        }

                        /* Set disconnect reason */
                        theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_ERROR;
                    }

                    /* Move to 'disconnected' state */
                    appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                }
                return;

            case A2DP_STATE_DISCONNECTING_MEDIA:
            case A2DP_STATE_DISCONNECTING:
            case A2DP_STATE_DISCONNECTED:
                {
                    /* Play disconnected UI if not the peer */
                    if (!appDeviceIsPeer(&theInst->bd_addr) &&
                            ind->reasonCode != L2CA_DISCONNECT_LINK_TRANSFERRED)
                    {
                        appAvSendUiMessageId(AV_DISCONNECTED);
                    }

                    if (ind->reasonCode == L2CA_DISCONNECT_LINK_TRANSFERRED)
                    {
                        /* Set disconnect reason for link transferred during handover */
                        theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_LINK_TRANSFERRED;
                    }
                    else
                    {
                        /* Set disconnect reason for normal disconnection */
                        theInst->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_NORMAL;
                    }

                    /* Move to 'disconnected' state */
                    appA2dpSetState(theInst, A2DP_STATE_DISCONNECTED);
                }
                return;

            default:
                appA2dpError(theInst, CSR_BT_AV_DISCONNECT_IND, ind);
                return;
        }
    }
}

/*! \brief A2DP media channel open indication

    A2DP Library has confirmed opening of the media channel, if the channel we opened
    successfully then store the SEID for use later one.

    Check if the channel should be suspended or streaming and move into the appropriate
    state.

    The state entry functions will handle resuming/suspending the channel.
    If the channel open failed then move back to 'connected signalling' state and play
    error tone.
*/
static void appA2dpHandleAvOpenInd(const CsrBtAvOpenInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);
    CsrBtAvResult result = CSR_BT_AV_ACCEPT;

    DEBUG_LOG("appA2dpHandleAvOpenInd(%p) sHandle(%d)", (void *)theInst, ind->shandle);

    if (theInst != NULL)
    {
        uint8 sep_index = appA2dpGetSepIndexBySeid(theInst, theInst->a2dp.current_seid & 0x3f);

        assert(theInst->a2dp.stream_id == ind->shandle);

        if (sep_index == A2DP_SEP_INDEX_INVALID)
        {
            result = CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID;
        }
        else
        {
            a2dpSepDataType *currentSep = (a2dpSepDataType *)PanicNull(appA2dpBlockGetCurrent(theInst, DATA_BLOCK_SEP_LIST));

            if (currentSep->sep_config->seid != (theInst->a2dp.current_seid & 0x3f))
            {
                result = CSR_BT_RESULT_CODE_A2DP_BAD_STATE;
            }
            else
            {
                MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);

                switch (appA2dpGetState(theInst))
                {
                    case A2DP_STATE_CONNECTED_SIGNALLING:
                    case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
                    case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
                    case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
                        {
                            a2dpProfile_StoreSeid(theInst, theInst->a2dp.current_seid);
                            theInst->a2dp.stream_id = ind->shandle;

                            /* this info would be required to send Av Open Response */
                            theInst->a2dp.tLabel = ind->tLabel;

                            /* Mark media channel as suspended by remote */
                            theInst->a2dp.suspend_state |= AV_SUSPEND_REASON_REMOTE;

                            /* Av Open Response will be sent on successful sync */
                            /* Move to 'connecting media remote' state, wait for Mtu Size Ind */
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC);
                        }
                        break;

                    case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
                    case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
                    case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
                    case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
                    case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
                    case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                    case A2DP_STATE_DISCONNECTING_MEDIA:
                    case A2DP_STATE_DISCONNECTING:
                    case A2DP_STATE_DISCONNECTED:
                        {
                            DEBUG_LOG("appA2dpHandleAvOpenInd(%p) rejecting, shandle(%d)", (void *)theInst, ind->shandle);

                            /* Reject media connection */
                            result = CSR_BT_RESULT_CODE_A2DP_BAD_STATE;
                        }
                        return;

                    default:
                        appA2dpError(theInst, CSR_BT_AV_OPEN_IND, ind);
                        break;
                }
            }
        }
    }

    if (result != CSR_BT_AV_ACCEPT)
    {
        DEBUG_LOG("appA2dpHandleAvOpenInd: rejecting, result(%d)", result);
        AvOpenResRejSend(ind->shandle, ind->tLabel, result);
    }
}

static void A2dpProfile_sendErrorIndication(void)
{
    appAvSendUiMessageId(AV_ERROR);
}

/*! \brief A2DP media channel open confirmation

    A2DP Library has confirmed opening of the media channel, if the channel we opened
    successfully then store the SEID for use later one.

    Check if the channel should be suspended or streaming and move into the appropriate
    state.

    The state entry functions will handle resuming/suspending the channel.
    If the channel open failed then move back to 'connected signalling' state and play
    error tone.
*/
static void appA2dpHandleAvOpenCfm(const CsrBtAvOpenCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvOpenCfm(%p): result(0x%04x) supplier(0x%04x) shandle(%d)",
                theInst, cfm->avResultCode, cfm->avResultSupplier, cfm->shandle);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == cfm->shandle);

        if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
            cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
        {
            a2dpProfile_StoreSeid(theInst, theInst->a2dp.current_seid);

            /* At this point check if SEID is for aptX adaptive, if it is
             * check the configuration, if it's TWS+ then use virtual SEID */
            if (theInst->a2dp.current_seid == AV_SEID_APTX_ADAPTIVE_SNK)
            {
                /* Attempt to get CODEC settings */
                a2dp_codec_settings *codec_settings = appA2dpGetCodecSettings(theInst);

                /* If pointer is valid then change SEID to virtual
                 * SEID AV_SEID_APTX_ADAPTIVE_TWS_SNK */
                if (codec_settings)
                {
                    a2dpProfile_StoreSeid(theInst, codec_settings->seid);
                    free(codec_settings);
                }
            }
        }

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
                {
                    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    {
                        Source src =  StreamL2capSource(theInst->a2dp.device_id);

                        SourceConfigure(src, STREAM_SOURCE_HANDOVER_POLICY, SOURCE_HANDOVER_ALLOW);

                        /* A sink instance should never initiate a start. Set the
                        reason to avoid starts until the source initiates the
                        start, which will clear the reason from the state. */

                        theInst->a2dp.suspend_state |= AV_SUSPEND_REASON_REMOTE;

                        /* Change to Media Starting Local or Suspended state after
                           receiving Stream Mtu Size Indication from AV library */
                    }
                    else
                    {
                        appA2dpAbortStreamConnect(theInst);
                    }
                }
                return;

            case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
                {
                    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    {
                        /* Remote initiated media channel defaults to suspended.
                           Move to Suspended state after MTU size indication.
                           Till then be in same state. */
                    }
                    /* Receiving A2DP_MEDIA_OPEN_CFM with a2dp_wrong_state in this
                       state means a A2DP MEDIA OPEN cross-over occurred.
                       Stay in CONNECTING_MEDIA_REMOTE_SYNC as There is no need to
                       perform a state transition. */
                    else if (cfm->avResultCode != CSR_BT_RESULT_CODE_AV_NOT_CONNECTED)
                    {
                        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
                    }
                }
                return;

            case A2DP_STATE_DISCONNECTED:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
                return;

            default:
                if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                    cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    appA2dpError(theInst, CSR_BT_AV_OPEN_CFM, cfm);
                return;
        }
    }
}

/*! \brief Find AV instance using A2dp stream handle list

    This function finds the AV instance whose stream handle
    matches with the stream handle exist in "sHandleList".
*/
static avInstanceTaskData * appA2dpFindAvInstanceBySHandle(uint8 sHandleListLen, uint8 *sHandleList)
{
    uint8 i;
    avInstanceTaskData * av_instance = NULL;

    for (i=0; i < sHandleListLen; i++)
    {
        av_instance = AvInstance_GetInstanceForSearchId(sHandleList[i], FALSE, DEVICE_PROFILE_A2DP);

        if (av_instance)
        {
            break;
        }
    }

    return av_instance;
}

/*! \brief Handle A2DP streaming start indication

    A2DP Library has indicated streaming of the media channel, accept the
    streaming request and move into the appropriate state.  If there is still
    a suspend reason active move into the 'streaming muted'.
*/
static void appA2dpHandleAvStartInd(const CsrBtAvStartInd *ind)
{
    avInstanceTaskData *theInst = appA2dpFindAvInstanceBySHandle(ind->listLength, ind->list);

    DEBUG_LOG("appA2dpHandleAvStartInd(%p)", (void *)theInst);

    PanicNull(theInst);

    /* Record the fact that remote device has request start */
    theInst->a2dp.suspend_state &= ~AV_SUSPEND_REASON_REMOTE;
    theInst->a2dp.tLabel = ind->tLabel;

    DEBUG_LOG("appA2dpHandleAvStartInd: enum:avA2dpState(%d) suspend_state(%d) sHandle(%d)",
              appA2dpGetState(theInst), theInst->a2dp.suspend_state, theInst->a2dp.stream_id);

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        {
            /* Check if we should still be suspended */
            if (theInst->a2dp.suspend_state == 0)
            {
                if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC)
                {
                    /* As apps already in the process of starting the MEDIA and
                       this flag suggets synchronisation is completed so need to
                       respond to AG. */
                    DEBUG_LOG("appA2dpHandleAvStartInd: local_media_start_audio_sync_complete %d",
                              theInst->a2dp.bitfields.local_media_start_audio_sync_complete);

                    if (theInst->a2dp.bitfields.local_media_start_audio_sync_complete)
                    {
                        a2dpProfileSync_HandleMediaStartResponse(theInst);
                        a2dpProfileSync_HandleMediaStartConfirm(theInst);
                    }
                    else
                    {
                        /* Receiving CSR_BT_AV_START_IND in state
                           A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC means a
                           start cross-over occurred. When entering state
                           STARTING_LOCAL_SYNC the same actions are performed as
                           required when entering STARTING_REMOVE_SYNC, so there is
                           no need to perform a state transition - instead just
                           change the local state without going through a state
                           transition. */
                        theInst->a2dp.state = A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC;
                    }
                }
                else
                {
                    /* Move to 'connected media starting remote' state to wait for
                       the other instance to start streaming */
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC);
                    TimestampEvent(TIMESTAMP_EVENT_A2DP_START_IND);
                }
            }
            else
            {
                /* Note: We could reject the AVDTP_START at this point, but this
                   seems to upset some AV sources, so instead we'll accept
                   but just drop the audio data */

                /* Accept streaming start request */
                appA2dpIssueStartRes(theInst, CSR_BT_AV_ACCEPT, theInst->a2dp.stream_id);

                /* Move into 'streaming muted' state */
                appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED);
            }
        }
        return;

        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
        {
            /* Accept streaming start request */
            appA2dpIssueStartRes(theInst, CSR_BT_AV_ACCEPT, theInst->a2dp.stream_id);

            /* Not ready to start streaming, so enter the 'streaming muted' state */
            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED);
        }
        return;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        {
            /* Accept streaming start request */
            appA2dpIssueStartRes(theInst, CSR_BT_AV_ACCEPT, theInst->a2dp.stream_id);
        }
        return;

        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        {
            /* Received duplicate, just ignore and hope for the best... */
        }
        return;

        default:
            appA2dpError(theInst, CSR_BT_AV_START_IND, ind);
            return;
    }
}

/*! \brief Handle A2DP streaming start confirmation

    A2DP Library has confirmed streaming of the media channel, if successful
    and in the 'suspended' or 'starting local' state then move into the
    'streaming' state if there is no suspend reasons pending otherwise
    move into the 'suspending local' state.  If streaming failed then move
    into the 'suspended' state and wait for the peer device to start streaming.
*/
static void appA2dpHandleAvStartCfm(const CsrBtAvStartCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvStartCfm(%p): result(0x%04x) supplier(0x%04x)",
               (void *)theInst, cfm->avResultCode, cfm->avResultSupplier);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.device_id == cfm->connectionId);

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
                {
                    /* Ignore, we're already streaming */
                }
                return;

            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                {
                    /* Check confirmation is success */
                    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    {
                        /* Check if we should start or suspend streaming */
                        if (theInst->a2dp.suspend_state != 0)
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL);
                        else
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING);
                    }
                    /* Receiving CSR_BT_AV_START_CFM with CSR_BT_RESULT_CODE_AV_NOT_CONNECTED in these
                       states means a A2DP MEDIA START cross-over occurred.
                       There is no need to perform a state transition. */
                    else if (cfm->avResultCode != CSR_BT_RESULT_CODE_AV_NOT_CONNECTED)
                    {
                        if(cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SIGNAL_TIMEOUT ||
                           cfm->avResultCode == CSR_BT_RESULT_CODE_A2DP_SEP_IN_USE)
                        {
                            /* Handling CSR_BT_RESULT_CODE_A2DP_SEP_IN_USE error to cover IOP issue with AirPods.
                             * If AV start request is send after initiating SCO connection, AirPods rejects with SEP_IN_USE error.
                             * Post SEP_IN_USE error, AirPods not responding to AV start requests, which leads to TIMEOUT. */
                            appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING_MEDIA);
                        }
                        else
                        {
                            /* Move to suspended state */
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDED);
                        }
                    }
                }
                return;

            default:
                appA2dpError(theInst, CSR_BT_AV_START_CFM, cfm);
                return;
        }
    }
}

/*! \brief Handle A2DP streaming suspend indication

    The peer device has suspended the media channel.  Move into the
    'connected suspended' state, the state entry function will handle
    turning off the DACs and powering down the DSP.
*/
static void appA2dpHandleAvSuspendInd(const CsrBtAvSuspendInd *ind)
{
    avInstanceTaskData *theInst = appA2dpFindAvInstanceBySHandle(ind->listLength, ind->list);
    uint8 *sHandleList = (uint8 *)PanicNull(malloc(sizeof(uint8)));

    PanicNull(theInst);

    *sHandleList = theInst->a2dp.stream_id;

    DEBUG_LOG("appA2dpHandleAvSuspendInd(%p) stream_id(%d)", (void *)theInst, theInst->a2dp.stream_id);

    if(appA2dpIsSourceCodec(theInst) && appA2dpGetState(theInst) == A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL)
    {
        /* If the above condition is TRUE a suspend crossover has been detected.
           To avoid a stalemate, we (master) don't set the AV_SUSPEND_REASON_REMOTE.
           The slave (that is remotely suspended) will be resumed by us later. */
        DEBUG_LOG("appA2dpHandleA2dpMediaSuspendIndication, crossover detected by master");
    }
    else
    {
        /* Record the fact that remote device has request suspend */
        theInst->a2dp.suspend_state |= AV_SUSPEND_REASON_REMOTE;
    }

    /* Handle different states */
    switch (appA2dpGetState(theInst))
    {
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
        {
            AvSuspendResAcpSend(ind->tLabel, 1, sHandleList);
            /* Move to 'connect media suspended' state */
            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDED);
        }
        return;

        default:
            AvSuspendResRejSend(theInst->a2dp.stream_id,
                                ind->tLabel,
                                CSR_BT_RESULT_CODE_A2DP_BAD_STATE,
                                1,
                                sHandleList);
            appA2dpError(theInst, CSR_BT_AV_SUSPEND_IND, ind);
            return;
    }
}

/*! \brief Handle A2DP streaming suspend confirmation

    Confirmation of request to suspend streaming.  If our request to suspend
    streaming was sucessful move to the 'suspended' or 'resuming' state depending
    on whether a resume was pending.

    If the suspend request was rejected then move to the 'streaming muted' or
    'streaming' state depending on whether a resume was pending.  If a resume was
    pending we can go straight to the 'streaming' state and the suspend never
    actually happened.
*/
static void appA2dpHandleAvSuspendCfm(const CsrBtAvSuspendCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvSuspendCfm(%p) result(0x%04x) supplier(0x%04x)",
               (void *)theInst, cfm->avResultCode, cfm->avResultSupplier);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.device_id == cfm->connectionId);

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
                /* Already suspended, so just ignore */
                return;

            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                {
                    /* Check if we suspended */
                    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                    {
                        /* Check if we should start or suspend streaming */
                        if (theInst->a2dp.suspend_state != 0)
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDED);
                        else
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC);
                    }
                    else
                    {
                        /* Check if we should start or mute streaming */
                        if (theInst->a2dp.suspend_state != 0)
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED);
                        else
                            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC);
                    }
                }
                return;

            default:
                appA2dpError(theInst, CSR_BT_AV_SUSPEND_CFM, cfm);
                return;
        }
    }
}

/*! \brief Handle media channel closed remotely, or in reponse to a  local call
    to AvCloseReqSend().

    The peer device has disconnected the media channel, move back to the
    'connected signalling' state, the state machine entry & exit functions
    will automatically disconnect the DACs & DSP if required.
*/
static void appA2dpHandleAvCloseInd(const CsrBtAvCloseInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == ind->shandle);
        DEBUG_LOG("appA2dpHandleAvCloseInd(%p) sHandle (%d)", (void *)theInst, ind->shandle);

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_SIGNALLING:
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
            case A2DP_STATE_DISCONNECTING_MEDIA:
                {
                    appA2dpStreamReset(theInst);
                    /* Send Close Indication Response as Success */
                    AvCloseResAcpSend(ind->shandle, ind->tLabel);
                    /* Move to 'connected signalling' */
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
                }
                return;

            case A2DP_STATE_CONNECTING_LOCAL:
            case A2DP_STATE_DISCONNECTED:
                {
                    /* Probably late message from A2DP profile library, just ignore */
                }
                return;

            default:
                {
                    AvCloseResRejSend(ind->shandle, ind->tLabel, CSR_BT_RESULT_CODE_A2DP_BAD_STATE);
                    appA2dpError(theInst, CSR_BT_AV_CLOSE_IND, ind);
                }
                return;
        }
    }
}

/*! \brief Handle media av sync delay indication

    Causes a delay report to be issued to a connected Source.
*/
static void appA2dpHandleAvDelayReportInd(const CsrBtAvDelayReportInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvDelayReportInd(%p) shandle(%d)", (void *)theInst, ind->shandle);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == ind->shandle);

        if (appA2dpIsConnected(theInst))
        {
            AvDelayReportResAcpSend(ind->shandle, ind->tLabel);
        }
        else
        {
            AvDelayReportResRejSend(ind->shandle, ind->tLabel, CSR_BT_RESULT_CODE_A2DP_BAD_STATE);
            appA2dpError(theInst, CSR_BT_AV_DELAY_REPORT_IND, ind);
        }
    }
}

static void appA2dpHandleAvReconfigureInd(const CsrBtAvReconfigureInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvReconfigureInd(%p)", (void *)theInst);

    if (theInst != NULL)
    {
        uint8 sepIndex = appA2dpGetSepIndexBySeid(theInst, theInst->a2dp.current_seid);

        assert(theInst->a2dp.stream_id == ind->shandle);

        if (sepIndex != A2DP_SEP_INDEX_INVALID)
        {
            uint8 errCode, errorSvcCat;

            DEBUG_LOG("appA2dpHandleAvReconfigureInd(%p) current_seid(%d)",
                      (void *)theInst, theInst->a2dp.current_seid);

            /* Reconfig Indication is received from AV library only if ACP Seid
             * (received from remote) is verified that it is in use.
             * So, no need to verify again. Just validate new capabilities
             * before reconfiguring.
             */
            if (appA2dpCapabilitiesDataValidated(theInst, ind, &errorSvcCat, &errCode, sepIndex))
            {
                /* New capabilities are validated, configure the same */
                appA2dpUpdateConfiguredServiceCaps(theInst, sepIndex, ind->servCapData, ind->servCapLen);
                theInst->a2dp.tLabel = ind->tLabel;
                /* Tell an registered audio_sync object of the changed codec parameters. */
                AudioSync_CodecReconfiguredIndication(&theInst->a2dp.sync_if, Av_GetSourceForInstance(theInst),
                        theInst->a2dp.current_seid, theInst->a2dp.device_id,
                        theInst->a2dp.stream_id);
                /* Send accept response to remote */
                AvReconfigResAcpSend(ind->shandle, ind->tLabel);
            }
            else
            {
                /* Send reject response to remote */
                AvReconfigResRejSend(ind->shandle, ind->tLabel, errCode, errorSvcCat);
            }
        }
        else
        {
            /* Send reject response to remote */
            AvReconfigResRejSend(ind->shandle, ind->tLabel,
                    CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID, *(ind->servCapData));
        }
    }
}

/*! \brief Handle internal indication that AVRCP is unlocked

    This function is called when we have determined that the AVRCP library is no
    longer locked, we need to make sure AVRCP is unlocked in case there is
    a passthrough command in the process of being sent.
*/
static void appA2dpHandleInternalAvrcpUnlockInd(avInstanceTaskData *theInst)
{
    /* AVRCP is now unlocked, we can proceed with the disconnect */
    AvDisconnectReqSend(theInst->a2dp.device_id);
}

/*! \brief Initialise AV instance

    This function initialises the specified AV instance, all state variables are
    set to defaults, with the exception of the streaming state which is
    initialised with the value supplied. If non-zero this has the effect of
    blocking streaming initially.

    \note This function should only be called on a newly created
    instance.

    \param theAv            The AV that has the A2DP instance to initialise
    \param suspend_state    The initial suspend_state to apply
 */
void appA2dpInstanceInit(avInstanceTaskData *theAv, uint8 suspend_state)
{
    audio_source_t source = Av_GetSourceForInstance(theAv);
    
    theAv->a2dp.state = A2DP_STATE_DISCONNECTED;
    theAv->a2dp.current_seid = AV_SEID_INVALID;
    theAv->a2dp.stream_id = CSR_BT_AV_MAX_NUM_STREAMS;
    theAv->a2dp.bitfields.flags = 0;
    theAv->a2dp.lock = 0;
    theAv->a2dp.sync_counter = 0;
    theAv->a2dp.suspend_state = suspend_state;
    theAv->a2dp.bitfields.local_initiated = FALSE;
    theAv->a2dp.bitfields.local_configured = FALSE;
    theAv->a2dp.bitfields.disconnect_reason = AV_A2DP_DISCONNECT_NORMAL;
    theAv->a2dp.bitfields.connect_retries = 0;
    theAv->a2dp.source_state = source_state_invalid;
    theAv->a2dp.supported_sample_rates = 0;
    theAv->a2dp.preferred_sample_rate = A2DP_PREFERRED_RATE_NONE;

    a2dpClearAllSyncFlags(theAv);

    /* No profile instance yet */
    theAv->a2dp.device_id = INVALID_DEVICE_ID;
    theAv->a2dp.data_blocks[0] = NULL;
    
    AudioSources_RegisterAudioInterface(source, A2dpProfile_GetHandsetSourceAudioInterface());
}

/*! \brief Message Handler

    This function is the main message handler for an A2DP instance, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.

    \param theInst      The instance data for the AV for this A2DP connection
    \param id           Message identifier. For internal messages, see \ref av_internal_messages
    \param[in] message  Message content, potentially NULL.

*/
void appA2dpInstanceHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message)
{
    /* Handle internal messages */
    switch (id)
    {
        case AV_INTERNAL_A2DP_CONNECT_REQ:
            appA2dpHandleInternalA2dpConnectRequest(theInst, (AV_INTERNAL_A2DP_CONNECT_REQ_T *)message);
            return;

        case AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ:
            appA2dpHandleInternalA2dpConnectMediaRequest(theInst, (AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ_T *)message);
            return;

        case AV_INTERNAL_A2DP_DISCONNECT_MEDIA_REQ:
            appA2dpHandleInternalA2dpDisconnectMediaRequest(theInst);
            return;

        case AV_INTERNAL_A2DP_DISCONNECT_REQ:
            appA2dpHandleInternalA2dpDisconnectRequest(theInst, (AV_INTERNAL_A2DP_DISCONNECT_REQ_T *)message);
            return;

        case AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ:
            appA2dpHandleInternalA2dpSuspendRequest(theInst, (AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ_T *)message);
            return;

        case AV_INTERNAL_A2DP_RESUME_MEDIA_REQ:
            appA2dpHandleInternalA2dpResumeRequest(theInst, (AV_INTERNAL_A2DP_RESUME_MEDIA_REQ_T *)message);
            return;

        case AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ:
            appA2dpHandleInternalA2dpReconfigureRequest(theInst, (AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ_T *)message);
            return;

        case AV_INTERNAL_AVRCP_UNLOCK_IND:
            appA2dpHandleInternalAvrcpUnlockInd(theInst);
            return;

        case AV_INTERNAL_A2DP_DESTROY_REQ:
            appAvInstanceDestroy(theInst);
            return;
    }

    /* Unhandled message */
    appA2dpError(theInst, id, message);
}

void A2dpProfile_SetPtsMode(bool is_enabled)
{
    pts_mode_enabled = is_enabled;
}

bool A2dpProfile_IsPtsMode(void)
{
    return pts_mode_enabled;
}

source_status_t A2dpProfile_SetSourceState(avInstanceTaskData *av_instance, source_state_t state)
{
    avA2dpState a2dp_state = appA2dpGetState(av_instance);
    source_status_t source_status = source_status_ready;
    
    source_state_t prev_state = av_instance->a2dp.source_state;
    av_instance->a2dp.source_state = state;

    DEBUG_LOG_FN_ENTRY("A2dpProfile_SetSourceState(%p) enum:avA2dpState:%d from enum:source_state_t:%d to enum:source_state_t:%d",
                        av_instance, a2dp_state, prev_state, state);

    switch(state)
    {
        case source_state_disconnected:
            /* Audio router has decided not to allow routing of A2DP source. So, reset any start request */
            appA2dpClearAudioStartLockBit(av_instance);
            a2dpClearSyncFlag(av_instance, A2DP_SYNC_PREPARED);
            /* Tell the registered audio_sync object that we are moving back to the 'connected' state. */
            a2dpProfileSync_SendStateIndication(av_instance, AUDIO_SYNC_STATE_CONNECTED);
        break;
        
        case source_state_disconnecting:
        {    
            if ((prev_state == source_state_connecting) &&
                (appA2dpCheckLockMaskIsSet(av_instance, APP_A2DP_AUDIO_STOP_LOCK)))
            {
                appA2dpClearAudioStopLockBit(av_instance);
            }
        }
        break;

        case source_state_connecting:
            switch(a2dp_state)
            {
                case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
                case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
                    if(a2dpIsSyncFlagSet(av_instance, A2DP_SYNC_PREPARED))
                    {
                        a2dpProfileSync_SendActiveIndication(av_instance);
                    }
                    else
                    {
                        if(!a2dpIsSyncFlagSet(av_instance, A2DP_SYNC_PREPARE_RESPONSE_PENDING))
                        {
                            a2dpProfileSync_SendPrepareIndication(av_instance);
                        }
                        /* stil waiting for PREPARE_RES therefore return pepraring */
                        source_status = source_status_preparing;
                    }
                break;
                
                case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
                {
                    source_status = source_status_preparing;
                }
                break;
                
                default:
                break;
            }
        break;

        default:
        break;
    }

    return source_status;
}

static bool appA2dpCapabilitiesDataValidated(avInstanceTaskData *theInst, 
                                             const CsrBtAvReconfigureInd *ind, uint8 *errorSvcCat,
                                             uint8 *errorCode, uint8 sepIndex)
{
    const a2dpSepConfigType *sepConfig = ((const a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst, 
                                                                                        DATA_BLOCK_SEP_LIST,
                                                                                        sepIndex)))->sep_config;

    if ((!appA2dpValidateServiceCaps(ind->servCapData, ind->servCapLen,
                                    FALSE, TRUE,
                                    FALSE, errorSvcCat,
                                    errorCode)) ||
        (!appA2dpValidateServiceCaps(ind->servCapData, ind->servCapLen,
                                     TRUE, FALSE,
                                     FALSE, errorSvcCat,
                                     errorCode)))
    {   /* bad caps or reconfig caps does not match our caps- reject */
        return FALSE;
    }
    else if (!appA2dpAreServicesCategoriesCompatible(sepConfig->caps, sepConfig->size_caps,
                                                     ind->servCapData, ind->servCapLen,
                                                     errorSvcCat))
    {   /* set config does not match our caps - reject */
        *errorCode = CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION;

        return FALSE;
    }
    else if (appA2dpFindMatchingCodecSpecificInformation(sepConfig->caps,
                                                         ind->servCapData,
                                                         0,errorCode) == NULL)
    {   /* requested codec is not compatible with our caps */
        *errorSvcCat  = CSR_BT_AV_SC_MEDIA_CODEC;
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static const a2dpSepDataType *appA2dpFindLocalCodecInfo(avInstanceTaskData *theInst, uint8 seid)
{
    a2dpSepDataType *sep_list = (a2dpSepDataType *)PanicNull(appA2dpBlockGetBase(theInst, DATA_BLOCK_SEP_LIST));
    unsigned sep_list_size = appA2dpBlockGetSize(theInst, DATA_BLOCK_SEP_LIST) / sizeof(a2dpSepDataType);

    while (sep_list_size--)
    {
        const a2dpSepConfigType *sep_config = sep_list->sep_config;

        if (sep_config->seid == seid)
        {
            DEBUG_LOG("appA2dpFindLocalCodecInfo: Found Local Seid:(%d)", seid);
            return sep_list;
        }

        sep_list++;
    }

    return NULL;
}

static bool appA2dpBuildPreferredList(avInstanceTaskData *theInst,
                                      const uint8 *seid_list,
                                      uint8 seid_list_size)
{
    unsigned i;
    uint8 available_seps = 0;

    DEBUG_LOG("appA2dpBuildPreferredList: seid_list=%p seid_list_size=%u", seid_list, seid_list_size);

    appA2dpBlockRemove(theInst, DATA_BLOCK_PREFERRED_LOCAL_SEIDS);

    if ((seid_list_size == 0) || (seid_list == NULL))
    {   /* Build preferred list from local seid list */
        a2dpSepDataType *sep_list = (a2dpSepDataType *)PanicNull(appA2dpBlockGetBase(theInst, DATA_BLOCK_SEP_LIST));
        unsigned sep_list_size    = appA2dpBlockGetSize(theInst, DATA_BLOCK_SEP_LIST) / sizeof(a2dpSepDataType);

        DEBUG_LOG("appA2dpBuildPreferredList: sep_list=%p sep_list_size=%u", sep_list, sep_list_size);

        /* Find available seps */
        for (i = 0; i < sep_list_size; i++)
        {
            if (sep_list[i].in_use == A2DP_SEP_IS_AVAILABLE)
            {
                available_seps++;
            }
        }

        if (available_seps)
        {
            a2dpSepInfo *preferred_list = (a2dpSepInfo *)PanicNull(appA2dpBlockAdd(theInst,
                                                                       DATA_BLOCK_PREFERRED_LOCAL_SEIDS,
                                                                       available_seps,
                                                                       sizeof(a2dpSepInfo)));

            DEBUG_LOG("appA2dpBuildPreferredList: Preferred list = ");

            while (sep_list_size--)
            {
                if (sep_list->in_use == A2DP_SEP_IS_AVAILABLE)
                {
                    const a2dpSepConfigType *sep_config = sep_list->sep_config;
                    const uint8 * local_codec = sep_config->caps;
                    uint16 local_codec_size = sep_config->size_caps;

                    /* Should never fail since local codec caps will always be present */
                    PanicFalse(appA2dpFindCodecSpecificInformation(&local_codec, &local_codec_size));

                    preferred_list->seid  = sep_config->seid;
                    preferred_list->role  = sep_config->sep_type;
                    preferred_list->codec = local_codec[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];

                    DEBUG_LOG("appA2dpBuildPreferredList: [seid:%u,role:%u,codec:%u] ",
                                    preferred_list->seid,
                                    preferred_list->role,
                                    preferred_list->codec);
                    preferred_list++;
                }

                sep_list++;
            }

            return TRUE;
        }
    }
    else
    {   /* Build preferred list from supplied seid list */
        /* Find available seps */
        for (i = 0; i < seid_list_size; i++)
        {
            const a2dpSepDataType *sep_data = appA2dpFindLocalCodecInfo(theInst, seid_list[i]);

            if (sep_data && !sep_data->in_use)
            {
                available_seps++;
            }
        }

        if (available_seps)
        {
            a2dpSepInfo *preferred_list = (a2dpSepInfo *)PanicNull(appA2dpBlockAdd(theInst,
                                                                       DATA_BLOCK_PREFERRED_LOCAL_SEIDS,
                                                                       available_seps,
                                                                       sizeof(a2dpSepInfo)));

            DEBUG_LOG("appA2dpBuildPreferredList: Preferred list = ");

            while (seid_list_size--)
            {
                const a2dpSepDataType *sep_data = appA2dpFindLocalCodecInfo(theInst, *seid_list);

                if (sep_data && !sep_data->in_use)
                {
                    const a2dpSepConfigType *sep_config = sep_data->sep_config;
                    const uint8 *local_codec = sep_config->caps;
                    uint16 local_codec_size  = sep_config->size_caps;

                    /* Should never fail since local codec caps will always be present */
                    PanicFalse(appA2dpFindCodecSpecificInformation(&local_codec, &local_codec_size));

                    preferred_list->seid  = *seid_list;
                    preferred_list->role  = sep_config->sep_type;
                    preferred_list->codec = local_codec[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];

                    DEBUG_LOG("appA2dpBuildPreferredList:: [seid:%u,role:%u,codec:%u] ", 
                                    preferred_list->seid,
                                    preferred_list->role,
                                    preferred_list->codec);
                    preferred_list++;
                }

                seid_list++;
            }

            return TRUE;
        }
    }

    DEBUG_LOG("\n");
    return FALSE;
}

/*! \brief Find the start of the service category in CODEC capabilities.

    \param          service_category The service category to search for.
    \param[in,out]  caps_ptr Must be set to point to a service category definition
    (followed by the service category length). The function will modify the pointer
    to the category header if service_category is found, or NULL if not found/malformed.
    \param[out]     size_caps_ptr Set to the total size of the category, or 0 if not
    found/malformed.
*/
static void appA2dpFindServiceCategory(uint8 service_category, const uint8 **caps_ptr, uint16 *size_caps_ptr)
{
    const uint8 *caps = *caps_ptr;
    signed size_caps = *size_caps_ptr;

    while (size_caps > 0)
    {
        if (service_category == caps[0])
        {
            break;
        }

        size_caps -= 2 + caps[1];
        caps += 2 + caps[1];
    }
    if (size_caps >= 2)
    {
        *caps_ptr = caps;
        *size_caps_ptr = size_caps;
    }
    else
    {
        /* Not found or malformed caps */
        *caps_ptr = NULL;
        *size_caps_ptr = 0;
    }
}

/*! \brief Checks if AptxAD codec configuration supported for source role.

    \param  theInst AV Instance
    \param  remote_codec Codec configuration Parameter

    \return bool TRUE if in sink role or codex is not AptxAdaptive or configuration acceptable for source role

 */
static bool appIsAptxAdConfigSupportedForSrcRole(avInstanceTaskData *theInst, uint8 *remote_codec)
{
    return theInst->av_callbacks->GetA2dpLocalRole() !=  CSR_BT_AV_AUDIO_SOURCE ||
           remote_codec[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET] != CSR_BT_AV_NON_A2DP_CODEC ||
           !isCodecAptxAdaptive(remote_codec) ||
           A2dpProfileAptxAdSelectCodecConfiguration(theInst, remote_codec);
}

static void appA2dpStreamReset(avInstanceTaskData *theInst)
{
    a2dpSepDataType *current_sep;

    DEBUG_LOG("appA2dpStreamReset");

    /* Mark current SEP as not in use anymore */
    if ((current_sep = (a2dpSepDataType *)appA2dpBlockGetCurrent(theInst, DATA_BLOCK_SEP_LIST))!=NULL)
    {
        current_sep->in_use = A2DP_SEP_IS_AVAILABLE;
    }

    /* Reset the local and remote SEIDs */
    appA2dpBlockSetCurrent(theInst, DATA_BLOCK_SEP_LIST, 0 );
    theInst->a2dp.remote_sep.codec = A2DP_MEDIA_CODEC_UNDEFINED;
    theInst->a2dp.local_sep.codec = A2DP_MEDIA_CODEC_UNDEFINED;
    theInst->a2dp.remote_sep.seid = 0;
    theInst->a2dp.local_sep.seid = 0;

    /* Remove stream specific data */
    appA2dpBlockRemove(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    appA2dpBlockRemove(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS);

    /* Clear locally configured flag */
    theInst->a2dp.bitfields.local_configured = FALSE;
}

static void appA2dpAbortStreamConnect(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpAbortStreamConnect(%p), stream_id:%d", (void *)theInst, theInst->a2dp.stream_id);
    if(theInst->a2dp.stream_id < CSR_BT_AV_MAX_NUM_STREAMS)
    {
        AvAbortReqSend(theInst->a2dp.stream_id, A2DP_ASSIGN_TLABEL(theInst));
        return;
    }
    appA2dpStreamReset(theInst);
    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
    A2dpProfile_sendErrorIndication();
}

static void appA2dpIssueStartRes(avInstanceTaskData *theInst, CsrBtAvResult result, uint8 strHdl)
{
    uint8 *list = PanicUnlessMalloc(1 * sizeof(uint8));

    *list = strHdl;

    if (result == CSR_BT_AV_ACCEPT)
    {
        /* Accept streaming start request */
        AvStartResAcpSend(theInst->a2dp.tLabel,
                          list,
                          1);
    }
    else
    {
        AvStartResRejSend(strHdl,
                          theInst->a2dp.tLabel,
                          result,
                          list,
                          1);
    }
}

static void appA2dpIssueGetCapabilitiesReq(avInstanceTaskData *theInst, uint8 tLabel)
{
    /* Ensure we are pointing to the current seid in both lists */
    theInst->a2dp.local_sep = *(a2dpSepInfo *)PanicNull(appA2dpBlockSetCurrent(theInst, DATA_BLOCK_PREFERRED_LOCAL_SEIDS, 0));
    theInst->a2dp.remote_sep = *(a2dpSepInfo *)PanicNull(appA2dpBlockSetCurrent(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS, 0));

    DEBUG_LOG("appA2dpIssueGetCapabilitiesReq: Requesting capabilities for remote seid(%d)..", theInst->a2dp.remote_sep.seid);

    /* Get capabilities of selected remote seid */
    AvGetCapabilitiesReqSend(theInst->a2dp.device_id,
                             (uint8)theInst->a2dp.remote_sep.seid,
                             tLabel);
}

static bool appA2dpProcessDiscoverRes(avInstanceTaskData *theInst, CsrBtAvDiscoverCfm *cfm)
{
    uint8 idx;
    uint8 discovered_remote_seps = 0;
    uint8 store_seps = 0;
    uint8 sep_mask = 0x01;
    CsrBtAvRole local_role = theInst->av_callbacks->GetA2dpLocalRole();
    CsrBtAvSep remote_sepType = (local_role == CSR_BT_AV_AUDIO_SINK)? CSR_BT_AV_SOURCE: CSR_BT_AV_SINK;

    /* clear record of discovered SEPs */
    appA2dpBlockRemove(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS);

    /* Check all matching media, sep types and SEPs which must not be in use */
    for(idx = 0; idx < cfm->seidInfoCount; idx++)
    {
        if (cfm->seidInfo[idx].inUse == FALSE &&
            cfm->seidInfo[idx].mediaType == CSR_BT_AV_AUDIO &&
            cfm->seidInfo[idx].sepType == remote_sepType)
        {
            discovered_remote_seps++;
            /* Record that this is a seid we are interested in */
            store_seps |= sep_mask;
        }
        sep_mask <<= 1;
    }

    DEBUG_LOG("appA2dpProcessDiscoverRes: discovered_remote_seps(%d)", discovered_remote_seps);

    if (discovered_remote_seps)
    {
        a2dpSepInfo *remote_sep = (a2dpSepInfo *)PanicNull(appA2dpBlockAdd(theInst,
                                                              DATA_BLOCK_DISCOVERED_REMOTE_SEIDS,
                                                              discovered_remote_seps,
                                                              sizeof(a2dpSepInfo)));

        /* Re-iterate through data and... */
        sep_mask = 0x01;

        for(idx = 0; idx < cfm->seidInfoCount; idx++)
        {
            if (store_seps & sep_mask)
            {/* store seids we are interested in */
                remote_sep->seid  = cfm->seidInfo[idx].acpSeid;
                remote_sep->role  = remote_sepType;
                remote_sep->codec = A2DP_MEDIA_CODEC_UNDEFINED;
                remote_sep++;
            }
            sep_mask <<= 1;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static bool appA2dpProcessGetCapsRes(avInstanceTaskData *theInst, CsrBtAvGetCapabilitiesCfm *cfm)
{
    uint8 error_cat, error_code;
    const uint8 *remote_codec;
    const uint8 *remote_caps = cfm->servCapData;
    uint8 sep_index = appA2dpGetSepIndexBySeid(theInst, theInst->a2dp.local_sep.seid);
    const a2dpSepConfigType *sep_config = ((a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst, 
                                                                            DATA_BLOCK_SEP_LIST,
                                                                            sep_index)))->sep_config;

    if (!appA2dpValidateServiceCaps(remote_caps, cfm->servCapLen, FALSE, FALSE, TRUE, &error_cat, &error_code))
    {   /* Returned capabilities are out of spec */
        DEBUG_LOG("appA2dpProcessGetCapsRes: Caps validation failed: error_code(0x%X)", error_code);
        return FALSE;
    }

    remote_codec = remote_caps;

    if (appA2dpFindCodecSpecificInformation(&remote_codec, 0))
    {   /* Update remote sep list with media codec type (slightly incohesive) */
        a2dpSepInfo *remote_sep = (a2dpSepInfo *)PanicNull(appA2dpBlockGetCurrent(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS));
        remote_sep->codec = remote_codec[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];

        /* Update local copy of remote sep info (Definitely incohesive) */
        theInst->a2dp.remote_sep = *remote_sep;
    }
    else
    {
        DEBUG_LOG("appA2dpProcessGetCapsRes: Codec Specific Info Not Found");
    }

    /* Codec Configuration. */
    if (appA2dpFindMatchingCodecSpecificInformation(sep_config->caps, remote_codec, TRUE,&error_code) == NULL)
    { /* Current local and remote codecs are not compatible -
       * either codec type and/or capabilities */
        DEBUG_LOG("appA2dpProcessGetCapsRes: Matching Codec Not Found : error_code(0x%X)", error_code);
        return FALSE;
    }

    if (!appIsAptxAdConfigSupportedForSrcRole(theInst, (uint8 *) remote_codec))
    {
        DEBUG_LOG("appA2dpProcessGetCapsRes: Unsupported aptX AD codec parameter for source role");
        return FALSE;
    }

    /* Will update the Configured Service caps to actually contain the
     * complete capabilities of the remote codec */
    appA2dpUpdateConfiguredServiceCaps(theInst, sep_index, remote_caps, cfm->servCapLen);

    return TRUE;
}

static bool appA2dpDoesCodecMatch(a2dpSepInfo *local_sep, a2dpSepInfo *remote_sep)
{
    DEBUG_LOG("appA2dpDoesCodecMatch: local_sep  = seid:0x%X role:%u codec:0x%X", local_sep->seid, local_sep->role, local_sep->codec);
    DEBUG_LOG("appA2dpDoesCodecMatch: remote_sep = seid:0x%X role:%u codec:0x%X", remote_sep->seid, remote_sep->role, remote_sep->codec);

    if ((local_sep->codec == remote_sep->codec) &&
        (local_sep->codec != A2DP_MEDIA_CODEC_UNDEFINED) &&
        (local_sep->role != remote_sep->role))
    {
        return TRUE;
    }

    return FALSE;
}

static bool appA2dpHaveCodecCaps(avInstanceTaskData *theInst, a2dpSepInfo *sep)
{
    const uint8 *service_caps;

    if ((service_caps = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS)) != NULL)
    {
        if (appA2dpFindCodecSpecificInformation(&service_caps, 0) )
        {
            if (service_caps[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET] == sep->codec)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static bool appA2dpSelectNextSeid(avInstanceTaskData *theInst)
{
    a2dpSepInfo *remote_sep;
    a2dpSepInfo *local_sep;

    DEBUG_LOG("appA2dpSelectNextSeid: selectNextSeid - ");

    if ((remote_sep = (a2dpSepInfo *)appA2dpBlockSetCurrent(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS,
                                         DATA_BLOCK_INDEX_NEXT)) == NULL)
    {
        remote_sep = (a2dpSepInfo *)PanicNull(appA2dpBlockSetCurrent(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS,
                                                  0));
        if ((local_sep = (a2dpSepInfo *)appA2dpBlockSetCurrent(theInst, DATA_BLOCK_PREFERRED_LOCAL_SEIDS,
                                            DATA_BLOCK_INDEX_NEXT)) == NULL)
        {
            DEBUG_LOG("appA2dpSelectNextSeid: Seps list exhausted\n");
            appA2dpAbortStreamConnect(theInst);
            return FALSE;
        }

        theInst->a2dp.local_sep = *local_sep;
    }

    theInst->a2dp.remote_sep = *remote_sep;

    DEBUG_LOG("appA2dpSelectNextSeid: local_sep = codec:0x%X seid:%u, remote_sep = codec:0x%X seid:%u\n",
                    theInst->a2dp.local_sep.codec, theInst->a2dp.local_sep.seid,
                    theInst->a2dp.remote_sep.codec, theInst->a2dp.remote_sep.seid);
    return TRUE;
}

static uint8 appA2dpGetSepIndexBySeid(avInstanceTaskData *theInst, uint8 seid)
{
    unsigned idx;
    unsigned max_index = appA2dpBlockGetSize(theInst, DATA_BLOCK_SEP_LIST) / sizeof(a2dpSepDataType);
    a2dpSepDataType *pSeps = (a2dpSepDataType *)PanicNull(appA2dpBlockGetBase(theInst, DATA_BLOCK_SEP_LIST));

    for (idx = 0; idx < max_index; idx++)
    {
        if (pSeps->sep_config->seid == seid)
        {
            return (uint8)idx;
        }

        pSeps++;
    }

    return A2DP_SEP_INDEX_INVALID;
}

static bool appA2dpConfigParamsSelected(avInstanceTaskData *theInst)
{
    uint8 *remote_service_caps = (uint8 *)PanicNull(appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS));
    uint8 sep_index = appA2dpGetSepIndexBySeid(theInst, theInst->a2dp.local_sep.seid);

    appA2dpBlockSetCurrent(theInst, DATA_BLOCK_SEP_LIST, sep_index);

    if (!appA2dpSelectOptimalCodecSettings(theInst, remote_service_caps))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static void appA2dpUpdateConfiguredServiceCaps(avInstanceTaskData *theInst,
                                               uint8 local_sep_index,
                                               const uint8 *new_service_caps,
                                               uint16 new_service_caps_size)
{
    const uint8 *mediaCodecCaps;
    uint8 *configuredCaps;
    uint16 mediaCodecCapsSize;
    uint16 serviceCapsSize;
    uint16 configuredCapsSize;
    bool contentProtectionSupported = FALSE;
    bool delayReportingSupported = FALSE;
    const uint8 media_transport_caps[]    = {(uint8)CSR_BT_AV_SC_MEDIA_TRANSPORT, 0x00};
    const uint8 content_protection_caps[] = {CSR_BT_AV_SC_CONTENT_PROTECTION, 0x02, A2DP_CP_TYPE_SCMS_LSB, A2DP_CP_TYPE_SCMS_MSB};
    const uint8 delay_reporting_caps[]    = {CSR_BT_AV_SC_DELAY_REPORTING, 0x00};

    /* Obtain location and size of currently configured service caps (if any) */
    configuredCaps     = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    configuredCapsSize = appA2dpBlockGetSize(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    serviceCapsSize    = sizeof(media_transport_caps);

    /* Determine location and size of the caps we wish to retain (media codec) */
    mediaCodecCaps     = new_service_caps;
    mediaCodecCapsSize = new_service_caps_size;

    if (!appA2dpFindCodecSpecificInformation(&mediaCodecCaps, &mediaCodecCapsSize))
    {   /* Caps not being updated, get current caps (if any) */
        mediaCodecCaps     = configuredCaps;
        mediaCodecCapsSize = configuredCapsSize;

        if (!appA2dpFindCodecSpecificInformation(&mediaCodecCaps, &mediaCodecCapsSize))
        {   /* No currently existing media codec caps */
            mediaCodecCaps = NULL;
        }
    }
    mediaCodecCapsSize  = (mediaCodecCaps == NULL) ? 0 : 2 + mediaCodecCaps[1];
    serviceCapsSize    += mediaCodecCapsSize;

    /* Determine location and size of the caps we wish to retain (content protection) */
    if (appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_CONTENT_PROTECTION,
                                   new_service_caps,
                                   new_service_caps_size))
    {   /* Caps being updated, do we support Content Protection locally? */
        const a2dpSepConfigType *local_sep_config = ((a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst,
                                                                                    DATA_BLOCK_SEP_LIST,
                                                                                    local_sep_index)))->sep_config;

        if (appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_CONTENT_PROTECTION,
                                       local_sep_config->caps,
                                       local_sep_config->size_caps))
        {
            /* CP supported locally, include in configured caps */
            contentProtectionSupported = TRUE;
            serviceCapsSize += sizeof(content_protection_caps);
        }
    }

    /* Determine location and size of the caps we wish to retain (content protection) */
    if (appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_DELAY_REPORTING,
                                   new_service_caps,
                                   new_service_caps_size))
    {   /* Caps being updated, do we support Delay Reporting locally? */
        const a2dpSepConfigType *local_sep_config = ((a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst,
                                                                                      DATA_BLOCK_SEP_LIST,
                                                                                      local_sep_index)))->sep_config;

        if (appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_DELAY_REPORTING, local_sep_config->caps, local_sep_config->size_caps))
        {   /* Delay Reporting supported locally, include in configured caps */
            delayReportingSupported = TRUE;
            serviceCapsSize += sizeof(delay_reporting_caps);
        }
    }

    if (serviceCapsSize)
    {
        uint8 *serviceCaps;
        uint8 *write_ptr;

        /* Allocate temporary storage area */
        write_ptr = serviceCaps = (uint8 *)PanicNull(malloc(serviceCapsSize));

        /* Fill temporary storage area with most current information */
        memmove(write_ptr, media_transport_caps, sizeof(media_transport_caps));
        write_ptr += sizeof(media_transport_caps);

        if (mediaCodecCaps)
        {
            memmove(write_ptr, mediaCodecCaps, mediaCodecCapsSize);
            write_ptr+= mediaCodecCapsSize;
        }

        if (contentProtectionSupported)
        {
            memmove(write_ptr, content_protection_caps, sizeof(content_protection_caps));
            write_ptr += sizeof(content_protection_caps);
        }

        if (delayReportingSupported)
        {
            memmove(write_ptr, delay_reporting_caps, sizeof(delay_reporting_caps));
            write_ptr += sizeof(delay_reporting_caps);
        }

        /* Update configured service caps data block */
        appA2dpBlockRemove(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
        configuredCaps = (uint8 *)PanicNull(appA2dpBlockAdd(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS, 1, (uint8)serviceCapsSize));
        memmove(configuredCaps, serviceCaps, serviceCapsSize);

        DEBUG_LOG("appA2dpUpdateConfiguredServiceCaps: Service Capabilities Configured");

        /* Release the temporary storage area */
        free(serviceCaps);
    }
}

static bool appA2dpIssueSetConfigurationReq(avInstanceTaskData *theInst, uint8 tLabel)
{
    if (appA2dpConfigParamsSelected(theInst))
    {
        /* Library has managed to select a suitable configuation */
        uint8 *configured_service_caps      = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
        uint16 configured_service_caps_size = appA2dpBlockGetSize(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
        uint8 *service_caps = (uint8 *)PanicNull(malloc(configured_service_caps_size));

        PanicNull(configured_service_caps);
        memmove(service_caps, configured_service_caps, configured_service_caps_size);
        DEBUG_LOG("appA2dpIssueSetConfigurationReq(%p): Sending Configuration Request\n", theInst);
        AvSetConfigReqSend(theInst->a2dp.device_id,
                           tLabel,
                           theInst->a2dp.remote_sep.seid,
                           theInst->a2dp.local_sep.seid,
                           configured_service_caps_size,
                           service_caps);
        /* store local seid as current seid */
        theInst->a2dp.current_seid = theInst->a2dp.local_sep.seid;

        return TRUE;
    }
    else
    {
        DEBUG_LOG("appA2dpIssueSetConfigurationReq(%p): Setting Configuration Failed\n", theInst);

        return FALSE;
    }
}

static bool appA2dpContinueStreamConnect(avInstanceTaskData *theInst,
                                            uint8 tLabel)
{
    /* Check for next seid and continue for stream connection */
    while (appA2dpSelectNextSeid(theInst))
    {
        if (theInst->a2dp.remote_sep.codec == A2DP_MEDIA_CODEC_UNDEFINED)
        {   /* We know nothing about this remote seid, request its caps */
            DEBUG_LOG("appA2dpContinueStreamConnect: Requesting Capabilities for remote seid(%d)", theInst->a2dp.remote_sep.seid);
            AvGetCapabilitiesReqSend(theInst->a2dp.device_id,
                                     (uint8)theInst->a2dp.remote_sep.seid,
                                     tLabel);
            return TRUE;
        }

        if (appA2dpDoesCodecMatch(&theInst->a2dp.local_sep,
                                  &theInst->a2dp.remote_sep))
        { /* Local (the one we want to use) and remote (the one we are prodding)
           * endpoints support the same codec */
            if (!appA2dpHaveCodecCaps(theInst, &theInst->a2dp.remote_sep))
            {   /* We don't have detailed caps stored, request the info */
                AvGetCapabilitiesReqSend(theInst->a2dp.device_id,
                                         (uint8)theInst->a2dp.remote_sep.seid,
                                         tLabel);
                return TRUE;
            }
            else
            {
                /* We have already cached the detailed caps for the remote seid,
                 * try to set something up */
                if (appA2dpIssueSetConfigurationReq(theInst,
                                                 tLabel))
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/*! Filter OUT the capability matching the filter (if any) returning the
 *  remaining length.
 *  If filtered_caps is supplied then the remaining caps are copied into
 *  this. The length needed can be determined by calling the function
 * first with a NULL pointer.
 *
 * @param filtered_caps  pointer to memory to take the filtered service caps
 * @param filter         service type to filter out. see Service Categories
 * @param full_caps      pointer to the full service capabilities
 * @param size_full_caps length of the full caps
 */
static uint16 appA2dpGetFilteredServiceCaps(uint8 *filtered_caps, uint8 filter,
                                         const uint8 *full_caps, uint16 size_full_caps)
{
    uint16 size_filtered_caps = 0;

    while (size_full_caps)
    {
        uint16 size = full_caps[1] + 2;

        if (filter!=full_caps[0])
        {   /* Copy this service category as it is not being filtered out */
            size_filtered_caps += size;
            if (filtered_caps)
            {
                memcpy(filtered_caps, full_caps, size);
                filtered_caps += size;
            }
        }

        full_caps += size;
        size_full_caps -= size;
    }

    return size_filtered_caps;
}

static void appA2dpIssueGetCapabilitiesRes(avInstanceTaskData *theInst, uint8 response,
                                           uint8 sepIndex, CsrBtAvPrim type, uint8 filter)
{
    if (response == CSR_BT_AV_ACCEPT)
    {
        const a2dpSepConfigType *sepConfig = ((const a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst,
                                                                                     DATA_BLOCK_SEP_LIST,
                                                                                     sepIndex)))->sep_config;
        /* This should only filter out illegal caps */
        uint16 capsSize = appA2dpGetFilteredServiceCaps(NULL, filter, sepConfig->caps, sepConfig->size_caps);
        uint8 *caps = (uint8 *)PanicUnlessMalloc(capsSize);

        appA2dpGetFilteredServiceCaps(caps, filter, sepConfig->caps, sepConfig->size_caps);

        if (type == CSR_BT_AV_GET_CAPABILITIES_IND)
        {
            AvGetCapabilitiesResAcpSend(theInst->a2dp.device_id,
                                        theInst->a2dp.tLabel,
                                        capsSize,
                                        caps);
        }
        else
        {
            AvGetAllCapabilitiesResAcpSend(theInst->a2dp.device_id,
                                           theInst->a2dp.tLabel,
                                           capsSize,
                                           caps);
        }
         DEBUG_LOG("appA2dpIssueGetCapabilitiesRes: Response accepted");
    }
    else
    {
        if (type == CSR_BT_AV_GET_CAPABILITIES_IND)
        {
            AvGetCapabilitiesResRejSend(theInst->a2dp.device_id,
                                        theInst->a2dp.tLabel,
                                        response);
        }
        else
        {
            AvGetAllCapabilitiesResRejSend(theInst->a2dp.device_id,
                                           theInst->a2dp.tLabel,
                                           response);
        }
        UNUSED(sepIndex);
        UNUSED(filter);
        DEBUG_LOG("appA2dpIssueGetCapabilitiesRes: Response rejected");
    }
}

static void appA2dpIssueOpenReq(avInstanceTaskData *theInst, uint8 tLabel)
{   /* AVDTP_SET_CONFIGURATION was successful */
    uint8 *configured_caps = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    uint16 size_configured_caps = appA2dpBlockGetSize(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    CsrBtAvStreamInfo strmInfo;
    uint8 local_role = theInst->a2dp.local_sep.role;

    /* Inform library about stream info */
    strmInfo.codecLocation = CSR_BT_AV_CODEC_LOCATION_ON_CHIP;
    AvSetStreamInfoReqSend(theInst->a2dp.stream_id, strmInfo);

    /* Issue an Av Sync delay notification,
     * if delay reporting has been configured and
     * local device is configured as a sink */
    if (configured_caps != NULL &&
        appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_DELAY_REPORTING,
                                  configured_caps,
                                  size_configured_caps) &&
        local_role == CSR_BT_AV_SINK)
    {
        uint16 delay;

        if (appA2dpIsSeidTwsSink(theInst->a2dp.local_sep.seid))
        {
            delay = TWS_SLAVE_LATENCY_MS * 10;
        }
        else
        {
            delay = Kymera_LatencyManagerGetLatencyForSeidInUs(theInst->a2dp.local_sep.seid) / 100;
        }

        DEBUG_LOG("appA2dpIssueOpenReq(%p) setting delay report(%d)", theInst, delay);
        /* Send Open request after receiving Delay Report confirmation */
        AvDelayReportReqSend(delay, theInst->a2dp.stream_id, tLabel);
    }
    else
    {
        /* Configure large MTU setting (if applicable) for the media channel */
        if (ConManagerGetQhsConnectStatus(&theInst->bd_addr) && appA2dpGetAptxAdQ2qMode(theInst))
        {
            DEBUG_LOG("appA2dpIssueOpenReq(%p) setting large MTU", theInst);
            AvUseLargeMtu(theInst->a2dp.device_id, TRUE);
        }
        AvOpenReqSend(theInst->a2dp.stream_id, tLabel);
    }
}

static void appA2dpHandleAvDiscoverInd(CsrBtAvDiscoverInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvDiscoverInd(%p): connectionId(%d)", theInst, ind->connectionId);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.device_id == ind->connectionId);

        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_SIGNALLING:
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
			case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            {
                unsigned i;
                uint8 sepCnt = 0;
                a2dpSepDataType *sepPtr = (a2dpSepDataType *)PanicNull(appA2dpBlockGetBase(theInst, DATA_BLOCK_SEP_LIST));
                uint8 seidInfoCnt = appA2dpBlockGetSize(theInst, DATA_BLOCK_SEP_LIST) / sizeof(a2dpSepDataType);
                CsrBtAvSeidInfo *seidInfo;

                DEBUG_LOG("appA2dpHandleAvDiscoverInd(%p): enum:avA2dpState(%d)",
                          theInst, appA2dpGetState(theInst));

                for(i = 0; i < seidInfoCnt; i++)
                {
                    if (!(sepPtr[i].in_use & A2DP_SEP_UNAVAILABLE))
                    {
                        sepCnt++;
                    }
                }

                seidInfo = (CsrBtAvSeidInfo *)(PanicUnlessMalloc(sizeof(CsrBtAvSeidInfo) * sepCnt));
                i = 0;

                while (seidInfoCnt-- && i < sepCnt)
                {
                    if (!(sepPtr->in_use & A2DP_SEP_UNAVAILABLE))
                    {
                        const a2dpSepConfigType *sep_config = sepPtr->sep_config;

                        seidInfo[i].acpSeid   = (uint8)(sep_config->seid);
                        seidInfo[i].inUse     = (sepPtr->in_use) ? TRUE : FALSE;
                        seidInfo[i].mediaType = sep_config->media_type;
                        seidInfo[i].sepType   = sep_config->sep_type;
                        i++;
                    }
                    sepPtr++;
                }

                AvDiscoverResAcpSend(ind->connectionId,
                                     ind->tLabel,
                                     sepCnt, // number of sep configs
                                     seidInfo);

                /* Move to 'connecting media remote' state only if media channel is not configured yet */
                if (theInst->a2dp.bitfields.local_configured == FALSE)
                {
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTING_MEDIA_REMOTE);
                }
            }
            return;

            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
            case A2DP_STATE_DISCONNECTING_MEDIA:
            case A2DP_STATE_DISCONNECTING:
            case A2DP_STATE_DISCONNECTED:
            {
                /* Reject media connection */
                AvDiscoverResRejSend(ind->connectionId,
                                     ind->tLabel,
                                     CSR_BT_RESULT_CODE_A2DP_BAD_STATE);
            }
            return;

            default:
                appA2dpError(theInst, CSR_BT_AV_DISCOVER_IND, ind);
                return;
        }
    }
}

static void appA2dpHandleAvGetCapabilitiesInd(CsrBtAvGetCapabilitiesInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvGetCapabilitiesInd(%p): acpSeid(%d)", theInst, ind->acpSeid);

    if (theInst != NULL)
    {
        uint8 sepIndex = appA2dpGetSepIndexBySeid(theInst, ind->acpSeid);
        assert(theInst->a2dp.device_id == ind->connectionId);

        theInst->a2dp.tLabel = ind->tLabel;

        if (sepIndex == A2DP_SEP_INDEX_INVALID)
        {
            appA2dpIssueGetCapabilitiesRes(theInst,
                                           CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID,
                                           sepIndex,
                                           ind->type,
                                           0);
            if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTING_MEDIA_REMOTE)
            {
                appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
            }
        }
        else
        {
            appA2dpIssueGetCapabilitiesRes(theInst,
                                           CSR_BT_AV_ACCEPT,
                                           sepIndex,
                                           ind->type,
                                           (ind->type == CSR_BT_AV_GET_CAPABILITIES_IND ? CSR_BT_AV_SC_DELAY_REPORTING : 0));
            if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTED_SIGNALLING)
            {
                appA2dpSetState(theInst, A2DP_STATE_CONNECTING_MEDIA_REMOTE);
            }
        }
    }
}

static void appA2dpHandleAvGetConfigInd(CsrBtAvGetConfigurationInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvGetConfigInd(%p) sHandle(%d)", (void *)theInst, ind->shandle);

    if (theInst != NULL)
    {
        uint8 sepIndex = appA2dpGetSepIndexBySeid(theInst, theInst->a2dp.current_seid);

        if (sepIndex == A2DP_SEP_INDEX_INVALID)
        {
            AvGetConfigResRejSend(ind->shandle,
                                  ind->tLabel,
                                  CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID);
        }
        else
        {
            uint8 *configured_caps;
            uint16 size_configured_caps;
            uint8 *stored_caps = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);

            size_configured_caps = appA2dpBlockGetSize(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
            configured_caps = (uint8 *)PanicUnlessMalloc(size_configured_caps);

            PanicNull(stored_caps);
            memmove(configured_caps, stored_caps, size_configured_caps);
            AvGetConfigResAcpSend(ind->shandle, ind->tLabel, size_configured_caps, configured_caps);
        }
    }
}

static void appA2dpHandleAvSetConfigInd(CsrBtAvSetConfigurationInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvSetConfigInd(%p): shandle(%d)", theInst, ind->shandle);

    if (theInst != NULL)
    {
        uint8 sepIndex = appA2dpGetSepIndexBySeid(theInst, ind->acpSeid);
        CsrBtAvResult result = CSR_BT_AV_ACCEPT;

        assert(theInst->a2dp.device_id == ind->connectionId);

        if (sepIndex == A2DP_SEP_INDEX_INVALID)
        {
            AvSetConfigResRejSend(ind->shandle,
                                       ind->tLabel,
                                       CSR_BT_RESULT_CODE_A2DP_BAD_ACP_SEID,
                                       *ind->servCapData);
			if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTING_MEDIA_REMOTE)
			{
				appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);				
			}
        }
        else
        {
            uint8 errorCat, errorCode;
            uint8 unsupportedService;
            const uint8 *codec_data = NULL;
            a2dpSepDataType *sepPtr = (a2dpSepDataType *)PanicNull(appA2dpBlockGetIndexed(theInst,
                                                                      DATA_BLOCK_SEP_LIST,
                                                                      sepIndex));
            const a2dpSepConfigType *sepConfig = sepPtr->sep_config;
            CsrBtAvRole local_role = theInst->av_callbacks->GetA2dpLocalRole();
            CsrBtAvSep remote_sepType = (local_role ==  CSR_BT_AV_AUDIO_SINK)? CSR_BT_AV_SOURCE : CSR_BT_AV_SINK;

            if (sepConfig->sep_type == remote_sepType)
            {   /* Remote device is attempting to configure a corresponding peer SEP while we are currently configuring */
                result = CSR_BT_RESULT_CODE_A2DP_BAD_STATE;
            }
            else if (sepPtr->in_use)
            {   /* SEP is already in use - reject (service capabilities were not the problem)*/
                result = CSR_BT_RESULT_CODE_A2DP_SEP_IN_USE;
            }
            else if (!appA2dpValidateServiceCaps(ind->servCapData,
                                                 ind->servCapLen,
                                                 FALSE,
                                                 FALSE,
                                                 FALSE,
                                                 &errorCat,
                                                 &errorCode))
            {   /* bad caps - reject */
                result = errorCode;
            }
            else if (!appA2dpAreServicesCategoriesCompatible(sepConfig->caps,
                                                          sepConfig->size_caps,
                                                          ind->servCapData,
                                                          ind->servCapLen,
                                                          &unsupportedService))
            {   /* Check that configuration only asks for services the local SEP supports set config does not match our caps - reject */
                result = CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION;
            }
            else if ((codec_data = appA2dpFindMatchingCodecSpecificInformation(sepConfig->caps,
                                                                 ind->servCapData,
                                                                 0,&errorCode)) == NULL)
            {   /*  Check the codec specific attributes are compatible set config does not match our caps - reject */
                DEBUG_LOG("appA2dpHandleAvSetConfigInd: invalid codec type ");
                result = errorCode;
            }
            else if (!appIsAptxAdConfigSupportedForSrcRole(theInst, (uint8 *) codec_data))
            {
                result = CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION;
                DEBUG_LOG("appA2dpHandleAvSetConfigInd: invalid aptX-AD codec config for source");
            }
            else
            {
                uint8 *configured_caps;
                uint16 size_configured_caps;

                DEBUG_LOG("appA2dpHandleAvSetConfigInd: Configuration Accepted on Seid(%d)", ind->acpSeid);

                if (appA2dpGetState(theInst) != A2DP_STATE_CONNECTING_MEDIA_REMOTE)
                {
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTING_MEDIA_REMOTE);
                }

                a2dpProfile_StoreSeid(theInst, ind->acpSeid);

                theInst->a2dp.stream_id = ind->shandle;

                /* Store local SEID */
                theInst->a2dp.local_sep.seid = sepConfig->seid;
                theInst->a2dp.local_sep.role = sepConfig->sep_type;
                theInst->a2dp.local_sep.codec = codec_data[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];

                /* Mark this SEP as in use */
                sepPtr->in_use = A2DP_SEP_IS_IN_USE;

                /* Store remote SEID */
                theInst->a2dp.remote_sep.seid = ind->intSeid;
                theInst->a2dp.remote_sep.role = (sepConfig->sep_type == CSR_BT_AV_SINK) ? CSR_BT_AV_SOURCE:CSR_BT_AV_SINK;  /* Remote role must be opposite of our own */
                theInst->a2dp.remote_sep.codec = codec_data[A2DP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];

                /* Set the index to the current SEID data */
                appA2dpBlockSetCurrent(theInst, DATA_BLOCK_SEP_LIST, sepIndex);
                appA2dpUpdateConfiguredServiceCaps(theInst, sepIndex, ind->servCapData, ind->servCapLen);
                /* Send accept response to remote */
                AvSetConfigResAcpSend(ind->shandle, ind->tLabel);

                configured_caps      = appA2dpBlockGetBase(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
                size_configured_caps = appA2dpBlockGetSize(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);

                if (configured_caps != NULL &&
                    appA2dpIsServiceSupported((uint8)CSR_BT_AV_SC_DELAY_REPORTING,
                                              configured_caps,
                                              size_configured_caps) &&
                    sepConfig->sep_type == CSR_BT_AV_SINK)
                {   /* As we are operating as a sink, issue Av Sync delay to source */
                    uint16 delay;

                    if (appA2dpIsSeidTwsSink(theInst->a2dp.local_sep.seid))
                    {
                        delay = TWS_SLAVE_LATENCY_MS * 10;
                    }
                    else
                    {
                        delay = Kymera_LatencyManagerGetLatencyForSeidInUs(theInst->a2dp.local_sep.seid) / 100;
                    }
                    /* Send Open request after receiving Delay Report confirmation */
                    AvDelayReportReqSend(delay, ind->shandle, ind->tLabel);
                }

                return;
            }

            AvSetConfigResRejSend(ind->shandle,
                                  ind->tLabel,
                                  result,
                                  *ind->servCapData);
			if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTING_MEDIA_REMOTE)
			{
				appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);				
			}
        }
    }
}

static void appA2dpHandleAvDiscoverCfm(CsrBtAvDiscoverCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvDiscoverCfm(%p): result(0x%04x) supplier(0x%04x)",
                theInst, cfm->avResultCode, cfm->avResultSupplier);

    if(theInst != NULL)
    {
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            {
                assert(theInst->a2dp.device_id == cfm->connectionId);
                if ((cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                     cfm->avResultSupplier == CSR_BT_SUPPLIER_AV) &&
                    (appA2dpProcessDiscoverRes(theInst, cfm)))
                {
                    appA2dpIssueGetCapabilitiesReq(theInst, cfm->tLabel);
                }
                else
                {
                    appA2dpAbortStreamConnect(theInst);
                }
           }
           break;

           default:
               /* Just ignore */
               break;
       }
    }
}

static void appA2dpHandleAvGetCapabilitiesCfm(CsrBtAvGetCapabilitiesCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvGetCapabilitiesCfm(%p): result(0x%04x) supplier(0x%04x)",
               (void *)theInst, cfm->avResultCode, cfm->avResultSupplier);

    if (theInst != NULL)
    {
        bool capsValidated = TRUE;
        assert(theInst->a2dp.device_id == cfm->connectionId);

        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            {
                if ((cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                     cfm->avResultSupplier == CSR_BT_SUPPLIER_AV &&
                     appA2dpProcessGetCapsRes(theInst, cfm)) &&
                    (appA2dpDoesCodecMatch(&theInst->a2dp.local_sep, &theInst->a2dp.remote_sep)))
                {
                    /* Try to config same, if failed, continue with next seid */
                    if (appA2dpIssueSetConfigurationReq(theInst, cfm->tLabel) == FALSE)
                    {
                        capsValidated = FALSE;
                    }
                }
                else
                {
                    capsValidated = FALSE;
                }
                break;
            }

            default:
                /* Ignore */
                break;
       }

        if (capsValidated == FALSE)
        {
            /* Continue getting capabilities for next seid, if can't, then
             * go to signalling state and generate UI error */
            if (!appA2dpContinueStreamConnect(theInst, cfm->tLabel))
            {
                appA2dpAbortStreamConnect(theInst);
            }
        }
    }
}

static void appA2dpHandleAvSetConfigCfm(CsrBtAvSetConfigurationCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvSetConfigCfm(%p): result(0x%04x) supplier(0x%04x)",
                theInst, cfm->avResultCode, cfm->avResultSupplier);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.device_id == cfm->connectionId);

        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            {
                if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                    cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                {
                    /* Mark the current SEP as in use */
                    ((a2dpSepDataType *)PanicNull(appA2dpBlockGetCurrent(theInst, DATA_BLOCK_SEP_LIST)))->in_use = A2DP_SEP_IS_IN_USE;
                    /* Store stream handle */
                    theInst->a2dp.stream_id = cfm->shandle;
                    /* Set locally configured flag */
                    theInst->a2dp.bitfields.local_configured = TRUE;
                    /* Set Configuration was successful, try to open the channel */
                    appA2dpIssueOpenReq(theInst, cfm->tLabel);
                }
                else
                {
                    if (!appA2dpContinueStreamConnect(theInst, cfm->tLabel))
                    {
                        appA2dpAbortStreamConnect(theInst);
                    }
                }

                break;
            }

            case A2DP_STATE_CONNECTING_MEDIA_REMOTE:
            {
                 /* This is crossover scenario, just ignore and let it go */
                 break;
            }

            default:
                /* Ignore */
                return;
        }
    }
}

static void appA2dpHandleAvStreamMtuSizeInd(const CsrBtAvStreamMtuSizeInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvStreamMtuSizeInd(%p) remoteMtuSize(%d)",
              (void *)theInst, ind->remoteMtuSize);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == ind->shandle);

        theInst->a2dp.mBtConnId = ind->btConnId; /* Saving media connection id */
        theInst->a2dp.media_sink = StreamL2capSink((uint16)(CSR_BT_CONN_ID_GET_MASK & ind->btConnId));
        theInst->a2dp.mtu = ind->remoteMtuSize;

        /* Kymera needs to have right MTU for media streaming,
         * that we received here, now we can changes the state accordingly */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTING_MEDIA_LOCAL:
            {
                avA2dpState next_state = A2DP_STATE_CONNECTED_MEDIA_SUSPENDED;

                if (!theInst->a2dp.suspend_state)
                {
                    next_state = A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC;
                }
                appAvSendStatusMessage(AV_A2DP_MEDIA_CONNECTED, NULL, 0);

                appA2dpSetState(theInst, next_state);
            }
            break;

            case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
            {
                if (theInst->a2dp.current_seid == AV_SEID_APTX_ADAPTIVE_SRC &&
                    theInst->av_callbacks->GetA2dpLocalRole() == CSR_BT_AV_AUDIO_SOURCE)
                {
                    /* We are the A2DP source device, and the remote sink initiated an
                       aptX Adaptive media channel. Disconnect it, and reconnect our own
                       media channel with the same SEID, so that the source can set the
                       configuration and thus have complete control over sample rate
                       selection and other aptx adaptive parameters. This is necessary
                       because some older sinks do not parse the capabilities correctly,
                       and end up selecting the wrong sample rate / aptX features. Also
                       the source may want to choose a different sample rate anyway,
                       based on its current audio input (e.g. USB sample rate). */
                    DEBUG_LOG_INFO("appA2dpHandleAvStreamMtuSizeInd: Remote initiated aptX Adaptive, restarting media channel");

                    theInst->a2dp.current_seid = AV_SEID_INVALID;

                    A2dpProfile_SendMediaConnectReq(theInst, AV_SEID_INVALID, 0, FALSE);

                    appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING_MEDIA);
                }
                else
                {
                    appAvSendStatusMessage(AV_A2DP_MEDIA_CONNECTED, NULL, 0);
                    /* Remote initiate media channel defaults to suspended */
                    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDED);
                }
           }
           break;

            default:
                /* ignore */
                break;
        }
    }
}

static void appA2dpHandleAvCloseCfm(const CsrBtAvCloseCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvCloseCfm(%p): result(0x%04x) supplier(0x%04x) shandle(%u)",
                theInst, cfm->avResultCode, cfm->avResultSupplier, cfm->shandle);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == cfm->shandle);

        /* Handle different states */
        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
            case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
            case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
            case A2DP_STATE_DISCONNECTING_MEDIA:
            {
                if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
                    cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
                {
                    /* Mark the current SEP as no longer being in use */
                    ((a2dpSepDataType *)PanicNull(appA2dpBlockGetCurrent(theInst, DATA_BLOCK_SEP_LIST)))->in_use = A2DP_SEP_IS_AVAILABLE;

                    /* Reset the local and remote SEIDs */
                    appA2dpBlockSetCurrent(theInst, DATA_BLOCK_SEP_LIST, 0);

                    theInst->a2dp.remote_sep.codec = A2DP_MEDIA_CODEC_UNDEFINED;
                    theInst->a2dp.local_sep.codec  = A2DP_MEDIA_CODEC_UNDEFINED;

                    /* Remove stream specific data */
                    appA2dpBlockRemove(theInst, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
                    appA2dpBlockRemove(theInst, DATA_BLOCK_DISCOVERED_REMOTE_SEIDS);

                    appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
                }
                else
                {   /* Don't take no for an answer - if our request is rejected then abort to force a close */
                    appA2dpAbortStreamConnect(theInst);
                }
            }
            return;

            default:
                appA2dpError(theInst, CSR_BT_AV_CLOSE_CFM, cfm);
                return;
        }
    }
}

/*! \brief Handle Delay Report Confirmation.
*/
static void appA2dpHandleDelayReportCfm(const CsrBtAvDelayReportCfm* cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleDelayReportCfm(%p): result(0x%04x) supplier(0x%04x) sHandle(%u)",
                theInst, cfm->avResultCode, cfm->avResultSupplier, cfm->shandle);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == cfm->shandle);

        /* Send Open Req only if local device has configured the endpoints */
        if ((cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
             cfm->avResultSupplier == CSR_BT_SUPPLIER_AV) &&
            (theInst->a2dp.bitfields.local_configured == TRUE))
        {
            /* Configure large MTU setting (if applicable) for the media channel */
            if (ConManagerGetQhsConnectStatus(&theInst->bd_addr) && appA2dpGetAptxAdQ2qMode(theInst))
            {
                DEBUG_LOG("appA2dpHandleDelayReportCfm setting large MTU");
                AvUseLargeMtu(theInst->a2dp.device_id, TRUE);
            }
            /* Delay report was sent in order to Open Streaming channel */
            AvOpenReqSend(theInst->a2dp.stream_id, cfm->tLabel);
        }
    }
}

/*! \brief Handle Abort Indication.
*/
static void appA2dpHandleAbortInd(const CsrBtAvAbortInd* ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAbortInd(%p): sHandle(%d)", (void *)theInst, ind->shandle);

    if (theInst != NULL)
    {
        assert(theInst->a2dp.stream_id == ind->shandle);

        /* Send Abort Response to remote */
        AvAbortResSend(ind->shandle, ind->tLabel);

        /* Handle different states */
        if (appA2dpGetState(theInst) == A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC ||
            appA2dpGetState(theInst) == A2DP_STATE_CONNECTING_MEDIA_REMOTE)
        {
            appA2dpSetState(theInst, A2DP_STATE_DISCONNECTING);
        }
        else if(appA2dpIsConnectedMedia(theInst))
        {
            appA2dpStreamReset(theInst);
            appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
            A2dpProfile_sendErrorIndication();
        }
    }
}

/*! \brief Handle Abort Confirmation.
*/
static void appA2dpHandleAbortCfm(const CsrBtAvAbortCfm* cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAbortCfm(%p): stream_id(%d)", (void *)theInst, cfm->shandle);

    if (theInst != NULL)
    {
        appA2dpStreamReset(theInst);
        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
        A2dpProfile_sendErrorIndication();
    }
}

static void appA2dpHandleAvReconfigureCfm(CsrBtAvReconfigureCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->shandle, FALSE, DEVICE_PROFILE_A2DP);

    DEBUG_LOG("appA2dpHandleAvReconfigureCfm(%p): result(0x%04x) supplier(0x%04x) sHandle(%u)",
                theInst, cfm->avResultCode, cfm->avResultSupplier, cfm->shandle);

    if (cfm->avResultCode == CSR_BT_RESULT_CODE_AV_SUCCESS &&
        cfm->avResultSupplier == CSR_BT_SUPPLIER_AV)
    {
        a2dp_codec_settings *codec_settings = appA2dpGetCodecSettings(theInst);

        if (codec_settings != NULL)
        {
            if (codec_settings->seid == AV_SEID_APTX_ADAPTIVE_SRC)
            {
                DEBUG_LOG_INFO("appA2dpHandleAvReconfigureCfm: Reconfigured aptX Adaptive media channel to %lukHz", codec_settings->rate / 1000);

                /* Clear the locally-set suspend reason, now reconfiguration is
                   complete. This will resume media if it was playing before
                   reconfiguration, but keep it suspended if it wasn't, as the
                   "remote" reason would still be set in that case. */
                MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);
                message->reason = AV_SUSPEND_REASON_LOCAL;
                MessageSendConditionally(&theInst->av_task,
                                         AV_INTERNAL_A2DP_RESUME_MEDIA_REQ,
                                         message, &appA2dpGetLock(theInst));
            }

            free(codec_settings);
        }

        switch (appA2dpGetState(theInst))
        {
            case A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING:
                appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDED);
            break;

            case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            break;

            default:
                appA2dpError(theInst, CSR_BT_AV_RECONFIGURE_CFM, cfm);
            break;
        }
    }
    else
    {
        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_SIGNALLING);
    }
}

/* Handle A2DP library messages */
void appA2dpHandleAvLibMessage(Message message)
{
    CsrBtAvPrim *primType = (CsrBtAvPrim *)message;
    
    DEBUG_LOG("appA2dpHandleAvLibMessage, MESSAGE:CsrBtAvPrim:0x%04X", *primType);

    switch(*primType)
    {
        case CSR_BT_AV_ACTIVATE_CFM:
        {
            appA2dpHandleAvActivateCfm((CsrBtAvActivateCfm *)message);
        }
        break;

        case CSR_BT_AV_CONNECT_IND:
        {
            appA2dpHandleAvConnectInd((CsrBtAvConnectInd *)message);
        }
        break;

        case CSR_BT_AV_CONNECT_CFM:
        {
            appA2dpHandleAvConnectCfm((CsrBtAvConnectCfm *)message);
        }
        break;

        case CSR_BT_AV_DISCONNECT_IND:
        {
            appA2dpHandleAvDisconnectInd((CsrBtAvDisconnectInd *)message);
        }
        break;

        case CSR_BT_AV_OPEN_IND:
        {
            appA2dpHandleAvOpenInd((CsrBtAvOpenInd *)message);
        }
        break;

        case CSR_BT_AV_OPEN_CFM:
        {
            appA2dpHandleAvOpenCfm((CsrBtAvOpenCfm *)message);
        }
        break;

        case CSR_BT_AV_START_IND:
        {
            appA2dpHandleAvStartInd((CsrBtAvStartInd *)message);
        }
        break;

        case CSR_BT_AV_START_CFM:
        {
            appA2dpHandleAvStartCfm((CsrBtAvStartCfm *)message);
        }
        break;

        case CSR_BT_AV_SUSPEND_IND:
        {
            appA2dpHandleAvSuspendInd((CsrBtAvSuspendInd *)message);
        }
        break;

        case CSR_BT_AV_SUSPEND_CFM:
        {
            appA2dpHandleAvSuspendCfm((CsrBtAvSuspendCfm *)message);
        }
        break;

        case CSR_BT_AV_CLOSE_IND:
        {
            appA2dpHandleAvCloseInd((CsrBtAvCloseInd *)message);
        }
        break;

        case CSR_BT_AV_STREAM_MTU_SIZE_IND:
        {
            appA2dpHandleAvStreamMtuSizeInd((CsrBtAvStreamMtuSizeInd *)message);
        }
        break;

        case CSR_BT_AV_DELAY_REPORT_IND:
        {
            appA2dpHandleAvDelayReportInd((CsrBtAvDelayReportInd *)message);
        }
        break;

        case CSR_BT_AV_RECONFIGURE_IND:
        {
            appA2dpHandleAvReconfigureInd((CsrBtAvReconfigureInd *)message);
        }
        break;

        case CSR_BT_AV_DISCOVER_IND:
        {
            appA2dpHandleAvDiscoverInd((CsrBtAvDiscoverInd *)message);
        }
        break;

        case CSR_BT_AV_DISCOVER_CFM:
        {
            appA2dpHandleAvDiscoverCfm((CsrBtAvDiscoverCfm *)message);
        }
        break;

        case CSR_BT_AV_GET_CAPABILITIES_CFM:
        {
            appA2dpHandleAvGetCapabilitiesCfm((CsrBtAvGetCapabilitiesCfm *)message);
        }
        break;

        case CSR_BT_AV_GET_ALL_CAPABILITIES_IND:
        case CSR_BT_AV_GET_CAPABILITIES_IND:
        {
            appA2dpHandleAvGetCapabilitiesInd((CsrBtAvGetCapabilitiesInd *)message);
        }
        break;

        case CSR_BT_AV_GET_CONFIGURATION_IND:
        {
            appA2dpHandleAvGetConfigInd((CsrBtAvGetConfigurationInd *)message);
        }
        break;

        case CSR_BT_AV_SET_CONFIGURATION_IND:
        {
            appA2dpHandleAvSetConfigInd((CsrBtAvSetConfigurationInd *)message);
        }
        break;

        case CSR_BT_AV_SET_CONFIGURATION_CFM:
        {
            appA2dpHandleAvSetConfigCfm((CsrBtAvSetConfigurationCfm *)message);
        }
        break;

        case CSR_BT_AV_CLOSE_CFM:
        {
            appA2dpHandleAvCloseCfm((CsrBtAvCloseCfm *)message);
        }
        break;

        case CSR_BT_AV_DELAY_REPORT_CFM:
        {
            appA2dpHandleDelayReportCfm((CsrBtAvDelayReportCfm*)message);
        }
        break;

        case CSR_BT_AV_ABORT_IND:
        {
            appA2dpHandleAbortInd((CsrBtAvAbortInd*)message);
        }
        break;

        case CSR_BT_AV_ABORT_CFM:
        {
            appA2dpHandleAbortCfm((CsrBtAvAbortCfm*)message);
        }
        break;

        case CSR_BT_AV_RECONFIGURE_CFM:
        {
            appA2dpHandleAvReconfigureCfm((CsrBtAvReconfigureCfm *)message);
        }
        break;

        default:
            break;
    }

    AvFreeUpstreamMessageContents((void *)message);
}

bool A2dpProfile_IsMediaSourceConnected(avInstanceTaskData *av_instance)
{
    return (av_instance && appA2dpIsConnectedMedia(av_instance) && appA2dpIsSourceCodec(av_instance));
}

bool A2dpProfile_IsMediaSourceStreaming(avInstanceTaskData *av_instance)
{
    return (av_instance && appA2dpIsStreaming(av_instance) && appA2dpIsSourceCodec(av_instance));
}

void A2dpProfile_ResumeMedia(avInstanceTaskData *av_instance)
{
    MessageCancelAll(&av_instance->av_task, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);
    MessageCancelAll(&av_instance->av_task, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);

    if (av_instance != NULL)
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);

        /* Send message to AV instance:
         * We would like to clear any/all suspend reason(s) and resume immediately */
        message->reason = AV_SUSPEND_REASON_ALL;
        MessageSendConditionally(&av_instance->av_task, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ,
                                message, &appA2dpGetLock(av_instance));
    }
}

void A2dpProfile_SuspendMedia(avInstanceTaskData *av_instance)
{
    MessageCancelAll(&av_instance->av_task, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);
    MessageCancelAll(&av_instance->av_task, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);

    if (av_instance != NULL)
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);

        /* Send message to AV instance:
         * we use reason REMOTE here to allow the remote device
         * to resume media anytime and so clear this flag */
        message->reason = AV_SUSPEND_REASON_REMOTE;
        MessageSendConditionally(&av_instance->av_task, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ,
                                message, &appA2dpGetLock(av_instance));
    }
}

static bool appA2dpReconfigureCodecIfRequired(avInstanceTaskData *av_instance)
{
    bool need_to_reconfigure = FALSE;
    uint8 *service_caps;
    uint8 service_caps_size;

    DEBUG_LOG("appA2dpReconfigureCodecIfRequired");

    if (!A2dpProfile_IsMediaSourceConnected(av_instance))
    {
        DEBUG_LOG("appA2dpReconfigureCodecIfRequired, no media channel to reconfigure");
        return need_to_reconfigure;
    }

    if (av_instance->a2dp.current_seid != AV_SEID_APTX_ADAPTIVE_SRC)
    {
        DEBUG_LOG("appA2dpReconfigureCodecIfRequired, reconfiguration not supported"
                  " for current codec (seid 0x%x)", av_instance->a2dp.current_seid);
        return need_to_reconfigure;
    }

    service_caps = appA2dpBlockGetBase(av_instance, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);
    service_caps_size = appA2dpBlockGetSize(av_instance, DATA_BLOCK_CONFIGURED_SERVICE_CAPS);

    if (service_caps != NULL)
    {
        uint8 *configured_media_caps = service_caps;
        uint16 configured_media_caps_size = service_caps_size;

        appA2dpFindServiceCategory(AVDTP_SERVICE_MEDIA_CODEC,
                                   (const uint8**)&configured_media_caps, &configured_media_caps_size);

        if (configured_media_caps && configured_media_caps_size > aptx_ad_sample_rate_offset)
        {
            need_to_reconfigure = A2dpProfileAptxAdReselectSampleRate(av_instance, configured_media_caps);
        }

        if (need_to_reconfigure)
        {
            switch (appA2dpGetState(av_instance))
            {
                case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
                case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
                case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
                case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
                {
                    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);

                    /* Need to suspend media streaming first. Set reason to
                       "local" to prevent sink resuming us until we're ready. */
                    DEBUG_LOG_INFO("appA2dpReconfigureCodecIfRequired: Suspending media channel to allow codec reconfiguration");

                    message->reason = AV_SUSPEND_REASON_LOCAL;
                    MessageSendConditionally(&av_instance->av_task,
                                             AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ,
                                             message, &appA2dpGetLock(av_instance));
                }
                /* Fall through */
                case A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL:
                {
                    /* Schedule reconfigure for after suspend completed. */
                    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ);
                    uint8 *new_media_caps = PanicUnlessMalloc(configured_media_caps_size);

                    memmove(new_media_caps, configured_media_caps, configured_media_caps_size);
                    message->new_media_caps = new_media_caps;
                    message->new_media_caps_size = configured_media_caps_size;
                    MessageSendConditionally(&av_instance->av_task,
                                             AV_INTERNAL_A2DP_RECONFIGURE_MEDIA_REQ,
                                             message, &appA2dpGetLock(av_instance));
                }
                break;

                case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
                {
                    /* Already suspended, can reconfigure immediately. */
                    appA2dpInitiateReconfig(av_instance, configured_media_caps, configured_media_caps_size);
                }
                break;

                case A2DP_STATE_CONNECTED_MEDIA_RECONFIGURING:
                default:
                    DEBUG_LOG("appA2dpReconfigureCodecIfRequired, reconfigure not allowed in state 0x%x",
                              appA2dpGetState(av_instance));
                    break;
            }
        }
    }

    return need_to_reconfigure;
}

bool A2dpProfile_SetPreferredSampleRate(avInstanceTaskData *av_instance, uint32 rate)
{
    avA2dpPreferredSampleRate new_preference;

    if (!av_instance)
    {
        return FALSE;
    }

    switch (rate)
    {
        case SAMPLE_RATE_96000:
            new_preference = A2DP_PREFERRED_RATE_96K;
            break;

        case SAMPLE_RATE_48000:
            new_preference = A2DP_PREFERRED_RATE_48K;
            break;

        case SAMPLE_RATE_44100:
            new_preference = A2DP_PREFERRED_RATE_44K1;
            break;

        default:
            new_preference = A2DP_PREFERRED_RATE_NONE;
            break;
    }

    if (av_instance->a2dp.preferred_sample_rate == new_preference)
    {
        DEBUG_LOG_DEBUG("A2dpProfile_SetPreferredSampleRate: Preferred sample rate already set to"
                        " enum:avA2dpPreferredSampleRate:%d", new_preference);
        return FALSE;
    }
    av_instance->a2dp.preferred_sample_rate = new_preference;

    DEBUG_LOG_INFO("A2dpProfile_SetPreferredSampleRate: Set preferred sample rate enum:avA2dpPreferredSampleRate:%d",
                   av_instance->a2dp.preferred_sample_rate);

    return appA2dpReconfigureCodecIfRequired(av_instance);
}

uint32 A2dpProfile_GetPreferredSampleRate(avInstanceTaskData *av_instance)
{
    uint32 preferred_rate = 0;

    if (av_instance)
    {
        switch (av_instance->a2dp.preferred_sample_rate)
        {
            case A2DP_PREFERRED_RATE_96K:
                preferred_rate = SAMPLE_RATE_96000;
                break;

            case A2DP_PREFERRED_RATE_48K:
                preferred_rate = SAMPLE_RATE_48000;
                break;

            case A2DP_PREFERRED_RATE_44K1:
                preferred_rate = SAMPLE_RATE_44100;
                break;

            case A2DP_PREFERRED_RATE_NONE:
            default:
                break;
        }
    }

    return preferred_rate;
}


#else
static const int compiler_happy;
#endif
