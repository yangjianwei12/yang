/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_vs_register_req
 *
 *  DESCRIPTION
 *      Build and send a DM_VS_REGISTER_REQ primitive to the DM cmd interface.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_vs_register_req(
    phandle_t phandle
    )
{
    DM_VS_REGISTER_REQ_T *p_prim = zpnew(DM_VS_REGISTER_REQ_T);

    p_prim->type = DM_VS_REGISTER_REQ;
    p_prim->phandle = phandle;

    DM_PutMsg(p_prim);
}

