/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_read_inquiry_response_tx_power_level_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_REQ
 *      primitive. If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_hci_read_inquiry_response_tx_power_level_req(
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_REQ_T *prim
        = zpnew(DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_REQ_T);

    prim->common.op_code = DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_REQ;

    if (pp_prim)
        *pp_prim = (DM_UPRIM_T*)prim;
    else
        DM_PutMsg(prim);
}
