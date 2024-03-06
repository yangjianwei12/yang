/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file       avrcp_profile.c
\brief      AVRCP State Machine
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

/****************************************************************************
    Header files
*/
#include <avrcp.h>
#include <panic.h>
#include <connection.h>
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <link_policy.h>
#include <ps.h>
#include <string.h>
#include <stdlib.h>
#include <stream.h>
#include "avrcp_profile.h"
#include "avrcp_profile_config.h"
#include "avrcp_profile_metadata.h"
#include "avrcp_profile_browsing.h"

#include "adk_log.h"
#include "av.h"
#include "audio_sources_media_control_interface.h"
#include "audio_sources.h"
#include "volume_messages.h"
#include <connection_manager.h>
#include "avrcp_lib.h"
#include <av_instance.h>


/*! Macro for creating an AV message based on the message name */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);
/*! Macro for creating a variable length AV message based on the message name */
#define MAKE_AV_MESSAGE_WITH_LEN(TYPE, LEN) \
    TYPE##_T *message = PanicUnlessMalloc(sizeof(TYPE##_T) + (LEN) - 1);

/*! Macro for max no of incoming AVRCP connection. */
#define AVRCP_MAX_CONNECTION_INSTANCES    (2)

#define AUDIO_VOLUME_STEP_UP              (8)
#define AUDIO_VOLUME_STEP_DOWN            (-8)

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)

/* static function declaration */
static void avrcpProfile_HandleAbsoluteVolumeInd(avInstanceTaskData *theInst, uint8 conn_id, uint8 vol, uint32 msg_id, bool send_response, uint8 tlabel);
static void appAvrcpSetState(avInstanceTaskData *theInst, avAvrcpState avrcp_state);

static void appAvrcpFinishAvrcpPassthroughRequest(avInstanceTaskData *theInst, avrcp_status_code status)
{
    MAKE_AV_MESSAGE(AV_AVRCP_VENDOR_PASSTHROUGH_CFM);
    message->av_instance = theInst;
    message->status = status;
    message->opid = theInst->avrcp.vendor_opid;
    MessageSend(theInst->avrcp.vendor_task, AV_AVRCP_VENDOR_PASSTHROUGH_CFM, message);

    DEBUG_LOG("appAvrcpFinishAvrcpPassthroughRequest(%p), data %p, status:%d",
               (void *)theInst,
               (void *)theInst->avrcp.vendor_data,
               status);

    /* Free allocated memory */
    free(theInst->avrcp.vendor_data);
    theInst->avrcp.vendor_data = NULL;
}

static void appAvrcpSuppressAbsoluteVolume(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpSuppressAbsoluteVolume");
    theInst->avrcp.bitfields.suppress_absolute_volume = TRUE;
    MessageSendLater(&theInst->av_task, AV_INTERNAL_ALLOW_ABSOLUTE_VOLUME, NULL, 2000);
}

static void appAvrcpAllowAbsoluteVolume(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpAllowAbsoluteVolume");
    theInst->avrcp.bitfields.suppress_absolute_volume = FALSE;
}

/*! \brief Enter 'connecting local' state

    The AVRCP state machine has entered 'connecting' state, set the
    'connect busy' flag to serialise connect attempts.
*/
static void appAvrcpEnterConnectingLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpEnterConnectingLocal(%p)", (void *)theInst);
}

/*! \brief Exit 'connecting local' state

    The AVRCP state machine has exited 'connecting local' state, clear the
    'connect busy' flag to allow pending connection attempts to proceed.
*/
static void appAvrcpExitConnectingLocal(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpExitConnectingLocal(%p)", (void *)theInst);

    /* We have finished (successfully or not) attempting to connect, so
     * we can relinquish our lock on the ACL.  Bluestack will then close
     * the ACL when there are no more L2CAP connections */
    ConManagerReleaseAcl(&theInst->bd_addr);
}

/*! \brief Enter 'connecting remote' state
    The AVRCP state machine has entered 'connecting remote' state.
*/
static void appAvrcpEnterConnectingRemote(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpEnterConnectingRemote(%p)", (void *)theInst);
}

/*! \brief Exit 'connecting remote' state
    The AVRCP state machine has exited 'connecting remote' state.
*/
static void appAvrcpExitConnectingRemote(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpExitConnectingRemote(%p)", (void *)theInst);
}

/*! \brief Enter 'connected' state

    The AVRCP state machine has entered 'connected' state, this means
    that the AVRCP control channel has been established.

    Kick the link policy manager to make sure this link is configured correctly
    and to maintain the correct link topology.
*/
static void appAvrcpEnterConnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpEnterConnected(%p)", (void *)theInst);

    /* Suppress Avrcp Absolute volume only if it is enabled. */
    if(appConfigAvrcpAbsoluteVolumeSuppressionEnabled())
    {
        DEBUG_LOG("appAvrcpEnterConnected, Suppress Absolute Volume Request on AVRCP Connection");
        appAvrcpSuppressAbsoluteVolume(theInst);
    }

    appAvInstanceAvrcpConnected(theInst);
}

/*! \brief Exit 'connected' state

    The AVRCP state machine has exited 'connected' state, this means
    that the AVRCP control channel has closed.
*/
static void appAvrcpExitConnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpExitConnected(%p)", (void *)theInst);
}

/*! \brief Enter 'disconnecting' state

    The AVRCP state machine has entered 'disconnecting' state, this means
    we are about to disconnect the AVRCP connection, set the operation lock
    to block any other operations until the disconnection is completed.
*/
static void appAvrcpEnterDisconnecting(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpEnterDisconnecting(%p)", (void *)theInst);

    /* Close all connections */
    AvrcpDisconnectReqSend(theInst->avrcp.connectionId);
}

/*! \brief Exit 'disconnecting' state

    The AVRCP state machine has exit 'disconnecting' state, this means
    we have now disconnected the AVRCP connection, clear the operation lock
    so that any blocked operations can now proceed.
*/
static void appAvrcpExitDisconnecting(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpExitDisconnecting(%p)", (void *)theInst);
}

/*! \brief Enter 'disconnected' state

    The AVRCP state machine has entered 'disconnected' state, this means there
    is now no AVRCP connection.  Cancel any request for remote control commands and
    clear the AVRCP profile library instance pointer.  Normally we stay in this
    state until an AV_INTERNAL_AVRCP_DESTROY_REQ or AV_INTERNAL_AVRCP_CONNECT_REQ
    message is received.
*/
static void appAvrcpEnterDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpEnterDisconnected(%p)", (void *)theInst);

    /* We may need the AV instance for the cleanup of other modules, so cancel any pending destroy requests */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);

    /* Cancel in-progress vendor passthrough command */
    if (theInst->avrcp.vendor_data)
        appAvrcpFinishAvrcpPassthroughRequest(theInst, avrcp_fail);

    /* We're not going to get confirmation for any passthrough request, so clear lock */
    appAvrcpClearLock(theInst, APP_AVRCP_LOCK_PASSTHROUGH_REQ);

    /* Clear any queued AVRCP messages whose handling depends on the AVRCP library pointer, 
       since this pointer is about to be freed below.
       If/when this AV instance eventually gets destroyed, all other pending messages 
       will also be safely cancelled. */

    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REQ);
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND);
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ);
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ);
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_TOGGLE_REQ);
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ);

    /*! Client disconnect requests are not confirmed, so it is acceptable to
        cancel any outstanding disconnect requests when entering disconnected. */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DISCONNECT_REQ);

    /* Clear notification lock */
    theInst->avrcp.notification_lock = 0;

    /* Clear the connection ID here to be in sync with the Synergy library. */
    theInst->avrcp.connectionId = AV_CONN_ID_INVALID;
    theInst->avrcp.btConnId = 0;

    /* Send ourselves a destroy message so that any other messages waiting on the
       operation lock can be handled */
    MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ, NULL, &appAvrcpGetLock(theInst));
}

/*! \brief Exit 'disconnected' state

    The AVRCP state machine has exited 'disconnected' state, this normally
    happend when an AV_INTERNAL_AVRCP_CONNECT_REQ message is received.
    Make sure that any queued AV_INTERNAL_AVRCP_DESTROY_REQ are destroyed.
*/
static void appAvrcpExitDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpExitDisconnected(%p)", (void *)theInst);

    /* Cancel any internal connect/destroy messages */
    MessageCancelFirst(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);
    if (MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_REQ))
    {
        DEBUG_LOG("appAvrcpExitDisconnected(%p), cancelling deferred connect req", (void *)theInst);
    }
}

