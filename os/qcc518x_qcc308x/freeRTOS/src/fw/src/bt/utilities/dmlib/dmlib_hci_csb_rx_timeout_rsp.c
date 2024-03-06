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
 *      dm_hci_csb_rx_timeout_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_CSB_RX_TIMEOUT_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_csb_rx_timeout_rsp(
    BD_ADDR_T *p_bd_addr,
    uint8_t lt_addr,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_CSB_RX_TIMEOUT_RSP_T *p_prim = pnew(DM_HCI_CSB_RX_TIMEOUT_RSP_T);

    p_prim->type = DM_HCI_CSB_RX_TIMEOUT_RSP;
    p_prim->lt_addr = lt_addr;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

