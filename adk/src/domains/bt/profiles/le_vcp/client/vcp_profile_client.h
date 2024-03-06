/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   vcp_profile_client  LE VCP
    @{
    \ingroup    profiles
    \brief      Header file for VCP Profile client
*/

#ifndef VCP_PROFILE_CLIENT_H
#define VCP_PROFILE_CLIENT_H

#include "bt_types.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*!
    @brief Create Instance of standalone VCP Client for this CID.
*/
bool VcpProfileClient_CreateInstance(gatt_cid_t cid);

/*!
    @brief Read the Volume State characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_STATE_CFM message will be sent to the registered application Task.

*/
void VcpProfileClient_ReadStateCharacteristic(void);

/*!
    @brief Read the Volume Flag characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_FLAG_CFM message will be sent to the registered application Task.

*/
void VcpProfileClient_ReadFlagCharacteristic(void);

/*!
    @brief Write the Volume Control point characteristic in order to execute
           the Absolute Volume operation.

    @param profileHandle  The Profile handle.
    @param volumeSetting  Value of volume to set

    NOTE: A VCP_ABS_VOL_CFM message will be sent to the registered application Task.

*/
void VcpProfileClient_SetAbsoluteVolume(void);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_UNMUTE_CFM message will be sent to the registered application Task.

*/
void VcpProfileClient_UnmuteVolume(void);

/*!
    @brief This API is used to write the client characteristic configuration of the Volume State and volume flag
    characteristic on a remote device, to enable notifications with the server.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: VCP_VOLUME_STATE_SET_NTF_CFM message will be sent to the registered application Task.

*/
void VcpProfileClient_RegisterNotification(void);

/*! \brief Destroy the VCP profile instance for the given GATT connection identifier

    \param cid GATT Connection identifier for which to close BAP Profile connection

    NOTE: VCP_DESTROY_CFM message will be sent to the registered application Task.

*/
bool VcpProfileClient_DestroyInstance(gatt_cid_t cid);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /*VCP_PROFILE_CLIENT_H */

/*! @} */