/*! \brief Set AVRCP state

    Called to change state.  Handles calling the state entry and exit functions.
*/
static void appAvrcpSetState(avInstanceTaskData *theInst, avAvrcpState avrcp_state)
{
    avAvrcpState avrcp_old_state = theInst->avrcp.state;
    DEBUG_LOG("appAvrcpSetState(%p) enum:avAvrcpState:%d -> enum:avAvrcpState:%d", (void *)theInst, avrcp_old_state, avrcp_state);

    /* Handle state exit functions */
    switch (avrcp_old_state)
    {
        case AVRCP_STATE_DISCONNECTED:
            appAvrcpExitDisconnected(theInst);
            break;
        case AVRCP_STATE_CONNECTING_LOCAL:
            appAvrcpExitConnectingLocal(theInst);
            break;
        case AVRCP_STATE_CONNECTING_REMOTE:
            appAvrcpExitConnectingRemote(theInst);
            break;
        case AVRCP_STATE_CONNECTED:
            appAvrcpExitConnected(theInst);
            break;
        case AVRCP_STATE_DISCONNECTING:
            appAvrcpExitDisconnecting(theInst);
            break;
        default:
            break;
    }

    /* Set new state */
    theInst->avrcp.state = avrcp_state;

    /* Update lock according to state */
    if (avrcp_state & AVRCP_STATE_LOCK)
        appAvrcpSetLock(theInst, APP_AVRCP_LOCK_STATE);
    else
        appAvrcpClearLock(theInst, APP_AVRCP_LOCK_STATE);

    /* Handle state entry functions */
    switch (avrcp_state)
    {
        case AVRCP_STATE_DISCONNECTED:
            appAvrcpEnterDisconnected(theInst);
            break;
        case AVRCP_STATE_CONNECTING_LOCAL:
            appAvrcpEnterConnectingLocal(theInst);
            break;
        case AVRCP_STATE_CONNECTING_REMOTE:
            appAvrcpEnterConnectingRemote(theInst);
            break;
        case AVRCP_STATE_CONNECTED:
            appAvrcpEnterConnected(theInst);
            break;
        case AVRCP_STATE_DISCONNECTING:
            appAvrcpEnterDisconnecting(theInst);
            break;
        default:
            break;
    }
}

/*! \brief Get the current AVRCP state of this AV instance

    \param[in] theAv    Instance to get the state of

    \returns Current state
*/
avAvrcpState appAvrcpGetState(avInstanceTaskData *theAv)
{
    PanicNull(theAv);
    return theAv->avrcp.state;
}

/*! \brief Handle AVRCP error

    Some error occurred in the AVRCP state machine, to avoid the state machine
    getting stuck, drop connection and move to 'disconnected' state.
*/
static void appAvrcpError(avInstanceTaskData *theInst, MessageId id, Message message)
{
    UNUSED(message); UNUSED(theInst); UNUSED(id);

#ifdef AV_DEBUG
    DEBUG_LOG("appAvrcpError(%p) state enum:avAvrcpState:%u, MESSAGE:0x%x", (void *)theInst, theInst->avrcp.state, id);
#else
    Panic();
#endif
}

/*! \brief Request AVRCP remote control

    Handle internal message to send passthrough command, only handle the request
    in the 'connected' state.

    If from_repeat_message is TRUE, this came from a
    AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ message.
*/
static void appAvrcpHandleInternalAvrcpRemoteRequest(avInstanceTaskData *theInst,
                                                     AV_INTERNAL_AVRCP_REMOTE_REQ_T *req,
                                                     bool from_repeat_message)
{
    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTED:
        {
            /* UI indication if remote control press */
            if (req->ui)
            {
                appAvSendUiMessageId(AV_REMOTE_CONTROL);
            }

            /* Set operation lock */
            appAvrcpSetLock(theInst, APP_AVRCP_LOCK_PASSTHROUGH_REQ);

            /* Store OPID so that we know what operation the AVRCP_PASSTHROUGH_CFM is for */
            theInst->avrcp.bitfields.op_id = req->op_id;
            theInst->avrcp.bitfields.op_state = req->state;
            theInst->avrcp.bitfields.op_repeat = from_repeat_message;

            /* Send remote control using main AV task */
            AvrcpCtPassThroughReqSend(&(AvGetTaskData()->task), theInst->avrcp.connectionId, req->op_id, req->state);
            DEBUG_LOG("appAvrcpHandleInternalAvrcpRemoteRequest(%p) enum:CsrBtAvrcpPTOpId:%d state enum:avAvrcpState:%d, reqState = %d", (void *)theInst, req->op_id, appAvrcpGetState(theInst), req->state);

            /* Repeat message every second */
            MessageCancelFirst(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ);
            if (!req->state && req->repeat_ms)
            {
                /* Send internal message */
                MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ);
                message->op_id = req->op_id;
                message->repeat_ms = req->repeat_ms;
                message->state = req->state;
                message->ui = FALSE;
                MessageSendLater(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ, message, req->repeat_ms);
            }
        }
        return;

        default:
            /* Ignore in any other states */
            DEBUG_LOG("appAvrcpHandleInternalAvrcpRemoteRequest(%p) ignored", (void *)theInst);
            return;
    }
}

/*! \brief Request outgoing AVRCP connection

    Handle AVRCP connect request from client task, call AvrcpConnectRequest()
    to create AVRCP channel and move into the 'connecting local' state.
*/

static void appAvrcpHandleInternalAvrcpConnectRequest(avInstanceTaskData *theInst, AV_INTERNAL_AVRCP_CONNECT_REQ_T *req)
{
    avAvrcpState state = appAvrcpGetState(theInst);

    DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectRequest(%p) addr[%04x,%02x,%06lx], state enum:avAvrcpState:%d",
                 (void *)theInst,
                 theInst->bd_addr.nap,
                 theInst->bd_addr.uap,
                 theInst->bd_addr.lap,
                 state);

    switch (state)
    {
        case AVRCP_STATE_DISCONNECTED:
        {
            /* Check ACL is connected */
            if (ConManagerIsConnected(&theInst->bd_addr))
            {
                CsrBtDeviceAddr deviceAddr;

                BdaddrConvertVmToBluestack(&deviceAddr, &theInst->bd_addr);
                /* Request outgoing connection */
                AvrcpConnectReqSend(deviceAddr);
                /* Move to 'connecting local' state */
                appAvrcpSetState(theInst, AVRCP_STATE_CONNECTING_LOCAL);
            }
            else
            {
                DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectRequest(%p) no ACL,", (void *)theInst);

                /* Send AV_AVRCP_CONNECT_CFM to all clients */
                MAKE_AV_MESSAGE(AV_AVRCP_CONNECT_CFM);
                message->av_instance = theInst;
                message->status = avrcp_device_not_connected;
                MessageSend(req->client_task, AV_AVRCP_CONNECT_CFM, message);

                appAvInstanceAvrcpDisconnected(theInst, FALSE);
                /* Move to 'disconnected' state */
                appAvrcpSetState(theInst, AVRCP_STATE_DISCONNECTED);
            }
        }
        return;

        case AVRCP_STATE_CONNECTED:
        {
            DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectRequest(%p) connected - requested_client %p",
                         (void *)theInst,
                         req->client_task);

            /* Need to release the ACL here to reduce the user count on it.
               Note: The user count was increased when ConManagerCreateAcl
                     was called when the original AV_INTERNAL_AVRCP_CONNECT_REQ
                     was sent. */
            ConManagerReleaseAcl(&theInst->bd_addr);

            /* Send confirm immediately */
            MAKE_AV_MESSAGE(AV_AVRCP_CONNECT_CFM);
            message->status = avrcp_success;
            message->av_instance = theInst;
            MessageSend(req->client_task, AV_AVRCP_CONNECT_CFM, message);
        }
        return;

        case AVRCP_STATE_CONNECTING_LOCAL:
        case AVRCP_STATE_CONNECTING_REMOTE:
        {
            PanicFalse(theInst->avrcp.lock);

            DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectRequest(%p) connecting - requested_client %p",
                         (void *)theInst,
                         req->client_task);

            /* Need to release the ACL here to reduce the user count on it.
               Note: The user count was increased when ConManagerCreateAcl
                     was called when the original AV_INTERNAL_AVRCP_CONNECT_REQ
                     was sent. */
            ConManagerReleaseAcl(&theInst->bd_addr);
        }
        return;

        case AVRCP_STATE_DISCONNECTING:
        {
            if (theInst->avrcp.lock)
            {
                MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_CONNECT_REQ);

                DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectRequest(%p) resending on lock", theInst);

                /* Re-send to ourselves conditional on AVRCP lock */
                message->client_task = req->client_task;
                MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_REQ, message,
                                         &appAvrcpGetLock(theInst));
            }
            else
            {
                /* Should never receive in these states as AVRCP lock is set */
                Panic();
            }
        }
        return;


        default:
            appAvrcpError(theInst, AV_INTERNAL_AVRCP_CONNECT_REQ, NULL);
            return;
    }
}

