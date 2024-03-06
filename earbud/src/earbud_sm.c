/*!
\copyright  Copyright (c) 2005 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application state machine
*/

/* local includes */
#include "app_task.h"
#include "earbud_sm.h"
#include "earbud_sm_marshal_defs.h"
#include "system_state.h"
#include "earbud_sm_private.h"
#include "earbud_led.h"
#include "adk_log.h"
#include "earbud_rules_config.h"
#include "earbud_common_rules.h"
#include "earbud_primary_rules.h"
#include "earbud_secondary_rules.h"
#include "earbud_config.h"
#include "earbud_production_test.h"
#include "earbud_hardware.h"
#include "gaming_mode.h"
#include "earbud_tones.h"
#include "gaia.h"
#include "charger_monitor.h"
#ifdef INCLUDE_SENSOR_PROFILE
#include "spatial_data.h"
#endif
#ifdef ENABLE_LE_DEBUG_SECONDARY
#include "earbud_secondary_debug.h"
#endif
#include "timestamp_event.h"

/* framework includes */
#include <volume_service.h>
#include <domain_message.h>
#include <state_proxy.h>
#include <pairing.h>
#include <power_manager.h>
#include <gaia_framework.h>
#include <dfu.h>
#include <dfu_peer.h>
#include <dfu_case.h>
#include <device_db_serialiser.h>
#include <gatt_handler.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <connection_manager_config.h>
#include <handset_service.h>
#include <hfp_profile.h>
#include <logical_input_switch.h>
#include <ui.h>
#include <peer_signalling.h>
#include <key_sync.h>
#include <gatt_server_gatt.h>
#include <device_list.h>
#include <device_properties.h>
#include <profile_manager.h>
#include <tws_topology.h>
#include <peer_find_role.h>
#include <unexpected_message.h>
#include <ui_indicator_prompts.h>
#include <ui_indicator_tones.h>
#include <user_accounts.h>
#include <device_test_service.h>
#include <device_test_service_config.h>
#include <hfp_profile_config.h>
#include <cc_with_case.h>
#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include <le_broadcast_manager.h>
#endif
#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
#include <le_audio_volume_sync.h>
#include <le_audio_messages.h>
#endif
#ifdef INCLUDE_LE_AUDIO_UNICAST
#include <le_unicast_manager.h>
#endif
#include <le_scan_manager.h>
#include <touch.h>
#include <proximity.h>
#ifdef PRODUCTION_TEST_MODE
#include <volume_mute.h>
#endif
#include <mirror_profile.h>
#ifdef INCLUDE_SENSOR_PROFILE
#include <sensor_profile.h>
#endif
#include <system_reboot.h>
#include <cc_with_case.h>
#include <audio_info.h>

#ifdef INCLUDE_FAST_PAIR
#include <fast_pair.h>
#endif


/* system includes */
#include <panic.h>
#ifdef USE_SYNERGY
#include <csr_bt_td_db.h>
#endif
#include <connection.h>
#include <ps.h>
#include <boot.h>
#include <message.h>
#include <system_clock.h>
#include <ps_key_map.h>
#include <cc_with_case.h>
#include <state_of_charge.h>

#ifdef INCLUDE_ACCESSORY_TRACKING
#include <accessory_tracking.h>
#endif

#include "earbud_topology_default.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(sm_internal_message_ids)
LOGGING_PRESERVE_MESSAGE_TYPE(earbud_sm_topology_role_changed_t)

PRESERVE_TYPE_FOR_DEBUGGING(smPostDisconnectAction)

/*! \name Connection library factory reset keys 

    These keys should be deleted during a factory reset.
*/
/*! @{ */
#ifdef USE_SYNERGY
#define CSR_BT_PS_KEY_SYSTEM  100
#else
#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define GATT_ATTRIBUTE_BASE_PSKEY_INDEX  110
#define PSKEY_SM_KEY_STATE_IR_ER_INDEX 140
#define TDL_BASE_PSKEY_INDEX        142
#define TDL_INDEX_PSKEY             141
#define TDL_SIZE                    BtDevice_GetMaxTrustedDevices()
#endif
/*! @} */

/*!< Application state machine. */
smTaskData app_sm;

static uint16 start_ps_free_count = 0;

const message_group_t sm_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
    UI_INPUTS_DEVICE_STATE_MESSAGE_GROUP
};

#ifdef INCLUDE_DFU
/*
Ideal flow of states for DFU is as follows:
APP_GAIA_CONNECTED
    ->APP_GAIA_UPGRADE_CONNECTED
        ->APP_GAIA_UPGRADE_ACTIVITY
            ->APP_GAIA_UPGRADE_DISCONNECTED
                ->APP_GAIA_DISCONNECTED

where,
    APP_GAIA_CONNECTED: A GAIA connection has been made
    APP_GAIA_DISCONNECTED: A GAIA connection has been lost
    APP_GAIA_UPGRADE_CONNECTED: An upgrade protocol has connected through GAIA
    APP_GAIA_UPGRADE_DISCONNECTED:An upgrade protocol has disconnected through GAIA
    APP_GAIA_UPGRADE_ACTIVITY: The GAIA module has seen some upgrade activity
*/
static void appSmEnterDfuOnStartup(bool upgrade_reboot, bool hasDynamicRole);
static void appSmNotifyUpgradeStarted(void);
static void appSmStartDfuTimer(bool hasDynamicRole);

static void appSmHandleDfuAborted(void);
static void appSmHandleDfuEnded(bool error);
static void appSmDFUEnded(void);
static void appSmCheckAndNotifyDeviceNotInUse(void);
static inline void appEnableKeepTopologyAliveForDfu(void);
static inline void appDisableKeepTopologyAliveForDfu(void);

bool keep_topology_alive_for_dfu = FALSE;
#endif /* INCLUDE_DFU */
static unsigned earbudSm_GetCurrentApplicationContext(void);

/*****************************************************************************
 * SM utility functions
 *****************************************************************************/
static bool earbudSm_OutOfCaseAndEar(void)
{
    if ((appSmIsOutOfCase() && appSmIsOutOfEar()) 
        || (StateProxy_IsPeerOutOfEar() && StateProxy_IsPeerOutOfCase()))
    {
        return TRUE;
    }
    return FALSE;
}

static void earbudSm_SendCommandToPeer(marshal_type_t type)
{
    earbud_sm_msg_empty_payload_t* msg = PanicUnlessMalloc(sizeof(earbud_sm_msg_empty_payload_t));
    appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                     PEER_SIG_MSG_CHANNEL_APPLICATION,
                                     msg, type);
}

static void earbudSm_CancelAndSendCommandToPeer(marshal_type_t type)
{
    appPeerSigMarshalledMsgChannelTxCancelAll(SmGetTask(),
                                              PEER_SIG_MSG_CHANNEL_APPLICATION,
                                              type);
    earbudSm_SendCommandToPeer(type);

}

/*! \brief Set event on active rule set */
static void appSmRulesSetEvent(rule_events_t event)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_SetEvent(event);
            break;
        case tws_topology_role_primary:
            CommonRules_SetEvent(event);
            PrimaryRules_SetEvent(event);
            break;
        case tws_topology_role_secondary:
            CommonRules_SetEvent(event);
            SecondaryRules_SetEvent(event);
            break;
        default:
            break;
    }
}
#ifdef INCLUDE_CASE_COMMS
/*! \brief Handle the messages received from the case */
static void appSmHandleCaseMessages(void){
    if(CcWithCase_GetLidState() == CASE_LID_STATE_CLOSED)
    {
        LedManager_RestartPattern();
    }
}
#endif

/*! \brief Reset event on active rule set */
static void appSmRulesResetEvent(rule_events_t event)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_ResetEvent(event);
            break;
        case tws_topology_role_primary:
            CommonRules_ResetEvent(event);
            PrimaryRules_ResetEvent(event);
            break;
        case tws_topology_role_secondary:
            CommonRules_ResetEvent(event);
            SecondaryRules_ResetEvent(event);
            break;
        default:
            break;
    }
}

/*! \brief Mark rule action complete on active rule set */
static void appSmRulesSetRuleComplete(MessageId message)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_SetRuleComplete(message);
            break;
        case tws_topology_role_primary:
            CommonRules_SetRuleComplete(message);
            PrimaryRules_SetRuleComplete(message);
            break;
        case tws_topology_role_secondary:
            CommonRules_SetRuleComplete(message);
            SecondaryRules_SetRuleComplete(message);
            break;
        default:
            break;
    }
}

#ifdef INCLUDE_DFU
static void appSmCancelDfuTimers(void)
{
    DEBUG_LOG_DEBUG("appSmCancelDfuTimers Cancelled all SM_INTERNAL_TIMEOUT_DFU_... timers");

    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_ENTRY);
    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_MODE_START);
    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT);
}
#endif

static void appSmClearPsStore(void)
{
    DEBUG_LOG_FN_ENTRY("appSmClearPsStore");

#ifdef USE_SYNERGY
    CsrBtTdDbDeleteAll(CSR_BT_TD_DB_FILTER_EXCLUDE_NONE);
    PsStore(CSR_BT_PS_KEY_SYSTEM, NULL, 0);
#else
    for (int i=0; i<TDL_SIZE; i++)
    {
        PsStore(ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(TDL_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(GATT_ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
    }

    PsStore(TDL_INDEX_PSKEY, NULL, 0);
    PsStore(PSKEY_SM_KEY_STATE_IR_ER_INDEX, NULL, 0);
#endif

#ifdef INCLUDE_DEVICE_DB_BACKUP
    PsStore(PS_KEY_EARBUD_DEVICES_BACKUP, NULL, 0);
#endif

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    DeviceTestService_ClearPsStore();
#endif
    PsStore(PS_KEY_HFP_CONFIG, NULL, 0);

#ifdef INCLUDE_DFU
    /* Clear out any in progress DFU status */
    Dfu_ClearPsStore();
#endif
}

/*! \brief Delete all peer and handset pairing and reboot device. */
static void appSmDeletePairingAndReset(void)
{
    DEBUG_LOG_ALWAYS("appSmDeletePairingAndReset");

    /* cancel the link disconnection, may already be gone if it fired to get us here */
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    /* Force a defrag before updating any PS Keys */
    PsDefragBlocking();

    appSmClearPsStore();

#ifdef INCLUDE_FAST_PAIR

    /* Delete the account keys */
    UserAccounts_DeleteAllAccountKeys();    

    /* Delete the personalized name */
    FastPair_DeletePName();

#endif

    SystemReboot_Reboot();
}

/*! \brief Disconnect the specified links.
    \return Bitfield of links that should be disconnected.
            If non-zero, caller needs to wait for link disconnection to complete
 * */
static smDisconnectBits appSmDisconnectLinks(smDisconnectBits links_to_disconnect)
{
    smDisconnectBits disconnecting_links = 0;

    DEBUG_LOG_DEBUG("appSmDisconnectLinks links_to_disconnect 0x%x", links_to_disconnect);

    if (links_to_disconnect & SM_DISCONNECT_HANDSET)
    {
        if (HandsetService_IsAnyDeviceConnected())
        {
            HandsetService_DisconnectAll(SmGetTask(), HCI_ERROR_OETC_USER);
            disconnecting_links |= SM_DISCONNECT_HANDSET;
        }
    }
    return disconnecting_links;
}


/*! \brief Determine which state to transition to based on physical state.
    \return appState One of the states that correspond to a physical earbud state.
*/
static appState appSmCalcCoreState(void)
{
    bool busy = (AudioInfo_GetRoutedGenericSource().type != source_type_invalid);

    switch (appSmGetPhyState())
    {
        case PHY_STATE_IN_CASE:
            return APP_STATE_IN_CASE_IDLE;

        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            return busy ? APP_STATE_OUT_OF_CASE_BUSY :
                          APP_STATE_OUT_OF_CASE_IDLE;

        case PHY_STATE_IN_EAR:
            return busy ? APP_STATE_IN_EAR_BUSY :
                          APP_STATE_IN_EAR_IDLE;

        /* Physical state is unknown, default to in-case state */
        case PHY_STATE_UNKNOWN:
            return APP_STATE_IN_CASE_IDLE;

        default:
            Panic();
    }

    return APP_STATE_IN_EAR_IDLE;
}

static void appSmSetCoreStateDiscardPairing(void)
{
    appState state = appSmCalcCoreState();
    if (state != appSmGetState())
    {
        appSmSetState(state);
    }
}

static void appSmSetCoreStatePreservePairing(void)
{
    appState current_state = appSmGetState();
    appState new_state = appSmCalcCoreState();

    if ((current_state & APP_SUBSTATE_HANDSET_PAIRING) != 0)
    {
        new_state |= APP_SUBSTATE_HANDSET_PAIRING;
    }

    if (new_state != current_state)
    {
        appSmSetState(new_state);
    }
}

/*! \brief Set the core app state for the first time. */
static void appSmSetInitialCoreState(void)
{
    bool prompts_enabled = FALSE;
    phyState current_phy_state;
    smTaskData *sm = SmGetTaskData();

    /* Register with physical state manager to get notification of state
     * changes such as 'in-case', 'in-ear' etc */
    appPhyStateRegisterClient(&sm->task);

    /* Get latest physical state */
    sm->phy_state = appPhyStateGetState();

    /* Generate physical state events */
    switch (sm->phy_state)
    {
        case PHY_STATE_IN_CASE:
            appSmRulesSetEvent(RULE_EVENT_IN_CASE);
            break;

        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_OUT_OF_EAR:
            appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
            appSmRulesSetEvent(RULE_EVENT_OUT_EAR);
            break;

        case PHY_STATE_IN_EAR:
            appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
            appSmRulesSetEvent(RULE_EVENT_IN_EAR);
            break;

        default:
            Panic();
    }
    
    /* Prompts should be enabled based on the current physical state
     * rather than the reported physical state. Otherwise power on
     * prompt may not get played even if earbud is in ear because of
     * the delay in reporting the current state by phy_state module.
     */
    current_phy_state = appPhyStateGetCurrentState();
    if (current_phy_state == PHY_STATE_IN_EAR)
    {
        prompts_enabled = TRUE;
    }

    UiTones_SetTonePlaybackEnabled(prompts_enabled);
    UiPrompts_SetPromptPlaybackEnabled(prompts_enabled);

    /* Update core state */
    appSmSetCoreStateDiscardPairing();
}


static void appSmPeerStatusSetWasPairingEnteringCase(void)
{
    smTaskData *sm = SmGetTaskData();

    DEBUG_LOG_VERBOSE("appSmPeerStatusSetWasPairingEnteringCase");

    sm->peer_was_pairing_when_entered_case = TRUE;
    MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_PEER_WAS_PAIRING, NULL,
                     appConfigPeerInCaseIndicationTimeoutMs());
}

static void appSmPeerStatusClearWasPairingEnteringCase(void)
{
    smTaskData *sm = SmGetTaskData();

    if (appSmPeerWasPairingWhenItEnteredCase())
    {
        DEBUG_LOG_INFO("appSmPeerStatusClearWasPairingEnteringCase. Was set");
    }

    sm->peer_was_pairing_when_entered_case = FALSE;
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_PEER_WAS_PAIRING);
}

static void appSmSendDisconnectCompleteMsg(smPostDisconnectAction post_disconnect_action)
{
    MESSAGE_MAKE(msg, SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T);

    msg->post_disconnect_action = post_disconnect_action;
    MessageSendConditionally(SmGetTask(), SM_INTERNAL_LINK_DISCONNECTION_COMPLETE,
                             msg, &appSmDisconnectLockGet());
}

/*! \brief Initiate disconnect of links */
static void appSmInitiateLinkDisconnection(smDisconnectBits links_to_disconnect, uint16 timeout_ms,
                                           smPostDisconnectAction post_disconnect_action)
{
    smDisconnectBits disconnecting_links = appSmDisconnectLinks(links_to_disconnect);
    if (!disconnecting_links)
    {
        appSmDisconnectLockClearLinks(SM_DISCONNECT_ALL);
    }
    else
    {
        appSmDisconnectLockSetLinks(disconnecting_links);
    }
    appSmSendDisconnectCompleteMsg(post_disconnect_action);

    /* Start a timer to force reset if we fail to complete disconnection */
    MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION, NULL, timeout_ms);
}

/*! \brief Cancel A2DP and SCO out of ear timers. */
static void appSmCancelOutOfEarTimers(void)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_MEDIA);
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO);
    PrimaryRules_SetRuleComplete(CONN_RULES_MEDIA_TIMEOUT_CANCEL);
}

/*! \brief Determine if we have an Media restart timer pending. */
bool appSmIsMediaRestartPending(void)
{
    /* try and cancel the Media auto restart timer, if there was one
     * then we're inside the time required to automatically restart
     * audio due to earbud being put back in the ear, so we need to
     * send a play request to the handset */
    return MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_MEDIA_START);
}

/*! \brief Deliberately disable the touchpad till it enters the case. */
static void appSmDisableTouchpad(void)
{
    SmGetTaskData()->touchpad_deliberately_disabled = TRUE;
    TouchSensor_Reset(TRUE);
}

/*****************************************************************************
 * SM state transition handler functions
 *****************************************************************************/

/*! \brief Enter APP_STATE_DFU_CHECK. */

