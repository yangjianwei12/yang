/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\version    %%version
\file       headset_sm.c
\brief     headset application SM .

*/

/* local includes */
#include "headset_sm.h"
#include "headset_sm_private.h"
#include "headset_product_config.h"
#include "adk_log.h"
#include "headset_led.h"
#include "wired_audio_source.h"
#include "headset_test.h"
#include "headset_ui_config.h"
#include "headset_usb.h"
#include "headset_wired_audio_controller.h"
#include "headset_config.h"

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "headset_lea_src.h"
#endif

/* framework includes */
#include <ui.h>
#include <unexpected_message.h>
#include <connection_manager.h>
#include <connection_manager_config.h>
#include <hfp_profile.h>
#include <av.h>
#include <power_manager.h>
#include <power_manager_conditions.h>
#include <charger_monitor.h>
#include <stereo_topology.h>
#include <pairing.h>
#include <device.h>
#include <device_properties.h>
#include <dfu.h>
#include <gaia_framework.h>
#include <handset_service.h>
#include <device_list.h>
#include <system_state.h>
#include <hfp_profile_config.h>
#include <usb_device.h>
#include <gatt_server_gatt.h>
#include <anc_state_manager.h>
#include <aec_leakthrough.h>
#include <system_reboot.h>
#include <ps_key_map.h>
#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include <le_broadcast_manager.h>
#endif
#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
#include <le_audio_messages.h>
#endif
#ifdef INCLUDE_LE_AUDIO_UNICAST
#include <le_unicast_manager.h>
#endif

#ifdef INCLUDE_FAST_PAIR
#include <fast_pair.h>
#endif

#ifdef INCLUDE_ACCESSORY_TRACKING
#include "accessory_tracking.h"
#endif

#ifdef ENABLE_LE_AUDIO_CSIP
#include <csip_set_member.h>
#include "headset_sirk_config.h"
#endif

#ifdef ENABLE_TWM_SPEAKER
#include <peer_find_role.h>
#include <mirror_profile.h>
#include <logical_input_switch.h>
#include <state_proxy.h>
#include <ui_indicator_prompts.h>
#include <ui_indicator_tones.h>
#endif

/* system includes */
#include <vm.h>
#include <vmtypes.h>
#include <panic.h>
#include <message.h>
#include <ps.h>
#ifdef USE_SYNERGY
#include <csr_bt_td_db.h>
#endif

#include <telephony_messages.h>
#include <handset_service_connectable.h>
/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(sm_internal_message_ids)

/*! \name Connection library factory reset keys

    These keys should be deleted during a factory reset.
*/
/*! @{ */
#ifdef USE_SYNERGY
#define CSR_BT_PS_KEY_SYSTEM  100
#else
#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define TDL_BASE_PSKEY_INDEX        142
#define TDL_INDEX_PSKEY             141
/* Only BT devices, which have device_property_bdaddr is stored in PDL */
#define TDL_SIZE                    BtDevice_GetMaxTrustedDevices()
#endif
/*! @} */

/*!< headset state machine. */
smTaskData headset_sm;

const message_group_t sm_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
    UI_INPUTS_DEVICE_STATE_MESSAGE_GROUP,
    UI_INPUTS_APP_MESSAGE_GROUP
};

/*! \brief Get the state machine disconnect lock. */
#define headsetSmGetDisconnectLock() (SmGetTaskData()->disconnect_lock)
/*! \brief Clear the disconnect lock */
#define headsetSmClearDisconnectLock() (headsetSmGetDisconnectLock() = FALSE)
/*! \brief Set the disconnect lock */
#define headsetSmSetDisconnectLock() (headsetSmGetDisconnectLock() = TRUE)
/*! @brief Query if pairing has been initiated by the user. */
#define headsetSmIsUserPairing() (SmGetTaskData()->user_pairing)
/*! @brief Set user initiated pairing flag. */
#define headsetSmSetUserPairing()  (SmGetTaskData()->user_pairing = TRUE)
/*! @brief Clear user initiated pairing flag. */
#define headsetSmClearUserPairing()  (SmGetTaskData()->user_pairing = FALSE)
/*! @brief Query auto poweron */
#define headsetSmGetAutoPowerOn() (SmGetTaskData()->auto_poweron)
/*! @brief Set headset auto poweron flag. */
#define headsetSmSetAutoPowerOn()  (SmGetTaskData()->auto_poweron = TRUE)
/*! @brief Clear auto poweron flag. */
#define headsetSmClearAutoPowerOn()  (SmGetTaskData()->auto_poweron = FALSE)

/*! \brief Return TRUE if the state is idle */
#define headsetSmStateIsIdle(state) (state == HEADSET_STATE_IDLE)

/*! \brief Return TRUE if there is no handset connection */
#define IsBtConnectionIdle() (headsetSmStateIsIdle(headsetGetState()) && (!appDeviceIsHandsetConnected()) && \
                                                                          headsetConfigIdleTimeoutMs())
/*! \brief Return TRUE if the idle timer needs to start */
#define headsetSmIsIdleTimerNeedToStart()  (IsBtConnectionIdle() && (!IsWiredAudioConnected()) && \
                           (!AncStateManager_IsEnabled()) && (!AecLeakthrough_IsLeakthroughEnabled()))

/*! \brief Return TRUE if A2DP state is 'connected media streaming' and avrcp status is paused  */
#define IsA2dpStreamingAndAvrcpPaused() ( Av_IsA2dpSinkStreaming() && Av_IsPaused())

/*! \brief Return TRUE if the state is pairing*/
#define headsetSmStateIsPairing(state) (state == HEADSET_STATE_PAIRING)

/*! \brief Check if the headset is active.
*/
#define headsetSmIsActiveState(state) ((state == HEADSET_STATE_IDLE) || \
                                                      (state == HEADSET_STATE_BUSY))

/*! \brief Return TRUE if Auto power on panic is enabled and last reset happened due to panic */
#define headsetSmIsLastResetDueToPanic()  ( appConfigEnableAutoPowerOnAfterPanic() && isHeadsetResetSourcePanic() )

#ifdef INCLUDE_LE_AUDIO_UNICAST
#define IsLeaUnicastActive() (LeUnicastManager_IsStreamingActive())
#else
#define IsLeaUnicastActive() (FALSE)
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST
#define IsLeaBroadcastActive() (LeBroadcastManager_IsAnySourceSyncedToBis())
#else
#define IsLeaBroadcastActive() (FALSE)
#endif

/*! \brief Check if the Bt audio is active.
*/                                                      
#define IsBtAudioActive() (Av_IsA2dpSinkStreaming() || HfpProfile_IsScoActive() || IsLeaUnicastActive() || IsLeaBroadcastActive())

/*! \brief Check if the wired audio is connected.
*/
#define IsWiredAudioConnected() (WiredAudioSource_IsAudioAvailable(audio_source_line_in) || HeadsetUsb_IsAudioConnected())
/*! \brief Check if the wired audio is active.
*/
#ifdef ALLOW_WA_BT_COEXISTENCE
/* Wired audio is active if wired audio is available. */
#define IsWiredAudioActive() (AudioSources_IsAudioAvailable(audio_source_line_in) || AudioSources_IsAudioAvailable(audio_source_usb) || VoiceSources_IsVoiceChannelAvailable(voice_source_usb))
#else
/* Wired audio is active if wired audio is connected. */
#define IsWiredAudioActive() IsWiredAudioConnected()
#endif  /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Check if BT connection allowed with respect to wired audio.
*/
#ifdef ALLOW_WA_BT_COEXISTENCE
#define IsBtConnectionAllowed() (TRUE)
#else
#define IsBtConnectionAllowed() (!IsWiredAudioConnected())
#endif  /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Check if the headset audio is active.
*/
#define IsHeadsetAudioActive() (IsBtAudioActive() || (IsWiredAudioActive()))

/*! \brief Check if the routed audio source is A2DP.
*/
#define IsRoutedAudioSourceA2dp() ((AudioSources_GetRoutedSource() == audio_source_a2dp_1) || (AudioSources_GetRoutedSource() == audio_source_a2dp_2))

#ifdef ENABLE_TWM_SPEAKER
#define IsStereoStandaloneActive() (SmGetTaskData()->spk_type_is_standalone)
#define IsPartyModeActive()        (SmGetTaskData()->spk_party_mode)
#define IsTwmRoleSecondary()       (SmGetTaskData()->twm_role == stereo_find_role_secondary)
#define IsTwmActive()              !IsStereoStandaloneActive()

/*! \brief utility function to set the TWM role
*/
static void headsetSmSetTwmRole(stereo_topology_find_role_t role);
#endif

#ifdef INCLUDE_DFU
/*! \brief Check if the headset upgrade is active
*/
#define headsetSmIsDfuActive(reboot_reason) (reboot_reason == REBOOT_REASON_DFU_RESET)
/*! \brief Check if the upgrade was reverted post reboot
*/
#define headsetSmIsDfuRevertReset(reboot_reason) (reboot_reason == REBOOT_REASON_REVERT_RESET)
#endif /* INCLUDE_DFU */

static void headsetSmHandleAncUpdateStateEnableInd(void);
static void headsetSmHandleAncUpdateStateDisableInd(void);
static void headsetSetState(headsetState new_state);
static headsetState headsetGetState(void);
static void headsetSMStopIdleTimer(void);
static void headsetSMStartIdleTimer(void);

static headsetState headsetSmDetermineCoreState(void);

static void headsetSmHandleAncUpdateStateEnableInd(void)
{
    if(AncStateManager_IsEnabled())
    {
        headsetSMStopIdleTimer();
    }
}

static void headsetSmHandleAncUpdateStateDisableInd(void)
{
    if(headsetSmIsIdleTimerNeedToStart())
    {
        headsetSMStartIdleTimer();
    }
}

