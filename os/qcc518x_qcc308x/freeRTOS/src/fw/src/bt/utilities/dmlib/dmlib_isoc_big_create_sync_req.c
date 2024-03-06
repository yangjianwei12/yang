/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_big_create_sync_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_BIG_CREATE_SYNC_REQ primitive.
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

void dm_isoc_big_create_sync_req(phandle_t   phandle,
                                 context_t   con_context,
                                 uint8_t     big_handle,
                                 uint16_t    sync_handle,
                                 uint8_t     encryption,
                                 uint8_t     *broadcast_code,
                                 uint8_t     mse,
                                 uint16_t    big_sync_timeout,
                                 uint8_t     num_bis,
                                 uint8_t     *bis,
                                 DM_UPRIM_T  **pp_prim)
{
    DM_ISOC_BIG_CREATE_SYNC_REQ_T *p_prim;
    uint8_t i;

    if (num_bis <= 0 || num_bis > HCI_ULP_MAX_BIS_HANDLES)
    {
        return;
    }

    p_prim = zpnew(DM_ISOC_BIG_CREATE_SYNC_REQ_T);
    p_prim->type = DM_ISOC_BIG_CREATE_SYNC_REQ;
    p_prim->phandle = phandle;
    p_prim->con_context = con_context;
    p_prim->big_handle = big_handle;
    p_prim->sync_handle = sync_handle;
    p_prim->encryption = encryption;

    for (i=0; i < 16; i++)
    {
        p_prim->broadcast_code[i] = broadcast_code[i];
    }

    p_prim->mse = mse;
    p_prim->big_sync_timeout = big_sync_timeout;

    if (num_bis)
    {
        p_prim->num_bis = num_bis;
        p_prim->bis = (uint8_t *) pmalloc((num_bis * sizeof(uint8_t)));
        for (i=0; i < num_bis; i++)
        {
            p_prim->bis[i] = bis[i];
        }
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