static void appEnterDfuCheck(void)
{
#ifdef INCLUDE_DFU
    /* We only get into DFU check, if there wasn't a pending DFU
       So send a message to go to startup. */
    switch(Dfu_GetRebootReason())
    {
        case REBOOT_REASON_ABRUPT_RESET:
            /*
             * Indicates abrupt reset as DFU_REQUESTED_IN_PROGRESS
             * received from upgrade library.
             */

            DEBUG_LOG_DEBUG("appEnterDfuCheck - Resume DFU post upgrade interrupted owing to abrupt reset.");

            appEnableKeepTopologyAliveForDfu();

            /*
             * Defer resume of DFU (if applicable), until roles are established.
             * Topology is made DFU aware based on the pskey info maintained
             * as is_dfu_mode. So allow Topology to evaluate if a role selection
             * is required to establish roles before DFU can progress further.
             *
             * APP_STATE_IN_CASE_DFU is no longer used/entered to progress DFU
             * interrupted by an abrupt reset because Topology now finalizes
             * a dynamic role rather than fixed role. In case of fixed roles,
             * APP_STATE_IN_CASE_DFU was required to activate the appropriate
             * topology goals based on fixed roles.
             */
            appSmSetState(APP_STATE_STARTUP);
            break;

        case REBOOT_REASON_DFU_RESET:
            /*
             * Indicates defined reset entering into post reboot DFU commit
             * phase, as DFU_REQUESTED_TO_CONFIRM received from upgrade library.
             */

            DEBUG_LOG_DEBUG("appEnterDfuCheck - Resume DFU post defined reboot phase of upgrade.");

            appEnableKeepTopologyAliveForDfu();

            /*
             * Defer resume of DFU (if applicable), until roles are established.
             * Topology is made DFU aware based on the pskey info maintained
             * as is_dfu_mode. So allow Topology to evaluate if a role selection
             * is required to establish roles before DFU can progress further.
             *
             * APP_STATE_IN_CASE_DFU is no longer used/entered to progress DFU
             * in the post reboot DFU commit phase because Topology now finalizes
             * a dynamic role rather than fixed role. Though APP_STATE_IN_CASE_DFU
             * is no longer used, still the DFU timers are activated at
             * appropriate stages while entered into APP_STATE_STARTUP so that
             * DFU can end cleanly on timeout if the DFU commit phase doesn't
             * complete within the expection time period.
             *
             * 2nd TRUE indicates to skip entering APP_STATE_IN_CASE_DFU and
             * instead manage the DFU timers as part of entering
             * APP_STATE_STARTUP.
             */

            appSmEnterDfuOnStartup(TRUE, TRUE);

            appSmSetState(APP_STATE_STARTUP);
            break;

         default:
            DEBUG_LOG_DEBUG("appEnterDfuCheck -No DFU");
            MessageSend(SmGetTask(), SM_INTERNAL_NO_DFU, NULL);
            break;
    }
#else
    DEBUG_LOG_DEBUG("appEnterDfuCheck -No DFU");
    MessageSend(SmGetTask(), SM_INTERNAL_NO_DFU, NULL);
#endif
}


/*! \brief Enter APP_STATE_FACTORY_RESET.
    \note The application will delete all pairing and reboot.
 */
static void appEnterFactoryReset(void)
{
#ifdef INCLUDE_ACCESSORY_TRACKING
    AccessoryTrackingFactoryReset();
#endif
    appSmDeletePairingAndReset();
}

/*! \brief Exit APP_STATE_FACTORY_RESET. */
static void appExitFactoryReset(void)
{
    /* Should never happen */
    Panic();
}

/*! \brief Enter APP_STATE_DEVICE_TEST_MODE. */
static void appExitDeviceTestMode(void)
{
    DeviceTestService_Stop(SmGetTask());
}

/*! \brief Exit APP_STATE_DEVICE_TEST_MODE. */
static void appEnterDeviceTestMode(void)
{
#if defined (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS)
    /* Some host systems expect DTS to be the only SPP service, so
     * we should stop the GAIA service when starting DTS.
     */
    gaia_transport_index index = 0;
    gaia_transport *t = Gaia_TransportFindService(gaia_transport_spp, &index);
    if (t)
    {
        Gaia_TransportStopService(t);
    }
#else   /* (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS) */
    DeviceTestService_Start(SmGetTask());
    appSmTestService_SetTestStep(1);
#endif  /* (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS) */
}

/*! \brief Enter APP_STATE_STARTUP. */
static void appEnterStartup(void)
{
    TwsTopology_ConfigureProductBehaviour(EarbudTopologyDefault_GetConfig());
    TwsTopology_Start();
}

/*! \brief Enter APP_STATE_HANDSET_PAIRING. */
static void appEnterHandsetPairing(void)
{
    if (appSmIsPrimary())
    {
        HandsetService_ConnectableRequest(SmGetTask());
        HandsetService_PairHandset(SmGetTask(), appSmIsUserPairing());
    }
}

/*! \brief Exit APP_STATE_HANDSET_PAIRING. */
static void appExitHandsetPairing(appState new_state)
{
    if (new_state == APP_STATE_IN_CASE_IDLE && PairingIsDiscoverable())
    {
        /* If handover occurs shortly after this in-case transition then this call will tell the 
           new primary to enter pairing mode when handover completes. */
        earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_ind_gone_in_case_while_pairing_t));
    }

    appSmRulesSetRuleComplete(CONN_RULES_HANDSET_PAIR);
    HandsetService_CancelPairHandset(NULL);
    appSmClearUserPairing();
}

/*! \brief Enter APP_STATE_IN_CASE_DFU. */
#ifdef INCLUDE_DFU
static void appEnterInCaseDfu(void)
{
    bool in_case = !appPhyStateIsOutOfCase();
    bool secondary;

    secondary = !BtDevice_IsMyAddressPrimary();

    DEBUG_LOG_DEBUG("appEnterInCaseDfu Case:%d Primary:%d", in_case, !secondary);

    Dfu_GetTaskData()->enter_dfu_mode = FALSE;

    if(secondary)
    {
        /*! \todo assuming that when we have entered case as secondary,
            it is appropriate to cancel DFU timeouts

            Exiting the case will leave DFU mode cleanly */
        if (in_case)
        {
            appSmCancelDfuTimers();
        }
    }

}

/*! \brief Exit APP_STATE_IN_CASE_DFU

  * NOTE: ONLY State variable reset done here need to be reset in
          appSmHandleDfuAborted() as well for abort/error notification
          scenarios, even if redundantly done except for topology event
          injection.
 */
static void appExitInCaseDfu(void)
{
    appSmCancelDfuTimers();

    /* If DFU is aborted due to SYNC ID mismatch, then retain the profiles
     * for handling the subsequent SYNC request
     */
    if(!UpgradeIsAbortDfuDueToSyncIdMismatch())
    {
        appDisableKeepTopologyAliveForDfu();
    }
    TwsTopology_EnableRoleSwapSupport();
    appSmDFUEnded();
}
#endif

/*! \brief Enter APP_STATE_OUT_OF_CASE_IDLE. */
static void appEnterOutOfCaseIdle(void)
{
    Ui_InformContextChange(ui_provider_phy_state, context_app_sm_out_of_case_idle);

    if (appConfigOutOfCaseIdleTimeoutMs())
    {
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE, NULL, appConfigOutOfCaseIdleTimeoutMs());
    }
}

static void appBeginInCasePowerDown(void)
{
    /* Don't start the Idle timeout unless we have stopped charging */
    if (appConfigInCaseIdleTimeoutMs())
    {
        if (!Charger_IsCharging())
        {
            MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE, NULL, appConfigInCaseIdleTimeoutMs());
        }
    }
}

static void appChargerCompleted(void)
{
    DEBUG_LOG_INFO("appChargerCompleted, state = %d", appSmGetState());
    if (appSmGetState() == APP_STATE_IN_CASE_IDLE)
    {
        appBeginInCasePowerDown();
    }
}

/*! \brief Enter APP_STATE_IN_CASE_IDLE. */
static void appEnterInCaseIdle(void)
{
    appBeginInCasePowerDown();
}

/*! \brief Exit APP_STATE_IN_CASE_IDLE. */
static void appExitInCaseIdle(void)
{
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
}

/*! \brief Exit APP_STATE_OUT_OF_CASE_IDLE. */
static void appExitOutOfCaseIdle(void)
{
    /* Stop idle on LEDs */
    Ui_InformContextChange(ui_provider_phy_state, context_app_sm_out_of_case_busy);

    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
}

/*! \brief Common actions when entering one of the APP_SUBSTATE_TERMINATING substates. */
static void appEnterSubStateTerminating(void)
{
    DEBUG_LOG_DEBUG("appEnterSubStateTerminating");
    DeviceDbSerialiser_Serialise();
    TwsTopology_Stop();
}

/*! \brief Common actions when exiting one of the APP_SUBSTATE_TERMINATING substates. */
static void appExitSubStateTerminating(void)
{
    DEBUG_LOG_DEBUG("appExitSubStateTerminating");
}

/*! \brief Exit APP_STATE_IN_CASE parent state. */
static void appExitParentStateInCase(void)
{
    DEBUG_LOG_DEBUG("appExitParentStateInCase");

    /* run rules for being taken out of the case */
    appSmRulesResetEvent(RULE_EVENT_IN_CASE);
    appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
}

/*! \brief Exit APP_STATE_OUT_OF_CASE parent state. */
static void appExitParentStateOutOfCase(void)
{
    DEBUG_LOG_DEBUG("appExitParentStateOutOfCase");
}

/*! \brief Exit APP_STATE_IN_EAR parent state. */
static void appExitParentStateInEar(void)
{
    DEBUG_LOG_DEBUG("appExitParentStateInEar");

    /* Run rules for out of ear event */
    appSmRulesResetEvent(RULE_EVENT_IN_EAR);
    appSmRulesSetEvent(RULE_EVENT_OUT_EAR);
}

/*! \brief Enter APP_STATE_IN_CASE parent state. */
static void appEnterParentStateInCase(void)
{
    DEBUG_LOG_DEBUG("appEnterParentStateInCase");

    /* Run rules for in case event */
    appSmRulesResetEvent(RULE_EVENT_OUT_CASE);
    appSmRulesSetEvent(RULE_EVENT_IN_CASE);

    /* Clear knowledge of peer's state. No longer useful */
    appSmPeerStatusClearWasPairingEnteringCase();
}

/*! \brief Enter APP_STATE_OUT_OF_CASE parent state. */
static void appEnterParentStateOutOfCase(void)
{
    DEBUG_LOG_DEBUG("appEnterParentStateOutOfCase");
}

/*! \brief Enter APP_STATE_IN_EAR parent state. */
static void appEnterParentStateInEar(void)
{
    DEBUG_LOG_DEBUG("appEnterParentStateInEar");

    /* Run rules for in ear event */
    appSmRulesResetEvent(RULE_EVENT_OUT_EAR);
    appSmRulesSetEvent(RULE_EVENT_IN_EAR);
}

/*****************************************************************************
 * End of SM state transition handler functions
 *****************************************************************************/

/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
void appSmSetState(appState new_state)
{
    appState previous_state = appSmGetState();

    DEBUG_LOG_STATE("appSmSetState enum:appState:%d->enum:appState:%d", previous_state, new_state);

    /* Handle state exit functions */
    switch (previous_state)
    {
        case APP_STATE_NULL:
            /* This can occur when DFU is entered during INIT. */
            break;

        case APP_STATE_INITIALISING:
            break;

        case APP_STATE_DFU_CHECK:
            break;

        case APP_STATE_FACTORY_RESET:
            appExitFactoryReset();
            break;

        case APP_STATE_DEVICE_TEST_MODE:
            appExitDeviceTestMode();
            break;

        case APP_STATE_STARTUP:
            break;

        case APP_STATE_IN_CASE_IDLE:
            appExitInCaseIdle();
            break;

#ifdef INCLUDE_DFU
        case APP_STATE_IN_CASE_DFU:
            appExitInCaseDfu();
            break;
#endif

        case APP_STATE_OUT_OF_CASE_IDLE:
            appExitOutOfCaseIdle();
            break;

        case APP_STATE_OUT_OF_CASE_BUSY:
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC:
        case APP_STATE_IN_CASE_SOPORIFIC:
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
        case APP_STATE_IN_CASE_SOPORIFIC_TERMINATING:
            break;

        case APP_STATE_TERMINATING:
            break;

        case APP_STATE_IN_CASE_TERMINATING:
            break;

        case APP_STATE_OUT_OF_CASE_TERMINATING:
            break;

        case APP_STATE_IN_EAR_TERMINATING:
            break;

        case APP_STATE_IN_EAR_IDLE:
            break;

        case APP_STATE_IN_EAR_BUSY:
            break;

        case APP_STATE_IN_CASE_SOPORIFIC_PAIRING:
        case APP_STATE_IN_CASE_IDLE_PAIRING:
        case APP_STATE_OUT_OF_CASE_SOPORIFIC_PAIRING:
        case APP_STATE_OUT_OF_CASE_IDLE_PAIRING:
        case APP_STATE_OUT_OF_CASE_BUSY_PAIRING:
        case APP_STATE_IN_EAR_IDLE_PAIRING:
        case APP_STATE_IN_EAR_BUSY_PAIRING:
            if (!appSmSubStateIsPairing(new_state))
            {
                appExitHandsetPairing(new_state);
            }
            break;

        default:
            DEBUG_LOG_ERROR("appSmSetState attempted to exit unsupported state enum:appState:%d", previous_state);
            Panic();
            break;
    }

    /* if exiting a sleepy state */
    if (appSmStateIsSleepy(previous_state) && !appSmStateIsSleepy(new_state))
        appPowerClientProhibitSleep(SmGetTask());

    /* if exiting a terminating substate */
    if (appSmSubStateIsTerminating(previous_state) && !appSmSubStateIsTerminating(new_state))
        appExitSubStateTerminating();

    /* if exiting the APP_STATE_IN_CASE parent state */
    if (appSmStateInCase(previous_state) && !appSmStateInCase(new_state))
        appExitParentStateInCase();

    /* if exiting the APP_STATE_OUT_OF_CASE parent state */
    if (appSmStateOutOfCase(previous_state) && !appSmStateOutOfCase(new_state))
        appExitParentStateOutOfCase();

    /* if exiting the APP_STATE_IN_EAR parent state */
    if (appSmStateInEar(previous_state) && !appSmStateInEar(new_state))
        appExitParentStateInEar();

    /* Set new state */
    SmGetTaskData()->state = new_state;

    /* if entering the APP_STATE_IN_CASE parent state */
    if (!appSmStateInCase(previous_state) && appSmStateInCase(new_state))
        appEnterParentStateInCase();

    /* if entering the APP_STATE_OUT_OF_CASE parent state */
    if (!appSmStateOutOfCase(previous_state) && appSmStateOutOfCase(new_state))
        appEnterParentStateOutOfCase();

    /* if entering the APP_STATE_IN_EAR parent state */
    if (!appSmStateInEar(previous_state) && appSmStateInEar(new_state))
        appEnterParentStateInEar();

    /* if entering a terminating substate */
    if (!appSmSubStateIsTerminating(previous_state) && appSmSubStateIsTerminating(new_state))
        appEnterSubStateTerminating();

    /* if entering a sleepy state */
    if (!appSmStateIsSleepy(previous_state) && appSmStateIsSleepy(new_state))
        appPowerClientAllowSleep(SmGetTask());

    /* Handle state entry functions */
    switch (new_state)
    {
        case APP_STATE_INITIALISING:
            break;

        case APP_STATE_DFU_CHECK:
            appEnterDfuCheck();
            break;

        case APP_STATE_FACTORY_RESET:
            appEnterFactoryReset();
            break;

        case APP_STATE_DEVICE_TEST_MODE:
            appEnterDeviceTestMode();
            break;

        case APP_STATE_STARTUP:
            appEnterStartup();
            break;

        case APP_STATE_IN_CASE_IDLE:
            appEnterInCaseIdle();
            break;

#ifdef INCLUDE_DFU
        case APP_STATE_IN_CASE_DFU:
            appEnterInCaseDfu();
            break;
#endif

        case APP_STATE_OUT_OF_CASE_IDLE:
            appEnterOutOfCaseIdle();
            break;

        case APP_STATE_OUT_OF_CASE_BUSY:
            break;

        case APP_STATE_IN_CASE_SOPORIFIC:
            break;
    
        case APP_STATE_IN_CASE_SOPORIFIC_TERMINATING:
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC:
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
            break;

        case APP_STATE_TERMINATING:
            break;

        case APP_STATE_IN_CASE_TERMINATING:
            break;

        case APP_STATE_OUT_OF_CASE_TERMINATING:
            break;

        case APP_STATE_IN_EAR_TERMINATING:
            break;

        case APP_STATE_IN_EAR_IDLE:
            break;

        case APP_STATE_IN_EAR_BUSY:
            break;

        case APP_STATE_IN_CASE_SOPORIFIC_PAIRING:
        case APP_STATE_IN_CASE_IDLE_PAIRING:
        case APP_STATE_OUT_OF_CASE_SOPORIFIC_PAIRING:
        case APP_STATE_OUT_OF_CASE_IDLE_PAIRING:
        case APP_STATE_OUT_OF_CASE_BUSY_PAIRING:
        case APP_STATE_IN_EAR_IDLE_PAIRING:
        case APP_STATE_IN_EAR_BUSY_PAIRING:
            if (!appSmSubStateIsPairing(previous_state))
            {
                appEnterHandsetPairing();
            }
            break;

        default:
            DEBUG_LOG_ERROR("appSmSetState attempted to enter unsupported state enum:appState:%d", new_state);
            Panic();
            break;
    }

    Ui_InformContextChange(ui_provider_app_sm, earbudSm_GetCurrentApplicationContext());
}

/*! \brief Modify the substate of the current parent appState and return the new state. */
static appState appSetSubState(appSubState substate)
{
    appState state = appSmGetState();
    state &= ~APP_SUBSTATE_MASK;
    state |= substate;
    return state;
}

static appState appAddSubState(appSubState substate)
{
    appState state = appSmGetState();
    state |= substate;
    return state;
}

static appSubState appGetSubState(void)
{
    return appSmGetState() & APP_SUBSTATE_MASK;
}

/*! \brief Update the disconnecting links lock status */
static void appSmUpdateDisconnectingLinks(void)
{
    if (appSmDisconnectLockHandsetIsDisconnecting())
    {
        DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks disconnecting handset");
        if (!HandsetService_IsAnyDeviceConnected())
        {
            DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks handset disconnected");
            appSmDisconnectLockClearLinks(SM_DISCONNECT_HANDSET);
        }
    }
}

