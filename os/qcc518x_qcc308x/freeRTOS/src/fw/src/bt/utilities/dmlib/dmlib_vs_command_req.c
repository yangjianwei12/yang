/*******************************************************************************

Copyright (C) 2019 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_vs_command_req
 *
 *  DESCRIPTION
 *      Build and send a DM_VS_COMMAND_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_vs_command_req(
    phandle_t phandle,
    uint32_t context,
    uint16_t opcode_ocf,
    uint8_t data_length,
    uint8_t *data,
    uint16_t flow_control_flags,
    bool_t credit_not_required,
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, offset, part_length;
    DM_VS_COMMAND_REQ_T *p_prim = zpnew(DM_VS_COMMAND_REQ_T);

    p_prim->type = DM_VS_COMMAND_REQ;
    p_prim->opcode_ocf = opcode_ocf;
    p_prim->data_length = data_length;
    p_prim->flow_control_flags = flow_control_flags;
    p_prim->credit_not_required = credit_not_required;
    p_prim->context = context;
    p_prim->phandle = phandle;

    for(offset = 0, index = 0; offset < p_prim->data_length;
                               index++, offset += part_length)
    {
        part_length = p_prim->data_length - offset;
        if(part_length > HCI_VS_DATA_BYTES_PER_PTR)
            part_length = HCI_VS_DATA_BYTES_PER_PTR;

        p_prim->vs_data_part[index] = pmalloc(HCI_VS_DATA_BYTES_PER_PTR);
        qbl_memscpy(p_prim->vs_data_part[index], HCI_VS_DATA_BYTES_PER_PTR,
                data + offset, part_length);
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

