/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_change_packet_type_acl
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_CHANGE_CONN_PKT_TYPE_REQ primitive for a ACL connection.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_change_packet_type_acl(
    BD_ADDR_T *p_bd_addr,
    hci_pkt_type_t pkt_type
    )
{
    DM_HCI_CHANGE_ACL_CONN_PKT_TYPE_REQ_T *p_prim = zpnew(DM_HCI_CHANGE_ACL_CONN_PKT_TYPE_REQ_T);

    p_prim->type = DM_HCI_CHANGE_ACL_CONN_PKT_TYPE_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->pkt_type = pkt_type;

    DM_PutMsg(p_prim);
}

