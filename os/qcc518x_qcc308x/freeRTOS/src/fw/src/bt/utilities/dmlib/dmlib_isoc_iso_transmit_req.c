/*******************************************************************************

Copyright (C) 2018-2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_iso_transmit_req
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

void dm_isoc_iso_transmit_req(
    hci_connection_handle_t handle,
    uint8_t payload_type,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_ISO_TRANSMIT_TEST_REQ_T *p_prim;


    p_prim = zpnew(DM_ISOC_ISO_TRANSMIT_TEST_REQ_T);
    p_prim->type = DM_ISOC_ISO_TRANSMIT_TEST_REQ;
    p_prim->handle = handle;
    p_prim->payload_type = payload_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

