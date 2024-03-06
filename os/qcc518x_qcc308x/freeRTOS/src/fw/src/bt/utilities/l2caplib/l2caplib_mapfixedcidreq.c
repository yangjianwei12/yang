/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_MAP_FIXED_CID_REQ for a fixed channel other than 2.

    Build and send a L2CA_MAP_FIXED_CID_REQ primitive to L2CAP.
    See \c L2CA_MAP_FIXED_CID_REQ_T for more details.
*/
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_SUPPORT
void L2CA_MapFixedCidReq(const TYPED_BD_ADDR_T *addrt,
                         l2ca_cid_t fixed_cid,
                         context_t  con_ctx,
                         l2ca_conflags_t flags)
{
    L2CA_MAP_FIXED_CID_REQ_T *prim = zpnew(L2CA_MAP_FIXED_CID_REQ_T);

    prim->type = L2CA_MAP_FIXED_CID_REQ;
    tbdaddr_copy(&prim->addrt, addrt);
    prim->fixed_cid = fixed_cid;
    prim->con_ctx = con_ctx;
    prim->flags = flags;

    L2CA_PutMsg(prim);
}
#endif
