/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Implementation of state machine transitions for the TWS topology.
*/

#include <logging.h>
#include <panic.h>
#include <watchdog.h>

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_client_msgs.h"
#include "tws_topology_advertising.h"
#include "tws_topology_config.h"
#include "tws_topology_sm.h"
#include "tws_topology_goals.h"
#include "tws_topology_peer_sig.h"

#include "connection_manager.h"
#include "mirror_profile.h"

#define CHECK_STABLE_STATE_RETRY_MILLISECS      100
#define MAX_CHECK_STABLE_STATE_MILLISECS        1500

/*! \brief Function pointer type handling state entry.
    \param sm The sm.
    \param arg Optional value that may be passed to the entry function.
*/
typedef void (*twsTopology_SmEnterState)(tws_topology_sm_t *sm, unsigned arg);

/*! \brief Function pointer type handling state exit.
    \param sm The sm.
    \param arg Optional value that may be passed to the exit function.
*/
typedef void (*twsTopology_SmExitState)(tws_topology_sm_t *sm, unsigned arg);

/*! \brief Function pointer type handling goal completion.
    \param sm The sm.
    \param result The result of the goal.
    \return The SM's next state.
*/
typedef tws_topology_state_t (*twsTopology_SmOnGoalComplete)(tws_topology_sm_t *sm, procedure_result_t result);

/*! Structure to store each SM state's entry and exit function config */
typedef struct
{
    /*! Function to call on exiting state. */
    twsTopology_SmExitState exit;
    /*! Argument to pass to exit state function. */
    unsigned exit_arg;
    /*! Function to call on entering state. */
    twsTopology_SmEnterState enter;
    /*! Argument to pass to enter state function. */
    unsigned enter_arg;
} tws_topology_sm_state_config_t;

static tws_topology_state_t twsTopology_SmGetNextState(tws_topology_state_t current, tws_topology_state_t target);
static void twsTopology_SetState(tws_topology_sm_t *sm, tws_topology_state_t new_state);
static void twsTopology_SetElectedRole(tws_topology_sm_t *sm, tws_topology_elected_role_t new_elected_role);
static void twsTopology_RelinquishElectedRole(tws_topology_sm_t *sm);
static void twsTopology_WatchdogKick(unsigned timeout_seconds);
static void twsTopology_WatchdogStop(unsigned timeout_seconds);
static void twsTopology_StartHdma(tws_topology_sm_t *sm);
static void twsTopology_StopHdma(tws_topology_sm_t *sm);
static void twsTopology_MoveToIdleStateIfRequired(tws_topology_sm_t *sm);
static void twsTopology_LeaveAsPrimary(void);
static bool twsTopology_IsTopologyInStableStateToLeave(tws_topology_sm_t *sm);

