/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_configure_data_path_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_CONFIGURE_DATA_PATH_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Each index of vendor_specific_config[] shall hold maximum of 
 *      HCI_CONFIGURE_DATA_PATH_PER_PTR number of octets.
 *
 *      Note:
 *      Ownership of pointers in vendor_specific_config[] present is transferred to 
 *      the stack and will be freed by the stack.
 *      Caller should not be free vendor_specific_config[].
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_hci_configure_data_path_req(
     uint8_t data_path_direction,
     uint8_t data_path_id,
     uint8_t vendor_specific_config_len,
     uint8_t *vendor_specific_config[],
     DM_UPRIM_T **pp_prim
     )
{
    DM_HCI_CONFIGURE_DATA_PATH_REQ_T *p_prim;
    uint8_t index;

    p_prim = zpnew(DM_HCI_CONFIGURE_DATA_PATH_REQ_T);

    p_prim->common.op_code = DM_HCI_CONFIGURE_DATA_PATH_REQ;
    p_prim->data_path_direction = data_path_direction;
    p_prim->data_path_id = data_path_id;
    p_prim->vendor_specific_config_len = vendor_specific_config_len;

    for (index = 0; index < HCI_CONFIGURE_DATA_PATH_PTRS; index++)
    {
        if (vendor_specific_config[index] != NULL)
        {
            p_prim->vendor_specific_config[index] = vendor_specific_config[index];
            vendor_specific_config[index] = NULL;
        }
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

