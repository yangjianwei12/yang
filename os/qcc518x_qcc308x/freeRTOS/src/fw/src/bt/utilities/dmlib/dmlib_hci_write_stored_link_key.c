/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_stored_link_key
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_STORED_LINK_KEY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_stored_link_key(
    uint8_t number_keys,
    /* Array of pmalloc()ed LINK_KEY_BD_ADDR_T pointers */
    LINK_KEY_BD_ADDR_T *ap_link_key_bd_addr[],
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t i;
    DM_HCI_WRITE_STORED_LINK_KEY_REQ_T *p_prim = zpnew(DM_HCI_WRITE_STORED_LINK_KEY_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_STORED_LINK_KEY_REQ;
    p_prim->number_keys = number_keys;
    for (i = 0; i < number_keys; i++)
    {
        p_prim->link_key_bd_addr[i] = ap_link_key_bd_addr[i];
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

