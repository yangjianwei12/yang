/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_adv_enable_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_ADV_ENABLE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_adv_enable_req(
    uint8_t adv_handle,
    uint16_t flags,
    uint8_t enable,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_ADV_ENABLE_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_ADV_ENABLE_REQ_T);

    prim->type = DM_ULP_PERIODIC_ADV_ENABLE_REQ;
    prim->adv_handle = adv_handle;
    prim->flags = flags;
    prim->enable = enable;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
