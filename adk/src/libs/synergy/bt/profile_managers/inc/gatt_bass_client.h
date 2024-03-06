/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_bass_client.h

DESCRIPTION
    Header file for the GATT Broadcast Scan Service (BASS) Client library.
*/


/*!
@file    gatt_bass_client.h
@brief   Header file for the GATT BASS Client library.

        This file provides documentation for the GATT BASS Client library
        API (library name: gatt_bass_client).
*/

#ifndef GATT_BASS_CLIENT_H
#define GATT_BASS_CLIENT_H


#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"
#include "csr_bt_gatt_prim.h"
#include "bluetooth.h"


#define GATT_BASS_CLIENT_LIB_MESSAGE_BASE 0x00

/*!
    @brief Handles of the BASS characteristics.

*/
typedef struct
{
    uint16 broadcastSourceNum;

    uint16 broadcastAudioScanControlPointHandle;
    uint16 *broadcastReceiveStateHandle;
    uint16 *broadcastReceiveStateHandleCcc;
} GattBassClientDeviceData;

/*!
    \brief IDs of messages a profile task can receive from the
           GATT BASS Client library.
*/
typedef uint16 GattBassClientMessageId;

/*! { */
/*! Values for GattBassClientMessageId */
#define GATT_BASS_CLIENT_INIT_CFM                             (GATT_BASS_CLIENT_LIB_MESSAGE_BASE)
#define GATT_BASS_CLIENT_TERMINATE_CFM                        (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0001u)
#define GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM  (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0002u)
#define GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0003u)
#define GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND          (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0004u)
#define GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM     (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0005u)
#define GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM                 (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0006u)
#define GATT_BASS_CLIENT_REMOTE_SCAN_START_CFM                (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0007u)
#define GATT_BASS_CLIENT_ADD_SOURCE_CFM                       (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0008u)
#define GATT_BASS_CLIENT_MODIFY_SOURCE_CFM                    (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x0009u)
#define GATT_BASS_CLIENT_SET_BROADCAST_CODE_CFM               (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x000Au)
#define GATT_BASS_CLIENT_REMOVE_SOURCE_CFM                    (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x000Bu)
#define GATT_BASS_CLIENT_MESSAGE_TOP                          (GATT_BASS_CLIENT_LIB_MESSAGE_BASE + 0x000Cu)
/*! } */

/*!
    \brief GATT BASS Client status code type.
*/
typedef uint16 GattBassClientStatus;

/*! { */
/*! Values of the GATT BASS Client status code. */
#define GATT_BASS_CLIENT_STATUS_SUCCESS                   (0x0000u) /*!> Request was a success*/
#define GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER         (0x0001u) /*!> Invalid parameter was supplied*/
#define GATT_BASS_CLIENT_STATUS_DISCOVERY_ERR             (0x0002u) /*!> Error in discovery of Characteristics*/
#define GATT_BASS_CLIENT_STATUS_FAILED                    (0x0003u) /*!> Request has failed*/
#define GATT_BASS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004u) /*!> Insufficient Resources to complete
                                                                         the request. */
#define GATT_BASS_CLIENT_STATUS_READ_ERR                  (0x0005u) /*!> Error in reading of Characteristics*/
/*! } */

/*!
    \brief BASS PA_Sync value type .
*/
typedef uint8 GattBassClientControlPointPaSyncType;

/*! { */
/*! Values of GattBassClientControlPointPaSyncType for the PA_Sync field of
 *  the Add and the Modify source operations  */
#define GATT_BASS_CLIENT_PA_SYNC_NOT_SYNCHRONIZE         (0x00u)  /*!> Not synchronize to PA*/
#define GATT_BASS_CLIENT_PA_SYNC_SYNCHRONIZE_PAST        (0x01u)  /*!> Synchronize to PA - PAST available */
#define GATT_BASS_CLIENT_PA_SYNC_SYNCHRONIZE_NO_PAST     (0x02u)  /*!> Synchronize to PA - PAST no available*/
#define GATT_BASS_CLIENT_PA_SYNC_LAST                    (0x03u)
/*! } */

