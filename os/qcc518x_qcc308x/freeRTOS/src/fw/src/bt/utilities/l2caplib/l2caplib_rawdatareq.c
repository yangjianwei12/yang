/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifdef INSTALL_L2CAP_RAW_SUPPORT

/*! \brief Send L2CA_RAW_DATA_REQ

    Build and send a L2CA_RAW_DATA_REQ primitive to L2CAP.
    See \c L2CA_RAW_DATA_REQ_T for more details.
*/    

void L2CA_RawDataReq(l2ca_cid_t cid,
                     uint16_t length,
                     void *p_data,
                     uint16_t raw_length,
                     uint16_t flush_to)
{
    L2CA_RAW_DATA_REQ_T *prim = pnew(L2CA_RAW_DATA_REQ_T);

    prim->type = L2CA_RAW_DATA_REQ;
    prim->cid = cid;
    prim->length = length;
    prim->raw_length = raw_length;
    prim->flush_to = flush_to;
    prim->data = L2CA_MblkWrap(p_data, length);

    L2CA_PutMsg(prim);
}
#endif
