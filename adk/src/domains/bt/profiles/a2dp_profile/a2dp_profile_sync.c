/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_profile_sync.c
\brief      Coordination between the Sink & Source (aka forwarding) A2DP roles.
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include <logging.h>
#include <message.h>
#include <panic.h>
#include <timestamp_event.h>

#include "av.h"
#ifdef USE_SYNERGY
#include <av_lib.h>
#include <connection_manager.h>
#include "a2dp_profile_codec_aptx_adaptive.h"
#endif /* USE_SYNERGY */
#include <av_instance.h>
#include "a2dp_profile.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_config.h"
#include <device.h>
#include <device_properties.h>

/*! \{
    Macros for diagnostic output that can be suppressed.
    Allows debug of this module at two levels. */
#define A2DP_SYNC_LOG        DEBUG_LOG
/*! \} */

/*! Code assertion that can be checked at run time        . This will cause a panic. */
#define assert(x) PanicFalse(x)

/*! Macro for simplifying creating messages */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*
    Functions that implement the audio_sync_interface_t for an a2dp profile
*/

/*
    Helpers for sending AV_INTERNAL_A2DP_* messages to an AV instance.
*/

/*
    Handlers for AUDIO_SYNC_... messages
*/

static void appA2dpSyncHandleA2dpSyncConnectResponse(avInstanceTaskData *theInst,
                                                     const AUDIO_SYNC_CONNECT_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    if (((res->sync_id + 1) & 0xff) != theInst->a2dp.sync_counter)
    {
        /* This means whilst waiting for a sync response from the other instance,
        something else triggered the instance to exit the _SYNC state. So this sync
        response is late, and now irrelevant. */
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse(%p) late state(0x%x) sync_id(%d) sync_count(%d)",
            theInst, local_state, res->sync_id, theInst->a2dp.sync_counter);
        return;
    }

    /* This will cancel any responses sent 'later' to catch the other instance
       not responding in time. */
    PanicFalse(MessageCancelAll(&theInst->av_task, AUDIO_SYNC_CONNECT_RES) <= 1);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse(%p) state(0x%x) sync_id(%d)",
               theInst, local_state, res->sync_id);

    switch (local_state)
    {
    case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
        /* Accept media connection */
#ifdef USE_SYNERGY
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse accepting A2dpMediaOpen device_id %u",
                      theInst->a2dp.device_id);
        /* Configure large MTU setting (if applicable) for the media channel */
        if (ConManagerGetQhsConnectStatus(&theInst->bd_addr) && appA2dpGetAptxAdQ2qMode(theInst))
        {
            A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse setting large MTU");
            AvUseLargeMtu(theInst->a2dp.device_id, TRUE);
        }
        AvOpenResAcpSend(theInst->a2dp.stream_id, theInst->a2dp.tLabel);
#else
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse accepting A2dpMediaOpen device_id %u", theInst->a2dp.device_id);
        PanicFalse(A2dpMediaOpenResponse(theInst->a2dp.device_id, TRUE));
#endif
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_OPEN_CFM. */
        break;

    default:
        appA2dpError(theInst, AUDIO_SYNC_CONNECT_RES, NULL);
        break;
    }
    theInst->a2dp.sync_counter++;
}

