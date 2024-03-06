/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_read_stored_link_key
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_STORED_LINK_KEY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_read_stored_link_key(
    BD_ADDR_T *p_bd_addr,       /* Optional, can be NULL */
    read_all_flag_t read_all,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_READ_STORED_LINK_KEY_REQ_T *p_prim = zpnew(DM_HCI_READ_STORED_LINK_KEY_REQ_T);

    p_prim->common.op_code = DM_HCI_READ_STORED_LINK_KEY_REQ;
    if (p_bd_addr)
    {
        bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    }
    p_prim->read_all = read_all;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