static void twsTopology_SmSetGoal(tws_topology_sm_t *sm, unsigned id);
static void twsTopology_EnterStopped(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterStarted(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterHandoverPrepared(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterPrimaryWithPeer(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterSecondary(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterStandalonePrimary(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterIdle(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_ExitIdle(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterHandoverRetry(tws_topology_sm_t *sm, unsigned delay);
static void twsTopology_ExitHandoverRetry(tws_topology_sm_t *sm, unsigned delay);
#ifdef ENABLE_SKIP_PFR
static void twsTopology_EnableRoleSelectedByPeerFindRole(tws_topology_sm_t *sm, bool enable);
static void twsTopology_EnterStaticHandover(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_ExitStaticHandover(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_EnterSelectPreservedRole(tws_topology_sm_t *sm, unsigned dummy);
static void twsTopology_ExitSelectPreservedRole(tws_topology_sm_t *sm, unsigned dummy);
#endif

static tws_topology_state_t twsTopology_SmHandleGoalPairPeerComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalNoRoleIdleComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalSecondaryConnectPeerComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalPrimaryConnectablePeerComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalPrimaryConnectPeerProfilesComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverPrepareComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverUndoPrepareComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalBecomeStandalonePrimaryComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalBecomePrimaryComplete(tws_topology_sm_t *sm, procedure_result_t result);
static tws_topology_state_t twsTopology_SmHandleGoalBecomeSecondaryComplete(tws_topology_sm_t *sm, procedure_result_t result);

inline static void twsTopology_ResetExternalAppEvent(tws_topology_sm_t *sm);
inline static void twsTopology_SwapRole(tws_topology_sm_t *sm, topology_app_event_t app_event);
inline static void twsTopology_SmSecondaryLinkLoss(tws_topology_sm_t *sm);
inline static bool twsTopology_IsDeviceTypeStandalone(void);

#ifdef ENABLE_SKIP_PFR
static inline void appEnableKeepTopologyAliveForStaticHandover(void);
static inline void appDisableKeepTopologyAliveForStaticHandover(void);

bool keep_topology_alive_for_static_handover = FALSE;
#endif

/*! This array defines the enter/exit functions for each SM state */
static const tws_topology_sm_state_config_t state_config[TWS_TOPOLOGY_STATES_END] =
    {
        [TWS_TOPOLOGY_STATE_PEER_PAIRING] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_pair_peer},
        [TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_primary_connectable_peer},
        [TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_primary_connectable_peer},
        [TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_become_standalone_primary},
        [TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_primary_connect_peer_profiles},
        [TWS_TOPOLOGY_STATE_HANDOVER] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_dynamic_handover},
        [TWS_TOPOLOGY_STATE_HANDOVER_PREPARE] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_dynamic_handover_prepare},
        [TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_dynamic_handover_undo_prepare},
        [TWS_TOPOLOGY_STATE_BECOME_SECONDARY] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_become_secondary},
        [TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_secondary_connect_peer},
        [TWS_TOPOLOGY_STATE_BECOME_IDLE] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_no_role_idle},
        [TWS_TOPOLOGY_STATE_FIND_ROLE] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_find_role},
        [TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY] = {NULL, 0, twsTopology_SmSetGoal, tws_topology_goal_primary_connectable_peer},
        [TWS_TOPOLOGY_STATE_HANDOVER_RETRY] = {twsTopology_ExitHandoverRetry, 0, twsTopology_EnterHandoverRetry, 0},
        [TWS_TOPOLOGY_STATE_STOPPED] = {NULL, 0, twsTopology_EnterStopped, 0},
        [TWS_TOPOLOGY_STATE_STARTED] = {NULL, 0, twsTopology_EnterStarted, 0},
        [TWS_TOPOLOGY_STATE_HANDOVER_PREPARED] = {NULL, 0, twsTopology_EnterHandoverPrepared, 0},
        [TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER] = {NULL, 0, twsTopology_EnterPrimaryWithPeer, 0},
        [TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY] = {NULL, 0, twsTopology_EnterStandalonePrimary, 0},
        [TWS_TOPOLOGY_STATE_SECONDARY] = {NULL, 0, twsTopology_EnterSecondary, 0},
        [TWS_TOPOLOGY_STATE_IDLE] = {twsTopology_ExitIdle, 0, twsTopology_EnterIdle, 0},
#ifdef ENABLE_SKIP_PFR
        [TWS_TOPOLOGY_STATE_STATIC_HANDOVER] = {twsTopology_ExitStaticHandover, 0, twsTopology_EnterStaticHandover, 0},
        [TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE] = {twsTopology_ExitSelectPreservedRole, 0, twsTopology_EnterSelectPreservedRole, 0},
#endif
};

/*! This array defines the goal complete handlers for each goal */
static const twsTopology_SmOnGoalComplete on_goal_complete_handlers[tws_topology_goals_end] =
    {
        [tws_topology_goal_pair_peer] = twsTopology_SmHandleGoalPairPeerComplete,
        [tws_topology_goal_no_role_idle] = twsTopology_SmHandleGoalNoRoleIdleComplete,
        [tws_topology_goal_secondary_connect_peer] = twsTopology_SmHandleGoalSecondaryConnectPeerComplete,
        [tws_topology_goal_primary_connectable_peer] = twsTopology_SmHandleGoalPrimaryConnectablePeerComplete,
        [tws_topology_goal_primary_connect_peer_profiles] = twsTopology_SmHandleGoalPrimaryConnectPeerProfilesComplete,
        [tws_topology_goal_dynamic_handover_prepare] = twsTopology_SmHandleGoalDynamicHandoverPrepareComplete,
        [tws_topology_goal_dynamic_handover] = twsTopology_SmHandleGoalDynamicHandoverComplete,
        [tws_topology_goal_dynamic_handover_undo_prepare] = twsTopology_SmHandleGoalDynamicHandoverUndoPrepareComplete,
        [tws_topology_goal_become_standalone_primary] = twsTopology_SmHandleGoalBecomeStandalonePrimaryComplete,
        [tws_topology_goal_become_primary_with_peer] = twsTopology_SmHandleGoalBecomePrimaryComplete,
        [tws_topology_goal_become_secondary] = twsTopology_SmHandleGoalBecomeSecondaryComplete,
};

CREATE_WATCHDOG(topology_watchdog);

void twsTopology_SmInit(tws_topology_sm_t *sm)
{
    sm->state = TWS_TOPOLOGY_STATE_STOPPED;
    sm->goal_task.handler = TwsTopology_HandleGoalDecision;
    TwsTopology_GoalsInit();
}

void twsTopology_SmStart(tws_topology_sm_t *sm)
{
    sm->state = TWS_TOPOLOGY_STATE_STARTING;
    twsTopology_SmEnqueueKick(sm);
}

void twsTopology_SmStop(tws_topology_sm_t *sm)
{
    uint32 timeout_ms = D_SEC(twsTopology_GetProductBehaviour()->timeouts.send_stop_cmd_sec);

    TwsTopology_SendStopCompletedMsg(tws_topology_status_success);

    if (timeout_ms)
    {
        MessageSendLater(TwsTopologyGetTask(),
                         TWSTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP, NULL, timeout_ms);
    }
    sm->target_state = TWS_TOPOLOGY_STATE_STOPPED;
    twsTopology_SmEnqueueKick(sm);
}

void twsTopology_SmKick(tws_topology_sm_t *sm)
{
    if (twsTopology_StateIsSteady(sm->state))
    {
        MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_KICK_SM);

        twsTopology_ResetExternalAppEvent(sm);
        twsTopology_SetTargetState(sm);
        if (sm->target_state == TWS_TOPOLOGY_STATE_IDLE ||
            sm->target_state == TWS_TOPOLOGY_STATE_STOPPED)
        {
            /*! Idle target requires a new role election */
            twsTopology_RelinquishElectedRole(sm);
        }

        
        if (sm->target_state != sm->state)
        {
            tws_topology_state_t old_state = sm->state;
            tws_topology_state_t new_state = twsTopology_SmGetNextState(old_state, sm->target_state);
            twsTopology_SetState(sm, new_state);
        }
        
        if(sm->initial_idle_notification_sent == FALSE && sm->state == TWS_TOPOLOGY_STATE_IDLE)
        {
            PanicFalse(sm->elected_role == tws_topology_role_none);
            sm->initial_idle_notification_sent = TRUE;
            TwsTopology_SendInitialIdleCompletedMsg();
        }
    }
}

void twsTopology_SmEnqueueKick(tws_topology_sm_t *sm)
{
    UNUSED(sm);
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_KICK_SM);
    MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_KICK_SM, NULL);
}

void twsTopology_SmHandlePeerLinkDisconnected(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandlePeerLinkDisconnected enum:tws_topology_elected_role_t:%d", sm->elected_role);
    switch (sm->elected_role)
    {
        case tws_topology_elected_role_primary_with_peer:
#ifdef ENABLE_SKIP_PFR
            if (sm->state == TWS_TOPOLOGY_STATE_STATIC_HANDOVER)
            {
                /* We assume the Secondary has disconnected the link to complete the handover.
                 * So we can now assume our new Secondary role. */
                twsTopology_SmHandleStaticHandoverToSecondary(sm);
            }
            else
            {
                twsTopology_SmHandleElectedStandalonePrimary(sm);
            }
#else
            // Potential race condition where leave occurs just before link loss message
            if(TwsTopology_GetJoinStatus())
            {
                twsTopology_SmHandleElectedStandalonePrimary(sm);
            }
            else
            {
               twsTopology_ForceIdleState(sm);
            }
#endif
        break;

        case tws_topology_elected_role_secondary:
            twsTopology_SmSecondaryLinkLoss(sm);
        break;

        default:
        break;
    }
}

void twsTopology_SmHandleElectedStandalonePrimary(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleElectedStandalonePrimary");
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_standalone_primary);
    twsTopology_SmKick(sm);
}

