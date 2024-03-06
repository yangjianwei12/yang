/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header defining the states and state machine design for the TWS topology.
*/

#ifndef TWS_TOPOLOGY_SM_STATES_H_
#define TWS_TOPOLOGY_SM_STATES_H_

#include "tws_topology.h"

/*!
@startuml

title TWS Topology State Machine
hide empty description

[*] --> TWS_TOPOLOGY_STATE_STARTING : Power On

state "No Role" as NoRole #yellow {
    state TWS_TOPOLOGY_STATE_STARTING
    state TWS_TOPOLOGY_STATE_STARTED
    state TWS_TOPOLOGY_STATE_BECOME_IDLE
    state TWS_TOPOLOGY_STATE_IDLE
    state TWS_TOPOLOGY_STATE_FIND_ROLE

    state "Peer Pairing" as PeerPairing #red {
        state TWS_TOPOLOGY_STATE_PEER_PAIRING
    }
}

state Primary #lightgreen {
    state TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY
    state TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY
    state TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER
    state TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY
    state TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES
    state TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER

    state "Handover" as HP #magenta {
        state TWS_TOPOLOGY_STATE_HANDOVER_PREPARE
        state TWS_TOPOLOGY_STATE_HANDOVER_PREPARED
        state TWS_TOPOLOGY_STATE_HANDOVER
        state TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE
        state TWS_TOPOLOGY_STATE_HANDOVER_RETRY
    }
}

state Secondary #red {
    state TWS_TOPOLOGY_STATE_BECOME_SECONDARY
    state TWS_TOPOLOGY_STATE_SECONDARY
    state TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY
    state TWS_TOPOLOGY_STATE_SECONDARY

    state "Handover" as HS #magenta {
        state TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY
    }
}

state PeerPaired <<choice>>
note right of PeerPaired
Peer Paired?
end note

TWS_TOPOLOGY_STATE_STARTING --> PeerPaired
PeerPaired --> TWS_TOPOLOGY_STATE_PEER_PAIRING : No
PeerPaired --> TWS_TOPOLOGY_STATE_STARTED : Yes
TWS_TOPOLOGY_STATE_PEER_PAIRING --> TWS_TOPOLOGY_STATE_STARTED
TWS_TOPOLOGY_STATE_STARTED --> TWS_TOPOLOGY_STATE_BECOME_IDLE
TWS_TOPOLOGY_STATE_BECOME_IDLE --> TWS_TOPOLOGY_STATE_IDLE
TWS_TOPOLOGY_STATE_IDLE --> TWS_TOPOLOGY_STATE_FIND_ROLE : Out of Case /' Primary '/
TWS_TOPOLOGY_STATE_FIND_ROLE -l-> TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY
TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY --> TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY
TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY --> TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER : Secondary Out of case
TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER -l-> TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY
TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY --> TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES
TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES -r-> TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER
TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER --> TWS_TOPOLOGY_STATE_HANDOVER_PREPARE : Handover Request
TWS_TOPOLOGY_STATE_HANDOVER_PREPARE  --> TWS_TOPOLOGY_STATE_HANDOVER_PREPARED
TWS_TOPOLOGY_STATE_HANDOVER_PREPARED --> TWS_TOPOLOGY_STATE_HANDOVER

TWS_TOPOLOGY_STATE_FIND_ROLE --> TWS_TOPOLOGY_STATE_BECOME_IDLE : Only Secondary
TWS_TOPOLOGY_STATE_IDLE --> TWS_TOPOLOGY_STATE_BECOME_SECONDARY
TWS_TOPOLOGY_STATE_BECOME_SECONDARY --> TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY
TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY --> TWS_TOPOLOGY_STATE_SECONDARY
TWS_TOPOLOGY_STATE_SECONDARY --> TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY : Handover Request
TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY --> TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER

state HandoverSuccessful <<choice>>
TWS_TOPOLOGY_STATE_HANDOVER -u-> HandoverSuccessful
note left of HandoverSuccessful
Handover Successful?
end note

HandoverSuccessful --> TWS_TOPOLOGY_STATE_SECONDARY : Yes /' Primary transition '/
HandoverSuccessful --> TWS_TOPOLOGY_STATE_HANDOVER_RETRY : No
HandoverSuccessful --> TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE : [Max. Retries Exceeded]
TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE --> TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER
TWS_TOPOLOGY_STATE_HANDOVER_RETRY --> TWS_TOPOLOGY_STATE_HANDOVER

@enduml
*/