/*! \brief Handle notification of (dis)connections. */
static void appSmHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T* ind)
{
    DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd connected:%d", ind->connected);

    if (ind->connected)
    {
        if (!ind->ble)
        {
            /* A BREDR connection won't necessarily evaluate rules. And if we
               evaluate them now, the conditions needed for upgrade control will
               not have changed.  Send a message instead. */
            MessageSend(SmGetTask(), SM_INTERNAL_BREDR_CONNECTED, NULL);
        }
    }
}

/*! \brief Prepare to enable ANC when the user first wears the earbud, if necessary. */
static void appSmPrepareInitialANC(void)
{
#ifdef DEFAULT_ANC_ON
    if (appSmIsInEar())
    {
        Ui_InjectUiInput(ui_input_anc_on);
    } else
    {
        appSmSetANCStateWhenOutOfEar(TRUE);
    }
#endif
}

/*! \brief Handle notification of handset connection. */
static void appSmHandleHandsetServiceConnectedInd(HANDSET_SERVICE_CONNECTED_IND_T *ind)
{
    DEBUG_LOG("appSmHandleHandsetServiceConnectedInd");
    UNUSED(ind);

    appSmPrepareInitialANC();
    appSmRulesSetEvent(RULE_EVENT_HANDSET_CONNECTED);
}

/*! \brief Handle notification of handset disconnection. */
static void appSmHandleHandsetServiceDisconnectedInd(HANDSET_SERVICE_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG("appSmHandleHandsetServiceDisconnectedInd status %u", ind->status);

    appSmUpdateDisconnectingLinks();

    if (ind->status == handset_service_status_link_loss)
    {
        appSmRulesSetEvent(RULE_EVENT_HANDSET_LINK_LOSS);
    }
    else
    {
        appSmRulesSetEvent(RULE_EVENT_HANDSET_DISCONNECTED);
    }
}

static void appSmHandleHandsetServiceMpConnectStopCfm(const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T *cfm)
{
    DEBUG_LOG("appSmHandleHandsetServiceMpConnectStopCfm status %u", cfm->status);
    PeerFindRole_PrepareResponse(SmGetTask());
}

/*! \brief Handle completion of application module initialisation. */
static void appSmHandleInitConfirm(void)
{
    phyState curr_phy_state;
    DEBUG_LOG_INFO("appSmHandleInitConfirm: Initialisation is complete");

    switch (appSmGetState())
    {
        case APP_STATE_INITIALISING:
            appSmSetState(APP_STATE_DFU_CHECK);
            curr_phy_state = appPhyStateGetCurrentState();
            if (BtDevice_IsPairedWithPeer() && (curr_phy_state != PHY_STATE_IN_CASE))
            {
                /* Device is peer paired and not in case. Peer find role will start soon.
                   Calling appPowerOn() here will result in playing the power on prompt
                   immmediately which will slow down the earbud boot up time. Hence we  
                   ensure appPowerOn() is sent only after peer find role starts. */
                PeerFindRole_RegisterStartClient(&SmGetTaskData()->task);
                appSmPfrStartLockSet(TRUE);
                MessageSendConditionally(SmGetTask(), SM_INTERNAL_APP_POWER_ON, NULL,
                                         &appSmPfrStartLockGet());
            }
            else
            {
                appPowerOn();
            }
            break;

        case APP_STATE_IN_CASE_DFU:
            appPowerOn();
            break;

        case APP_STATE_TERMINATING:
            break;

        default:
            Panic();
    }
}

/*! \brief Handle completion of handset pairing. */
static void appSmHandlePairHandsetConfirm(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePairHandsetConfirm");

    if (appSmIsPairing())
    {
        appSmSetCoreStateDiscardPairing();
    }

    switch (appSmGetState())
    {
        case APP_STATE_FACTORY_RESET:
            /* Nothing to do, even if pairing with handset succeeded, the final
            act of factory reset is to delete handset pairing */
            break;

        default:
            /* Ignore, paired with handset with known address as requested by peer */
            break;
    }
}

/*! \brief Handle state machine transitions when earbud removed from the ear,
           or transitioning motionless out of the ear to just out of the ear. */
static void appSmHandlePhyStateOutOfEarEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateOutOfEarEvent state=0x%x", appSmGetState());

    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }
}

/*! \brief Handle state machine transitions when earbud motionless out of the ear. */
static void appSmHandlePhyStateOutOfEarAtRestEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateOutOfEarAtRestEvent state=0x%x", appSmGetState());

    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }
}

/*! \brief Handle state machine transitions when earbud is put in the ear. */
static void appSmHandlePhyStateInEarEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateInEarEvent state=0x%x", appSmGetState());

    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }
}

/*! \brief Handle state machine transitions when earbud is put in the case. */
static void appSmHandlePhyStateInCaseEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateInCaseEvent state=0x%x", appSmGetState());

    /*! \todo Need to add other non-core states to this conditional from which we'll
     * permit a transition back to a core state, such as... sleeping? */
    if (appSmIsCoreState() || appSmIsPairing())
    {
        appSmSetCoreStateDiscardPairing();
    }

#ifdef INCLUDE_DFU
    /* check and inform the DFU domain if the earbuds are not-in-use state(used for silent commit) */
    appSmCheckAndNotifyDeviceNotInUse();
#endif /* INCLUDE_DFU */

    /* Inject an event to the rules engine to make sure DFU is enabled */
    appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
}

/*! \brief Project specific power supply control for entering case. */
static void appSmEnablePowerInCase(void)
{
}

/*! \brief Project specific power supply control for leaving case. */
static void appSmEnablePowerOutOfCase(void)
{
}

/*! \brief Handle changes in physical state of the earbud. */
static void appSmHandlePhyStateChangedInd(smTaskData* sm, PHY_STATE_CHANGED_IND_T *ind)
{
bool hold_tp_in_reset = FALSE;
    UNUSED(sm);

    DEBUG_LOG_DEBUG("appSmHandlePhyStateChangedInd new phy state %d", ind->new_state);

    // If we were in the case and the new state is out of case, enable sensors
    if (appSmIsInCase() && ind->new_state != PHY_STATE_IN_CASE)
    {
        sm->touchpad_deliberately_disabled = FALSE;
        /* Application may need to re-enable power supplies when leaving the case */
        appSmEnablePowerOutOfCase();
        /* Enable proximity when out case */
        appProximityEnableSensor(SmGetTask(), TRUE);
    }
    
    // If we were out of the case and we are now in the case, turn off the power supplies
    if (appSmIsOutOfCase() && ind->new_state == PHY_STATE_IN_CASE)
    {
        sm->touchpad_deliberately_disabled = FALSE;
        /* Disable proximity when in case */
        appProximityEnableSensor(SmGetTask(), FALSE);
        /* Application may need to save power when entering the case */
        appSmEnablePowerInCase();
        /* Hold the touchpad in reset when in the case. */
        hold_tp_in_reset = TRUE;
    }
    
    /* We always want to reset the touchpad so the user does not get confused with long 
     * button presses during state transitions. */
    if (!sm->touchpad_deliberately_disabled) {
        TouchSensor_Reset(hold_tp_in_reset);
    }
    
    sm->phy_state = ind->new_state;

    switch (ind->new_state)
    {
        case PHY_STATE_IN_CASE:
            appSmHandlePhyStateInCaseEvent();
            return;
        case PHY_STATE_OUT_OF_EAR:
            appSmHandlePhyStateOutOfEarEvent();
            return;
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appSmHandlePhyStateOutOfEarAtRestEvent();
            return;
        case PHY_STATE_IN_EAR:
            appSmHandlePhyStateInEarEvent();
            return;
        default:
            break;
    }
}

/*! \brief Take action following power's indication of imminent sleep.
    SM only permits sleep when soporific. */
static void appSmHandlePowerSleepPrepareInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerSleepPrepareInd");

    PanicFalse(APP_STATE_OUT_OF_CASE_SOPORIFIC == appSmGetState()
        || APP_STATE_IN_CASE_SOPORIFIC == appSmGetState());

#ifdef ENABLE_SKIP_PFR
    /* Store the preserved role onto the PS before allowing sleep */
    PeerFindRole_PersistPreservedRole();
#endif

    if (APP_STATE_IN_CASE_SOPORIFIC == appSmGetState())
    {
        appSmSetState(APP_STATE_IN_CASE_SOPORIFIC_TERMINATING);
    }
    else
    {
        appSmSetState(APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING);
    }
}

/*! \brief Handle sleep cancellation. */
static void appSmHandlePowerSleepCancelledInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerSleepCancelledInd");
    /* Need to restart the SM to ensure topology is re-started */
    appSmSetState(APP_STATE_STARTUP);
}

/*! \brief Take action following power's indication of imminent shutdown.
    Can be received in any state. */
static void appSmHandlePowerShutdownPrepareInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerShutdownPrepareInd");

#ifdef ENABLE_SKIP_PFR
    /* Store the preserved role onto the PS before allowing shutdown */
    PeerFindRole_PersistPreservedRole();
#endif

    appSmSetState(appSetSubState(APP_SUBSTATE_TERMINATING));
}

/*! \brief Handle shutdown cancelled. */
static void appSmHandlePowerShutdownCancelledInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerShutdownCancelledInd");
    /* Shutdown can be entered from any state (including non core states), so startup again,
       to determine the correct state to enter. */
    appSmSetState(APP_STATE_STARTUP);
}

static void appSmHandleConnRulesHandsetPair(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesHandsetPair");

    switch (appSmGetState())
    {
        case APP_STATE_OUT_OF_CASE_IDLE:
        case APP_STATE_IN_EAR_IDLE:
        case APP_STATE_IN_CASE_IDLE:
            if (!appSmIsPairing())
            {
                DEBUG_LOG_DEBUG("appSmHandleConnRulesHandsetPair, rule said pair with handset");
                appSmClearUserPairing();
                appSmSetState(appAddSubState(APP_SUBSTATE_HANDSET_PAIRING));
            }
            break;
        default:
            break;
    }
}

static void appSmHandleConnRulesHandsetConnect(const CONN_RULES_HANDSET_CONNECT_T *cr_hc)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesHandsetConnect");
    if (cr_hc->reason == RULE_CONNECT_REASON_LINK_LOSS)
    {
        HandsetService_ReconnectLinkLossRequest(SmGetTask());
    }
    else
    {
        HandsetService_ReconnectRequest(SmGetTask());
    }
    appSmRulesSetRuleComplete(CONN_RULES_HANDSET_CONNECT);
}

static void appSmHandleConnRulesAllHandsetsDisconnect(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAllHandsetsDisconnect");
    appSmDisconnectHandsets();
    appSmRulesSetRuleComplete(CONN_RULES_ALL_HANDSETS_DISCONNECT);
}

static void appSmHandleConnRulesLruHandsetDisconnect(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLruHandsetDisconnect");
    appSmDisconnectLruHandset();
    appSmRulesSetRuleComplete(CONN_RULES_LRU_HANDSET_DISCONNECT);
}

static void appSmHandleConnRulesEnterDfu(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesEnterDfu");

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_IDLE:
        case APP_STATE_IN_CASE_DFU:
#ifdef INCLUDE_DFU
            appSmEnterDfuMode();
#endif
            break;

        default:
            break;
    }
}

#ifdef INCLUDE_DFU
/*! \brief Indicate to the peer (Secondary) that DFU has aborted owing to
           DFU data transfer not initiated with the prescribed timeout period.
*/
static void EarbudSm_HandleDfuStartTimeoutNotifySecondary(void)
{
    DEBUG_LOG_DEBUG("EarbudSm_HandleDfuEndedNotifySecondary");

    earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_ind_dfu_start_timeout_t));
}
#endif

static void appSmHandleConnRulesForwardLinkKeys(void)
{
    bdaddr peer_addr;

    DEBUG_LOG_DEBUG("appSmHandleConnRulesForwardLinkKeys");

    /* Can only send this if we have a peer earbud */
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* Attempt to send handset link keys to peer device */
        KeySync_Sync();
    }

    /* In all cases mark rule as done */
    appSmRulesSetRuleComplete(CONN_RULES_PEER_SEND_LINK_KEYS);
}

#ifdef INCLUDE_FAST_PAIR
static void appSmHandleConnRulesForwardAccountKeys(void)
{
    bdaddr peer_addr;
    DEBUG_LOG("appSmHandleConnRulesForwardAccountKeys");

    /* Can only send this if we have a peer earbud */
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* Attempt to send account keys to peer device */
        UserAccountsSync_Sync();
    }

    /* In all cases mark rule as done */
    appSmRulesSetRuleComplete(CONN_RULES_PEER_SEND_FP_ACCOUNT_KEYS);
}
#endif 


/*! \brief Checks if any active media related out of ear timers */
static bool appSmIsOutOfEarMediaTimerRunning(void)
{
    return (MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_MEDIA, NULL) ||
        MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_MEDIA_START, NULL));
}

/*! \brief Start timer to stop Media if earbud stays out of the ear. */
static void appSmHandleConnRulesMediaTimeout(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesMediaTimeout");

    /* only take action if timeout is configured */
    if (appConfigOutOfEarAudioTimeoutSecs() && !appSmIsOutOfEarMediaTimerRunning())
    {
        /* either earbud is out of ear and case */
        if ((appSmIsOutOfCase()) ||
            (StateProxy_IsPeerOutOfCase() && StateProxy_IsPeerOutOfEar()))
        {
            DEBUG_LOG_DEBUG("appSmHandleConnRulesMediaTimeout, out of case/ear, so starting %u second timer", appConfigOutOfEarAudioTimeoutSecs());
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_MEDIA,
                             NULL, D_SEC(appConfigOutOfEarAudioTimeoutSecs()));
        }
    }

    appSmRulesSetRuleComplete(CONN_RULES_MEDIA_PAUSE);
}

/*! \brief Handle decision to start media. */
static void appSmHandleConnRulesMediaPlay(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesMediaPlay media play");

    /* request the handset plays the media player */
    Ui_InjectUiInput(ui_input_play);

    appSmRulesSetRuleComplete(CONN_RULES_MEDIA_PLAY);
}

/*! \brief Start timer to transfer SCO to AG if earbud stays out of the ear. */
static void appSmHandleConnRulesScoTimeout(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTimeout");

    if (!appSmIsInEar() && appConfigOutOfEarScoTimeoutSecs())
    {
        DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTimeout, out of case/ear, so starting %u second timer", appConfigOutOfEarScoTimeoutSecs());
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO,
                         NULL, D_SEC(appConfigOutOfEarScoTimeoutSecs()));
    }

    appSmRulesSetRuleComplete(CONN_RULES_SCO_TIMEOUT);
}

/*! \brief Accept incoming call if the primary/secondary EB enters in ear */
static void appSmHandleConnRulesAcceptincomingCall(void)
{
    DEBUG_LOG("appSmHandleConnRulesAcceptincomingCall");
    Ui_InjectUiInput(ui_input_voice_call_accept);
    appSmRulesSetRuleComplete(CONN_RULES_ACCEPT_INCOMING_CALL);
}

/*! \brief Transfer SCO from AG as earbud is in the ear. */
static void appSmHandleConnRulesScoTransferToEarbud(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTransferToEarbud, in ear transferring audio this %u peer %u", appSmIsInEar(), StateProxy_IsPeerInEar());
    Ui_InjectUiInput(ui_input_voice_transfer_to_headset);
    appSmRulesSetRuleComplete(CONN_RULES_SCO_TRANSFER_TO_EARBUD);
}

static void appSmHandleConnRulesScoTransferToHandset(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTransferToHandset");
    Ui_InjectUiInput(ui_input_voice_transfer_to_ag);
    appSmRulesSetRuleComplete(CONN_RULES_SCO_TRANSFER_TO_HANDSET);
}

/*! \brief Turns LEDs on */
static void appSmHandleConnRulesLedEnable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLedEnable");

    LedManager_Enable(TRUE);
    appSmRulesSetRuleComplete(CONN_RULES_LED_ENABLE);
}

/*! \brief Turns LEDs off */
static void appSmHandleConnRulesLedDisable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLedDisable");

    LedManager_Enable(FALSE);
    appSmRulesSetRuleComplete(CONN_RULES_LED_DISABLE);
}

#ifdef INCLUDE_SENSOR_PROFILE
/*! \brief Disable spatial audio if put in case. */
static void appSmHandleConnRulesSpatialAudioDisable(void)
{
    DEBUG_LOG_DEBUG("%s", __func__);

    SpatialData_Enable(spatial_data_disabled, 0, 0);
    appSmRulesSetRuleComplete(CONN_RULES_SPATIAL_AUDIO_DISABLE);
}
#endif /* INCLUDE_SENSOR_PROFILE */

/*! \brief Pause Media as an earbud is out of the ear for too long. */
static void appSmHandleInternalTimeoutOutOfEarMedia(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutOutOfEarMedia out of ear pausing audio");

    /* Double check we're still out of case when the timeout occurs */
    if (earbudSm_OutOfCaseAndEar())
    {
        /* request the handset pauses the media player */
        Ui_InjectUiInput(ui_input_pause_all);

        /* start the timer to autorestart the audio if earbud is placed
         * back in the ear. */
        if (appConfigInEarAudioStartTimeoutSecs())
        {
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_MEDIA_START,
                             NULL, D_SEC(appConfigInEarAudioStartTimeoutSecs()));
        }
    }
}

/*! \brief Nothing to do. */
static void appSmHandleInternalTimeoutInEarMediaStart(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutInEarMediaStart");

    /* no action needed, we're just using the presence/absence of an active
     * timer when going into InEar to determine if we need to restart audio */

#ifdef INCLUDE_LE_AUDIO_BROADCAST
    /* For LE Broadcast: while paused we maintained PA sync in readiness for
       restarting the broadcast audio. But if we are not going to restart audio
       then broadcast manager should un-sync from all sources. */
    if (LeBroadcastManager_IsPaused())
    {
        LeBroadcastManager_Unsync();
    }
#endif
}

