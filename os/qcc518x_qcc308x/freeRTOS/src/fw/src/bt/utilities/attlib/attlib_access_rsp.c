/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_access_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_ACCESS_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      This will take ownership of the block pointed to by value.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_access_rsp(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    att_result_t result,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ACCESS_RSP_T *prim = zpnew(ATT_ACCESS_RSP_T);

    prim->type = ATT_ACCESS_RSP;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->result = result;
    prim->size_value = size_value;
    prim->value = value;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}

#endif /* INSTALL_ATT_MODULE */
