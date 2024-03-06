/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_flow_spec
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_FLOW_SPEC_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_flow_specification(
    hci_connection_handle_t handle,
    BD_ADDR_T *p_bd_addr,
    uint8_t flags,
    uint8_t flow_direction,
    uint8_t service_type,
    uint32_t token_rate,
    uint32_t token_bucket_size,
    uint32_t peak_bandwidth,
    uint32_t access_latency,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_FLOW_SPEC_REQ_T *p_prim = zpnew(DM_HCI_FLOW_SPEC_REQ_T);

    p_prim->common.op_code = DM_HCI_FLOW_SPEC_REQ;
    p_prim->handle = handle;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->flags = flags;
    p_prim->flow_direction = flow_direction;
    p_prim->service_type = service_type;
    p_prim->token_rate = token_rate;
    p_prim->token_bucket_size = token_bucket_size;
    p_prim->peak_bandwidth = peak_bandwidth;
    p_prim->access_latency = access_latency;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
