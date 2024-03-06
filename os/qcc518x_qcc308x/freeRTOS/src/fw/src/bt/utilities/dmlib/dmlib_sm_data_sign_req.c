/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dmlib_sm_data_sign_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_DATA_SIGN_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dmlib_sm_data_sign_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    context_t context,
    bool_t verify,
    uint16_t length,
    uint8_t *data,
    DM_UPRIM_T **pp_prim)
{
    DM_SM_DATA_SIGN_REQ_T *prim = zpnew(DM_SM_DATA_SIGN_REQ_T);

    prim->type = DM_SM_DATA_SIGN_REQ;
    prim->phandle = phandle;
    tbdaddr_copy(&prim->addrt, addrt);
    prim->context = context;
    prim->verify = verify;
    prim->length = length;
    prim->data = data;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
