/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_encrypt_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_ENCRYPT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_encrypt_req(
    uint8_t *aes_key,
    uint8_t *plaintext_data,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_ENCRYPT_REQ_T *prim = zpnew(DM_HCI_ULP_ENCRYPT_REQ_T);

    prim->common.op_code = DM_HCI_ULP_ENCRYPT_REQ;
    qbl_memscpy(prim->aes_key, sizeof(prim->aes_key), aes_key,
            16 * sizeof(uint8_t));
    qbl_memscpy(prim->plaintext_data, sizeof(prim->plaintext_data),
            plaintext_data, 16 * sizeof(uint8_t));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

