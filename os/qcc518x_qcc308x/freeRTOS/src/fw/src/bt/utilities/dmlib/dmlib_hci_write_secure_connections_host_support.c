/*******************************************************************************

Copyright (C) 2013 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_secure_connections_host_support
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT_REQ
 *      primitive.  If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_secure_connections_host_support(
    uint8_t      secure_connections_host_support,
    DM_UPRIM_T **pp_prim )
{
    DM_HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT_REQ_T *p_prim =
       zpnew( DM_HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT_REQ_T );

    p_prim->common.op_code = DM_HCI_WRITE_SECURE_CONNECTIONS_HOST_SUPPORT_REQ;
    p_prim->secure_connections_host_support = secure_connections_host_support;

    if( pp_prim )
        *pp_prim = (DM_UPRIM_T *)p_prim;
    else
        DM_PutMsg(p_prim );
}