/*! \brief Request AVRCP disconnection

    Handle AVRCP disconnect request from AV parent task.  Move into the
    'disconnecting' state, this will initiate the disconnect.
*/
static void appAvrcpHandleInternalAvrcpDisconnectRequest(avInstanceTaskData *theInst, AV_INTERNAL_AVRCP_DISCONNECT_REQ_T *req)
{
    DEBUG_LOG("appAvrcpHandleInternalAvrcpDisconnectRequest(%p) requested_client %p,state enum:avAvrcpState:%d",
               (void *)theInst,
               req->client_task,
               appAvrcpGetState(theInst));

    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTED:
        case AVRCP_STATE_CONNECTING_REMOTE:
        {
            DEBUG_LOG("appAvrcpHandleInternalAvrcpDisconnectRequest(%p) disconnecting AVRCP", theInst);

            /* Move to 'disconnecting' state */
            appAvrcpSetState(theInst, AVRCP_STATE_DISCONNECTING);
        }
        return;

        case AVRCP_STATE_DISCONNECTED:
            /* Connection cross-over occured, ignore */
            return;

        default:
            appAvrcpError(theInst, AV_INTERNAL_AVRCP_DISCONNECT_REQ, NULL);
            return;
    }
}

/*! \brief Accept incoming AVRCP connection

    AVRCP Library has indicating an incoming AVRCP connection, if we're currently in the
    'disconnected' state accept the connection and move into the 'connecting remote' state, otherwise
    reject the connection.
*/
static void appAvrcpHandleInternalAvrcpConnectIndication(avInstanceTaskData *theInst,
                                                         const AV_INTERNAL_AVRCP_CONNECT_IND_T *ind)
{
    DEBUG_LOG("appAvrcpHandleInternalAvrcpConnectIndication(%p) state enum:avAvrcpState:%d",
                 (void *)theInst,
                 appAvrcpGetState(theInst));

    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTING_REMOTE:
        {
            /* Send AV_AVRCP_CONNECT_CFM to all clients */
            MAKE_AV_MESSAGE(AV_AVRCP_CONNECT_CFM);
            message->status = avrcp_success;
            message->av_instance = theInst;
            MessageSend(&theInst->av_task, AV_AVRCP_CONNECT_CFM, message);

            appAvrcpSetState(theInst, AVRCP_STATE_CONNECTED);
        }
        return;

        case AVRCP_STATE_CONNECTED:
        case AVRCP_STATE_DISCONNECTING:
            return;

        default:
            appAvrcpError(theInst, AV_INTERNAL_AVRCP_CONNECT_IND, ind);
            return;
    }
}

static void appAvrcpHandleInternalAvrcpVendorPassthroughRequest(avInstanceTaskData *theInst,
                                                                AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ_T *req)
{
    DEBUG_LOG("appAvrcpHandleInternalAvrcpVendorPassthroughRequest(%p)", (void *)theInst);

    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTED:
        {
            DEBUG_LOG("appAvrcpHandleInternalAvrcpVendorPassthroughRequest: Vendor Pass Through Cmd Not Supported");
        }
        return;

        case AVRCP_STATE_DISCONNECTED:
        {
            /* No connection, so reject command immediately */
            MAKE_AV_MESSAGE(AV_AVRCP_VENDOR_PASSTHROUGH_CFM);
            message->status = avrcp_device_not_connected;
            message->opid = req->op_id;
            MessageSend(req->client_task, AV_AVRCP_VENDOR_PASSTHROUGH_CFM, message);
        }
        return;

        default:
            appAvrcpError(theInst, AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ, req);
            return;
    }
}

static void appAvrcpHandleInternalAvrcpNotificationRegisterRequest(avInstanceTaskData *theInst,
                                                                   AV_INTERNAL_AVRCP_NOTIFICATION_REGISTER_REQ_T *req)
{
    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTED:
        {
			DEBUG_LOG("appAvrcpHandleInternalAvrcpNotificationRegisterRequest(%p), events %04x", theInst, req->event_mask);
			PanicFalse(theInst->avrcp.notification_lock == 0);

            AvrcpCtNotiRegisterReqSend(&(AvGetTaskData()->task),
                                       theInst->avrcp.connectionId,
                                       req->event_mask,
                                       0,
                                       CSR_BT_AVRCP_NOTI_REG_STANDARD);

            theInst->avrcp.notification_lock = req->event_mask;

            avrcp_supported_events event_id;
            for (event_id = avrcp_event_playback_status_changed;
                 event_id <= avrcp_event_volume_changed;
                 event_id++)
            {
                if (req->event_mask & appAvrcpEventIdToMask(event_id))
                {
                    /* Set registered event bit */
                    appAvrcpSetEventRegistered(theInst, event_id);
                }
            }
        }
        break;

        default:
            break;
    }

}

static void appAvrcpHandleInternalSetAbsoluteVolumeInd(avInstanceTaskData *theInst, AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND_T *ind)
{
    DEBUG_LOG("appAvrcpHandleInternalSetAbsoluteVolumeInd");
    avrcpProfile_HandleAbsoluteVolumeInd(theInst, ind->conn_id, ind->volume, ind->msg_id, FALSE, 0xFF);
}

static void appAvrcpSetPlaybackLock(avInstanceTaskData *theInst)
{
    assert(theInst->avrcp.playback_lock == 0);

    if (appAvrcpIsEventRegistered(theInst, CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS))
    {
        /* Set lock, and send timed message to automatically clear lock if no playback status received */
        theInst->avrcp.playback_lock = 1;
        MessageSendLater(&theInst->av_task,
                         AV_INTERNAL_AVRCP_CLEAR_PLAYBACK_LOCK_IND, NULL,
                         appConfigAvrcpPlayStatusNotificationTimeout());
        DEBUG_LOG("appAvrcpSetPlaybackLock(%p), set lock for playback operation", theInst);
    }
}

void appAvrcpClearPlaybackLock(avInstanceTaskData *theInst)
{
    if (theInst->avrcp.playback_lock)
    {
        DEBUG_LOG("appAvrcpClearPlaybackLock(%p), clear lock for playback operation", theInst);
        theInst->avrcp.playback_lock = 0;
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_CLEAR_PLAYBACK_LOCK_IND);
    }
}

static bool appAvrcpIsPlaybackLocked(avInstanceTaskData *theInst)
{
    return theInst->avrcp.playback_lock && appAvrcpIsEventRegistered(theInst, CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS);
}

static void appAvrcpHandleInternalAvrcpPlayRequest(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpHandleInternalAvrcpPlayRequest(%p)", theInst);
    if (appAvrcpIsPlaybackLocked(theInst))
    {
        /* Re-send message if lock set, as it may be delayed message */
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
    }
    else
    {
        /* Check if we should send AVRCP_PLAY or if we don't know */
        if (!Av_IsInstancePlaying(theInst))
        {
            appAvrcpRemoteControl(theInst, CSR_BT_AVRCP_PT_OP_ID_PLAY, CSR_BT_AVRCP_PT_STATE_PRESS, FALSE, 0);
            appAvrcpRemoteControl(theInst, CSR_BT_AVRCP_PT_OP_ID_PLAY, CSR_BT_AVRCP_PT_STATE_RELEASE, FALSE, 0);
            appAvHintPlayStatus(theInst, avrcp_play_status_playing);
            appAvrcpSetPlaybackLock(theInst);
        }
    }
}

static void appAvrcpHandleInternalAvrcpPauseRequest(avInstanceTaskData *theInst)
{
    if (appAvrcpIsPlaybackLocked(theInst))
    {
        /* Re-send message if lock set, as it may be delayed message */
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PAUSE_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
        DEBUG_LOG("appAvrcpHandleInternalAvrcpPauseRequest(%p) playback locked", theInst);
    }
    else
    {
        /* Check if we should send AVRCP_PAUSE */
        if (!Av_IsInstancePaused(theInst))
        {
            DEBUG_LOG("appAvrcpHandleInternalAvrcpPauseRequest(%p) not locked not paused", theInst);
            appAvrcpRemoteControl(theInst, CSR_BT_AVRCP_PT_OP_ID_PAUSE, CSR_BT_AVRCP_PT_STATE_PRESS, FALSE, 0);
            appAvrcpRemoteControl(theInst, CSR_BT_AVRCP_PT_OP_ID_PAUSE, CSR_BT_AVRCP_PT_STATE_RELEASE, FALSE, 0);
            appAvHintPlayStatus(theInst, avrcp_play_status_paused);
            appAvrcpSetPlaybackLock(theInst);
        }
        else
        {
            DEBUG_LOG("appAvrcpHandleInternalAvrcpPauseRequest(%p) not locked paused", theInst);
        }
    }

}

