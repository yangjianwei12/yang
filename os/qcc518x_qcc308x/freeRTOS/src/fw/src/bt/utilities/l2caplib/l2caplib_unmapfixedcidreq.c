/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_UNMAP_FIXED_CID_REQ for any type of fixed channel.

    Build and send a L2CA_UNMAP_FIXED_CID_REQ primitive to L2CAP.
    See \c L2CA_UNMAP_FIXED_CID_REQ_T for more details.
*/
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_BASE_SUPPORT
void L2CA_UnmapFixedCidReq(l2ca_cid_t cid)
{
    L2CA_UNMAP_FIXED_CID_REQ_T *prim = zpnew(L2CA_UNMAP_FIXED_CID_REQ_T);

    prim->type = L2CA_UNMAP_FIXED_CID_REQ;
    prim->cid = cid;

    L2CA_PutMsg(prim);
}
#endif
