/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_adv_multi_enable_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_ADV_MULTI_ENABLE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_adv_multi_enable_req(
    uint8_t enable,
    uint8_t num_sets,
    EA_ENABLE_CONFIG_T config[],
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_ADV_MULTI_ENABLE_REQ_T *prim =
            zpnew(DM_ULP_EXT_ADV_MULTI_ENABLE_REQ_T);
    uint8_t index;

    prim->type = DM_ULP_EXT_ADV_MULTI_ENABLE_REQ;
    prim->enable = enable;
    prim->num_sets = num_sets;

    if ((num_sets <= DM_ULP_EXT_ADV_MAX_NUM_ENABLE) && (config != NULL))
    {
        for (index = 0; index < prim->num_sets; index++)
        {
            prim->config[index] = config[index];
        }
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
