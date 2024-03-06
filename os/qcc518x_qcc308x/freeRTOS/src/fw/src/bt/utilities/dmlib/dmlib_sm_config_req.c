/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_config_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_CONFIG_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_config_req(phandle_t phandle,
                      uint16_t config_mask,
                      uint8_t length,
                      void *params,
                      DM_UPRIM_T **pp_prim)
{
    DM_SM_CONFIG_REQ_T *p_prim = pnew(DM_SM_CONFIG_REQ_T);

    p_prim->type        = DM_SM_CONFIG_REQ;
    p_prim->phandle     = phandle;
    p_prim->config_mask = config_mask;
    p_prim->length      = length;
    p_prim->params      = params;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