static void appAvrcpHandleInternalAvrcpPlayToggleRequest(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvrcpHandleInternalAvrcpPlayToggleRequest(%p)", theInst);
    if (theInst->avrcp.playback_lock && appAvrcpIsEventRegistered(theInst, CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS))
    {
        /* Re-send message if lock set, as it may be delayed message */
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_TOGGLE_REQ, NULL,
                                 &theInst->avrcp.playback_lock);
    }
    else
    {
        CsrBtAvrcpPTOpId opid;
        CsrBtAvrcpPlaybackStatus status;

        if (Av_IsInstancePlaying(theInst))
        {
            opid = CSR_BT_AVRCP_PT_OP_ID_PAUSE;
            status = avrcp_play_status_paused;
        }
        else
        {
            opid = CSR_BT_AVRCP_PT_OP_ID_PLAY;
            status = avrcp_play_status_playing;
        }

        appAvrcpRemoteControl(theInst, opid, CSR_BT_AVRCP_PT_STATE_PRESS, FALSE, 0);
        appAvrcpRemoteControl(theInst, opid, CSR_BT_AVRCP_PT_STATE_RELEASE, FALSE, 0);
        appAvHintPlayStatus(theInst, status);
        appAvrcpSetPlaybackLock(theInst);
    }
}

static void appAvrcpHandleAvrcpTgMpRegisterConfirm(CsrBtAvrcpTgMpRegisterCfm *cfm)
{
    DEBUG_LOG("appAvrcpHandleAvrcpTgMpRegisterConfirm: status 0x%04x, supplier 0x%04x",
                cfm->resultCode, cfm->resultSupplier);

    AvrcpActivateReqSend(AVRCP_MAX_CONNECTION_INSTANCES);
}

static void avrcpProfile_HandleAbsoluteVolumeInd(avInstanceTaskData *theInst, uint8 conn_id, uint8 vol, uint32 msg_id, bool send_response, uint8 tlabel)
{
    DEBUG_LOG("avrcpProfile_HandleAbsoluteVolumeInd(%p), volume %u", (void *)theInst, vol);

    if (appDeviceIsHandset(&theInst->bd_addr) && theInst->avrcp.bitfields.suppress_absolute_volume)
    {
        CsrBtAvrcpStatus response = CSR_BT_AVRCP_STATUS_INTERNAL_ERROR;
        uint8 volume = 0;
        audio_source_t source = Av_GetSourceForInstance(theInst);

        if (source != audio_source_none)
        {
            volume = AudioSources_GetVolume(source).value;
            response = CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE;
        }

        DEBUG_LOG("avrcpProfile_HandleAbsoluteVolumeInd(%p) suppressed, overriding response with %u", (void *)theInst, volume);

        if (send_response)
        {
            AvrcpTgSetVolumeResSend(conn_id, volume, msg_id, tlabel, response);
        }
    }
    else
    {
        /* Calculate time since last CSR_BT_AVRCP_TG_SET_VOLUME_IND */
        rtime_t delta = theInst->avrcp.bitfields.volume_time_valid ? rtime_sub(VmGetClock(), theInst->avrcp.volume_time) : 0;

        /* Cancel any pending AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND message */
        MessageCancelFirst(&theInst->av_task, AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND);

        /* If time since last CSR_BT_AVRCP_TG_SET_VOLUME_IND is less than 200ms, delay handling of message
         * by sending it back to ourselves with a delay */
        if ((delta > 0) && (delta < 200))
        {
            const uint32_t delay = 200 - delta;
            MAKE_AV_MESSAGE(AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND);
            message->conn_id = conn_id;
            message->volume  = vol;
            message->msg_id  = msg_id;
            MessageSendLater(&theInst->av_task, AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND, message, delay);
            DEBUG_LOG("avrcpProfile_HandleAbsoluteVolumeInd(%p), delaying for %ums", (void *)theInst, delay);

            /* Accept the volume change, but wait to actually use it */
            AvrcpTgSetVolumeResSend(conn_id, vol, msg_id, tlabel, CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE);
        }
        else
        {
            /* Send set volume ind to all clients */
            MAKE_AV_MESSAGE(AV_AVRCP_SET_VOLUME_IND)
            message->av_instance = theInst;
            message->bd_addr = theInst->bd_addr;
            message->volume = vol;
            MessageSend(&theInst->av_task, AV_AVRCP_SET_VOLUME_IND, message);

            /* Accept the volume change */
            if (send_response)
                AvrcpTgSetVolumeResSend(conn_id, vol, msg_id, tlabel, CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE);

            /* Remember time AVRCP_SET_ABSOLUTE_VOLUME_IND was handled, so that we can check
             * timing of subsequent messages */
            theInst->avrcp.volume_time = VmGetClock();
            theInst->avrcp.bitfields.volume_time_valid = TRUE;
        }
    }
}

/*! \brief Handle incoming AVRCP connection

    AVRCP Library has indicating an incoming AVRCP connection,
    Check if we can create or use an existing AV instance, if so accept the
    incoming connection otherwise reject it.
*/
static void appAvrcpHandleAvrcpConnectInd(const CsrBtAvrcpConnectInd *ind)
{
    avInstanceTaskData *av_inst;
    bdaddr bd_addr;

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);

    /* Create task (or find exising one) for the AVRCP connection */
    av_inst = appAvInstanceFindFromBdAddr(&bd_addr);
    if (av_inst == NULL)
        av_inst = appAvInstanceCreate(&bd_addr, &av_plugin_interface);
    else
    {
        /* Make sure there's no pending destroy message */
        MessageCancelAll(&av_inst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
        MessageCancelAll(&av_inst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);
    }

    DEBUG_LOG("appAvrcpHandleAvrcpConnectInd(%p) addr(%04x,%02x,%06lx) connectionId(%d)",
                av_inst, bd_addr.nap, bd_addr.uap, bd_addr.lap, ind->connectionId);

    /* Send message to task if possible, otherwise reject connection */
    if (av_inst != NULL)
    {
         switch (appAvrcpGetState(av_inst))
         {
             case AVRCP_STATE_DISCONNECTED:
             /* Fall-through to next case */
             case AVRCP_STATE_CONNECTING_LOCAL:
             {
                 /* Outgoing connection already initiated for the device. Move the state to connecting remote*/
                 /* Store the connection Ids */
                 av_inst->avrcp.connectionId = ind->connectionId;
                 av_inst->avrcp.btConnId     = ind->btConnId;

                 /* Wait for Remote Features Indication */
                 appAvrcpSetState(av_inst, AVRCP_STATE_CONNECTING_REMOTE);
                 break;
             }

             default:
                 break;
         }
    }
    else
    {
        DEBUG_LOG("appAvrcpHandleAvrcpConnectInd, rejecting");
        /* Connection is already established, disconnect it */
        AvrcpDisconnectReqSend(ind->connectionId);
    }
}

/*! \brief Handle AVRCP remote feature indication
    AVRCP Library has indicated about remote feature details.
    Go for register notification now.
*/
static void appAvrcpHandleAvrcpRemoteFeaturesInd(const CsrBtAvrcpRemoteFeaturesInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleAvrcpRemoteFeaturesInd(%p): connectionId(%d)", theInst, ind->connectionId);

    if (theInst && (theInst->avrcp.connectionId == ind->connectionId))
    {
        switch (appAvrcpGetState(theInst))
        {
            case AVRCP_STATE_CONNECTING_REMOTE:
            {
                /* Create message to send to AV instance */
                MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_CONNECT_IND);
                message->connection_id = ind->connectionId;
                MessageSend(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_IND, message);
            }
            return;

            default:
                appAvrcpError(theInst, CSR_BT_AVRCP_REMOTE_FEATURES_IND, ind);
                return;
        }
    }
}

