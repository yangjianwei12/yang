/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_auto_flush_timeout
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_auto_flush_timeout(
    BD_ADDR_T *p_bd_addr,
    uint16_t timeout,       /* N x 0.625msec */
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_REQ_T *p_prim = zpnew(DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->timeout = timeout;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

