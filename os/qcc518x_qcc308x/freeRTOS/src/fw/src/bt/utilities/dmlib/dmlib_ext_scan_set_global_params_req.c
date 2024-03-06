/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_set_global_params_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_set_global_params_req(
    phandle_t phandle,
    uint8_t flags,
    uint8_t own_address_type,
    uint8_t scanning_filter_policy,
    uint8_t filter_duplicates,
    uint16_t scanning_phys,
    ES_SCANNING_PHY_T phys[],
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, num_of_phys = 0;
    DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_REQ_T);

    prim->type = DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_REQ;
    prim->phandle = phandle;
    prim->flags = flags;
    prim->own_address_type = own_address_type;
    prim->scanning_filter_policy = scanning_filter_policy;
    prim->filter_duplicates = filter_duplicates;
    prim->scanning_phys = scanning_phys;

    /* Work out the number of phys in parameter phys */
    for (index = 0; index < 16; index++)
    {
        if (scanning_phys & 1)
        {
            num_of_phys++;
        }
        scanning_phys = scanning_phys >> 1;
    }
    if (num_of_phys > DM_ULP_EXT_SCAN_MAX_SCANNING_PHYS)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    /* Copy each phy config into allocated memory */
    for (index = 0; index < num_of_phys; index++)
    {
        prim->phys[index] = phys[index];
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
