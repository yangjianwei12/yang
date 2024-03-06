/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_change_packet_type_sco
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_CHANGE_CONN_PKT_TYPE_REQ primitive for a SCO connection.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_change_packet_type_sco(
    hci_connection_handle_t handle,
    hci_pkt_type_t pkt_type
    )
{
    DM_HCI_CHANGE_CONN_PKT_TYPE_REQ_T *p_prim = zpnew(DM_HCI_CHANGE_CONN_PKT_TYPE_REQ_T);

    p_prim->common.op_code = DM_HCI_CHANGE_CONN_PKT_TYPE_REQ;
    p_prim->handle = handle;
    p_prim->pkt_type = pkt_type;

    DM_PutMsg(p_prim);
}