/*! Definition of TWS Topology states */
typedef enum
{
    /*
    -- Steady states MUST be listed first. --

    A steady state is one in which a state transition may occur towards the
    target state. Non-steady states are ones which must wait for a goal to
    complete prior to transitioning towards the target state.

    TWS_TOPOLOGY_STATE_LAST_STEADY_STATE must be updated if extra steady states
    are added.

    */
    /*! Topology is no longer stopped and can transition to next state */
    TWS_TOPOLOGY_STATE_STARTING,

    /*! Topology has started  */
    TWS_TOPOLOGY_STATE_STARTED,

    /*! Idle */
    TWS_TOPOLOGY_STATE_IDLE,

#ifdef ENABLE_SKIP_PFR
    /*! After bootup if skip PFR is enabled, then elect the roles from PS key.
    This state will help to read from PS key and update the new elected roles.
    */
    TWS_TOPOLOGY_STATE_SELECT_PRESERVED_ROLE,
#endif

    /*! In the candidate state, the earbud finds its role using the "peer find
    role" component. The elected role (standalone primary, primary, secondary)
    determines the subsequent address and behaviour of the earbud. Both earbuds
    use the primary address in the candidate state. */
    TWS_TOPOLOGY_STATE_FIND_ROLE,

    /*! The standalone primary state is used when the earbud is unable to connect to
    its peer during role selection in the TWS_TOPOLOGY_STATE_FIND_ROLE state.
    After a timeout, the earbud self-elects itself to the primary role but with
    "standalone" prefix. The standalone primary behaviour is generally the same as the
    primary behaviour, with the difference that the earbud is continuing in the
    background to attempt to reconnect to its peer. */
    TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY,

    /*! When the primary role is elected in the TWS_TOPOLOGY_STATE_FIND_ROLE
    state, the earbud becomes connectable (page scanning) allowing the secondary
    earbud to initiate the ACL connection. The earbud remains connectable for a
    finite period. If the secondary fails to connect during this period, the
    earbud falls-back to standalone-primary role. This is a "steady" state because
    the earbud may transition back to idle state which immediately stops the
    connectable mode.
    */
    TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY,

    /*! Having become primary, the earbud activates the handover decision making
    algorithm (HDMA). This is a long term steady-state of the primary earbud. */
    TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER,

    /*! After the secondary has connected to the primary in the
    TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING state, it enters this state. This is
    the long term steady-state of the secondary earbud.
    */
    TWS_TOPOLOGY_STATE_SECONDARY,

    /*! When the primary earbud determines it should handover connections and
    become secondary, it prepares to handover in the
    TWS_TOPOLOGY_STATE_HANDOVER_PREPARE state. Once prepared, it enters this
    state before starting the handover procedure in the
    TWS_TOPOLOGY_STATE_HANDOVER state. This short-term steady-state exists so
    that handover may be cancellled post-prepare allowing the earbud to
    transition back to the primary role via the
    TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE state. */
    TWS_TOPOLOGY_STATE_HANDOVER_PREPARED,

    /*! If handover is vetoed in the TWS_TOPOLOGY_STATE_HANDOVER state, the SM
    enters this state. Here, it waits for a short period before re-attempting
    handover. This steady state exists so that handover may be cancelled
    allowing the earbud to transition back to the primary role via the
    TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE state. */
    TWS_TOPOLOGY_STATE_HANDOVER_RETRY,

    /* TWS_TOPOLOGY_STATE_LAST_STEADY_STATE must be updated if extra steady
    states are added. */

    /*
    -- Non-steady-states MUST be placed after this comment --
    */

    /*! Topology is stopped, this is not a steady state meaning the state
    must explicity be set to a steady state to allow the SM to operate.  */
    TWS_TOPOLOGY_STATE_STOPPED,

    /*! Before the earbuds can connect to handsets, the two earbuds _must_ pair.
    During peer pairing, the two earbuds pair, select which of their addresses
    is used as the primary and secondary and synchronise their root keys. */
    TWS_TOPOLOGY_STATE_PEER_PAIRING,

    /*! After election to primary role in the TWS_TOPOLOGY_STATE_FIND_ROLE
    state, this state performs the procedures required for the primary role. */
    TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER,

    /*! When handover completes on the secondary, it needs to become primary. */
    TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY,

    /*! After election to standalone-primary role in the
    TWS_TOPOLOGY_STATE_FIND_ROLE state, this state performs the procedures
    required for the standalone-primary role */
    TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY,

    /*! After election to secondary role in the TWS_TOPOLOGY_STATE_FIND_ROLE
    state, this state performs the procedures required for the secondary role.
    This includes swapping addresses from primary to secondary.
    */
    TWS_TOPOLOGY_STATE_BECOME_SECONDARY,

    /*! After election to secondary role in the TWS_TOPOLOGY_STATE_FIND_ROLE
    state and becoming secondary in the TWS_TOPOLOGY_STATE_BECOME_SECONDARY
    state, the secondary earbud initiates the connection to the primary earbud
    in this state. */
    TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY,

    /*! After the secondary has connected to the primary earbud in the
    TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING state, the primary _must_ connect
    all peer profiles. The secondary must _not_ connect any profiles. This
    requirement exists due to the way L2CAP CIDs are managed by the upper stack
    during in order to support handover. */
    TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES,

    /*! In this state the primary earbud prepares the system for handover. */
    TWS_TOPOLOGY_STATE_HANDOVER_PREPARE,

    /*! In this state the handover is performed where all handsets are
    transferred to the secondary earbud and the roles and addresses are swapped.
    */
    TWS_TOPOLOGY_STATE_HANDOVER,

    /*! If handover is cancelled (either after preparing in the
    TWS_TOPOLOGY_STATE_HANDOVER_PREPARED state, or after a vetoed handover in
    the TWS_TOPOLOGY_STATE_HANDOVER_RETRY state, the SM un-does the actions
    performed in the TWS_TOPOLOGY_STATE_HANDOVER_PREPARE state. This allows the
    earbud to resume its primary role. */
    TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE,

    /*! In numerous scenarios, the SM needs to return to the idle state.
    In this state the SM performs the actions required to transtion from active
    (in any role) to idle. */
    TWS_TOPOLOGY_STATE_BECOME_IDLE,

    /*! If handset is not connected, then HDMA will not be triggered. In that case
    if the primary goes in case, need to do static handover if the other EB is out of case.
    */
    TWS_TOPOLOGY_STATE_STATIC_HANDOVER,

    /*! Always the final state, only used to determine the number of states in
    the SM */
    TWS_TOPOLOGY_STATES_END,

} tws_topology_state_t;