/*!
    \brief BASS PA_Sync_State value type.
*/
typedef uint8 GattBassClientPaSyncStateType;

/*! { */
/*! Values of the GattBassClientPaSyncStateType for the PA_Sync_State field of
 *  the Broadcast Receive State characteristic */
#define GATT_BASS_CLIENT_PA_SYNC_STATE_NOT_SYNCHRONIZED         (0x00u)  /*!> Not synchronized to PA*/
#define GATT_BASS_CLIENT_PA_SYNC_STATE_SYNC_INFO_REQUEST        (0x01u)  /*!> SyncInfo Request*/
#define GATT_BASS_CLIENT_PA_SYNC_STATE_SYNCHRONIZED             (0x02u)  /*!> Synchronized to PA*/
#define GATT_BASS_CLIENT_PA_SYNC_STATE_FAILED_TO_SYNCHRONIZE    (0x03u)  /*!> Failed to synchronize to PA*/
#define GATT_BASS_CLIENT_PA_SYNC_STATE_NO_PAST                  (0x04u)  /*!> No PAST*/
#define GATT_BASS_CLIENT_PA_SYNC_STATE_LAST                     (0x05u)
/*! } */

/*!
\brief BASS Broadacast Big Encryption value type.
*/
typedef uint8 GattBassClientBroadcastBigEncryptionType;

/*! { */
/*! Values of the BIG_encryption field of the Broadcast Receive State characteristic */
#define GATT_BASS_CLIENT_NOT_ENCRYPTED            (0x00u) /*!> Not encrypted*/
#define GATT_BASS_CLIENT_BROADCAST_CODE_REQUIRED  (0x01u) /*!> Broadcast_Code required*/
#define GATT_BASS_CLIENT_DECRYPTING               (0x02u) /*!> Decrypting */
#define GATT_BASS_CLIENT_BAD_CODE                 (0x03u) /*!> Incorrect encryption key */
#define GATT_BASS_CLIENT_BIG_ENCRYPTION_LAST      (0x04u)
/*! } */

/*!
    @brief Parameters used by the Initialization API
*/
typedef struct
{
     connection_id_t cid;      /*! Connection ID. */
     uint16 startHandle;       /*! The first handle of the service that needs to be accessed */
     uint16 endHandle;         /*! The last handle of the service that needs to be accessed */
} GattBassClientInitData;

/*!
    @brief Subgroups data
*/
typedef struct
{
     uint32 bisSync;     /*! BIS Synchronization state. */
     uint8 metadataLen;  /*! Metadata size. */
     uint8 *metadata;    /*! Metadata value. It exists only if
                             metadataLen is not zero.*/
} GattBassClientSubGroupsData;

/*!
    @brief Parameters used by the Add Source operation
*/
typedef struct
{
    TYPED_BD_ADDR_T sourceAddress;                        /*! BT address of the Broadcast Source State */
    uint8 advSid;                                         /*! Advertising SID */
    uint32 broadcastId;                                   /*! Broadcast _ID of the Broadcast Source */
    GattBassClientControlPointPaSyncType paSyncState;     /*! PA Synchronization state */
    uint16 paInterval;                                    /*! SyncInfo field Interval parameter value */
    uint8 numSubGroups;                                   /*! Number of subgroups */
    GattBassClientSubGroupsData *subGroupsData;           /*! Subgroups data. It exists only
                                                              if numSubGroups is not zero. */
} GattBassClientAddSourceParam;

/*!
    @brief Parameters used by the Modify Source operation
*/
typedef struct
{
    uint8 sourceId;                                    /*! Source_id of the Broadcast Source */
    GattBassClientControlPointPaSyncType paSyncState;  /*! PA Synchronization state */
    uint16 paInterval;                                 /*! SyncInfo field Interval parameter value */
    uint8 numSubGroups;                                /*! Number of subgroups */
    GattBassClientSubGroupsData *subGroupsData;        /*! Subgroups data. It exists only
                                                           if numSubGroups is not zero. */
} GattBassClientModifySourceParam;

