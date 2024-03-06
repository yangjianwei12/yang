/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_read_sc_host_support_override_max_bd_addr_req
 *
 *  DESCRIPTION
 *      Build and send a DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_read_sc_host_support_override_max_bd_addr_req(phandle_t     phandle,
                                                      DM_UPRIM_T    **pp_prim)
{
    DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ_T *p_prim =
                    pnew(DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ_T);

    p_prim->type = DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ;
    p_prim->phandle = phandle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *)p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
