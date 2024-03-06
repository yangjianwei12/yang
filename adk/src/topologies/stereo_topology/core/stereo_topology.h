/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   stereo Topology
\ingroup    topologies
\brief      Stereo topology public interface.
*/

#ifndef STEREO_TOPOLOGY_H_
#define STEREO_TOPOLOGY_H_

#include "domain_message.h"
#include "handset_service.h"

/*! Definition of messages that Stereo Topology can send to clients. */
typedef enum
{
    STEREO_TOPOLOGY_STOP_CFM = STEREO_TOPOLOGY_MESSAGE_BASE,
    STEREO_TOPOLOGY_STOPPING_CFM,
    STEREO_TOPOLOGY_STARTED_CFM,
    STEREO_TOPOLOGY_STARTING_CFM,
    STEREO_TOPOLOGY_PEER_PAIR_CFM,
#ifdef ENABLE_LE_AUDIO_CSIP
    STEREO_TOPOLOGY_SIRK_UPDATE_CFM,
#endif
    STEREO_TOPOLOGY_FIND_ROLE_CFM,
    STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM,
    STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM,
    /*! This must be the final message */
    STEREO_TOPOLOGY_MESSAGE_END
} stereo_topology_message_t;
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(STEREO_TOPOLOGY, STEREO_TOPOLOGY_MESSAGE_END)

/*! Definition of status code returned by Stereo Topology. */
typedef enum
{
    /*! The operation has been successful */
    stereo_topology_status_success,

    /*! The requested operation has failed. */
    stereo_topology_status_fail,
} stereo_topology_status_t;

typedef enum
{
    stereo_find_role_no_peer = 0,
    stereo_find_role_acting_primary,
    stereo_find_role_primary,
    stereo_find_role_secondary,
}stereo_topology_find_role_t;


/*! Definition of the #STEREO_TOPOLOGY_STATE_CFM_T message. */
typedef struct
{
    stereo_topology_status_t        status;
} STEREO_TOPOLOGY_STATE_CFM_T;

/*! Definition of the #STEREO_TOPOLOGY_FIND_ROLE_CFM_T message. */
typedef struct
{
    stereo_topology_find_role_t        role;
} STEREO_TOPOLOGY_FIND_ROLE_CFM_T;

/*! Result of the #StereoTopology_Stop() operation.
    If this is not stereo_topology_status_success then the topology was not
    stopped cleanly within the time requested */
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_STOP_CFM_T;

typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_STOPPING_CFM_T;
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_STARTING_CFM_T;
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_STARTED_CFM_T;
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_PEER_PAIR_CFM_T;
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_PEER_PROFILE_CONN_CFM_T;
typedef STEREO_TOPOLOGY_STATE_CFM_T STEREO_TOPOLOGY_ENABLE_STANDALONE_CFM_T;


/*! \brief Initialise the  Stereo topology component

    \param init_task    Task to send init completion message (if any) to

    \returns TRUE
*/
bool StereoTopology_Init(Task init_task);


/*! \brief Start the  Stereo topology

    The topology will run semi-autonomously from this point.

    \param requesting_task Task to send messages to

    \returns TRUE
*/
bool StereoTopology_Start(Task requesting_task);


/*! \brief Register client task to receive  Stereo topology messages.

    \param[in] client_task Task to receive messages.
*/
void StereoTopology_RegisterMessageClient(Task client_task);


/*! \brief Unregister client task from Stereo topology.

    \param[in] client_task Task to unregister.
*/
void StereoTopology_UnRegisterMessageClient(Task client_task);


/*! \brief Stop the Stereo topology

    The topology will enter a known clean state then send a message to
    confirm.

    The device should be restarted after the STEREO_TOPOLOGY_STOP_CFM message
    is sent.

    \param requesting_task Task to send the response to

    \return TRUE
*/
bool StereoTopology_Stop(Task requesting_task);

/*! \brief Start LE Peer Pair

   The topology will start LE Advert and Scan which is required for peer pair

    \param requesting_task Task to send the response to
    \param timeout user configured timeout

    \return TRUE
*/

bool StereoTopology_StartPeerPair(Task requesting_task, uint8 timeout);

/*! \brief Start PFR

   The topology will trigger peer find role

    \param requesting_task Task to send the response to
    \param timeout user configured timeout

    \return TRUE
*/

bool StereoTopology_StartPeerFindRole(Task requesting_task, uint8 timeout);

/*! \brief Start Peer profile connection */
void StereoTopology_StartPeerProfileConn(Task requesting_task);

/*! \brief Start Peer BR/EDR ACL connection */
void StereoTopology_StartPeerAclConn(void);

/*! \brief Start disconnecting peer links and enable standalone */
void StereoTopology_EnableStereoStandalone(void);


#endif /* STEREO_TOPOLOGY_H_ */
