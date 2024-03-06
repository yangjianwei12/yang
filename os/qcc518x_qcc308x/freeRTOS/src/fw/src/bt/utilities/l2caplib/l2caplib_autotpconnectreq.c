/*******************************************************************************

Copyright (C) 2017 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_AUTO_TP_CONNECT_REQ

    Build and send a L2CA_AUTO_TP_CONNECT_REQ primitive to L2CAP.
    See L2CA_AUTO_TP_CONNECT_REQ_T for more details.
*/
void L2CA_AutoTpConnectReq(l2ca_cid_t           cid,
                           psm_t                psm_local,
                           const TP_BD_ADDR_T   *p_tp_bd_addr,
                           psm_t                psm,
                           context_t            con_ctx,
                           l2ca_controller_t    remote_control,
                           l2ca_controller_t    local_control,
                           uint16_t             conftab_length,
                           uint16_t            *conftab)
{
    L2CA_AUTO_TP_CONNECT_REQ_T *prim = pnew(L2CA_AUTO_TP_CONNECT_REQ_T);

    prim->type = L2CA_AUTO_TP_CONNECT_REQ;
    prim->cid = cid;
    prim->psm_local = psm_local;
    prim->tp_addrt = *p_tp_bd_addr;
    prim->psm_remote = psm;
    prim->con_ctx = con_ctx;
    prim->remote_control = remote_control;
    prim->local_control = local_control;
    prim->conftab_length = conftab_length;
    prim->conftab = conftab;

    L2CA_PutMsg(prim);
}

#endif
