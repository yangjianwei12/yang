/*!
\copyright  Copyright (c) 2015 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Interface to TWS Topology use of peer signalling marshalled message channel.
*/

#ifndef TWS_TOPOLOGY_PEER_SIG_H_
#define TWS_TOPOLOGY_PEER_SIG_H_

#include <peer_signalling.h>
#include <app/marshal/marshal_if.h>
#include <marshal_common.h>

#include "tws_topology_private.h"

typedef enum
{
    peer_left_topology,
    peer_joined_topology,
    disable_swap_role,
    enable_swap_role,
    invalid_cmd
}topology_peer_sig_cmd_t;

typedef struct
{
    topology_peer_sig_cmd_t topology_peer_sig_cmd;
}topology_ind_t;

typedef struct tws_topology_msg_empty_payload
{
    uint8 dummy;
} tws_topology_msg_empty_payload_t;

typedef tws_topology_msg_empty_payload_t tws_topology_msg_static_handover_req_t;

/* Create base list of marshal types the Earbud SM will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(topology_ind_t)\
    ENTRY(tws_topology_msg_static_handover_req_t) \

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const topology_ind_marshal_type_descriptors[];

/*! \brief Handle incoming message on the topology peer signalling channel. */
void TwsTopology_HandleMarshalledMsgChannelRxInd(tws_topology_sm_t *sm, PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind);

/*! \brief Handle confirmation that message was transmitted on topology peer signalling channel. */
void TwsTopology_HandleMarshalledMsgChannelTxCfm(tws_topology_sm_t *sm, PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm);

/*! \brief Peer signals the requested command */
void TwsTopology_PeerSignalTopologyCmd(topology_peer_sig_cmd_t topology_peer_sig_cmd);

#endif /* TWS_TOPOLOGY_PEER_SIG_H_ */
