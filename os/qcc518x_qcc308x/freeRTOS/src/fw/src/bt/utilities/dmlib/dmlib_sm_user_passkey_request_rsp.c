/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_user_passkey_request_res
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_USER_PASSKEY_REQUEST_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_user_passkey_request_rsp(
    TP_BD_ADDR_T *tp_addrt,
    uint32_t   numeric_value,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_USER_PASSKEY_REQUEST_RSP_T *p_prim = pnew(DM_SM_USER_PASSKEY_REQUEST_RSP_T);

    p_prim->type = DM_SM_USER_PASSKEY_REQUEST_RSP;
    p_prim->numeric_value = numeric_value;
    tbdaddr_copy(&p_prim->tp_addrt.addrt, &tp_addrt->addrt);
    p_prim->tp_addrt.tp_type = tp_addrt->tp_type;
    
    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

