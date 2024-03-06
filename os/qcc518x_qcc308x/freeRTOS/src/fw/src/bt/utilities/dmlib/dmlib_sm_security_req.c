/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_security_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_SECURITY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_sm_security_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    l2ca_conflags_t connection_flags,
    context_t context,
    uint16_t security_requirements,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_SECURITY_REQ_T *prim = zpnew(DM_SM_SECURITY_REQ_T);

    prim->type = DM_SM_SECURITY_REQ;
    prim->phandle = phandle;
    tbdaddr_copy(&prim->addrt, addrt);
    prim->connection_flags = (uint16_t)connection_flags;
    prim->context = context;
    prim->security_requirements = security_requirements;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
