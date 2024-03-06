/*!

Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.

All Rights Reserved.

Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file dmlib_controller_ready_ntf.c

*/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_controller_ready_ntf
 *
 *  DESCRIPTION
 *      Build and send a DM_CONTROLLER_READY_NTF primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *      This being a notification needs no phandle.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_controller_ready_ntf(
     const uint8_t     status,
     DM_UPRIM_T  **pp_prim )
{
    DM_CONTROLLER_READY_NTF_T *p_prim = pnew(DM_CONTROLLER_READY_NTF_T);
    p_prim->type                      = DM_CONTROLLER_READY_NTF;
    p_prim->status                    = status; /* Success is 0x00 */

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

