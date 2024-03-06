/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*!
@file    gatt_bass_server.h
@brief   Header file for the GATT Broadcast Audio Scan Service (BASS) server library.

         This file provides documentation for the GATT BASS server library
         API (library name: gatt_bass_server).
*/

#ifndef GATT_BASS_SERVER_H_
#define GATT_BASS_SERVER_H_

#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"
#include "csr_bt_gatt_lib.h"
#include <service_handle.h>
#include "csr_bt_addr.h"


/*!
    \brief BASS Broadacast Big Encryption value type.
*/
typedef uint8 GattBassServerBroadcastBigEncryption;

/*! { */
/*! Values of the BIG_encryption field of the Broadcast Receive State characteristic */
#define GATT_BASS_SERVER_NOT_ENCRYPTED            ((GattBassServerBroadcastBigEncryption)0x00u) /*!> Not encrypted*/
#define GATT_BASS_SERVER_BROADCAST_CODE_REQUIRED  ((GattBassServerBroadcastBigEncryption)0x01u) /*!> Broadcast_Code required*/
#define GATT_BASS_SERVER_DECRYPTING               ((GattBassServerBroadcastBigEncryption)0x02u) /*!> Decrypting */
#define GATT_BASS_BAD_CODE                        ((GattBassServerBroadcastBigEncryption)0x03u) /*!> Incorrect encryption key */
#define GATT_BASS_BIG_ENCRYPTION_LAST             ((GattBassServerBroadcastBigEncryption)0x04u)
/*! } */

/*!
    \brief BASS PA_Sync_state value type.
*/
typedef uint8 GattBassServerPaSyncState;

/*! { */
/*! Values of the pa_sync_state field of the Broadcast Receive State characteristic */
#define GATT_BASS_SERVER_NOT_SYNCHRONIZED         ((GattBassServerPaSyncState)0x00u)  /*!> Not synchronized to PA*/
#define GATT_BASS_SERVER_SYNC_INFO_REQUEST        ((GattBassServerPaSyncState)0x01u)  /*!> SyncInfo Request*/
#define GATT_BASS_SERVER_SYNCHRONIZED             ((GattBassServerPaSyncState)0x02u)  /*!> Synchronized to PA*/
#define GATT_BASS_SERVER_FAILED_TO_SYNCHRONIZE    ((GattBassServerPaSyncState)0x03u)  /*!> Failed to synchronize to PA*/
#define GATT_BASS_SERVER_NO_PAST                  ((GattBassServerPaSyncState)0x04u)  /*!> No PAST*/
#define GATT_BASS_SERVER_PA_SYNC_STATE_LAST       ((GattBassServerPaSyncState)0x05u)
/*! } */

/*!
    @brief Size of the Broadcast Code in bytes
*/
#define GATT_BASS_SERVER_BROADCAST_CODE_SIZE  (16)

/*!
    @brief Status code returned from the GATT BASS server library

    This status code indicates the outcome of the request.
*/
typedef uint16 GattBassServerStatus;

/*! { */
/*! Values for the BASS status code */
#define GATT_BASS_SERVER_STATUS_SUCCESS              ((GattBassServerStatus)0x0000u)  /*!> Request was a success*/
#define GATT_BASS_SERVER_STATUS_REGISTRATION_FAILED  ((GattBassServerStatus)0x0001u)  /*!> Server failed to register
                                                                     with GATT Manager.*/
#define GATT_BASS_SERVER_STATUS_INVALID_PARAMETER    ((GattBassServerStatus)0x0002u)  /*!> Invalid parameter was supplied*/
#define GATT_BASS_SERVER_STATUS_NOT_ALLOWED          ((GattBassServerStatus)0x0003u)  /*!> Request is not allowed*/
#define GATT_BASS_SERVER_STATUS_FAILED               ((GattBassServerStatus)0x0004u)  /*!> Request has failed*/
#define GATT_BASS_SERVER_STATUS_BC_SOURCE_IN_SYNC    ((GattBassServerStatus)0x0005u)  /*!> Request has failed, because the
                                                                     selected Broadcast Source is in sync
                                                                     with the PA and/or BIS/BIG*/
