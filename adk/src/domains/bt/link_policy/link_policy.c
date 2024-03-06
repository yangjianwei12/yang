/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\ingroup    link_policy
\brief      Link policy manager general functionality and initialisation.
*/

#include "adk_log.h"
#include "link_policy_private.h"

#include <message.h>
#include <panic.h>
#include <bdaddr.h>
#include <bt_device.h>
#include <connection_manager.h>
#include <dm_prim.h>
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
#include <le_audio_messages.h>
#endif

LOGGING_PRESERVE_MESSAGE_TYPE(link_policy_internal_message_t)


/*!< Link Policy Manager data structure */
lpTaskData  app_lp = {0};

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;

static void appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(const bdaddr *bd_addr, uint8 override_value)
{
#ifdef USE_SYNERGY
    CsrBtDeviceAddr DeviceAddr;
    BdaddrConvertVmToBluestack(&DeviceAddr, bd_addr);

    CmWriteScHostSupportOverrideRequest(LinkPolicyGetTask(), &DeviceAddr, override_value);
#else
    ConnectionDmSecureConnectionsOverrideReq(LinkPolicyGetTask(), bd_addr, override_value);
#endif

    DEBUG_LOG("appLinkPolicyBredrSecureConnectionHostSupportOverrideSet 0x%x:%d", bd_addr->lap, override_value);
}

void appLinkPolicyHandleAddressSwap(void)
{
    typed_bdaddr bd_addr_primary = {.type = TYPED_BDADDR_PUBLIC, .addr = {0}};
    typed_bdaddr bd_addr_secondary = {.type = TYPED_BDADDR_PUBLIC, .addr= {0}};

    PanicFalse(appDeviceGetPrimaryBdAddr(&bd_addr_primary.addr));
    PanicFalse(appDeviceGetSecondaryBdAddr(&bd_addr_secondary.addr));
    PanicFalse(!BdaddrIsSame(&bd_addr_primary.addr, &bd_addr_secondary.addr));

#ifdef INCLUDE_SM_PRIVACY_1P2
    ConnectionDmUlpSetPrivacyModeReq(&bd_addr_primary, privacy_mode_device);
    ConnectionDmUlpSetPrivacyModeReq(&bd_addr_secondary, privacy_mode_device);
#endif

    /* By default, BR/EDR secure connections is disabled.
    TWM requires the link between the two earbuds to have BR/EDR secure connections
    enabled, so selectively enable SC for connections to the other earbud.
    The addresses of both earbuds need to be overridden, as the addresses of the
    two devices swap during handover. Handover will fail if both addresses
    are not overridden. */
    appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(&bd_addr_primary.addr, 0x01);
    appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(&bd_addr_secondary.addr, 0x01);
}

