/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_service_register_outgoing_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_SERVICE_REGISTER_OUTGOING_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_service_register_outgoing_req(
    phandle_t phandle,
    context_t context,
    BD_ADDR_T *p_bd_addr,
    dm_protocol_id_t protocol_id,
    uint16_t remote_channel,
    dm_security_level_t outgoing_security_level,
    uint8_t min_enc_key_size,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_SERVICE_REGISTER_OUTGOING_REQ_T *p_prim = pnew(DM_SM_SERVICE_REGISTER_OUTGOING_REQ_T);

    p_prim->type = DM_SM_SERVICE_REGISTER_OUTGOING_REQ;
    p_prim->phandle = phandle;
    p_prim->context = context;
    tbdaddr_copy_from_public_bd_addr(&p_prim->connection.addrt, p_bd_addr);
    p_prim->connection.service.protocol_id = protocol_id;
    p_prim->connection.service.channel = remote_channel;
    p_prim->security_level = outgoing_security_level;
    p_prim->min_enc_key_size = min_enc_key_size;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

