/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_crypto_aes_ctr_req
 *
 *  DESCRIPTION
 *      Build and send a DM_CRYPTO_AES_CTR_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_crypto_aes_ctr_req(
    phandle_t phandle,
    context_t context,
    uint32_t counter,
    dm_crypto_flags_t flags,
    uint16_t *secret_key,
    uint16_t *nonce,
    uint16_t data_len,
    uint16_t *data,
    DM_UPRIM_T **pp_prim
    )
{
    DM_CRYPTO_AES_CTR_REQ_T *p_prim = pnew(DM_CRYPTO_AES_CTR_REQ_T);
    uint16_t data_size = data_len * sizeof(uint16_t);

    p_prim->type     = DM_CRYPTO_AES_CTR_REQ;
    p_prim->phandle  = phandle;
    p_prim->context  = context;
    p_prim->counter  = counter;
    p_prim->flags    = flags;
    p_prim->data_len = data_len;
    p_prim->data     = pmalloc(data_size);

    qbl_memscpy(p_prim->secret_key,
                sizeof(p_prim->secret_key),
                secret_key,
                sizeof(p_prim->secret_key));
    qbl_memscpy(p_prim->nonce,
                sizeof(p_prim->nonce),
                nonce,
                sizeof(p_prim->nonce));
    qbl_memscpy(p_prim->data,
                data_size,
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