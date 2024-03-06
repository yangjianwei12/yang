/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_host_num_completed_packets_sco
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_HOST_NUM_COMPLETED_PACKETS_REQ primitive for an
 *      SCO connection.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_host_num_completed_packets_sco(
    uint8_t num_handles,
    /* Array of pointers to 16 HANDLE_COMPLETE_T structures */
    HANDLE_COMPLETE_T *ap_handle_completes[]
    )
{
    DM_HCI_HOST_NUM_COMPLETED_PACKETS_REQ_T *p_prim = zpnew(DM_HCI_HOST_NUM_COMPLETED_PACKETS_REQ_T);
    uint8_t i;

    p_prim->common.op_code = DM_HCI_HOST_NUM_COMPLETED_PACKETS_REQ;
    p_prim->num_handles = num_handles;
    for (i = 0; i < num_handles; i++)
    {
        p_prim->num_completed_pkts_ptr[i] = ap_handle_completes[i];
    }

    DM_PutMsg(p_prim);
}