static void LinkPolicy_HandleDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T * ind)
{
    const bdaddr * addr = &ind->tpaddr.taddr.addr;

    if (ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
    {
        if (appDeviceIsHandset(addr))
        {
            appLinkPolicyUpdatePowerTable(addr);
        }
    }
}

static void LinkPolicy_HandleConnectInd(const CON_MANAGER_TP_CONNECT_IND_T * ind)
{
    if (ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        tp_bdaddr bredraddress = {.transport=TRANSPORT_BREDR_ACL,
                                  .taddr.type = TYPED_BDADDR_PUBLIC,
                                  .taddr.addr=ind->tpaddr.taddr.addr};
        lpPerConnectionState bredr_state;
        lpPerConnectionState le_state;

        /* See if we have a stored state for the LE Link */
        if (ConManagerGetLpStateTp(&ind->tpaddr, &le_state))
        {
            if (le_state.pt_index != POWERTABLE_UNASSIGNED)
            {
                DEBUG_LOG_WARN("LinkPolicy_HandleConnectInd  LE Status was set ?");
                return;
            }
        }

        if (ConManagerGetLpStateTp(&bredraddress, &bredr_state))
        {
            if (bredr_state.pt_index == POWERTABLE_A2DP_STREAMING)
            {
                DEBUG_LOG_INFO("LinkPolicy_HandleConnectInd BREDR Streaming");
                le_state = bredr_state;
                ConManagerRequestDeviceQos(&ind->tpaddr, cm_qos_lea_idle);
                ConManagerSetLpStateTp(&ind->tpaddr, le_state);
            }
        }
        
    }
}

#if defined(INCLUDE_LEA_LINK_POLICY) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
static void appLinkPolicyHandleLeAudioConnected(const LE_AUDIO_BROADCAST_CONNECTED_T *msg)
{
    UNUSED(msg);

    DEBUG_LOG("appLinkPolicyHandleLeAudioConnected");

    appLinkPolicyForceUpdatePowerTable(NULL);
}

static void appLinkPolicyHandleLeAudioDisconnected(const LE_AUDIO_BROADCAST_DISCONNECTED_T *msg)
{
    UNUSED(msg);

    DEBUG_LOG("appLinkPolicyHandleLeAudioDisconnected");

    appLinkPolicyForceUpdatePowerTable(NULL);
}

/* Re-route callback from connection manager to our own callback function */
static bool linkPolicy_CheckLeParams(const tp_bdaddr *tpaddr, uint16 *min, uint16 *max)
{
    return app_lp.parameter_adjust_callbacks.LeParams(tpaddr, min, max);
}

static con_manager_connparams_callback_t linkPolicy_LeParamsCallback = 
                { .LeParams = linkPolicy_CheckLeParams };

#endif /* LEA_LINK_POLICY && (BROADCAST||UNICAST)*/

void LinkPolicy_SetParameterCallbacks(link_policy_parameter_callbacks_t *callback)
{
    PanicFalse(callback->LeParams || callback->BredrParams);

#if defined(INCLUDE_LEA_LINK_POLICY) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
    if (callback->LeParams)
    {
        PanicNotNull((void*)app_lp.parameter_adjust_callbacks.LeParams);
        ConManager_SetConnParamCallback(&linkPolicy_LeParamsCallback);
    }
    if (callback->BredrParams)
    {
        PanicNotNull((void*)app_lp.parameter_adjust_callbacks.BredrParams);
    }
    app_lp.parameter_adjust_callbacks = *callback;
#endif
}


static void appLinkPolicyMessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case CON_MANAGER_TP_DISCONNECT_IND:
            LinkPolicy_HandleDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)msg);
            break;

        case CON_MANAGER_TP_CONNECT_IND:
            LinkPolicy_HandleConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *)msg);
            break;

        case LINK_POLICY_SCHEDULED_UPDATE:
            appLinkPolicyForceUpdatePowerTable(NULL);
            break;

        case LINK_POLICY_A2DP_TERMINATED_DELAY_EXPIRED:
            appLinkPolicyHandleA2dpTerminatedDelayExpired();
            break;

        case LINK_POLICY_DISCOVER_ROLE:
            appLinkPolicyHandleDiscoverRole((const LINK_POLICY_DISCOVER_ROLE_T *)msg);
            break;

#if defined(INCLUDE_LEA_LINK_POLICY) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
        case LE_AUDIO_BROADCAST_CONNECTED:
            appLinkPolicyHandleLeAudioConnected((const LE_AUDIO_BROADCAST_CONNECTED_T *)msg);
            break;

        case LE_AUDIO_BROADCAST_DISCONNECTED:
            appLinkPolicyHandleLeAudioDisconnected((const LE_AUDIO_BROADCAST_DISCONNECTED_T *)msg);
            break;
#endif /* LEA_LINK_POLICY && (BROADCAST || UNICAST) */

        default:
#ifdef USE_SYNERGY
            appLinkPolicyHandleCMMessage(task, id, msg);
            break;
#else
            appLinkPolicyHandleConnectionLibraryMessages(id, msg, FALSE);
            break;
#endif
    }
}

void appLinkPolicyUpdatePowerTableDeferred(const bdaddr *bd_addr)
{
    UNUSED(bd_addr);

    DEBUG_LOG("appLinkPolicyUpdatePowerTableDeferred");

    MessageSend(LinkPolicyGetTask(), LINK_POLICY_SCHEDULED_UPDATE, NULL);
}

/*! \brief Initialise link policy manager. */
bool appLinkPolicyInit(Task init_task)
{
    lpTaskData *theLp = LinkPolicyGetTaskData();
    theLp->task.handler = appLinkPolicyMessageHandler;
    cm_transport_t transports = cm_transport_bredr;

#if defined(INCLUDE_LEA_LINK_POLICY)
    transports |= cm_transport_ble;
#endif

    ConManagerRegisterTpConnectionsObserver(transports, &theLp->task);

#ifdef USE_SYNERGY
    CmSetEventMaskReqSend(&LinkPolicyGetTaskData()->task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE |
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
#endif

#if defined(INCLUDE_LEA_LINK_POLICY) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
    LeAudioMessages_ClientRegister(&theLp->task);
#endif

    UNUSED(init_task);
    return TRUE;
}

#ifdef USE_SYNERGY
void appLinkPolicySetMode(const bdaddr *bd_addr,
                          link_policy_settings_t link_policy_settings)
{
    CsrBtDeviceAddr addr = { 0 };

    BdaddrConvertVmToBluestack(&addr, bd_addr);
    CmWriteLinkPolicyReqSend(NULL, addr, link_policy_settings, TRUE, NULL);
}
#endif

