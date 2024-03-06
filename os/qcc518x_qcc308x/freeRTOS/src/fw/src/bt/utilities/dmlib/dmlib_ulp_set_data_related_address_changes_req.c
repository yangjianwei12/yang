/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ulp_set_data_related_address_changes_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ulp_set_data_related_address_changes_req(
    uint8_t adv_handle,
    uint8_t flags,
    uint8_t change_reasons,
    DM_UPRIM_T **pp_prim)
{
    DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ_T *prim =
            zpnew(DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ_T);

    prim->type = DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ;
    prim->adv_handle = adv_handle;
    prim->flags = flags;
    prim->change_reasons = change_reasons;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
