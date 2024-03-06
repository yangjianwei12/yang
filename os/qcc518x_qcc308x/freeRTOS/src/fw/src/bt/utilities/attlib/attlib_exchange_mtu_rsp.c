/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_exchange_mtu_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_EXCHANGE_MTU_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_exchange_mtu_rsp(
    phandle_t phandle,
    uint16_t cid,
    uint16_t server_mtu,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_EXCHANGE_MTU_RSP_T *prim = zpnew(ATT_EXCHANGE_MTU_RSP_T);

    prim->type = ATT_EXCHANGE_MTU_RSP;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->server_mtu = server_mtu;

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
