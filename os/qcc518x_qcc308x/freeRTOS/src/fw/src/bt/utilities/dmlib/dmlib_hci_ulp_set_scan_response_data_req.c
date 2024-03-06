/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_scan_response_data_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      It is the caller's responsibility to ensure that 
 *      scan_response_data_len does not exceed the maximum (31).
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_scan_response_data_req(
    uint8_t scan_response_data_len,
    uint8_t *scan_response_data,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_REQ_T *prim = zpnew(DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_REQ;
    prim->scan_response_data_len = scan_response_data_len;
    qbl_memscpy(prim->scan_response_data,
            sizeof(prim->scan_response_data),
            scan_response_data,
            scan_response_data_len * sizeof(uint8_t));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

