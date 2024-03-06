/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_set_host_channel_classification_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_set_host_channel_classification_req(
    uint8_t *channel_map,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_REQ_T *prim = zpnew(DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_REQ_T);

    prim->common.op_code = DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_REQ;
    qbl_memscpy(prim->channel_map, sizeof(prim->channel_map),
            channel_map, sizeof(prim->channel_map));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

