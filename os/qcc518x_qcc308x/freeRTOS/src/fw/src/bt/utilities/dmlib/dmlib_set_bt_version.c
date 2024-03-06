/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_bt_version
 *
 *  DESCRIPTION
 *      Build and send a DM_SET_BT_VERSION_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_bt_version(
    uint8_t     version,
    DM_UPRIM_T  **pp_prim
    )
{
    DM_SET_BT_VERSION_REQ_T *p_prim = pnew(DM_SET_BT_VERSION_REQ_T);
    p_prim->type                    = DM_SET_BT_VERSION_REQ;
    p_prim->version                 = version;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

