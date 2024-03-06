/****************************************************************************
Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_gmas_client.h
    
DESCRIPTION
    Header file for the GATT GMAS Client library.
*/


/*!
@file    gatt_gmas_client.h
@brief   Header file for the GATT GMAS Client library.

        This file provides documentation for the GATT GMAS Client library
        API (library name: gatt_gmas_client).
*/

#ifndef GATT_GMAS_CLIENT_H
#define GATT_GMAS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"
#include "csr_pmem.h"

/* Connection ID type */
typedef uint32       GmapClientConnectionId;

/*!
    @brief Handles of the GMAS characteristics.
*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 roleHandle;
    uint16 uggFeaturesHandle;
    uint16 ugtFeaturesHandle;
    uint16 bgsFeaturesHandle;
    uint16 bgrFeaturesHandle;
} GattGmasClientDeviceData;

/*!
    \brief GATT GMAS Client messages.
*/
typedef uint8                                          GattGmasClientMessageId;
#define GATT_GMAS_CLIENT_INIT_CFM                      ((GattGmasClientMessageId) 0x01)
#define GATT_GMAS_CLIENT_READ_ROLE_CFM                 ((GattGmasClientMessageId) 0x02)
#define GATT_GMAS_CLIENT_READ_UNICAST_FEATURES_CFM     ((GattGmasClientMessageId) 0x03)
#define GATT_GMAS_CLIENT_READ_BROADCAST_FEATURES_CFM   ((GattGmasClientMessageId) 0x04)
#define GATT_GMAS_CLIENT_TERMINATE_CFM                 ((GattGmasClientMessageId) 0x05)


/*!
    \brief GATT GMAS Client status code type.
*/
typedef uint16                                        GattGmasClientStatus;
#define GATT_GMAS_CLIENT_STATUS_SUCCESS              ((GattGmasClientStatus) 0x00)  /*!> Request was a success*/
#define GATT_GMAS_CLIENT_NOT_SUPPORTED               ((GattGmasClientStatus) 0x01)  /*!>  Not supported by remote device*/
#define GATT_GMAS_CLIENT_STATUS_COMMAND_INCOMPLETE   ((GattGmasClientStatus) 0x02)  /*!> Command requested could not be completed*/
#define GATT_GMAS_CLIENT_STATUS_DISCOVERY_ERR        ((GattGmasClientStatus) 0x03)  /*!> Error in discovery of one of the services*/
#define GATT_GMAS_CLIENT_STATUS_FAILED               ((GattGmasClientStatus) 0x04)  /*!> Request has failed*/
#define GATT_GMAS_CLIENT_STATUS_BUSY                 ((GattGmasClientStatus) 0x05)  /*!> Register for notif req pending*/
#define GATT_GMAS_CLIENT_STATUS_INVALID_PARAMETER    ((GattGmasClientStatus) 0x06)  /*!> Invalid parameter was supplied*/

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
    GmapClientConnectionId  cid;               /*! Connection ID. */
    uint16                  startHandle;       /*! The first handle of the service that needs to be accessed */
    uint16                  endHandle;         /*! The last handle of the service that needs to be accessed */
} GattGmasClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattGmasClientInitReq API.
*/
typedef struct
{
    GattGmasClientMessageId  id;
    GmapClientConnectionId   cid;        /*! Connection ID. */
    ServiceHandle            srvcHndl;   /*! Reference handle for the instance */
    GattGmasClientStatus     status;     /*! Status of the initialisation attempt */
} GattGmasClientInitCfm;

/*! @brief Result of reading of a requested characteristic on the server.
 */
typedef struct
{
    GattGmasClientMessageId  id;
    ServiceHandle            srvcHndl;    /*! Reference handle for the instance */
    uint8                    value;       /*! Characteristic Value */
    GattGmasClientStatus     status;      /*! Status of the reading attempt */
} GattGmasClientReadCommonCfm;

typedef GattGmasClientReadCommonCfm  GattGmasClientReadRoleCfm;
typedef GattGmasClientReadCommonCfm  GattGmasClientReadUnicastFeaturesCfm;
typedef GattGmasClientReadCommonCfm  GattGmasClientReadBroadcastFeaturesCfm;


/*!
    @brief Gatt client library message sent as a result of calling the GattGmasClientTerminateReq API.
*/
typedef struct
{
    GattGmasClientMessageId    id;
    ServiceHandle              srvcHndl;    /*! Reference handle for the instance */
    GattGmasClientStatus       status;      /*! Status of the initialisation attempt */
    GattGmasClientDeviceData   deviceData;  /*! Device data: handles used for the peer device. */
} GattGmasClientTerminateCfm;

/*!
    @brief GATT GMAS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    @param initData    Configuration data for client initialisation.
    @param deviceData  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_GMAS_CLIENT_INIT_CFM will be received with a GattGmasClientStatus status code.
*/
void GattGmasClientInitReq(AppTask theAppTask,
                           const GattGmasClientInitData   *initData,
                           const GattGmasClientDeviceData *deviceData);

/*!
    @brief This API is used to read the role value.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_GMAS_CLIENT_READ_ROLE_CFM message will be sent to the registered application Task.

*/
void GattGmasClientReadRoleReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Unicast features.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_GMAS_CLIENT_READ_UNICAST_FEATURES_CFM message will be sent to the registered application Task.

*/
void GattGmasClientReadUnicastFeaturesReq(ServiceHandle clntHndl, uint8 role);

/*!
    @brief This API is used to read the broadcast features.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_GMAS_CLIENT_READ_BROADCAST_FEATURES_CFM message will be sent to the registered application Task.

*/
void GattGmasClientReadBroadcastFeaturesReq(ServiceHandle clntHndl, uint8 role);

/*!
    @brief This API is used to retrieve the Service device data stored
           by the Service library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattGmasClientDeviceData : The structure containing start and end handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          by the function.

*/
GattGmasClientDeviceData *GattGmasClientGetDeviceDataReq(ServiceHandle clntHndl);

/*!
    @brief GATT GMAS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_GMAS_CLIENT_TERMINATE_CFM will be received with a GattGmasClientStatus status code.
*/
void GattGmasClientTerminateReq(ServiceHandle clntHndl);

#endif /* GATT_GMAS_CLIENT_H */

