/*******************************************************************************

Copyright (C) 2018 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_register_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_REGISTER_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_register_req(
    phandle_t phandle,
    uint16_t isoc_type,
    context_t reg_context,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_REGISTER_REQ_T *p_prim = zpnew(DM_ISOC_REGISTER_REQ_T);

    p_prim->type = DM_ISOC_REGISTER_REQ;
    p_prim->phandle = phandle;
    p_prim->isoc_type = isoc_type;
    p_prim->reg_context = reg_context;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

