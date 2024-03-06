/*!
Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   dmlib_acl_get_mode.c

\brief  Get current mode of a device for a particular BD address.

*/

#include "dmlib_private.h"

#include INC_DIR(dm,dm_acl_core.h)

/**
 * This API is used by the application to determine the current mode of the device
 * for a particular bd address.
 *
 * This function fetches the device role for a specified acl connection handle.
 * For a valid connection handle the API returns a valid mode HCI_BT_MODE_ACTIVE 
 * / HCI_BT_MODE_SNIFF etc and for an invalid acl handle it returns 
 * HCI_BT_MODE_MAX.
 *
 * \param tp_addrt Pointer to BD address
 * \return hci_bt_mode_t HCI_BT_MODE_ACTIVE/HCI_BT_MODE_SNIFF for a valid acl,
                         HCI_BT_MODE_MAX otherwise.
 */

hci_bt_mode_t dm_acl_get_mode(const TP_BD_ADDR_T *const tp_addrt)
{
    DM_ACL_T *p_acl;

    if ((p_acl = dm_acl_find_by_tbdaddr_acl_type(&tp_addrt->addrt,
                                                 tp_addrt->tp_type)) != NULL)
    {
        return p_acl->current_mode;
    }

    return HCI_BT_MODE_MAX;
}