/*! \brief AVRCP connect confirmation

    AVRCP library has confirmed connection request.
    First of all check if the request was successful, if it was then we should
    store the pointer to the newly created AVRCP instance.  After this move into the
    'connected' state as we now have an active AVRCP channel.

    If the request was unsucessful, move back to the 'disconnected' state and
    play an error tone if this connection request was silent.  Note: Moving to
    the 'disconnected' state may result in this AV instance being free'd.
*/
static void appAvrcpHandleAvrcpConnectCfm(const CsrBtAvrcpConnectCfm *cfm)
{
    avInstanceTaskData *theInst;
    bdaddr bd_addr;

    BdaddrConvertBluestackToVm(&bd_addr, &cfm->deviceAddr);

    /* there must be a relevant instance for this address */
    theInst = appAvInstanceFindFromBdAddr(&bd_addr);

    DEBUG_LOG("appAvrcpHandleAvrcpConnectCfm(%p) addr(%04x,%02x,%06lx) result(0x%04x) supplier(0x%04x) connectionId(%d)",
                theInst, bd_addr.nap, bd_addr.uap, bd_addr.lap,
                cfm->resultCode, cfm->resultSupplier, cfm->connectionId);

    if (theInst)
    {
        switch (appAvrcpGetState(theInst))
        {
            case AVRCP_STATE_CONNECTING_LOCAL:
            {
                /* Send AV_AVRCP_CONNECT_CFM to all clients */
                MAKE_AV_MESSAGE(AV_AVRCP_CONNECT_CFM);
                message->av_instance = theInst;

                /* Check if signalling channel created successfully */
                if (cfm->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS &&
                    cfm->resultSupplier == CSR_BT_SUPPLIER_AVRCP)
                {
                    message->status = avrcp_success;
                    MessageSend(&theInst->av_task, AV_AVRCP_CONNECT_CFM, message);

                    /* Move to 'connected' state */
                    theInst->avrcp.connectionId = cfm->connectionId;
                    theInst->avrcp.btConnId     = cfm->btConnId;
                    appAvrcpSetState(theInst, AVRCP_STATE_CONNECTED);
                }
                else
                {
                    message->status = avrcp_fail;
                    if(cfm->resultSupplier == CSR_BT_SUPPLIER_L2CAP_CONNECT && cfm->resultCode == L2CA_CONNECT_TIMEOUT)
                    {
                        message->status = avrcp_timeout;
                    }
                    MessageSend(&theInst->av_task, AV_AVRCP_CONNECT_CFM, message);

                    appAvInstanceAvrcpDisconnected(theInst, FALSE);
                    /* Move to 'disconnected' state */
                    appAvrcpSetState(theInst, AVRCP_STATE_DISCONNECTED);
                }
                return;
            }

            case AVRCP_STATE_CONNECTING_REMOTE:
                /* Incoming connection already established.
                 * AV_AVRCP_CONNECT_CFM will be sent on moving to connected state */
                return;

            default:
                appAvrcpError(theInst, CSR_BT_AVRCP_CONNECT_CFM, cfm);
                return;
        }
    }
}

/*! \brief AVRCP connection disconnected

    AVRCP Library has indicated that the AVRCP connection has been
    disconnected, move to the 'disconnected' state, this will result
    in this AV instance being destroyed.
*/
static void appAvrcpHandleAvrcpDisconnectInd(const CsrBtAvrcpDisconnectInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleAvrcpDisconnectInd(%p): result(0x%04x) supplier(0x%04x) connectionId(%d)",
              theInst, ind->reasonCode, ind->reasonSupplier, ind->connectionId);

    if (theInst && (theInst->avrcp.connectionId == ind->connectionId))
    {
        switch (appAvrcpGetState(theInst))
        {
            case AVRCP_STATE_CONNECTING_LOCAL:
            case AVRCP_STATE_CONNECTING_REMOTE:
            case AVRCP_STATE_CONNECTED:
            {
                /* Fall-through to next case */
            }

            case AVRCP_STATE_DISCONNECTING:
            {
                if (ind->reasonCode != L2CA_DISCONNECT_LINK_TRANSFERRED)
                {
                    appAvInstanceAvrcpDisconnected(theInst, TRUE);
                    /* Move to 'disconnected' state */
                    appAvrcpSetState(theInst, AVRCP_STATE_DISCONNECTED);
                }
                else
                {
                    /* Move to 'disconnected' state */
                    theInst->avrcp.state = AVRCP_STATE_DISCONNECTED;

                    /* Clear the connection ID here to be in sync with the Synergy library. */
                    theInst->avrcp.connectionId = AV_CONN_ID_INVALID;
                    theInst->avrcp.btConnId = 0;

                    /* Destroy the AV isntance if both A2DP and AVRCP profiles are disconnected */
                    appAvInstanceDestroy(theInst);
                }
            }
            return;

            case AVRCP_STATE_DISCONNECTED:
            {
                /* Disconnect crossover, do nothing this instance is about to be destroyed */
            }
            return;

            default:
                appAvrcpError(theInst, CSR_BT_AVRCP_DISCONNECT_IND, ind);
                return;
        }
    }
    else
    {
        DEBUG_LOG("appAvrcpHandleAvrcpDisconnectInd: Avrcp instance not found");
    }
}

/*! \brief AVRCP passthrough command confirmation
*/
static void appAvrcpHandleAvrcpPassthroughCfm(const CsrBtAvrcpCtPassThroughCfm *cfm)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleAvrcpPassthroughCfm(%p), result(0x%04x) supplier(0x%04x)",
               (void *)theInst, cfm->resultCode, cfm->resultSupplier);

    if (theInst && (theInst->avrcp.connectionId == cfm->connectionId))
    {
        switch (appAvrcpGetState(theInst))
        {
            case AVRCP_STATE_CONNECTED:
            {
                /* Clear operation lock */
                appAvrcpClearLock(theInst, APP_AVRCP_LOCK_PASSTHROUGH_REQ);

                /* Clear any pending requests if this one failed, so the rest will probably as well */
                if (cfm->resultCode != CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
                {
                    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ);
                    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REQ);
                }

                /* Play specific tone if required */
                switch (theInst->avrcp.bitfields.op_id)
                {
                    case CSR_BT_AVRCP_PT_OP_ID_VENDOR_DEP:
                    {
                        appAvrcpFinishAvrcpPassthroughRequest(theInst, cfm->resultCode);
                    }
                    break;

                    default:
                    {
                        /* Play standard AV error tone (only for button press, not for button release) */
                        if ((theInst->avrcp.bitfields.op_state == 0) &&
                             (cfm->resultCode != CSR_BT_RESULT_CODE_AVRCP_SUCCESS))
                        {
                            appAvSendUiMessageId(AV_ERROR);
                        }
                    }
                    break;
                }
            }
            return;

            case AVRCP_STATE_DISCONNECTED:
            {
                /* Received in invalid state, due to some race condition, we can
                   just ignore */
                DEBUG_LOG("appAvrcpFinishAvrcpPassthroughRequest stale passthrough cfm msg from avrcp - ignore");
            }
            return;

            default:
                appAvrcpError(theInst, CSR_BT_AVRCP_CT_PASS_THROUGH_CFM, cfm);
                return;
        }
    }
    else
    {
        DEBUG_LOG("appAvrcpHandleAvrcpPassthroughCfm: Avrcp instance not found");
    }
}

/*! \brief Confirmation of SetAbsoluteVolume Command (Controller->Target)
*/
static void appAvrcpHandleSetAbsoluteVolumeCfm(CsrBtAvrcpCtSetVolumeCfm *cfm)
{
    DEBUG_LOG("appAvrcpHandleSetAbsoluteVolumeCfm status:0x%04x, supplier:0x%04x",
                cfm->resultCode, cfm->resultSupplier);
}

/*! \brief Absolute volume change from A2DP Source (Handset or TWS Master)
*/
static void appAvrcpHandleSetAbsoluteVolumeInd(CsrBtAvrcpTgSetVolumeInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleSetAbsoluteVolumeInd(%p) volume(%u)", (void *)theInst, ind->volume);

    if (theInst != NULL)
    {
        avrcpProfile_HandleAbsoluteVolumeInd(theInst, ind->connectionId, ind->volume, ind->msgId, TRUE, ind->tLabel);
    }
}

static bool appAvrcpClearNotificationLock(avInstanceTaskData *theInst, CsrBtAvrcpNotiMask event_mask)
{
    if (theInst->avrcp.notification_lock & event_mask)
    {
        DEBUG_LOG("appAvrcpClearNotificationLock(%p), lock 0x%04x, clearing events 0x%04x", theInst,
                                                    theInst->avrcp.notification_lock, event_mask);
        theInst->avrcp.notification_lock &= ~event_mask;
        return TRUE;
    }
    else
    {
        DEBUG_LOG("appAvrcpClearNotificationLock(%p), lock 0x%04x, failed to clear events 0x%04x", theInst,
                                                    theInst->avrcp.notification_lock, event_mask);
        return FALSE;
    }
}

