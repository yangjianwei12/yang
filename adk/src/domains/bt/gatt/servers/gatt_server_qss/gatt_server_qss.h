/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for GATT QSS Server
*/

#ifndef GATT_SERVER_QSS_H
#define GATT_SERVER_QSS_H

#include "domain_message.h"
#include "bt_types.h"

/*! \brief Events sent by GATT QSS Server to other modules. */
typedef enum
{
    /*! QSS notification got updated (Disabled/Enabled by remote) */
    GATT_QSS_SERVER_CONFIG_UPDATED = GATT_QSS_SERVER_MESSAGE_BASE,

    /*! This must be the final message */
    GATT_QSS_SERVER_MESSAGE_END
} gatt_qss_server_msg_t;

/*! \brief Data associated with GATT QSS Server config updated message */
typedef struct
{
    /*! GATT Connection identifier */
    gatt_cid_t cid;

    /*! Notification enabled/disabled */
    bool ntf_enabled;
} GATT_QSS_SERVER_CONFIG_UPDATED_T;

/*! \brief Initialize the GATT QSS server.
 */
void GattServerQss_Init(void);

/*! \brief Sets user description for QSS service.

    \param description Pointer to user description
    \param len Length of user description

    \returns TRUE if user description is being updated successfully, else FALSE
*/
bool GattServerQss_SetQssUserDescription(const char *description, uint8 len);

/*! \brief Notifies QSS server with lossless audio data.

    \param lossless_data.
*/
void GattServerQss_UpdateLosslessAudiodata(uint32 lossless_data);

/*! \brief Is QSS Notifications enabled by the remote? */
bool GattServerQss_IsNotificationEnabled(void);

/*! \brief Register for GATT QSS Service notifications */
void GattServerQss_ClientRegister(Task client_task);

/*! \brief Unregister for GATT QSS Service notifications */
void GattServerQss_ClientUnregister(Task client_task);

#endif /* GATT_SERVER_QSS_H */

