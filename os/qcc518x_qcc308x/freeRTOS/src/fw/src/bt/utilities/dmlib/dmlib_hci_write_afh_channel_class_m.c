/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_afh_channel_class_m
 *
 *  DESCRIPTION
 *      Build and send a dm_hci_write_afh_channel_class_m primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_afh_channel_class_m(
    uint8_t class_mode,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_REQ_T *p_prim = zpnew(DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_REQ_T);
    p_prim->common.op_code = DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_REQ;
    p_prim->class_mode = class_mode;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