/*! \brief Transfer SCO to AG as earbud is out of the ear for too long. */
static void appSmHandleInternalTimeoutOutOfEarSco(void)
{
    /* Evaluate the rule again which decides if sco audio needs to be transferred to
       the handset or not. This rule will check here again if the secondary earbud is
       there in ear now and it is streaming sco audio or not. If secondary earbud has
       come back into the ear and streaming sco audio then sco audio will not be
       transferred back to handset. */
    if (PrimaryRuleTransferScoToHandsetIfOutOfEar() == rule_action_run)
    {
        DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutOutOfEarSco"
                        " out of ear transferring audio");
        /* Transfer SCO to handset */
        Ui_InjectUiInput(ui_input_voice_transfer_to_ag);
    }
}

static void appSmHandleConnRulesPeerScoControl(CONN_RULES_PEER_SCO_CONTROL_T* crpsc)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesPeerScoControl peer sco %u", crpsc->enable);
    if (crpsc->enable)
    {
        MirrorProfile_EnableMirrorEsco();
    }
    else
    {
        MirrorProfile_DisableMirrorEsco();
    }
    appSmRulesSetRuleComplete(CONN_RULES_PEER_SCO_CONTROL);
}

static void appSmHandleConnRulesAudioUiIndControl(CONN_RULES_AUDIO_UI_IND_CONTROL_T * cr_ui_ind_ctrl)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAudioUiIndControl playback_enabled=%u", cr_ui_ind_ctrl->enable_playback);

    UiTones_SetTonePlaybackEnabled(cr_ui_ind_ctrl->enable_playback);
    UiPrompts_SetPromptPlaybackEnabled(cr_ui_ind_ctrl->enable_playback);

    appSmRulesSetRuleComplete(CONN_RULES_AUDIO_UI_IND_CONTROL);
}

/*! \brief Handle confirmation of SM marhsalled msg TX, free the message memory. */
static void appSm_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    PanicNull((void*)cfm);
    switch(cfm->type)
    {
        case MARSHAL_TYPE(earbud_sm_msg_gaming_mode_t):
            GamingMode_HandlePeerSigTxCfm(cfm);
        break;
        default:
        break;
    }
}

/*! \brief Handle message from peer notifying of deleting handset. */
static void appSmHandlePeerDeleteHandsetWhenFull(earbud_sm_req_delete_handset_if_full_t *msg)
{
    DEBUG_LOG("appSmHandlePeerDeleteHandsetWhenFull");

    if(msg)
    {
        DEBUG_LOG("appSmHandlePeerDeleteHandsetWhenFull address %04x,%02x,%06lx",
                    msg->bd_addr.lap,
                    msg->bd_addr.uap,
                    msg->bd_addr.nap);

        if (!appDeviceDelete(&msg->bd_addr))
        {
            DEBUG_LOG_WARN("appSmHandlePeerDeleteHandsetWhenFull WARNING device is still connected");
        }
    }
}

/*! \brief Handle SM->SM marshalling messages.

    Currently the only handling is to make the Primary/Secondary role switch
    decision.
*/
static void appSm_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    DEBUG_LOG_DEBUG("appSm_HandleMarshalledMsgChannelRxInd enum:earbud_sm_marshal_types_t:%d received from peer", ind->type);

    switch(ind->type)
    {
#ifdef INCLUDE_DFU
        case MARSHAL_TYPE(earbud_sm_req_dfu_active_when_in_case_t):
            appSmEnterDfuModeInCase(TRUE, FALSE);
            break;

        case MARSHAL_TYPE(earbud_sm_req_dfu_started_t):
            /* Set the various Upgrade specfic variable states appropriately */
            UpgradeSetInProgressId(((earbud_sm_req_dfu_started_t*)ind->msg)->id_in_progress);
            DfuPeer_SetRole(FALSE);
            /* For DFU, the main roles are dynamically selected through all
             * DFU phases (including abrupt reset and post reboot DFU commit phase).
             * A role switch can occur during these scenarios/phases and hence
             * mark the DFU specific links for retention, even for secondary.
             */
            appEnableKeepTopologyAliveForDfu();
#ifdef INCLUDE_DFU_PEER
            /* Prohibit Sleep for Peer DFU */
            DEBUG_LOG_VERBOSE("appSm_HandleMarshalledMsgChannelRxInd Prohibit earbud to sleep");
            Dfu_ProhibitSleep();
#endif
            break;

        case MARSHAL_TYPE(earbud_sm_ind_dfu_start_timeout_t):
            /* Peer (Primary) has indicated that DFU has aborted owing to DFU
             * data transfer not initiated with the prescribed timeout period.
             * Hence exit DFU so that the earbud states are synchronized.
             */
            appSmHandleDfuEnded(TRUE);
            break;
#endif

        case MARSHAL_TYPE(earbud_sm_req_stereo_audio_mix_t):
            appKymeraSetStereoLeftRightMix(TRUE);
#if defined (INCLUDE_LE_AUDIO_UNICAST) && defined (ENABLE_LE_AUDIO_FT_UPDATE)
            LeUnicastManager_EnableSourceMix(TRUE);
#endif
            break;

        case MARSHAL_TYPE(earbud_sm_req_mono_audio_mix_t):
            appKymeraSetStereoLeftRightMix(FALSE);
#if defined (INCLUDE_LE_AUDIO_UNICAST) && defined (ENABLE_LE_AUDIO_FT_UPDATE)
            LeUnicastManager_EnableSourceMix(FALSE);
#endif
            break;

        case MARSHAL_TYPE(earbud_sm_req_delete_handsets_t):
            appSmDeleteHandsets();
            break;

        case MARSHAL_TYPE(earbud_sm_req_delete_handset_if_full_t):
            appSmHandlePeerDeleteHandsetWhenFull((earbud_sm_req_delete_handset_if_full_t*)ind->msg);
            break;

        case MARSHAL_TYPE(earbud_sm_msg_gaming_mode_t):
            GamingMode_HandlePeerMessage((earbud_sm_msg_gaming_mode_t*)ind->msg);
            break;

        case MARSHAL_TYPE(earbud_sm_ind_gone_in_case_while_pairing_t):
            appSmPeerStatusSetWasPairingEnteringCase();
            break;

        case MARSHAL_TYPE(earbud_sm_req_factory_reset_t):
            appSmFactoryReset();
            break;

        case MARSHAL_TYPE(earbud_sm_req_disable_touchpad_t):
            appSmDisableTouchpad();
            break;

        default:
            break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

/*! \brief Use pairing activity to generate rule event to check for link keys that may need forwarding. */
static void appSm_HandlePairingActivity(const PAIRING_ACTIVITY_T* pha)
{
    if (pha->status == pairingActivityCompleteVersionChanged)
    {
        appSmRulesSetEvent(RULE_EVENT_PEER_UPDATE_LINKKEYS);
    }

    appSmPeerStatusClearWasPairingEnteringCase();
}

/*! \brief Idle timeout */
static void appSmHandleInternalTimeoutIdle(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutIdle");
    PanicFalse(APP_STATE_OUT_OF_CASE_IDLE == appSmGetState()
        || APP_STATE_IN_CASE_IDLE == appSmGetState());

    if (APP_STATE_IN_CASE_IDLE == appSmGetState())
    {
        appSmSetState(APP_STATE_IN_CASE_SOPORIFIC);
    }
    else
    {
        appSmSetState(APP_STATE_OUT_OF_CASE_SOPORIFIC);
    }
}

static void appSmHandlePeerSigDisconnected(void)
{
    if (!TwsTopology_IsRolePrimary()) 
    {
        /* Look for any handsets in the reconnecting context, if there are any, set them back to the link
           loss, so we don't continue reconnection through both earbuds going in case. Otherwise set connected
           handsets as disconnected*/
        device_t* devices = NULL;
        unsigned num_devices = 0;
        unsigned index;
        deviceType type = DEVICE_TYPE_HANDSET;
        DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);
        for(index = 0; index < num_devices; index++)
        {
            handset_bredr_context_t context = DeviceProperties_GetHandsetBredrContext(devices[index]);
            if (context > handset_bredr_context_link_loss_reconnecting)
            {
                context = handset_bredr_context_disconnected;
            }
            else if (context == handset_bredr_context_link_loss_reconnecting)
            {
                context = handset_bredr_context_link_loss;
            }
            DeviceProperties_SetHandsetBredrContext(devices[index], context);
        }
        free(devices);
    }
}

static void appSmHandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    if (ind->status == peerSigStatusConnected)
    {
        appSmRulesResetEvent(RULE_EVENT_PEER_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_PEER_CONNECTED);
    }
    else
    {
        appSmRulesResetEvent(RULE_EVENT_PEER_CONNECTED);
        appSmRulesSetEvent(RULE_EVENT_PEER_DISCONNECTED);
        appSmHandlePeerSigDisconnected();
    }
}

static void appSmHandleBtDeviceSelfCreated(const BT_DEVICE_SELF_CREATED_IND_T *ind)
{
    device_t device = PanicNull(ind->device);
#ifdef INCLUDE_ACCESSORY
    uint32 supported_profiles = (DEVICE_PROFILE_AVRCP | DEVICE_PROFILE_A2DP | DEVICE_PROFILE_HFP | DEVICE_PROFILE_ACCESSORY);
#else
    uint32 supported_profiles = (DEVICE_PROFILE_AVRCP | DEVICE_PROFILE_A2DP | DEVICE_PROFILE_HFP);
#endif
    /* Setting profiles supported by SELF. */
    BtDevice_AddSupportedProfilesToDevice(device, supported_profiles);
    DeviceDbSerialiser_Serialise();
}

static void appSmHandleAvStreamingActiveInd(MessageId id)
{
    DEBUG_LOG_DEBUG("appSmHandleAvStreamingActiveInd state=0x%x MESSAGE:0x%x", appSmGetState(), id);

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }
#ifdef INCLUDE_DFU
#ifdef INCLUDE_LE_AUDIO_UNICAST
    /* Don't change peer link policy to sniff mode upon unicast media connected message */
    if (id != LE_AUDIO_UNICAST_MEDIA_CONNECTED)
#endif
    {
        DfuPeer_SetLinkPolicy(lp_sniff);
    }
#endif

    appSmRulesSetEvent(RULE_EVENT_A2DP_ACTIVE);
}

static void appSmHandleAvStreamingInactiveInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleAvStreamingInactiveInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }
#ifdef INCLUDE_DFU
    DfuPeer_SetLinkPolicy(lp_active);
#endif

    appSmRulesSetEvent(RULE_EVENT_A2DP_INACTIVE);
}

static void appSmHandleHfpScoConnectedInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpScoConnectedInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }

    appSmRulesSetEvent(RULE_EVENT_SCO_ACTIVE);

#ifdef INCLUDE_DFU
    UpgradeSetScoActive(TRUE);
    DEBUG_LOG("UpgradeSetScoActive state = TRUE");
    DfuPeer_SetLinkPolicy(lp_sniff);
#endif

}

static void appSmHandleHfpScoDisconnectedInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpScoDisconnectedInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
    {
        appSmSetCoreStatePreservePairing();
    }

#ifdef INCLUDE_DFU
    UpgradeSetScoActive(FALSE);
    DEBUG_LOG("UpgradeSetScoActive state = FALSE");
    DfuPeer_SetLinkPolicy(lp_active);
#endif
}

/*! \brief Notify peer of deleting handset */
static void appSmHandleNotifyPeerDeleteHandset(bdaddr bd_addr)
{

    earbud_sm_req_delete_handset_if_full_t* msg = PanicUnlessMalloc(sizeof(earbud_sm_req_delete_handset_if_full_t));

    DEBUG_LOG_DEBUG("appSmHandleNotifyPeerDeleteHandset address %04x,%02x,%06lx", bd_addr.lap, bd_addr.uap, bd_addr.nap);

    msg->bd_addr = bd_addr;

    appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                     PEER_SIG_MSG_CHANNEL_APPLICATION,
                                     msg,
                                     MARSHAL_TYPE(earbud_sm_req_delete_handset_if_full_t));

}

#ifndef USE_SYNERGY
bool appSmHandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    bool handled = FALSE;
    UNUSED(already_handled);

    switch(id)
    {
        case CL_SM_AUTH_DEVICE_DELETED_IND:
            {
                CL_SM_AUTH_DEVICE_DELETED_IND_T *ind = (CL_SM_AUTH_DEVICE_DELETED_IND_T *)message;

                if (!KeySync_IsPdlUpdateInProgress(&ind->taddr.addr))
		        {
                    appSmHandleNotifyPeerDeleteHandset(ind->taddr.addr);
                }
				
                handled = TRUE;
            }
            break;

        default:
            break;
    }

    return handled;
}
#endif
static void appSmHandleInternalPairHandset(void)
{
    if (appSmIsCoreState() && appSmIsPrimary() && !appSmIsPairing())
    {
        appSmSetUserPairing();
        appSmSetState(appAddSubState(APP_SUBSTATE_HANDSET_PAIRING));
    }
    else
        DEBUG_LOG_DEBUG("appSmHandleInternalPairHandset can only pair once role has been decided and is in IDLE state");
}

/*! \brief Delete pairing for all handsets.
    \note There must be no connections to a handset for this to succeed. */
static void appSmHandleInternalDeleteHandsets(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalDeleteHandsets");

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_IDLE:
        case APP_STATE_OUT_OF_CASE_IDLE:
        case APP_STATE_IN_EAR_IDLE:
        {
            MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_BLOCK_RESET_HANDSET_DELETE);
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_BLOCK_RESET_HANDSET_DELETE, NULL, 
                             appConfigHandsetDeleteMinimumTimeoutMs());
            appSmInitiateLinkDisconnection(SM_DISCONNECT_HANDSET, appConfigLinkDisconnectionTimeoutTerminatingMs(),
                                                POST_DISCONNECT_ACTION_DELETE_HANDSET_PAIRING);
            break;
        }
        default:
            DEBUG_LOG_WARN("appSmHandleInternalDeleteHandsets bad state %u",
                                                        appSmGetState());
            break;
    }
}

/*! \brief Handle request to start factory reset. */
static void appSmHandleInternalFactoryReset(void)
{
    if (appSmGetState() >= APP_STATE_STARTUP)
    {
        DEBUG_LOG_DEBUG("appSmHandleInternalFactoryReset");
        appSmSetState(APP_STATE_FACTORY_RESET);
    }
    else
    {
        DEBUG_LOG_WARN("appSmHandleInternalFactoryReset can only factory reset in IDLE state");
    }
}

/*! \brief Handle failure to successfully disconnect links within timeout.
*/
static void appSmHandleInternalLinkDisconnectionTimeout(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalLinkDisconnectionTimeout");

    /* Give up waiting for links to disconnect, clear the lock */
    appSmDisconnectLockClearLinks(SM_DISCONNECT_ALL);
}

#if defined (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS)
/*! Start the DTS service once we learn that GAIA has stopped.
*/
static void appSm_HandleGaiaStopServiceCfm(GAIA_STOP_SERVICE_CFM_T* message)
{
    /* Check that the spp transport stopped */
    if (message->transport_type == gaia_transport_spp)
    {
        DeviceTestService_Start(SmGetTask());
        appSmTestService_SetTestStep(1);
    }
}
#endif /* (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS) */

#ifdef INCLUDE_DFU
/*! Change the state to DFU mode with an optional timeout

    \param timeoutMs Timeout (in ms) after which the DFU state may be exited.
    \param hasDynamicRole Indicates whether dynamic role is supported in the post
                          reboot DFU commit phase.
                          TRUE then supported and hence skip
                          APP_STATE_IN_CASE_DFU but rather enter
                          APP_STATE_STARTUP for dynamic role selection.
                          FALSE then dynamic role selection is unsupported and
                          hence APP_STATE_IN_CASE_DFU is entered.
                          FALSE is also used in DFU phases other than post
                          reboot DFU commit phase.
 */
static void appSmHandleEnterDfuWithTimeout(uint32 timeoutMs, bool hasDynamicRole)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_ENTRY);
    if (timeoutMs)
    {
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_ENTRY, NULL, timeoutMs);
    }

    if (!hasDynamicRole)
    {
        /*
         * APP_STATE_IN_CASE_DFU needs to be entered only at the start of
         * in-case DFU. Thereafter both in case of reboot post abrupt reset
         * as well as in the post reboot DFU commit phase, APP_STATE_IN_CASE_DFU
         * isn't entered rather APP_STATE_STARTUP is entered to establish the
         * main roles dynamically and thereafter resume DFU (as applicable).
         */
        if (APP_STATE_IN_CASE_DFU != appSmGetState())
        {
            DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout. restarted SM_INTERNAL_TIMEOUT_DFU_ENTRY - %dms",timeoutMs);

            appSmSetState(APP_STATE_IN_CASE_DFU);
        }
        else
        {
            DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout restarted SM_INTERNAL_TIMEOUT_DFU_ENTRY. Either Already in IN_CASE_DFU or BDFU in progress - %dms.",
                            timeoutMs);
            /* For serialized DFU, if handset link loss happens during SEB data transfer then DFU timers
             * are restarted which are not cancelled.Cancel timers if upgrade is in progress.
             */
            if(Dfu_IsUpgradeInProgress())
            {
                DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout cancelling DFU timers if upgrade is in progress");
                appSmCancelDfuTimers();
            }
        }
    }
    else
    {
        DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout restarted SM_INTERNAL_TIMEOUT_DFU_ENTRY. hasDynamicRole:%d timeoutMs:%dms.", hasDynamicRole, timeoutMs);
    }
}

static void appSmHandleEnterDfuStartup(bool hasDynamicRole)
{
    appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutToStartAfterRebootMs(), hasDynamicRole);
}

/*
 * NOTE: ONLY State variable reset in appExitInCaseDfu() need to be reset here
         as well for abort/error notification scenarios, even if redundantly
         done except for topology event injection.
 */
