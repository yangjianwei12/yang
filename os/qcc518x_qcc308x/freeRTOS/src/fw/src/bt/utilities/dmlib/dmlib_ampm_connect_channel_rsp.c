/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


void dm_ampm_connect_channel_rsp(const DM_AMPM_CONNECT_CHANNEL_IND_T *const p_ind_prim,
                                 const amp_link_id_t logical_link_id,
                                 const uint8_t physical_link_id,
                                 const phandle_t hci_data_queue,
                                 const hci_return_t status)
{
    DM_AMPM_CONNECT_CHANNEL_RSP_T *p_prim = pnew(DM_AMPM_CONNECT_CHANNEL_RSP_T);

    qbl_memscpy(&p_prim->tx_flowspec, sizeof(L2CA_FLOWSPEC_T),
            &p_ind_prim->tx_flowspec, sizeof(L2CA_FLOWSPEC_T));
    qbl_memscpy(&p_prim->rx_flowspec, sizeof(L2CA_FLOWSPEC_T),
            &p_ind_prim->rx_flowspec, sizeof(L2CA_FLOWSPEC_T));
    bd_addr_copy(&p_prim->bd_addr, &p_ind_prim->bd_addr);

    p_prim->cid             = p_ind_prim->cid;
    p_prim->local_amp_id    = p_ind_prim->local_amp_id;
    p_prim->logical_link_id = logical_link_id;
    p_prim->physical_link_id = physical_link_id;
    p_prim->hci_data_queue = hci_data_queue;
    p_prim->status          = status;

    dm_put_message(p_prim, DM_AMPM_CONNECT_CHANNEL_RSP);
}
