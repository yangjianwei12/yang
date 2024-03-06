/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_authorise_res
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_AUTHORISE_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_authorise_rsp(
    TYPED_BD_ADDR_T *p_addrt,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t incoming,
    uint16_t authorisation,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_AUTHORISE_RSP_T *p_prim = zpnew(DM_SM_AUTHORISE_RSP_T);

    p_prim->type = DM_SM_AUTHORISE_RSP;
    tbdaddr_copy(&p_prim->cs.connection.addrt, p_addrt);
    p_prim->cs.connection.service.protocol_id = protocol_id;
    p_prim->cs.connection.service.channel = channel;
    p_prim->cs.incoming = incoming;
    p_prim->authorisation = authorisation;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

