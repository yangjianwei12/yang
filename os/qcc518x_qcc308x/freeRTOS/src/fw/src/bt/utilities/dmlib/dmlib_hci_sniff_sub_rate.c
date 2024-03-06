/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"



/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_sniff_sub_rate
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SNIFF_SUB_RATE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_sniff_sub_rate(
    BD_ADDR_T   *p_bd_addr,
    uint16_t    max_remote_latency,
    uint16_t    min_remote_timeout,
    uint16_t    min_local_timeout,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_SNIFF_SUB_RATE_REQ_T *p_prim = zpnew(DM_HCI_SNIFF_SUB_RATE_REQ_T);
    p_prim->common.op_code      = DM_HCI_SNIFF_SUB_RATE_REQ;
    p_prim->max_remote_latency  = max_remote_latency;
    p_prim->min_remote_timeout  = min_remote_timeout;
    p_prim->min_local_timeout   = min_local_timeout;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

