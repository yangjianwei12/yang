/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_add_device_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_ADD_DEVICE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      This function has a variable number of arguments. The number of
 *      additional arguments after 'keys_present' is determined by the
 *      value of keys_present. keys_present should be set using the
 *      DM_SM_KEYS_PRESENT() macro in dm_prim.h. The arguments to that
 *      macro are taken from the DM_SM_KEYS_TYPE_T enum. For each key
 *      type used in that macro, the corresponding key (of type taken from
 *      the DM_SM_UKEY_T union) shall be included as an additional argument.
 *      The exception is that no more arguments are required if all
 *      remaining types are DM_SM_KEY_TYPE_NONE, i.e., you may stop adding
 *      arguments when you run out of keys.
 *
 *      For example, if you want to provide identity and signing information
 *      then you could call this function like this:
 *
 *      dm_sm_add_device_req(
 *              NULL,
 *              phandle,
 *              bd_addr,
 *              trust,
 *              security_requirements,
 *              encryption_key_size,
 *              DM_SM_KEYS_PRESENT(DM_SM_KEY_ID,
 *                                 DM_SM_KEY_SIGN,
 *                                 DM_SM_KEY_NONE,
 *                                 DM_SM_KEY_NONE,
 *                                 DM_SM_KEY_NONE,
 *                                 DM_SM_KEYS_REPLACE_EXISTING),
 *              id,
 *              sign);
 *
 *      Where id is of type DM_SM_KEY_ID_T* and sign is of type
 *      DM_SM_KEY_SIGN_T*. Note that all of the variable arguments
 *      have a pointer type, except for 'div', which is a uint16_t.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_add_device_req(
    DM_UPRIM_T **pp_prim,
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_SM_TRUST_T trust,
    uint16_t security_requirements,
    uint16_t encryption_key_size,
    unsigned int keys_present,
    ...
    )
{
    uint8_t i;
    va_list ap;
    DM_SM_ADD_DEVICE_REQ_T *prim = zpnew(DM_SM_ADD_DEVICE_REQ_T);

    prim->type = DM_SM_ADD_DEVICE_REQ;
    prim->phandle = phandle;
    tbdaddr_copy(&prim->addrt, addrt);
    prim->trust = trust;
    prim->keys.security_requirements = security_requirements;
    prim->keys.encryption_key_size = encryption_key_size;
    prim->keys.present = (uint16_t)keys_present;

    va_start(ap, keys_present);
    for (i = 0; i != DM_SM_MAX_NUM_KEYS && keys_present != 0;
                ++i, keys_present >>= DM_SM_NUM_KEY_BITS)
    {
        DM_SM_KEY_TYPE_T type = (DM_SM_KEY_TYPE_T)
            (keys_present & ((1 << DM_SM_NUM_KEY_BITS) - 1));

        switch (type)
        {
            case DM_SM_KEY_NONE:
                /* This one is a bit pointless. Throw their argument away
                   and set the pointer to NULL. */
                prim->keys.u[i].none = NULL;
                (void)va_arg(ap, void*);
                break;

            case DM_SM_KEY_ENC_BREDR:
                prim->keys.u[i].enc_bredr = va_arg(ap, DM_SM_KEY_ENC_BREDR_T*);
                break;

            case DM_SM_KEY_ENC_CENTRAL:
                prim->keys.u[i].enc_central =
                        va_arg(ap, DM_SM_KEY_ENC_CENTRAL_T*);
                break;

            case DM_SM_KEY_DIV:
                prim->keys.u[i].div = va_arg(ap, unsigned int);
                break;

            case DM_SM_KEY_SIGN:
                prim->keys.u[i].sign = va_arg(ap, DM_SM_KEY_SIGN_T*);
                break;

            case DM_SM_KEY_ID:
                prim->keys.u[i].id = va_arg(ap, DM_SM_KEY_ID_T*);
                break;

            default:
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
                BLUESTACK_BREAK_IF_PANIC_RETURNS
        }
    }

    va_end(ap);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

