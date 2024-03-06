/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_write_sc_host_support_override_req
 *
 *  DESCRIPTION
 *      Build and send a DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_write_sc_host_support_override_req(phandle_t    phandle,
                                           BD_ADDR_T    *p_bd_addr,
                                           uint8_t       host_support_override,
                                           DM_UPRIM_T   **pp_prim)
{
    DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ_T *p_prim =
                                 pnew(DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ_T);

    p_prim->type = DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ;
    p_prim->phandle = phandle;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->host_support_override = host_support_override;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *)p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
