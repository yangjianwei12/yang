/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*! \brief Build and send a DM_SYNC_UNREGISTER_REQ primitive.
    \param phandle Handle/queue of registration.
    \param pv_cbarg Semi-opaque context of registration.
*/
void dm_sync_unregister_req(phandle_t phandle, context_t pv_cbarg)
{
    DM_SYNC_UNREGISTER_REQ_T *p_prim = pnew(DM_SYNC_UNREGISTER_REQ_T);

    p_prim->type        = DM_SYNC_UNREGISTER_REQ;
    p_prim->phandle     = phandle;
    p_prim->pv_cbarg    = pv_cbarg;
    DM_PutMsg(p_prim);
}
