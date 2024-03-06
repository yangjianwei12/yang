/*******************************************************************************

Copyright (C) 2010 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


void dm_ampm_register_req(const phandle_t phandle)
{
    DM_AMPM_REGISTER_REQ_T *p_prim = pnew(DM_AMPM_REGISTER_REQ_T);

    p_prim->phandle = phandle;

    dm_put_message(p_prim, DM_AMPM_REGISTER_REQ);
}
