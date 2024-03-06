/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_MAP_FIXED_CID_RSP for a fixed channel other than 2.

    Build and send a L2CA_MAP_FIXED_CID_RSP primitive to L2CAP.
    See \c L2CA_MAP_FIXED_CID_RSP_T for more details.
*/
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_SUPPORT
void L2CA_MapFixedCidRsp(l2ca_cid_t cid,
                         context_t  con_ctx,
                         l2ca_conflags_t flags)
{
    L2CA_MAP_FIXED_CID_RSP_T *prim = zpnew(L2CA_MAP_FIXED_CID_RSP_T);

    prim->type = L2CA_MAP_FIXED_CID_RSP;
    prim->cid = cid;
    prim->con_ctx = con_ctx;
    prim->flags = flags;

    L2CA_PutMsg(prim);
}
#endif
