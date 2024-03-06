/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*!
@file    gatt_vcs_server.h
@brief   Header file for the GATT VCS (Volume Control Service) Server library.

        This file provides documentation for the GATT VCS server library
        API (library name: gatt_vcs_server).
*/

#ifndef GATT_VCS_H_
#define GATT_VCS_H_

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"

/* Maximum number of GATT connections */
#define GATT_VCS_MAX_CONNECTIONS (3)

#define GATT_VCS_SERVER_INVALID_MUTE_VALUE ((uint8) 0xFF)
#define GATT_VCS_SERVER_INVALID_VOLUME_FLAG_VALUE ((uint8) 0xFF)

/*! @brief Enumeration of messages an application task can receive from the
    GATT VCS Server library
 */
typedef enum {
    GATT_VCS_SERVER_VOLUME_STATE_IND,
    GATT_VCS_SERVER_VOLUME_FLAG_IND,
    GATT_VCS_SERVER_CONFIG_CHANGE_IND,
    GATT_VCS_SERVER_MESSAGE_TOP
} GattVcsMessageId;

/*! @brief Definition of data required for the initialisation
 *         of the VCS Server Library.
 */
typedef struct
{
    uint8                 volumeSetting;
    uint8                 mute;
    uint8                 changeCounter;
    uint8                 stepSize;
} GattVcsInitData;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of VCS
 */
typedef struct
{
    uint16 volumeStateClientCfg;
    uint16 volumeFlagClientCfg;
} GattVcsServerConfig;

/*! @brief Contents of the GATT_VCS_SERVER_VOLUME_STATE_IND message that is sent by the library,
    due to a change of the local volume state characteristic by the client.
 */
typedef struct  __GattVcsServerVolumeStateInd
{
    GattVcsMessageId id;
    ServiceHandle vcsServiceHandle;
    connection_id_t cid;
    uint8 volumeSetting;
    uint8 mute;
    uint8 changeCounter;
} GattVcsServerVolumeStateInd;

/*! @brief Contents of the GATT_VCS_SERVER_VOLUME_FLAG_IND message that is sent by the library,
    due to a change of the local volume flag characteristic by the library.
 */
typedef struct  __GattVcsServerVolumeFlagInd
{
    GattVcsMessageId id;
    ServiceHandle vcsServiceHandle;
    uint8 volumeFlag;
} GattVcsServerVolumeFlagInd;

/*! @brief Contents of the GATT_VCS_SERVER_CONFIG_CHANGE_IND  message that is sent by the library,
    when CCCD is toggled by remote client.
 */
typedef struct  __GattVcsServerCccdWriteInd
{
    GattVcsMessageId id;
    ServiceHandle vcsServiceHandle;
    connection_id_t cid;
    bool configChangeComplete; /* will be TRUE if all CCCD of VCS are written once */
} GattVcsServerConfigChangeInd;

/*! @brief Instantiate the GATT VCS Server Service Library.

    The GATT Service Init function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param theAppTask The client task that will receive messages from this Service.
    @param startHandle The first handle in the ATT database for this Service instance.
    @param endHandle The last handle in the ATT database for this Service instance.
    @param initData The initialisation data to set

    @return ServiceHandle If the service handle returned is 0, this indicates a failure
                             during GATT Service initialisation.
*/
ServiceHandle GattVcsServerInit(
                     AppTask theAppTask,
                     uint16  startHandle,
                     uint16  endHandle,
                     GattVcsInitData* initData);

/*!
    \brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    \param srvcHndl Instance handle for the service.
    \param cid The Connection ID to the peer device.
    \param config Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    \return status_t status of the Add Configuration operation.
*/
status_t GattVcsServerAddConfig(
                  ServiceHandle               srvcHndl,
                  connection_id_t                cid,
                  GattVcsServerConfig *const config);

/*!
    \brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    \param srvcHndl The GATT service instance handle.
    \param cid A Connection ID for the peer device.

    \return GattVcsServerConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/
GattVcsServerConfig* GattVcsServerRemoveConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid);


/*!
    \brief Gets the configuration for a peer device, identified by its
           Connection ID.

    This gets the configuration for that peer device from the
    service library.
    It is recommnded to call this API after GATT_VCS_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    \param srvcHndl The GATT service instance handle.
    \param cid A Connection ID for the peer device.

    \return GattVcsServerConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattVcsServerConfig* GattVcsServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid);

/*!
    @brief This API is used to set the volume value.

    @param srvcHndl Service handle of the VCS server instance.
    @param volume Value of Volume Setting to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattVcsServerSetVolume(ServiceHandle srvcHndl, uint8 volume);

/*!
    @brief This API is used to set the mute value.

    @param srvcHndl Service handle of the VCS server instance.
    @param mute Value of Mute to set

    @return TRUE if successful, FALSE otherwise

*/
bool GattVcsServerSetMute(ServiceHandle srvcHndl, uint8 mute);


/*!
    @brief This API is used when the server application needs to get the volume value.

    @param srvcHndl Service handle of the VCS server instance.

    @return uint8 Volume value.
*/
uint8 GattVcsServerGetVolume(ServiceHandle srvcHndl);

/*!
    @brief This API is used when the server application needs to get the mute value.

    @param srvcHndl Service handle of the VCS server instance.

    @return uint8 Mute value or GATT_VCS_SERVER_INVALID_MUTE_VALUE if srvcHndl is not valid.
*/
uint8 GattVcsServerGetMute(ServiceHandle srvcHndl);

/*!
    @brief This API is used when the server application needs to get the Volume Flag value.

    @param srvcHndl Service handle of the VCS server instance.

    @return uint8 Volume Flag value or GATT_VCS_SERVER_INVALID_VOLUME_FLAG_VALUE if handle
                  is not valid.
*/
uint8 GattVcsServerGetVolumeFlag(ServiceHandle srvcHndl);

/*!
    @brief This API is used to set the Volume flag value.

    @param srvcHndl Service handle of the VCS server instance.
    @param volumeFlag Value of Volume flag.
                      Accepted values :
                      0x00 (Reset Volume Setting)
                      0x01 (User Set Volume Setting)

    @return TRUE if successful, FALSE otherwise

    Note : Default value for volume flag is set to 0x00 (Reset Volume Setting) during initialisation
           of VCS Server. It will be changed to 0x01 (User Set Volume Setting) after volume state has
           been modified by user action.
           This API is recommended to call after initialisation and before any connection is
           initiated with a remote device.

*/
bool GattVcsServerSetVolumeFlag(ServiceHandle srvcHndl, uint8 volumeFlag);

/*!
    @brief This API is used to set the Volume, Mute and Change counter value.

    @param srvcHndl Service handle of the VCS server instance.
    @param volume Value of Volume Setting to set
    @param mute Value of Mute to set
    @param change_counter Value of Change counter to set

    @return TRUE if successful, FALSE otherwise

    Note : Change counter value will be ignored if there is an active connection with a remote device.

*/
bool GattVcsServerSetVolumeMuteCc(ServiceHandle srvcHndl, uint8 volume, uint8 mute, uint8 change_counter);

#endif /* GATT_VCS_SERVER_H_ */
