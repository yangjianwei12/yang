/*!
Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   dmlib_get_hci_handle.c

\brief  Get HCI handle for a given transport BD address.

*/

#include "dmlib_private.h"
#include INC_DIR(dm,dm_acl_core.h)

/**
 * Retrieves HCI handle of an ACL connection for the given transport bd address
 * 
 * This function works for both LE and BREDR ACLs with valid adddress. 
 *
 * \param tp_addrt Remote device's Transport Bluetooth Device Address 
 * \return Valid HCI handle if connection is found, otherwise HCI_HANDLE_INVALID
 */
hci_connection_handle_t dm_get_hci_handle(const TP_BD_ADDR_T *const tp_addrt)
{
    DM_ACL_T *p_acl;
    hci_connection_handle_t handle = HCI_HANDLE_INVALID;
    
    if ((p_acl = dm_acl_find_by_tbdaddr_acl_type(&tp_addrt->addrt,
                                                 tp_addrt->tp_type)) != NULL)
    {
        handle = p_acl->handle;
    }

    return handle;
}
