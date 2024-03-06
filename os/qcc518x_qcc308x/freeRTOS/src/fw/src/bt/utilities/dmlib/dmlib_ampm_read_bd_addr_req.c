/*******************************************************************************

Copyright (C) 2010 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


void dm_ampm_read_bd_addr_req(void)
{
    DM_AMPM_READ_BD_ADDR_REQ_T *p_prim = pnew(DM_AMPM_READ_BD_ADDR_REQ_T);

    dm_put_message(p_prim, DM_AMPM_READ_BD_ADDR_REQ);
}
