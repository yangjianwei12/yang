/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_MOVE_CHANNEL_RSP

    Build and send a L2CA_MOVE_CHANNEL_RSP primitive to L2CAP.
    See \ref L2CA_MOVE_CHANNEL_RSP_T for more details.
*/
#ifdef INSTALL_AMP_SUPPORT
void L2CA_MoveChannelRsp(l2ca_identifier_t   identifier,
                         l2ca_cid_t          cid,
                         l2ca_move_result_t  response)
{
    L2CA_MOVE_CHANNEL_RSP_T *prim = pnew(L2CA_MOVE_CHANNEL_RSP_T);

    prim->type = L2CA_MOVE_CHANNEL_RSP;
    prim->identifier = identifier;
    prim->cid = cid;
    prim->response = response;

    L2CA_PutMsg(prim);
}
#endif
