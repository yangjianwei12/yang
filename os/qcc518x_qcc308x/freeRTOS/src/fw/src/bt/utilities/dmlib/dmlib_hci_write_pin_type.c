/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_pin_type
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_PIN_TYPE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_pin_type(
    pin_type_t pin_type,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_PIN_TYPE_REQ_T *p_prim = zpnew(DM_HCI_WRITE_PIN_TYPE_REQ_T);

    p_prim->common.op_code = DM_HCI_WRITE_PIN_TYPE_REQ;
    p_prim->pin_type = pin_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

