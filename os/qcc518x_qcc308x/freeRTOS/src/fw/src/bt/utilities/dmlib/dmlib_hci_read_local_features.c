/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_read_local_features
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_LOCAL_SUPP_FEATURES_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_read_local_features(
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_READ_LOCAL_SUPP_FEATURES_REQ_T *p_prim = zpnew(DM_HCI_READ_LOCAL_SUPP_FEATURES_REQ_T);

    p_prim->common.op_code = DM_HCI_READ_LOCAL_SUPP_FEATURES_REQ;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

