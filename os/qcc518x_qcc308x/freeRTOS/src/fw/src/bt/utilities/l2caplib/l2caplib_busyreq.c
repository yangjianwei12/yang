/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_BUSY_REQ

    Build and send a L2CA_BUSY_REQ primitive to L2CAP.
    See \c L2CA_BUSY_REQ_T for more details.
*/
#ifdef INSTALL_L2CAP_ENHANCED_SUPPORT
void L2CA_BusyReq(l2ca_cid_t cid, bool_t busy)
{
    L2CA_BUSY_REQ_T *prim = pnew(L2CA_BUSY_REQ_T);
    
    prim->type = L2CA_BUSY_REQ;
    prim->cid = cid;
    prim->busy = busy;

    L2CA_PutMsg(prim);
}
#endif
