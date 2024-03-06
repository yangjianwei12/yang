/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_event_mask_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_EVENT_MASK_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_event_mask_req(
    hci_event_mask_t *ulp_event_mask,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_EVENT_MASK_REQ_T *prim = zpnew(DM_HCI_ULP_SET_EVENT_MASK_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_EVENT_MASK_REQ;
    qbl_memscpy(prim->ulp_event_mask, sizeof(prim->ulp_event_mask),
            ulp_event_mask, 2 * sizeof(hci_event_mask_t));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