static void appAvrcpHandleRegisterNotificationCfm(CsrBtAvrcpCtNotiRegisterCfm *cfm)
{
    CsrBtAvrcpNotiId event_id;
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(cfm->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleRegisterNotificationCfm(%p) status:0x%04x, supplier:0x%04x",
                (void *)theInst, cfm->resultCode, cfm->resultSupplier);

    if (theInst && (theInst->avrcp.connectionId == cfm->connectionId))
    {
        if (cfm->resultCode == CSR_BT_RESULT_CODE_AVRCP_COMMAND_DISALLOWED &&
            cfm->resultSupplier == CSR_BT_SUPPLIER_AVRCP)
        {
            DEBUG_LOG("appAvrcpHandleRegisterNotificationCfm(%p) event mask %d is already registered",
                        (void *)theInst, cfm->notiMask);
        }
        else
        {
            DEBUG_LOG("appAvrcpHandleRegisterNotificationCfm(%p) status:0x%04x, supplier:0x%04x, event %d",
                        (void *)theInst, cfm->resultCode,
                        cfm->resultSupplier, cfm->notiMask);

            /* Clear all registered event bit */
            if (cfm->resultCode != CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
            {
                for (event_id = CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS;
                     event_id <= CSR_BT_AVRCP_NOTI_ID_VOLUME;
                     event_id++)
                {
                    if (cfm->notiMask & appAvrcpEventIdToMask(event_id))
                    {
                        appAvrcpClearEventRegistered(theInst, event_id);
                    }
                }
            }
        }

        /* Clear any notification request lock for this event*/
        appAvrcpClearNotificationLock(theInst, cfm->notiMask);

    }
    else
    {
        DEBUG_LOG("appAvrcpHandleRegisterNotificationCfm: Avrcp instance not found");
    }
}

static void appAvrcpHandleEventVolumeChangedInd(CsrBtAvrcpCtNotiVolumeInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleEventVolumeChangedInd: volume %u", ind->volume);

    if (theInst && (theInst->avrcp.connectionId == ind->connectionId))
    {
        /* Send AV_VOLUME_CHANGED_IND to all clients */
        MAKE_AV_MESSAGE(AV_AVRCP_VOLUME_CHANGED_IND);
        message->av_instance = theInst;
        message->bd_addr = theInst->bd_addr;
        message->volume = ind->volume;
        MessageSend(&theInst->av_task, AV_AVRCP_VOLUME_CHANGED_IND, message);
    }
    else
    {
        DEBUG_LOG("appAvrcpHandleEventVolumeChangedInd: Avrcp instance not found");
    }
}

static void avrcpProfile_DoVolumeOperation(avInstanceTaskData *theInst, const CsrBtAvrcpTgPassThroughInd *ind)
{
    avrcp_response_type response = CSR_BT_AVRCP_PT_STATUS_ACCEPT;

    if (ind->state == CSR_BT_AVRCP_PT_STATE_PRESS)
    {
        audio_source_t source = Av_GetSourceForInstance(theInst);

        if (source != audio_source_none)
        {
            int new_volume = AudioSources_GetVolume(source).value + ((ind->operationId == CSR_BT_AVRCP_PT_OP_ID_VOLUME_UP) ? AUDIO_VOLUME_STEP_UP : AUDIO_VOLUME_STEP_DOWN);
            Volume_SendAudioSourceVolumeUpdateRequest(source, event_origin_local, new_volume);
        }
        else
        {
            response = CSR_BT_AVRCP_PT_STATUS_REJECT;
        }
    }

    AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, response);
}

static void avrcpProfile_DoAvrcpOperation(avInstanceTaskData *theInst, const CsrBtAvrcpTgPassThroughInd *ind, bool (*avrcpOp)(void *, bool))
{
    if (avrcpOp == NULL || avrcpOp(theInst, !ind->state))
    {
       AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, CSR_BT_AVRCP_PT_STATUS_ACCEPT);
    }
    else
    {
       AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, CSR_BT_AVRCP_PT_STATUS_REJECT);
    }
}

static void appAvrcpHandlePassThroughInd(CsrBtAvrcpTgPassThroughInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandlePassThroughInd: connection id %d", ind->connectionId);

    assert(theInst);

    switch (appAvrcpGetState(theInst))
    {
        case AVRCP_STATE_CONNECTED:
        case AVRCP_STATE_CONNECTING_REMOTE:
        {
            switch (ind->operationId)
            {
                case CSR_BT_AVRCP_PT_OP_ID_POWER:
                    AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, CSR_BT_AVRCP_PT_STATUS_NOT_IMPL);
                    break;

                case CSR_BT_AVRCP_PT_OP_ID_VOLUME_UP:
                case CSR_BT_AVRCP_PT_OP_ID_VOLUME_DOWN:
                    avrcpProfile_DoVolumeOperation(theInst, ind);
                    break;

                case CSR_BT_AVRCP_PT_OP_ID_PLAY:
                    avrcpProfile_DoAvrcpOperation(theInst, ind, theInst->av_callbacks->OnAvrcpPlay);
                    break;

                case CSR_BT_AVRCP_PT_OP_ID_PAUSE:
                    avrcpProfile_DoAvrcpOperation(theInst, ind, theInst->av_callbacks->OnAvrcpPause);
                    break;

                case CSR_BT_AVRCP_PT_OP_ID_FORWARD:
                    avrcpProfile_DoAvrcpOperation(theInst, ind, theInst->av_callbacks->OnAvrcpForward);
                    break;

                case CSR_BT_AVRCP_PT_OP_ID_BACKWARD:
                    avrcpProfile_DoAvrcpOperation(theInst, ind, theInst->av_callbacks->OnAvrcpBackward);
                    break;

                default:
                    AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, CSR_BT_AVRCP_PT_STATUS_ACCEPT);
                    break;
            }
        }
        return;

        default:
            AvrcpTgPassThroughResSend(ind->connectionId, ind->msgId, CSR_BT_AVRCP_PT_STATUS_REJECT);
            return;
    }
}

static void avrcpProfile_SendPlaybackStatusPlayingIndication(avInstanceTaskData *theInst)
{
    MAKE_AV_MESSAGE(AV_AVRCP_PLAY_STATUS_PLAYING_IND);
    message->av_instance = theInst;
    appAvSendStatusMessage(AV_AVRCP_PLAY_STATUS_PLAYING_IND, message, sizeof(*message));
}

static void avrcpProfile_SendPlaybackStatusNotPlayingIndication(avInstanceTaskData *theInst)
{
    MAKE_AV_MESSAGE(AV_AVRCP_PLAY_STATUS_NOT_PLAYING_IND);
    message->av_instance = theInst;
    appAvSendStatusMessage(AV_AVRCP_PLAY_STATUS_NOT_PLAYING_IND, message, sizeof(*message));
}

/* The peer side (controller) has registered for these events to the target */
static void appAvrcpHandleRegisterNotificationInd(CsrBtAvrcpTgNotiInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);
    CsrBtAvrcpStatus response = CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE;

    assert(theInst);

    DEBUG_LOG("appAvrcpHandleRegisterNotificationInd(%p), enum:avrcp_supported_events:%d",
              (void *)theInst, ind->notiId);
    appAvrcpSetEventSupported(theInst, ind->notiId);
    
    switch (ind->notiId)
    {
        case CSR_BT_AVRCP_NOTI_ID_VOLUME:
        {
            audio_source_t source = Av_GetSourceForInstance(theInst);
            volume_t volume       = AudioSources_GetVolume(source);

            DEBUG_LOG("appAvrcpHandleRegisterNotificationInd(%p) new volume:%d",
                      (void *)theInst, volume.value);
            AvrcpTgNotiVolumeRes(ind->connectionId, response,
                                 ind->msgId, volume.value);
        }
        break;

        case CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS:
        {
            DEBUG_LOG("appAvrcpHandleRegisterNotificationInd(%p), play_status %u",
                      (void *)theInst, theInst->avrcp.play_status);
            AvrcpTgNotiPlaybackStatusRes(ind->connectionId,
                                         response,
                                         ind->msgId,
                                         theInst->avrcp.play_status);
        }
        break;
        
        case CSR_BT_AVRCP_NOTI_ID_TRACK:
        {
            DEBUG_LOG("appAvrcpHandleRegisterNotificationInd(%p), track_changed", (void *)theInst);
            AvrcpMetadata_HandleEventTrackChanged(theInst, AvrcpProfileAbstract_GetResponseCode(response), ind->msgId);
        }
        break;
        
        default:
        {
            uint8 notiData[CSR_BT_AVRCP_TG_NOTI_MAX_SIZE] = { 0 };
            DEBUG_LOG_ERROR("appAvrcpHandleRegisterNotificationInd(%p), unsupported event %u", (void *)theInst, ind->notiId);
            AvrcpTgNotiResSend(ind->connectionId, ind->notiId, notiData, CSR_BT_AVRCP_DATA_AVC_RTYPE_NOT_IMP, ind->msgId);
        }
        break;
    }
}

