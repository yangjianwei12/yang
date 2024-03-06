/*******************************************************************************

Copyright (C) 2015 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_read_authenticated_payload_timeout
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_read_authenticated_payload_timeout(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T  **pp_prim
    )
{
    DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_REQ_T *p_prim =
        zpnew(DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_REQ_T);

    p_prim->common.op_code = DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_REQ;
    tbdaddr_copy(&p_prim->tp_addrt.addrt, &tp_addrt->addrt);
    p_prim->tp_addrt.tp_type = tp_addrt->tp_type;

    if (pp_prim)
        *pp_prim = (DM_UPRIM_T *) p_prim;
    else
        DM_PutMsg(p_prim);
}
