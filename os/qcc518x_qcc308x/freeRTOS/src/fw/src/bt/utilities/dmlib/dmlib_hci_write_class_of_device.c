/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_class_of_device
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_CLASS_OF_DEVICE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_class_of_device(
    uint24_t dev_class,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_CLASS_OF_DEVICE_REQ_T *p_prim = zpnew(DM_HCI_WRITE_CLASS_OF_DEVICE_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_CLASS_OF_DEVICE_REQ;
    p_prim->dev_class = dev_class;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

