/****************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.


FILE NAME
    gatt_vcs_client.h
    
DESCRIPTION
    Header file for the GATT VCS Client library.
*/


/*!
@file    gatt_vcs_client.h
@brief   Header file for the GATT VCS Client library.

        This file provides documentation for the GATT VCS Client library
        API (library name: gatt_vcs_client).
*/

#ifndef GATT_VCS_CLIENT_H
#define GATT_VCS_CLIENT_H

#include<message.h>
#include <gatt.h>

#include "library.h"
#include "service_handle.h"

/*!
    @brief Handles of the VCS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 volumeStateHandle;
    uint16 volumeControlPointHandle;
    uint16 volumeFlagsHandle;

    uint16 volumeStateCccHandle;
    uint16 volumeFlagsCccHandle;
} GattVcsClientDeviceData;

/*!
    \brief IDs of messages a profile task can receive from the
           GATT VCS Client library.
*/
typedef uint16 GattVcsClientMessageId;

/*! { */
/*! Values for GattVcsClientMessageId */
#define GATT_VCS_CLIENT_INIT_CFM                   (GATT_VCS_CLIENT_MESSAGE_BASE)
#define GATT_VCS_CLIENT_TERMINATE_CFM              (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM   (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0002u)
#define GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM    (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0003u)
#define GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM  (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM   (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_VCS_CLIENT_VOLUME_STATE_IND           (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0006u)
#define GATT_VCS_CLIENT_VOLUME_FLAG_IND            (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0007u)
#define GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM      (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0008u)
#define GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM       (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0009u)
#define GATT_VCS_CLIENT_REL_VOL_DOWN_CFM           (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Au)
#define GATT_VCS_CLIENT_REL_VOL_UP_CFM             (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Bu)
#define GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM    (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Cu)
#define GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM      (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Du)
#define GATT_VCS_CLIENT_ABS_VOL_CFM                (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Eu)
#define GATT_VCS_CLIENT_UNMUTE_CFM                 (GATT_VCS_CLIENT_MESSAGE_BASE + 0x000Fu)
#define GATT_VCS_CLIENT_MUTE_CFM                   (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0010u)
#define GATT_VCS_CLIENT_MESSAGE_TOP                (GATT_VCS_CLIENT_MESSAGE_BASE + 0x0011u)
/*! } */

/*!
    \brief GATT VCS Client status code type.
*/
typedef uint16 GattVcsClientStatus;

/*! { */
/*! Values of the GATT VCS Client status code. */
#define GATT_VCS_CLIENT_STATUS_SUCCESS                   (0x0000u) /*!> Request was a success*/
#define GATT_VCS_CLIENT_STATUS_INVALID_PARAMETER         (0x0001u) /*!> Invalid parameter was supplied*/
#define GATT_VCS_CLIENT_STATUS_DISCOVERY_ERR             (0x0002u) /*!> Error in discovery of Characteristics*/
#define GATT_VCS_CLIENT_STATUS_FAILED                    (0x0003u) /*!> Request has failed*/
#define GATT_VCS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004u) /*!> Insufficient Resources to complete
                                                                        the request. */
/*! } */

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
     connection_id_t cid;          /*! Connection ID. */
     uint16          startHandle;  /*! The first handle of the service that needs to be accessed */
     uint16          endHandle;    /*! The last handle of the service that needs to be accessed */
} GattVcsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattVcsClientInitReq API.
*/
typedef struct
{
    ServiceHandle     srvcHndl;   /*! Reference handle for the instance */
    GattVcsClientStatus  status;      /*! Status of the initialisation attempt */
} GattVcsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattVcsClientTerminateReq API.
*/
typedef struct
{
    GattVcsClientStatus      status;      /*! Status of the initialisation attempt */
    GattVcsClientDeviceData  deviceData;  /*! Device data: handles used for the peer device. */
} GattVcsClientTerminateCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Volume State characteristic.
 */
typedef struct
{
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    gatt_status_t     status;      /*! Status of the setting attempt */
} GattVcsClientVolumeStateSetNtfCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Volume flag characteristic.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientVolumeFlagSetNtfCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_REL_VOL_DOWN_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the relative
    volume down opearation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientRelVolDownCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_REL_VOL_UP_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the relative
    volume up operation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientRelVolUpCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute/relative
    volume down operation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientUnmuteRelVolDownCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute/relative
    volume up operation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientUnmuteRelVolUpCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_ABS_VOL_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the absolute
    volume opearation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientAbsVolCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_UNMUTE_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute
    operation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientUnmuteCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_MUTE_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the mute
    operation.
 */
typedef GattVcsClientVolumeStateSetNtfCfm GattVcsClientMuteCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_VOLUME_STATE_IND message that is sent by the library,
    as a result of a notification of the remote volume state.
 */
typedef struct
{
    ServiceHandle  srvcHndl;
    uint8             volumeState;
    uint8             mute;
    uint8             changeCounter;
} GattVcsClientVolumeStateInd;

/*! @brief Contents of the GATT_VCS_CLIENT_VOLUME_FLAG_IND message that is sent by the library,
    as a result of a notification of the remote battery level.
 */
typedef struct
{
    ServiceHandle  srvcHndl;
    uint8             volumeFlag;
} GattVcsClientVolumeFlagInd;

