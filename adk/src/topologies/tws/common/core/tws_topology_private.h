/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Private header file for the TWS topology.
*/

#ifndef TWS_TOPOLOGY_PRIVATE_H_
#define TWS_TOPOLOGY_PRIVATE_H_

#include "tws_topology_sm.h"
#include "tws_topology_config.h"
#include "tws_topology.h"
#include <goals_engine.h>
#include <task_list.h>
#include <stdlib.h>

#include <message.h>
#include <bdaddr.h>

/*! Defines the roles changed task list initalc capacity */
#define MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY 1

typedef enum
{
    /*! Used by procedure notify role change clients to indicate clients are
    prepared for role change */
    TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED = INTERNAL_MESSAGE_BASE,

    /*! Used by procedure notify role change clients to indicate clients have
    rejected the role change.  */
    TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION,

    /*! Used by the procedure nofiy role change clients to handle failure
    of clients to response to role change proposal. */
    TWSTOP_INTERNAL_PROPOSAL_TIMEOUT,

    /*! Used by the procedure nofiy role change clients to handle timing
    after nofiying clients of forced role change. */
    TWSTOP_INTERNAL_FORCE_TIMEOUT,

    /*! Internal message sent if the topology stop command times out */
    TWSTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP,

    /*! Message that triggers the SM re-evaluate its state */
    TWSTOP_INTERNAL_KICK_SM,

    /*! Message to trigger handover retry */
    TWSTOP_INTERNAL_RETRY_HANDOVER,

    /*! Internal message to indicate the handover window period has ended */
    TWSTOP_INTERNAL_HANDOVER_WINDOW_TIMEOUT,

    /*! Internal message to trigger device joining topology */
    TWSTOP_INTERNAL_JOIN_REQUEST,

    /*! Internal message to trigger device leaving topology */
    TWSTOP_INTERNAL_LEAVE_REQUEST,

    /*! Internal message to trigger swapping roles */
    TWSTOP_INTERNAL_SWAP_ROLE_REQUEST,

    /*! Internal message to trigger device swapping roles and disconnecting */
    TWSTOP_INTERNAL_SWAP_ROLE_AND_DISCONNECT_REQUEST,

    /*! Internal message to indicate the static handover timeout happened */
    TWSTOP_INTERNAL_STATIC_HANDOVER_TIMEOUT,

    /*! Internal message to indicate the primary that secondary is out of case
     for this timeout and let the primary to decide whether it should do
     static handover or not. */
    TWSTOP_INTERNAL_SECONDARY_OUT_OF_CASE_TIMEOUT,

    TWSTOP_INTERNAL_MESSAGE_END
} tws_topology_internal_message_t;

/* Validate that internal message range has not been breached. */
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(TWSTOP_INTERNAL_MESSAGE_END)

/*! Structure holding information for the TWS Topology task */
typedef struct
{
    /*! Task for handling messages */
    TaskData                task;

    /*! TWS topology SM state */
    tws_topology_sm_t       tws_sm;

    /*! List of clients registered to receive TWS_TOPOLOGY_ROLE_CHANGED_IND_T
     * messages */
    TASK_LIST_WITH_INITIAL_CAPACITY(MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY)   message_client_tasks;

    /*! Queue of goals already decided but waiting to be run. */
    TaskData                pending_goal_queue_task;

    /*! The TWS topology goal set */
    goal_set_t              goal_set;

    /*! Whether Handover is allowed or prohibited. controlled by APP */
    bool                    enable_role_swap:1;

    /*! The currently selected advertising parameter set */
    tws_topology_le_adv_params_set_type_t advertising_params;

    /*! Product behaviour configuration*/
    const tws_topology_product_behaviour_t*   product_behaviour;

} twsTopologyTaskData;

extern twsTopologyTaskData tws_topology;

/*! Get pointer to the task data */
#define TwsTopologyGetTaskData()         (&tws_topology)

/*! Get pointer to the TWS Topology task */
#define TwsTopologyGetTask()             (&tws_topology.task)

/*! Get pointer to the TWS Topology role changed tasks */
#define TwsTopologyGetMessageClientTasks() (task_list_flexible_t *)(&tws_topology.message_client_tasks)

#define TwsTopology_GetSm() (&TwsTopologyGetTaskData()->tws_sm)

#define TwsTopology_IsLeaveStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.leave)
#define TwsTopology_ClearLeaveStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.leave = FALSE)
#define TwsTopology_SetLeaveStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.leave = TRUE)

#define TwsTopology_IsJoinStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.join)
#define TwsTopology_ClearJoinStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.join = FALSE)
#define TwsTopology_SetJoinStatusWaitingTxToPeer() (tws_topology.tws_sm.topology_status.pending_peer_send.join = TRUE)

#define TwsTopology_GetJoinStatus() (tws_topology.tws_sm.topology_status.join_status.self)
#define TwsTopology_ClearJoinStatus() (tws_topology.tws_sm.topology_status.join_status.self = FALSE)
#define TwsTopology_SetJoinStatus() (tws_topology.tws_sm.topology_status.join_status.self = TRUE)

#define TwsTopology_GetPeerJoinStatus() (tws_topology.tws_sm.topology_status.join_status.peer)
#define TwsTopology_ClearPeerJoinStatus() (tws_topology.tws_sm.topology_status.join_status.peer = FALSE)
#define TwsTopology_SetPeerJoinStatus() (tws_topology.tws_sm.topology_status.join_status.peer = TRUE)

/*! Macro to create a TWS topology message. */
#define MAKE_TWS_TOPOLOGY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))

/*! Private API used for test functionality

    \return TRUE if topology has been started, FALSE otherwise
 */
bool twsTopology_IsRunning(void);

/*! \brief Gets the product behaviour configuration

    \return product behaviour config
 */
const tws_topology_product_behaviour_t* twsTopology_GetProductBehaviour(void);

/*! \brief Enables/disables support for role swap

    \param enable_role_swap - TRUE enables role swap, FALSE disables role swap
 */
void twsTopology_SetRoleSwapSupport(bool enable_role_swap);

/*! \brief Transitions the secondary to be standalone primary
 *
    \param sm - topology state machine
 */
void twsTopology_TransitionSecondaryToStandalonePrimary(tws_topology_sm_t* sm);

#ifdef ENABLE_SKIP_PFR
/*! \brief Get whether the role is decided using PFR or not
*/
bool TwsToplogy_IsRoleElectedByPeerFindRole(void);
#endif

#endif /* TWS_TOPOLOGY_H_ */
