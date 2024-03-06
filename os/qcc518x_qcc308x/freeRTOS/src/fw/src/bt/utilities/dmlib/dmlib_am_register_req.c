/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_am_register_req
 *
 *  DESCRIPTION
 *      Build and send a DM_AM_REGISTER_REQ primitive to the DM cmd interface.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_am_register_req(
    phandle_t phandle
    )
{
    DM_AM_REGISTER_REQ_T *p_prim = zpnew(DM_AM_REGISTER_REQ_T);

    p_prim->type = DM_AM_REGISTER_REQ;
    p_prim->phandle = phandle;

    DM_PutMsg(p_prim);
}