static void appA2dpSyncHandleA2dpSyncPrepareResponse(avInstanceTaskData *theInst,
                                                     const AUDIO_SYNC_PREPARE_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    if (((res->sync_id + 1) & 0xff) != theInst->a2dp.sync_counter)
    {
        if(!a2dpIsSyncFlagSet(theInst, A2DP_SYNC_PREPARE_RESPONSE_PENDING) && !a2dpIsSyncFlagSet(theInst, A2DP_SYNC_MEDIA_START_PENDING))
        {
            /* This means whilst waiting for a sync response from the other instance,
            something else triggered the instance to exit the _SYNC state. So this sync
            response is late, but we still need to complete the sequence */
            A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncPrepareResponse(%p) late state(enum:avA2dpState:%d) sync_id(%d) sync_count(%d) reason (enum:audio_sync_reason_t:%d)",
                           theInst, local_state, res->sync_id, theInst->a2dp.sync_counter, res->reason);
            
            return;
        }
    }
    
    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncPrepareResponse(%p) state(enum:avA2dpState:%d) sync_id(%d) reason (enum:audio_sync_reason_t:%d)",
                   theInst, local_state, res->sync_id, res->reason);

    switch (local_state)
    {
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
        {
            if(res->reason != audio_sync_success)
            {
                a2dpProfileSync_SendMediaStartResponse(theInst);
            }
            /* Send response if audio sync is required (earbud) and codec is aptX adaptive*/
            else if(theInst->a2dp.local_sep.seid == AV_SEID_APTX_ADAPTIVE_SNK)  
            {
                /* To reduce the response time earbud send A2DP_START_RSP when sync prepare finishes (L2CAP mirroring connected), instead of wait until sync activated*/
                DEBUG_LOG("Early respond for aptX adaptive");
                a2dpProfileSync_SendMediaStartResponse(theInst);
            }
        }
        break;
        case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        {
            if(res->reason != audio_sync_success)
            {
                a2dpProfileSync_SendMediaStartRequest(theInst);

                /* If the A2DP instance is acting in the source role, without the benefit of
                 * the Audio Router to eventually trigger ACTIVATE_IND, then a2dp_profile
                 * should set the media start audio sync flag to true */
                if (appA2dpIsSourceCodec(theInst) && res->reason == audio_sync_not_required)
                {
                    theInst->a2dp.bitfields.local_media_start_audio_sync_complete = TRUE;
                }
            }
        }
        break;
        
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        break;

        default:
            appA2dpError(theInst, AUDIO_SYNC_PREPARE_RES, NULL);
        break;
    }
    
    if(res->reason != audio_sync_timeout)
    {
        a2dpClearSyncFlag(theInst, A2DP_SYNC_PREPARE_RESPONSE_PENDING);

        /* Cancel any pending prepare-response timeout */
        MessageCancelAll(&theInst->av_task, AUDIO_SYNC_PREPARE_RES);

        /* in case of VA response, because of time criticality, A2DP chain could 
           be started even before response comes from VA. So, we need not set
           A2DP start lock as its already started */
        if((theInst->a2dp.source_state != source_state_disconnected) &&
           (theInst->a2dp.source_state != source_state_connected))
        {
            appA2dpSetAudioStartLockBit(theInst);
        }

        if(res->reason != audio_sync_rejected)
        {
            /* Don't set prepared flag on rejection so that prepare 
               stage is repeated before the source is routed */
            a2dpSetSyncFlag(theInst, A2DP_SYNC_PREPARED);
        }

        AvSendAudioConnectedStatusMessage(theInst, AV_A2DP_AUDIO_CONNECTED);
        theInst->a2dp.sync_counter++;
    }

#ifdef USE_SYNERGY
    /* This is done here in case of synergy for maintaining the order being followed in ADK
       of media player handling AV_A2DP_AUDIO_CONNECTED first and then moving to streaming/muted state. */
    if(local_state == A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC &&
       res->reason != audio_sync_success &&
       a2dpIsSyncFlagSet(theInst, A2DP_SYNC_MEDIA_START_PENDING))
    {
        a2dpProfileSync_HandleMediaStartConfirm(theInst);
    }
#endif /* USE_SYNERGY */
}


