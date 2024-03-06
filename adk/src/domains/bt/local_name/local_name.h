/*!
    \copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       local_name.h
    \defgroup   local_name  Local Name
    @{
    \ingroup    bt_domain
    \brief      Header file for Bluetooth Local Name component

*/

#ifndef _DOMAINS_BT_LOCAL_NAME_
#define _DOMAINS_BT_LOCAL_NAME_

#include "domain_message.h"

/*! \brief Messages sent by the Local Name module.  */
typedef enum
{
    /*! Message confirming that the Local Name module initialisation is complete */
    LOCAL_NAME_INIT_CFM = LOCAL_NAME_MESSAGE_BASE,

    /*! Notifies that the name has changed */
    LOCAL_NAME_NAME_CHANGED_IND,

    /*! This must be the final message */
    LOCAL_NAME_MESSAGE_END
} local_name_message_t;

/*!
    \brief Initialise Local Name module

    \param init_task
           Task to receive completion message

    \return TRUE to indicate successful initialisation,
            FALSE otherwise.
*/
bool LocalName_Init(Task init_task);

/*!
    \brief Register for notifications

    \param client
           The client to receive the notifications
*/
void LocalName_RegisterNotifications(Task client);

/*!
    \brief Return the friendly name of the local Bluetooth device
*/
const uint8 *LocalName_GetName(uint16* name_len);

/*!
    \brief Return the friendly name of the local Bluetooth device
           prefixed for use with Bluetooth Low Energy services
*/
const uint8 *LocalName_GetPrefixedName(uint16* name_len);

/*!
    \brief Enables or disables the inclusion of the suffix in the name

    \param bt_enable
           Whether or not the suffix is enabled for Bluetooth Classic
    \param ble_enable
           Whether or not the suffix is enabled for BLE
*/
void LocalName_SuffixEnable(bool bt_enable, bool ble_enable);

/*!
    \brief Sets the local name suffix

    Will panic if it's already set.

    \param suffix
            The suffix to set to
*/
void LocalName_SetSuffix(const char *suffix);

#endif /* _DOMAINS_BT_LOCAL_NAME_ */
/**! @} */