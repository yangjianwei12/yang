/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*============================================================================*
    Private Data Types
 *============================================================================*/
/* None */

/*============================================================================*
    Private Data
 *============================================================================*/
/* None */

/*============================================================================*
    Private Function Prototypes
 *============================================================================*/
/* None */

/*============================================================================*
    Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_access_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_ACCESS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_sm_access_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *p_addrt,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t incoming,
    uint32_t  context,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_ACCESS_REQ_T *p_prim = zpnew(DM_SM_ACCESS_REQ_T);

    p_prim->type = DM_SM_ACCESS_REQ;
    p_prim->phandle = phandle;
    tbdaddr_copy(&p_prim->conn_setup.connection.addrt, p_addrt);
    p_prim->conn_setup.connection.service.protocol_id = protocol_id;
    p_prim->conn_setup.connection.service.channel = channel;
    p_prim->conn_setup.incoming = incoming;
    p_prim->context = context;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