static void appA2dpSyncHandleA2dpSyncActivateResponse(avInstanceTaskData *theInst,
                                                      const AUDIO_SYNC_ACTIVATE_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    if (((res->sync_id + 1) & 0xff) != theInst->a2dp.sync_counter)
    {
        /* This means whilst waiting for a sync response from the other instance,
        something else triggered the instance to exit the _SYNC state. So this sync
        response is late, and now irrelevant. */
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateResponse(%p) late state(0x%x) sync_id(%d) sync_count(%d)",
                       theInst, local_state, res->sync_id, theInst->a2dp.sync_counter);
        return;
    }

    /* This will cancel any responses sent 'later' to catch the other instance
       not responding in time. */
    PanicFalse(MessageCancelAll(&theInst->av_task, AUDIO_SYNC_ACTIVATE_RES) <= 1);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateResponse(%p) state(0x%x) sync_id(%d)",
                   theInst, local_state, res->sync_id);
    /* Set the flag as received the AUDIO_SYNC_CONNECT_RES */
    theInst->a2dp.bitfields.local_media_start_audio_sync_complete = TRUE;
    DEBUG_LOG("appA2dpSyncHandleA2dpSyncActivateResponse: local_media_start_audio_sync_complete %d", theInst->a2dp.bitfields.local_media_start_audio_sync_complete);

    switch (local_state)
    {
    case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
    {
        /* Start streaming request */
        a2dpProfileSync_SendMediaStartRequest(theInst);
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_START_CFM. */
        break;
    }
    case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
    {
#ifndef USE_SYNERGY
        TimestampEvent(TIMESTAMP_EVENT_A2DP_START_RSP);
#endif
        if(theInst->a2dp.local_sep.seid != AV_SEID_APTX_ADAPTIVE_SNK)
            a2dpProfileSync_SendMediaStartResponse(theInst);

#ifdef USE_SYNERGY
        if(a2dpIsSyncFlagSet(theInst, A2DP_SYNC_MEDIA_START_PENDING))
        {
            a2dpProfileSync_HandleMediaStartConfirm(theInst);
        }
#else
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_START_CFM. */
#endif
        break;
    }

    case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
    case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
    case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        break;

    default:
        appA2dpError(theInst, AUDIO_SYNC_ACTIVATE_RES, NULL);
        break;
    }
    
    theInst->a2dp.sync_counter++;
}

static void appA2dpSyncHandleA2dpSyncDestroyResponse(avInstanceTaskData *theInst, const AUDIO_SYNC_DESTROY_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);
    
    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateResponse(%p) state(enum:avA2dpState:%d) sync_id(%d)", theInst, local_state, res->sync_id);
    
    /* Clear the sync lock so AV_INTERNAL_A2DP_DESTROY_REQ can be delivered */
    appA2dpClearAudioSyncLockBit(theInst);
}

/*! \brief Initialise a audio_sync_t instance for use with an a2dp profile instance */
void appA2dpSyncInitialise(avInstanceTaskData *theInst)
{
    /* No client registered initially with this AV instance. */
    memset(&theInst->a2dp.sync_if, 0, sizeof(audio_sync_t));
}

/*! \brief Get the audio_sync_state_t for a given avA2dpState. */
audio_sync_state_t appA2dpSyncGetAudioSyncState(avInstanceTaskData *theInst)
{
    audio_sync_state_t audio_state = AUDIO_SYNC_STATE_DISCONNECTED;
    
    avA2dpState a2dp_state = appA2dpGetState(theInst);

    DEBUG_LOG("appA2dpSyncGetAudioSyncState enum:avA2dpState:%d", a2dp_state);

    if (appA2dpIsStateConnectedMediaStreaming(a2dp_state))
    {
        if(theInst->a2dp.source_state == source_state_connected)
        {
            audio_state = AUDIO_SYNC_STATE_ACTIVE;
        }
        else
        {
            audio_state = AUDIO_SYNC_STATE_READY;
        }
    }
    else if (appA2dpIsStateConnectedMedia(a2dp_state))
    {
        audio_state = AUDIO_SYNC_STATE_CONNECTED;
    }

    return audio_state;
}

