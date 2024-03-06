/*******************************************************************************

Copyright (C) 2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_generate_cross_trans_key_request_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_generate_cross_trans_key_request_rsp(
        TP_BD_ADDR_T     *tp_addrt,
        uint8_t           identifier,
        uint16_t          flags,
        DM_UPRIM_T      **pp_prim)
{
    DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP_T *prim =
                            zpnew(DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP_T);

    prim->type = DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP;
    if (tp_addrt)
    {
        tbdaddr_copy(&prim->tp_addrt.addrt, &tp_addrt->addrt);
        prim->tp_addrt.tp_type = tp_addrt->tp_type;
    }
    prim->identifier = identifier;
    prim->flags = flags;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

