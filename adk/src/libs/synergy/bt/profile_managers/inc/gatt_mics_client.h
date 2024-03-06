/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version

FILE NAME
    gatt_mics_client.h
    
DESCRIPTION
    Header file for the GATT MICS Client library.
*/


/*!
@file    gatt_mics_client.h
@brief   Header file for the GATT MICS Client library.

        This file provides documentation for the GATT MICS Client library
        API (library name: gatt_mics_client).
*/

#ifndef GATT_MICS_CLIENT_H
#define GATT_MICS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"

#define GATT_MICS_CLIENT_MESSAGE_BASE 0x00

/*!
   MICS Mute Characteristic values
*/

#define MICS_CLIENT_NOT_MUTED       (0x00)
#define MICS_CLIENT_MUTED           (0x01)
#define MICS_CLIENT_MUTE_DISABLED   (0x02)

/*!
    @brief Handles of the MICS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 muteHandle;
    uint16 muteCccHandle;
} GattMicsClientDeviceData;

/*!
    \brief IDs of messages a profile task can receive from the
           GATT MICS Client library.
*/
typedef uint16 GattMicsClientMessageId;

/*! { */
/*! Values for GattMicsClientMessageId */
#define GATT_MICS_CLIENT_INIT_CFM                   (GATT_MICS_CLIENT_MESSAGE_BASE)
#define GATT_MICS_CLIENT_TERMINATE_CFM              (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_MICS_CLIENT_NTF_CFM                    (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0002u)
#define GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM    (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0003u)
#define GATT_MICS_CLIENT_MUTE_VALUE_IND             (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM        (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM         (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0006u)
#define GATT_MICS_CLIENT_MESSAGE_TOP                (GATT_MICS_CLIENT_MESSAGE_BASE + 0x0007u)
/*! } */

/*!
    \brief GATT MICS Client status code type.
*/
typedef uint16 GattMicsClientStatus;

/*! { */
/*! Values of the GATT MICS Client status code. */
#define GATT_MICS_CLIENT_STATUS_SUCCESS                   (0x0000u) /*!> Request was a success*/
#define GATT_MICS_CLIENT_STATUS_INVALID_PARAMETER         (0x0001u) /*!> Invalid parameter was supplied*/
#define GATT_MICS_CLIENT_STATUS_DISCOVERY_ERR             (0x0002u) /*!> Error in discovery of Characteristics*/
#define GATT_MICS_CLIENT_STATUS_FAILED                    (0x0003u) /*!> Request has failed*/
#define GATT_MICS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004u) /*!> Insufficient Resources to complete
                                                                        the request. */
/*! } */

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
     connection_id_t cid;       /*! Connection ID. */
     uint16 startHandle;       /*! The first handle of the service that needs to be accessed */
     uint16 endHandle;         /*! The last handle of the service that needs to be accessed */
} GattMicsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattMicsClientInitReq API.
*/
typedef struct
{
    GattMicsClientMessageId id;
    connection_id_t  cid;         /*! Connection ID. */
    ServiceHandle srvcHndl;    /*! Reference handle for the instance */
    GattMicsClientStatus  status;  /*! Status of the initialisation attempt */
} GattMicsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattMicsClientTerminateReq API.
*/
typedef struct
{
    GattMicsClientMessageId id;
    ServiceHandle srvcHndl;          /*! Reference handle for the instance */
    GattMicsClientStatus status;         /*! Status of the initialisation attempt */
    GattMicsClientDeviceData deviceData; /*! Device data: handles used for the peer device. */
} GattMicsClientTerminateCfm;

/*! @brief Contents of the GATT_MICS_CLIENT_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Mute characteristic.
 */
typedef struct
{
    GattMicsClientMessageId id;
    ServiceHandle         srvcHndl;   /*! Reference handle for the instance */
    status_t                 status;     /*! Status of the setting attempt */
} GattMicsClientNtfCfm;

/*! @brief Contents of the GATT_MICS_CLIENT_MUTE_VALUE_IND message that is sent by the library,
    as a result of a notification of the remote mute.
 */
typedef struct
{
    GattMicsClientMessageId id;
    ServiceHandle  srvcHndl;
    uint8             muteValue;
} GattMicsClientMuteValueInd;

/*! @brief Contents of the GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM message that is sent by the library,
    as a result of reading of the Mute Client Configuration characteristic on the server.
 */
typedef struct
{
    GattMicsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    status_t          status;     /*! Status of the reading attempt */
    uint16            sizeValue;  /*! Value size*/
    uint8             value[1];    /*! Read value */
} GattMicsClientReadMuteValueCccCfm;

/*! @brief Contents of the GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM message that is sent by the library,
    as a result of a reading of the Mute characteristic on the server.
 */
typedef struct
{
    GattMicsClientMessageId id;
    ServiceHandle svcHndl;   /*! Reference handle for the instance */
    status_t status;            /*! Status of the reading attempt */
    uint8 muteValue;                 /*! Mute value */
} GattMicsClientReadMuteValueCfm;

/*! @brief Contents of the GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM message that is sent by the library,
    as a result of writing the Mute characteristic on the server using the Set Mute operation.
 */
typedef GattMicsClientNtfCfm GattMicsClientSetMuteValueCfm;

/*!
    @brief GATT MICS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    @param initData     Configuration data for client initialisation.
    @param deviceData   Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_MICS_CLIENT_INIT_CFM will be received with a GattMicsClientStatus status code.
*/
void GattMicsClientInitReq(AppTask theAppTask,
                          const GattMicsClientInitData   *initData,
                          const GattMicsClientDeviceData *deviceData);


/*!
    @brief GATT MICS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_MICS_CLIENT_TERMINATE_CFM will be received with a GattMicsClientStatus status code.
*/
void GattMicsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of the Mute
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_MICS_CLIENT_NTF_CFM message will be sent to the registered application Task.

*/
void GattMicsClientRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Mute characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM message will be sent to the registered application Task.

*/
void GattMicsClientReadMuteValueCccReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Mute characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM message will be sent to the registered application Task.

*/
void GattMicsClientReadMuteValueReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the Mute characteristic in order to execute
           the Set Mute operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Mute characteristic.

    NOTE: A GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM message will be sent to the registered application Task.

*/
void GattMicsClientSetMuteValueReq(ServiceHandle clntHndl,
                                         uint8 muteValue);

/*!
    @brief This API is used to retrieve the Mute characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattMicsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.

*/
GattMicsClientDeviceData *GattMicsClientGetHandles(ServiceHandle clntHndl);

#endif /* GATT_MICS_CLIENT_H */
