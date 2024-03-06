/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_lp_settings
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_LP_SETTINGS primitive, changing only the LM
 *      link policy settings.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_lp_settings(
    BD_ADDR_T *p_bd_addr,
    link_policy_settings_t link_policy_settings,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ_T *p_prim = zpnew(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->link_policy_settings = link_policy_settings;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