void twsTopology_SmHandleElectedPrimaryWithPeer(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleElectedPrimary");
#ifdef ENABLE_SKIP_PFR
    PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_primary);
#endif
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_primary_with_peer);
    twsTopology_SmKick(sm);
}

void twsTopology_SmHandleElectedSecondary(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleElectedSecondary");
#ifdef ENABLE_SKIP_PFR
    PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_secondary);
    twsTopology_EnableRoleSelectedByPeerFindRole(sm, TRUE);
#endif
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_secondary);
    twsTopology_SmKick(sm);
}

void twsTopology_SmHandleHandoverToPrimary(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleHandoverToPrimary");
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_primary_with_peer);
    twsTopology_SetTargetState(sm);
    twsTopology_SetState(sm, TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY);
}

void twsTopology_SmHandleStaticHandoverToSecondary(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleStaticHandoverToSecondary");
    PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_secondary);
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_secondary);
    twsTopology_SetState(sm, TWS_TOPOLOGY_STATE_BECOME_IDLE);
}

void twsTopology_SmHandleStaticHandoverToPrimary(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleStaticHandoverToPrimary");
    PeerFindRole_SetPreservedRoleInGlobalVariable(peer_find_role_preserved_role_primary);
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_primary_with_peer);
    twsTopology_SmKick(sm);
}

#ifdef ENABLE_SKIP_PFR
void twsTopology_SmHandlePostIdleStaticHandoverTimeout(tws_topology_sm_t *sm)
{
    UNUSED(sm);

    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandlePostIdleStaticHandoverTimeout");
    /* Disable KeepTopologyAlive flag and allow the peer EB to handle the close lid event */
    appDisableKeepTopologyAliveForStaticHandover();
}
#endif

void twsTopology_SmHandleInternalRetryHandover(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleInternalRetryHandover");
    switch (sm->state)
    {
        case TWS_TOPOLOGY_STATE_HANDOVER_RETRY:
            twsTopology_SetState(sm, TWS_TOPOLOGY_STATE_HANDOVER);
        break;
        default:
            Panic();
        break;
    }
}

void twsTopology_SmHandleInternalStopTimeout(tws_topology_sm_t *sm)
{
    UNUSED(sm);
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleInternalStopTimeout");
    TwsTopology_SendStopCompletedMsg(tws_topology_status_fail);
}

void twsTopology_SmHandleInternalJoinRequest(tws_topology_sm_t *sm)
{
    twsTopology_ExecuteJoinActions(sm);
    TwsTopology_ClearLeaveStatusWaitingTxToPeer();
    if(!twsTopology_IsDeviceTypeStandalone())
    {
        TwsTopology_SetJoinStatusWaitingTxToPeer();
    }
    TwsTopology_SendJoinRequestCompletedMsg();
}

void twsTopology_SmHandleInternalLeaveRequest(tws_topology_sm_t *sm)
{
    TwsTopology_ClearJoinStatus();
    TwsTopology_PeerSignalTopologyCmd(peer_left_topology);

    if(appDeviceIsPeerConnected())
    {
        // If we have a peer connection then delay executing leave commands until cfm from peer signalling has been rx'd
        sm->topology_status.pending_peer_send.leave = TRUE;
    }
    else
    {
        twsTopology_ExecuteLeaveActions(sm);
    }

}

void twsTopology_SmHandleInternalSwapRoleRequest(tws_topology_sm_t *sm)
{
    twsTopology_SwapRole(sm, TOPOLOGY_APP_EVENT_NONE);
}

void twsTopology_SmHandleInternalSwapRoleAndDisconnectRequest(tws_topology_sm_t *sm)
{
    twsTopology_SwapRole(sm, TOPOLOGY_APP_EVENT_SWAP_ROLE_AND_DISCONNECT);
}

void twsTopology_SmHandleHDMARequest(tws_topology_sm_t *sm, hdma_handover_decision_t* message)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleHDMARequest");
    if(TwsTopology_IsRoleSwapSupported())
    {
        sm->handover_reason = message->reason;
        twsTopology_SmKick(sm);
    }
}

void twsTopology_SmHandleHDMACancelHandover(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_SmHandleHDMACancelHandover");
    sm->handover_reason = HDMA_HANDOVER_REASON_INVALID;
    twsTopology_SmKick(sm);
}

void twsTopology_SmGoalComplete(tws_topology_sm_t *sm, tws_topology_goal_id completed_goal, procedure_result_t result)
{
    twsTopology_SmOnGoalComplete handler = on_goal_complete_handlers[completed_goal];
    if (handler)
    {
        tws_topology_state_t next_state = handler(sm, result);
        twsTopology_SetState(sm, next_state);
    }
}

