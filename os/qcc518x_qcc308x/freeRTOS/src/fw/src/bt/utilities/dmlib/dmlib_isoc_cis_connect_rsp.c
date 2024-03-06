/*******************************************************************************

Copyright (C) 2018-2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_cis_connect_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_CIS_CONNECT_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_cis_connect_rsp(
    phandle_t phandle,
    context_t con_context,
    hci_connection_handle_t cis_handle,
    uint8_t status,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_CIS_CONNECT_RSP_T *p_prim = zpnew(DM_ISOC_CIS_CONNECT_RSP_T);

    p_prim->type = DM_ISOC_CIS_CONNECT_RSP;
    p_prim->phandle = phandle;
    p_prim->con_context = con_context;
    p_prim->cis_handle = cis_handle;
    p_prim->status = status;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

