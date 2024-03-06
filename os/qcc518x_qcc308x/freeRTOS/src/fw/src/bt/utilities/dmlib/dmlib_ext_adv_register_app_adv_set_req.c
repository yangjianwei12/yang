/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_adv_register_app_adv_set_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_adv_register_app_adv_set_req(
    uint8_t adv_handle,
    uint32_t flags,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_REQ_T *prim =
            zpnew(DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_REQ_T);

    prim->type = DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_REQ;
    prim->adv_handle = adv_handle;
    prim->flags = flags;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
