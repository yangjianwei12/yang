/*!
\copyright  Copyright (c) 2015 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Implementation of TWS Topology use of peer signalling marshalled message channel.
*/

#include "tws_topology_peer_sig.h"
#include "tws_topology_private.h"
#include "tws_topology.h"

#include <marshal_common.h>
#include <marshal.h>
#include <timestamp_event.h>
#include "panic.h"
#include "logging.h"

const marshal_type_descriptor_t marshal_type_descriptor_topology_ind_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(topology_ind_t);

const marshal_type_descriptor_t marshal_type_descriptor_tws_topology_msg_static_handover_req_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(tws_topology_msg_static_handover_req_t);

#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const topology_ind_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};

void TwsTopology_PeerSignalTopologyCmd(topology_peer_sig_cmd_t topology_peer_sig_cmd)
{
    if(appDeviceIsPeerConnected())
    {
        topology_ind_t* msg = PanicUnlessMalloc(sizeof(topology_ind_t));
        memset(msg, 0 , sizeof(*msg));
        msg->topology_peer_sig_cmd = topology_peer_sig_cmd;
        appPeerSigMarshalledMsgChannelTx(TwsTopologyGetTask(),
                                         PEER_SIG_MSG_CHANNEL_TOPOLOGY,
                                         msg,
                                         MARSHAL_TYPE_topology_ind_t);
    }
}

void TwsTopology_HandleMarshalledMsgChannelRxInd(tws_topology_sm_t *sm, PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE_topology_ind_t:
        {
            DEBUG_LOG_ALWAYS("TwsTopology_HandleMarshalledMsgChannelRxInd MARSHAL_TYPE_topology_ind_t");
            topology_ind_t* msg = (topology_ind_t*)ind->msg;
            if(msg->topology_peer_sig_cmd == peer_left_topology)
            {
                TwsTopology_ClearPeerJoinStatus();
            }
            else if(msg->topology_peer_sig_cmd == peer_joined_topology)
            {
                TwsTopology_SetPeerJoinStatus();
            }
            else if(msg->topology_peer_sig_cmd == disable_swap_role)
            {
                twsTopology_SetRoleSwapSupport(FALSE);
            }
            else if(msg->topology_peer_sig_cmd == enable_swap_role)
            {
                twsTopology_SetRoleSwapSupport(TRUE);
            }
            else
            {
                Panic();
            }
        }
        break;

        case MARSHAL_TYPE(tws_topology_msg_static_handover_req_t):
            {
                /* On fast in case events, secondary would have gone in case as well immediately
                 * after primary, before we complete the static handover, so complete this handover
                 * even if the secondary state moves to idle */
                PanicFalse(sm->state == TWS_TOPOLOGY_STATE_SECONDARY || sm->target_state == TWS_TOPOLOGY_STATE_IDLE);

                DEBUG_LOG_INFO("TwsTopology_HandleMarshalledMsgChannelRxInd, tws_topology_static_handover_req_t");
                TimestampEvent(TIMESTAMP_EVENT_ROLE_SWAP_COMMAND_RECEIVED);

                twsTopology_SmHandleStaticHandoverToPrimary(sm);
            }
            break;

        default:
            break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

void TwsTopology_HandleMarshalledMsgChannelTxCfm(tws_topology_sm_t *sm, PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    DEBUG_LOG_ALWAYS("TwsTopology_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);

    UNUSED(sm);

    DEBUG_LOG_INFO("TwsTopology_HandleMarshalledMsgChannelTxCfm sts:%u type:%d role:%d", cfm->status, cfm->type, TwsTopology_IsRolePrimary()?tws_topology_role_primary:tws_topology_role_secondary);

    if (cfm->type == MARSHAL_TYPE(tws_topology_msg_static_handover_req_t))
    {
        PanicFalse(sm->state == TWS_TOPOLOGY_STATE_STATIC_HANDOVER);
        twsTopology_SmHandleStaticHandoverToSecondary(sm);
    }
}
