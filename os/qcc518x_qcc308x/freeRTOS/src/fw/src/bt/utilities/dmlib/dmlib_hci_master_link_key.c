/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_master_link_key
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_MASTER_LINK_KEY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_master_link_key(
    hci_key_flag_t link_key_type,   /* 0 = regular link key, 1 = temp link key */
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_MASTER_LINK_KEY_REQ_T *p_prim = zpnew(DM_HCI_MASTER_LINK_KEY_REQ_T);

    p_prim->common.op_code = DM_HCI_MASTER_LINK_KEY_REQ;
    p_prim->link_key_type = link_key_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