static void appAvrcpHandleEventPlaybackStatusChangedInd(CsrBtAvrcpCtNotiPlaybackStatusInd *ind)
{
    avInstanceTaskData *theInst = AvInstance_GetInstanceForSearchId(ind->connectionId, TRUE, DEVICE_PROFILE_AVRCP);

    DEBUG_LOG("appAvrcpHandleEventPlaybackStatusChangedInd(%p): enum:CsrBtAvrcpPlaybackStatus:%d connectionId %d",
              (void *)theInst, ind->playbackStatus, ind->connectionId);

    if (theInst != NULL)
    {
        assert(theInst->avrcp.connectionId == ind->connectionId);

        /* Clear playback lock, so that any queued up play/pause requests can be processed */
        appAvrcpClearPlaybackLock(theInst);

        if (ind->playbackStatus ==  CSR_BT_AVRCP_PLAYBACK_STATUS_PLAYING)
        {
            avrcpProfile_SendPlaybackStatusPlayingIndication(theInst);
        }
        else
        {
            avrcpProfile_SendPlaybackStatusNotPlayingIndication(theInst);
        }

        /* Send AV_AVRCP_PLAY_STATUS_CHANGED_IND to all clients */
        MAKE_AV_MESSAGE(AV_AVRCP_PLAY_STATUS_CHANGED_IND);
        message->av_instance = theInst;
        message->bd_addr = theInst->bd_addr;
        message->play_status = ind->playbackStatus;
        MessageSend(&theInst->av_task, AV_AVRCP_PLAY_STATUS_CHANGED_IND, message);

        /* Keep copy of current play status */
        theInst->avrcp.play_status = ind->playbackStatus;

        /* Update link policy as soon as the play status changes to prevent audio glitches in multipoint use cases. */
        appLinkPolicyUpdatePowerTable(&theInst->bd_addr);
    }
}

/*! \brief  Send a remote control request, with optional repetition

    \param  theInst     Instance to send message on
    \param  op_id       The AVRCP operation to send
    \param  rstate      Repeat state. Non-zero indicates cancel.
    \param  ui          Whether to issue a UI indication for this event
    \param  repeat_ms   Delay between repetitions, 0 means no repeat
*/
void appAvrcpRemoteControl(avInstanceTaskData *theInst, CsrBtAvrcpPTOpId op_id, CsrBtAvrcpPTState rstate, bool ui, uint16 repeat_ms)
{
    PanicFalse(appAvIsValidInst(theInst));
    DEBUG_LOG("appAvrcpRemoteControl");

    /* Cancel repeated operation, exit if it doesn't exist */
    if (rstate && repeat_ms && !MessageCancelFirst(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ))
    {
        return;
    }
    else
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_REMOTE_REQ);

        /* Send internal message */
        message->op_id = op_id;
        message->state = rstate;
        message->ui = ui;
        message->repeat_ms = repeat_ms;
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_REMOTE_REQ,
                                 message, &appAvrcpGetLock(theInst));
    }
}

/*! \brief Handle AVRCP messages from the AV module, and also from the AVRCP library

    \param  theInst The AV Instance that this message is for
    \param  id      ID of message
    \param[in]  message Pointer to received message content, can be NULL.

*/
void appAvrcpInstanceHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message)
{
    /* Handle internal messages */
    switch (id)
    {
        case AV_INTERNAL_AVRCP_CONNECT_IND:
            appAvrcpHandleInternalAvrcpConnectIndication(theInst, (AV_INTERNAL_AVRCP_CONNECT_IND_T *)message);
            return;

        case AV_INTERNAL_AVRCP_CONNECT_REQ:
            appAvrcpHandleInternalAvrcpConnectRequest(theInst, (AV_INTERNAL_AVRCP_CONNECT_REQ_T *)message);
            return;

        case AV_INTERNAL_AVRCP_DISCONNECT_REQ:
            appAvrcpHandleInternalAvrcpDisconnectRequest(theInst, (AV_INTERNAL_AVRCP_DISCONNECT_REQ_T *)message);
            return;

        case AV_INTERNAL_AVRCP_REMOTE_REQ:
            appAvrcpHandleInternalAvrcpRemoteRequest(theInst, (AV_INTERNAL_AVRCP_REMOTE_REQ_T *)message, FALSE);
            return;

        case AV_INTERNAL_AVRCP_REMOTE_REPEAT_REQ:
            appAvrcpHandleInternalAvrcpRemoteRequest(theInst, (AV_INTERNAL_AVRCP_REMOTE_REQ_T *)message, TRUE);
            return;

        case AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ:
            appAvrcpHandleInternalAvrcpVendorPassthroughRequest(theInst, (AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ_T *)message);
            return;

        case AV_INTERNAL_AVRCP_NOTIFICATION_REGISTER_REQ:
            appAvrcpHandleInternalAvrcpNotificationRegisterRequest(theInst, (AV_INTERNAL_AVRCP_NOTIFICATION_REGISTER_REQ_T *)message);
            return;

        case AV_INTERNAL_AVRCP_PLAY_REQ:
            appAvrcpHandleInternalAvrcpPlayRequest(theInst);
            return;

        case AV_INTERNAL_AVRCP_PAUSE_REQ:
            appAvrcpHandleInternalAvrcpPauseRequest(theInst);
            return;

        case AV_INTERNAL_AVRCP_PLAY_TOGGLE_REQ:
            appAvrcpHandleInternalAvrcpPlayToggleRequest(theInst);
            return;

        case AV_INTERNAL_AVRCP_CLEAR_PLAYBACK_LOCK_IND:
            appAvrcpClearPlaybackLock(theInst);
            return;

        case AV_INTERNAL_AVRCP_DESTROY_REQ:
            appAvInstanceDestroy(theInst);
            return;

        case AV_INTERNAL_ALLOW_ABSOLUTE_VOLUME:
            appAvrcpAllowAbsoluteVolume(theInst);
            return;

        case AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND:
            appAvrcpHandleInternalSetAbsoluteVolumeInd(theInst, (AV_INTERNAL_SET_ABSOLUTE_VOLUME_IND_T *) message);
            return;
    }

    /* Unhandled message */
    appAvrcpError(theInst, id, message);
}

/*! \brief Register a task to handle vendor passthrough messages on this link

    \param  theInst     The AV instance to register for
    \param  client_task Task to register

    \returns The previous handling task (if any)
 */
Task appAvrcpVendorPassthroughRegister(avInstanceTaskData *theInst, Task client_task)
{
    Task task = theInst->avrcp.vendor_task;
    PanicFalse(appAvIsValidInst(theInst));
    theInst->avrcp.vendor_task = client_task;
    return task;
}

void appAvrcpNotificationsRegister(avInstanceTaskData *theInst, uint16 ntfMask)
{
    DEBUG_LOG("appAvrcpNotificationsRegister(%p), events 0x%04x", theInst, ntfMask);
    if (ntfMask)
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_NOTIFICATION_REGISTER_REQ)
        message->event_mask = ntfMask;
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_NOTIFICATION_REGISTER_REQ, message, &theInst->avrcp.notification_lock);
    }
}

void appAvAvrcpVolumeNotification(avInstanceTaskData *theInst, uint8 volume)
{
    if (appAvrcpIsConnected(theInst) && appAvrcpIsEventSupported(theInst, CSR_BT_AVRCP_NOTI_ID_VOLUME))
    {
        DEBUG_LOG("appAvAvrcpVolumeNotification, event registered %p, sending changed response, volume %u", theInst, volume);
        AvrcpTgNotiVolumeRes(theInst->avrcp.connectionId, CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED, 0, volume);
        appAvrcpClearEventSupported(theInst, CSR_BT_AVRCP_NOTI_ID_VOLUME);
    }
    else
    {
        DEBUG_LOG("appAvAvrcpVolumeNotification, event not registered %p", theInst);
    }
}

void appAvrcpSetAbsoluteVolumeRequest(avInstanceTaskData *theInst, uint8 volume)
{
    AvrcpCtSetVolumeReqSend(&(AvGetTaskData()->task), theInst->avrcp.connectionId, 
                            volume);
}

void appAvAvrcpPlayStatusNotification(avInstanceTaskData *theInst, CsrBtAvrcpNotiId play_status)
{
    theInst->avrcp.play_status = play_status;
    if (appAvrcpIsConnected(theInst) && appAvrcpIsEventSupported(theInst, CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS))
    {
        DEBUG_LOG("appAvAvrcpPlayStatusNotification, event registered %p, sending changed response, status %u", theInst, play_status);
        AvrcpTgNotiPlaybackStatusRes(theInst->avrcp.connectionId, CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED, 0, theInst->avrcp.play_status);
        appAvrcpClearEventSupported(theInst, CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS);
    }
    else
    {
        DEBUG_LOG("appAvAvrcpPlayStatusNotification, event not registered %p", theInst);
    }
}

/*! \brief Send a passthrough request on the AVRCP link

    \param  theInst       The AV instance this applies to
    \param  op_id         The request code to send
    \param  size_payload  The size of the variable length payload (in octets)
    \param  payload       Pointer to the variable length payload
 */
void appAvrcpVendorPassthroughRequest(avInstanceTaskData *theInst, CsrBtAvrcpPTOpId op_id, uint16 size_payload, const uint8 *payload)
{
    if (appAvIsValidInst(theInst) && appAvrcpIsConnected(theInst))
    {
        MAKE_AV_MESSAGE_WITH_LEN(AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ, size_payload);
        message->client_task = theInst->avrcp.vendor_task;
        message->op_id = op_id;
        message->size_payload = size_payload;
        memcpy(message->payload, payload, size_payload);
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_VENDOR_PASSTHROUGH_REQ, message, &appAvrcpGetLock(theInst));
    }
    else
    {
        MAKE_AV_MESSAGE(AV_AVRCP_VENDOR_PASSTHROUGH_CFM);
        message->av_instance = theInst;
        message->status = avrcp_device_not_connected;
        message->opid = op_id;
        MessageSend(theInst->avrcp.vendor_task, AV_AVRCP_VENDOR_PASSTHROUGH_CFM, message);
    }
}