/*! \brief Function to start or stop headset idle timer for leakthrough*/
static void headsetSmHandleLeakthroughStateInd(LEAKTHROUGH_UPDATE_STATE_IND_T *msg)
{
    if(msg->state)
    {
        headsetSMStopIdleTimer();
    }
    else
    {
        if (headsetSmIsIdleTimerNeedToStart())
        {
            headsetSMStartIdleTimer();
        }
    }
}

/*! \brief function to start headset idle timer */
static void headsetSMStartIdleTimer(void)
{
    if(headsetConfigIdleTimeoutMs() && headsetSmIsIdleTimerNeedToStart())
    {
       MessageCancelAll(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
       MessageSendLater(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_IDLE, NULL, headsetConfigIdleTimeoutMs()); 
    }
}

/*! \brief function to stop headset idle timer */
static void headsetSMStopIdleTimer(void)
{
    MessageCancelAll(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
}

/*! \brief function to check if the last reset was caused by Panic */
static bool isHeadsetResetSourcePanic(void)
{
    vm_reset_source source = VmGetResetSource();

    switch (source)
    {
        case RESET_SOURCE_PANIC:
        case RESET_SOURCE_APP_PANIC:
        case RESET_SOURCE_SUBSYSTEM_PANIC:
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetSm_StopWiredAudio()
#else
static void headsetSm_StopWiredAudio(void)
{
    /* Stop Wired Audio PIO monitoring */
    WiredAudioSource_StopMonitoring(headsetSmGetTask());

    /* Disable USB Audio Feature */
    HeadsetUsb_AudioDisable(headsetSmGetTask());
}
#endif /* ALLOW_WA_BT_COEXISTENCE */

#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetSm_StartWiredAudio()
#else
static void headsetSm_StartWiredAudio(void)
{
    /* Stop Wired Audio PIO monitoring */
    WiredAudioSource_StartMonitoring(headsetSmGetTask());

    /* Disable USB Audio Feature */
    HeadsetUsb_AudioEnable(headsetSmGetTask());
}
#endif /* ALLOW_WA_BT_COEXISTENCE */


#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetPrepareForWiredAudio()
#else
/*! \brief Prepare for streaming wired audio */
static void headsetPrepareForWiredAudio(void)
{
    /* Disable wired audio controller */
    HeadsetWiredAudioController_Disable();
	
    /* Disable Headset topology */
    StereoTopology_Stop(headsetSmGetTask());
}
#endif /* ALLOW_WA_BT_COEXISTENCE */

#ifdef ALLOW_WA_BT_COEXISTENCE
#define headsetPrepareForBtAudio()
#else
/*! \brief Prepare for streaming Bluetooth audio */
static void headsetPrepareForBtAudio(void)
{
    /* Enable Headset topology */
    StereoTopology_Start(headsetSmGetTask());
}
#endif /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Initiate disconnect of all links */
static void headsetSmInitiateLinkDisconnection(uint16 timeout_ms)
{
    bool disconnecting_link = headsetSmDisconnectLink();

    DEBUG_LOG_VERBOSE("headsetSmInitiateLinkDisconnection");

    if (!disconnecting_link)
    {
        headsetSmClearDisconnectLock();
        DEBUG_LOG_VERBOSE("headsetSmInitiateLinkDisconnection: Lock cleared");
    }
    else
    {
        headsetSmSetDisconnectLock();
        DEBUG_LOG_VERBOSE("headsetSmInitiateLinkDisconnection: Lock set");
    }

    MessageSendConditionally(headsetSmGetTask(), SM_INTERNAL_LINK_DISCONNECTION_COMPLETE,
                             NULL, &headsetSmGetDisconnectLock());

    /* Start a timer to force reset if we fail to complete disconnection */
    MessageSendLater(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION, NULL, timeout_ms);
}

static void headsetSmClearPsStore(void)
{
    DEBUG_LOG_FN_ENTRY("headsetSmClearPsStore");

#ifdef USE_SYNERGY
    CsrBtTdDbDeleteAll(CSR_BT_TD_DB_FILTER_EXCLUDE_NONE);
    PsStore(CSR_BT_PS_KEY_SYSTEM, NULL, 0);
#else
    for (int i=0; i<TDL_SIZE; i++)
    {
        PsStore(ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(TDL_BASE_PSKEY_INDEX+i, NULL, 0);
    }

    PsStore(TDL_INDEX_PSKEY, NULL, 0);
#endif
    PsStore(PS_KEY_HFP_CONFIG, NULL, 0);

    /* Clear out any in progress DFU status */
#ifdef INCLUDE_DFU
    Dfu_ClearPsStore();
#endif
}

/*! \brief Delete handset pairing and reboot device. */
static void headsetSmDeletePairingAndReset(void)
{
    DEBUG_LOG_ALWAYS("headsetSmDeletePairingAndReset");

    /* cancel the link disconnection, may already be gone if it fired to get us here */
    MessageCancelFirst(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    headsetSmClearPsStore();

#ifdef INCLUDE_FAST_PAIR
    /* Delete the account keys */
    FastPair_DeleteAccountKeys();
#endif

    SystemReboot_Reboot();
}

/*! \brief Handle indication all requested links are now disconnected. */
static void headsetSmHandleInternalLinkDisconnectionComplete(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleInternalLinkDisconnectionComplete 0x%x", headsetGetState());
    MessageCancelFirst(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    /* Delete all devices for headset device type. */
    BtDevice_DeleteAllDevicesOfType(DEVICE_TYPE_HANDSET);

#ifdef INCLUDE_FAST_PAIR
    /* Delete the account keys */
    FastPair_DeleteAccountKeys();
#endif

}

/*! \brief Handle failure to successfully disconnect links within timeout.
*/
static void headsetSmLinkDisconnectionTimeout(void)
{
    DEBUG_LOG_DEBUG("headsetSmLinkDisconnectionTimeout 0x%x, Lock = %d", headsetGetState(), headsetSmGetDisconnectLock());

    if(headsetSmGetDisconnectLock())
    {
        MessageCancelAll(headsetSmGetTask(), SM_INTERNAL_LINK_DISCONNECTION_COMPLETE);
        headsetSmClearDisconnectLock();
    }
}

/*! \brief Start/Restart headset limbo timer, if its not zero */
static void headsetSmStartLimboTimer(void)
{
    if(headsetConfigLimboTimeoutMs())
    {
        DEBUG_LOG_DEBUG("headsetSmStartLimboTimer : Limbo Timer started LimboTimeoutMs %u",headsetConfigLimboTimeoutMs());
        MessageCancelAll(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LIMBO);
        MessageSendLater(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LIMBO, NULL, headsetConfigLimboTimeoutMs());
    }
}

/*! \brief Stop headset limbo timer. */
static void headsetSmStopLimboTimer(void)
{
    DEBUG_LOG_DEBUG("headsetSmStopLimboTimer : Limbo Timer stopped");
    MessageCancelAll(headsetSmGetTask(), SM_INTERNAL_TIMEOUT_LIMBO);
}

/*! \brief Take action following chargers indication of charger disconnect */
static void headsetSmHandleChargerMessageDetached(void)
{
    DEBUG_LOG_VERBOSE("headsetSmHandleChargerMessageDetached , state %d", headsetGetState());
    if(HEADSET_STATE_LIMBO ==  headsetGetState())
    {
        headsetSmStartLimboTimer();
    }
}

/*! \brief Take action following power's indication of imminent shutdown.
    Can be received in any state. */
static void headsetSmHandlePowerShutdownPrepareInd(void)
{
    DEBUG_LOG_VERBOSE("headsetSmHandlePowerShutdownPrepareInd, state %d", headsetGetState()); 
    switch (headsetGetState())
    {
        case HEADSET_STATE_LIMBO:
            DEBUG_LOG_VERBOSE("headsetSmHandlePowerShutdownPrepareInd, SilentCommitEnabled %d", Dfu_IsSilentCommitEnabled());
            /* Do not respond to shut down request if silent commit is pending
             * because DFU reboot will be in progress and that will get
             * interrupted.
             */
            if(!Dfu_IsSilentCommitEnabled())
            {
                appPowerShutdownPrepareResponse(headsetSmGetTask());
            }
            break;
        default : 
            headsetSetState(HEADSET_STATE_TERMINATING);
            break;
    }
}

/*! \brief Request a factory reset. */
static void headsetSmFactoryReset(void)
{
    MessageSend(headsetSmGetTask(), SM_INTERNAL_FACTORY_RESET, NULL);
}

/*! \brief Request handset pair. */
static void headsetSmPairHandset(void)
{
    MessageSend(headsetSmGetTask(), SM_INTERNAL_PAIR_HANDSET, NULL);
}

#ifdef ENABLE_TWM_SPEAKER
static void headsetSmPeerPair(void)
{
    /* we are assuming that user wants to switch to TWM mode */
    MessageSend(headsetSmGetTask(), SM_INTERNAL_PEER_PAIR, NULL);
}

static void headsetSmEnterTwmMode(uint8 timeout)
{
    MESSAGE_MAKE(msg, SM_INTERNAL_PEER_FIND_ROLE_T);
    msg->timeout = timeout;
    
    if(MessagePendingFirst(headsetSmGetTask(), SM_INTERNAL_ENTER_TWM_MODE, NULL))
    {
        DEBUG_LOG_DEBUG("headsetSmEnterTwmMode: already have TWM mode queued. Not allow debounce");
        return;
    }
    /* we are assuming that user wants to switch to TWM mode */
    MessageSend(headsetSmGetTask(), SM_INTERNAL_ENTER_TWM_MODE, msg);
}

static void headsetSmEnterStereoStandaloneMode(void)
{
    if(MessagePendingFirst(headsetSmGetTask(), SM_INTERNAL_ENTER_STANDALONE_MODE, NULL))
    {
        DEBUG_LOG_DEBUG("headsetSmEnterStereoStandaloneMode: already have Stereo Standalone mode queued. Not allow debounce");
        return;
    }
    MessageSend(headsetSmGetTask(), SM_INTERNAL_ENTER_STANDALONE_MODE, NULL);
}

static inline void headsetSmSetPartyMode(bool party_mode)
{
    SmGetTaskData()->spk_party_mode = party_mode;
}

static void headsetSmHandlePartyModeToggleInput(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandlePartyModeToggleInput: standalone:%d, current party_mode: %d", IsStereoStandaloneActive(), IsPartyModeActive());
    
    /* Cannot change sides for standalone */
    if(!IsStereoStandaloneActive())
    {
        if(IsPartyModeActive())
        {
            /* based on the role, set either left or right */
            if(IsTwmRoleSecondary())
            {
                Kymera_SetAudioType(KYMERA_AUDIO_MIRROR_MONO_RIGHT, TRUE);
            }
            else
            {
                Kymera_SetAudioType(KYMERA_AUDIO_MIRROR_MONO_LEFT, TRUE);
            }
            headsetSmSetPartyMode(FALSE);
        }
        else
        {
            Kymera_SetAudioType(KYMERA_AUDIO_MIRROR_STEREO, TRUE);
            headsetSmSetPartyMode(TRUE);
        }
    }
}
#endif /* ENABLE_TWM_SPEAKER */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
static void headsetSmHandleLeaBroadcastMediaSenderToggle(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleLeaBroadcastMediaSenderToggle: Is Audio Audio Active %d, Is Broacasting %d", IsHeadsetAudioActive(), SmGetTaskData()->spk_lea_broadcasting_media);
    if(SmGetTaskData()->spk_lea_broadcasting_media)
    {
        /* As Speaker is already broadcasting, we need to stop the broadcasting.
         * First we need to disconnect the ISO sink from the input audio chain, and then stop broadcasting */

        HeadsetLeaSrc_AudioStop();
        SmGetTaskData()->spk_lea_broadcasting_media = FALSE;
    }
    else
    {
        /* Need to start broadcasting, but need to first check if media is streaming, else just ignore the request */
        /* Need to start broadcasting, if routed audio source is A2dp, else ignore the request */
        if(IsHeadsetAudioActive() && IsRoutedAudioSourceA2dp())
        {
            HeadsetLeaSrc_AudioStart();
            SmGetTaskData()->spk_lea_broadcasting_media = TRUE;
        }
    }
}
#endif

/*! \brief Request handset connection. */
static void headsetSmConnectHandset(void)
{
    DEBUG_LOG_DEBUG("headsetSmConnectHandset: allowed %d", IsBtConnectionAllowed());
    if (IsBtConnectionAllowed())
    {
        headsetSmSetEventConnectMruHandset();
    }
}

static void headsetSmDisconnectLruHandset(void)
{
    DEBUG_LOG_DEBUG("headsetSmDisconnectLruHandset");
    headsetSmSetEventDisconnectLruHandset();
}

/*! \brief Enable BR/EDR multipoint */
static void headsetSmEnableMultipoint(void)
{
    DEBUG_LOG_DEBUG("headsetSmEnableMultipoint");

    if(!HandsetService_IsBrEdrMultipointEnabled())
    {
        /* Configure Handset Service */
        HandsetService_Configure(handset_service_multipoint_config);
    }

    /* Make headset to be connectable(i.e. start PAGE scanning) only if one
       handset is already connected. Headset is not discoverable so headset
       cannot be seen by handset who wants to connect to.
       However, if Handset was connected to headset in the past and entry of headset
       exists in handset's PDL then handset can connect to headset. */
    if(HandsetService_IsBrEdrMultipointEnabled() && (HandsetService_GetNumberOfConnectedBredrHandsets() == 1))
    {
        HandsetService_ConnectableRequest(NULL);
    }
}

/*! \brief Disable BR/EDR multipoint */
static void headsetSmDisableMultipoint(void)
{
    DEBUG_LOG_DEBUG("headsetSmDisableMultipoint");

    if (HandsetService_IsBrEdrMultipointEnabled())
    {
        HandsetService_Configure(handset_service_singlepoint_config);

        if (HandsetService_GetNumberOfConnectedBredrHandsets() > HandsetService_GetMaxNumberOfConnectedBredrHandsets())
        {
            /* If multipoint was disabled but 2 handsets are connected then
               one of them needs to be disconnected. */
            headsetSmSetEventDisconnectLruHandset();
        }
    }
}

/*! \brief Request handset delete. */
static void headsetSmDeleteHandsets(void)
{
    MessageSend(headsetSmGetTask(), SM_INTERNAL_DELETE_HANDSETS, NULL);
}

/*! \brief Request headset power off. */
static void headsetSmPowerOff(void)
{
    MessageSend(headsetSmGetTask(), SM_INTERNAL_POWER_OFF, NULL);
}

/*! \brief Handle power on confirmation
 */
static void headsetSmHandlePoweredOn(void)
{
    if(IsBtConnectionAllowed())
    {
        if (BtDevice_IsPairedWithHandset())
        {
            DEBUG_LOG_DEBUG("headsetSmHandlePoweredOn, already paired with handset");

            /* Move to idle state and initiate connection */
            headsetSetState(HEADSET_STATE_IDLE);
            return;
        }
        else
        {
            DEBUG_LOG_DEBUG("headsetSmHandlePoweredOn, no device in PDL, move to pairing");

            headsetSm_StopWiredAudio();

            headsetSmClearUserPairing();
            headsetSetState(HEADSET_STATE_PAIRING);
        }
    }
    else
    {
        /* Changing state to HEADSET_STATE_IDLE if wired audio source connected message is not received.
        This will handle the case of 'Wired audio source is removed before sending connected message' */
        if(headsetGetState() == HEADSET_STATE_POWERING_ON)
        {
            headsetSetState(HEADSET_STATE_IDLE);
        }
    }
}

#ifdef ENABLE_TWM_SPEAKER
static void headsetSmHandlePeerPairedCfm(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandlePeerPairedCfm");

    switch (headsetGetState())
    {
        case HEADSET_STATE_PEER_PAIRING:
            DEBUG_LOG_VERBOSE("PEER PAIRING COMPLETE");
            /* Done with peer-pairing, handset service can now start monitoring connections */
            handsetService_ObserveConnections();
            IsHeadsetAudioActive() ? headsetSetState(HEADSET_STATE_BUSY) : headsetSetState(HEADSET_STATE_IDLE);
            break;

        case HEADSET_STATE_FACTORY_RESET:
            /* Nothing to do, even if pairing with handset succeeded, the final
            act of factory reset is to delete handset pairing */
            break;

        default:
            /* Ignore, paired with handset with known address as requested by peer */
            break;
    }
}
#endif

/*! \brief Idle timeout */
static void headsetSmHandleTimeoutIdle(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleTimeoutIdle, state %d, handset connected %d wired connection %d",
                    headsetGetState(), appDeviceIsHandsetConnected(), IsWiredAudioConnected());
    if((HEADSET_STATE_IDLE == headsetGetState()) && (!appDeviceIsHandsetConnected()) && (!IsWiredAudioConnected()))
    {
        headsetSmPowerOff();
    }
}

/*! \brief Limbo timeout */
static void headsetSmHandleTimeoutLimbo(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleTimeoutLimbo, state %d", headsetGetState());
    if((HEADSET_STATE_LIMBO == headsetGetState()))
    {
        appPowerOffRequest();
    }
}

#ifdef INCLUDE_DFU
/*! \brief Check if DFU was active.
 */
static void headsetCheckDfu(void)
{
    dfu_reboot_reason_t reboot_reason = Dfu_GetRebootReason();
    DEBUG_LOG_DEBUG("headsetCheckDfu: Reboot Reason enum:dfu_reboot_reason_t:%d", reboot_reason);
    /* Limbo to power-on transition should happen only for interactive commit
     * or when the upgrade is aborted on the final commit screen.
     * Silent commit happens when user puts HS in limbo state so, the same
     * state should be retained even after reboot. */
    if((headsetSmIsDfuActive(reboot_reason) || headsetSmIsDfuRevertReset(reboot_reason)) && !Dfu_IsSilentCommitEnabled())
    {
        if(headsetSmIsDfuActive(reboot_reason))
        {
            /*
             * Safe restart/stretch of DFU reconnection timer as limbo to power-on
             * transition is delayed.
             */
            UpgradeRestartReconnectionTimer();
        }
        Ui_InjectBackEndUiInput(ui_input_sm_power_on,
                                headsetConfigDfuCommitDelayForLimboToPowerOn());
        if(headsetSmIsDfuRevertReset(reboot_reason))
        {
            Dfu_SetRebootReason(REBOOT_REASON_NONE);
        }
    }
}
#endif

/*****************************************************************************
 * SM state transition handler functions
 *****************************************************************************/

/*! \brief Enter initialising state

    This function is called whenever the state changes to
    HEADSET_STATE_LIMBO.
    It is reponsible for initialising global aspects of the application,
    i.e. non application module related state.
*/
static void headsetEnterLimbo(void)
{
    DEBUG_LOG_DEBUG("headsetEnterLimbo : HEADSET_STATE_LIMBO");
    headsetSmStartLimboTimer();
#ifdef INCLUDE_DFU
    headsetCheckDfu();
    
    /* check and inform the DFU domain if the headset is in limbo state (not-in-use state) 
     * and silent commit is enabled */
    if(Dfu_IsSilentCommitEnabled())
    {
        Dfu_HandleDeviceNotInUse();
    }
#endif
    if(headsetSmGetAutoPowerOn())
    {
        DEBUG_LOG_DEBUG("headsetEnterLimbo : Auto PowerOn");
        headsetSmClearAutoPowerOn();
        Ui_InjectUiInput(ui_input_sm_power_on);
    }
}

/*! \brief Exit limbo state.
 */
static void headsetExitLimbo(void)
{
    DEBUG_LOG_DEBUG("headsetExitLimbo");
    headsetSmStopLimboTimer();
}

/*! \brief Enter powering on state.
 */
static void headsetEnterPoweringOn(void)
{
    DEBUG_LOG_ALWAYS("headsetEnterPoweringOn : HEADSET_STATE_POWERING_ON");
}

/*! \brief Exit powering on state.
 */
static void headsetExitPoweringOn(void)
{
    DEBUG_LOG_DEBUG("headsetExitPoweringOn");
}

/*! \brief Enter powering off state.
 */
static void headsetEnterPoweringOff(void)
{
    DEBUG_LOG_ALWAYS("headsetEnterPoweringOff : HEADSET_STATE_POWERING_OFF");
}

/*! \brief Exit powering off state.
 */
static void headsetExitPoweringOff(void)
{
    DEBUG_LOG_DEBUG("headsetExitPoweringOff");
}

/*! \brief Enter actions when we enter the factory reset state.
 */
static void headsetEnterFactoryReset(void)
{
    DEBUG_LOG_DEBUG("headsetEnterFactoryReset : HEADSET_STATE_FACTORY_RESET");

#ifdef INCLUDE_ACCESSORY_TRACKING
    AccessoryTrackingFactoryReset();
#endif
    headsetSmDeletePairingAndReset();
}

/*! \brief Exit factory reset. */
static void headsetExitFactoryReset(void)
{
    /* Should never happen */
    Panic();
}

/*! \brief Enter Idle state.
 */
static void headsetEnterIdle(void)
{
    DEBUG_LOG_DEBUG("headsetEnterIdle : HEADSET_STATE_IDLE");

    if(headsetSmIsIdleTimerNeedToStart())
    {
        headsetSMStartIdleTimer();
    }

    Ui_InformContextChange(ui_provider_app_sm, context_app_sm_idle);
}

/*! \brief Exit Idle on state.
 */
static void headsetExitIdle(void)
{
    DEBUG_LOG_DEBUG("headsetExitIdle");

    headsetSMStopIdleTimer();

    Ui_InformContextChange(ui_provider_app_sm, context_app_sm_exit_idle);
}

/*! \brief Enter Busy state.
 */
static void headsetEnterBusy(void)
{
   DEBUG_LOG_DEBUG("headsetEnterBusy : HEADSET_STATE_BUSY");
   if(!IsBtConnectionAllowed())
   {   
      headsetPrepareForWiredAudio();
   }
}

/*! \brief Exit Busy state.
 */
static void headsetExitBusy(void)
{
    DEBUG_LOG_DEBUG("headsetExitBusy");
    if(IsBtConnectionAllowed())
    {
       headsetPrepareForBtAudio();
    }
}

/*! \brief Enter Pairing state.
 */
static void headsetEnterPairing(void)
{
    DEBUG_LOG_DEBUG("headsetEnterPairing : HEADSET_STATE_PAIRING");
    HandsetService_PairHandset(headsetSmGetTask(), headsetSmIsUserPairing());
    HandsetService_ConnectableRequest(headsetSmGetTask());
}

#ifdef ENABLE_TWM_SPEAKER
static void headsetEnterPeerPairing(void)
{
    DEBUG_LOG_DEBUG("headsetEnterPeerPairing : HEADSET_STATE_PEER_PAIRING");
    /* Ideally peer connection need not go through handset service, so handset service 
       should ignore connections */
    handsetService_DontObserveConnections();
    StereoTopology_StartPeerPair(headsetSmGetTask(), appConfigSpeakerPeerPairTimeout());
}
#endif


/*! \brief Exit Pairing on state.
 */
static void headsetExitPairing(void)
{
    DEBUG_LOG_DEBUG("headsetExitPairing");
    HandsetService_CancelPairHandset(NULL);
    headsetSmClearUserPairing();
}

/*! \brief Enter Terminating state.
 */
static void headsetEnterTerminating(void)
{
    DEBUG_LOG_DEBUG("headsetEnterTerminating : HEADSET_STATE_TERMINATING");
    bdaddr addr;
    if (appDeviceGetHandsetBdAddr(&addr) && HandsetService_IsBredrConnected(&addr))
    {
        ConManagerSendCloseAclRequest(&addr, TRUE);
    }
    WiredAudioSource_StopMonitoring(headsetSmGetTask());
    appPowerShutdownPrepareResponse(headsetSmGetTask());
}

/*! \brief Exit Terminating state.
 */
static void headsetExitTerminating(void)
{
    DEBUG_LOG_DEBUG("headsetExitTerminating");
}

/*! \brief Provides Headset Application state machine context changes to the User Interface module.

    \param[in]  void

    \return     current_sm_ctxt - current application context of sm module.
*/
static unsigned headsetSm_GetApplicationCurrentContext(void)
{
    sm_provider_context_t context = context_app_sm_powered_on;

     switch(headsetGetState())
     {
        case HEADSET_STATE_NULL : /* fall through */
        case HEADSET_STATE_LIMBO: /* fall through */
        case HEADSET_STATE_POWERING_ON: /* fall through */
        case HEADSET_STATE_POWERING_OFF: /* fall through */
        case HEADSET_STATE_TERMINATING: /* fall through */
        case HEADSET_STATE_FACTORY_RESET: context = context_app_sm_powered_off;
            break;

        case HEADSET_STATE_BUSY: /* fall through */
        case HEADSET_STATE_IDLE:
            /* It could so happen if USB co-exists with BT that, USB could be streaming while BT is just connected */
            if(IsWiredAudioActive())
                context = context_app_sm_active;
            else
                context = appDeviceIsHandsetConnected() == TRUE ? context_app_sm_idle_connected : context_app_sm_idle;
            break;

        default : break;
     }

    return (unsigned)context;
}


/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
static void headsetSetState(headsetState new_state)
{
    smTaskData* sm = SmGetTaskData();
    headsetState previous_state = SmGetTaskData()->state;

    DEBUG_LOG_STATE("headsetSetState, state 0x%02x to 0x%02x", previous_state, new_state);

    /* Handle state exit functions */
    switch (previous_state)
    {
        case HEADSET_STATE_NULL:
            /* This can occur when DFU is entered during INIT. */
            break;

        case HEADSET_STATE_LIMBO:
            headsetExitLimbo();
            break;

        case HEADSET_STATE_POWERING_ON:
            headsetExitPoweringOn();
            break;

        case HEADSET_STATE_POWERING_OFF:
            headsetExitPoweringOff();
            break;

        case HEADSET_STATE_FACTORY_RESET:
            headsetExitFactoryReset();
            break;

        case HEADSET_STATE_PAIRING:
            headsetExitPairing();
            break;

        case HEADSET_STATE_IDLE:
            headsetExitIdle();
            break;

        case HEADSET_STATE_BUSY:
            headsetExitBusy();
            break;

        case HEADSET_STATE_TERMINATING:
            headsetExitTerminating();
            break;          
#ifdef ENABLE_TWM_SPEAKER
        case HEADSET_STATE_PEER_PAIRING:
            break;
#endif
        default:
            DEBUG_LOG_ERROR("Attempted to exit unsupported state 0x%02x", SmGetTaskData()->state);
            Panic();
            break;
    }
    /* Set new state */
    SmGetTaskData()->state = new_state;
    /* Handle state entry functions */
    switch (new_state)
    {
        case HEADSET_STATE_LIMBO:
            headsetEnterLimbo();
            break;

        case HEADSET_STATE_FACTORY_RESET:
            headsetEnterFactoryReset();
            break;

        case HEADSET_STATE_POWERING_ON:
            headsetEnterPoweringOn();
            break;

        case HEADSET_STATE_PAIRING:
            headsetEnterPairing();
            break;

        case HEADSET_STATE_IDLE:
            headsetEnterIdle();
            break;

        case HEADSET_STATE_POWERING_OFF:
            headsetEnterPoweringOff();
            break;

        case HEADSET_STATE_BUSY:
            headsetEnterBusy();
            break;
            
        case HEADSET_STATE_TERMINATING:
            headsetEnterTerminating();
            break;
#ifdef ENABLE_TWM_SPEAKER
        case HEADSET_STATE_PEER_PAIRING:
            headsetEnterPeerPairing();
            break;
#endif
        default:
            DEBUG_LOG_ERROR("Attempted to enter unsupported state 0x%02x", new_state);
            Panic();
            break;
    }

    Ui_InformContextChange(ui_provider_app_sm, headsetSm_GetApplicationCurrentContext());
    DEBUG_LOG_VERBOSE("headsetSetState, new state 0x%02x", sm->state);
}

static headsetState headsetGetState(void)
{
    return SmGetTaskData()->state;
}

/*! \brief Handle request to start factory reset. */
static void headsetSmHandleInternalFactoryReset(void)
{
    if (headsetGetState() > HEADSET_STATE_POWERING_ON)
    {
        DEBUG_LOG_DEBUG("headsetSmHandleInternalFactoryReset");
        headsetSetState(HEADSET_STATE_FACTORY_RESET);
    }
    else
        DEBUG_LOG_WARN("headsetSmHandleInternalFactoryReset cannot be done in state %d", headsetGetState());
}


/*! \brief Handle request to start handset pair. */
static void headsetSmHandleInternalPairHandset(void)
{
    /* Pairing is allowed if headset is either in IDLE or BUSY state and BT connection is allowed */
    if(IsBtConnectionAllowed() && headsetSmIsActiveState(headsetGetState()))
    {
        headsetSm_StopWiredAudio();

        DEBUG_LOG_DEBUG("headsetSmHandleInternalPairHandset USER PAIRING REQUEST");
        headsetSmSetUserPairing();
        headsetSetState(HEADSET_STATE_PAIRING);
    }
    else
    {
        DEBUG_LOG_WARN("headsetSmHandleInternalPairHandset: Pairing is not allowed");
    }
}

#ifdef ENABLE_TWM_SPEAKER
static void headsetSmSetStandaloneMode(bool is_standalone)
{
    if(SmGetTaskData()->spk_type_is_standalone != is_standalone)
    {
        DEBUG_LOG_DEBUG("Speaker is now standalone: %d", is_standalone);
        SmGetTaskData()->spk_type_is_standalone = is_standalone;
    }
}

static void headsetSmHandleInternalPeerPair(void)
{
    /* Pairing is allowed if headset is either in IDLE or BUSY state and BT connection is allowed */
    if(IsBtConnectionAllowed() && headsetSmIsActiveState(headsetGetState()))
    {
        DEBUG_LOG_DEBUG("headsetSmHandleInternalPeerPair USER PEER PAIRING REQUEST");
        
        if(appConfigSpeakerRemovePeerInfoOnTrigger())
        {
            bdaddr bd_addr;
            /* Get the previous peer address */
            if (appDeviceGetPeerBdAddr(&bd_addr))
            {
                DEBUG_LOG_DEBUG("Old Peer info shall be deleted");
                PanicFalse(appDeviceDelete(&bd_addr));
            }
            /* reset the current role, as we might end up being any role */
            headsetSmSetTwmRole(stereo_find_role_no_peer);
        }
        headsetSetState(HEADSET_STATE_PEER_PAIRING);
    }
    else
    {
        DEBUG_LOG_WARN("headsetSmHandleInternalPairHandset: Pairing is not allowed");
    }
}

static void headsetSmHandleInternalEnterTwmMode(const SM_INTERNAL_PEER_FIND_ROLE_T* msg)
{
    /* Pairing is allowed if headset is either in IDLE or BUSY state and BT connection is allowed */
    if(IsBtConnectionAllowed() && headsetSmIsActiveState(headsetGetState()))
    {

        DEBUG_LOG_DEBUG("headsetSmHandleInternalPeerFindRole with timeout: %d", msg->timeout);
        StereoTopology_StartPeerFindRole(headsetSmGetTask(), msg->timeout);
    }
    else
    {
        DEBUG_LOG_WARN("headsetSmHandleInternalPeerFindRole: PFR not allowed");
    }
}

static void headsetSmHandleInternalEnterStereoStandaloneMode(void)
{
    if(headsetSmIsActiveState(headsetGetState()))
    {
        DEBUG_LOG_DEBUG("headsetSmEnableStereoStandalone");
        StereoTopology_EnableStereoStandalone();
    }
    else
    {
        DEBUG_LOG_WARN("headsetSmEnableStereoStandalone: not allowed");
    }
}

#endif


/*! \brief Delete pairing for all handsets.
    \note There must be no connections to a handset for this to succeed. */
static void headsetSmHandleInternalDeleteHandsets(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleInternalDeleteHandsets");

    switch (headsetGetState())
    {
        case HEADSET_STATE_IDLE:
        case HEADSET_STATE_BUSY:
            /* Stop being connectable when deleting handset pairing */
            HandsetService_CancelConnectableRequest(headsetSmGetTask());
            headsetSmInitiateLinkDisconnection(headsetConfigDisconnectTimeoutMs());
            break;

        default:
            DEBUG_LOG_WARN("headsetSmHandleInternalDeleteHandsets bad state %u",
                                                        headsetGetState());
            break;
    }
}

/*! \brief Handle request to power off headset. */
static void headsetSmHandleInternalPowerOff(void)
{
    if (headsetGetState() > HEADSET_STATE_LIMBO)
    {
        SystemState_PowerOff();
    }
}

/*! \brief Handle request to power on headset. */
static void headsetSmHandlePowerOn(void)
{
#if defined(ENABLE_SIMPLE_SPEAKER) && defined(ENABLE_LE_AUDIO_CSIP)
    /* Allow PowerOn only if peer is paried */
    if(SmGetTaskData()->peer_pairing)
    {
        DEBUG_LOG_WARN("headsetSmHandlePowerOn - can't process as peer pair is in progress");
        return;
    }
#endif

    if (headsetGetState() == HEADSET_STATE_LIMBO)
    {
        SystemState_PowerOn();
    }
}

/*! \brief handles sm module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void headsetSmHandleUiInput(MessageId ui_input)
{
    switch (ui_input)
    {
        case ui_input_sm_power_on:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_sm_power_on");
            headsetSmHandlePowerOn();
            break;

        case ui_input_sm_power_off:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_sm_power_off");
            headsetSmPowerOff();
            break;

        case ui_input_connect_handset:
             DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_connect_handset");
             headsetSmConnectHandset();
            break;

        case ui_input_disconnect_lru_handset:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_disconnect_lru_handset");
            headsetSmDisconnectLruHandset();
            break;

        case ui_input_sm_pair_handset:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_sm_pair_handset");
            headsetSmPairHandset();
            break;

        case ui_input_sm_delete_handsets:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_sm_delete_handset");
            headsetSmDeleteHandsets();
            break;

        case ui_input_enable_multipoint:
            headsetSmEnableMultipoint();
            break;

        case ui_input_disable_multipoint:
            headsetSmDisableMultipoint();
            break;

        case ui_input_factory_reset_request:
            headsetSmFactoryReset();
            break;
#ifdef ENABLE_TWM_SPEAKER
        case ui_input_app_peer_pair:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_app_peer_pair");
            headsetSmPeerPair();
            break;

        case ui_input_app_toggle_twm_standalone:
            DEBUG_LOG_VERBOSE("headsetSmHandleUiInput received ui_input_app_toggle_twm_standalone, standalone mode is %d", IsStereoStandaloneActive());
            if(IsStereoStandaloneActive())
            {
                headsetSmEnterTwmMode(appConfigSpeakerPeerFindRoleTimeout());
            }
            else
            {
                headsetSmEnterStereoStandaloneMode();
            }
            break;

        case ui_input_app_toggle_party_mode:
            headsetSmHandlePartyModeToggleInput();
            break;
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        case ui_input_app_toggle_broadcast_media_sender:
            headsetSmHandleLeaBroadcastMediaSenderToggle();
            break;
#endif

        default:
            break;
    }
}

/*! \brief Handle completion of application module initialisation. */
static void headsetSmHandleSystemSartedUpToLimbo(void)
{
    DEBUG_LOG_INFO("headsetSmHandleSystemSartedUpToLimbo");

    switch (headsetGetState())
    {
        case HEADSET_STATE_NULL:
            {
                headsetSetState(HEADSET_STATE_LIMBO);
#if defined(ENABLE_SIMPLE_SPEAKER) && defined(ENABLE_LE_AUDIO_CSIP)
                /* Lets try to find peer pair re-using limbo timer to find the peer */
                StereoTopology_StartPeerPair(headsetSmGetTask());
                SmGetTaskData()->peer_pairing = TRUE;
#endif

#ifdef ADVERTISE_WHEN_POWERED_OFF
                LeAdvertisingManager_AllowAdvertising(NULL, TRUE);
#endif
            }
        break;

        default:
            Panic();
    }
}

static void headsetSmHandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG_DEBUG("headsetSmHandleSystemStateChange old state 0x%x, new state 0x%x", msg->old_state, msg->new_state);

    if(msg->old_state == system_state_starting_up && msg->new_state == system_state_limbo)
    {
        headsetSmHandleSystemSartedUpToLimbo();
    }
    else if(msg->old_state == system_state_limbo && msg->new_state == system_state_powering_on)
    {
        headsetSetState(HEADSET_STATE_POWERING_ON);
    }
    else if(msg->old_state == system_state_powering_on && msg->new_state == system_state_active)
    {
#ifdef ENABLE_TWM_SPEAKER
        if(PeerFindRole_IsActive())
        {
            DEBUG_LOG_DEBUG("headsetSmHandlePoweredOn, PRF in progress, wait till it completes to trigger topology start");
            return;
        }
#endif
        headsetSmHandlePoweredOn();
    }
    else if(msg->old_state == system_state_active && msg->new_state == system_state_powering_off)
    {
        headsetSetState(HEADSET_STATE_POWERING_OFF);
    }
}

Task headsetSmGetTask(void)
{
  return &headset_sm.task;
}

bool headsetSmIsAllowedToRunHeadsetRules(void)
{
    return SmGetTaskData()->allow_rules_to_run;
}

/*! \brief Handle completion of handset pairing. */
static void headsetSmHandlePairHandsetConfirm(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandlePairingPairConfirm");

    switch (headsetGetState())
    {
        case HEADSET_STATE_PAIRING:
            DEBUG_LOG_VERBOSE("PAIRING COMPLETE");

            headsetSm_StartWiredAudio();

            IsHeadsetAudioActive() ? headsetSetState(HEADSET_STATE_BUSY) : headsetSetState(HEADSET_STATE_IDLE);
            break;

        case HEADSET_STATE_FACTORY_RESET:
            /* Nothing to do, even if pairing with handset succeeded, the final
            act of factory reset is to delete handset pairing */
            break;

        default:
            /* Ignore, paired with handset with known address as requested by peer */
            break;
    }
}

/*! \brief Print bluetooth address of the handset. */
static void headsetSmPrintBdaddr(const bdaddr addr)
{
    DEBUG_LOG("headsetSmPrintBdaddr %04x,%02x,%06lx", addr.nap,
                                                     addr.uap,
                                                     addr.lap);
}

/*! \brief Handle notification of Handset Service connection. */
static void headsetSmHandleHandsetServiceConnectedInd(HANDSET_SERVICE_CONNECTED_IND_T* ind)
{
    UNUSED(ind);
    /* We need not be in IDLE state, as USB could be streaming while BT connects */
    DEBUG_LOG("headsetSmHandleHandsetServiceConnectedInd: HANDSET_SERVICE_CONNECTED_IND profiles_connected = %d",
                  ind->profiles_connected);
    headsetSmPrintBdaddr(ind->addr);
    if(headsetSmStateIsIdle(headsetGetState()))
    {
        /* Inform UI that a handset has been connected */
        Ui_InformContextChange(ui_provider_app_sm, context_app_sm_idle_connected);
        headsetSMStopIdleTimer();
    }
}

/*! \brief Auto connect to handset once the stereo topology state has transitioned to started state */
static void headsetSmSetEventAutoConnect(void)
{
    DEBUG_LOG_DEBUG("headsetSmSetEventAutoConnect");
    HeadsetRules_SetEvent(HS_EVENT_AUTO_CON_HANDSET);
}

/*! \brief Handle stereo topology stopping state. */
static void headsetSmHandleStereoTopologyStateStopping(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyStateStopping STEREO_TOPOLOGY_STOPPING_CFM");
    headsetSmSetAllowHeadsetRulesToRun(FALSE);
}

/*! \brief Handle stereo topology started state. */
static void headsetSmHandleStereoTopologyStateStarted(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyStateStarted STEREO_TOPOLOGY_STARTED_CFM");

    headsetSmSetAllowHeadsetRulesToRun(TRUE);

    /* Auto connect to handset once the stereo topology state has transitioned to started state */
    headsetSmSetEventAutoConnect();
}

/*! \brief Handle stereo topology starting state. */
static void headsetSmHandleStereoTopologyStateStarting(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyStateStarting STEREO_TOPOLOGY_STARTING_CFM");
}

#ifdef ENABLE_LE_AUDIO_CSIP
/*! \brief Handle stereo topology starting state. */
static void headsetSmHandleStereoTopologySirkUpdateCfm(STEREO_TOPOLOGY_SIRK_UPDATE_CFM_T *msg)
{
    uint8 sirk[SIZE_SIRK_KEY] = {0};
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyStateStarting STEREO_TOPOLOGY_SIRK_UPDATE_CFM");
    HeadsetSirk_GenerateAndStoreSirkKey(sirk, msg->key_a, msg->key_b);
    CsipSetMember_SetSirkKey(sirk);
}
#endif

#ifdef ENABLE_TWM_SPEAKER
static void headsetSmPrepareForPrimaryRole(bool is_primary)
{
    if(is_primary)
    {
        DEBUG_LOG_DEBUG("headsetSmPrepareForPrimaryRole prepare for primary");
        Av_SetupForPrimaryRole();
        HfpProfile_SetRole(TRUE);
        MirrorProfile_SetRole(TRUE);
        LogicalInputSwitch_SetRerouteToPeer(FALSE);
        StateProxy_SetRole(TRUE);
        UiPrompts_GenerateUiEvents(TRUE);
        UiTones_GenerateUiEvents(TRUE);
    }
    else
    {
        DEBUG_LOG_DEBUG("headsetSmPrepareForPrimaryRole prepare for secondary");
        Av_SetupForSecondaryRole();
        HfpProfile_SetRole(FALSE);
        MirrorProfile_SetRole(FALSE);
        LogicalInputSwitch_SetRerouteToPeer(TRUE);
        StateProxy_SetRole(FALSE);
        UiPrompts_GenerateUiEvents(FALSE);
        UiTones_GenerateUiEvents(FALSE);
    }
}

static inline void headsetSmResetSecondaryRole(void)
{
    /* If exiting from secondary role, need to update all the module 
       as though its primary, so that it can connect to AGs and act upon individually */
    headsetSmPrepareForPrimaryRole(TRUE);
}

static void headsetSmSetTwmRole(stereo_topology_find_role_t role)
{
    if(SmGetTaskData()->twm_role != role)
    {
        SmGetTaskData()->twm_role = role;
    }
}

static void headsetSmHandleStereoTopologyFindRoleCfm(const STEREO_TOPOLOGY_FIND_ROLE_CFM_T* selected_role)
{
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyFindRoleCfm: role is current role enum:stereo_topology_find_role_t:%d elected role enum:stereo_topology_find_role_t:%d", SmGetTaskData()->twm_role, selected_role->role);
    switch(selected_role->role)
    {
        case stereo_find_role_no_peer: /* No peer paired, so we ignore and continue standalone, fall thru */
            break;
        case stereo_find_role_acting_primary:
            /* Didn't find peer, PFR will fallback to low duty cycle and keep finding. Act as primary */
            headsetSmSetStandaloneMode(FALSE);
            /* since we timed out PFR, we shall get the role information later */
            PeerFindRole_RegisterTask(headsetSmGetTask());
            break;
        case stereo_find_role_primary:
            /* primary should allow first secondary to connect, then it will trigger connecting its profiles*/
            headsetSmPrepareForPrimaryRole(TRUE);
            Kymera_SetAudioType(KYMERA_AUDIO_MIRROR_MONO_LEFT, FALSE); /* lets fix primary to play left */
            StereoTopology_StartPeerProfileConn(headsetSmGetTask());
            headsetSmSetStandaloneMode(FALSE);
            PeerFindRole_UnregisterTask(headsetSmGetTask());
            break;
        case stereo_find_role_secondary:
            headsetSmPrepareForPrimaryRole(FALSE);
            Kymera_SetAudioType(KYMERA_AUDIO_MIRROR_MONO_RIGHT, FALSE); /* lets fix secondary to play right */
            StereoTopology_Stop(headsetSmGetTask());
            /* trigger become secondary goal to just connect to primary */
            StereoTopology_StartPeerAclConn();
            headsetSmSetStandaloneMode(FALSE);
            break;
        default:
            /* should not be here */
            Panic();
    }
    headsetSmSetTwmRole(selected_role->role);
}

static void headsetSmHandleStandaloneCompleteCfm(const STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM_T* cfm)
{
    UNUSED(cfm);
    bool start_top = FALSE;
    DEBUG_LOG_DEBUG("headsetSmHandleStandaloneCompleteCfm is complete");
    headsetSmSetStandaloneMode(TRUE);
    Kymera_SetAudioType(KYMERA_AUDIO_STANDALONE_STEREO, FALSE);
    if(IsTwmRoleSecondary())
    {
        /* Need to reset what was set when role was selected as secondary in TWM mode */
        headsetSmResetSecondaryRole();
        start_top = TRUE;
    }
    headsetSmSetTwmRole(stereo_find_role_no_peer);
    if(start_top)
    {
        StereoTopology_Start(headsetSmGetTask());
    }
}

static void headsetSmHandleStereoTopologyPeerProfileConnCfm(const STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM_T* cfm)
{
    DEBUG_LOG_DEBUG("headsetSmHandleStereoTopologyPeerProfileConnCfm: enum:stereo_topology_status_t:%d", cfm->status);
}
#endif /*ENABLE_TWM_SPEAKER*/

static void headsetSmUpdateDisconnectingLink(void)
{
    /* Update the disconnecting handset link lock status */
    if(headsetSmGetDisconnectLock())
    {
        DEBUG_LOG_DEBUG("headsetSmUpdateDisconnectingLink disconnecting handset");
        if (!HandsetService_IsAnyBredrConnected())
        {
            DEBUG_LOG_DEBUG("headsetSmUpdateDisconnectingLink handset disconnected");
            headsetSmClearDisconnectLock();
            DEBUG_LOG_VERBOSE("headsetSmUpdateDisconnectingLink: Lock cleared");
        }
        else
        {
            DEBUG_LOG_DEBUG("headsetSmUpdateDisconnectingLink: Still connected");
        }
    }
}

/*! \brief Generate handset related disconnection event. */
static void headsetSmSetEventDisconnectionByLinkLoss(void)
{
    HeadsetRules_SetEvent(HS_EVENT_LINK_LOSS);
}

/*! \brief Handle notification of handset disconnection. */
static void headsetSmHandleHandsetServiceDisconnectedInd(HANDSET_SERVICE_DISCONNECTED_IND_T *ind)
{  
    DEBUG_LOG_DEBUG("headsetSmHandleHandsetServiceDisconnectedInd %04x,%02x,%06lx status %u", ind->addr.nap,
                                                                                               ind->addr.uap,
                                                                                               ind->addr.lap,
                                                                                               ind->status);

    if(ind->status == handset_service_status_link_loss)
    {
        headsetSmSetEventDisconnectionByLinkLoss();
    }

    headsetSmUpdateDisconnectingLink();

    /* if USB is allowed along with BT, then it could so happen that BT disconnects while USB is streaming */
    if(!headsetSmStateIsPairing(headsetGetState()) && headsetSmStateIsIdle(headsetSmDetermineCoreState()))
    {
        /* Inform UI that a handset has been disconnected */
        Ui_InformContextChange(ui_provider_app_sm, context_app_sm_idle);
    }
    if(headsetSmIsIdleTimerNeedToStart())
    {
        headsetSMStartIdleTimer();
    }
}

static void headsetSmHandleHandsetServiceMpConnectStopCfm(const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T *cfm)
{
    DEBUG_LOG("headsetSmHandleHandsetServiceMpConnectStopCfm status %u", cfm->status);
}

/*! \brief Handle notification of ACL connection/disconnection. */
static void headsetSmHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *ind)
{
    DEBUG_LOG("headsetSmHandleConManagerConnectionInd: CON_MANAGER_CONNECTION_IND Connected = %d, Transport BLE = %d",
                     ind->connected, ind->ble);
    headsetSmPrintBdaddr(ind->bd_addr);
    
#ifdef ENABLE_TWM_SPEAKER
    if(!ind->connected && !ind->ble)
    {
        /* did we disconnect peer? */
        if(appDeviceIsPeer(&ind->bd_addr) && IsTwmActive())
        {
            /* we disconnected peer but we are still in TWM mode, probably waiting for peer to come back? 
               let's re-start PFR and wait for peer to come-back */
            StereoTopology_StartPeerFindRole(headsetSmGetTask(), appConfigSpeakerPeerFindRoleTimeout());
        }
    }
#endif
}

static headsetState headsetSmDetermineCoreState(void)
{
    bool busy = IsHeadsetAudioActive();

    headsetState current_state = headsetGetState();
    
    if ((HEADSET_STATE_IDLE == current_state) || (HEADSET_STATE_BUSY == current_state))
    {
        return busy ? HEADSET_STATE_BUSY:
                          HEADSET_STATE_IDLE;
    }
    else
    {
        return current_state;
    }
}

bool headetSmHandleTopologyStopCfm(Message message)
{
    UNUSED(message);
    DEBUG_LOG_VERBOSE("headetSmHandleTopologyStopCfm STEREO_TOPOLOGY_STOP_CFM");

    if(headsetGetState() == HEADSET_STATE_POWERING_OFF)
    {
        /* Stop topology had called as part of power off. */
        headsetSetState(HEADSET_STATE_LIMBO);
#ifndef INCLUDE_ACCESSORY_TRACKING
        appPowerOffRequest();
#endif
    }
    else
    {
       /* Topology has stopped. Enable Wired audio controller. */
       HeadsetWiredAudioController_Enable();
    }

    return TRUE;
}

#ifdef ALLOW_WA_BT_COEXISTENCE
static void headsetSmHandleUsbDeviceEnumerated(void)
{
    DEBUG_LOG_VERBOSE("headsetSmHandleUsbDeviceEnumerated");
    if(HeadsetUsb_IsAudioEnabled() && headsetSmStateIsIdle(headsetGetState()))
    {
        headsetSMStopIdleTimer();
    }
}

static void headsetSmHandleUsbDeviceDeconfigured(void)
{
    DEBUG_LOG_VERBOSE("headsetSmHandleUsbDeviceDeconfigured");
    if(HeadsetUsb_IsAudioEnabled() && headsetSmStateIsIdle(headsetGetState()) && headsetSmIsIdleTimerNeedToStart())
    {
        headsetSMStartIdleTimer();
    }
}
#else
void headsetSmWiredAudioConnected(void)
{
    headsetState current_state = headsetGetState();

    DEBUG_LOG_INFO("headsetSmWiredAudioConnected Headset state %d",current_state);
    switch(current_state)
    {
        case HEADSET_STATE_POWERING_ON:
        case HEADSET_STATE_IDLE:
        case HEADSET_STATE_BUSY:
        {
          /* Move to Busy state */
            headsetSetState(HEADSET_STATE_BUSY);
        }
        break;

        default:
        break;
    }
}

void headsetSmWiredAudioDisconnected(void)
{
    headsetState current_state = headsetGetState();

    DEBUG_LOG_INFO("headsetSmWiredAudioDisconnected Headset state %d",current_state);
    switch(current_state)
    {
        case HEADSET_STATE_BUSY:
        {
            /* Move to headset idle state */
            headsetSetState(HEADSET_STATE_IDLE);
        }
        break;

        default:
        break;
    }
}
#endif /* ALLOW_WA_BT_COEXISTENCE */

/*! \brief Mark rule action complete on active rule set */
static void appSmRulesSetRuleComplete(MessageId message)
{
    DEBUG_LOG_DEBUG("appSmRulesSetRuleComplete %d", message);
    HeadsetRules_SetRuleComplete(message);
}

static void headsetSmHandleHSRulesConnectHandset(const CONNECT_HANDSET_PARAMS_T* params)
{
    DEBUG_LOG_DEBUG("headsetSmHandleHSRulesConnectHandset link_loss=%d", params->link_loss);

    bdaddr handset_addr;
    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        if (params->link_loss)
        {
            HandsetService_ReconnectLinkLossRequest(headsetSmGetTask());
        }
        else
        {
            HandsetService_ReconnectRequest(headsetSmGetTask());
        }
    }
    else
    {
        DEBUG_LOG_ERROR("HeadsetRules_HandleDecision shouldn't be called with no paired handset");
        Panic();
    }
    appSmRulesSetRuleComplete(HS_RULES_CON_HANDSET);
}

static void headsetSmHandleHSRulesDisconnectHandset(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleHSRulesDisconnectHandset");
    HandsetService_DisconnectAll(headsetSmGetTask(), HCI_ERROR_OETC_USER);
    appSmRulesSetRuleComplete(HS_RULES_DISCON_ALL_HANDSET);
}

static void headsetSmHandleHSRulesDisconnectLRUHandset(void)
{
    DEBUG_LOG_DEBUG("headsetSmHandleHSRulesDisconnectLRUHandset");
    HandsetService_DisconnectLruHandsetRequest(headsetSmGetTask());
    appSmRulesSetRuleComplete(HS_RULES_DISCON_LRU_HANDSET);
}

void headsetSmHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    if (isMessageUiInput(id))
    {
       headsetSmHandleUiInput(id);
       return;
    }

    switch (id)
    {
#ifdef INCLUDE_DFU
        case DFU_REQUESTED_TO_CONFIRM:
            /* Upgrade almost over, Stay in upgrade mode for upgrade to be completed */
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_REQUESTED_TO_CONFIRM");
            Dfu_SetRebootReason(REBOOT_REASON_DFU_RESET);
            break;

        case DFU_REQUESTED_IN_PROGRESS:
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_REQUESTED_IN_PROGRESS");
            Dfu_SetRebootReason(REBOOT_REASON_ABRUPT_RESET);
            break;

        case DFU_STARTED:
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_STARTED");
            break;

        case DFU_COMPLETED:
            GattServerGatt_SetGattDbChanged();
            Dfu_SetRebootReason(REBOOT_REASON_NONE);
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_COMPLETED");
            break;

        case DFU_ABORTED:
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_ABORTED");
            break;

        case DFU_READY_FOR_SILENT_COMMIT:
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_DFU_READY_FOR_SILENT_COMMIT");
            break;
#endif
        case ANC_UPDATE_STATE_ENABLE_IND:
             headsetSmHandleAncUpdateStateEnableInd();
             DEBUG_LOG_DEBUG("headsetSmHandleMessage ANC_UPDATE_STATE_ENABLE_IND");
             break;

        case ANC_UPDATE_STATE_DISABLE_IND:
             headsetSmHandleAncUpdateStateDisableInd();
             DEBUG_LOG_DEBUG("headsetSmHandleMessage ANC_UPDATE_STATE_DISABLE_IND");
             break;

        case LEAKTHROUGH_UPDATE_STATE_IND:
             headsetSmHandleLeakthroughStateInd((LEAKTHROUGH_UPDATE_STATE_IND_T *)message);
             DEBUG_LOG_DEBUG("headsetSmHandleMessage LEAKTHROUGH_UPDATE_ENABLE_IND");
             break;

        case SYSTEM_STATE_STATE_CHANGE:
             headsetSmHandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
             break;

        /* Pairing completion confirmations */
        case HANDSET_SERVICE_PAIR_HANDSET_CFM:
            headsetSmHandlePairHandsetConfirm();
            break;

        /* Handset Service disconnected indication */
        case HANDSET_SERVICE_DISCONNECTED_IND:
             headsetSmHandleHandsetServiceDisconnectedInd((HANDSET_SERVICE_DISCONNECTED_IND_T *) message);
             break;

        /* Handset service connected indication */
        case HANDSET_SERVICE_CONNECTED_IND:
            headsetSmHandleHandsetServiceConnectedInd((HANDSET_SERVICE_CONNECTED_IND_T*)message);
            break;

        /* Handset service connection stop confirmation */
        case HANDSET_SERVICE_MP_CONNECT_STOP_CFM:
            headsetSmHandleHandsetServiceMpConnectStopCfm((const HANDSET_SERVICE_MP_CONNECT_STOP_CFM_T *)message);
            break;

        /* Topology Messages */
        case STEREO_TOPOLOGY_STOP_CFM:
            headetSmHandleTopologyStopCfm(message);
            break;

        case STEREO_TOPOLOGY_STOPPING_CFM:
            headsetSmHandleStereoTopologyStateStopping();
            break;

        case STEREO_TOPOLOGY_STARTED_CFM:
            headsetSmHandleStereoTopologyStateStarted();
            break;

        case STEREO_TOPOLOGY_STARTING_CFM:
            headsetSmHandleStereoTopologyStateStarting();
            break;

        case STEREO_TOPOLOGY_PEER_PAIR_CFM:
            /* peer pair is complete */
            DEBUG_LOG_DEBUG("headsetSmHandleMessage STEREO_TOPOLOGY_PEER_PAIR_CFM");
#ifdef ENABLE_TWM_SPEAKER
            headsetSmHandlePeerPairedCfm();
#endif
            break;

#ifdef ENABLE_TWM_SPEAKER
        case STEREO_TOPOLOGY_FIND_ROLE_CFM:
            /* PFR is complete */
            headsetSmHandleStereoTopologyFindRoleCfm((STEREO_TOPOLOGY_FIND_ROLE_CFM_T*)message);
        break;

        case STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM:
            /* Speaker has entered standalone */
            headsetSmHandleStandaloneCompleteCfm((STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM_T*)message);
            break;

        case STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM:
            /* primary has fininished connecting peer profiles, start now normal startup procedure */
            headsetSmHandleStereoTopologyPeerProfileConnCfm((STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM_T*)message);
        break;

        case PEER_FIND_ROLE_PRIMARY:
            {
                /* looks like peer got connected and happen to be primary */
                STEREO_TOPOLOGY_FIND_ROLE_CFM_T cfm = { stereo_find_role_primary };
                headsetSmHandleStereoTopologyFindRoleCfm(&cfm);
            }
            break;
#endif /* ENABLE_TWM_SPEAKER */

        /* TODO Add handling for messages from various registered domains/components */

#ifdef ALLOW_WA_BT_COEXISTENCE
        /* USB device enumerated indication */
        case USB_DEVICE_ENUMERATED:
            headsetSmHandleUsbDeviceEnumerated();
            break;

        /* USB device deconfigured indication */
        case USB_DEVICE_DECONFIGURED:
            headsetSmHandleUsbDeviceDeconfigured();
            break;

        /* USB Audio status change inidications */
        case USB_AUDIO_CONNECTED_IND:
        case USB_AUDIO_DISCONNECTED_IND:
        /* Line-in status change indications */
        case WIRED_AUDIO_DEVICE_CONNECT_IND:
        case WIRED_AUDIO_DEVICE_DISCONNECT_IND:
#endif /* ALLOW_WA_BT_COEXISTENCE */

        /* AV status change indications */
        case AV_STREAMING_ACTIVE_IND:
        case AV_STREAMING_INACTIVE_IND:
        /* Telephony status change indications. Include all voice sources like HFP,USB Voice, etc */
        case TELEPHONY_CALL_ENDED:
        case TELEPHONY_CALL_ONGOING:
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case LE_AUDIO_UNICAST_MEDIA_CONNECTED:
        case LE_AUDIO_UNICAST_MEDIA_DISCONNECTED:
        case LE_AUDIO_UNICAST_VOICE_CONNECTED:
        case LE_AUDIO_UNICAST_VOICE_DISCONNECTED:
#endif
#ifdef INCLUDE_LE_AUDIO_BROADCAST
        case LE_AUDIO_BROADCAST_CONNECTED:
        case LE_AUDIO_BROADCAST_DISCONNECTED:
#endif
            if(headsetSmIsActiveState(headsetGetState()))
            {
                headsetSetState(headsetSmDetermineCoreState());
            }
#ifdef INCLUDE_DFU
            /* Inform upgrade library when SCO is connected/disconnected */
            if (id == TELEPHONY_CALL_ONGOING || id == TELEPHONY_CALL_ENDED)
            {
                 telephony_message_t *ind = (telephony_message_t *)message;
                 /* This is not applicable for USB as voice source */
                 if(ind->voice_source != voice_source_usb)
                     UpgradeSetScoActive(id == TELEPHONY_CALL_ONGOING);
            }
#endif
            break;

        /* Charger indications */
        case CHARGER_MESSAGE_DETACHED:
            headsetSmHandleChargerMessageDetached();
            break;
        case CHARGER_MESSAGE_CHARGING_OK:
        case CHARGER_MESSAGE_CHARGING_LOW:
            /* Consume frequently occuring charger messages with no operation required. */
            break;

        /* Power indications */
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            DEBUG_LOG_DEBUG("headsetSmHandleMessage APP_POWER_SHUTDOWN_PREPARE_IND");
            headsetSmHandlePowerShutdownPrepareInd();
            break;

        case SM_INTERNAL_FACTORY_RESET:
            headsetSmHandleInternalFactoryReset();
            break;

        case SM_INTERNAL_PAIR_HANDSET:
            headsetSmHandleInternalPairHandset();
            break;
#ifdef ENABLE_TWM_SPEAKER
        case SM_INTERNAL_PEER_PAIR:
            headsetSmHandleInternalPeerPair();
            break;

        case SM_INTERNAL_ENTER_TWM_MODE:
            headsetSmHandleInternalEnterTwmMode((SM_INTERNAL_PEER_FIND_ROLE_T*)message);
            break;

        case SM_INTERNAL_ENTER_STANDALONE_MODE:
            headsetSmHandleInternalEnterStereoStandaloneMode();
            break;
#endif

        case SM_INTERNAL_DELETE_HANDSETS:
            headsetSmHandleInternalDeleteHandsets();
            break;

        case SM_INTERNAL_POWER_OFF:
            headsetSmHandleInternalPowerOff();
            break;

        case SM_INTERNAL_TIMEOUT_IDLE:
            headsetSmHandleTimeoutIdle();
            break;

        case SM_INTERNAL_TIMEOUT_LIMBO:
            headsetSmHandleTimeoutLimbo();
            break;

        case SM_INTERNAL_LINK_DISCONNECTION_COMPLETE:
            headsetSmHandleInternalLinkDisconnectionComplete();
            break;

        case SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION:
            headsetSmLinkDisconnectionTimeout();
            break;

        /* Headset Rules Messages */
        case HS_RULES_CON_HANDSET:
            headsetSmHandleHSRulesConnectHandset((CONNECT_HANDSET_PARAMS_T*) message);
            break;

        case HS_RULES_DISCON_ALL_HANDSET:
            headsetSmHandleHSRulesDisconnectHandset();
            break;

        case HS_RULES_DISCON_LRU_HANDSET:
            headsetSmHandleHSRulesDisconnectLRUHandset();
            break;

        /* Connection Manager Messages */
        case CON_MANAGER_CONNECTION_IND:
            headsetSmHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *) message);
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}
/*! \brief  Provides Headset active state context to the User Interface module.Serves as a callback to the
 *          UI for current context. Returns only context_app_sm_active / context_app_sm_inactive.

    \param[in]  void

    \return     current_sm_ctxt - current context of sm module.
*/
static unsigned headsetSm_GetApplicationActiveStateContext(void)
{
    sm_provider_context_t context = context_app_sm_active;

    if(headsetSm_GetApplicationCurrentContext() == context_app_sm_powered_off)
    {
        context = context_app_sm_inactive;
    }

    return (unsigned)context;
}

/*! \brief  Indicates whether the Application is currently screening Logical Inputs
            (i.e. button presses) to inhibit the generation of UI Inputs in the UI domain

    \param[in]  logical_input - the logical input to check for

    \return     bool - TRUE if screening is active.
*/
static bool headsetSm_IsLogicalInputScreeningActive(unsigned logical_input)
{
    bool screen_logical_input = FALSE;
    if (headsetSm_GetApplicationCurrentContext() == context_app_sm_powered_off)
    {
        /* Ensure we don't screen the POWER_ON button press, or any other non-screened events */
        if (AppUi_IsLogicalInputScreenedInLimboState(logical_input))
        {
            screen_logical_input = TRUE;
        }
    }
    return screen_logical_input;
}

/*! \brief Initiate disconnect of handset link */
bool headsetSmDisconnectLink(void)
{
    bdaddr handset_addr = {0};
    bool disconnecting = FALSE;

    if (appDeviceGetHandsetBdAddr(&handset_addr) && HandsetService_IsAnyDeviceConnected())
    {
        headsetSmSetEventDisconnectAllHandsets();
        disconnecting = TRUE;
    }

    return disconnecting;
}

void headsetSmSetEventConnectMruHandset(void)
{
    DEBUG_LOG_DEBUG("HeadsetRules_ConnectMruHandset");
    HeadsetRules_SetEvent(HS_EVENT_USER_CON_HANDSET);
}

void headsetSmSetEventDisconnectLruHandset(void)
{
    DEBUG_LOG_DEBUG("HeadsetRules_DisconnectLruHandset");
    HeadsetRules_SetEvent(HS_EVENT_DISCON_LRU_HANDSET);
}

void headsetSmSetEventDisconnectAllHandsets(void)
{
    DEBUG_LOG_DEBUG("HeadsetRules_DisconnectAllHandsets");
    HeadsetRules_SetEvent(HS_EVENT_DISCON_ALL_HANDSET);
}

void headsetSmSetAllowHeadsetRulesToRun(bool allow)
{
    DEBUG_LOG_DEBUG("HeadsetRules_AllowRulesToRun %d", allow);
    SmGetTaskData()->allow_rules_to_run = allow;
    return;
}

/*! \brief Initialise the main application state machine.
 */
bool headsetSmInit(Task init_task)
{
    smTaskData* sm = SmGetTaskData();
    memset(sm, 0, sizeof(*sm));
    sm->task.handler = headsetSmHandleMessage;
    sm->state = HEADSET_STATE_NULL;
    sm->disconnect_lock= 0;
    sm->user_pairing = FALSE;
    sm->allow_rules_to_run = FALSE;
#ifdef ENABLE_TWM_SPEAKER
    sm->spk_type_is_standalone = TRUE;
    sm->spk_party_mode = FALSE;
    sm->twm_role = stereo_find_role_no_peer;
#endif

#ifdef ENABLE_HEADSET_AUTO_POWER_ON
    sm->auto_poweron = TRUE;
#else
    sm->auto_poweron = FALSE;
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    sm->spk_lea_broadcasting_media = FALSE;
#endif

    if( (SystemReboot_GetAction() == reboot_action_active_state) || headsetSmIsLastResetDueToPanic() )
    {
        headsetSmSetAutoPowerOn();
        SystemReboot_ResetAction();
    }
    /* register with connection manager to get notification of (dis)connections */
    ConManagerRegisterConnectionsClient(&sm->task);

    /* register with Telephony service for changes in state */
    Telephony_RegisterForMessages(&sm->task);

    /* register with AV to receive notifications of A2DP and AVRCP activity */
    appAvStatusClientRegister(&sm->task);

    /* register with power to receive sleep/shutdown messages. */
    appPowerClientRegister(&sm->task);

    /* register with charger monitor to receive charger messages. */    
    (void)Charger_ClientRegister(&sm->task);

    /* register with handset service as we need disconnect and connect notification */
    HandsetService_ClientRegister(&sm->task);

    /* Register for topology message indications */
    StereoTopology_RegisterMessageClient(headsetSmGetTask());

    /* Register for connection manager TP message indication */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, headsetSmGetTask());
    
    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(&sm->task);

    Ui_RegisterUiInputConsumer(headsetSmGetTask(), sm_ui_inputs, ARRAY_DIM(sm_ui_inputs));

    /* Register sm as ui provider*/
    Ui_RegisterUiProvider(ui_provider_app_sm, headsetSm_GetApplicationActiveStateContext );
    Ui_RegisterLogicalInputScreeningDecider(headsetSm_IsLogicalInputScreeningActive);

    /* Register with ANC state manager to receive ANC ON/OFF notifications */
    AncStateManager_ClientRegister(&sm->task);

    /* Register to receive Leakthrough Enable/Disable notifications */
    AecLeakthrough_ClientRegister(&sm->task);

#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))
    /* Register to receive to receive LE Audio messages */
    LeAudioMessages_ClientRegister(&sm->task);
#endif

#ifdef ALLOW_WA_BT_COEXISTENCE
    /* Register for wired audio source messages */
    WiredAudioSource_ClientRegister(headsetSmGetTask());

    /* Register for USB messages */
    UsbDevice_ClientRegister(headsetSmGetTask());
    /* Register for USB audio source messages*/
    UsbAudio_ClientRegister(headsetSmGetTask(), USB_AUDIO_REGISTERED_CLIENT_STATUS);
#endif /* ALLOW_WA_BT_COEXISTENCE */


    /* If DFU support is enabled, then set the QoS as low latency for better
     * DFU performance over LE Transport.
     * This will come at the cost of high power consumption.
     */

    UNUSED(init_task);
    return TRUE;
}

