/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_MOVE_CHANNEL_REQ

    Build and send a L2CA_MOVE_CHANNEL_REQ primitive to L2CAP.
    See \ref L2CA_MOVE_CHANNEL_REQ_T for more details.
*/
#ifdef INSTALL_AMP_SUPPORT
void L2CA_MoveChannelReq(l2ca_cid_t          cid,
                         l2ca_controller_t   remote_control,
                         l2ca_controller_t   local_control)
{
    L2CA_MOVE_CHANNEL_REQ_T *prim = pnew(L2CA_MOVE_CHANNEL_REQ_T);

    prim->type = L2CA_MOVE_CHANNEL_REQ;
    prim->cid = cid;
    prim->remote_control = remote_control;
    prim->local_control = local_control;

    L2CA_PutMsg(prim);
}
#endif
