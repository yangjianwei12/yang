/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_sirk_operation_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_SIRK_OPERATION_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_sirk_operation_req(
    phandle_t phandle,
    context_t context,
    TP_BD_ADDR_T *tp_addrt,
    uint8_t flags,
    uint8_t sirk_key[],
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_SIRK_OPERATION_REQ_T *p_prim = zpnew(DM_SM_SIRK_OPERATION_REQ_T);

    p_prim->type = DM_SM_SIRK_OPERATION_REQ;
    p_prim->phandle = phandle;
    p_prim->context = context;
    tbdaddr_copy(&p_prim->tp_addrt.addrt, &tp_addrt->addrt);
    p_prim->tp_addrt.tp_type = tp_addrt->tp_type;
    p_prim->flags = flags;
    qbl_memscpy(p_prim->sirk_key, sizeof(p_prim->sirk_key), sirk_key, sizeof(p_prim->sirk_key));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