void twsTopology_ExecuteJoinActions(tws_topology_sm_t *sm)
{
    TwsTopology_SetJoinStatus();

    if(sm->elected_role == tws_topology_elected_role_none)
    {
        DEBUG_LOG_ERROR("twsTopology_ExecuteJoinActions: trigger peer connect procedure");
        sm->pending_app_event = TOPOLOGY_APP_EVENT_JOIN_REQUEST;
        twsTopology_SmEnqueueKick(sm);
    }
    else if(TwsTopology_IsRoleStandAlonePrimary() && !PeerFindRole_IsActive())
    {
        // This is required as when the primary leaves topology (without a peer connection)
        // the primary will then block any future attempts by peer to connect. It does this
        // until the primary rejoins topology
        PeerFindRole_FindRole(D_SEC(twsTopology_GetProductBehaviour()->timeouts.peer_find_role_sec));
    }
}

void twsTopology_ExecuteLeaveActions(tws_topology_sm_t *sm)
{
    if(!twsTopology_IsTopologyInStableStateToLeave(sm))
    {
        MessageSendLater(TwsTopologyGetTask(), TWSTOP_INTERNAL_LEAVE_REQUEST, 0, CHECK_STABLE_STATE_RETRY_MILLISECS);
    }
    else
    {
        sm->pending_app_event = TOPOLOGY_APP_EVENT_NONE;
        if(TwsTopology_IsRolePrimary())
        {
            twsTopology_LeaveAsPrimary();
        }
        twsTopology_MoveToIdleStateIfRequired(sm);
        TwsTopology_SendLeaveRequestCompletedMsg();
    }
}

void twsTopology_ForceIdleState(tws_topology_sm_t *sm)
{
    DEBUG_LOG_FN_ENTRY("twsTopology_ForceIdleState");
    twsTopology_RelinquishElectedRole(sm);
    twsTopology_SmKick(sm);
}

/*! \brief Relinquish the currently elected role.

    The SM sometimes makes a decision or handles a failure that requires a new
    role to be elected. This function relinquishes the last elected role by
    setting the elected_role back to none. This will force the SM to transition
    back through the candidate state to obtain a new role because the target
    state is always updated before the SM transitions away from a steady state,
    and the elected_role influences the target state. */
static void twsTopology_RelinquishElectedRole(tws_topology_sm_t *sm)
{
    sm->handover_reason = HDMA_HANDOVER_REASON_INVALID;
    sm->pending_app_event = TOPOLOGY_APP_EVENT_NONE;
    twsTopology_SetElectedRole(sm, tws_topology_elected_role_none);
}

/*! \brief Set the elected role. */
static void twsTopology_SetElectedRole(tws_topology_sm_t *sm, tws_topology_elected_role_t new_elected_role)
{
    DEBUG_LOG_STATE("twsTopology_SetElectedRole "
                    "enum:tws_topology_elected_role_t:%d ->"
                    "enum:tws_topology_elected_role_t:%d",
                    sm->elected_role, new_elected_role);
    sm->elected_role = new_elected_role;

#ifdef ENABLE_SKIP_PFR
    if (new_elected_role != tws_topology_elected_role_primary_with_peer &&
        new_elected_role != tws_topology_elected_role_secondary)
    {
        if (MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT))
        {
            DEBUG_LOG("twsTopology_SetElectedRole, cancelling window timeout");
            appDisableKeepTopologyAliveForStaticHandover();
        }
    }
#endif
}

/*! \brief Kick the watchdog.
    \param timeout_seconds The watchdog timeout
*/
static void twsTopology_WatchdogKick(unsigned timeout_seconds)
{
    if (timeout_seconds)
    {
        Watchdog_Kick(&topology_watchdog, timeout_seconds);
    }
}

/*! \brief Stop the watchdog.
    \param timeout_seconds The watchdog timeout
*/
static void twsTopology_WatchdogStop(unsigned timeout_seconds)
{
    if (timeout_seconds)
    {
        Watchdog_Stop(&topology_watchdog);
    }
}

/*! \brief Start the handover decision making algorithm */
static void twsTopology_StartHdma(tws_topology_sm_t *sm)
{
    sm->handover_reason = HDMA_HANDOVER_REASON_INVALID;
    Hdma_Init(TwsTopologyGetTask());

#ifdef ENABLE_SKIP_PFR
    /* HDMA has been started, so static handover is no longer needed */
    if (MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT))
    {
        DEBUG_LOG_INFO("twsTopology_StartHdma, clear the remain active peer flag of static handover");
        appDisableKeepTopologyAliveForStaticHandover();
    }
#endif
}

/*! \brief Stop the handover decision making algorithm */
static void twsTopology_StopHdma(tws_topology_sm_t *sm)
{
    sm->handover_reason = HDMA_HANDOVER_REASON_INVALID;
    Hdma_Destroy();
    MessageCancelAll(TwsTopologyGetTask(), HDMA_HANDOVER_NOTIFICATION);
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_STOPPED state */
static void twsTopology_EnterStopped(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP);
}

