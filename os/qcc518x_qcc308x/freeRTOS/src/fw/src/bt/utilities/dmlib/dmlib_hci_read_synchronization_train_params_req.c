/*******************************************************************************

Copyright (C) 2015 - 2020 Qualcomm Technologies International, Ltd.
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
 *      dmlib_hci_read_synchronization_train_params
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_READ_SYNCHRONIZATION_TRAIN_PARAMS primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_read_synchronization_train_params_req(
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_READ_SYNCHRONIZATION_TRAIN_PARAMS_REQ_T *p_prim =
                      pnew(DM_HCI_READ_SYNCHRONIZATION_TRAIN_PARAMS_REQ_T);

    p_prim->common.op_code = DM_HCI_READ_SYNCHRONIZATION_TRAIN_PARAMS_REQ;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

