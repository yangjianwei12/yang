/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_adv_set_random_address_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_ADV_SET_RANDOM_ADDR_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_adv_set_random_address_req(
    uint8_t      adv_handle,
    uint16_t     action,
    BD_ADDR_T   *random_addr,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_ADV_SET_RANDOM_ADDR_REQ_T *prim = zpnew(DM_ULP_EXT_ADV_SET_RANDOM_ADDR_REQ_T);

    prim->type = DM_ULP_EXT_ADV_SET_RANDOM_ADDR_REQ;
    prim->adv_handle = adv_handle;
    prim->action = action;
    if (random_addr != NULL)
    {
        bd_addr_copy(&prim->random_addr, random_addr);
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
