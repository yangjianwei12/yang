/*******************************************************************************

Copyright (C) 2018-2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_cis_connect_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_CIS_CONNECT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Parameter cis_config_array is an array of pointer for
 *      CIS configuration, caller of this API shall pass the array with 
 *      num_cis number elements with valid pointers. Number of CIS shall
 *      be atleast 1 and maximum shall be DM_MAX_SUPPORTED_CIS.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.

 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_cis_connect_req(
    phandle_t phandle,
    context_t con_context,
    uint8_t cis_count,
    DM_CIS_CONNECTION_T *cis_conn[],
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_CIS_CONNECT_REQ_T *p_prim;
    uint16_t i;

    if(cis_count == 0 || cis_count > DM_MAX_SUPPORTED_CIS ||
        cis_conn == NULL)
    {
        return;
    }

    p_prim = zpnew(DM_ISOC_CIS_CONNECT_REQ_T);
    p_prim->type = DM_ISOC_CIS_CONNECT_REQ;
    p_prim->phandle = phandle;
    p_prim->con_context = con_context;
    p_prim->cis_count = cis_count;
    for (i=0; i < cis_count; i++)
    {
        p_prim->cis_conn[i] = cis_conn[i];
        cis_conn[i] = NULL;
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