/*! @brief Contents of the GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Volume State Client Configuration characteristic on the server.
 */
typedef struct
{
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    gatt_status_t     status;      /*! Status of the reading attempt */
    uint16            sizeValue;  /*! Value size*/
    uint8             value[1];    /*! Read value */
} GattVcsClientReadVolumeStateCccCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM message that is sent by the library,
    as a result of reading of the Volume Flag Client Configuration characteristic on the server.
 */
typedef GattVcsClientReadVolumeStateCccCfm GattVcsClientReadVolumeFlagCccCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM message that is sent by the library,
    as a result of a reading of the Volume State characteristic on the server.
 */
typedef struct
{
    ServiceHandle  srvcHndl;       /*! Reference handle for the instance */
    gatt_status_t     status;         /*! Status of the reading attempt */
    uint8             volumeSetting;  /*! Volume setting value */
    uint8             mute;           /*! Mute value */
    uint8             changeCounter;  /*! Change counter value */
} GattVcsClientReadVolumeStateCfm;

/*! @brief Contents of the GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM message that is sent by the library,
    as a result of a read of the Volume Flag characteristic of the server.
 */
typedef struct
{
    ServiceHandle  srvcHndl;    /*! Reference handle for the instance */
    gatt_status_t     status;      /*! Status of the reading attempt */
    uint8             volumeFlag;  /*! Value of the Volume Flag characteristic */
}GattVcsClientReadVolumeFlagCfm;

/*!
    @brief GATT VCS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    \param init_data    Configuration data for client initialisation.
    \param device_data  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_VCS_CLIENT_INIT_CFM will be received with a GattVcsClientStatus status code.
*/
void GattVcsClientInitReq(Task                           theAppTask,
                          const GattVcsClientInitData   *initData,
                          const GattVcsClientDeviceData *deviceData);


/*!
    \brief GATT VCS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    \param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_VCS_CLIENT_TERMINATE_CFM will be received with a GattVcsClientStatus status code.
*/
void GattVcsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of the Volume State
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_VCS_CLIENT_VOLUME_STATE_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattVcsClientVolStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                     bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Volume Flags
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_VCS_CLIENT_VOLUME_FLAG_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattVcsClientVolFlagRegisterForNotificationReq(ServiceHandle clntHndl,
                                                    bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Volume State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VCS_CLIENT_READ_VOLUME_STATE_CCC_CFM message will be sent to the registered application Task.

*/
void GattVcsClientReadVolumeStateCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Volume Flag characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VCS_CLIENT_READ_VOLUME_FLAG_CCC_CFM message will be sent to the registered application Task.

*/
void GattVcsClientReadVolumeFlagCCCRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Volume State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VCS_CLIENT_READ_VOLUME_STATE_CFM message will be sent to the registered application Task.

*/
void GattVcsClientReadVolumeStateRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Volume Flag characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VCS_CLIENT_READ_VOLUME_FLAG_CFM message will be sent to the registered application Task.

*/
void GattVcsClientReadVolumeFlagRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Relative Volume Down operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic.

    NOTE: A GATT_VCS_CLIENT_REL_VOL_DOWN_CFM message will be sent to the registered application Task.

*/
void GattVcsClientRelativeVolDownRequest(ServiceHandle clntHndl,
                                         uint8 changeCounter);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Relative Volume Up operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic.

    NOTE: A GATT_VCS_CLIENT_REL_VOL_UP_CFM message will be sent to the registered application Task.

*/
void GattVcsClientRelativeVolUpRequest(ServiceHandle clntHndl,
                                       uint8 changeCounter);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute/Relative Volume Down operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic.

    NOTE: A GATT_VCS_CLIENT_UNMUTE_REL_VOL_DOWN_CFM message will be sent
          to the registered application Task.

*/
void GattVcsClientUnmuteRelativeVolDownRequest(ServiceHandle clntHndl,
                                               uint8 changeCounter);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute/Relative Volume Up operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic.

    NOTE: A GATT_VCS_CLIENT_UNMUTE_REL_VOL_UP_CFM message will be sent
          to the registered application Task.

*/
void GattVcsClientUnmuteRelativeVolUpRequest(ServiceHandle clntHndl,
                                             uint8 changeCounter);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Absolute Volume operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic.
    @param volumeSetting value of volume to set

    NOTE: A GATT_VCS_CLIENT_ABS_VOL_CFM message will be sent
          to the registered application Task.

*/
void GattVcsClientAbsoluteVolRequest(ServiceHandle clntHndl,
                                     uint8 changeCounter,
                                     uint8 volumeSetting);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Volume State characteristic

    NOTE: A GATT_VCS_CLIENT_UNMUTE_CFM message will be sent to the registered application Task.

*/
void GattVcsClientUnmuteRequest(ServiceHandle clntHndl, uint8 changeCounter);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Mute operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter of the Volume State characteristic.

    NOTE: A GATT_VCS_CLIENT_MUTE_CFM message will be sent to the registered application Task.

*/
void GattVcsClientMuteRequest(ServiceHandle clntHndl, uint8 changeCounter);

#endif /* GATT_VCS_CLIENT_H */

