/*******************************************************************************

Copyright (C) 2019-2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_remove_iso_data_path_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_ISO_TRANSMIT_TEST_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_remove_iso_data_path_req(
    hci_connection_handle_t handle,
    uint8_t data_path_direction,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_REMOVE_ISO_DATA_PATH_REQ_T *p_prim;


    p_prim = zpnew(DM_ISOC_REMOVE_ISO_DATA_PATH_REQ_T);
    p_prim->type = DM_ISOC_REMOVE_ISO_DATA_PATH_REQ;
    p_prim->handle = handle;
    p_prim->data_path_direction = data_path_direction;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