#define GATT_BASS_SERVER_STATUS_INVALID_SOURCE_ID    ((GattBassServerStatus)0x0006u)  /*!> Request has failed, because an invalid
                                                                     source id was supplied*/
#define GATT_BASS_SERVER_STATUS_NO_EMPTY_BRS         ((GattBassServerStatus)0x0007u)  /*!> Request has failed, because no source id
                                                                     was supplied and all the Broadcast
                                                                     Receive State Characteristics are full */
#define GATT_BASS_SERVER_STATUS_BRS_NOT_CHANGED      ((GattBassServerStatus)0x0008u)  /*!> BRS contents are not changed and hence
                                                                     NTFs are not sent over GATT*/

#define BASS_SERVER_STATUS_INVALID_BT_CONN_ID        (ATT_RESULT_INVALID_CID)

/*! @brief Enumeration of messages an application task can receive from the BASS server library.
 */
typedef enum
{
    /* Server messages */
    GATT_BASS_SERVER_BROADCAST_CODE_IND,
    GATT_BASS_SERVER_SCANNING_STATE_IND,
    GATT_BASS_SERVER_ADD_SOURCE_IND,
    GATT_BASS_SERVER_REMOVE_SOURCE_IND,
    GATT_BASS_SERVER_MODIFY_SOURCE_IND,
    GATT_BASS_SERVER_CONFIG_CHANGE_IND,

    /* Library message limit */
    GATT_BASS_SERVER_MESSAGE_TOP
} GattBassServerMessageId;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of BASS.
 */
typedef struct
{
    uint8  receiveStateCccSize;
    uint16 *receiveStateCcc;
} GattBassServerConfig;

/*!
    @brief Subgroups data
*/
typedef struct
{
     uint32 bisSync;     /*! BIS Synchronization state. */
     uint8 metadataLen;  /*! Metadata size. */
     uint8 *metadata;    /*! Metadata value. It exists only if
                             metadataLen is not zero.*/
} GattBassServerSubGroupsData;


/*!
   @brief Definition of data about the state of a broadcast source.
*/
typedef struct
{
    GattBassServerPaSyncState paSyncState;
    GattBassServerBroadcastBigEncryption bigEncryption;
    CsrBtTypedAddr sourceAddress;
    uint32 broadcastId;
    uint8 sourceAdvSid;
    uint8 numSubGroups;
    uint8 *badCode;
    GattBassServerSubGroupsData *subGroupsData;
} GattBassServerReceiveState;

/*! @brief Contents of the GATT_BASS_SERVER_BROADCAST_CODE_IND message that is sent by the library,
    due to a broadcast code sent by a client to attempt to decrypt or encrypt  BIS.
 */
typedef struct
{
    GattBassServerMessageId id;
    ServiceHandle srvcHndl;
    connection_id_t cid;
    uint8 sourceId;
    uint8 broadcastCode[GATT_BASS_SERVER_BROADCAST_CODE_SIZE];
} GattBassServerBroadcastCodeInd;

/*! @brief Contents of the GATT_BASS_SERVER_SCANNING_STATE_IND message that is sent by the library,
    due to a request from the client to start or to stop scanning on behalf of the server.
 */
typedef struct
{
    GattBassServerMessageId id;
    ServiceHandle srvcHndl;
    connection_id_t cid;
    bool clientScanningState;
} GattBassServerScanningStateInd;

/*! @brief Contents of the GATT_BASS_SERVER_ADD_SOURCE_IND message that is sent by the library,
    due to a request from the client to add a source.
 */
typedef struct
{
    GattBassServerMessageId id;
    ServiceHandle srvcHndl;
    connection_id_t cid;
    uint8 paSync;
    CsrBtTypedAddr advertiserAddress;
    uint32 broadcastId;
    uint8 sourceAdvSid;
    uint16 paInterval;
    uint8 numSubGroups;
    GattBassServerSubGroupsData *subGroupsData;
} GattBassServerAddSourceInd;

