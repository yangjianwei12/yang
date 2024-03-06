/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_create_big_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_CREATE_BIG_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_create_big_req(phandle_t   phandle,
                            context_t   con_context,
                            DM_BIG_CONFIG_PARAM_T   *big_config,
                            uint8_t     big_handle,
                            uint8_t     adv_handle,
                            uint8_t     num_bis,
                            uint8_t     encryption,
                            uint8_t     *broadcast_code,
                            DM_UPRIM_T  **pp_prim)
{
    DM_ISOC_CREATE_BIG_REQ_T *p_prim;
    uint8_t i;

    if (num_bis == 0 || num_bis > HCI_ULP_MAX_BIS_HANDLES)
    {
        return;
    }

    p_prim = zpnew(DM_ISOC_CREATE_BIG_REQ_T);
    p_prim->type = DM_ISOC_CREATE_BIG_REQ;
    p_prim->phandle = phandle;
    p_prim->con_context = con_context;
    if(big_config != NULL)
    {
        p_prim->big_config = *big_config;
    }
    p_prim->big_handle = big_handle;
    p_prim->adv_handle = adv_handle;
    p_prim->encryption = encryption;
    p_prim->num_bis = num_bis;

    for (i=0; i < 16; i++)
    {
        p_prim->broadcast_code[i] = broadcast_code[i];
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

