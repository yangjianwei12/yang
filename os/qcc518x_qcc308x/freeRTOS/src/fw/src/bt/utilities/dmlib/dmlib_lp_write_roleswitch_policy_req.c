/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_lp_write_roleswitch_policy_req
 *
 *  DESCRIPTION
 *      Build and send a DM_LP_WRITE_ROLESWITCH_POLICY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 * 
 *----------------------------------------------------------------------------*/
void dm_lp_write_roleswitch_policy_req(
    uint16_t    version,
    uint16_t    length,
    uint16_t    *rs_table,
    DM_UPRIM_T  **pp_prim
    )
{
    DM_LP_WRITE_ROLESWITCH_POLICY_REQ_T *prim =
        pnew(DM_LP_WRITE_ROLESWITCH_POLICY_REQ_T);

    prim->type = DM_LP_WRITE_ROLESWITCH_POLICY_REQ;
    prim->version = version;
    prim->length = length;
    prim->rs_table = rs_table;

    if (pp_prim == NULL)
        DM_PutMsg(prim);
    else
        *pp_prim = (DM_UPRIM_T*)prim;
}