/*! @brief Contents of the GATT_BASS_SERVER_REMOVE_SOURCE_IND message that is sent by the library,
    due to a request from the client to remove a source.
 */
typedef struct
{
    GattBassServerMessageId id;
    connection_id_t cid;
    ServiceHandle srvcHndl;
    uint8 sourceId;
} GattBassServerRemoveSourceInd;

/*! @brief Contents of the GATT_BASS_SERVER_MODIFY_SOURCE_IND message that is sent by the library,
    due to a request from the client to synchronize to, or to stop synchronization to, a PA
    and/or a BIS.
 */
typedef struct
{
    GattBassServerMessageId id;
    ServiceHandle srvcHndl;
    connection_id_t cid;
    uint8 sourceId;
    uint8 paSyncState;
    uint16 paInterval;
    uint8 numSubGroups;
    GattBassServerSubGroupsData *subGroupsData;
} GattBassServerModifySourceInd;

/*! @brief Contents of the GATT_BASS_SERVER_CONFIG_CHANGE_IND message that is sent by the library,
    when any CCCD is toggled by the remote client.
 */
typedef struct 
{
    GattBassServerMessageId id;
    ServiceHandle srvcHndl;
    connection_id_t cid;
    bool configChangeComplete; /* will be TRUE if all CCCD of BASS are written once */
} GattBassServerConfigChangeInd;

/*!
    @brief Instantiate the GATT BASS Server Service Library

    The GATT Service Init function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param theAppTask                The client task that will receive messages from this Service.
    @param startHandle               The first handle in the ATT database for this Service instance.
    @param endHandle                 The last handle in the ATT database for this Service instance.
    @param broadcastReceiveStateNum  Numbers of Broadcast Receive State characteristic
                                     in the ATT database

    @return ServiceHandle If the service handle returned is 0, this indicates a failure
                             during GATT Service initialisation.
*/
ServiceHandle    GattBassServerInit(const AppTask theAppTask,
                                    uint16 startHandle,
                                    uint16 endHandle,
                                    uint8 broadcastReceiveStateNum);

/*!
    \brief Add configuration for a previously paired peer device, identified by its
    Connection ID (CID).

    @param srvcHndl Instance handle for the service.
    @param cid    The Connection ID to the peer device.
    @param config Client characteristic configurations for this connection.
                  If this is NULL, this indicates a default config should be used for the
                  peer device identified by the CID.

    @return status_t status of the Add Configuration operation.
*/
status_t GattBassServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t cid,
                                 const GattBassServerConfig *config);


/*!
    \brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    @param srvcHndl The GATT service instance handle.
    @param cid      A Connection ID for the peer device.

    @return GattBassServerConfig Pointer to the peer device configuration
            data. It is the application's responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory. It is necessary to free also the memory of
            the receiveStateCcc pointer that is an internal field of the GattBassServerConfig
            structure.
*/
GattBassServerConfig *GattBassServerRemoveConfig(ServiceHandle srvcHndl,
                                                 connection_id_t  cid);


/*!
    \brief Gets the configuration for a peer device, identified by its
           Connection ID.

    This gets the configuration for that peer device from the
    service library.
    It is recommnded to call this API after GATT_BASS_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    \param srvcHndl The GATT service instance handle.
    \param cid A Connection ID for the peer device.

    \return GattBassServerConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattBassServerConfig* GattBassServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid);


/*! \brief Query if the server has any client connections.
    @param srvcHndl The GATT service instance handle.

    @return TRUE if any clients are connected to the server.
*/
bool GattBassServerIsAnyClientConnected(ServiceHandle srvcHndl);

