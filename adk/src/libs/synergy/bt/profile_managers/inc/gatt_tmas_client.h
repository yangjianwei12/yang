/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_tmas_client.h
    
DESCRIPTION
    Header file for the GATT TMAS Client library.
*/


/*!
@file    gatt_tmas_client.h
@brief   Header file for the GATT TMAS Client library.

        This file provides documentation for the GATT TMAS Client library
        API (library name: gatt_tmas_client).
*/

#ifndef GATT_TMAS_CLIENT_H
#define GATT_TMAS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"
#include "csr_pmem.h"

/* TMAS service Characteristic type */
typedef uint16       RoleType;

/* Connection ID type */
typedef uint32       TmapClientConnectionId;

/*!
    @brief Handles of the TMAS characteristics.
*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 roleHandle;
} GattTmasClientDeviceData;

/*!
    \brief GATT TMAS Client messages.
*/

typedef uint8                                          GattTmasClientMessageId;

#define GATT_TMAS_CLIENT_INIT_CFM                               ((GattTmasClientMessageId) 0x01)
#define GATT_TMAS_CLIENT_ROLE_CFM                               ((GattTmasClientMessageId) 0x02)
#define GATT_TMAS_CLIENT_TERMINATE_CFM                          ((GattTmasClientMessageId) 0x03)


/*!
    \brief GATT TMAS Client status code type.
*/

typedef uint16                                          GattTmasClientStatus;

#define GATT_TMAS_CLIENT_STATUS_SUCCESS              ((GattTmasClientStatus) 0x00)  /*!> Request was a success*/
#define GATT_TMAS_CLIENT_NOT_SUPPORTED               ((GattTmasClientStatus) 0x01)  /*!>  Not supported by remote device*/
#define GATT_TMAS_CLIENT_STATUS_COMMAND_INCOMPLETE   ((GattTmasClientStatus) 0x02)  /*!> Command requested could not be completed*/
#define GATT_TMAS_CLIENT_STATUS_DISCOVERY_ERR        ((GattTmasClientStatus) 0x03)  /*!> Error in discovery of one of the services*/
#define GATT_TMAS_CLIENT_STATUS_FAILED               ((GattTmasClientStatus) 0x04)  /*!> Request has failed*/
#define GATT_TMAS_CLIENT_STATUS_BUSY                 ((GattTmasClientStatus) 0x05)  /*!> Register for notif req pending*/
#define GATT_TMAS_CLIENT_STATUS_INVALID_PARAMETER    ((GattTmasClientStatus) 0x06)  /*!> Invalid parameter was supplied*/

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
    TmapClientConnectionId  cid;               /*! Connection ID. */
    uint16                  startHandle;       /*! The first handle of the service that needs to be accessed */
    uint16                  endHandle;         /*! The last handle of the service that needs to be accessed */
} GattTmasClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattTmasClientInitReq API.
*/
typedef struct
{
    GattTmasClientMessageId  id;
    TmapClientConnectionId   cid;        /*! Connection ID. */
    ServiceHandle            srvcHndl;   /*! Reference handle for the instance */
    GattTmasClientStatus     status;     /*! Status of the initialisation attempt */
} GattTmasClientInitCfm;

/*! @brief Contents of the GATT_TMAS_CLIENT_ROLE_CFM message that is sent by the library,
    as a result of reading of a requested characteristic on the server.
 */
typedef struct
{
    GattTmasClientMessageId  id;
    ServiceHandle            srvcHndl;               /*! Reference handle for the instance */
    RoleType                 role;                   /* Role Characteristic */
    GattTmasClientStatus     status;                 /*! Status of the reading attempt */
} GattTmasClientRoleCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattTmasClientTerminateReq API.
*/
typedef struct
{
    GattTmasClientMessageId    id;
    ServiceHandle              srvcHndl;    /*! Reference handle for the instance */
    GattTmasClientStatus       status;      /*! Status of the initialisation attempt */
    GattTmasClientDeviceData   deviceData;  /*! Device data: handles used for the peer device. */
} GattTmasClientTerminateCfm;

/*!
    @brief GATT TMAS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    @param initData    Configuration data for client initialisation.
    @param deviceData  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_TMAS_CLIENT_INIT_CFM will be received with a GattTmasClientStatus status code.
*/
void GattTmasClientInitReq(AppTask theAppTask,
                           const GattTmasClientInitData   *initData,
                           const GattTmasClientDeviceData *deviceData);

/*!
    @brief This API is used to retrieve the Service device data stored
           by the Service library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattTmasClientDeviceData : The structure containing start and end handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          by the function.

*/
GattTmasClientDeviceData *GattTmasClientGetDeviceDataReq(ServiceHandle clntHndl);

/*!
    @brief GATT TMAS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_TMAS_CLIENT_TERMINATE_CFM will be received with a GattTmasClientStatus status code.
*/
void GattTmasClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the value of role characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_TMAS_CLIENT_ROLE_CFM message will be sent to the registered application Task.

*/
void GattTmasClientReadRoleReq(ServiceHandle clntHndl);

#endif /* GATT_TMAS_CLIENT_H */

