/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_set_afh_channel_class
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SET_AFH_CHANNEL_CLASS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_set_afh_channel_class(
    uint8_t * map,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T *p_prim = zpnew(DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T);
    p_prim->common.op_code = DM_HCI_SET_AFH_CHANNEL_CLASS_REQ;
    qbl_memscpy(p_prim->map, sizeof(p_prim->map), map, sizeof(p_prim->map));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

