/*!
Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   dmlib_acl_get_role.c

\brief  Get role of the device for a particular BD address.

*/

#include "dmlib_private.h"
#include INC_DIR(dm,dm_acl_core.h)

/**
 * This API is used by the application to determine which role the device
 * is performing for a particular bd address.
 *
 * This function fetches the device role for a specified acl connection handle.
 * For a valid connection handle the API returns a valid role HCI_SLAVE / 
 * HCI_MASTER and for an invalid acl handle or if ACL role discovery is pending it returns HCI_MASTER_SLAVE_UNKNOWN.
 *
 * \param tp_addrt Pointer to BD address
 * \return hci_role_t HCI_SLAVE/HCI_MASTER for valid acl. For an invalid acl handle or if ACL role discovery is  
                      pending, returns HCI_MASTER_SLAVE_UNKNOWN.
 */

hci_role_t dm_acl_get_role(const TP_BD_ADDR_T *const tp_addrt)
{
    DM_ACL_T *p_acl;
    hci_role_t role = HCI_MASTER_SLAVE_UNKNOWN;

    if ((p_acl = dm_acl_find_by_tbdaddr_acl_type(&tp_addrt->addrt,
                                                 tp_addrt->tp_type)) != NULL &&
         !DM_ACL_IS_CONNECTON_FLAG_SET(p_acl, DM_ACL_CONNECTION_FLAG_ROLE_DISCOVERY_PENDING))
    {
        if ((p_acl->flags & DM_ACL_ROLE) != DM_ACL_ROLE_MASTER)
            role = HCI_SLAVE;
        else
            role = HCI_MASTER;
    }

    return role;
}
