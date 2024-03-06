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
 *      dm_hci_set_csb
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SET_CSB primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_set_csb_req(
    uint8_t enable,
    uint8_t lt_addr,
    uint8_t lpo_allowed,
    hci_pkt_type_t packet_type,
    uint16_t interval_min,
    uint16_t interval_max,
    uint16_t supervision_timeout,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_SET_CSB_REQ_T *p_prim = pnew(DM_HCI_SET_CSB_REQ_T);

    p_prim->common.op_code = DM_HCI_SET_CSB_REQ;
    p_prim->enable = enable;
    p_prim->lt_addr = lt_addr;
    p_prim->lpo_allowed = lpo_allowed;
    p_prim->packet_type = packet_type;
    p_prim->interval_min = interval_min;
    p_prim->interval_max = interval_max;
    p_prim->supervision_timeout = supervision_timeout;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

