/*******************************************************************************

Copyright (C) 2017 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_read_random_address_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_READ_RANDOM_ADDRESS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_read_random_address_req(
    TP_BD_ADDR_T *tp_peer_addrt,
    uint16_t flags,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_READ_RANDOM_ADDRESS_REQ_T *prim = zpnew(DM_SM_READ_RANDOM_ADDRESS_REQ_T);

    prim->type = DM_SM_READ_RANDOM_ADDRESS_REQ;
    prim->flags = flags;
    if(tp_peer_addrt)
    {
        tbdaddr_copy(&prim->tp_peer_addrt.addrt, &tp_peer_addrt->addrt);
        prim->tp_peer_addrt.tp_type = tp_peer_addrt->tp_type;
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

