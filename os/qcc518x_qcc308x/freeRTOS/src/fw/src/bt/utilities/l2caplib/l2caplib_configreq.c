/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_CONFIG_REQ

    Build and send an L2CA_CONFIG_REQ primitive to L2CAP.
    NB: Ownership of any pointers will be transferred to the primitive!
    See \c L2CA_CONFIG_REQ_T for more details.
*/
#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
void L2CA_ConfigReq(l2ca_cid_t cid,
                    l2ca_options_t options,
                    l2ca_options_t hints,
                    uint8_t unknown_length,
                    uint8_t *unknown,
                    l2ca_mtu_t mtu,
                    l2ca_timeout_t flush_to,
                    L2CA_QOS_T *qos,
                    L2CA_FLOW_T *flow,
                    l2ca_fcs_t fcs,
                    L2CA_FLOWSPEC_T *flowspec,
                    l2ca_window_size_t ext_window)
{
    L2CA_CONFIG_REQ_T *prim = pnew(L2CA_CONFIG_REQ_T);

    prim->type = L2CA_CONFIG_REQ;
    prim->cid = cid;

    prim->config.options = options;
    prim->config.hints = hints;
    prim->config.unknown_length = unknown_length;
    prim->config.unknown = unknown;
    prim->config.mtu = mtu;
    prim->config.flush_to = flush_to;
    prim->config.qos = qos;
    prim->config.flow = flow;
    prim->config.fcs = fcs;
    prim->config.flowspec = flowspec;
    prim->config.ext_window = ext_window;

    L2CA_PutMsg(prim);
}
#endif 
