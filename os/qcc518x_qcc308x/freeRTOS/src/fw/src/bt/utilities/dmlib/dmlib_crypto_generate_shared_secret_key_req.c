/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_crypto_generate_shared_secret_key_req
 *
 *  DESCRIPTION
 *      Build and send a DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_crypto_generate_shared_secret_key_req(
    phandle_t phandle,
    uint8_t key_type,
    context_t context,
    uint16_t *local_private_key,
    uint16_t *remote_public_key,
    DM_UPRIM_T **pp_prim
    )
{
    DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ_T *p_prim = pnew(DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ_T);

    p_prim->type     = DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ;
    p_prim->phandle  = phandle;
    p_prim->key_type = key_type;
    p_prim->context  = context;

    qbl_memscpy(p_prim->local_private_key,
                sizeof(p_prim->local_private_key),
                local_private_key,
                sizeof(p_prim->local_private_key));
    qbl_memscpy(p_prim->remote_public_key,
                sizeof(p_prim->remote_public_key),
                remote_public_key,
                sizeof(p_prim->remote_public_key));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}