/*!
    @brief Broadcast Receice State characteristic fields
*/
typedef struct
{
    uint8 sourceId;                                         /*! Source_id of the Broadcast Receive State
                                                                characteristic */
    TYPED_BD_ADDR_T sourceAddress;                          /*! BT address of the Broadcast Source State */
    uint8 advSid;                                           /*! Advertising SID */
    uint32 broadcastId;                                     /*! Broadcast_ID of the Broadcast Source  */
    GattBassClientPaSyncStateType paSyncState;              /*! PA Synchronization state */
    GattBassClientBroadcastBigEncryptionType bigEncryption; /*! BIG encryption state */
    uint8 *badcode;                                         /*! Incorrect 16-octet Broadcast_Code that fails
                                                                to decrypt the BIG. NULL if BIG_Encryption
                                                                field value = 0x00, 0x01, or 0x02. */
    uint8 numSubGroups;                                     /*! Number of subgroups . */
    GattBassClientSubGroupsData *subGroupsData;             /*! Subgroups data. It exists only
                                                                if numSubGroups is not zero. */
} GattBassClientBroadcastReceiveState;

/*!
    @brief Gatt client library message sent as a result of calling the GattBassClientInitReq API.
*/
typedef struct
{
    GattBassClientMessageId id;   /*! service message id */
    ServiceHandle clntHndl;       /*! Reference handle for the instance */
    connection_id_t  cid;         /*! Connection ID */
    GattBassClientStatus status;  /*! Status of the initialization attempt */
} GattBassClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattBassClientTerminateReq API.

    NOTE : broadcastReceiveStateHandles contains both the handles of all the Broadcast Receive State
           Characteristics and the handles of their Client Characteristic Configurations Descriptors:
           Example with 2 Broadcast Receive State Characteristics (broadcast_source_num = 2):
           broadcastReceiveStateHandles[0] -> Handle of the first Broadcast Receive State characteristic
           broadcastReceiveStateHandles[1] -> Handle of the CCC of the first Broadcast Receive State
                                              characteristic
           broadcastReceiveStateHandles[2] -> Handle of the second Broadcast Receive State characteristic
           broadcastReceiveStateHandles[3] -> Handle of the CCC of the second Broadcast Receive State
                                              characteristic
*/
typedef struct
{
    GattBassClientMessageId id;                   /*! service message id */
    ServiceHandle clntHndl;                       /*! Reference handle for the instance */
    GattBassClientStatus status;                  /*! Status of the initialization
                                                      attempt */
    uint16 startHandle;                           /*! Start handle of BASS */
    uint16 endHandle;                             /*! End handle of BASS */
    uint16 broadcastSourceNum;                    /*! Number of Broadcast Receive State
                                                      characteristics */
    uint16 broadcastAudioScanControlPointHandle;  /*! Handle of the Audio Scan Control
                                                      Point characteristic*/
    uint16 broadcastReceiveStateHandles[1];       /*! Handles of the Broadcast Receive
                                                      characteristic and of their
                                                      Client Characteristic Configurations */
} GattBassClientTerminateCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM message that is sent
           by the library, as a result of setting notifications on the server for a Broadcast
           Receive State characteristic.
 */
typedef struct
{
    GattBassClientMessageId id;  /*! service message id */
    ServiceHandle clntHndl;      /*! Reference handle for the instance */
    uint8 sourceId;              /*! Source_id of the Broadcast Receive State characteristic*/
    status_t status;             /*! Status of the setting attempt */
} GattBassClientBroadcastReceiveStateSetNtfCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Remote Scan Stop operation.
*/
typedef struct
{
    GattBassClientMessageId id;  /*! service message id */
    ServiceHandle clntHndl;      /*! Reference handle for the instance */
    status_t status;             /*! Status of the setting attempt */
}GattBassClientRemoteScanStopCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_REMOTE_SCAN_START_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Remote Scan Start operation.
 */
