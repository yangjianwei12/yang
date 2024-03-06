/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for TWS Topology State Machine.
*/

#ifndef TWS_TOPOLOGY_SM_H_
#define TWS_TOPOLOGY_SM_H_

#include "tws_topology_goals.h"
#include "tws_topology_sm_states.h"
#include "procedures.h"
#include <hdma.h>

typedef struct
{
    /*! Task for handling goal messages (from the SM) */
    TaskData                goal_task;

    /*! The role elected by "peer find role" */
    tws_topology_elected_role_t elected_role:4;

    topology_app_event_t pending_app_event:4;

    /*! Current state */
    tws_topology_state_t state:8;

    /*! Desired target state */
    tws_topology_state_t target_state:8;

    /*! Flag to indicate handover failed(either handover window expired or handover failed for other reasons) */
    bool handover_failed:1;

    /*! Handover reason */
    hdma_handover_reason_t  handover_reason:4;


    struct
    {
        struct
        {
            /*! Join topology state of self */
            bool self:1;

            /*! Join topology state of peer */
            bool peer:1;
        }join_status;

        struct
        {
            /*! Status of whether the join status has been sent to peer */
            bool join:1;

            /*! Status of whether the leave status has been sent to peer */
            bool leave:1;
        }pending_peer_send;
    }topology_status;

    /*! Flag indicating whether the device should be forced to relinquish its role on leaving topology */
    bool                    force_role_relinquish_on_leave:1;
    
    bool initial_idle_notification_sent:1;
    
    bool error_forced_no_role:1;

#ifdef ENABLE_SKIP_PFR
    /*! Flag to indicate whether the role is elected throgh PFR or not */
    bool role_elected_by_pfr;
#endif

} tws_topology_sm_t;

/*! \brief Initialise the SM.
    \param sm The sm.
*/
void twsTopology_SmInit(tws_topology_sm_t *sm);

/*! \brief Start the topology SM.
    \param sm The sm.
*/
void twsTopology_SmStart(tws_topology_sm_t *sm);

/*! \brief Stop the topology SM.
    \param sm The sm.
*/
void twsTopology_SmStop(tws_topology_sm_t *sm);

/*! \brief Trigger immediate state machine transition if SM is in a steady state.
    \param sm The sm.
*/
void twsTopology_SmKick(tws_topology_sm_t *sm);

/*! \brief Trigger a queued state machine transition if SM is in a steady state.
    \param sm The sm.
    \note The kick is queued by sending a message
*/
void twsTopology_SmEnqueueKick(tws_topology_sm_t *sm);

/*! \brief Handle the completion of a goal
    \param sm The sm.
    \param completed_goal The goal's ID.
    \param result The goal's result.
 */
void twsTopology_SmGoalComplete(tws_topology_sm_t *sm, tws_topology_goal_id completed_goal, procedure_result_t result);

/*! \brief Handle the peer earbud link disconnecting.
    \param sm The sm.
*/
void twsTopology_SmHandlePeerLinkDisconnected(tws_topology_sm_t *sm);

/*! \brief Handle standalone primary election result.
    \param sm The sm.
*/
void twsTopology_SmHandleElectedStandalonePrimary(tws_topology_sm_t *sm);

/*! \brief Handle primary election result.
    \param sm The sm.
*/
void twsTopology_SmHandleElectedPrimaryWithPeer(tws_topology_sm_t *sm);

/*! \brief Handle secondary election result.
    \param sm The sm.
*/
void twsTopology_SmHandleElectedSecondary(tws_topology_sm_t *sm);

/*! \brief Handle the completion of handover as secondary resulting in a
transition to the primary role.
    \param sm The sm.
*/
void twsTopology_SmHandleHandoverToPrimary(tws_topology_sm_t *sm);

/*! \brief Handle the completion of static handover to secondary
resulting in a transition to the secondary role.
    \param sm The sm.
*/
void twsTopology_SmHandleStaticHandoverToSecondary(tws_topology_sm_t *sm);

/*! \brief Handle the completion of static handover to primary
resulting in a transition to the primary role.
    \param sm The sm.
*/
void twsTopology_SmHandleStaticHandoverToPrimary(tws_topology_sm_t *sm);

#ifdef ENABLE_SKIP_PFR
/*! \brief Handle the completion of static handover timeout
    \param sm The sm.
*/
void twsTopology_SmHandlePostIdleStaticHandoverTimeout(tws_topology_sm_t *sm);

/*! \brief Determine if Topology should be kept alive for static handover

    \return TRUE if Topology should be kept alive, FALSE otherwise
*/
bool appIsKeepTopologyAliveForStaticHandoverEnabled(void);
#endif

/*! \brief Handle the TWSTOP_INTERNAL_RETRY_HANDOVER message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalRetryHandover(tws_topology_sm_t *sm);

/*! \brief Handle the TWSTOP_INTERNAL_TIMEOUT_TOPOLOGY_STOP message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalStopTimeout(tws_topology_sm_t *sm);

/*! \brief Handle the TWSTOP_INTERNAL_JOIN_REQUEST message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalJoinRequest(tws_topology_sm_t *sm);

/*! \brief Handle the TWSTOP_INTERNAL_LEAVE_REQUEST message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalLeaveRequest(tws_topology_sm_t *sm);

/*! \brief Handle the TWSTOP_INTERNAL_SWAP_ROLE message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalSwapRoleRequest(tws_topology_sm_t *sm);

/*! \brief Handle the TWSTOP_INTERNAL_SWAP_ROLE_AND_DISCONNECT_REQUEST message.
    \param sm The sm.
*/
void twsTopology_SmHandleInternalSwapRoleAndDisconnectRequest(tws_topology_sm_t *sm);

/*! \brief Handle handover request from HDMA.
    \param sm The sm.
    \param message Decision information.
*/
void twsTopology_SmHandleHDMARequest(tws_topology_sm_t *sm, hdma_handover_decision_t* message);

/*! \brief Handle handover cancel request from HDMA.
    \param sm The sm.
*/
void twsTopology_SmHandleHDMACancelHandover(tws_topology_sm_t *sm);

/*! \brief Set the SM's target state.
    \param sm The sm.
*/
void twsTopology_SetTargetState(tws_topology_sm_t *sm);

/*! \brief Forces the state of idle
    \param sm The sm.
*/
void twsTopology_ForceIdleState(tws_topology_sm_t *sm);

/*! \brief Performs the actions required to join topology
    \param sm The sm.
*/
void twsTopology_ExecuteJoinActions(tws_topology_sm_t *sm);

/*! \brief Performs the actions required to leave topology
    \param sm The sm.
*/
void twsTopology_ExecuteLeaveActions(tws_topology_sm_t *sm);

#endif /* TWS_TOPOLOGY_SM_H_ */