/*! \brief Exit the TWS_TOPOLOGY_STATE_STOPPED state */
static void twsTopology_EnterStarted(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    TwsTopology_SendStartCompletedMsg(tws_topology_status_success);
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_HANDOVER_PREPARED state */
static void twsTopology_EnterHandoverPrepared(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);

    sm->handover_failed = FALSE;

    /* start the handover window */
    MessageSendLater(TwsTopologyGetTask(),
                     TWSTOP_INTERNAL_HANDOVER_WINDOW_TIMEOUT,
                     NULL,
                     D_SEC(twsTopology_GetProductBehaviour()->timeouts.max_handover_window_sec));
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER state */
static void twsTopology_EnterPrimaryWithPeer(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    twsTopology_UpdateAdvertisingParams();
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_SECONDARY state */
static void twsTopology_EnterSecondary(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY state */
static void twsTopology_EnterStandalonePrimary(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    twsTopology_UpdateAdvertisingParams();
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_IDLE state */
static void twsTopology_EnterIdle(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);

    sm->handover_failed = FALSE;

    twsTopology_WatchdogKick(twsTopology_GetProductBehaviour()->timeouts.reset_device_sec);
}

/*! \brief Exit the TWS_TOPOLOGY_STATE_IDLE state */
static void twsTopology_ExitIdle(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    twsTopology_WatchdogStop(twsTopology_GetProductBehaviour()->timeouts.reset_device_sec);
}

/*! \brief Enter the TWS_TOPOLOGY_STATE_HANDOVER_RETRY state */
static void twsTopology_EnterHandoverRetry(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    MessageSendLater(TwsTopologyGetTask(), TWSTOP_INTERNAL_RETRY_HANDOVER, NULL, twsTopology_GetProductBehaviour()->timeouts.handover_retry_ms);
}

/*! \brief Exit the TWS_TOPOLOGY_STATE_HANDOVER_RETRY state */
static void twsTopology_ExitHandoverRetry(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
    MessageCancelFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_RETRY_HANDOVER);
}

inline static bool twsTopology_IsDeviceTypeStandalone(void)
{
    return twsTopology_GetProductBehaviour()->device_type == topology_device_type_standalone;
}

inline static void twsTopology_SmSecondaryLinkLoss(tws_topology_sm_t *sm)
{
    // There is a potential race condition where leave is processed before link loss on secondary
    // so need to check join status before taking action
    if(TwsTopology_GetJoinStatus())
    {
        // Primary left without handover, in which case secondary should become Standalone primary
        twsTopology_TransitionSecondaryToStandalonePrimary(sm);
    }
}

inline static void twsTopology_DisconnectPeer(void)
{
    bdaddr bd_addr;
    if(appDeviceGetPeerBdAddr(&bd_addr))
    {
        ConManagerSendCloseAclRequest(&bd_addr, TRUE);
    }
    else
    {
        DEBUG_LOG_ERROR("twsTopology_DisconnectPeer: NO PEER");
    }
}

inline static void twsTopology_ExecuteStateExitFunction(tws_topology_sm_t *sm, tws_topology_state_t current_state)
{
    const tws_topology_sm_state_config_t *state_config_entry = &state_config[current_state];
    if (state_config_entry->exit)
    {
        state_config_entry->exit(sm, state_config_entry->exit_arg);
    }
}

inline static void twsTopology_ExecuteStateEntryFunction(tws_topology_sm_t *sm, tws_topology_state_t current_state, tws_topology_state_t new_state)
{
    const tws_topology_sm_state_config_t *state_config_entry = &state_config[new_state];
    if (state_config_entry->enter)
    {
        state_config_entry->enter(sm, state_config_entry->enter_arg);
    }

    tws_topology_role new_role = twsTopology_RoleFromState(new_state);
    tws_topology_role old_role = twsTopology_RoleFromState(current_state);
    if (old_role != new_role)
    {
        TwsTopology_SendRoleChangedCompletedMsg(new_role, sm->error_forced_no_role);
        sm->error_forced_no_role = FALSE;
    }

    if (twsTopology_StateIsSteady(new_state))
    {
        twsTopology_SmEnqueueKick(sm);
    }
}

inline static void twsTopology_ManageHdmaOperation(tws_topology_sm_t *sm, tws_topology_state_t current_state, tws_topology_state_t new_state)
{
    bool new_state_requires_hdma = twsTopology_StateRequiresHdma(new_state);
    bool old_state_requires_hdma = twsTopology_StateRequiresHdma(current_state);

    if (old_state_requires_hdma && !new_state_requires_hdma)
    {
        twsTopology_StopHdma(sm);
    }

    else if (new_state_requires_hdma && !old_state_requires_hdma)
    {
        twsTopology_StartHdma(sm);
    }

    else
    {
        /* No change in state, do nothing */
    }
}

inline static bool twsTopology_ApplicationRequestInQ(tws_topology_sm_t *sm)
{
    return sm->pending_app_event != TOPOLOGY_APP_EVENT_NONE;
}

inline static bool twsTopology_HandoverInIdleState(tws_topology_sm_t *sm)
{
    return sm->handover_reason == HDMA_HANDOVER_REASON_INVALID;
}

inline static bool twsTopology_ActiveRoleAssigned(tws_topology_sm_t *sm)
{
    return sm->elected_role != tws_topology_elected_role_none;
}

inline static void twsTopology_ResetExternalAppEvent(tws_topology_sm_t *sm)
{
    if(twsTopology_ApplicationRequestInQ(sm) && twsTopology_HandoverInIdleState(sm) && twsTopology_ActiveRoleAssigned(sm))
    {
        sm->pending_app_event = TOPOLOGY_APP_EVENT_NONE;
    }
}

inline static void twsTopology_SwapRole(tws_topology_sm_t *sm, topology_app_event_t app_event)
{
    if(TwsTopology_IsRolePrimaryConnectedToPeer() && TwsTopology_IsRoleSwapSupported())
    {
        sm->pending_app_event = app_event;
        Hdma_ExternalHandoverRequest();        
    }
}

#ifdef ENABLE_SKIP_PFR
static inline void appEnableKeepTopologyAliveForStaticHandover(void)
{
    keep_topology_alive_for_static_handover = TRUE;
}

static inline void appDisableKeepTopologyAliveForStaticHandover(void)
{
    keep_topology_alive_for_static_handover = FALSE;
}

bool appIsKeepTopologyAliveForStaticHandoverEnabled(void)
{
    return keep_topology_alive_for_static_handover;
}

static void twsTopology_EnableRoleSelectedByPeerFindRole(tws_topology_sm_t *sm, bool enable)
{
    sm->role_elected_by_pfr = enable;
}

static void twsTopology_EnterStaticHandover(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);

    tws_topology_msg_static_handover_req_t* req = PanicUnlessMalloc(sizeof(tws_topology_msg_static_handover_req_t));

    DEBUG_LOG_INFO("twsTopology_EnterStaticHandover, send static handover request to secondary");
    appPeerSigMarshalledMsgChannelTx(TwsTopologyGetTask(),
                                     PEER_SIG_MSG_CHANNEL_TOPOLOGY,
                                     req, MARSHAL_TYPE(tws_topology_msg_static_handover_req_t));
}

static void twsTopology_ExitStaticHandover(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);

    if (MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT))
    {
        DEBUG_LOG_INFO("twsTopology_ExitStaticHandover, cancelling window timeout");
        appDisableKeepTopologyAliveForStaticHandover();
    }
}

