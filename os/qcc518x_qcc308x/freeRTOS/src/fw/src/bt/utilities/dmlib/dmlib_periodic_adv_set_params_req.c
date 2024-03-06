/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_adv_set_params_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_ADV_SET_PARAMS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_adv_set_params_req(
    uint8_t adv_handle,
    uint32_t flags,
    uint16_t periodic_adv_interval_min,
    uint16_t periodic_adv_interval_max,
    uint16_t periodic_adv_properties,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_ADV_SET_PARAMS_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_ADV_SET_PARAMS_REQ_T);

    prim->type = DM_ULP_PERIODIC_ADV_SET_PARAMS_REQ;
    prim->adv_handle = adv_handle;
    prim->flags = flags;
    prim->periodic_adv_interval_min = periodic_adv_interval_min;
    prim->periodic_adv_interval_max = periodic_adv_interval_max;
    prim->periodic_adv_properties = periodic_adv_properties;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
