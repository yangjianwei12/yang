/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT

/*! \brief Send L2CA_CONNECT_REQ

    Build and send an L2CA_CONNECT_REQ primitive to L2CAP.
    See \c L2CA_CONNECT_REQ_T for more details.
*/
void L2CA_ConnectReq(const BD_ADDR_T *p_bd_addr,
                     psm_t psm_local,
                     psm_t psm_remote,
                     context_t con_ctx,
                     DM_SM_SERVICE_T *substitute_security_service)
{
    L2CA_CONNECT_REQ_T *prim = zpnew(L2CA_CONNECT_REQ_T);

    prim->type = L2CA_CONNECT_REQ;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);
    prim->psm_remote = psm_remote;
    prim->psm_local = psm_local;
    prim->con_ctx = con_ctx;

    if (substitute_security_service != NULL)
        prim->service = *substitute_security_service;

    L2CA_PutMsg(prim);
}

#endif 
