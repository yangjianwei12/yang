/*******************************************************************************

Copyright (C) 2015 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*============================================================================*
    Private Data Types
 *============================================================================*/
/* None */

/*============================================================================*
    Private Data
 *============================================================================*/
/* None */

/*============================================================================*
    Private Function Prototypes
 *============================================================================*/
/* None */

/*============================================================================*
    Public Function Implementations
 *============================================================================*/


/*----------------------------------------------------------------------------*
 *  NAME
 *      dmlib_hci_receive_synchronization_train
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_RECEIVE_SYNCHRONIZATION_TRAIN primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_receive_synchronization_train_req(
    BD_ADDR_T *p_bd_addr,
    uint16_t sync_scan_timeout,
    uint16_t sync_scan_window,
    uint16_t sync_scan_interval,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_RECEIVE_SYNCHRONIZATION_TRAIN_REQ_T *p_prim =
                           pnew(DM_HCI_RECEIVE_SYNCHRONIZATION_TRAIN_REQ_T);

    p_prim->common.op_code = DM_HCI_RECEIVE_SYNCHRONIZATION_TRAIN_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->sync_scan_timeout = sync_scan_timeout;
    p_prim->sync_scan_window = sync_scan_window;
    p_prim->sync_scan_interval = sync_scan_interval;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

