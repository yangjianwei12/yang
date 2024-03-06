/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_test_end_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_TEST_END_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_test_end_req(
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_TEST_END_REQ_T *prim = zpnew(DM_HCI_ULP_TEST_END_REQ_T);

    prim->common.op_code = DM_HCI_ULP_TEST_END_REQ;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

