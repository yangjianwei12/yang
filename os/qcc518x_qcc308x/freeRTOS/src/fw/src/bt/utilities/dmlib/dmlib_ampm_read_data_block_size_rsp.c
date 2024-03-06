/*******************************************************************************

Copyright (C) 2010 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


void dm_ampm_read_data_block_size_rsp(const l2ca_controller_t local_amp_id,
                                      const uint8_t status,
                                      const bool_t fragmentable,
                                      const uint16_t max_pdu_length,
                                      const uint16_t max_acl_data_packet_length,
                                      const uint16_t data_block_length,
                                      const uint16_t total_num_data_blocks)
{
    DM_AMPM_READ_DATA_BLOCK_SIZE_RSP_T *p_prim = pnew(DM_AMPM_READ_DATA_BLOCK_SIZE_RSP_T);

    p_prim->local_amp_id = local_amp_id;
    p_prim->status = status;
    p_prim->fragmentable = fragmentable;
    p_prim->max_pdu_length = max_pdu_length;
    p_prim->max_acl_data_packet_length = max_acl_data_packet_length;
    p_prim->data_block_length = data_block_length;
    p_prim->total_num_data_blocks = total_num_data_blocks;

    dm_put_message(p_prim, DM_AMPM_READ_DATA_BLOCK_SIZE_RSP);
}