static void twsTopology_EnterSelectPreservedRole(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);

    peer_find_role_preserved_role_t role = PeerFindRole_GetPreservedRole();

    if (role == peer_find_role_preserved_role_primary)
    {
        DEBUG_LOG("twsTopology_EnterBecomePreservedRole, elected role is primary");
        twsTopology_SetElectedRole(sm, tws_topology_elected_role_primary_with_peer);
        twsTopology_SmEnqueueKick(sm);
    }
    else
    {
        DEBUG_LOG("twsTopology_EnterBecomePreservedRole, elected role is secondary");
        /* Reset the role_elected_by_pfr, after every boot to trigger the
         * 2s timer for secondary connect peer */
        twsTopology_EnableRoleSelectedByPeerFindRole(sm, FALSE);
        twsTopology_SetElectedRole(sm, tws_topology_elected_role_secondary);
        twsTopology_SmEnqueueKick(sm);
    }

    if (role == peer_find_role_preserved_role_primary)
    {
        /* start the internal static handover timeout when the EB starts as a preserved
           primary */
        PanicFalse(MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT) == 0);

        MessageSendLater(TwsTopologyGetTask(),
                         TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT,
                         NULL,
                         D_SEC(twsTopology_GetProductBehaviour()->timeouts.post_idle_static_handover_timeout_sec));

        appEnableKeepTopologyAliveForStaticHandover();
    }
}

static void twsTopology_ExitSelectPreservedRole(tws_topology_sm_t *sm, unsigned dummy)
{
    UNUSED(dummy);
    UNUSED(sm);
}

#endif

/*! \brief Set a new state, triggering state exit/enter functions */
static void twsTopology_SetState(tws_topology_sm_t *sm, tws_topology_state_t new_state)
{
    tws_topology_state_t old_state = sm->state;

    if (old_state == new_state)
    {
        return;
    }

    PanicFalse(new_state < TWS_TOPOLOGY_STATES_END);

    DEBUG_LOG_STATE("twsTopology_SetState "
                    "enum:tws_topology_state_t:%d --> "
                    "enum:tws_topology_state_t:%d"
                    " /' Target state: enum:tws_topology_state_t:%d '/",
                    old_state, new_state, sm->target_state);

    twsTopology_ExecuteStateExitFunction(sm, old_state);

    twsTopology_ManageHdmaOperation(sm, old_state, new_state);

    sm->state = new_state;

    twsTopology_ExecuteStateEntryFunction(sm, old_state, new_state);
}

/*! \brief Set a goal, by sending the goal id to the goal task */
static void twsTopology_SmSetGoal(tws_topology_sm_t *sm, unsigned id)
{
    MessageSend(&sm->goal_task, (uint16)id, NULL);
}

/*! \brief Default method of handling goal completion.
    \note Assumes 1) the goal can only complete in one state 2) the goal was
    successful.
*/
static tws_topology_state_t twsTopology_SmGoalCompleteDefaultHandler(tws_topology_sm_t *sm, procedure_result_t result, tws_topology_state_t expected, tws_topology_state_t next)
{
    PanicFalse(sm->state == expected);
    PanicFalse(result == procedure_result_success);
    return next;
}

/*! \brief Handle tws_topology_goal_pair_peer */
static tws_topology_state_t twsTopology_SmHandleGoalPairPeerComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state;

    PanicFalse(sm->state == TWS_TOPOLOGY_STATE_PEER_PAIRING);
    if (result == procedure_result_success)
    {
        next_state = TWS_TOPOLOGY_STATE_STARTED;
    }
    else
    {
        next_state = TWS_TOPOLOGY_STATE_STARTING;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_no_role_idle */
static tws_topology_state_t twsTopology_SmHandleGoalNoRoleIdleComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    return twsTopology_SmGoalCompleteDefaultHandler(sm, result, TWS_TOPOLOGY_STATE_BECOME_IDLE, TWS_TOPOLOGY_STATE_IDLE);
}

