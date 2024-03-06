/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_crypto_decrypt_req
 *
 *  DESCRIPTION
 *      Build and send a DM_CRYPTO_DECRYPT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_crypto_decrypt_req(
    phandle_t phandle,
    uint8_t flags,
    context_t context,
    uint16_t *cipher_text,
    uint16_t *decryption_key,
    DM_UPRIM_T **pp_prim
    )
{
    DM_CRYPTO_DECRYPT_REQ_T *p_prim = pnew(DM_CRYPTO_DECRYPT_REQ_T);

    p_prim->type    = DM_CRYPTO_DECRYPT_REQ;
    p_prim->phandle = phandle;
    p_prim->flags   = flags;
    p_prim->context = context;

    qbl_memscpy(p_prim->cipher_text,
                sizeof(p_prim->cipher_text),
                cipher_text,
                sizeof(p_prim->cipher_text));
    qbl_memscpy(p_prim->decryption_key,
                sizeof(p_prim->decryption_key),
                decryption_key,
                sizeof(p_prim->decryption_key));

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}