/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ext_adv_set_scan_resp_data_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      It is the responsibility of the caller to ensure that
 *      scan_resp_data_len does not exceed the maximum (251).
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ext_adv_set_scan_resp_data_req(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t scan_resp_data_len,
    uint8_t *scan_resp_data,
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, offset, scan_resp_data_element_len;
    DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ_T *prim = zpnew(DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ_T);

    if (scan_resp_data_len > HCI_ULP_SCAN_RESP_DATA_LENGTH)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    prim->common.op_code = DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ;
    prim->adv_handle = adv_handle;
    prim->operation = operation;
    prim->frag_preference = reserved;
    prim->scan_resp_data_len = scan_resp_data_len;

    for(offset = 0, index = 0; offset < prim->scan_resp_data_len;
                               index++, offset += scan_resp_data_element_len)
    {
        scan_resp_data_element_len = prim->scan_resp_data_len - offset;
        if(scan_resp_data_element_len > HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR)
            scan_resp_data_element_len = HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR;

        prim->scan_resp_data[index] = pmalloc(HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR);
        qbl_memscpy(prim->scan_resp_data[index], HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR,
                scan_resp_data + offset, scan_resp_data_element_len);
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
