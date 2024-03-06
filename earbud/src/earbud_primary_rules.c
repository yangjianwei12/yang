/*!
\copyright  Copyright (c) 2005 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_primary_rules.c
\brief	    Earbud application rules run when in a Primary earbud role.
*/

#include "earbud_primary_rules.h"
#include "earbud_config.h"
#include "earbud_rules_config.h"
#include "earbud_sm_private.h"

#include <earbud_config.h>
#include <earbud_init.h>
#include <adk_log.h>
#include <anc_state_manager.h>
#include <earbud_sm.h>
#include <connection_manager.h>
#include <earbud_test.h>

#include <domain_message.h>
#include <av.h>
#include <phy_state.h>
#include <bt_device.h>
#include <bredr_scan_manager.h>
#include <hfp_profile.h>
#include <state_proxy.h>
#include <rules_engine.h>
#include <peer_signalling.h>
#include <mirror_profile.h>
#include <dfu.h>
#include <dfu_peer.h>
#include <cc_with_case.h>

#include <bdaddr.h>
#include <panic.h>
#include <system_clock.h>
#include <voice_ui.h>
#include <cc_with_case.h>
#include <handset_service.h>
#include <handset_service_config.h>

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include <le_unicast_manager.h>
#endif
/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(earbud_primary_rules_messages)

#pragma unitsuppress Unused

/*! \{
    Macros for diagnostic output that can be suppressed. */
#define PRIMARY_RULE_LOG         DEBUG_LOG_DEBUG
/*! \} */

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below */
static rule_action_t PrimaryRulesCopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.
    Copies the parameters/data into conn_rules where the rules engine can uses
    it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   PrimaryRulesCopyRunParams(&(x), sizeof(x))

/*! Get pointer to the connection rules task data structure. */
#define PrimaryRulesGetTaskData()           (&primary_rules_task_data)

/*!< Connection rules. */
PrimaryRulesTaskData primary_rules_task_data;

/*! \{
    Rule function prototypes, so we can build the rule tables below. */
DEFINE_RULE(ruleAutoHandsetPair);
DEFINE_RULE(ruleConnectHandsetRoleSwitch);
DEFINE_RULE(ruleConnectHandsetUser);
DEFINE_RULE(ruleConnectHandsetLinkLoss);
DEFINE_RULE(ruleDisconnectLruHandsetUser);
DEFINE_RULE(ruleDisconnectAllHandsetsUser);
DEFINE_RULE(ruleForwardLinkKeys);
#ifdef INCLUDE_FAST_PAIR
DEFINE_RULE(ruleForwardAccountKeys);
#endif

DEFINE_RULE(ruleOutOfEarMediaActive);
DEFINE_RULE(ruleInEarCancelAudioPause);
DEFINE_RULE(ruleInCaseCancelAudioPause);
DEFINE_RULE(rulePeerInCaseCancelAudioPause);

DEFINE_RULE(ruleInEarMediaRestart);

DEFINE_RULE(ruleInEarScoTransferToEarbud);
DEFINE_RULE(ruleInCaseEnterDfu);

#ifdef INCLUDE_DFU
DEFINE_RULE(ruleCheckUpgradable);
#endif

DEFINE_RULE(ruleInCaseScoTransferToHandset);
DEFINE_RULE(rulePeerScoControl);

DEFINE_RULE(ruleOutOfCaseAncTuning);
DEFINE_RULE(ruleInCaseLedEnable);
#ifdef INCLUDE_SENSOR_PROFILE
DEFINE_RULE(ruleInCaseSpatialAudioDisable);
#endif
DEFINE_RULE(ruleInCaseAncTuning);

DEFINE_RULE(ruleOutOfCaseAdaptiveAncTuning);

DEFINE_RULE(ruleOutEarDisableAnc);
DEFINE_RULE(ruleInEarEnableAnc);

DEFINE_RULE(ruleInCaseDisableLeakthrough);

DEFINE_RULE(ruleSelectRemoteAudioMix);
DEFINE_RULE(ruleSelectLocalAudioMix);

DEFINE_RULE(ruleInEarCheckIncomingCall);

DEFINE_RULE(ruleDetermineIfEitherEarbudIsInEar);
/*! \} */

/*! \brief Set of rules to run on Earbud startup. */
const rule_entry_t primary_rules_set[] =
{
#ifdef INCLUDE_DFU
    /*! \{
        Rules that should always run on any event */
    RULE_ALWAYS(RULE_EVENT_CHECK_DFU,           ruleCheckUpgradable,               CONN_RULES_DFU_ALLOW),
#endif

    /*! \} */

    RULE(RULE_EVENT_ROLE_SWITCH,                ruleAutoHandsetPair,               CONN_RULES_HANDSET_PAIR),
    RULE(RULE_EVENT_ROLE_SWITCH,                ruleConnectHandsetRoleSwitch,      CONN_RULES_HANDSET_CONNECT),
    RULE(RULE_EVENT_HANDSET_LINK_LOSS,          ruleConnectHandsetLinkLoss,        CONN_RULES_HANDSET_CONNECT),
    RULE(RULE_EVENT_CONNECT_HANDSET_USER,       ruleConnectHandsetUser,            CONN_RULES_HANDSET_CONNECT),
    RULE(RULE_EVENT_DISCONNECT_LRU_HANDSET_USER,ruleDisconnectLruHandsetUser,      CONN_RULES_LRU_HANDSET_DISCONNECT),
    RULE(RULE_EVENT_DISCONNECT_ALL_HANDSETS_USER,ruleDisconnectAllHandsetsUser,    CONN_RULES_ALL_HANDSETS_DISCONNECT),

    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseLedEnable,               CONN_RULES_LED_ENABLE),