/*! \brief Handle tws_topology_goal_secondary_connect_peer */
static tws_topology_state_t twsTopology_SmHandleGoalSecondaryConnectPeerComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state = sm->state;

    PanicFalse(sm->state == TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY);
    if (result == procedure_result_success)
    {
        next_state = TWS_TOPOLOGY_STATE_SECONDARY;
    }
    else
    {
        sm->error_forced_no_role = TRUE;
        twsTopology_RelinquishElectedRole(sm);
        next_state = TWS_TOPOLOGY_STATE_BECOME_IDLE;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_primary_connectable_peer */
static tws_topology_state_t twsTopology_SmHandleGoalPrimaryConnectablePeerComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state = sm->state;

    switch (sm->state)
    {
        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER:
            if(result == procedure_result_success)
            {
                next_state = TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY;
            }
            else
            {
                // Revert to last stable state
                twsTopology_SetElectedRole(sm, tws_topology_elected_role_standalone_primary);
                next_state = TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
            }            
        break;

        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY:
            if(result == procedure_result_success)
            {
                next_state = TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER;
            }
            else
            {
                // Revert to last stable state
                twsTopology_SetElectedRole(sm, tws_topology_elected_role_secondary);
                next_state = TWS_TOPOLOGY_STATE_SECONDARY;
            }
        break;

        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY:
            if (result == procedure_result_success)
            {
                next_state = TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES;
            }
            else
            {
                /*! The elected role was primary, but the peer failed to
                    connect, so the elected role must be switched to standalone
                    primary */
                twsTopology_SetElectedRole(sm, tws_topology_elected_role_standalone_primary);
                next_state = TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
            }
        break;
        case TWS_TOPOLOGY_STATE_BECOME_IDLE:
            /* tws_topology_goal_no_role_idle is a cancel goal, so it can cancel
               tws_topology_goal_primary_connectable_peer. In that case the SM
               will be in the TWS_TOPOLOGY_STATE_BECOME_IDLE state when the
               cancelled procedure completes and no further action is required. */
            DEBUG_LOG("twsTopology_SmHandleGoalPrimaryConnectablePeerComplete becoming idle, "
                      "enum:procedure_result_t:%d", result);
        break;

        default:
            Panic();
        break;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_primary_connect_peer_profiles */
static tws_topology_state_t twsTopology_SmHandleGoalPrimaryConnectPeerProfilesComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state = sm->state;

    switch (sm->state)
    {
        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES:
            if (result == procedure_result_success)
            {
                next_state = TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER;
            }
            else
            {
                /*! The elected role was primary, but the peer profiles failed
                    to connect, so the elected role must be switched to standalone
                    primary */
                twsTopology_SetElectedRole(sm, tws_topology_elected_role_standalone_primary);
                next_state = TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
            }
        break;
        default:
            Panic();
        break;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_dynamic_handover_prepare */
static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverPrepareComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    return twsTopology_SmGoalCompleteDefaultHandler(sm, result, TWS_TOPOLOGY_STATE_HANDOVER_PREPARE, TWS_TOPOLOGY_STATE_HANDOVER_PREPARED);
}

static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state = sm->state;

    switch (sm->state)
    {
        case TWS_TOPOLOGY_STATE_HANDOVER:
            switch (result)
            {
                case procedure_result_success:
                    twsTopology_SetElectedRole(sm, tws_topology_elected_role_secondary);
                    next_state = TWS_TOPOLOGY_STATE_SECONDARY;
                    if(sm->pending_app_event == TOPOLOGY_APP_EVENT_SWAP_ROLE_AND_DISCONNECT)
                    {
                        TwsTopology_Leave(role_change_auto);
                        twsTopology_DisconnectPeer();
                        sm->pending_app_event = TOPOLOGY_APP_EVENT_NONE;
                    }
                break;

                case procedure_result_timeout:
                    if (MessagePendingFirst(TwsTopologyGetTask(), TWSTOP_INTERNAL_HANDOVER_WINDOW_TIMEOUT, NULL))
                    {
                        next_state = TWS_TOPOLOGY_STATE_HANDOVER_RETRY;
                        break;
                    }
                    //fall-through, max attempts

                case procedure_result_failed:
                    next_state = TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE;
                    sm->handover_reason = HDMA_HANDOVER_REASON_INVALID;
                    sm->handover_failed = TRUE;
                break;

                default:
                    Panic();
                break;
            }
        break;

        default:
            Panic();
        break;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_dynamic_handover_undo_prepare */
static tws_topology_state_t twsTopology_SmHandleGoalDynamicHandoverUndoPrepareComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    return twsTopology_SmGoalCompleteDefaultHandler(sm, result, TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE, TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER);
}

/*! \brief Handle tws_topology_goal_become_standalone_primary */
static tws_topology_state_t twsTopology_SmHandleGoalBecomeStandalonePrimaryComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    PanicFalse(sm->state == TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY);
    PanicFalse(result == procedure_result_success);

    if((TwsTopology_GetJoinStatus() == FALSE) || twsTopology_IsDeviceTypeStandalone())
    {
        PeerFindRole_FindRoleCancel();
    }
    return TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY;
}

/*! \brief Handle tws_topology_goal_become_primary */
static tws_topology_state_t twsTopology_SmHandleGoalBecomePrimaryComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    tws_topology_state_t next_state = sm->state;

    PanicFalse(result == procedure_result_success);

    switch (sm->state)
    {
        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER:
            next_state = TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY;
        break;
        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY:
            next_state = TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER;
        break;
        default:
            Panic();
        break;
    }
    return next_state;
}

/*! \brief Handle tws_topology_goal_become_secondary */
static tws_topology_state_t twsTopology_SmHandleGoalBecomeSecondaryComplete(tws_topology_sm_t *sm, procedure_result_t result)
{
    return twsTopology_SmGoalCompleteDefaultHandler(sm, result, TWS_TOPOLOGY_STATE_BECOME_SECONDARY, TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY);
}