/*! The last steady state from the enumeration */
#define TWS_TOPOLOGY_STATE_LAST_STEADY_STATE TWS_TOPOLOGY_STATE_HANDOVER_RETRY

/*! Definition of the earbud elected roles in the TWS topology SM. */
typedef enum
{
    /*! The earbud has no role. It cannot take any other role without first
    being elected to a role in the TWS_TOPOLOGY_STATE_FIND_ROLE state. */
    tws_topology_elected_role_none,
    /*! The earbud has attempted and failed to connect to the other earbud in
    the TWS_TOPOLOGY_STATE_FIND_ROLE state. It has self-elected itself to become
    "standalone" primary. */
    tws_topology_elected_role_standalone_primary,
    /*! The earbud has been elected as the primary earbud in the
    TWS_TOPOLOGY_STATE_FIND_ROLE state. */
    tws_topology_elected_role_primary_with_peer,
    /*! The earbud has been elected as the secondary earbud in the
    TWS_TOPOLOGY_STATE_FIND_ROLE state. */
    tws_topology_elected_role_secondary,

} tws_topology_elected_role_t;

typedef enum
{
    TOPOLOGY_APP_EVENT_NONE,
    TOPOLOGY_APP_EVENT_JOIN_REQUEST,
    TOPOLOGY_APP_EVENT_SWAP_ROLE_AND_DISCONNECT,
}topology_app_event_t;

/*! \brief Query if the state is a steady one */
static inline bool twsTopology_StateIsSteady(tws_topology_state_t state)
{
    return (state <= TWS_TOPOLOGY_STATE_LAST_STEADY_STATE);
}

/*! \brief Get the topology role from the SM's state. */
static inline tws_topology_role twsTopology_RoleFromState(tws_topology_state_t state)
{
    switch (state)
    {
        case TWS_TOPOLOGY_STATE_BECOME_STANDALONE_PRIMARY:
        case TWS_TOPOLOGY_STATE_STANDALONE_PRIMARY:
        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_WITH_PEER:
        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY:
        case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARED:
        case TWS_TOPOLOGY_STATE_HANDOVER_RETRY:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARE:
        case TWS_TOPOLOGY_STATE_HANDOVER:
        case TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE:
#ifdef ENABLE_SKIP_PFR
        case TWS_TOPOLOGY_STATE_STATIC_HANDOVER:
#endif
            return tws_topology_role_primary;
        case TWS_TOPOLOGY_STATE_SECONDARY:
        case TWS_TOPOLOGY_STATE_SECONDARY_CONNECTING_TO_PRIMARY:
        case TWS_TOPOLOGY_STATE_BECOME_PRIMARY_FROM_SECONDARY:
            return tws_topology_role_secondary;
        default:
            return tws_topology_role_none;
    }
}

static inline bool twsTopology_GetPrimaryPeerConnectStatusFromState(tws_topology_state_t state)
{
    //Panic(TwsTopology_IsPrimaryRole());
    bool is_peer_connected = FALSE;
    switch (state)
    {
        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECTABLE_FOR_SECONDARY:
        case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
        case TWS_TOPOLOGY_STATE_PRIMARY_CONNECT_PEER_PROFILES:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARED:
        case TWS_TOPOLOGY_STATE_HANDOVER_RETRY:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARE:
        case TWS_TOPOLOGY_STATE_HANDOVER:
        case TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE:
            is_peer_connected = TRUE;
            break;

        default:
            break;
    }

    return is_peer_connected;
}

/*! \brief Query if the state requires HDMA */
static inline bool twsTopology_StateRequiresHdma(tws_topology_state_t state)
{
    switch (state)
    {
        case TWS_TOPOLOGY_STATE_PRIMARY_WITH_PEER:
        case TWS_TOPOLOGY_STATE_HANDOVER:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARE:
        case TWS_TOPOLOGY_STATE_HANDOVER_PREPARED:
        case TWS_TOPOLOGY_STATE_HANDOVER_RETRY:
        case TWS_TOPOLOGY_STATE_HANDOVER_UNDO_PREPARE:
            return TRUE;
        default:
            return FALSE;
    }
}

#endif
