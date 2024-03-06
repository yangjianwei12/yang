/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_GETINFO_REQ

    Build and send an L2CA_GETINFO_REQ primitive to L2CAP.
    See \c L2CA_GETINFO_REQ_T for more details.
*/
void L2CA_GetInfoReq(const BD_ADDR_T *p_bd_addr,
                     phandle_t phandle,
                     uint16_t info_type,
                     context_t req_ctx,
                     l2ca_conflags_t flags)
{
    L2CA_GETINFO_REQ_T *prim = pnew(L2CA_GETINFO_REQ_T);

    prim->type = L2CA_GETINFO_REQ;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);
    prim->info_type = info_type;
    prim->phandle = phandle;
    prim->req_ctx = req_ctx;
    prim->flags = flags;

    L2CA_PutMsg(prim);
}
#endif 