/*! \brief Get the next state in the path from current state to target state.
    \param current The current SM state.
    \param target The target SM state.
    \return The next state.

    \note This function specifies the transitions from steady states to
    transient states. The transitions from transient states to transient states,
    or transient states to steady states are coded in the goal complete
    handler functions (twsTopology_SmHandleGoal.*)
*/
static tws_topology_state_t twsTopology_SmGetNextState(tws_topology_state_t current, tws_topology_state_t target)
{
    switch (current)
    {
        case TWS_TOPOLOGY_STATE_STARTING:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_PEER_PAIRING:
                    return TWS_TOPOLOGY_STATE_PEER_PAIRING;
                default:
                    return TWS_TOPOLOGY_STATE_STARTED;
            }

        case TWS_TOPOLOGY_STATE_STARTED:
            return TWS_TOPOLOGY_STATE_BECOME_IDLE;

        case TWS_TOPOLOGY_STATE_IDLE:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_STOPPED:
                    return TWS_TOPOLOGY_STATE_STOPPED;
                case TWS_TOPOLOGY_STATE_SECONDARY:
                    return TWS_TOPOLOGY_STATE_BECOME_SECONDARY;
                case TWS_TOPOLOGY_STATE_FIND_ROLE:
                    return TWS_TOPOLOGY_STATE_FIND_ROLE;
#ifdef ENABLE_SKIP_PFR
                case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
                    return TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER;
                case TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE:
                    return TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE;
#endif
                default:
                break;
            }
        break;

        case TWS_TOPOLOGY_STATE_FIND_ROLE:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
                    return TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
                case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
                    return TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER;
                default:
                    return TWS_TOPOLOGY_STATE_BECOME_IDLE;
            }

        case TWS_TOPOLOGY_STATE_SECONDARY:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
                    return TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
                default:
                    return TWS_TOPOLOGY_STATE_BECOME_IDLE;
            }

        case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
                    return TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER;
                default:
                    return TWS_TOPOLOGY_STATE_BECOME_IDLE;
            }

        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
                    /* Remain in state until secondary connects */
                    return TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY;
                case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
                    return TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
                default:
                    return TWS_TOPOLOGY_STATE_BECOME_IDLE;
            }

        case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_SECONDARY:
                    return TWS_TOPOLOGY_STATE_HANDOVER_PREPARE;
                case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
                    return TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY;
#ifdef ENABLE_SKIP_PFR
                case TWS_TOPOLOGY_STATE_STATIC_HANDOVER:
                    return TWS_TOPOLOGY_STATE_STATIC_HANDOVER;
#endif
                default:
                    return TWS_TOPOLOGY_STATE_BECOME_IDLE;
            }

        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARED:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_SECONDARY:
                    return TWS_TOPOLOGY_STATE_HANDOVER;
                default:
                    return TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE;
            }

        case TWS_TOPOLOGY_STATE_HANDOVER_RETRY:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_SECONDARY:
                    /* Remain in state until retry timer expires */
                    return TWS_TOPOLOGY_STATE_HANDOVER_RETRY;
                default:
                    return TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE;
            }

#ifdef ENABLE_SKIP_PFR
        case TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE:
            switch (target)
            {
                case TWS_TOPOLOGY_STATE_SECONDARY:
                    return TWS_TOPOLOGY_STATE_BECOME_SECONDARY;
                case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
                    return TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER;

                default:
                break;
             }

        case TWS_TOPOLOGY_STATE_STATIC_HANDOVER:
            return TWS_TOPOLOGY_STATE_BECOME_IDLE;
#endif

        default:
        break;
    }
    Panic();
    return 0;
}

static bool twsTopology_IsTopologyInStableStateToLeave(tws_topology_sm_t *sm)
{    
    static int16 remaining_retry_time_ms = MAX_CHECK_STABLE_STATE_MILLISECS;
    bool stable_state = TRUE;
    bool is_mirror_profile_connecting = TwsTopology_IsRolePrimaryConnectedToPeer() && !MirrorProfile_IsBredrMirroringConnected();
    bool is_state_unsteady = !twsTopology_StateIsSteady(sm->state);
    if((is_mirror_profile_connecting || is_state_unsteady) && (remaining_retry_time_ms > 0))
    {        
        /* Topology is not is stable state if:
         * a) Can be in a steady state as far as SM is concerned, but still in process of connecting mirror profile
         * b) Not in a steady state.
         * We need these conditions to be false before executing topology API. This will ensures that the SM does not get
         * into an incorrect state */                
        remaining_retry_time_ms -= CHECK_STABLE_STATE_RETRY_MILLISECS;
        DEBUG_LOG_WARN("twsTopology_IsTopologyInStableState: Delay Leave (mirror profile %d, state %d, retry remaining %dms)", is_mirror_profile_connecting, sm->state, remaining_retry_time_ms);
        stable_state = FALSE;
    }
    else
    {
        remaining_retry_time_ms = MAX_CHECK_STABLE_STATE_MILLISECS;
    }
    return stable_state;
}

static void twsTopology_LeaveAsPrimary(void)
{
    PanicFalse(TwsTopology_IsRolePrimary());
    if(TwsTopology_IsRolePrimaryConnectedToPeer())
    {
        if(TwsTopology_GetPeerJoinStatus())
        {
            DEBUG_LOG("twsTopology_LeaveAsPrimary: Swapping roles before leave as peer is connected");
            twsTopology_SmHandleInternalSwapRoleAndDisconnectRequest(TwsTopology_GetSm());
        }
        else
        {
            DEBUG_LOG("twsTopology_LeaveAsPrimary: Peer also just issued leave, but role not yet updated");
        }
    }
    else
    {
        // Just block peer connection attempts
        // Note: Topology has no knowledge of handset, so standalone primary servicing a leave request
        // will not cause the device to relinquish its role
        PanicFalse(TwsTopology_IsRoleStandAlonePrimary());
        PeerFindRole_FindRoleCancel();
    }
}

static void twsTopology_MoveToIdleStateIfRequired(tws_topology_sm_t *sm)
{
    if(TwsTopology_IsRoleSecondary() || sm->force_role_relinquish_on_leave)
    {
        //twsTopology_ForceIdleState(TwsTopology_GetSm());
        //twsTopology_RelinquishElectedRole(sm);
    
        // MASH TODO - Need to revisit this. We want to push the state machine through the
        // procedures but without role changing to none until any handover has completed. Shouldn't
        // really be directly setting state here, but need to be careful how this is resolved
        // without breaking something.
        sm->force_role_relinquish_on_leave = FALSE;
        sm->target_state = TWS_TOPOLOGY_STATE_IDLE;
        twsTopology_SmEnqueueKick(sm);
    }
}
