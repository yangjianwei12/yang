/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWS Topology utility functions for sending messages to clients.
*/

#ifndef TWS_TOPOLOGY_CLIENT_MSGS_H
#define TWS_TOPOLOGY_CLIENT_MSGS_H

#include "tws_topology.h"

/*! \brief Send role change message to registered clients of topology.
    \param[in] role New Earbud role.
*/
void TwsTopology_SendRoleChangedCompletedMsg(tws_topology_role role, bool error_forced_no_role);

/*! \brief Send start completed message to registered clients of topology.
    \param[in] status success or failure status
*/
void TwsTopology_SendStartCompletedMsg(tws_topology_status_t status);

/*! \brief Send stop completed message to registered clients of topology.
    \param[in] status success or failure status
*/
void TwsTopology_SendStopCompletedMsg(tws_topology_status_t status);

/*! \brief Send notification that topology has transitioned from start to initial idle state to registered clients of topology.
*/
void TwsTopology_SendInitialIdleCompletedMsg(void);

/*! \brief Send join request completed message to registered clients of topology.
*/
void TwsTopology_SendJoinRequestCompletedMsg(void);

/*! \brief Send leave request completed message to registered clients of topology.
*/
void TwsTopology_SendLeaveRequestCompletedMsg(void);

#endif /* TWS_TOPOLOGY_CLIENT_MSGS_H */
