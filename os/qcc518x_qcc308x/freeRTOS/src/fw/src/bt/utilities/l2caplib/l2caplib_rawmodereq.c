/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifdef INSTALL_L2CAP_RAW_SUPPORT

/*! \brief Send L2CA_RAW_MODE_REQ

    Compatibility form of function.
*/

void L2CA_RawModeReq(l2ca_cid_t cid,
                     bool_t cid_rx,
                     bool_t acl_rx,
                     phandle_t phandle)
{
    l2ca_raw_t raw_mode = L2CA_RAW_MODE_OFF;

    if (acl_rx)
        raw_mode = L2CA_RAW_MODE_BYPASS_CONNECTION;
    else if (cid_rx)
        raw_mode = L2CA_RAW_MODE_BYPASS_CHANNEL;

    L2CA_ExRawModeReq(cid, raw_mode, phandle);
}

/*! \brief Send L2CA_RAW_MODE_REQ

    Build and send a L2CA_RAW_MODE_REQ primitive to L2CAP.
    See \c L2CA_RAW_MODE_REQ_T for more details.
*/

void L2CA_ExRawModeReq(l2ca_cid_t cid,
                       l2ca_raw_t raw_mode,
                       phandle_t phandle)
{
    L2CA_RAW_MODE_REQ_T *prim = pnew(L2CA_RAW_MODE_REQ_T);

    prim->type = L2CA_RAW_MODE_REQ;
    prim->cid = cid;
    prim->raw_mode = raw_mode;
    prim->phandle = phandle;

    L2CA_PutMsg(prim);
}
#endif
