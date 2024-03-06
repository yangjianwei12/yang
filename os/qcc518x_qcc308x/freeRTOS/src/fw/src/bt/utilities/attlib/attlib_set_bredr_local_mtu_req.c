/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_set_bredr_local_mtu_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_SET_BREDR_LOCAL_MTU_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_set_bredr_local_mtu_req(
    phandle_t phandle,
    context_t context,
    uint16_t mtu,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_SET_BREDR_LOCAL_MTU_REQ_T *prim = zpnew(ATT_SET_BREDR_LOCAL_MTU_REQ_T);

    prim->type = ATT_SET_BREDR_LOCAL_MTU_REQ;

    prim->phandle = phandle;
    prim->context = context;
    prim->mtu = mtu;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}
#endif /* INSTALL_ATT_MODULE */
