/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_terminate_big_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_TERMINATE_BIG_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_terminate_big_req(uint8_t     big_handle,
                               uint8_t     reason,
                               DM_UPRIM_T  **pp_prim)
{
    DM_ISOC_TERMINATE_BIG_REQ_T *p_prim;

    p_prim = zpnew(DM_ISOC_TERMINATE_BIG_REQ_T);
    p_prim->type = DM_ISOC_TERMINATE_BIG_REQ;
    p_prim->big_handle = big_handle;
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