static void appSmHandleDfuAborted(void)
{
    appSmCancelDfuTimers();

#ifdef INCLUDE_GAIA
    /* This is insignificant for the Secondary and out-case DFU. Hence skipped.*/
    if (BtDevice_IsMyAddressPrimary() && (appPhyStateGetState() == PHY_STATE_IN_CASE))
    {
        gaiaFrameworkInternal_GaiaDisconnect(0);
    }
#endif
}

static void appSmHandleDfuEnded(bool error)
{
    appSmCancelDfuTimers();
    appSmDFUEnded();

    DEBUG_LOG_DEBUG("appSmHandleDfuEnded, Reset the reboot reason flag.");
    Dfu_SetRebootReason(REBOOT_REASON_NONE);

    /* If DFU is aborted due to SYNC ID mismatch, then retain the profiles
     * for handling the subsequent SYNC request
     */
    if(!UpgradeIsAbortDfuDueToSyncIdMismatch())
    {
        /* No need to remain active either for peer or handset */
        appDisableKeepTopologyAliveForDfu();
    }
    TwsTopology_EnableRoleSwapSupport();

    if (appSmGetState() == APP_STATE_IN_CASE_DFU)
    {
        appSmSetState(APP_STATE_STARTUP);
    }

    /* Regardless of out-of-case or in-case DFU, do the required cleanup */
    if (error)
    {
        appSmHandleDfuAborted();
    }
    /* Inject an event to the rules engine to make sure DFU check is enabled 
     * after current DFU is either completed or aborted
     */
    appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
}

static void appSmHandleUpgradeConnected(void)
{
    /*
     * Set the main role (Primary/Secondary) so that the DFU Protocol PDUs
     * (UPGRADE_HOST_SYNC_REQ, UPGRADE_HOST_START_REQ &
     * UPGRADE_HOST_START_DATA_REQ) exchanged before the actual data transfer
     * can rightly handled based on its main role especially.
     * This is much required especially after a successful handover where the
     * main roles are swapped and upgrade library won't be aware of the
     * true/effective main roles until the DFU role is setup post the initial
     * DFU Protocol PDU exchange and before actual start of DFU data transfer.
     */
    DfuPeer_SetRole(BtDevice_IsMyAddressPrimary());

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_DFU:
            /* If we are in the special DFU case mode then start a fresh time
               for dfu mode as well as a timer to trap for an application that
               opens then closes the upgrade connection. */
            DEBUG_LOG_DEBUG("appSmHandleUpgradeConnected, valid state to enter DFU");
            appSmStartDfuTimer(FALSE);

            DEBUG_LOG_DEBUG("appSmHandleUpgradeConnected Start SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT %dms",
                                appConfigDfuTimeoutCheckForSupportMs());
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT,
                                NULL, appConfigDfuTimeoutCheckForSupportMs());
            break;

        default:
            if (Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET)
            {
                /*
                 * We are here if its the post reboot DFU commit phase and
                 * with dynamic role support.
                 *
                 * If we are in the special DFU case mode then start a fresh
                 * time for dfu mode as well as a timer to trap for an
                 * application that opens then closes the upgrade connection.
                 */
                DEBUG_LOG_DEBUG("appSmHandleUpgradeConnected, valid state to enter DFU");
                appSmStartDfuTimer(TRUE);

                DEBUG_LOG_DEBUG("appSmHandleUpgradeConnected Start SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT %dms",
                                    appConfigDfuTimeoutCheckForSupportMs());
                MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT,
                                    NULL, appConfigDfuTimeoutCheckForSupportMs());
            }
            else
            {
                /* In all other states and modes we don't need to do anything.
                   Start of an actual upgrade will be blocked if needed,
                   see appSmHandleDfuAllow() */
            }
            break;
    }
}


static void appSmHandleUpgradeDisconnected(void)
{
    DEBUG_LOG_DEBUG("appSmHandleUpgradeDisconnected");

    if (MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT))
    {
        DEBUG_LOG_DEBUG("appSmHandleUpgradeDisconnected Cancelled SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT timer");

        if (Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET)
        {
            appSmStartDfuTimer(TRUE);
        }
        else
        {
            appSmStartDfuTimer(FALSE);
        }
    }
}

static void appSmDFUEnded(void)
{
    DEBUG_LOG_DEBUG("appSmDFUEnded");

    /*
     * Before rebooting for the DFU commit phase, the link (including profiles)
     * to Handset aren't disconnected. If Primary tries to establish connection
     * before link-loss owing to link supervision timeout occurs on the Handset
     * there is a likelihood that connection attempt may fail. And later when
     * Handset tries to establish connection, only an ACL may be setup but the
     * profiles connection won't be reatttempted from the Primary (as per the
     * implementation scheme of handset service) for an inbound connection.
     * But profiles can be setup if the Handset exclusively requests to
     * establish these once the ACL is established.
     * In order to establish profiles from the Primary on an established ACL,
     * here a exclusive request is made to setup these profiles managed by
     * handset service.
     */
    if (appPhyStateGetState() != PHY_STATE_IN_CASE && TwsTopology_IsRolePrimary())
    {
        HandsetService_ReconnectRequest(SmGetTask());
    }
}
#endif


static void appSmHandleNoDfu(void)
{

    if (appSmGetState() != APP_STATE_IN_CASE_DFU)
    {
        DEBUG_LOG_DEBUG("appSmHandleNoDfu. Not gone into DFU, so safe to assume we can continue");
#ifdef PRODUCTION_TEST_MODE
        switch(appSmTestService_BootMode())
        {
            case sm_boot_production_test_mode:
                /* disable LE scanning and advertisement to avoid peer pairing*/
                LeScanManager_Disable(SmGetTask());
                LeAdvertisingManager_EnableConnectableAdvertising(SmGetTask(), FALSE);
                appSmSetState(APP_STATE_DEVICE_TEST_MODE);
                break;
            case sm_boot_mute_mode:
                /* request to mute in shipping mode */
                Volume_SetMuteState(TRUE);
                /* This is fall through as intended*/
            case sm_boot_normal_mode:
            default:
                appSmSetState(APP_STATE_STARTUP);
                break;
        }
#else
        DEBUG_LOG_DEBUG("appSmHandleNoDfu. Not gone into DFU, so safe to assume we can continue");

        if (!DeviceTestService_TestMode())
        {
            appSmSetState(APP_STATE_STARTUP);
        }
        else
        {
            appSmSetState(APP_STATE_DEVICE_TEST_MODE);
        }
#endif
    }
    else
    {
        DEBUG_LOG_DEBUG("appSmHandleNoDfu. We are in DFU mode !");
    }
}

/*! \brief Reboot the earbud, no questions asked. */
static void appSmHandleInternalReboot(void)
{
    SystemReboot_Reboot();
}

/*! \brief Reboot the earbud in the request DTS mode. */
static void appSm_HandleEnterDtsMode(uint16 mode)
{
  uint16 dts_mode = mode;

  /* Set DTS PS Key to requested mode */
  PsStore(PS_KEY_DTS_ENABLE, &dts_mode, sizeof(dts_mode));

  /* Reboot device */
  SystemReboot_Reboot();
}

static void appSm_DeleteHandsetsAfterDisconnect(void)
{
    BtDevice_MarkForDeletionAllDevicesOfType(DEVICE_TYPE_HANDSET);

#ifdef INCLUDE_FAST_PAIR
    /* Delete the account keys */
    UserAccounts_DeleteAllAccountKeys();
    /* Synchronization b/w the peers after deletion */
    UserAccountsSync_Sync();
#endif
    if (!MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_BLOCK_RESET_HANDSET_DELETE, NULL))
    {
        /* Minimum delay has elapsed, reboot */
        SystemReboot_Reboot();
    }
}

static void appSmHandleMinimumDeleteTimeout(void)
{
    if (!MessagePendingFirst(SmGetTask(), SM_INTERNAL_LINK_DISCONNECTION_COMPLETE, NULL))
    {
        /* Disconnect has happened and been processed */
        SystemReboot_Reboot();
    }
}

/*! \brief Handle indication all requested links are now disconnected. */
static void appSmHandleInternalAllRequestedLinksDisconnected(SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T *msg)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalAllRequestedLinksDisconnected 0x%x Action enum:smPostDisconnectAction:%d",
                    appSmGetState(), msg->post_disconnect_action);
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    switch (msg->post_disconnect_action)
    {
        case POST_DISCONNECT_ACTION_DELETE_HANDSET_PAIRING:
            appSm_DeleteHandsetsAfterDisconnect();
            break;

        case POST_DISCONNECT_ACTION_NONE:
        default:
            break;
    }
}

static void appSm_HandleTwsTopologyRoleChange(tws_topology_role new_role)
{
    tws_topology_role old_role = SmGetTaskData()->role;

#ifdef ENABLE_SKIP_PFR
    if(old_role == tws_topology_role_primary  && new_role == tws_topology_role_secondary)
    {
        DEBUG_LOG("appSm_HandleTwsTopologyRoleChange, Set the preserved role to secondary");
        PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_secondary);
    }
    else if(old_role == tws_topology_role_secondary && new_role == tws_topology_role_primary)
    {
        DEBUG_LOG("appSm_HandleTwsTopologyRoleChange, Set the preserved role to primary");
        PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_primary);
    }
#endif

    /* Recalculate the restart_pairing flag
       The flag remembers that we were primary and in handset pairing before
       role selection. If we are now secondary we can reset the flag
       and make sure that handset connection is now allowed. 
       Flag is remembered if we adopt "none" role (in case) */
    if (!appSmIsPairing()
        && old_role == tws_topology_role_primary
        && new_role == tws_topology_role_secondary)
    {
        appSmSetRestartPairingAfterRoleSelection(FALSE);
        appSmSetRestartReconnectionAfterRoleSelection(FALSE);
    }

#ifdef ENABLE_LE_DEBUG_SECONDARY
    EarbudSecondaryDebug_HandleTwsTopologyRoleChange(new_role,old_role);
#endif

    switch (new_role)
    {
        case tws_topology_role_none:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange NONE.DFU state:%d",
                        APP_STATE_IN_CASE_DFU == appSmGetState());


            /* May need additional checks here */
            if (APP_STATE_IN_CASE_DFU != appSmGetState())
            {
                SmGetTaskData()->role = tws_topology_role_none;

                TaskList_MessageSendId(SmGetClientTasks(), EARBUD_ROLE_PRIMARY);

                SecondaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
                PrimaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            }
            else
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange staying in role:%d, DFU was in progress (app)", old_role);
            }
            break;

        case tws_topology_role_primary:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange PRIMARY");

            SmGetTaskData()->role = tws_topology_role_primary;
#ifdef ENABLE_SKIP_PFR
            /* Clear peer find role start lock since the role might have been chosen from PS key and let the power on
               prompt played immediately if PFR is not used*/
            appSmPfrStartLockClear();
#endif
            Av_SetupForPrimaryRole();
            HfpProfile_SetRole(TRUE);
            MirrorProfile_SetRole(TRUE);
#ifdef INCLUDE_SENSOR_PROFILE
            SensorProfile_SetRole(TRUE);
#endif
            LogicalInputSwitch_SetRerouteToPeer(FALSE);
            StateProxy_SetRole(TRUE);
            UiPrompts_GenerateUiEvents(TRUE);
            UiTones_GenerateUiEvents(TRUE);
#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
            LeAudioVolumeSync_SetRole(TRUE);
#endif

            TaskList_MessageSendId(SmGetClientTasks(), EARBUD_ROLE_PRIMARY);

            /* Generate role switch event for handset reconnection initiation */
            PrimaryRules_SetEvent(RULE_EVENT_ROLE_SWITCH);

#ifdef INCLUDE_DFU
            if (Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET)
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange Post Reboot DFU commit phase.");

                /*
                 * Unblock message to activate DFU timers corresponding to
                 * appConfigDfuTimeoutToStartAfterRestartMs().
                 * In case of dynamic role selection during post reboot DFU
                 * commit phase, the following is significant whereas in case
                 * of fixed roles, its a NOP as DFU timers are managed as
                 * part of entering APP_STATE_IN_CASE_DFU).
                 */
                appSmDfuDynamicRoleLockClear();
            }
            else
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange Dfu_GetRebootReason():%d.",Dfu_GetRebootReason());
#else
            {
#endif
                if (appPhyStateGetState() != PHY_STATE_IN_CASE)
                {
                    PrimaryRules_SetEvent(RULE_EVENT_OUT_CASE);
                }
            }
            break;

        case tws_topology_role_secondary:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange SECONDARY");
            SmGetTaskData()->role = tws_topology_role_secondary;

#ifdef ENABLE_SKIP_PFR
            /* Clear peer find role start lock since the role might have been chosen from PS key and let the power on
               prompt played immediately if PFR is not used*/
            appSmPfrStartLockClear();
#endif
            Av_SetupForSecondaryRole();
            HfpProfile_SetRole(FALSE);
            MirrorProfile_SetRole(FALSE);
#ifdef INCLUDE_SENSOR_PROFILE
            SensorProfile_SetRole(FALSE);
#endif
            LogicalInputSwitch_SetRerouteToPeer(TRUE);
            StateProxy_SetRole(FALSE);
            UiPrompts_GenerateUiEvents(FALSE);
            UiTones_GenerateUiEvents(FALSE);
#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
            LeAudioVolumeSync_SetRole(FALSE);
#endif

            TaskList_MessageSendId(SmGetClientTasks(), EARBUD_ROLE_SECONDARY);

#ifdef INCLUDE_DFU
            if (Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET)
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange Post Reboot DFU commit phase.");

                /*
                 * Unblock message to activate DFU timers corresponding to
                 * appConfigDfuTimeoutToStartAfterRestartMs().
                 * In case of dynamic role selection during post reboot DFU
                 * commit phase, the following is significant whereas in case
                 * of fixed roles, its a NOP as DFU timers are managed as
                 * part of entering APP_STATE_IN_CASE_DFU).
                 */
                appSmDfuDynamicRoleLockClear();

                SecondaryRules_SetEvent(RULE_EVENT_ROLE_SWITCH);
            }
            else
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange Dfu_GetRebootReason():%d.",Dfu_GetRebootReason());
#else
            {
#endif
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange old_role:%d.",old_role);
                SecondaryRules_SetEvent(RULE_EVENT_ROLE_SWITCH);
                if (appPhyStateGetState() != PHY_STATE_IN_CASE)
                {
                    SecondaryRules_SetEvent(RULE_EVENT_OUT_CASE);
                }

                /* As a primary the earbud may have been in a non-core state, in
                   particular, it may have been handset pairing. When transitioning
                   to secondary, force a transition to a core state so any primary
                   role activities are cancelled as a result of the state transition
                 */
                if (old_role == tws_topology_role_primary)
                {
                    /* For in-case DFU, re-valuation of core state causes the state transition
                     * from APP_STATE_IN_CASE_DFU to APP_STATE_IN_CASE_IDLE which results in 
                     * ending the DFU. So, avoiding it for in-case DFU.
                     */
                    if ((!appSmIsCoreState() || (appSmIsCoreState() && appSmIsPairing())) &&
                            (appSmGetState() != APP_STATE_IN_CASE_DFU))
                    {
                        appSmSetCoreStateDiscardPairing();
                    }
                }
            }
            break;

        default:
            break;
    }
}

static void appSm_HandleTwsTopologyStartCfm(TWS_TOPOLOGY_START_COMPLETED_T* cfm)
{
    
    DEBUG_LOG_STATE("appSm_HandleTwsTopologyStartCfm status enum:tws_topology_status_t:%d enum:appState:%d",
                    cfm->status, appSmGetState());

    if (appSmGetState() == APP_STATE_STARTUP)
    {
        /* topology up and running, SM can move on from STARTUP state */
        appSmSetInitialCoreState();
    }
    else
    {
        DEBUG_LOG_WARN("appSm_HandleTwsTopologyStartCfm unexpected state enum:appState:%d", appSmGetState());
    }
}

static void appSm_HandleTwsTopologyStopCfm(const TWS_TOPOLOGY_STOP_COMPLETED_T *cfm)
{
    DEBUG_LOG_STATE("appSm_HandleTwsTopologyStopCfm enum:tws_topology_status_t:%d", cfm->status);

    if (appSmSubStateIsTerminating(appSmGetState()))
    {
        switch (appSmGetState())
        {
        case APP_STATE_IN_CASE_SOPORIFIC_TERMINATING:
        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
                appPowerSleepPrepareResponse(SmGetTask());
            break;
            default:
                switch (appGetSubState())
                {
                    case APP_SUBSTATE_TERMINATING:
                        appPowerShutdownPrepareResponse(SmGetTask());
                        break;
                    default:
                        break;
                }
            break;
        }
    }
    else
    {
        DEBUG_LOG_WARN("appSm_HandleTwsTopologyStopCfm unexpected state enum:appState:%d", appSmGetState());
    }
}

static void appSm_HandleTwsTopologyRoleChangedInd(TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T* ind)
{
    DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChangedInd enum:tws_topology_role:%u", ind->role);

    appSm_HandleTwsTopologyRoleChange(ind->role);
}


static void appSmHandleDfuAllow(const CONN_RULES_DFU_ALLOW_T *dfu)
{
#ifdef INCLUDE_DFU
    Dfu_AllowUpgrades(dfu->enable);
#else
    UNUSED(dfu);
#endif
}


static void appSmHandleInternalBredrConnected(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalBredrConnected");

    /* Kick the state machine to see if this changes DFU state.
       A BREDR connection for upgrade can try an upgrade before we
       allow it. Always evaluate events do not need resetting. */
    appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
}


/*! \brief provides Earbud Application state machine context to the User Interface module.

    \param[in]  void

    \return     current_sm_ctxt - current context of sm module.
*/
static unsigned earbudSm_GetCurrentPhyStateContext(void)
{
    phy_state_provider_context_t context = BAD_CONTEXT;

    if (appSmIsOutOfCase())
    {
        context = context_phy_state_out_of_case;
    }
    else
    {
        context = context_phy_state_in_case;
    }

    return (unsigned)context;
}

