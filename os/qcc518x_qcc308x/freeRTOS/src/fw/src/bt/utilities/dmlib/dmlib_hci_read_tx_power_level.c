/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_read_tx_power_level
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_TX_POWER_LEVEL_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_read_tx_power_level(
    TP_BD_ADDR_T *tp_addrt,
    uint8_t type,       /* 0=current 1=Max */
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_READ_TX_POWER_LEVEL_REQ_T *p_prim = zpnew(DM_HCI_READ_TX_POWER_LEVEL_REQ_T);

    p_prim->common.op_code = DM_HCI_READ_TX_POWER_LEVEL_REQ;
    tbdaddr_copy(&p_prim->tp_addrt.addrt, &tp_addrt->addrt);
    p_prim->tp_addrt.tp_type = tp_addrt->tp_type;
    p_prim->type = type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