#ifdef INCLUDE_SENSOR_PROFILE
    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseSpatialAudioDisable,     CONN_RULES_SPATIAL_AUDIO_DISABLE),
#endif

    /*! \{
        Rules to drive ANC tuning. */
    RULE(RULE_EVENT_OUT_CASE,                   ruleOutOfCaseAncTuning,            CONN_RULES_ANC_TUNING_STOP),
    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseAncTuning,               CONN_RULES_ANC_TUNING_START),
    /*! \} */

    /*! \{
        Rule to exit Adaptive ANC tuning. */
    RULE(RULE_EVENT_OUT_CASE,                   ruleOutOfCaseAdaptiveAncTuning,    CONN_RULES_ADAPTIVE_ANC_TUNING_STOP),
    /*! \} */

    /*! \{
        Rules to drive ANC states in RDP project. */
    RULE(RULE_EVENT_OUT_EAR,                    ruleOutEarDisableAnc,              CONN_RULES_ANC_DISABLE),
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarEnableAnc,                CONN_RULES_ANC_ENABLE),
    /*! \} */

    /*! \{
        Rule to enable automatic handset pairing when taken out of the case or case lid is opened. */
    RULE(RULE_EVENT_OUT_CASE,                   ruleAutoHandsetPair,               CONN_RULES_HANDSET_PAIR),
    RULE(RULE_EVENT_CASE_LID_OPEN,              ruleAutoHandsetPair,               CONN_RULES_HANDSET_PAIR),
    /*! \} */

    /*! \{
        Rules to synchronise link keys.
        \todo will be moving into TWS topology */
    RULE(RULE_EVENT_PEER_UPDATE_LINKKEYS,       ruleForwardLinkKeys,               CONN_RULES_PEER_SEND_LINK_KEYS),
    RULE(RULE_EVENT_PEER_CONNECTED,             ruleForwardLinkKeys,               CONN_RULES_PEER_SEND_LINK_KEYS),
    /*! \} */

#ifdef INCLUDE_FAST_PAIR
    /*! \{
        Rule to synchronize fp account keys.*/
    RULE(RULE_EVENT_PEER_CONNECTED,             ruleForwardAccountKeys,            CONN_RULES_PEER_SEND_FP_ACCOUNT_KEYS),
    /*! \} */
#endif


    /*** Below here are more Earbud product behaviour type rules, rather than connection/topology type rules
     *   These are likely to stay in the application rule set.
     ****/

    /*! \{
        Rules to start DFU when going in the case. */
    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseEnterDfu,                         CONN_RULES_ENTER_DFU),
    /*! \} */

    /*! \{
        Rules to control audio pauses when in/out of the ear. */
    RULE(RULE_EVENT_OUT_EAR,                    ruleOutOfEarMediaActive,                    CONN_RULES_MEDIA_PAUSE),
    RULE(RULE_EVENT_PEER_OUT_EAR,               ruleOutOfEarMediaActive,                    CONN_RULES_MEDIA_PAUSE),
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarCancelAudioPause,                  CONN_RULES_MEDIA_TIMEOUT_CANCEL),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleInEarCancelAudioPause,                  CONN_RULES_MEDIA_TIMEOUT_CANCEL),
    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseCancelAudioPause,                 CONN_RULES_MEDIA_TIMEOUT_CANCEL),
    RULE(RULE_EVENT_PEER_IN_CASE,               rulePeerInCaseCancelAudioPause,             CONN_RULES_MEDIA_TIMEOUT_CANCEL),
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarMediaRestart,                      CONN_RULES_MEDIA_PLAY),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleInEarMediaRestart,                      CONN_RULES_MEDIA_PLAY),
    RULE(RULE_EVENT_OUT_EAR,                    PrimaryRuleTransferScoToHandsetIfOutOfEar,  CONN_RULES_SCO_TIMEOUT),
    RULE(RULE_EVENT_PEER_OUT_EAR,               PrimaryRuleTransferScoToHandsetIfOutOfEar,  CONN_RULES_SCO_TIMEOUT),
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarCheckIncomingCall,                 CONN_RULES_ACCEPT_INCOMING_CALL),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleInEarCheckIncomingCall,                 CONN_RULES_ACCEPT_INCOMING_CALL),
    /*! \} */

    /*! \{
        Rules to control SCO audio transfer. */
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarScoTransferToEarbud,       CONN_RULES_SCO_TRANSFER_TO_EARBUD),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleInEarScoTransferToEarbud,       CONN_RULES_SCO_TRANSFER_TO_EARBUD),
    RULE(RULE_EVENT_PEER_IN_CASE,               ruleInCaseScoTransferToHandset,     CONN_RULES_SCO_TRANSFER_TO_HANDSET),
    /*! \} */
    /*! \{
        Rules to control SCO on peer. */
    RULE(RULE_EVENT_ROLE_SWITCH,                rulePeerScoControl,                 CONN_RULES_PEER_SCO_CONTROL),
    RULE(RULE_EVENT_PEER_IN_EAR,                rulePeerScoControl,                 CONN_RULES_PEER_SCO_CONTROL),
    RULE(RULE_EVENT_PEER_OUT_EAR,               rulePeerScoControl,                 CONN_RULES_PEER_SCO_CONTROL),
    RULE(RULE_EVENT_SCO_ACTIVE,                 rulePeerScoControl,                 CONN_RULES_PEER_SCO_CONTROL),
