/*******************************************************************************

Copyright (C) 2017 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_auto_configure_local_address_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_auto_configure_local_address_req(
        DM_SM_RANDOM_ADDRESS_T random_addr_type,
        TP_BD_ADDR_T *static_addrt,
        uint16_t rpa_timeout,
        DM_UPRIM_T **pp_prim)
{
    DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_REQ_T *prim =
                                    zpnew(DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_REQ_T);

    prim->type = DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_REQ;
    prim->set_random_addr_type = random_addr_type;
    if(static_addrt)
    {
        tbdaddr_copy(&prim->static_addrt.addrt, &static_addrt->addrt);
        prim->static_addrt.tp_type = static_addrt->tp_type;
    }
    prim->rpa_timeout = rpa_timeout;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

