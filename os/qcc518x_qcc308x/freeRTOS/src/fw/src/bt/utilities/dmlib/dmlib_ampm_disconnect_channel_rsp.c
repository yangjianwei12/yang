/*******************************************************************************

Copyright (C) 2010 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


void dm_ampm_disconnect_channel_rsp(const DM_AMPM_DISCONNECT_CHANNEL_IND_T *const p_ind_prim,
                                    const hci_return_t status)
{
    DM_AMPM_DISCONNECT_CHANNEL_RSP_T *p_prim = pnew(DM_AMPM_DISCONNECT_CHANNEL_RSP_T);

    p_prim->cid             = p_ind_prim->cid;
    p_prim->local_amp_id    = p_ind_prim->local_amp_id;
    p_prim->status          = status;

    dm_put_message(p_prim, DM_AMPM_DISCONNECT_CHANNEL_RSP);
}