#ifdef ENABLE_DYNAMIC_HANDOVER
    RULE(RULE_EVENT_IN_CASE,                    rulePeerScoControl,                 CONN_RULES_PEER_SCO_CONTROL),
#endif
    /*! \} */

    /*! \{
        Rules to control local/remote audio mix. */
    RULE(RULE_EVENT_PEER_CONNECTED,             ruleSelectRemoteAudioMix,           CONN_RULES_SET_REMOTE_AUDIO_MIX),
    RULE(RULE_EVENT_OUT_EAR,                    ruleSelectRemoteAudioMix,           CONN_RULES_SET_REMOTE_AUDIO_MIX),
    RULE(RULE_EVENT_IN_EAR,                     ruleSelectRemoteAudioMix,           CONN_RULES_SET_REMOTE_AUDIO_MIX),
    RULE(RULE_EVENT_IN_CASE,                    ruleSelectRemoteAudioMix,           CONN_RULES_SET_REMOTE_AUDIO_MIX),
    RULE(RULE_EVENT_OUT_CASE,                   ruleSelectRemoteAudioMix,           CONN_RULES_SET_REMOTE_AUDIO_MIX),

    /* In Ear and Out of Case need to trigger selection of local audio mix,
     * otherwise we start in single channel mode
     */
    RULE(RULE_EVENT_IN_EAR,                     ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_OUT_CASE,                   ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),

    RULE(RULE_EVENT_PEER_CONNECTED ,            ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_PEER_DISCONNECTED,          ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_PEER_OUT_EAR,               ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_PEER_IN_CASE,               ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    RULE(RULE_EVENT_PEER_OUT_CASE,              ruleSelectLocalAudioMix,            CONN_RULES_SET_LOCAL_AUDIO_MIX),
    /*! \} */

    /*! \{
        Rule to Disable Leakthrough. */
    RULE(RULE_EVENT_IN_CASE,                    ruleInCaseDisableLeakthrough,       CONN_RULES_LEAKTHROUGH_DISABLE),
    /*! \} */

    /*! \{
        Rules to control tones and prompts playback enabled status. */
    RULE(RULE_EVENT_OUT_EAR,                    ruleDetermineIfEitherEarbudIsInEar, CONN_RULES_AUDIO_UI_IND_CONTROL),
    RULE(RULE_EVENT_PEER_OUT_EAR,               ruleDetermineIfEitherEarbudIsInEar, CONN_RULES_AUDIO_UI_IND_CONTROL),
    RULE(RULE_EVENT_PEER_IN_EAR,                ruleDetermineIfEitherEarbudIsInEar, CONN_RULES_AUDIO_UI_IND_CONTROL),
    /*! \} */
};

/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/
/*! @brief Rule to determine if Earbud should start automatic handset pairing
    @startuml

    start
        if (IsInCase()) then (yes)
            :Earbud is in case, do nothing;
            end
        endif
        if (IsPairedWithHandset()) then (yes)
            :Already paired with handset, do nothing;
            end
        endif
        if (IsPeerPairing()) then (yes)
            :Peer is already pairing, do nothing;
            end
        endif
        if (IsPeerPairWithHandset()) then (yes)
            :Peer is already paired with handset, do nothing;
            end
        endif
        if (IsPeerInCase()) then (yes)
            :Start pairing, peer is in case;
            stop
        endif

        :Both Earbuds are out of case;
        if (IsPeerLeftEarbud) then (yes)
            stop
        else (no)
            end
        endif
    @enduml
*/
static rule_action_t ruleAutoHandsetPair(void)
{
    /* NOTE: Ordering of these checks is important */

    if (appSmIsInCase())
    {
        if (!CcWithCase_EventsEnabled())
        {
            PRIMARY_RULE_LOG("ruleAutoHandsetPair, ignore, in the case and lid events not enabled");
            return rule_action_ignore;
        }
        else
        {
            if (CcWithCase_GetLidState() == CASE_LID_STATE_CLOSED)
            {
                PRIMARY_RULE_LOG("ruleAutoHandsetPair, ignore, in the case with lid event enabled and lid is closed");
                return rule_action_ignore;
            }
        }
    }

    /* If paired with Handset, but remote in case exit pairing state, continue enter pairing */
    if(appSmPeerWasPairingWhenItEnteredCase())
    {
        PRIMARY_RULE_LOG("ruleAutoHandsetPair, override, peer device was pairing when it last entered case");
    }
    else if (BtDevice_IsPairedWithHandset())
    {
        PRIMARY_RULE_LOG("ruleAutoHandsetPair, complete, already paired with handset");
        return rule_action_complete;
    }

#ifndef DISABLE_TEST_API
    if (appTestHandsetPairingBlocked)
    {
        PRIMARY_RULE_LOG("ruleAutoHandsetPair, ignore, blocked by test command");
        return rule_action_ignore;
    }
#endif

    if (appSmIsPairing())
    {
        PRIMARY_RULE_LOG("ruleAutoHandsetPair, ignore, already in pairing mode");
        return rule_action_ignore;
    }

    if (StateProxy_HasPeerHandsetPairing())
    {
        PRIMARY_RULE_LOG("ruleAutoHandsetPair, complete, peer is already paired with handset");
        return rule_action_complete;
    }

    PRIMARY_RULE_LOG("ruleAutoHandsetPair, run, primary out of the case with no handset pairing");
    return rule_action_run;
}

