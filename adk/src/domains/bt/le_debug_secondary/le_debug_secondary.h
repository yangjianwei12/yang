/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       le_debug_secondary.h
\defgroup   le_debug_secondary
\ingroup    bt_domain

\brief      Bluetooth domain functionality to enable LE connection on secondary for additional debugging
            purpose that can be used by the application.
*/
#ifndef LE_DEBUG_SECONDARY_H
#define LE_DEBUG_SECONDARY_H

#ifdef ENABLE_LE_DEBUG_SECONDARY

#include <bt_device.h>
#include <logging.h>

/*! @{ */

/*! \brief notifications sent by le_debug_secondary to other modules. */
typedef enum
{
    /*! Update IRK complete */
    LE_DEBUG_SECONDARY_UPDATE_IRK_CFM = LE_DEBUG_SECONDARY_MESSAGE_BASE,
    /*! This must be the final message */
    LE_DEBUG_SECONDARY_MESSAGE_END
} le_debug_secondary_msg_t;

/*! \brief Status codes used by le_debug_secondary */
typedef enum
{
    le_debug_update_irk_status_success = 0,
    le_debug_update_irk_status_failed
} le_debug_update_irk_status_t;

/*! \brief Confirmation of the result of IRK Update. */
typedef struct
{
    /*! Status of IRK Update. */
    le_debug_update_irk_status_t status;
} LE_DEBUG_SECONDARY_UPDATE_IRK_CFM_T;

/*! \brief Update Identity Resolving Key (IRK) based on the role.

    This function needs to be invoked in following cases
    - When role is selected (Ex: After PEER find role).
    - When role is updated/swapped (Ex: After Handover)

    \param  for_secondary_role TRUE to use different IRK for secondary purpose. This needs to be set only if role is secondary.
                               FALSE will restore to default primary IRK.
    \param client the CFM will be sent to when the request is completed.
    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void LeDebugSecondary_UpdateLocalIRK(bool for_secondary_role, Task client);

/*! \brief Test if Secondary IRK is in use.
    \return TRUE if different IRK is in use for secondary purpose, FALSE if default primary IRK is in use.
    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
bool LeDebugSecondary_IsSecondaryIRKInUse(void);


/*! \defgroup   le_debug_secondary Debug Secondary (LE)

The Debug Secondary module provides application with ability to 
modify the IRK(Identity Resolving Key) on the secondary earbud.


This modification is required, For any GATT central device to 
differentiate between primary/secondary earbud and to establish 
connection with secondary earbud.

The application using this module shall implement functionality to
- Start/Stop Advertisements after modifying IRK on secondary.
- Accept pairing and connection on secondary earbud from a 
  Gatt Central Debug Device.
- Stop Advertisements and disconnect Gatt Central Debug Device, Switch
  the local IRK back to primary IRK before starting the Handover on secondary
  earbud.
- restart advertisements on secondary earbud when Handover is vetoed.

Responsibilities:
- This module is responsible for creating a new IRK based on Encryption Root key and Identity Root key.
- Notify the status of IRK Update procedure to application.

\b Disclaimer:
- Handover shall not be attempted whenever IRK has been modified on the secondary earbud.i.e. Handover
  shall not be attempted when there is an active debug BLE link with secondary earbud or secondary earbud
  is advertising with a modified IRK. Applications using this module to modify the local IRK, shall always
  switch the IRK back to primary IRK before trying to perform handover.
- It is important to note that this function \b SHALL be used exclusively for 
  development purposes only and \b SHALL \b NOT be invoked in an end product.
*/

/*! @} */

#endif /* ENABLE_LE_DEBUG_SECONDARY */
#endif /* LE_DEBUG_SECONDARY_H */
