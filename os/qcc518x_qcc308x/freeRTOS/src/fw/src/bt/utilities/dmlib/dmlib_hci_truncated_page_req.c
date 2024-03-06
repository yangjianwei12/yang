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
 *      dmlib_hci_truncated_page
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_TRUNCATED_PAGE primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_truncated_page_req(
    BD_ADDR_T *p_bd_addr,
    page_scan_rep_mode_t page_scan_rep_mode,
    clock_offset_t clock_offset,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_TRUNCATED_PAGE_REQ_T *p_prim = pnew(DM_HCI_TRUNCATED_PAGE_REQ_T);

    p_prim->common.op_code = DM_HCI_TRUNCATED_PAGE_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->page_scan_rep_mode = page_scan_rep_mode;
    p_prim->clock_offset = clock_offset;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

