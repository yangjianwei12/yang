/*******************************************************************************

Copyright (C) 2017 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_privacy_mode_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_PRIVACY_MODE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_privacy_mode_req(
    TYPED_BD_ADDR_T *peer_addrt,
    uint8_t privacy_mode,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_PRIVACY_MODE_REQ_T *prim =
            zpnew(DM_HCI_ULP_SET_PRIVACY_MODE_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_PRIVACY_MODE_REQ;
    prim->peer_identity_address_type = tbdaddr_copy_to_bd_addr(&prim->peer_identity_address, peer_addrt);
    prim->privacy_mode = privacy_mode;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

