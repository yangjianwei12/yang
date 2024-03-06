/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_set_event_mask
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SET_EVENT_MASK_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_set_event_mask(
    hci_event_mask_t event_mask_low,
    hci_event_mask_t event_mask_high,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_SET_EVENT_MASK_REQ_T *p_prim = zpnew(DM_HCI_SET_EVENT_MASK_REQ_T);

    p_prim->common.op_code = DM_HCI_SET_EVENT_MASK_REQ;
    p_prim->event_mask[0] = event_mask_low;
    p_prim->event_mask[1] = event_mask_high;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

