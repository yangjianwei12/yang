/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_key_request_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_KEY_REQUEST_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_sm_key_request_rsp(
    TYPED_BD_ADDR_T *addrt,
    uint16_t security_requirements,
    DM_SM_KEY_TYPE_T key_type,
    DM_SM_UKEY_T key,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_KEY_REQUEST_RSP_T *prim = pnew(DM_SM_KEY_REQUEST_RSP_T);

    prim->type = DM_SM_KEY_REQUEST_RSP;
    tbdaddr_copy(&prim->addrt, addrt);
    prim->security_requirements = security_requirements;
    prim->key_type = key_type;
    prim->key = key;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