typedef GattBassClientRemoteScanStopCfm GattBassClientRemoteScanStartCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_ADD_SOURCE_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Add Source operation.
 */
typedef GattBassClientRemoteScanStopCfm GattBassClientAddSourceCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_MODIFY_SOURCE_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Modify Source operation.
 */
typedef GattBassClientRemoteScanStopCfm GattBassClientModifySourceCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_SET_BROADCAST_CODE_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Set Broadcast Code operation.
 */
typedef GattBassClientRemoteScanStopCfm GattBassClientSetBroadcastCodeCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_REMOVE_SOURCE_CFM message that is sent by the library,
    as a result of writing the Broadcast Audio Scan Control point characteristic
    on the server using the Remove Source operation.
 */
typedef GattBassClientRemoteScanStopCfm GattBassClientRemoveSourceCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND message that is sent by the library,
    as a result of a notification of the remote broadcast receive state.

    NOTE: It's application/profile responsability to free the memory associated with all the pointers in this message.
 */
typedef struct
{
    GattBassClientMessageId id;                             /*! service message id */
    ServiceHandle clntHndl;                              /*! Reference handle for the instance */
    GattBassClientBroadcastReceiveState brsValue;           /*! Value of the Broadcast Receive State characteristic */
} GattBassClientBroadcastReceiveStateInd;

/*! @brief Contents of the GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM message that is sent
           by the library, as a result of reading the Client Characteristic Configuration of a
           Broadcast Receive State characteristic on the server.
 */
typedef struct
{
    GattBassClientMessageId id;  /*! service message id */
    ServiceHandle clntHndl;      /*! Reference handle for the instance */
    uint8 sourceId;              /*! Source_id of the Broadcast Receive State characteristic */
    status_t status;             /*! Status of the reading attempt */
    uint16 sizeValue;            /*! Value size */
    uint8 value[1];              /*! Read value */
} GattBassClientReadBroadcastReceiveStateCccCfm;

/*! @brief Contents of the GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message that is sent
           by the library, as a result of reading the Broadcast Receive State characteristic
           on the server.

     NOTE: It's application/profile responsability to free the memory associated with all the pointers in this message.
 */
typedef struct
{
    GattBassClientMessageId    id;                 /*! service message id */
    ServiceHandle  clntHndl;                    /*! Reference handle for the instance */
    status_t status;                               /*! Status of the reading attempt */
    GattBassClientBroadcastReceiveState brsValue;  /*! Value of the Broadcast Receive State
                                                       characteristic */
} GattBassClientReadBroadcastReceiveStateCfm;

/*!
    @brief GATT BASS Client Service Initialization Request.

    @param theAppTask  The client task that will receive messages from this library.
    @param initData    Configuration data for client initialization.
    @param deviceData  Cached handles/data from previous connection with Peer Device or
                       NULL if this is the first time this peer device has connected.

    NOTE: GATT_BASS_CLIENT_INIT_CFM will be received with a GattBassClientStatus status code.
*/
void GattBassClientInitReq(AppTask theAppTask,
                           const GattBassClientInitData   *initData,
                           const GattBassClientDeviceData *deviceData);


/*!
    @brief GATT BASS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_BASS_CLIENT_TERMINATE_CFM will be received with a GattBassClientStatus status code.
*/
void GattBassClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of one or all the Broadcast
           Receive State characteristics on a remote device, to enable notifications with the server.
           An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.
    @param allSource           If TRUE all the Client Characteristic Configurations will be written and
                               source_id parameter will be ignored.
    @param sourceId            Source id of the Broadcast Receive State characteristic whose
                               Client Characteristic Configuration is asked to be written. This will be
                               ignored, if allSource is TRUE.

    @return GattBassClientStatus Result of the operation

    NOTE: A GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM message will be sent to the registered
          application Task.

