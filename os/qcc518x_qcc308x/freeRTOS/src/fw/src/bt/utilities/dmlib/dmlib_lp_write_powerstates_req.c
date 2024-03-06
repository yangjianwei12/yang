/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_lp_write_powerstates_req
 *
 *  DESCRIPTION
 *      Build and send a DM_LP_WRITE_POWERSTATES_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 * 
 *----------------------------------------------------------------------------*/
void dm_lp_write_powerstates_req(
    BD_ADDR_T       *p_bd_addr,
    uint16_t        num_states,
    LP_POWERSTATE_T *states,
    DM_UPRIM_T      **pp_prim
    )
{
    DM_LP_WRITE_POWERSTATES_REQ_T *prim =
        pnew(DM_LP_WRITE_POWERSTATES_REQ_T);

    prim->type = DM_LP_WRITE_POWERSTATES_REQ;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);

    prim->num_states = num_states;
    prim->states = states;


    if (pp_prim == NULL)
        DM_PutMsg(prim);
    else
        *pp_prim = (DM_UPRIM_T*)prim;
}
