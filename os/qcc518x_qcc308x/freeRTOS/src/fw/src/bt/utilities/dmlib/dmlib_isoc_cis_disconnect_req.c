/*******************************************************************************

Copyright (C) 2018 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_cis_disconnect_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_CIS_DISCONNECT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_cis_disconnect_req(
    hci_connection_handle_t cis_handle,
    uint8_t reason,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_CIS_DISCONNECT_REQ_T *p_prim = zpnew(DM_ISOC_CIS_DISCONNECT_REQ_T);

    p_prim->type = DM_ISOC_CIS_DISCONNECT_REQ;
    p_prim->cis_handle = cis_handle;
    p_prim->reason = reason;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