*/
GattBassClientStatus GattBassClientBroadcastReceiveStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                                                   uint8 sourceId,
                                                                                   bool allSource,
                                                                                   bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of a Broadcast Receive
           State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.
    @param sourceId  Source id of the Broadcast Receive State characteristic whose
                     Client Characteristic Configuration is asked to be read. If NULL, all the
                     Client Characteristic Configurations will be read.
    @param allSource If TRUE all the Client Characteristic Configurations will be read and
                     sourceId parameter will be ignored.

    @return GattBassClientStatus Result of the operation

    NOTE: A GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM message will be sent
          to the registered application Task.

*/
GattBassClientStatus GattBassClientReadBroadcastReceiveStateCccRequest(ServiceHandle clntHndl,
                                                                      uint8 sourceId,
                                                                      bool allSource);

/*!
    @brief This API is used to read a Broadcast Receive State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.
    @param sourceId  Source id of the Broadcast Receive State characteristic that has to be read.
                     If NULL, all the Broadcast receive Stete Characteristics will be read.
    @param allSource If TRUE all the Broadcast receive Stete Characteristics will be read and
                     sourceId parameter will be ignored.

    NOTE: A GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message will be sent
          to the registered application Task.
*/
GattBassClientStatus GattBassClientReadBroadcastReceiveStateRequest(ServiceHandle clntHndl,
                                                                    uint8 sourceId,
                                                                    bool allSource);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Remote Scan Stop operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                         FALSE otherwhise.

    NOTE: A GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM message will be sent to the registered application Task.

*/
void GattBassClientRemoteScanStopRequest(ServiceHandle clntHndl, bool noResponse);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Remote Scan Start operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                         FALSE otherwhise.

    NOTE: A GATT_BASS_CLIENT_REMOTE_SCAN_START_CFM message will be sent to the registered application Task.

*/
void GattBassClientRemoteScanStartRequest(ServiceHandle clntHndl, bool noResponse);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Add Source operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param param         Data of the Broadcast Source to add. I't responsability of the application
                         to free the memory (if it exists) associated to the metadata_value
                         contained in param.
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                         FALSE otherwhise.
    @param longWritte    TRUE if GATT long write procedure to be used.


    NOTE: A GATT_BASS_CLIENT_ADD_SOURCE_CFM message will be sent
          to the registered application Task.

*/
void GattBassClientAddSourceRequest(ServiceHandle clntHndl,
                                    GattBassClientAddSourceParam *param,
                                    bool noResponse,
                                    bool longWritte);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Modify Source operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param params         Data of the Broadcast Source to modify. I't responsability of the application
                          to free the memory (if it exists) associated to the metadata_value
                          contained in param.
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                          FALSE otherwhise.

    NOTE: A GATT_BASS_CLIENT_MODIFY_SOURCE_CFM message will be sent
          to the registered application Task.

*/
void GattBassClientModifySourceRequest(ServiceHandle clntHndl,
                                       GattBassClientModifySourceParam *params,
                                       bool noResponse);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Set Broadcast Code operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param sourceId      Source id of the Broadcast Receive State characteristic.
    @param broadcastCode Value of Broadcast Code to set (Should be formated in little-endian)
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                         FALSE otherwhise.

    NOTE: A GATT_BASS_CLIENT_SET_BROADCAST_CODE_CFM message will be sent
          to the registered application Task.

*/
void GattBassClientSetBroadcastCodeRequest(ServiceHandle clntHndl,
                                           uint8 sourceId,
                                           uint8 *broadcastCode,
                                           bool noResponse);

/*!
    @brief This API is used to write the Broadcast Audio Scan Control point characteristic
           in order to execute the Remove Source operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param sourceId      Source id of the Broadcast Receive State characteristic.
    @param noResponse    TRUE if the operation has to be executed with the GATT Write Without Response procedure,
                         FALSE otherwhise.

    NOTE: A GATT_BASS_CLIENT_REMOVE_SOURCE_CFM message will be sent
          to the registered application Task.

*/
void GattBassClientRemoveSourceRequest(ServiceHandle clntHndl,
                                       uint8 sourceId,
                                       bool noResponse);

GattBassClientDeviceData *GattBassClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_BASS_CLIENT_H */

