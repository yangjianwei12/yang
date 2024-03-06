/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_crypto_hash_req
 *
 *  DESCRIPTION
 *      Build and send a DM_CRYPTO_HASH_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_crypto_hash_req(
    phandle_t phandle,
    uint8_t operation,
    uint8_t flags,
    context_t context,
    uint16_t data_size,
    uint16_t *data,
    DM_UPRIM_T **pp_prim
    )
{
    DM_CRYPTO_HASH_REQ_T *p_prim = pnew(DM_CRYPTO_HASH_REQ_T);

    p_prim->type      = DM_CRYPTO_HASH_REQ;
    p_prim->phandle   = phandle;
    p_prim->operation = operation;
    p_prim->flags     = flags;
    p_prim->context   = context;
    p_prim->data_size = data_size;

    qbl_memscpy(p_prim->data,
                sizeof(p_prim->data),
                data,
                data_size);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}