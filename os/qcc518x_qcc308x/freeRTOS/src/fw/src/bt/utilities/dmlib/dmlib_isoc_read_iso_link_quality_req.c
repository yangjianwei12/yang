/*******************************************************************************

Copyright (C) 2020 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_read_iso_link_quality_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_READ_ISO_LINK_QUALITY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_read_iso_link_quality_req(
    hci_connection_handle_t handle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_READ_ISO_LINK_QUALITY_REQ_T *p_prim;


    p_prim = zpnew(DM_ISOC_READ_ISO_LINK_QUALITY_REQ_T);
    p_prim->type = DM_ISOC_READ_ISO_LINK_QUALITY_REQ;
    p_prim->handle = handle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

