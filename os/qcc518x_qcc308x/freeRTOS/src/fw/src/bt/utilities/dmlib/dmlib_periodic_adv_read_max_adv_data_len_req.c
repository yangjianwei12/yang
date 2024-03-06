/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_adv_read_max_adv_data_len_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_adv_read_max_adv_data_len_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ_T);

    prim->type = DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ;
    prim->adv_handle = adv_handle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
