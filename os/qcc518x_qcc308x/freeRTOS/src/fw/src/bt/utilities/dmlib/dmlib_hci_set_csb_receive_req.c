/*******************************************************************************

Copyright (C) 2015 - 2021 Qualcomm Technologies International, Ltd.
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
 *      dmlib_hci_set_csb_receive
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SET_CSB_RECEIVE primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_set_csb_receive_req(
    uint8_t enable,
    BD_ADDR_T *p_bd_addr,
    uint8_t lt_addr,
    uint16_t interval,
    uint32_t clock_offset,
    uint32_t next_csb_clock,
    uint16_t supervision_timeout,
    uint8_t remote_timing_accuracy,
    uint8_t skip,
    hci_pkt_type_t packet_type,
    uint8_t *afh_channel_map,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_SET_CSB_RECEIVE_REQ_T *p_prim = pnew(DM_HCI_SET_CSB_RECEIVE_REQ_T);

    p_prim->common.op_code = DM_HCI_SET_CSB_RECEIVE_REQ;
    p_prim->enable = enable;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);
    p_prim->lt_addr = lt_addr;
    p_prim->interval = interval;
    p_prim->clock_offset = clock_offset;
    p_prim->next_csb_clock = next_csb_clock;
    p_prim->supervision_timeout = supervision_timeout;
    p_prim->remote_timing_accuracy = remote_timing_accuracy;
    p_prim->skip = skip;
    p_prim->packet_type = packet_type;
    qbl_memscpy(p_prim->afh_channel_map, sizeof(p_prim->afh_channel_map),
            afh_channel_map, sizeof(p_prim->afh_channel_map));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