#ifdef PRODUCTION_TEST_MODE
/*! \brief provides Earbud Application production test mode state context to the User Interface module.

    \param[in]  void

    \return     current_ptm_ctxt - current production test mode state
*/
static unsigned earbudSm_GetCurrentPtmStateContext(void)
{
    ptm_state_provider_context_t context = BAD_CONTEXT;

    context = sm_boot_production_test_mode == appSmTestService_BootMode() ?
                context_ptm_state_ptm : context_ptm_state_none;

    return (unsigned)context;
}
#endif

static unsigned earbudSm_GetCurrentApplicationContext(void)
{
    app_sm_provider_context_t context = BAD_CONTEXT;
    appState state = appSmGetState();

    if (appSmStateInCase(state))
    {
        context = context_app_sm_in_case;
    }
    else if (appSmStateInEar(state))
    {
        context = context_app_sm_in_ear;
    }
    else if (appSmStateOutOfCase(state))
    {
        if (state == APP_STATE_OUT_OF_CASE_BUSY)
        {
            context = context_app_sm_out_of_case_busy;
        }
        else
        {
            context = HandsetService_IsAnyDeviceConnected() ?
                        context_app_sm_out_of_case_idle_connected :
                        context_app_sm_out_of_case_idle;
        }
    }
    else
    {
        context = context_app_sm_non_core_state;
    }
    return (unsigned)context;
}

/*! \brief handles sm module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void appSmHandleUiInput(MessageId  ui_input)
{
    /* Ignore ui inputs whilst terminating connections */
    if (!appSmSubStateIsTerminating(appSmGetState()))
    {
        switch (ui_input)
        {
            case ui_input_connect_handset:
                appSmRulesSetEvent(RULE_EVENT_CONNECT_HANDSET_USER);
                break;
            case ui_input_disconnect_lru_handset:
                appSmRulesSetEvent(RULE_EVENT_DISCONNECT_LRU_HANDSET_USER);
                break;
            case ui_input_enable_multipoint:
                appSmEnableMultipoint();
                break;
            case ui_input_disable_multipoint:
                appSmDisableMultipoint();
                break;
            case ui_input_sm_pair_handset:
                appSmPairHandset();
                break;
            case ui_input_sm_delete_handsets:
                earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_delete_handsets_t));
                appSmDeleteHandsets();
                break;
            case ui_input_factory_reset_request:
                earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_factory_reset_t));
                MessageSendLater(SmGetTask(), SM_INTERNAL_FACTORY_RESET, NULL, appConfigDelayBeforeFactoryResetMs());
                break;
#ifdef INCLUDE_DFU
            case ui_input_dfu_active_when_in_case_request:
                appSmEnterDfuModeInCase(TRUE, TRUE);
                break;
#endif
#ifdef PRODUCTION_TEST_MODE
            case ui_input_production_test_mode:
#ifdef PLAY_PRODUCTION_TEST_TONES
                /* handle 3s press notification for production test mode only*/
                if(sm_boot_production_test_mode == appSmTestService_BootMode())
                {
                    appKymeraTonePlay(app_tone_button_2, 0, FALSE, FALSE, NULL, 0);
                }
#endif
                break;
            case ui_input_production_test_mode_request:
#ifdef PLAY_PRODUCTION_TEST_TONES
                if(sm_boot_production_test_mode == appSmTestService_BootMode())
                {
                    appKymeraTonePlay(app_tone_power_on, SystemClockGetTimerTime() + 300000, FALSE, FALSE, NULL, 0);
                }
#endif
                appSmEnterProductionTestMode();
                break;
#endif
#if defined(QCC3020_FF_ENTRY_LEVEL_AA) || (defined HAVE_RDP_HW_YE134) || (defined HAVE_RDP_HW_18689) || (defined HAVE_RDP_HW_MOTION)
            case ui_input_force_reset:
                Panic();
                break;
#endif
            case ui_input_dts_mode:
              appSmEnterDtsMode(DTS_MODE_ENABLED);
              break;

            case ui_input_dts_mode_idle:
              appSmEnterDtsMode(DTS_MODE_ENABLED_IDLE);
              break;

            case ui_input_dts_mode_dut:
              appSmEnterDtsMode(DTS_MODE_ENABLED_DUT);
              break;
              
            case ui_input_disable_touchpad:
                earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_disable_touchpad_t));
                appSmDisableTouchpad();
            break;

            default:
                break;
        }
    }
}

static void appSmGeneratePhyStateRulesEvents(phy_state_event event)
{
    switch(event)
    {
        case phy_state_event_in_case:
            appSmRulesSetEvent(RULE_EVENT_PEER_IN_CASE);
            break;
        case phy_state_event_out_of_case:
            appSmRulesSetEvent(RULE_EVENT_PEER_OUT_CASE);
            break;
        case phy_state_event_in_ear:
            appSmRulesSetEvent(RULE_EVENT_PEER_IN_EAR);
            break;
        case phy_state_event_out_of_ear:
            appSmRulesSetEvent(RULE_EVENT_PEER_OUT_EAR);
            break;
        default:
            DEBUG_LOG_WARN("appSmGeneratePhyStateRulesEvents unhandled event %u", event);
            break;
    }
}


static void appSmHandleStateProxyEvent(const STATE_PROXY_EVENT_T* sp_event)
{
    DEBUG_LOG_DEBUG("appSmHandleStateProxyEvent enum:state_proxy_source:%u enum:state_proxy_event_type:%u", sp_event->source, sp_event->type);
    switch(sp_event->type)
    {
        case state_proxy_event_type_phystate:
            {
                smTaskData* sm = SmGetTaskData();
                /* Get latest physical state */
                sm->phy_state = appPhyStateGetState();

                DEBUG_LOG("appSmHandleStateProxyEvent phystate new_state=enum:phyState:%d event=enum:phy_state_event:%d",
                          sp_event->event.phystate.new_state, sp_event->event.phystate.event);

                if (sp_event->source == state_proxy_source_remote)
                {
                    appSmGeneratePhyStateRulesEvents(sp_event->event.phystate.event);

#ifdef ENABLE_SKIP_PFR
                    if(sm->role == tws_topology_role_primary)
                    {
                        if(sp_event->event.phystate.new_state >= PHY_STATE_OUT_OF_EAR &&
                            sp_event->event.phystate.new_state <= PHY_STATE_IN_EAR)
                        {
                            /* When secondary is out of case, kick the sm after a timeout to check whether the static handover
                             * needs to be done or not based on the local phy state */
                            TwsTopology_CancelSecondaryOutOfCaseMessage();
                            TwsTopology_SendSecondaryOutOfCaseMessage();
                        }
                        else if(sp_event->event.phystate.new_state == PHY_STATE_IN_CASE)
                        {
                            /* If secondary is kept in case, then cancel the secondary out of case message */
                            TwsTopology_CancelSecondaryOutOfCaseMessage();
                        }
                    }
#endif
#ifdef INCLUDE_DFU
                    /* check and inform the DFU domain if the earbuds are not-in-use state(used for silent commit) */
                    appSmCheckAndNotifyDeviceNotInUse();
#endif /* INCLUDE_DFU */
                }
#ifdef PRODUCTION_TEST_MODE
                else if (sp_event->source == state_proxy_source_local && sm_boot_production_test_mode == appSmTestService_BootMode())
                {
                    /* for production test */
                    if(appSmIsOutOfCase())
                    {
                        if (sp_event->event.phystate.event == phy_state_event_in_ear)
                        {
                            appInEarDetected(); /* 2 flash */
                            appKymeraTonePlay(app_tone_button, 0, TRUE, FALSE, NULL, 0);
                        }
                        else if (sp_event->event.phystate.event == phy_state_event_out_of_ear)
                        {
                            appOutEarDetected(); /* 4 flash */
                            appKymeraTonePlay(app_tone_button_3, 0, TRUE, FALSE, NULL, 0);
                        }

                        /* disable scan and advert to avoid peer pairing*/
                        LeScanManager_Disable(SmGetTask());
                        LeAdvertisingManager_EnableConnectableAdvertising(SmGetTask(), FALSE);
                    }
                }
#endif
            }
            break;

        default:
            break;
    }
}

static void appSmHandleConnRulesAncEnable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncEnable ANC enabled");
    Ui_InjectUiInput(ui_input_anc_on);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_ENABLE);
}

static void appSmHandleConnRulesAncDisable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncDisable ANC disable");
    Ui_InjectUiInput(ui_input_anc_off);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_DISABLE);
}

/*! \brief Enter ANC tuning mode */
static void appSmHandleConnRulesAncEnterTuning(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncEnterTuning ANC Enter Tuning Mode");

    /* Inject UI input ANC enter tuning */
    Ui_InjectRedirectableUiInput(ui_input_anc_enter_tuning_mode, FALSE);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_TUNING_START);
}

/*! \brief Exit ANC tuning mode */
static void appSmHandleConnRulesAncExitTuning(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncExitTuning ANC Exit Tuning Mode");

    /* Inject UI input ANC exit tuning */
    Ui_InjectRedirectableUiInput(ui_input_anc_exit_tuning_mode, FALSE);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_TUNING_STOP);
}

/*! \brief Exit Adaptive ANC tuning mode */
static void appSmHandleConnRulesAdaptiveAncExitTuning(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAdaptiveAncExitTuning Exit Adaptive ANC Tuning Mode");

    /* Inject UI input ANC exit Adaptive ANC tuning */
    Ui_InjectRedirectableUiInput(ui_input_anc_exit_adaptive_anc_tuning_mode, FALSE);

    appSmRulesSetRuleComplete(CONN_RULES_ADAPTIVE_ANC_TUNING_STOP);
}

/*! \brief Disable Leakthrough */
static void appSmHandleConnRulesLeakthroughDisable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLeakthroughDisable Leakthrough Disable");

    if(appSmIsInCase() && AecLeakthrough_IsLeakthroughEnabled())
    {
        Ui_InjectUiInput(ui_input_leakthrough_off);
    }

    appSmRulesSetRuleComplete(CONN_RULES_LEAKTHROUGH_DISABLE);
}

/*! \brief Handle setting remote audio mix */
static void appSmHandleConnRulesSetRemoteAudioMix(CONN_RULES_SET_REMOTE_AUDIO_MIX_T *remote)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesSetRemoteAudioMix stereo_mix=%d", remote->stereo_mix);

    if (appPeerSigIsConnected())
    {
        marshal_type_t type = remote->stereo_mix ?
                                MARSHAL_TYPE(earbud_sm_req_stereo_audio_mix_t) :
                                MARSHAL_TYPE(earbud_sm_req_mono_audio_mix_t);
        earbudSm_CancelAndSendCommandToPeer(type);
    }

    appSmRulesSetRuleComplete(CONN_RULES_SET_REMOTE_AUDIO_MIX);
}

/*! \brief Handle setting local audio mix */
static void appSmHandleConnRulesSetLocalAudioMix(CONN_RULES_SET_LOCAL_AUDIO_MIX_T *local)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesSetLocalAudioMix stereo_mix=%d", local->stereo_mix);

    appKymeraSetStereoLeftRightMix(local->stereo_mix);
    
#if defined (INCLUDE_LE_AUDIO_UNICAST) && defined (ENABLE_LE_AUDIO_FT_UPDATE)
     LeUnicastManager_EnableSourceMix(local->stereo_mix);
#endif

    appSmRulesSetRuleComplete(CONN_RULES_SET_LOCAL_AUDIO_MIX);
}

/*! \brief Handle PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION message */
static void appSm_HandlePeerFindRolePrepareForRoleSelection(void)
{
    appState state= appSmGetState();

    DEBUG_LOG_DEBUG("appSm_HandlePeerFindRolePrepareForRoleSelection state=%d", state);

    if(appSmIsPairing())
    {
        /* Response is called in appSm_HandleCancelPairHandsetCfm */
        appSmSetRestartPairingAfterRoleSelection(TRUE);
        HandsetService_CancelPairHandset(SmGetTask());
    }
    else if (HandsetService_IsConnecting())
    {
        if (HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_link_loss_reconnecting))
        {
            DEBUG_LOG_VERBOSE("appSm_HandlePeerFindRolePrepareForRoleSelection continue link loss reconnection");

            appSmSetReconnectPostHandover(TRUE);
            appSmSetContinueLinkLossReconnectPostHandover(TRUE);
        }

        /* Response is called in appSmHandleHandsetServiceMpConnectStopCfm */
        HandsetService_StopReconnect(SmGetTask());

        appSmSetRestartReconnectionAfterRoleSelection(TRUE);
    }
    else
    {
        PeerFindRole_PrepareResponse(SmGetTask());
    }
}

/*! \brief Handle PEER_FIND_ROLE_SECONDARY role selection 
           The primary will transition through other states
           which may trigger the ANC enable/disable algorithm.
           However, we want the secondary to also enable ANC if
           both earbuds start "out of the ear" and the user
           chooses to wear the secondary first.
*/
static void appSm_HandlePeerFindRoleSecondaryRoleSelected(void)
{   
    DEBUG_LOG_STATE("appSm_HandlePeerFindRoleSecondaryRoleSelected();");
    appSmPrepareInitialANC();
}


/*! \brief Handle PEER_FIND_ROLE_PRIMARY role selection */
static void appSm_HandlePeerFindRolePrimaryRoleSelected(void)
{   
    DEBUG_LOG_STATE("appSm_HandlePeerFindRolePrimaryRoleSelected");

    if(appSmRestartPairingAfterRoleSelection())
    {
        /* Re-enable Handset pairing on being selected as primary role */
        smTaskData *sm = SmGetTaskData();
        MessageSend(&sm->task, SM_INTERNAL_PAIR_HANDSET, NULL);
        
        appSmSetRestartPairingAfterRoleSelection(FALSE);
    }
    else if (appSmRestartReconnectionAfterRoleSelection())
    {
        if (appSmGetContinueLinkLossReconnectPostHandover())
        {
            HandsetService_ReconnectLinkLossRequest(SmGetTask());
            appSmSetContinueLinkLossReconnectPostHandover(FALSE);
        }
        else
        {
            HandsetService_ReconnectRequest(SmGetTask());
        }
        appSmSetRestartReconnectionAfterRoleSelection(FALSE);
    }
}

/*! \brief Handle PEER_FIND_ROLE_STARTED message */
static void appSm_HandlePeerFindRoleStarted(void)
{
    DEBUG_LOG_STATE("appSm_HandlePeerFindRoleStarted");

    /* Clear peer find role start lock */
    appSmPfrStartLockClear();

    /* Now we can unregister for PEER_FIND_ROLE_STARTED */
    PeerFindRole_UnregisterStartClient(&SmGetTaskData()->task);
}

/*! \brief Handle PAIRING_STOP_CFM message */
static void appSm_HandleCancelPairHandsetCfm(void)
{
    DEBUG_LOG_DEBUG("appSm_HandleCancelPairHandsetCfm");

    PeerFindRole_PrepareResponse(SmGetTask());
}

static void appSm_HandleDeviceTestServiceEnded(void)
{
    appSmSetState(APP_STATE_STARTUP);
}


static void appSmHandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG("appSmHandleSystemStateChange state enum:system_state_state_t:%d -> enum:system_state_state_t:%d", msg->old_state, msg->new_state);

    if(msg->old_state == system_state_starting_up && msg->new_state == system_state_active)
    {
        appSmHandleInitConfirm();
    }
}

#ifdef USE_SYNERGY
static void appSmHandleCmSecurityEventInd(Task task, void *msg)
{
    CsrBtCmSecurityEventInd *ind = (CsrBtCmSecurityEventInd *) msg;

    if (ind->event == CSR_BT_CM_SECURITY_EVENT_DEBOND)
    {
        bdaddr bd_addr = { 0 };

        BdaddrConvertBluestackToVm(&bd_addr, &ind->addrt.addr);

        DEBUG_LOG_DEBUG("appSmHandleCmSecurityEventInd CSR_BT_CM_SECURITY_EVENT_DEBOND type: 0x%x lap 0x%x",
                        ind->addrt.type,
                        bd_addr.lap);

        if (!KeySync_IsPdlUpdateInProgress(&bd_addr))
		{
            appSmHandleNotifyPeerDeleteHandset(bd_addr);
        }
    }

    UNUSED(task);
}

