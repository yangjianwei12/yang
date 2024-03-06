/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_read_local_oob_data_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_READ_LOCAL_OOB_DATA_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_read_local_oob_data_req(
    PHYSICAL_TRANSPORT_T tp_type,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_READ_LOCAL_OOB_DATA_REQ_T *p_prim = pnew(DM_SM_READ_LOCAL_OOB_DATA_REQ_T);
    p_prim->type            = DM_SM_READ_LOCAL_OOB_DATA_REQ;
    p_prim->tp_type = tp_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

