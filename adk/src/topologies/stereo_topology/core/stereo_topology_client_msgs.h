/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to STEREO Topology utility functions for sending messages to clients.
*/

#ifndef STEREO_TOPOLOGY_CLIENT_MSGS_H
#define STEREO_TOPOLOGY_CLIENT_MSGS_H
#include <handset_service.h>
#include "stereo_topology.h"


/*! \brief Send confirmation message to the task which called #StereoTopology_Stop().
    \param[in] Status Status of the stop operation.

    \note It is expected that the task will be the application SM task.
*/
void StereoTopology_SendStopCfm(stereo_topology_status_t status);

/*! \brief Send indication to registered clients that stereo topology is stopping.
    \param[in] Status Status of the stopping operation.
*/
void StereoTopology_SendStoppingCfm(stereo_topology_status_t status);

/*! \brief Send indication to registered clients that stereo topology has started.
    \param[in] Status Status of the started operation.
*/
void StereoTopology_SendStartedCfm(stereo_topology_status_t status);

/*! \brief Send indication to registered clients that stereo topology is starting.
    \param[in] Status Status of the starting operation.
*/
void StereoTopology_SendStartingCfm(stereo_topology_status_t status);

/*! \brief Send indication to registered clients that peer pairing is done.
    \param[in] Status Status of the peer pair operation.
*/
void StereoTopology_SendPeerPairCfm(stereo_topology_status_t status);

void StereoTopology_SendFindRoleCfm(const stereo_topology_find_role_t* role);

void StereoTopology_SendEnableStandaloneCfm(void);


void StereoTopology_SendPeerProileConnCfm(stereo_topology_status_t status);

#endif /* STEREO_TOPOLOGY_CLIENT_MSGS_H */
