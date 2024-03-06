/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_MULTICAST_REQ

    Build and send an L2CA_MULTICAST_REQ primitive to L2CAP.
    See \c L2CA_MULTICAST_REQ_T for more details.
*/
void L2CA_MulticastReq(l2ca_cid_t *cids,
                       uint16_t length,
                       void *p_data)
{
    L2CA_MULTICAST_REQ_T *prim = pnew(L2CA_MULTICAST_REQ_T);

    prim->type = L2CA_MULTICAST_REQ;
    qbl_memscpy(prim->cids, sizeof(prim->cids), cids, sizeof(l2ca_cid_t) * L2CA_MAX_MULTICAST_CIDS);
    prim->length = length;
    prim->data = L2CA_MblkWrap(p_data, length);
    L2CA_PutMsg(prim);
}
#endif 
