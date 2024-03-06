/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    gatt_vcs_server.h
@brief   Header file for the GATT VCS (Volume Control Service) Server library.

        This file provides documentation for the GATT VCS server library
        API (library name: gatt_vcs_server).
*/

#ifndef GATT_VCS_H_
#define GATT_VCS_H_

#include <library.h>
#include <gatt.h>
#include <service_handle.h>

/* Maximum number of GATT connections */
#define GATT_VCS_MAX_CONNECTIONS (6)

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
    unsigned volumeStateClientCfg:2;
    unsigned volumeFlagClientCfg:2;
} GattVcsServerConfig;

/*! @brief Contents of the GATT_VCS_SERVER_VOLUME_STATE_IND message that is sent by the library,
    due to a change of the local volume state characteristic by the client.
 */
typedef struct  __GattVcsServerVolumeStateInd
{
    ServiceHandle vcsServiceHandle;
    uint8 volumeSetting;
    uint8 mute;
    uint8 changeCounter;
    connection_id_t cid;
} GattVcsServerVolumeStateInd;

/*! @brief Contents of the GATT_VCS_SERVER_VOLUME_FLAG_IND message that is sent by the library,
    due to a change of the local volume flag characteristic by the library.
 */
typedef struct  __GattVcsServerVolumeFlagInd
{
    ServiceHandle vcsServiceHandle;
    uint8 volumeFlag;
} GattVcsServerVolumeFlagInd;

/*!
    \brief IDs of messages an application task can receive from the
    GATT VCS Server library.
*/
typedef uint16 gattVcsServerMessageId;

/*! { */
/*! Values for gattVcsServerMessageId */
#define GATT_VCS_SERVER_VOLUME_STATE_IND  (GATT_VCS_SERVER_MESSAGE_BASE)
#define GATT_VCS_SERVER_VOLUME_FLAG_IND   (GATT_VCS_SERVER_MESSAGE_BASE + 0x0001u)
/*! } */

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
ServiceHandle GattVcsServerInit(Task theAppTask,
                                   uint16 startHandle,
                                   uint16 endHandle,
                                   GattVcsInitData initData);

/*!
    \brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    \param srvcHndl Instance handle for the service.
    \param cid The Connection ID to the peer device.
    \param config Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    \return gatt_status_t status of the Add Configuration operation.
*/
gatt_status_t GattVcsServerAddConfig(ServiceHandle srvcHndl,
                                     connection_id_t cid,
                                     const GattVcsServerConfig* config);

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
GattVcsServerConfig * GattVcsServerRemoveConfig(ServiceHandle srvcHndl,
                                                connection_id_t cid);

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

    @param handle Service handle of the VCS server instance.

    @return uint8 Volume value.
*/
uint8 GattVcsServerGetVolume(ServiceHandle srvcHndl);

/*!
    @brief This API is used when the server application needs to get the mute value.

    @param handle Service handle of the VCS server instance.

    @return uint8 Mute value.
*/
uint8 GattVcsServerGetMute(ServiceHandle srvcHndl);

/*!
    @brief This API is used when the server application needs to get the Volume Flag value.

    @param handle Service handle of the VCS server instance.

    @return uint8 Volume Flag value.

*/
uint8 GattVcsServerGetVolumeFlag(ServiceHandle srvcHndl);

#endif /* GATT_VCS_SERVER_H_ */