static void appSmHandleCmPrim(Task task, void *msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_MASK_CFM:
            /* Ignore */
            break;

        case CSR_BT_CM_SECURITY_EVENT_IND:
            appSmHandleCmSecurityEventInd(task, msg);
            break;

        default:
            DEBUG_LOG_DEBUG("appSmHandleCmPrim, unexpected CM prim 0x%x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents(msg);
}
#endif

#ifdef INCLUDE_CASE_COMMS
static void appSm_HandleCaseLidState(const CASE_LID_STATE_T* cls)
{
    DEBUG_LOG("appSm_HandleCaseLidState");

    /* Use this to trigger a Soc event.
     * Normally the low and ok levels are handled as a UI context, so don't need
     * any triggering. However, if the battery is full, then the UI is a one-off
     * event, so needs to be sent again.
     */
    if (cls->lid_state == CASE_LID_STATE_OPEN || cls->lid_state == CASE_LID_STATE_CLOSED)
    {
        /* SoC Force resend event */
        Soc_ForceNotifyBatteryChanged();
    }

    /* Generate or clear the lid status event in the rules engine.
       Used to decide if automatic handset pairing should be started when the case
       lid is opened. */
    if (cls->lid_state == CASE_LID_STATE_OPEN)
    {
        TimestampEvent(TIMESTAMP_EVENT_LID_OPENED);
        appSmRulesSetEvent(RULE_EVENT_CASE_LID_OPEN);
    }
    else
    {
        appSmRulesResetEvent(RULE_EVENT_CASE_LID_OPEN);

        /* If the lid was closed and we're pairing attempt to stop pairing
           will return to core state once pairing is stopped, may be delayed
           until pairing actually completes, if already reached authentication
           stage of pairing */
        if (cls->lid_state == CASE_LID_STATE_CLOSED && appSmIsPairing())
        {
            HandsetService_CancelPairHandset(FALSE);
        }
    }
}
#endif

/*! \brief Application state machine message handler. */
void appSmHandleMessage(Task task, MessageId id, Message message)
{
    smTaskData* sm = (smTaskData*)task;

    if (isMessageUiInput(id))
    {
        appSmHandleUiInput(id);
        return;
    }

    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            appSmHandleCmPrim(task, (void *) message);
            break;
#endif
#ifdef INCLUDE_CASE_COMMS
        case CASE_LID_STATE:
            appSm_HandleCaseLidState((const CASE_LID_STATE_T *)message);
        case CASE_POWER_STATE:
            appSmHandleCaseMessages();
            break;
#endif
        case SYSTEM_STATE_STATE_CHANGE:
            appSmHandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
            break;

        /* Pairing completion confirmations */
        case HANDSET_SERVICE_PAIR_HANDSET_CFM:
            appSmHandlePairHandsetConfirm();
            break;

        /* Connection manager status indications */
        case CON_MANAGER_CONNECTION_IND:
            appSmHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

        /* Handset Service status change indications */
        case HANDSET_SERVICE_CONNECTED_IND:
            appSmHandleHandsetServiceConnectedInd((HANDSET_SERVICE_CONNECTED_IND_T *)message);
            break;
        case HANDSET_SERVICE_DISCONNECTED_IND:
            appSmHandleHandsetServiceDisconnectedInd((HANDSET_SERVICE_DISCONNECTED_IND_T *)message);
            break;
        case HANDSET_SERVICE_MP_CONNECT_STOP_CFM:
            appSmHandleHandsetServiceMpConnectStopCfm((const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T *)message);
            break;

        /* AV status change indications */
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case LE_AUDIO_UNICAST_MEDIA_CONNECTED:
#endif
        case AV_STREAMING_ACTIVE_IND:
        case MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND:
            appSmHandleAvStreamingActiveInd(id);
            break;

#ifdef INCLUDE_LE_AUDIO_UNICAST
        case LE_AUDIO_UNICAST_MEDIA_DISCONNECTED:
#endif
        case AV_STREAMING_INACTIVE_IND:
        case MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND:
            appSmHandleAvStreamingInactiveInd();
            break;

        case APP_HFP_SCO_CONNECTED_IND:
        case MIRROR_PROFILE_ESCO_CONNECT_IND:
            appSmHandleHfpScoConnectedInd();
            break;

        case APP_HFP_SCO_DISCONNECTED_IND:
        case MIRROR_PROFILE_ESCO_DISCONNECT_IND:
            appSmHandleHfpScoDisconnectedInd();
            break;

        /* Physical state changes */
        case PHY_STATE_CHANGED_IND:
            appSmHandlePhyStateChangedInd(sm, (PHY_STATE_CHANGED_IND_T*)message);
            break;

        /* Power indications */
        case APP_POWER_SLEEP_PREPARE_IND:
            appSmHandlePowerSleepPrepareInd();
            break;
        case APP_POWER_SLEEP_CANCELLED_IND:
            appSmHandlePowerSleepCancelledInd();
            break;
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            appSmHandlePowerShutdownPrepareInd();
            break;
        case APP_POWER_SHUTDOWN_CANCELLED_IND:
            appSmHandlePowerShutdownCancelledInd();
            break;

        /* Connection rules */
#ifdef INCLUDE_FAST_PAIR
        case CONN_RULES_PEER_SEND_FP_ACCOUNT_KEYS:
            appSmHandleConnRulesForwardAccountKeys();
            break;
#endif

#ifdef INCLUDE_SENSOR_PROFILE
        case CONN_RULES_SPATIAL_AUDIO_DISABLE:
             appSmHandleConnRulesSpatialAudioDisable();
             break;
#endif
        case CONN_RULES_PEER_SEND_LINK_KEYS:
            appSmHandleConnRulesForwardLinkKeys();
            break;
        case CONN_RULES_MEDIA_PAUSE:
            appSmHandleConnRulesMediaTimeout();
            break;
        case CONN_RULES_MEDIA_TIMEOUT_CANCEL:
            appSmCancelOutOfEarTimers();
            break;
        case CONN_RULES_MEDIA_PLAY:
            appSmHandleConnRulesMediaPlay();
            break;
        case CONN_RULES_SCO_TIMEOUT:
            appSmHandleConnRulesScoTimeout();
            break;
        case CONN_RULES_ACCEPT_INCOMING_CALL:
            appSmHandleConnRulesAcceptincomingCall();
            break;
        case CONN_RULES_SCO_TRANSFER_TO_EARBUD:
            appSmHandleConnRulesScoTransferToEarbud();
            break;
        case CONN_RULES_SCO_TRANSFER_TO_HANDSET:
            appSmHandleConnRulesScoTransferToHandset();
            break;
        case CONN_RULES_LED_ENABLE:
            appSmHandleConnRulesLedEnable();
            break;
        case CONN_RULES_LED_DISABLE:
            appSmHandleConnRulesLedDisable();
            break;
        case CONN_RULES_HANDSET_PAIR:
            appSmHandleConnRulesHandsetPair();
            break;
        case CONN_RULES_HANDSET_CONNECT:
            appSmHandleConnRulesHandsetConnect((CONN_RULES_HANDSET_CONNECT_T *)message);
            break;
        case CONN_RULES_LRU_HANDSET_DISCONNECT:
            appSmHandleConnRulesLruHandsetDisconnect();
            break;
        case CONN_RULES_ALL_HANDSETS_DISCONNECT:
            appSmHandleConnRulesAllHandsetsDisconnect();
            break;
        case CONN_RULES_ENTER_DFU:
            appSmHandleConnRulesEnterDfu();
            appSmRulesSetRuleComplete(CONN_RULES_ENTER_DFU);
            break;
        case CONN_RULES_PEER_SCO_CONTROL:
            appSmHandleConnRulesPeerScoControl((CONN_RULES_PEER_SCO_CONTROL_T*)message);
            break;
        case CONN_RULES_ANC_TUNING_START:
            appSmHandleConnRulesAncEnterTuning();
            break;
        case CONN_RULES_ANC_TUNING_STOP:
            appSmHandleConnRulesAncExitTuning();
            break;
        case CONN_RULES_ADAPTIVE_ANC_TUNING_STOP:
            appSmHandleConnRulesAdaptiveAncExitTuning();
            break;
        case CONN_RULES_ANC_ENABLE:
            appSmHandleConnRulesAncEnable();
            break;
        case CONN_RULES_ANC_DISABLE:
            appSmHandleConnRulesAncDisable();
            break;
        case CONN_RULES_LEAKTHROUGH_DISABLE:
            appSmHandleConnRulesLeakthroughDisable();
            break;
        case CONN_RULES_SET_REMOTE_AUDIO_MIX:
            appSmHandleConnRulesSetRemoteAudioMix((CONN_RULES_SET_REMOTE_AUDIO_MIX_T*)message);
            break;
        case CONN_RULES_SET_LOCAL_AUDIO_MIX:
            appSmHandleConnRulesSetLocalAudioMix((CONN_RULES_SET_LOCAL_AUDIO_MIX_T*)message);
            break;
        case CONN_RULES_AUDIO_UI_IND_CONTROL:
            appSmHandleConnRulesAudioUiIndControl((CONN_RULES_AUDIO_UI_IND_CONTROL_T *)message);
            break;
        case CONN_RULES_DFU_ALLOW:
            appSmHandleDfuAllow((const CONN_RULES_DFU_ALLOW_T*)message);
            break;
        case SEC_CONN_RULES_ENTER_DFU:
            appSmHandleConnRulesEnterDfu();
            appSmRulesSetRuleComplete(SEC_CONN_RULES_ENTER_DFU);
            break;
        /* Peer signalling messages */
        case PEER_SIG_CONNECTION_IND:
        {
            const PEER_SIG_CONNECTION_IND_T *ind = (const PEER_SIG_CONNECTION_IND_T *)message;
            GamingMode_HandlePeerSigConnected(ind);
            appSmHandlePeerSigConnectionInd(ind);
            break;
        }

        /* Device property related messages */
        case BT_DEVICE_SELF_CREATED_IND:
            appSmHandleBtDeviceSelfCreated((const BT_DEVICE_SELF_CREATED_IND_T*) message);
            break;

        /* TWS Topology messages */
        case TWS_TOPOLOGY_START_COMPLETED:
            appSm_HandleTwsTopologyStartCfm((TWS_TOPOLOGY_START_COMPLETED_T*)message);
            if(appPhyStateIsOutOfCase())
            {
                TwsTopology_Join();
            }
            break;

        case TWS_TOPOLOGY_STOP_COMPLETED:
            appSm_HandleTwsTopologyStopCfm((TWS_TOPOLOGY_STOP_COMPLETED_T*)message);
            break;

        case TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED:
            appSm_HandleTwsTopologyRoleChangedInd((TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T*)message);
            break;

#if defined (INCLUDE_GAIA) && !defined (ALLOW_GAIA_WITH_DTS)
        case APP_GAIA_SERVICE_STOPPED:
            appSm_HandleGaiaStopServiceCfm((GAIA_STOP_SERVICE_CFM_T*)message);
        break;
#endif

#ifdef INCLUDE_DFU
        case DFU_TRANSPORT_CONNECTED:
            appSmHandleUpgradeConnected();
            DEBUG_LOG_INFO("appSmHandleMessage, Setting upgrade ctx as : UPGRADE_CONTEXT_GAIA");
            /* Set the upgrade context on primary device and sync it with 
             * secondary device/peer.*/
            Dfu_SetContext(UPGRADE_CONTEXT_GAIA);
            break;

        case DFU_TRANSPORT_DISCONNECTED:
            appSmHandleUpgradeDisconnected();
            break;

        /* Messages from UPGRADE */
        case DFU_REQUESTED_TO_CONFIRM:
            break;

        case DFU_REQUESTED_IN_PROGRESS:
            break;

        case DFU_ACTIVITY:
            /* We do not monitor activity at present.
               Might be a use to detect long upgrade stalls without a disconnect */
            break;

        case DFU_STARTED:
            appSmNotifyUpgradeStarted();
            break;

        case DFU_COMPLETED:
            appSmHandleDfuEnded(FALSE);
            GattServerGatt_SetGattDbChanged();
            break;

        /* Clean up DFU state variables after abort */
        case DFU_CLEANUP_ON_ABORT:
            appSmHandleDfuEnded(TRUE);
            break;

        case DFU_ABORTED:
            appSmHandleDfuEnded(TRUE);
            break;

#ifdef INCLUDE_DFU_PEER
        case DFU_PEER_STARTED:
        {
            /* During ongoing DFU, for peer device, trigger
             * appSmCancelDfuTimers() to extend the DFU timeout.
             */
            if(!BtDevice_IsMyAddressPrimary())
            {
                /* Trigger appSmCancelDfuTimers() to extend DFU timeout */
                appSmNotifyUpgradeStarted();
            }

            /* Inject an event to the rules engine to make sure DFU is enabled */
            appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
        }
            break;
        case DFU_PEER_DISCONNECTED:
        {
            /* Inject an event to the rules engine to make sure DFU check is enabled 
             * after current DFU is either completed or aborted and dfu peers are
             * disconnected.
             */
            DEBUG_LOG_INFO("appSmHandleMessage DFU_PEER_DISCONNECTED");
            appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
        }
            break;
#endif /* INCLUDE_DFU_PEER */

        case DFU_READY_FOR_SILENT_COMMIT:
        {
            DEBUG_LOG("appSmHandleMessage DFU_READY_FOR_SILENT_COMMIT");
            /* DFU domain is ready to do the silent commit. check and inform the DFU domain
             * if the earbuds are already in not-in-use state */
            appSmCheckAndNotifyDeviceNotInUse();
        }
            break;

#ifdef INCLUDE_DFU_CASE
        case DFU_CASE_EARBUDS_IN_CASE_REQUSETED:
        {
            DEBUG_LOG("appSmHandleMessage DFU_CASE_EARBUDS_IN_CASE_REQUSETED");
            /* dfu domain has requested user to put earbuds in case so, topology should maintain
             * its state while earbud goes in case for DFU. */
            TwsTopology_DisableRoleSwapSupport();
        }
        break;

        case DFU_CASE_EARBUDS_IN_CASE_CONFIRM:
        {
            DEBUG_LOG("appSmHandleMessage DFU_CASE_EARBUDS_IN_CASE_CONFIRM");
            TwsTopology_EnableRoleSwapSupport();
        }
        break;
#endif /* INCLUDE_DFU_CASE */
#endif /* INCLUDE_DFU */

        case STATE_PROXY_EVENT:
            appSmHandleStateProxyEvent((const STATE_PROXY_EVENT_T*)message);
            break;
        case STATE_PROXY_EVENT_INITIAL_STATE_RECEIVED:
#ifdef INCLUDE_DFU
            DEBUG_LOG("appSmHandleMessage STATE_PROXY_EVENT_INITIAL_STATE_RECEIVED");
            /* state_proxy exchanges this INITIAL_STATE after every peer reconnection to
             * synchronize with the peer. During the linkloss, peer's phy state could have changed.
             * so, verify and inform the DFU domain if earbuds are not-in-use state. */
            appSmCheckAndNotifyDeviceNotInUse();
#endif /* INCLUDE_DFU */
            break;

        /* SM internal messages */
        case SM_INTERNAL_PAIR_HANDSET:
            appSmHandleInternalPairHandset();
            break;
        case SM_INTERNAL_DELETE_HANDSETS:
            appSmHandleInternalDeleteHandsets();
            break;
        case SM_INTERNAL_FACTORY_RESET:
            appSmHandleInternalFactoryReset();
            break;
        case SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION:
            appSmHandleInternalLinkDisconnectionTimeout();
            break;

#ifdef INCLUDE_DFU
        case SM_INTERNAL_ENTER_DFU_UI:
            /*
             * For start of an in-case DFU, the app state of
             * APP_STATE_IN_CASE_DFU needs to be entered and hence the 2nd
             * argument is insignificant and is passed as FALSE to allow enter
             * APP_STATE_IN_CASE_DFU.
             */
            appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutAfterEnteringCaseMs(), FALSE);
            break;

        case SM_INTERNAL_ENTER_DFU_UPGRADED:
            /* Can be used to handle any DFU specific operation on App side 
             * post DFU reboot
             */
            break;

        case SM_INTERNAL_ENTER_DFU_STARTUP:
            appSmHandleEnterDfuStartup(*((bool *)message));
            break;

        case SM_INTERNAL_TIMEOUT_DFU_ENTRY:
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_ENTRY");
            EarbudSm_HandleDfuStartTimeoutNotifySecondary();
            appSmHandleDfuEnded(TRUE);
            DfuCase_Abort();
            break;

        case SM_INTERNAL_TIMEOUT_DFU_MODE_START:
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_MODE_START");

            appSmEnterDfuModeInCase(FALSE, FALSE);
            DfuCase_Abort();
            break;

        case SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT:
            /* No action needed for this timer */
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT");
            break;
#endif /* INCLUDE_DFU */

        case SM_INTERNAL_NO_DFU:
            appSmHandleNoDfu();
            break;

        case SM_INTERNAL_TIMEOUT_OUT_OF_EAR_MEDIA:
            appSmHandleInternalTimeoutOutOfEarMedia();
            break;

        case SM_INTERNAL_TIMEOUT_IN_EAR_MEDIA_START:
            appSmHandleInternalTimeoutInEarMediaStart();
            break;

        case SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO:
            appSmHandleInternalTimeoutOutOfEarSco();
            break;

        case SM_INTERNAL_TIMEOUT_IDLE:
            appSmHandleInternalTimeoutIdle();
            break;

        case SM_INTERNAL_TIMEOUT_PEER_WAS_PAIRING:
            appSmPeerStatusClearWasPairingEnteringCase();
            break;

        case SM_INTERNAL_TIMEOUT_BLOCK_RESET_HANDSET_DELETE:
            appSmHandleMinimumDeleteTimeout();
            break;

        case SM_INTERNAL_REBOOT:
            appSmHandleInternalReboot();
            break;

        case SM_INTERNAL_APP_POWER_ON:
            appPowerOn();
            break;

        case SM_INTERNAL_LINK_DISCONNECTION_COMPLETE:
            appSmHandleInternalAllRequestedLinksDisconnected((SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T *)message);
            break;

        case SM_INTERNAL_BREDR_CONNECTED:
            appSmHandleInternalBredrConnected();
            break;

            /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            appSm_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            appSm_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

        case PAIRING_ACTIVITY:
            appSm_HandlePairingActivity((PAIRING_ACTIVITY_T*)message);
            break;

        case PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION:
            appSm_HandlePeerFindRolePrepareForRoleSelection();
            break;

        case PEER_FIND_ROLE_PRIMARY:
            appSm_HandlePeerFindRolePrimaryRoleSelected();
            break;

        case PEER_FIND_ROLE_SECONDARY:
            appSm_HandlePeerFindRoleSecondaryRoleSelected();
            break;

        case PEER_FIND_ROLE_STARTED:
            appSm_HandlePeerFindRoleStarted();
            break;

        case HANDSET_SERVICE_CANCEL_PAIR_HANDSET_CFM:
            appSm_HandleCancelPairHandsetCfm();
            break;

#ifdef PRODUCTION_TEST_MODE
        case SM_INTERNAL_ENTER_PRODUCTION_TEST_MODE:
            appSmHandleInternalEnterProductionTestMode();
            break;

        case SM_INTERNAL_ENTER_DUT_TEST_MODE:
            appSmHandleInternalEnterDUTTestMode();
            break;
#endif

#ifdef ENABLE_LE_DEBUG_SECONDARY
        case LE_DEBUG_SECONDARY_UPDATE_IRK_CFM:
            EarbudSecondaryDebug_HandleUpdateIrkCfm((LE_DEBUG_SECONDARY_UPDATE_IRK_CFM_T *)message);
            break;
#endif

        case DEVICE_TEST_SERVICE_ENDED:
            appSm_HandleDeviceTestServiceEnded();
            break;

        case SM_INTERNAL_ENTER_DTS_MODE:
            appSm_HandleEnterDtsMode(*((uint16 *)message));
            break;

        case CHARGER_MESSAGE_COMPLETED:
            appChargerCompleted();
            break;

#ifdef INCLUDE_HALL_EFFECT_SENSOR
        case CHARGER_MESSAGE_DETACHED:
            appChargerCompleted();   /* This function begins power down. */
            break;

        case CHARGER_MESSAGE_ATTACHED:  
            /* Cancel the timeout message which may push us into the soporific state (which
             * ultimately drives us to dormant). 
             * If we're already in SOPORIFIC state then we're committed to entering dormant 
             * so this will have no real effect. */
            MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
            break;
#endif

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}

