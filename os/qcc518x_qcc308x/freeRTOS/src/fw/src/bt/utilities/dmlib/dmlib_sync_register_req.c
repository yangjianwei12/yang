/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*! \brief Build and send a DM_SYNC_REGISTER_REQ primitive.
    \param phandle Handle/queue for registering entity.
    \param pv_cbarg Semi-opaque context for registering entity.
*/
void dm_sync_register_req(phandle_t phandle, context_t pv_cbarg)
{
    DM_SYNC_REGISTER_REQ_T *p_prim = zpnew(DM_SYNC_REGISTER_REQ_T);

    p_prim->type        = DM_SYNC_REGISTER_REQ;
    p_prim->phandle     = phandle;
    p_prim->pv_cbarg    = pv_cbarg;
    DM_PutMsg(p_prim);
}
