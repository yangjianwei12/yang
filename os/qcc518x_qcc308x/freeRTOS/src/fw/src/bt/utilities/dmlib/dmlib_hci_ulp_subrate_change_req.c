/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_subrate_change_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SUBRATE_CHANGE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_subrate_change_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t subrate_min,
    uint16_t subrate_max,
    uint16_t max_latency,
    uint16_t continuation_num,
    uint16_t supervision_timeout,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SUBRATE_CHANGE_REQ_T *prim = zpnew(DM_HCI_ULP_SUBRATE_CHANGE_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SUBRATE_CHANGE_REQ;
    prim->subrate_min = subrate_min;
    prim->subrate_max = subrate_max;
    prim->max_latency = max_latency;
    prim->continuation_num = continuation_num;
    prim->supervision_timeout = supervision_timeout;

    tbdaddr_copy(&prim->addrt, addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