static bool connectHandsetForDfu(void)
{
    return Dfu_IsUpgradeInProgress() && !Dfu_IsSilentCommitEnabled();
}

static rule_action_t ruleConnectHandset(ruleConnectReason reason)
{
    PRIMARY_RULE_LOG("ruleConnectHandset, reason enum:ruleConnectReason:%d",reason);
    /* NOTE: Ordering of these checks is important */

    if (!BtDevice_IsPairedWithHandset())
    {
        PRIMARY_RULE_LOG("ruleConnectHandset, ignore, not paired with handset");
        return rule_action_ignore;
    }

    if (appSmIsInCase())
    {
        if (!connectHandsetForDfu())
        {
            if (CcWithCase_EventsEnabled())
            {
                if (CcWithCase_GetLidState() == CASE_LID_STATE_CLOSED)
                {
                    PRIMARY_RULE_LOG("ruleConnectHandset, ignore as in case and lid is closed");
                    return rule_action_ignore;
                }
            }
            else
            {
                PRIMARY_RULE_LOG("ruleConnectHandset ignore as in case");
                return rule_action_ignore;
            }
        }
    }

    if (appSmIsPairing())
    {
        PRIMARY_RULE_LOG("ruleConnectHandset, ignore as pairing");
        return rule_action_ignore;
    }

    if (appSmGetReconnectPostHandover() && reason != RULE_CONNECT_REASON_LINK_LOSS)
    {
        PRIMARY_RULE_LOG("ruleConnectHandset, ignore as reconnection already pending post handover");
        return rule_action_ignore;
    }

    if (reason == RULE_CONNECT_REASON_ROLE_SWITCH)
    {
        if (HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_link_loss))
        {
            bool continue_link_loss_reconnection = appSmRestartReconnectionAfterRoleSelection();

            PRIMARY_RULE_LOG("ruleConnectHandset link loss handset found, continue_link_loss_reconnection=%d", continue_link_loss_reconnection);
            if (continue_link_loss_reconnection)
            {
                reason = RULE_CONNECT_REASON_LINK_LOSS;
            }
        }

        if (reason != RULE_CONNECT_REASON_LINK_LOSS)
        {
            if (HandsetService_IsAnyBredrHandsetFullyConnected())
            {
                PRIMARY_RULE_LOG("ruleConnectHandset, ignore as roleswitch and already fully connected to handset");
                return rule_action_ignore;
            }

            if (HandsetService_IsAnyLeConnected())
            {
                PRIMARY_RULE_LOG("ruleConnectHandset, ignore as roleswitch and already connected to LE handset");
                return rule_action_ignore;
            }
        }
    }

    PRIMARY_RULE_LOG("ruleConnectHandset, run");
    return RULE_ACTION_RUN_PARAM(reason);
}

static rule_action_t ruleConnectHandsetRoleSwitch(void)
{
    return ruleConnectHandset(RULE_CONNECT_REASON_ROLE_SWITCH);
}

static rule_action_t ruleConnectHandsetLinkLoss(void)
{
    return ruleConnectHandset(RULE_CONNECT_REASON_LINK_LOSS);
}

static rule_action_t ruleConnectHandsetUser(void)
{
    ruleConnectReason reason = RULE_CONNECT_REASON_USER;

    /* After a handover has occurred, this rule function is executed. If the old-primary was trying to reconnect
       a link-lost handset prior to the handover, continue this reconnection on the new primary Earbud. */
    if (HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_link_loss))
    {
        bool continue_link_loss_reconnection = appSmGetContinueLinkLossReconnectPostHandover();

        PRIMARY_RULE_LOG("ruleConnectHandsetUser link loss handset found, continue_link_loss_reconnection=%d", continue_link_loss_reconnection);
        if (continue_link_loss_reconnection)
        {
            appSmSetContinueLinkLossReconnectPostHandover(FALSE);
            reason = RULE_CONNECT_REASON_LINK_LOSS;
        }
    }

    return ruleConnectHandset(reason);
}

static rule_action_t ruleDisconnectAllHandsetsUser(void)
{
    if (!HandsetService_IsAnyDeviceConnected())
    {
        PRIMARY_RULE_LOG("ruleDisconnectAllHandsetsUser, ignore as no handset's connected");
        return rule_action_ignore;
    }
    return rule_action_run;
}

static rule_action_t ruleDisconnectLruHandsetUser(void)
{
    if (!HandsetService_IsAnyDeviceConnected())
    {
        PRIMARY_RULE_LOG("ruleDisconnectLruHandsetUser, ignore as no handset's connected");
        return rule_action_ignore;
    }
    return rule_action_run;
}