/*! \brief Initialise AV instance

    This function initialises the specified AV instance, all state variables are
    set to defaults.  NOTE: This function should only be called on a newly created
    instance.

    \param theInst  Pointer to instance to initialise
*/
void appAvrcpInstanceInit(avInstanceTaskData *theInst)
{
    audio_source_t source = Av_GetSourceForInstance(theInst);
    
    /* Initialise state */
    theInst->avrcp.state = AVRCP_STATE_DISCONNECTED;
    theInst->avrcp.lock = 0;
    theInst->avrcp.notification_lock = 0;
    theInst->avrcp.playback_lock = 0;
    theInst->avrcp.bitfields.supported_events = 0;
    theInst->avrcp.bitfields.changed_events = 0;
    theInst->avrcp.bitfields.registered_events = 0;
    theInst->avrcp.vendor_task = NULL;
    theInst->avrcp.vendor_data = NULL;
    theInst->avrcp.vendor_opid = 0;
#ifdef INCLUDE_AV_SOURCE
    theInst->avrcp.play_status = avrcp_play_status_stopped;
    theInst->avrcp.play_hint = avrcp_play_status_stopped;
#else
    theInst->avrcp.play_status = avrcp_play_status_error;
    theInst->avrcp.play_hint = avrcp_play_status_error;
#endif
    theInst->avrcp.bitfields.volume_time_valid = FALSE;
    theInst->avrcp.bitfields.suppress_absolute_volume = FALSE;
    theInst->avrcp.connectionId = AV_CONN_ID_INVALID;
    theInst->avrcp.btConnId = 0;
    
    AudioSources_RegisterMediaControlInterface(source, AvrcpProfile_GetMediaControlInterface());
}

void appAvrcpHandleAvrcpLibMessage(Message message)
{
    CsrBtAvPrim *primType = (CsrBtAvPrim *)message;

    DEBUG_LOG("appAvrcpHandleAvrcpLibMessage, MESSAGE:CsrBtAvrcpPrim:0x%04X", *primType);

    switch(*primType)
    {
/***************************   Initialization messages ***********************************/
        case CSR_BT_AVRCP_CONFIG_CFM:
        {
            CsrBtAvrcpConfigCfm *cfm = (CsrBtAvrcpConfigCfm *)message;

            if (cfm->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS &&
                cfm->resultSupplier == CSR_BT_SUPPLIER_AVRCP)
            {
                appAvHandleAvrcpConfigConfirm();
            }
            else
            {
                appAvHandleAvrcpInitConfirm(FALSE);
            }
        }
        break;

        case CSR_BT_AVRCP_TG_MP_REGISTER_CFM:
        {
            CsrBtAvrcpTgMpRegisterCfm * cfm = (CsrBtAvrcpTgMpRegisterCfm *) message;

            if (cfm->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS &&
                cfm->resultSupplier == CSR_BT_SUPPLIER_AVRCP)
            {
                appAvrcpHandleAvrcpTgMpRegisterConfirm(cfm);
            }
            else
            {
                appAvHandleAvrcpInitConfirm(FALSE);
            }
        }
        break;

        case CSR_BT_AVRCP_ACTIVATE_CFM:
        {
            CsrBtAvrcpActivateCfm * cfm = (CsrBtAvrcpActivateCfm *) message;

            appAvHandleAvrcpInitConfirm(cfm->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS);
        }
        break;

/****************************   General Messages *****************************************/

        case CSR_BT_AVRCP_CONNECT_IND:
        {
             appAvrcpHandleAvrcpConnectInd((CsrBtAvrcpConnectInd *)message);
        }
        break;

        case CSR_BT_AVRCP_REMOTE_FEATURES_IND:
        {
             appAvrcpHandleAvrcpRemoteFeaturesInd((CsrBtAvrcpRemoteFeaturesInd *)message);
        }
        break;

        case CSR_BT_AVRCP_CONNECT_CFM:
        {
            appAvrcpHandleAvrcpConnectCfm((CsrBtAvrcpConnectCfm *)message);
        }
        break;

        case CSR_BT_AVRCP_DISCONNECT_IND:
        {
            appAvrcpHandleAvrcpDisconnectInd((CsrBtAvrcpDisconnectInd *)message);
        }
        break;

/***************************  CT messages *************************************************/

        case CSR_BT_AVRCP_CT_PASS_THROUGH_CFM:
        {
            appAvrcpHandleAvrcpPassthroughCfm((CsrBtAvrcpCtPassThroughCfm *)message);
        }
        break;

        case CSR_BT_AVRCP_CT_SET_VOLUME_CFM:
        {
            appAvrcpHandleSetAbsoluteVolumeCfm((CsrBtAvrcpCtSetVolumeCfm *)message);
        }
        break;

        case CSR_BT_AVRCP_CT_NOTI_REGISTER_CFM:
        {
            appAvrcpHandleRegisterNotificationCfm((CsrBtAvrcpCtNotiRegisterCfm *)message);
        }
        break;

/***************************  CT Notifications received ***********************************/

        case CSR_BT_AVRCP_CT_NOTI_PLAYBACK_STATUS_IND:
        {
            appAvrcpHandleEventPlaybackStatusChangedInd((CsrBtAvrcpCtNotiPlaybackStatusInd *)message);
        }
        break;

        case CSR_BT_AVRCP_CT_NOTI_VOLUME_IND:
        {
            appAvrcpHandleEventVolumeChangedInd((CsrBtAvrcpCtNotiVolumeInd *)message);
        }
        break;

/***************************  TG messages *************************************************/
        case CSR_BT_AVRCP_TG_SET_VOLUME_IND:
        {
            appAvrcpHandleSetAbsoluteVolumeInd((CsrBtAvrcpTgSetVolumeInd *)message);
        }
        break;
        
        case CSR_BT_AVRCP_TG_PASS_THROUGH_IND:
        {
            appAvrcpHandlePassThroughInd((CsrBtAvrcpTgPassThroughInd *)message);
        }
        break;

        case CSR_BT_AVRCP_TG_GET_ATTRIBUTES_IND:
        {        
            AvrcpMetadata_HandleGetElementAttributesInd(AvInstance_GetInstanceForSearchId(((AVRCP_GET_ELEMENT_ATTRIBUTES_IND_T*)message)->connectionId , TRUE, DEVICE_PROFILE_AVRCP), ((AVRCP_GET_ELEMENT_ATTRIBUTES_IND_T*)message));
        }
        break;

        case CSR_BT_AVRCP_TG_GET_PLAY_STATUS_IND:
        {
            AvrcpMetadata_HandleGetPlayStatusInd(AvInstance_GetInstanceForSearchId(((AVRCP_GET_PLAY_STATUS_IND_T*)message)->connectionId , TRUE, DEVICE_PROFILE_AVRCP), ((AVRCP_GET_PLAY_STATUS_IND_T*)message));
        }
        break;

        case CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_IND:
        {
            AvrcpBrowsing_HandleGetFolderItemsInd(AvInstance_GetInstanceForSearchId(((AVRCP_BROWSE_GET_FOLDER_ITEMS_IND_T*)message)->connectionId , TRUE, DEVICE_PROFILE_AVRCP), ((AVRCP_BROWSE_GET_FOLDER_ITEMS_IND_T*)message));
        }
        break;
        
        case CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_IND:
        {
            AvrcpBrowsing_HandleGetNumberOfItemsInd(AvInstance_GetInstanceForSearchId(((AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_IND_T*)message)->connectionId , TRUE, DEVICE_PROFILE_AVRCP), ((AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_IND_T*)message));
        }
        break;
        
        case CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_IND:
        {
            AvrcpBrowsing_HandleSetAddressedPlayerInd(AvInstance_GetInstanceForSearchId(((AVRCP_SET_ADDRESSED_PLAYER_IND_T*)message)->connectionId , TRUE, DEVICE_PROFILE_AVRCP), ((AVRCP_SET_ADDRESSED_PLAYER_IND_T*)message));
        }
        break;
        /* indication to notify when the controller regsiters for specific notification identifiers*/
        case CSR_BT_AVRCP_TG_NOTI_IND:
        {
            appAvrcpHandleRegisterNotificationInd((CsrBtAvrcpTgNotiInd *)message);
        }
        break;
/*******************************************************************************************/

        default:
        break;
    }

    AvrcpFreeUpstreamMessageContents((void *)message);
}

#else
static const int compiler_happy;
#endif
