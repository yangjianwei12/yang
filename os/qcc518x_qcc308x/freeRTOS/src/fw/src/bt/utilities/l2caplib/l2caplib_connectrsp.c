/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_CONNECT_RSP

    Build and send an L2CA_CONNECT_RSP primitive to L2CAP.
    See \c L2CA_CONNECT_RSP_T for more details.
*/
void L2CA_ConnectRsp(l2ca_cid_t cid,
                     l2ca_identifier_t identifier,
                     l2ca_conn_result_t response,
                     context_t con_ctx)
{
    L2CA_CONNECT_RSP_T *prim = pnew(L2CA_CONNECT_RSP_T);

    prim->type = L2CA_CONNECT_RSP;
    prim->identifier = identifier;
    prim->cid = cid;
    prim->response = response;
    prim->con_ctx = con_ctx;

    L2CA_PutMsg(prim);
}

#endif 