/*! @brief Rule to determine if Earbud should attempt to forward handset link-key to peer
    @startuml

    start
        if (IsPairedWithPeer()) then (yes)
            :Forward any link-keys to peer;
            stop
        else (no)
            :Not paired;
            end
    @enduml
*/
static rule_action_t ruleForwardLinkKeys(void)
{
    if (BtDevice_IsPairedWithPeer())
    {
        PRIMARY_RULE_LOG("ruleForwardLinkKeys, run");
        return rule_action_run;
    }
    else
    {
        PRIMARY_RULE_LOG("ruleForwardLinkKeys, ignore as there's no peer");
        return rule_action_ignore;
    }
}

/*! @brief Rule to determine if Earbud should attempt to forward fp account keys to peer
    @startuml

    start
        if (IsPairedWithPeer()) then (yes)
            :Forward fp aacount keys to peer;
            stop
        else (no)
            :Not paired;
            end
    @enduml
*/
#ifdef INCLUDE_FAST_PAIR
static rule_action_t ruleForwardAccountKeys(void)
{
    if(BtDevice_IsPairedWithPeer())
    {
        PRIMARY_RULE_LOG("ruleForwardAccountKeys, run");
        return rule_action_run;
    }
    else
    {
        PRIMARY_RULE_LOG("ruleForwardAccountKeys, ignore as there's no peer");
        return rule_action_ignore;
    }
}
#endif


#ifdef INCLUDE_LE_AUDIO_BROADCAST
static bool isBroadcastActive(void)
{
    bool active = FALSE;

    switch(AudioSources_GetSourceContext(audio_source_le_audio_broadcast))
    {
        case context_audio_is_high_priority:
        case context_audio_is_broadcast:
            active =  TRUE;
            break;

        default:
            break;
    }
    return active;
}
#endif /* INCLUDE_LE_AUDIO_BROADCAST */

/*! @brief Rule to determine if media streaming when out of ear
    Rule is triggered by the 'out of ear' event
    @startuml

    start
    if (IsMediaStreaming()) then (yes)
        :Run rule, as out of ear with Media streaming;
        stop
    endif
    end
    @enduml
*/
static rule_action_t ruleOutOfEarMediaActive(void)
{
    /* AVRCP playback status is updated first if supported and then after some delay(handset dependent) A2DP state.
       So, check AVRCP first then A2DP state. */
    if (appAvIsPlayStatusActive() && (VoiceUi_IsSessionInProgress() == FALSE))
    {
        PRIMARY_RULE_LOG("ruleOutOfEarMediaActive, run as A2DP is active and earbud out of ear");
        return rule_action_run;
    }

#ifdef INCLUDE_LE_AUDIO_UNICAST
    /* Only run the rule if unicast is active. */
    if (LeUnicastManager_IsUnicastMediaActive() && (VoiceUi_IsSessionInProgress() == FALSE))
    {
        PRIMARY_RULE_LOG("ruleOutOfEarMediaActive, run as Unicast is active and earbud out of ear");
        return rule_action_run;
    }
#endif /*INCLUDE_LE_AUDIO_UNICAST*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST
    /* Only run the rule if broadcast is active. */
    if (isBroadcastActive() && (VoiceUi_IsSessionInProgress() == FALSE))
    {
        PRIMARY_RULE_LOG("ruleOutOfEarMediaActive, run as broadcast is active and earbud out of ear");
        return rule_action_run;
    }
#endif

    PRIMARY_RULE_LOG("ruleOutOfEarMediaActive, ignore as media not active out of ear");
    return rule_action_ignore;
}

/*! \brief In ear event on either Earbud should cancel A2DP or SCO out of ear pause/tranfer. */
static rule_action_t ruleInEarCancelAudioPause(void)
{
    PRIMARY_RULE_LOG("ruleInEarCancelAudioPause, run as always to cancel timers on in ear event");
    /* Always running looks a little odd, but it means we have a common path for handling
     * either the local earbud in ear or in case, and the same events from the peer.
     * The alternative would be phystate event handling in SM for local events and
     * state proxy events handling in SM for peer in ear/case. */
    return rule_action_run;
}

/*! \brief In case event Earbud should cancel A2DP. */
static rule_action_t ruleInCaseCancelAudioPause(void)
{
    /*Cancel A2DP time out timer when peer earbud out of case*/
    if (StateProxy_IsPeerOutOfEar())
    {
        PRIMARY_RULE_LOG("ruleInCaseCancelAudioPause, ignore as peer in out of ear");
        return rule_action_ignore;
    }

    PRIMARY_RULE_LOG("ruleInCaseCancelAudioPause, run as always to cancel timers on in case event");
    return rule_action_run;
}

/*! \brief In case event on peer Earbud should cancel A2DP or SCO out of ear pause/tranfer. */
static rule_action_t rulePeerInCaseCancelAudioPause(void)
{
    /* Peer in case but earbud is not in ear. Don't cancel, transfer the call to AG */
    if (HfpProfile_IsScoActive() && !appSmIsInEar())
    {
        PRIMARY_RULE_LOG("rulePeerInCaseCancelAudioPause, ignore as SCO is active and earbud not in ear");
        return rule_action_ignore;
    }

    if (appSmIsOutOfEar())
    {
        PRIMARY_RULE_LOG("rulePeerInCaseCancelAudioPause, ignore as peer is incase and earbud not in ear");
        return rule_action_ignore;
    }

    /* For A2DP we always want to cancel the timer */
    PRIMARY_RULE_LOG("rulePeerInCaseCancelAudioPause, run as always to cancel timers on in case event");
    return rule_action_run;
}


