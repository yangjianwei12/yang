/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Rules to control the TWS topology target state.
*/

#include "tws_topology_sm.h"
#include "tws_topology_private.h"

#include <hdma.h>
#include <mirror_profile.h>
#include <bandwidth_manager.h>
#include <state_proxy.h>
#include <device_db_serialiser_assert.h>
#include <charger_monitor.h>

#include <logging.h>

#define IGNORE FALSE
#define RUN TRUE

static bool twsTopology_IsTopologySmActive(void)
{
    return (TwsTopology_GetSm()->target_state != TWS_TOPOLOGY_STATE_IDLE);
}

/*! \brief The SM rule that decides when the earbud should pair with its peer.
    \return TRUE if rule decides SM should pair with peer.
*/
static bool twsTopology_IsPairWithPeerRequired(void)
{
    bdaddr bd_addr_primary, bd_addr_secondary;

    /* Both Primary and Secondary Addresses should exist if peer-pairing already occurred */
    if (appDeviceGetPrimaryBdAddr(&bd_addr_primary) && appDeviceGetSecondaryBdAddr(&bd_addr_secondary) &&
        !BdaddrIsSame(&bd_addr_primary, &bd_addr_secondary))
    {
        DEBUG_LOG_INFO("twsTopology_IsPairWithPeerRequired, ignore as already paired with peer");
        return IGNORE;
    }
    else if (appDeviceGetPrimaryBdAddr(&bd_addr_primary) || appDeviceGetSecondaryBdAddr(&bd_addr_secondary))
    {
        DEBUG_LOG_INFO("twsTopology_IsPairWithPeerRequired, run as only primary or secondary exists");
        DeviceDbSerialiser_HandleCorruption();
        return RUN;
    }
    else
    {
        DEBUG_LOG_INFO("twsTopology_IsPairWithPeerRequired, run");
        return RUN;
    }
}

/*! \brief The SM rule that decides when the earbud should become idle.
    \return TRUE if rule decides SM should become idle.
*/
static bool twsTopology_IsDeviceInactive(void)
{
    bool become_idle = FALSE;

#ifdef ENABLE_SKIP_PFR
    if(!twsTopology_IsTopologySmActive() && !appIsKeepTopologyAliveForStaticHandoverEnabled())
    {
        become_idle = TRUE;
    }
#else
    if(!twsTopology_IsTopologySmActive())
    {
        become_idle = TRUE;
    }
#endif
    DEBUG_LOG("twsTopology_RuleBecomeIdle: %s", become_idle ? "TRUE" : "FALSE");
    return become_idle;
}

/*! \brief The SM rule that decides when the primary earbud should enable handover.
    The actual decision on when to handover is decided by the handover decision
    making algorithm (HDMA).
    \return TRUE if rule decides primary role SM should enable handover.
*/
static bool twsTopology_IsHandoverPossible(void)
{
    bool enableHandover = FALSE;
    if (!TwsTopologyConfig_DynamicHandoverSupported())
    {
        DEBUG_LOG("twsTopology_RuleIsHandoverPossible ignore - handover not supported");
    }
    else if (TwsTopology_IsRoleSwapSupported() == FALSE)
    {
        DEBUG_LOG("twsTopology_RuleIsHandoverPossible ignore - app prohibited handover");
    }
    else if (PeerFindRole_HasFixedRole())
    {
        DEBUG_LOG("twsTopology_RuleHandoverEnable ignore - using fixed role");
    }
    else
    {
        bool peer_connected = appDeviceIsPeerConnected();
        bool state_proxy_rx = StateProxy_InitialStateReceived();

        if (peer_connected && state_proxy_rx)
        {
            DEBUG_LOG("twsTopology_RuleIsHandoverPossible run");
            enableHandover = TRUE;
        }
        else
        {
            DEBUG_LOG("twsTopology_RuleIsHandoverPossible ignore - peer %u stateproxy %u",
                      peer_connected, state_proxy_rx);
        }
    }
    return enableHandover;
}

#ifdef ENABLE_SKIP_PFR
/*! \brief The SM rule that decides when the primary earbud should start static handover.
    \return TRUE if rule decides SM should start static handover.
*/
static bool twsTopology_RuleStaticHandoverStart(void)
{
    bool peer_connected = appDeviceIsPeerConnected();
    bool state_proxy_rx = StateProxy_InitialStateReceived();

    if (!peer_connected || !state_proxy_rx || TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG("twsTopology_RuleStaticHandoverStart, ignore as absent or ongoing peer connection");
        return IGNORE;
    }

    if (appPhyStateGetState() != PHY_STATE_IN_CASE || !StateProxy_IsPeerOutOfCase())
    {
        DEBUG_LOG("twsTopology_RuleStaticHandoverStart, ignore as local is not in case or remote is not out-of-case");
        return IGNORE;
    }
  /*  else if((CcWithCase_EventsEnabled() && CcWithCase_GetLidState() == CASE_LID_STATE_OPEN) &&
            MessagePendingFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_SECONDARY_OUT_OF_CASE_TIMEOUT, NULL))
    {
        DEBUG_LOG("twsTopology_RuleStaticHandoverStart, ignore as we're in the case but the lid is open");
        return IGNORE;
    } */

    DEBUG_LOG_INFO("twsTopology_RuleStaticHandoverStart, run");
    return RUN;
}

