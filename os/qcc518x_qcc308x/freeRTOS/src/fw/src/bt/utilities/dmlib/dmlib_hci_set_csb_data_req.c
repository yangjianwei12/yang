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
 *      dm_hci_set_csb_data
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_SET_CSB_DATA primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_set_csb_data_req(
    uint8_t lt_addr,
    uint8_t fragment,
    uint8_t data_length,
    uint8_t *data_part,
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, offset, part_length;
    DM_HCI_SET_CSB_DATA_REQ_T *p_prim = zpnew(DM_HCI_SET_CSB_DATA_REQ_T);

    p_prim->common.op_code = DM_HCI_SET_CSB_DATA_REQ;
    p_prim->lt_addr = lt_addr;
    p_prim->fragment = fragment;
    p_prim->data_length = data_length;

    for(offset = 0, index = 0; offset < p_prim->data_length;
                               index++, offset += part_length)
    {
        part_length = p_prim->data_length - offset;
        if(part_length > HCI_SET_CSB_DATA_BYTES_PER_PTR)
            part_length = HCI_SET_CSB_DATA_BYTES_PER_PTR;

        p_prim->data_part[index] = pmalloc(HCI_SET_CSB_DATA_BYTES_PER_PTR);
        qbl_memscpy(p_prim->data_part[index], HCI_SET_CSB_DATA_BYTES_PER_PTR,
                data_part + offset, part_length);
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