/*! \brief Decide if we should restart media on going back in the ear within timeout. */
static rule_action_t ruleInEarMediaRestart(void)
{
    if (appSmIsMediaRestartPending())
    {
        PRIMARY_RULE_LOG("ruleInEarMediaRestart, run as media is paused within restart timer");
        return rule_action_run;
    }

    PRIMARY_RULE_LOG("ruleInEarMediaRestart, ignore as media restart timer not running");
    return rule_action_ignore;
}


/*! @brief Rule to determine if there is an incoming call
    Rule is triggered by the 'in ear' event of primary/secondary EB
    @startuml

    start
    if (appHfpIsCallIncoming()) then (yes)
        :Run rule, as in ear to accept the incoming call;
        stop
    endif
    end
    @enduml
*/
static rule_action_t ruleInEarCheckIncomingCall(void)
{
    if (appHfpIsCallIncoming())
    {
        PRIMARY_RULE_LOG("ruleInEarCheckIncomingCall, running");
        return rule_action_run;
    }

    PRIMARY_RULE_LOG("ruleInEarCheckIncomingCall, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleInEarScoTransferToEarbud(void)
{
    /* Nothing to do if call isn't already active or outgoing. */
    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();
    if (instance == NULL)
    {
        PRIMARY_RULE_LOG("ruleInEarScoTransferToEarbud, ignore as there is no focused HFP instance.");
        return rule_action_ignore;
    }

    if (!appHfpIsCallActiveForInstance(instance) && !appHfpIsCallOutgoingForInstance(instance))
    {
        PRIMARY_RULE_LOG("ruleInEarScoTransferToEarbud, ignore as this earbud has no active/outgoing call");
        return rule_action_ignore;
    }

    /* May already have SCO audio if kept while out of ear in order to service slave
     * for SCO forwarding */
    if (HfpProfile_IsScoActiveForInstance(instance))
    {
        PRIMARY_RULE_LOG("ruleInEarScoTransferToEarbud, ignore as this earbud already has SCO");
        return rule_action_ignore;
    }

    /* For TWS+ transfer the audio the local earbud is in Ear.
     * For TWS Standard, transfer the audio if local earbud or peer is in Ear. */
    if (appSmIsInEar() || (!appDeviceIsTwsPlusHandset(HfpProfile_GetHandsetBdAddr(instance)) && StateProxy_IsPeerInEar()))
    {
        PRIMARY_RULE_LOG("ruleInEarScoTransferToEarbud, run as call is active and an earbud is in ear");
        return rule_action_run;
    }

    PRIMARY_RULE_LOG("ruleInEarScoTransferToEarbud, ignore as SCO not active or both earbuds out of the ear");
    return rule_action_ignore;
}

/*! @brief Rule to determine if Earbud should start DFU  when put in case
    Rule is triggered by the 'in case' event
    @startuml

    start
    if (IsInCase() and DfuUpgradePending()) then (yes)
        :DFU upgrade can start as it was pending and now in case;
        stop
    endif
    end
    @enduml
*/
static rule_action_t ruleInCaseEnterDfu(void)
{
#ifdef INCLUDE_DFU
    if (appSmIsInCase() && appSmIsInDfuMode())
    {
        PRIMARY_RULE_LOG("ruleInCaseCheckDfu, run as still in case & DFU pending/active");
        return rule_action_run;
    }
    else
    {
        PRIMARY_RULE_LOG("ruleInCaseCheckDfu, ignore as not in case or no DFU pending");
        return rule_action_ignore;
    }
#else
    return rule_action_ignore;
#endif
}


#ifdef INCLUDE_DFU
static rule_action_t ruleCheckUpgradable(void)
{
    bool allow_dfu = TRUE;
    bool block_dfu = FALSE;
    bool is_upgrade_in_progress = Dfu_IsUpgradeInProgress();

    if (appSmIsOutOfCase())
    {

        if (appConfigDfuOnlyFromUiInCase())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as only allow DFU from UI (and in case)");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if (!is_upgrade_in_progress && !appPeerSigIsConnected())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as peer not connected and dfu is not in progress");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if(DfuPeer_StillInUseAfterAbort())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as abort clean up process is yet to be completed");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if (ConManagerAnyTpLinkConnected(cm_transport_ble) && appConfigDfuAllowBleUpgradeOutOfCase())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, allow as BLE connection");
            return RULE_ACTION_RUN_PARAM(allow_dfu);
        }
        if (appDeviceIsHandsetConnected() && appConfigDfuAllowBredrUpgradeOutOfCase())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, allow as BREDR connection");
            return RULE_ACTION_RUN_PARAM(allow_dfu);
        }
        if (Dfu_IsSilentCommitEnabled())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, allow as silent commit pending");
            return RULE_ACTION_RUN_PARAM(allow_dfu);
        }

        PRIMARY_RULE_LOG("ruleCheckUpgradable, block as out of case and not permitted");
        return RULE_ACTION_RUN_PARAM(block_dfu);
    }
    else
    {
        if (appConfigDfuOnlyFromUiInCase())
        {
            if (appSmIsInDfuMode())
            {
                PRIMARY_RULE_LOG("ruleCheckUpgradable, allow as in case - DFU pending");
                return RULE_ACTION_RUN_PARAM(allow_dfu);
            }
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as only allow DFU from UI");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if (!is_upgrade_in_progress && !appPeerSigIsConnected())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as peer not connected and dfu is not in progress");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if (!is_upgrade_in_progress && !StateProxy_IsPeerInCase())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as peer is not in-case and dfu is not in progress so, not permitted");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }
        if(DfuPeer_StillInUseAfterAbort())
        {
            PRIMARY_RULE_LOG("ruleCheckUpgradable, block as abort clean up process is yet to be completed");
            return RULE_ACTION_RUN_PARAM(block_dfu);
        }

        PRIMARY_RULE_LOG("ruleCheckUpgradable, allow as in case");
        return RULE_ACTION_RUN_PARAM(allow_dfu);
    }
}
#endif

