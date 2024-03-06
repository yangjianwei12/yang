/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_periodic_inquiry
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_PERIODIC_INQUIRY_MODE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_periodic_inquiry(
    uint16_t max_period_length,
    uint16_t min_period_length,
    uint24_t lap,
    uint8_t inquiry_length,
    uint8_t num_responses,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_PERIODIC_INQUIRY_MODE_REQ_T *p_prim = zpnew(DM_HCI_PERIODIC_INQUIRY_MODE_REQ_T);

    p_prim->common.op_code = DM_HCI_PERIODIC_INQUIRY_MODE_REQ;
    p_prim->max_period_length = max_period_length;
    p_prim->min_period_length = min_period_length;
    p_prim->lap = lap;
    p_prim->inquiry_length = inquiry_length;
    p_prim->num_responses = num_responses;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