void appA2dpSyncRegister(avInstanceTaskData *theInst, const audio_sync_t *sync_if)
{
    DEBUG_LOG("appA2dpSyncRegister(%p)", (void *)theInst);

    theInst->a2dp.sync_if = *sync_if;

    /* Notify the current state to the synchronised instance. */
    a2dpProfileSync_SendStateIndication(theInst, appA2dpSyncGetAudioSyncState(theInst));
}

void appA2dpSyncUnregister(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appA2dpSyncUnregister theInst %p is_valid %d", theInst, appAvIsValidInst(theInst));

    if (appAvIsValidInst(theInst))
    {
        AudioSync_CancelQueuedMessages(&theInst->a2dp.sync_if);
        memset(&theInst->a2dp.sync_if, 0, sizeof(audio_sync_t));
    }
}

/*! \brief Handler function for all audio_sync_msg_t messages */
void appA2dpSyncHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message)
{
    switch (id)
    {
        case AUDIO_SYNC_CONNECT_RES:
            appA2dpSyncHandleA2dpSyncConnectResponse(theInst, (const AUDIO_SYNC_CONNECT_RES_T *)message);
        break;
        
        case AUDIO_SYNC_PREPARE_RES:
            appA2dpSyncHandleA2dpSyncPrepareResponse(theInst, (const AUDIO_SYNC_PREPARE_RES_T *)message);
        break;

        case AUDIO_SYNC_ACTIVATE_RES:
            appA2dpSyncHandleA2dpSyncActivateResponse(theInst, (const AUDIO_SYNC_ACTIVATE_RES_T *)message);
        break;
        
        case AUDIO_SYNC_DESTROY_RES:
            appA2dpSyncHandleA2dpSyncDestroyResponse(theInst, (const AUDIO_SYNC_DESTROY_RES_T *)message);
        break;

        default:
            A2DP_SYNC_LOG("appA2dpSyncHandleMessage unhandled msg id MESSAGE:0x%x", id);
        break;
    }
}

void a2dpProfileSync_SendConnectIndication(avInstanceTaskData *av_instance)
{
    Task av_task = &av_instance->av_task;
    uint8 sync_id = av_instance->a2dp.sync_counter++;
    struct audio_sync_t *sync_inst = &av_instance->a2dp.sync_if;
    
    AudioSync_ConnectIndication(sync_inst, av_task, Av_GetSourceForInstance(av_instance),
                                av_instance->a2dp.current_seid, sync_id);
}

void a2dpProfileSync_SendPrepareIndication(avInstanceTaskData *av_instance)
{
    Task av_task = &av_instance->av_task;
    uint8 sync_id = av_instance->a2dp.sync_counter++;
    struct audio_sync_t *sync_inst = &av_instance->a2dp.sync_if;
    
    MessageCancelAll(&av_instance->av_task, AUDIO_SYNC_PREPARE_RES);
    a2dpSetSyncFlag(av_instance, A2DP_SYNC_PREPARE_RESPONSE_PENDING);
    
    AudioSync_PrepareIndication(sync_inst, av_task, Av_GetSourceForInstance(av_instance),
                                av_instance->a2dp.current_seid, sync_id);
}

void a2dpProfileSync_SendActiveIndication(avInstanceTaskData *av_instance)
{
    Task av_task = &av_instance->av_task;
    uint8 sync_id = av_instance->a2dp.sync_counter++;
    struct audio_sync_t *sync_inst = &av_instance->a2dp.sync_if;
    
    /* Cancel any pending prepare response/timeout */
    MessageCancelAll(&av_instance->av_task, AUDIO_SYNC_PREPARE_RES);
    
    AudioSync_ActivateIndication(sync_inst, av_task, Av_GetSourceForInstance(av_instance),
                                 av_instance->a2dp.current_seid, sync_id);
}

void a2dpProfileSync_SendStateIndication(avInstanceTaskData* av_instance, audio_sync_state_t state)
{
    struct audio_sync_t *sync_inst = &av_instance->a2dp.sync_if;
    AudioSync_StateIndication(sync_inst, Av_GetSourceForInstance(av_instance), state, av_instance->a2dp.current_seid);
}

