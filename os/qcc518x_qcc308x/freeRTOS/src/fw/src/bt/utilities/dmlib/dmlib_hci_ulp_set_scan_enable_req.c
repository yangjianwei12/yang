/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_scan_enable_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_SCAN_ENABLE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_scan_enable_req(
    uint8_t scan_enable,
    uint8_t filter_duplicates,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_SCAN_ENABLE_REQ_T *prim = zpnew(DM_HCI_ULP_SET_SCAN_ENABLE_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_SCAN_ENABLE_REQ;
    prim->scan_enable = scan_enable;
    prim->filter_duplicates = filter_duplicates;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

