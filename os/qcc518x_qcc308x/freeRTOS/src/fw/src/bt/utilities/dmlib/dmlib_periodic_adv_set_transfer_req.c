/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_adv_set_transfer_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_ADV_SET_TRANSFER_REQ_T primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_adv_set_transfer_req(
    phandle_t       phandle,
    TYPED_BD_ADDR_T *addrt,
    uint16_t        service_data,
    uint8_t         adv_handle,
    DM_UPRIM_T      **pp_prim
    )
{
    DM_ULP_PERIODIC_ADV_SET_TRANSFER_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_ADV_SET_TRANSFER_REQ_T);

    prim->type = DM_ULP_PERIODIC_ADV_SET_TRANSFER_REQ;
    prim->phandle = phandle;
    prim->addrt = *addrt;
    prim->service_data = service_data;
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