void a2dpProfileSync_SendDestroyIndication(avInstanceTaskData *av_instance)
{
    Task av_task = &av_instance->av_task;
    uint8 sync_id = av_instance->a2dp.sync_counter++;
    struct audio_sync_t *sync_inst = &av_instance->a2dp.sync_if;
    
    /* Do not allow this instance to be destroyed until response is received */
    appA2dpSetAudioSyncLockBit(av_instance);
    
    AudioSync_DestroyIndication(sync_inst, av_task, Av_GetSourceForInstance(av_instance), sync_id);
}

void a2dpProfileSync_SendMediaStartRequest(avInstanceTaskData* theInst)
{
    if(a2dpIsSyncFlagSet(theInst, A2DP_SYNC_MEDIA_START_PENDING))
    {
        A2DP_SYNC_LOG("a2dpProfileSync_SendMediaStartRequest: Sending Start Request...");
#ifdef USE_SYNERGY
        uint8 *list = PanicUnlessMalloc(1 * sizeof(uint8));

        *list = theInst->a2dp.stream_id;
        AvStartReqSend(A2DP_ASSIGN_TLABEL(theInst), list, 1);
#else
        PanicFalse(A2dpMediaStartRequest(theInst->a2dp.device_id, theInst->a2dp.stream_id));
#endif
        a2dpClearSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);
    }
}

void a2dpProfileSync_SendMediaStartResponse(avInstanceTaskData* theInst)
{
    if(a2dpIsSyncFlagSet(theInst, A2DP_SYNC_MEDIA_START_PENDING))
    {
#ifdef USE_SYNERGY
        a2dpProfileSync_HandleMediaStartResponse(theInst);
#else
        PanicFalse(A2dpMediaStartResponse(theInst->a2dp.device_id, theInst->a2dp.stream_id, TRUE));
        a2dpClearSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);
#endif
    }
}

#ifdef USE_SYNERGY
void a2dpProfileSync_HandleMediaStartConfirm(avInstanceTaskData* theInst)
{
    assert(theInst != NULL);

    /* Check if we should start or suspend streaming, this is done here
       as synergy doesn't send confirmation to the command
       CSR_BT_AV_START_RES. */
    if (theInst->a2dp.suspend_state != 0)
        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL);
    else
        appA2dpSetState(theInst, A2DP_STATE_CONNECTED_MEDIA_STREAMING);

    a2dpClearSyncFlag(theInst, A2DP_SYNC_MEDIA_START_PENDING);
}

void a2dpProfileSync_HandleMediaStartResponse(avInstanceTaskData* theInst)
{
    uint32 start_time;
    uint8 *list = PanicUnlessMalloc(1 * sizeof(uint8));

    DEBUG_LOG("a2dpProfileSync_HandleMediaStartResponse accepting A2dpMediaStart device_id %u", theInst->a2dp.device_id);
    TimestampEvent(TIMESTAMP_EVENT_A2DP_START_RSP);
    *list = theInst->a2dp.stream_id;
    AvStartResAcpSend(theInst->a2dp.tLabel, list, 1);

    /* There will not be Start Cfm received for above Start Response call.
     * To meet KPI requirement, we would consider Start Cfm at this moment
     * to calculate time-difference between Start Ind & Cfm */
    TimestampEvent(TIMESTAMP_EVENT_A2DP_START_CFM);
    start_time = TimestampEvent_Delta(TIMESTAMP_EVENT_A2DP_START_IND,
                                      TIMESTAMP_EVENT_A2DP_START_CFM);
    DEBUG_LOG("a2dpProfileSync_HandleMediaStartResponse: start_time(%d)", start_time);
}
#endif /* USE_SYNERGY */

#endif /* INCLUDE_AV */