static bool twsTopology_RuleSelectPreservedRole(void)
{
    tws_topology_sm_t *sm = TwsTopology_GetSm();

    if (sm->state != TWS_TOPOLOGY_STATE_IDLE && sm->state != TWS_TOPOLOGY_STATE_STARTING &&
                                                sm->state != TWS_TOPOLOGY_STATE_STARTED)
    {
        DEBUG_LOG("twsTopology_RuleSelectPreservedRole, ignore as the state is not IDLE");
        return IGNORE;
    }

    if (sm->target_state != TWS_TOPOLOGY_STATE_IDLE && sm->target_state != TWS_TOPOLOGY_STATE_STOPPED &&
           sm->target_state != TWS_TOPOLOGY_STATE_STARTING && sm->target_state != TWS_TOPOLOGY_STATE_STARTED &&
           sm->target_state != TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE)
    {
        DEBUG_LOG("twsTopology_RuleSelectPreservedRole, ignore as target state is not IDLE");
        return IGNORE;
    }

    if (PeerFindRole_GetPreservedRole() == peer_find_role_preserved_role_invalid)
    {
        DEBUG_LOG("twsTopology_RuleSelectPreservedRole, ignore as preserved role is invalid");
        return IGNORE;
    }

    if(Charger_IsChargerConnectedAtBoot() == FALSE)
    {
        DEBUG_LOG("twsTopology_RuleSelectPreservedRole, ignore as earbud booted not due to Lid open");
        return IGNORE;
    }

    /* Clear the charger detected at boot flag, so that PFR will be used if the device boots when earbud
     * is out of case. This flag will be set again if device boots due to lid open */
     Charger_ClearChargerConnectedAtBootFlag();

    DEBUG_LOG("twsTopology_RuleSelectPreservedRole, run");
    return RUN;
}
#endif

/*! \brief The SM rule that decides when the primary earbud should start handover.
    \param reason The current reason provided by hdma.
    \return TRUE if rule decides SM should start handover.
*/
static bool twsTopology_IsHandoverAuthorised(hdma_handover_reason_t handover_reason)
{
    PanicFalse(handover_reason != HDMA_HANDOVER_REASON_INVALID);
    return twsTopology_GetProductBehaviour()->authoriseStartRoleSwap(handover_reason);
}

static inline bool twsTopoology_IsHandoverRequestPending(tws_topology_sm_t *sm)
{
    return (sm->handover_reason != HDMA_HANDOVER_REASON_INVALID);
}

static inline bool twsTopoology_IsJoinRequestPending(tws_topology_sm_t *sm)
{
    return sm->pending_app_event == TOPOLOGY_APP_EVENT_JOIN_REQUEST;
}

void twsTopology_SetTargetState(tws_topology_sm_t *sm)
{
    tws_topology_state_t target;

    switch (sm->elected_role)
    {
        case tws_topology_elected_role_none:
            if (twsTopology_IsPairWithPeerRequired())
            {
                target = TWS_TOPOLOGY_STATE_PEER_PAIRING;
            }
            else if(twsTopoology_IsJoinRequestPending(sm))
            {
#ifdef ENABLE_SKIP_PFR
                    if (twsTopology_RuleSelectPreservedRole())
                    {
                        target = TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE;
                    }
                    else
                    {
                        target = TWS_TOPOLOGY_STATE_FIND_ROLE;
                    }
#else
                target = TWS_TOPOLOGY_STATE_FIND_ROLE;
#endif
            }
            else
            {
                target = TWS_TOPOLOGY_STATE_IDLE;
            }
        break;

        case tws_topology_elected_role_standalone_primary:
            target = TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY;
            if (twsTopology_IsDeviceInactive())
            {
                target = TWS_TOPOLOGY_STATE_IDLE;
            }
        break;

        case tws_topology_elected_role_primary_with_peer:
            target = TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER;

            if (twsTopoology_IsHandoverRequestPending(sm) &&
                twsTopology_IsHandoverPossible() &&
                twsTopology_IsHandoverAuthorised(sm->handover_reason))
            {
                target = TWS_TOPOLOGY_STATE_SECONDARY;
            }
#ifdef ENABLE_SKIP_PFR
            else if (twsTopology_RuleStaticHandoverStart())
            {
                target = TWS_TOPOLOGY_STATE_STATIC_HANDOVER;
            }
#endif
            else if (twsTopology_IsDeviceInactive())
            {
                target = TWS_TOPOLOGY_STATE_IDLE;
            }
        break;

        case tws_topology_elected_role_secondary:
            target = TWS_TOPOLOGY_STATE_SECONDARY;
            if (twsTopology_IsDeviceInactive())
            {
                target = TWS_TOPOLOGY_STATE_IDLE;
            }
        break;

        default:
            Panic();
            target = TWS_TOPOLOGY_STATE_IDLE;
        break;
    }

    if (target != sm->target_state)
    {
        DEBUG_LOG_STATE("/'twsTopology_SetTargetState'/ "
                        "enum:tws_topology_state_t:%d --> "
                        "enum:tws_topology_state_t:%d",
                        sm->target_state, target);

        sm->target_state = target;
    }
}