/*!
    @brief Get the number and the values of the source IDs of all the Broadcast Receive State characteristics.

    @param srvcHndl    The GATT service instance handle.
    @param sourceIdNum Pointer to the variable in which the library will put the number of Source IDs.

    @return uint8 * pointer to the list of values of the Source IDs. It's responsibility of the application
                    to free the memory associated to this pointer.
*/
uint8 * GattBassServerGetSourceIdsRequest(ServiceHandle srvcHndl,
                                          uint16 *sourceIdNum);

/*!
    @brief Get the info of a specific broadcast source.

    @param srvcHndl The GATT service instance handle.
    @param sourceId ID of the specific broadcast source.
    @param state    Pointer to the structure containing all the info.
                    The GattBassServerReceiveState structure contains a pointer (metadata):
                    it is the application's responsibility to free the memory of the this pointer.

    @return GattBassServerStatus Result of the operation.

*/
GattBassServerStatus GattBassServerGetBroadcastReceiveStateRequest(ServiceHandle srvcHndl,
                                                                   uint8 sourceId,
                                                                   GattBassServerReceiveState* state);
/*!
    @brief Add a broadcast source.

    @param srvcHndl   The GATT service instance handle.
    @param sourceId   Pointer to the value of the Source_ID of the Broadcast Receive State characteristic where to
                      add the new broadcast source.
                      If the value pointed by sourceId is zero, the library will try to add the source in the
                      first available Broadcast Receive State characteristic. If they are all busy, this function
                      will return the error code GATT_BASS_SERVER_STATUS_NO_EMPTY_BRS.
                      If the value pointed by sourceId is not zero and it matches the value contained in one
                      of the Broadcast Receive State characteristic, the library will try to add the source in that
                      characteristic. If the characteristic is not free, the library will try to free it and to add
                      the new source. If the PA state and/or the state of one or more BISes of the source in the Broadcast
                      Receive State to free is synchronized, the library will be not able to free the characteristic
                      and this function will return the error code GATT_BASS_SERVER_STATUS_BC_SOURCE_IN_SYNC.
    @param sourceInfo Pointer to the structure containing the info of the broadcast source to set
                      in the Receive State characteristic.
                      It is the application's responsibility to free the memory of this pointer.

    @return GattBassServerStatus Result of the operation.

*/
GattBassServerStatus  GattBassServerAddBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                              uint8 *sourceId,
                                                              GattBassServerReceiveState *sourceInfo);

/*!
    @brief Remove a broadcast source.

    @param srvcHndl The GATT service instance handle.
    @param sourceId ID of the specific broadcast source to remove.

    @return GattBassServerStatus Result of the operation.

*/
GattBassServerStatus  GattBassServerRemoveBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                                 uint8 sourceId);

/*!
    @brief Modify a broadcast source.

    @param srvcHndl   The GATT service instance handle.
    @param sourceId   ID of the specific broadcast source to modify.
    @param sourceInfo Pointer to the structure containing the info of the broadcast source to modify
                      in the Receive State characteristic.
                      It is the application responsibility to free the memory of this pointer.

    @return GattBassServerStatus Result of the operation.

*/
GattBassServerStatus  GattBassServerModifyBroadcastSourceRequest(ServiceHandle srvcHndl,
                                                                 uint8 sourceId,
                                                                 GattBassServerReceiveState *sourceInfo);

/*!
    @brief Get the Broadcast Code of a specific broadcast source.

    @param srvcHndl The GATT service instance handle.
    @param sourceId ID of the specific broadcast source.

    @return Broadcast Code for that Broadcast Source.

*/
uint8* GattBassServerGetBroadcastCodeRequest(ServiceHandle srvcHndl,
                                             uint8 sourceId);
/*!
    @brief Set the Broadcast Code of a specific broadcast source.

    @param srvcHndl      The GATT service instance handle.
    @param sourceId      ID of the specific broadcast source.
    @param broadcastCode The broadcast code to set.

    @return GattBassServerStatus Result of the operation.

*/
GattBassServerStatus GattBassServerSetBrodcastCodeRequest(ServiceHandle srvcHndl,
                                                          uint8 sourceId,
                                                          uint8 *broadcastCode);

#endif
