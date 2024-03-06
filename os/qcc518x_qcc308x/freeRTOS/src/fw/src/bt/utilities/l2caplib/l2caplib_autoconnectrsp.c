/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_AUTO_CONNECT_RSP

    Build and send a L2CA_AUTO_CONNECT_RSP primitive to L2CAP.
    See L2CA_MOVE_AUTO_CONNECT_RSP_T for more details.
*/
void L2CA_AutoConnectRsp(l2ca_identifier_t    identifier,
                         l2ca_cid_t           cid,
                         l2ca_conn_result_t   response,
                         context_t            con_ctx,
                         uint16_t             conftab_length,
                         uint16_t            *conftab)
{
    L2CA_AUTO_CONNECT_RSP_T *prim = pnew(L2CA_AUTO_CONNECT_RSP_T);

    prim->type = L2CA_AUTO_CONNECT_RSP;
    prim->identifier = identifier;
    prim->cid = cid;
    prim->response = response;
    prim->con_ctx = con_ctx;
    prim->conftab_length = conftab_length;
    prim->conftab = conftab;

    L2CA_PutMsg(prim);
}
#endif 