bool appSmIsInEar(void)
{
    smTaskData *sm = SmGetTaskData();
    return sm->phy_state == PHY_STATE_IN_EAR;
}

bool appSmIsOutOfEar(void)
{
    smTaskData *sm = SmGetTaskData();
    return (sm->phy_state >= PHY_STATE_IN_CASE) && (sm->phy_state <= PHY_STATE_OUT_OF_EAR_AT_REST);
}

bool appSmIsInCase(void)
{
    smTaskData *sm = SmGetTaskData();
    return (sm->phy_state == PHY_STATE_IN_CASE) || (sm->phy_state == PHY_STATE_UNKNOWN);
}

bool appSmIsOutOfCase(void)
{
    smTaskData *sm = SmGetTaskData();
    DEBUG_LOG_DEBUG("appSmIsOutOfCase Current State %d", sm->phy_state);
    return (sm->phy_state >= PHY_STATE_OUT_OF_EAR) && (sm->phy_state <= PHY_STATE_IN_EAR);
}

bool appSmIsInDfuMode(void)
{
    /* Check if EB is in DFU mode. This is used for in-case DFU to know if EB
     * is in DFU mode before entering in-case
     * Variable enter_dfu_mode is set while entering in DFU mode through UI
     * and gets reset when entering in-case DFU, appEnterInCaseDfu().
     * App state gets set to APP_STATE_IN_CASE_DFU after this.
     * So to check the DFU mode, check if either enter_dfu_mode is set or app
     * state is APP_STATE_IN_CASE_DFU.
     * This is not used for out-of-case DFU.
     */
#ifdef INCLUDE_DFU
    return (Dfu_GetTaskData()->enter_dfu_mode
           || APP_STATE_IN_CASE_DFU == appSmGetState());
#else
    return FALSE;
#endif
}

bool appSmGetANCStateWhenOutOfEar(void)
{
    smTaskData *state = SmGetTaskData();
    return state->ANC_state_when_out_of_ear;
}

void appSmSetANCStateWhenOutOfEar(bool ANC_state)
{
    smTaskData *state = SmGetTaskData();
    state->ANC_state_when_out_of_ear = ANC_state;
}

void appSmPairHandset(void)
{
    smTaskData *sm = SmGetTaskData();
    MessageSend(&sm->task, SM_INTERNAL_PAIR_HANDSET, NULL);
}

void appSmDeleteHandsets(void)
{
    smTaskData *sm = SmGetTaskData();
    MessageSend(&sm->task, SM_INTERNAL_DELETE_HANDSETS, NULL);
}

void appSmEnterDtsMode(uint16 mode)
{
  uint16 *msg = (uint16 *)PanicUnlessMalloc(sizeof(uint16));
  *msg = mode;

  MessageSend(SmGetTask(), SM_INTERNAL_ENTER_DTS_MODE, msg);
}

#ifdef INCLUDE_DFU
void appSmEnterDfuMode(void)
{
    MessageSend(SmGetTask(),SM_INTERNAL_ENTER_DFU_UI, NULL);
}

/*! \brief Start DFU Timers.
    \param hasDynamicRole Indicates whether dynamic role is supported in the post
                          reboot DFU commit phase.
                          TRUE then supported and hence skip
                          APP_STATE_IN_CASE_DFU but rather enter
                          APP_STATE_STARTUP for dynamic role selection.
                          FALSE then dynamic role selection is unsupported and
                          hence APP_STATE_IN_CASE_DFU is entered.
                          FALSE is also used in DFU phases other than post
                          reboot DFU commit phase.
*/
static void appSmStartDfuTimer(bool hasDynamicRole)
{
    appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutAfterGaiaConnectionMs(), hasDynamicRole);
}

void appSmEnterDfuModeInCase(bool enableDfuMode, bool inform_peer)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_MODE_START);
    Dfu_GetTaskData()->enter_dfu_mode = enableDfuMode;
    enableDfuMode ? appEnableKeepTopologyAliveForDfu() : appDisableKeepTopologyAliveForDfu();
    enableDfuMode ? TwsTopology_DisableRoleSwapSupport() : TwsTopology_EnableRoleSwapSupport();

    if (enableDfuMode)
    {
        DEBUG_LOG_DEBUG("appSmEnterDfuModeInCase (re)start SM_INTERNAL_TIMEOUT_DFU_MODE_START %dms",
                            appConfigDfuTimeoutToPlaceInCaseMs());
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_MODE_START,
                            NULL, appConfigDfuTimeoutToPlaceInCaseMs());
        if(BtDevice_IsMyAddressPrimary())
        {
            DfuCase_HandleDfuMode();
        }
    }

    if (inform_peer)
        earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_dfu_active_when_in_case_t));
}


/*! \brief Tell the state machine to enter DFU mode following a reboot.
    \param upgrade_reboot If TRUE, indicates that DFU triggered the reboot.
                          If FALSE, indicates the device was rebooted whilst
                          an upgrade was in progress.
    \param hasDynamicRole Indicates whether dynamic role is supported in the post
                          reboot DFU commit phase.
                          TRUE then supported and hence skip
                          APP_STATE_IN_CASE_DFU but rather enter
                          APP_STATE_STARTUP for dynamic role selection.
                          FALSE then dynamic role selection is unsupported and
                          hence APP_STATE_IN_CASE_DFU is entered.
                          FALSE is also used in DFU phases other than post
                          reboot DFU commit phase.
*/
static void appSmEnterDfuOnStartup(bool upgrade_reboot, bool hasDynamicRole)
{
    bool *msg = (bool *)PanicUnlessMalloc(sizeof(bool));

    *msg = hasDynamicRole;
    appSmDfuDynamicRoleLockSet(hasDynamicRole);
    MessageSendConditionally(SmGetTask(),
                upgrade_reboot ? SM_INTERNAL_ENTER_DFU_UPGRADED
                               : SM_INTERNAL_ENTER_DFU_STARTUP,
                msg, &appSmDfuDynamicRoleLockGet());
}


/*! \brief Notification to the state machine of Upgrade start */
static void appSmNotifyUpgradeStarted(void)
{
    DEBUG_LOG_DEBUG("appSmNotifyUpgradeStarted");
    /* Set the device role and DFU mode when upgrade starts.
     */

    DEBUG_LOG_DEBUG("appSmNotifyUpgradePreStart - primary role:%d, secondary role:%d",TwsTopology_IsRolePrimary(),TwsTopology_IsRoleSecondary());

    if (TwsTopology_IsRolePrimary())
    {
        /*! Set the role for Primary and inform topology to remain active for
         * seamless DFU across physical state changes and abrupt reset. Marshal
         * it to Secondary device for same setting at secondary side */
        DfuPeer_SetRole(TRUE);

        earbud_sm_req_dfu_started_t *ind = PanicUnlessMalloc(sizeof(*ind));
        ind->id_in_progress = UpgradeInProgressId();
        appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                        PEER_SIG_MSG_CHANNEL_APPLICATION,
                                        ind, MARSHAL_TYPE(earbud_sm_req_dfu_started_t));
    }
    else if (TwsTopology_IsRoleSecondary())
    {
        /*! Set the role for Secondary and inform topology to remain active for
         * seamless DFU across physical state changes and abrupt reset. */
        DfuPeer_SetRole(FALSE);
    }

    appEnableKeepTopologyAliveForDfu();

    /*
     * This notification is an early indication of an upgrade to start.
     * In the pre phase, the earbud's may prepare the alternate DFU bank by
     * erasing.
     * To avoid psstore (for upgrade pskey) being blocked owing to ongoing erase,
     * this phase is separated. Since actual upgrade start is serialized to
     * erase completion, its required to cancel the DFU timers in order to avoid
     * false DFU timeout owing to erase.
     *
     * These timers are majorly significant for in-case DFU. In case of out-of-
     * case DFU, these are significant if DFU mode is explicity entered using
     * DFU mode button. But when out-of-case DFU started without explicit
     * DFU mode button, then these timers are insignificant.
     * But its safe to commonly cancel all the timers irrespective of in-case
     * or out-of-case DFU.
     *
     * Note: In case of in-case DFU, for Secondary, these are already
     * cancelled in appEnterInCaseDfu().
     *
     * ToDo: When DFU timers management is moved to DFU domain, this too
     * should be considered.
     */
    appSmCancelDfuTimers();

    if(!DfuCase_IsEarbudsInCaseRequested())
    {
        /* Once the DFU starts, all phy state transitions and device reset scenarios are 
         * supported for both in-case and out-case DFU. */
        TwsTopology_EnableRoleSwapSupport();
    }
}

/*! \brief Check and notify DFU domain if device is not in use. 
            For earbuds to be not in use, both earbuds should be in-case, connected with each other
            and silent commit should be enabled.
*/
static void appSmCheckAndNotifyDeviceNotInUse(void)
{
    /* Verify that peer signaling is connected other wise the state_proxy data could be out of sync.
     * Also, only primary device need to inform the DFU domain. */
    DEBUG_LOG("appSmCheckAndNotifyDeviceNotInUse Dfu_IsSilentCommitEnabled %d, BtDevice_IsMyAddressPrimary %d,"
                " appPeerSigIsConnected %d, StateProxy_IsPeerInCase %d, appPhyStateIsOutOfCase %d",
                Dfu_IsSilentCommitEnabled(), BtDevice_IsMyAddressPrimary(), appPeerSigIsConnected(),
                StateProxy_IsPeerInCase(), appPhyStateIsOutOfCase());
    if(Dfu_IsSilentCommitEnabled() && BtDevice_IsMyAddressPrimary() &&
       appPeerSigIsConnected() && StateProxy_IsPeerInCase() &&
       !appPhyStateIsOutOfCase())
    {
        Dfu_HandleDeviceNotInUse();
    }
}

static inline void appEnableKeepTopologyAliveForDfu(void)
{
    keep_topology_alive_for_dfu = TRUE;
}

static inline void appDisableKeepTopologyAliveForDfu(void)
{
    keep_topology_alive_for_dfu = FALSE;
}

#endif /* INCLUDE_DFU */

bool appIsKeepTopologyAliveForDfuEnabled(void)
{
#ifdef INCLUDE_DFU
    return keep_topology_alive_for_dfu;
#else
    return FALSE;
#endif
}

/*! \brief provides state manager(sm) task to other components

    \param[in]  void

    \return     Task - sm task.
*/
Task SmGetTask(void)
{
  return &app_sm.task;
}

/*! \brief Initialise the main application state machine.
 */
bool appSmInit(Task init_task)
{
    smTaskData* sm = SmGetTaskData();
    start_ps_free_count = 0;

    memset(sm, 0, sizeof(*sm));
    sm->task.handler = appSmHandleMessage;
    sm->state = APP_STATE_NULL;
    sm->phy_state = appPhyStateGetState();

    /* configure rule sets */
    sm->primary_rules = PrimaryRules_GetRuleSet();
    sm->secondary_rules = SecondaryRules_GetRuleSet();
    sm->role = tws_topology_role_none;

    TaskList_Initialise(SmGetClientTasks());

#ifdef INCLUDE_CASE_COMMS
    /* register with cc_with_case manager to get notifications of received case messages */
    CcWithCase_RegisterStateClient(&sm->task);
#endif
    /* register with connection manager to get notification of (dis)connections */
    ConManagerRegisterConnectionsClient(&sm->task);

    /* register with HFP for changes in state */
    HfpProfile_RegisterStatusClient(&sm->task);

    /* register with AV to receive notifications of A2DP and AVRCP activity */
    appAvStatusClientRegister(&sm->task);

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    /* register with LE unicast to receive notifications of unicast streaming and activity */
    LeAudioMessages_ClientRegister(&sm->task);
#endif

    /* register with peer signalling to connect/disconnect messages */
    appPeerSigClientRegister(&sm->task);

    /* register with power to receive sleep/shutdown messages. */
    appPowerClientRegister(&sm->task);

    /* register with handset service as we need disconnect and connect notification */
    HandsetService_ClientRegister(&sm->task);

    /* get remote phy state events */
    StateProxy_EventRegisterClient(&sm->task, appConfigStateProxyRegisteredEventsDefault());

    /* get the events concerning state_proxy itself */
    StateProxy_StateProxyEventRegisterClient(&sm->task);

    /* Register with peer signalling to use the State Proxy msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(SmGetTask(),
                                               PEER_SIG_MSG_CHANNEL_APPLICATION,
                                               earbud_sm_marshal_type_descriptors,
                                               NUMBER_OF_SM_MARSHAL_OBJECT_TYPES);

    /* Register to get pairing activity reports */
    Pairing_ActivityClientRegister(&sm->task);

    /* Register for role changed indications from TWS Topology */
    TwsTopology_RegisterMessageClient(SmGetTask());

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(&sm->task);

    /* Register for mirror profile messages */
    MirrorProfile_ClientRegister(&sm->task);

    /* Register for charger messages */
    (void) Charger_ClientRegister(&sm->task);
    
    /* Register for PFR role indications */
    PeerFindRole_RegisterTask(&sm->task);

    PeerFindRole_RegisterPrepareClient(&sm->task);
    
    appSmSetState(APP_STATE_INITIALISING);

    /* Register sm as ui provider*/
    Ui_RegisterUiProvider(ui_provider_phy_state, earbudSm_GetCurrentPhyStateContext);

    Ui_RegisterUiProvider(ui_provider_app_sm, earbudSm_GetCurrentApplicationContext);

#ifdef PRODUCTION_TEST_MODE
    Ui_RegisterUiProvider(ui_provider_ptm_state, earbudSm_GetCurrentPtmStateContext);
#endif

    Ui_RegisterUiInputConsumer(SmGetTask(), sm_ui_inputs, ARRAY_DIM(sm_ui_inputs));
#ifdef USE_SYNERGY
     CmSetEventMaskReqSend(&sm->task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
#endif
    /* register for bt device messages */
    BtDevice_RegisterListener(&sm->task);

#ifdef INCLUDE_CASE_COMMS
    /* register for case lid notifications */
    CcWithCase_RegisterStateClient(&sm->task);
#endif

    UNUSED(init_task);
    return TRUE;
}

void appSmDisconnectHandsets(void)
{
    DEBUG_LOG_DEBUG("DisconnectHandsets");
    HandsetService_DisconnectAll(SmGetTask(), HCI_ERROR_OETC_USER);
}

void appSmDisconnectLruHandset(void)
{
    DEBUG_LOG_DEBUG("appSmDisconnectLruHandset");
    HandsetService_DisconnectLruHandsetRequest(SmGetTask());
}

void appSmEnableMultipoint(void)
{
    DEBUG_LOG_DEBUG("appSmEnableMultipoint");

    if(!HandsetService_IsBrEdrMultipointEnabled())
    {
        /* Configure Handset Service */
        HandsetService_Configure(handset_service_multipoint_config);
    }

    /* Make earbud to be connectable(i.e. start PAGE scanning) only if one
       handset is already connected. Earbud is not discoverable so earbud
       cannot be seen by handset who wants to connect to.
       However, if Handset was connected to earbud in the past and entry of earbud
       exists in handset's PDL then handset can connect to earbud. */
    if(HandsetService_IsBrEdrMultipointEnabled() && (HandsetService_GetNumberOfConnectedBredrHandsets() == 1))
    {
        HandsetService_ConnectableRequest(NULL);
    }
}

void appSmDisableMultipoint(void)
{
    DEBUG_LOG_DEBUG("appSmDisableMultipoint");

    if (HandsetService_IsBrEdrMultipointEnabled())
    {
        HandsetService_Configure(handset_service_singlepoint_config);

        if (HandsetService_GetNumberOfConnectedBredrHandsets() > HandsetService_GetMaxNumberOfConnectedBredrHandsets())
        {
            /* If multipoint was disabled but 2 handsets are connected then
               one of them needs to be disconnected. */
            HandsetService_DisconnectLruHandsetRequest(SmGetTask());
        }
    }
}

/*! \brief Request a factory reset. */
void appSmFactoryReset(void)
{
    MessageSend(SmGetTask(), SM_INTERNAL_FACTORY_RESET, NULL);
}

/*! \brief Reboot the earbud. */
extern void appSmReboot(void)
{
    /* Post internal message to action the reboot, this breaks the call
     * chain and ensures the test API will return and not break. */
    MessageSend(SmGetTask(), SM_INTERNAL_REBOOT, NULL);
}

/*! \brief Determine if this Earbud is Primary.

    \todo this will move to topology.
*/
bool appSmIsPrimary(void)
{
    return SmGetTaskData()->role == tws_topology_role_primary;
}

bool appSmIsPairing(void)
{
    return (0 != (appGetSubState() & APP_SUBSTATE_HANDSET_PAIRING));
}

static void earbudSm_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == EARBUD_ROLE_MESSAGE_GROUP);
    TaskList_AddTask(SmGetClientTasks(), task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(EARBUD_ROLE, earbudSm_RegisterMessageGroup, NULL);
