/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_remove_device_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_REMOVE_DEVICE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_remove_device_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_REMOVE_DEVICE_REQ_T *prim = zpnew(DM_SM_REMOVE_DEVICE_REQ_T);

    prim->type = DM_SM_REMOVE_DEVICE_REQ;
    prim->phandle = phandle;
    tbdaddr_copy(&prim->addrt, addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

