/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_scan_parameters_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_SCAN_PARAMETERS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_scan_parameters_req(
    uint8_t scan_type,
    uint16_t scan_interval,
    uint16_t scan_window,
    uint8_t own_address_type,
    uint8_t scanning_filter_policy,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_SCAN_PARAMETERS_REQ_T *prim = zpnew(DM_HCI_ULP_SET_SCAN_PARAMETERS_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_SCAN_PARAMETERS_REQ;
    prim->scan_type = scan_type;
    prim->scan_interval = scan_interval;
    prim->scan_window = scan_window;
    prim->own_address_type = own_address_type;
    prim->scanning_filter_policy = scanning_filter_policy;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