static rule_action_t ruleInCaseLedEnable(void)
{
    PRIMARY_RULE_LOG("ruleInCaseLedEnable, run and enable LEDs");
    return rule_action_run;
}

#ifdef INCLUDE_SENSOR_PROFILE
static rule_action_t ruleInCaseSpatialAudioDisable(void)
{
    if (SpatialData_EnabledPrimary())
    {
        PRIMARY_RULE_LOG("%s, run and disable spatial audio", __func__);
        return rule_action_run;
    }
    else
    {
        PRIMARY_RULE_LOG("%s, already disabled, ignore", __func__);
        return rule_action_ignore;
    }
}
#endif /* INCLUDE_SENSOR_PROFILE */

static rule_action_t ruleInCaseDisableLeakthrough(void)
{
    if(appSmIsInCase() && AecLeakthrough_IsLeakthroughEnabled())
    {
        PRIMARY_RULE_LOG("ruleInEarDisableLeakthrough, run and Disable Leakthrough");
        return rule_action_run;
    }
    PRIMARY_RULE_LOG("ruleInEarDisableLeakthrough, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleInCaseAncTuning(void)
{
    if (appConfigAncTuningEnabled())
    {
        PRIMARY_RULE_LOG("ruleInCaseAncTuning, run and enter into the tuning mode");
        return rule_action_run;
    }
    PRIMARY_RULE_LOG("ruleInCaseAncTuning, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleInEarEnableAnc(void)
{
    if(appSmIsOutOfCase() && appSmGetANCStateWhenOutOfEar())
    {
        PRIMARY_RULE_LOG("ruleInEarEnableAnc, run and turn on the ANC");
        return rule_action_run;
    }
    else
    {
        PRIMARY_RULE_LOG("ruleInEarEnableAnc, ignored");
        return rule_action_ignore;
    }
}

static rule_action_t ruleOutEarDisableAnc(void)
{
#ifdef DEFAULT_ANC_ON
    if(appSmIsOutOfCase())
    {
        appSmSetANCStateWhenOutOfEar(AncStateManager_IsEnabled());
        PRIMARY_RULE_LOG("ruleOutEarDisableAnc, run and disable the ANC");
        return rule_action_run;
    }
#endif
    PRIMARY_RULE_LOG("ruleOutEarDisableAnc, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleOutOfCaseAncTuning(void)
{
    if (AncStateManager_IsTuningModeActive())
    {
        PRIMARY_RULE_LOG("ruleOutOfCaseAncTuning, run and exit from the tuning mode");
        return rule_action_run;
    }
    PRIMARY_RULE_LOG("ruleOutOfCaseAncTuning, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleOutOfCaseAdaptiveAncTuning(void)
{
#ifdef ENABLE_ADAPTIVE_ANC
    if (AncStateManager_IsAdaptiveAncTuningModeActive())
    {
        PRIMARY_RULE_LOG("ruleOutOfCaseAdaptiveAncTuning, run and exit from the Adaptive ANC tuning mode");
        return rule_action_run;
    }
#endif
    PRIMARY_RULE_LOG("ruleOutOfCaseAdaptiveAncTuning, ignored");
    return rule_action_ignore;
}

static rule_action_t ruleInCaseScoTransferToHandset(void)
{
    if (!HfpProfile_IsScoActive())
    {
        PRIMARY_RULE_LOG("ruleInCaseScoTransferToHandset, ignore as no active call");
        return rule_action_ignore;
    }
    if (appSmIsInCase() && StateProxy_IsPeerInCase())
    {
        PRIMARY_RULE_LOG("ruleInCaseScoTransferToHandset, run, we have active call but both earbuds in case");
        return rule_action_run;
    }
    PRIMARY_RULE_LOG("ruleInCaseScoTransferToHandset, ignored");
    return rule_action_ignore;
}

static rule_action_t rulePeerScoControl(void)
{
    const bool enabled = TRUE;
    const bool disabled = FALSE;

    if (!StateProxy_IsPeerInEar())
    {
#ifdef ENABLE_DYNAMIC_HANDOVER
        if (appSmIsInCase())
        {
            /* Ignore the rule, if primary enters case and secondary is in case */
            if (StateProxy_IsPeerInCase())
            {
                PRIMARY_RULE_LOG("rulePeerScoControl, ignore as peer in case");
                return rule_action_ignore;
            }
            /* For handover to succeed, if primary has active eSCO, the secondary must
            be mirroring the eSCO. But this rule disables eSCO mirroring if the
            secondary is not in ear. To make handover succeed when primary enters
            case with active eSCO but secondary is not in ear, eSCO mirroring is
            enabled */
            else
            {
                PRIMARY_RULE_LOG("rulePeerScoControl, run and enable as entered case");
                return RULE_ACTION_RUN_PARAM(enabled);
            }
        }
#endif
        PRIMARY_RULE_LOG("rulePeerScoControl, run and disable as peer out of ear");
        return RULE_ACTION_RUN_PARAM(disabled);
    }
    if (StateProxy_IsPeerInEar())
    {
        PRIMARY_RULE_LOG("rulePeerScoControl, run and enable as peer in ear");
        return RULE_ACTION_RUN_PARAM(enabled);
    }

    PRIMARY_RULE_LOG("rulePeerScoControl, ignore");
    return rule_action_ignore;
}

/*! @brief Rule to select the audio mix to be rendered by the remote earbud */
static rule_action_t ruleSelectLocalAudioMix(void)
{
    bool stereo_mix = TRUE;

    if (appPeerSigIsConnected() && StateProxy_IsPeerInEar())
    {
        stereo_mix = FALSE;
    }

    PRIMARY_RULE_LOG("ruleSelectLocalAudioMix stereo_mix=%d", stereo_mix);
    return RULE_ACTION_RUN_PARAM(stereo_mix);
}

/*! @brief Rule to select the audio mix to be rendered by the local earbud */
static rule_action_t ruleSelectRemoteAudioMix(void)
{
    bool stereo_mix = TRUE;

    if (appPeerSigIsConnected())
    {
        if (appSmIsInEar())
        {
            stereo_mix = FALSE;
        }
        PRIMARY_RULE_LOG("ruleSelectRemoteAudioMix stereo_mix=%d", stereo_mix);
        return RULE_ACTION_RUN_PARAM(stereo_mix);
    }
    else
    {
        return rule_action_ignore;
    }
}

static rule_action_t ruleDetermineIfEitherEarbudIsInEar(void)
{
    bool isEitherEarbudInEar = FALSE;
    if (StateProxy_IsPeerInEar() || appSmIsInEar())
    {
        isEitherEarbudInEar = TRUE;
    }
    PRIMARY_RULE_LOG("ruleDetermineIfEitherEarbudIsInEar %d", isEitherEarbudInEar);
    return RULE_ACTION_RUN_PARAM(isEitherEarbudInEar);
}

/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Initialise the primary rules module. */
bool PrimaryRules_Init(Task init_task)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    rule_set_init_params_t rule_params;

    UNUSED(init_task);

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = primary_rules_set;
    rule_params.rules_count = ARRAY_DIM(primary_rules_set);
    rule_params.nop_message_id = CONN_RULES_NOP;
    rule_params.event_task = SmGetTask();
    primary_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);

    return TRUE;
}

rule_set_t PrimaryRules_GetRuleSet(void)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    return primary_rules->rule_set;
}

void PrimaryRules_SetEvent(rule_events_t event_mask)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    RulesEngine_SetEvent(primary_rules->rule_set, event_mask);
}

void PrimaryRules_ResetEvent(rule_events_t event)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    RulesEngine_ResetEvent(primary_rules->rule_set, event);
}

rule_events_t PrimaryRules_GetEvents(void)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    return RulesEngine_GetEvents(primary_rules->rule_set);
}

void PrimaryRules_SetRuleComplete(MessageId message)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    RulesEngine_SetRuleComplete(primary_rules->rule_set, message);
}

void PrimaryRulesSetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(primary_rules->rule_set, message, event);
}

/*! \brief Decide if primary earbud should transfer sco audio back to handset or not. */
rule_action_t PrimaryRuleTransferScoToHandsetIfOutOfEar(void)
{
    if (MirrorProfile_IsEscoActive() && StateProxy_IsPeerInEar())
    {
        PRIMARY_RULE_LOG("PrimaryRuleTransferScoToHandsetIfOutOfEar,"
                         " ignore as we have peer sco running and peer is in ear");
        return rule_action_ignore;
    }

    if (HfpProfile_IsScoActive() && appSmIsOutOfEar())
    {
        PRIMARY_RULE_LOG("PrimaryRuleTransferScoToHandsetIfOutOfEar,"
                         " run as SCO is active and earbud out of ear");
        return rule_action_run;
    }

    PRIMARY_RULE_LOG("PrimaryRuleTransferScoToHandsetIfOutOfEar,"
                     " ignore as SCO not active out of ear");
    return rule_action_ignore;
}

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
 */
static rule_action_t PrimaryRulesCopyRunParams(const void* param, size_t size_param)
{
    PrimaryRulesTaskData *primary_rules = PrimaryRulesGetTaskData();
    RulesEngine_CopyRunParams(primary_rules->rule_set, param, size_param);
    return rule_action_run_with_param;
}
