/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_pagescan_activity
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_PAGESCAN_ACTIVITY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_pagescan_activity(
    uint16_t pagescan_interval,
    uint16_t pagescan_window,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_PAGESCAN_ACTIVITY_REQ_T *p_prim = zpnew(DM_HCI_WRITE_PAGESCAN_ACTIVITY_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_PAGESCAN_ACTIVITY_REQ;
    p_prim->pagescan_interval = pagescan_interval;
    p_prim->pagescan_window = pagescan_window;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

