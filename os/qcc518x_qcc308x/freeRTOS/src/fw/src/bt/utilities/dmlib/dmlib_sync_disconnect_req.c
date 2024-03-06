/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*! \brief Build and send a DM_SYNC_DISCONNECT_REQ primitive.
    \param handle HCI connection handle of SCO/eSCO link.
    \param reason HCI reason code for disconnection.
*/
void dm_sync_disconnect_req(hci_connection_handle_t handle, hci_reason_t reason)
{
    DM_SYNC_DISCONNECT_REQ_T *prim = pnew(DM_SYNC_DISCONNECT_REQ_T);

    prim->type              = DM_SYNC_DISCONNECT_REQ;
    prim->handle            = handle;
    prim->reason            = reason;
    DM_PutMsg(prim);
}
