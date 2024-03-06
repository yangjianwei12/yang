/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_add_device_to_white_list_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_add_device_to_white_list_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_REQ_T *prim =
            zpnew(DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_REQ_T);

    prim->common.op_code = DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_REQ;
    prim->address_type = tbdaddr_copy_to_bd_addr(&prim->address, addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

