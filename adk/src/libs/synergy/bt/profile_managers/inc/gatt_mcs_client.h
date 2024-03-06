/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_mcs_client.h
    
DESCRIPTION
    Header file for the GATT MCS Client library.
*/


/*!
@file    gatt_mcs_client.h
@brief   Header file for the GATT MCS Client library.

        This file provides documentation for the GATT MCS Client library
        API (library name: gatt_mcs_client).
*/

#ifndef GATT_MCS_CLIENT_H
#define GATT_MCS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "mcp_mcs_common.h"

/*!
    @brief Handles of the MCS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 mediaPlayerNameHandle;
    uint16 mediaPlayerIconObjIdHandle;
    uint16 mediaPlayerIconUrlHandle;
    uint16 trackChangedHandle;
    uint16 trackTitleHandle;
    uint16 trackDurationHandle;
    uint16 trackPositionHandle;
    uint16 playbackSpeedHandle;
    uint16 seekingSpeedHandle;
    uint16 currentTrackSegmentsObjIdHandle;
    uint16 currentTrackObjIdHandle;
    uint16 nextTrackObjIdHandle;
    uint16 currentGroupObjIdHandle;
    uint16 parentGroupObjIdHandle;
    uint16 playingOrderHandle;
    uint16 playingOrderSuppHandle;
    uint16 mediaStateHandle;
    uint16 mediaControlPointHandle;
    uint16 mediaControlPointOpSuppHandle;
    uint16 searchResultsObjIdHandle;
    uint16 searchControlPointHandle;
    uint16 contentControlIdHandle;


    uint16 mediaPlayerNameCccHandle;
    uint16 trackTitleCccHandle;
    uint16 trackChangedCccHandle;
    uint16 trackDurationCccHandle;
    uint16 trackPositionCccHandle;
    uint16 playbackSpeedCccHandle;
    uint16 seekingSpeedCccHandle;
    uint16 currentTrackObjIdCccHandle;
    uint16 nextTrackObjIdCccHandle;
    uint16 currentGroupObjIdCccHandle;
    uint16 parentGroupObjIdCccHandle;
    uint16 playingOrderCccHandle;
    uint16 mediaStateCccHandle;
    uint16 mediaControlPointCccHandle;
    uint16 mediaControlPointOpSuppCccHandle;
    uint16 searchResultsObjIdCccHandle;
    uint16 searchControlPointCccHandle;
} GattMcsClientDeviceData;

/*! @brief Enumeration of messages a client task may receive from the client library.
 */

typedef uint8                                          GattMcsClientMessageId;

#define GATT_MCS_CLIENT_INIT_CFM                               ((GattMcsClientMessageId) 0x01)
#define GATT_MCS_CLIENT_TERMINATE_CFM                          ((GattMcsClientMessageId) 0x02)
#define GATT_MCS_CLIENT_NTF_IND                                ((GattMcsClientMessageId) 0x03)
#define GATT_MCS_CLIENT_NTF_CFM                                ((GattMcsClientMessageId) 0x04)
#define GATT_MCS_CLIENT_GET_MEDIA_PLAYER_ATTRIBUTE_CFM         ((GattMcsClientMessageId) 0x05)
#define GATT_MCS_CLIENT_SET_MEDIA_PLAYER_ATTRIBUTE_CFM         ((GattMcsClientMessageId) 0x06)
#define GATT_MCS_CLIENT_MEDIA_PLAYER_ATTRIBUTE_IND             ((GattMcsClientMessageId) 0x07)
#define GATT_MCS_CLIENT_SET_MEDIA_CONTROL_POINT_CFM            ((GattMcsClientMessageId) 0x08)


/*!
    \brief GATT MCS Client status code type.
*/

typedef uint16                                          GattMcsClientStatus;

#define GATT_MCS_CLIENT_STATUS_SUCCESS              ((GattMcsClientStatus) 0x01)  /*!> Request was a success*/
#define GATT_MCS_CLIENT_NOT_SUPPORTED               ((GattMcsClientStatus) 0x02) /*!>  Not supported by remote device*/
#define GATT_MCS_CLIENT_STATUS_MEDIAPLAYER_INACTIVE ((GattMcsClientStatus) 0x03) /*!>  Media Player in remote device is inactive*/
#define GATT_MCS_CLIENT_STATUS_COMMAND_INCOMPLETE   ((GattMcsClientStatus) 0x04)  /*!> Command requested could not be completed*/
#define GATT_MCS_CLIENT_STATUS_DISCOVERY_ERR        ((GattMcsClientStatus) 0x05)  /*!> Error in discovery of one of the services*/
#define GATT_MCS_CLIENT_STATUS_FAILED               ((GattMcsClientStatus) 0x06)  /*!> Request has failed*/
#define GATT_MCS_CLIENT_STATUS_BUSY                 ((GattMcsClientStatus) 0x07)  /*!> Register for notif req pending*/
#define GATT_MCS_CLIENT_STATUS_INVALID_PARAMETER    ((GattMcsClientStatus) 0x08)  /*!> Invalid parameter was supplied*/
#define GATT_MCS_CLIENT_STATUS_TRUNCATED_DATA       ((GattMcsClientStatus) 0x09)  /*!> Result is truncated*/

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
    McpConnectionId cid;       /*! Connection ID. */
    uint16 startHandle;       /*! The first handle of the service that needs to be accessed */
    uint16 endHandle;         /*! The last handle of the service that needs to be accessed */
} GattMcsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattMcsClientInitReq API.
*/
typedef struct
{
    GattMcsClientMessageId id;
    McpConnectionId        cid;         /*! Connection ID. */
    ServiceHandle          srvcHndl;   /*! Reference handle for the instance */
    GattMcsClientStatus    status;      /*! Status of the initialisation attempt */
} GattMcsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattMcsClientTerminateReq API.
*/
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle          srvcHndl;   /*! Reference handle for the instance */
    GattMcsClientStatus       status;      /*! Status of the initialisation attempt */
    GattMcsClientDeviceData   deviceData; /*! Device data: handles used for the peer device. */
} GattMcsClientTerminateCfm;

/*! @brief Contents of the GATT_MCS_CLIENT_NTF_CFM_T message that is sent by the library,
    as a result of setting notifications on the server for selected characteristics.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle         srvcHndl;    /*! Reference handle for the instance */
    MediaPlayerAttribute   charac;      /* Characteristic name */
    status_t         status;      /*! Status of the setting attempt */
} GattMcsClientNtfInd;

/*! @brief Contents of the GATT_MCS_CLIENT_NTF_CFM_T message that is sent by the library,
    as a result of setting notifications on the server for selected characteristics.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle         srvcHndl;   /*! Reference handle for the instance */
} GattMcsClientNtfCfm;

/*! @brief Contents of the GATT_MCS_READ_CHARAC_VALUE_CFM message that is sent by the library,
    as a result of reading of a requested characteristic on the server.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    MediaPlayerAttribute     charac;       /* Characteristic name */
    status_t     status;      /*! Status of the reading attempt */
    uint16            sizeValue;  /*! Value size*/
    uint8             *value;    /*! Read value */
} GattMcsClientGetMediaPlayerAttributeCfm;

/*! @brief Contents of the GATT_MCS_CHARAC_VALUE_IND message that is sent by the library,
    as a result of a notification of a characteristic.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle  srvcHndl;    /*! Reference handle for the instance */
    MediaPlayerAttribute     charac;      /* Characteristic name */
    uint16            sizeValue;  /*! Value size*/
    uint8             *value;    /*! Read value */
} GattMcsClientMediaPlayerAttributeInd;

/*! @brief Contents of the GATT_MCS_WRITE_CHARAC_VALUE_CFM message that is sent by the library,
    as a result of writing on the requested characteristic on the server.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    MediaPlayerAttribute     charac;      /* Characteristic name */
    status_t     status;      /*! Status of the reading attempt */
} GattMcsClientSetMediaPlayerAttributeCfm;

/*! @brief Contents of the GATT_MCS_SET_MEDIA_CONTROL_POINT_CFM message that is sent by the library,
    as a result of writing on Media Control Point characteristic on the server.
 */
typedef struct
{
    GattMcsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    GattMcsOpResult   status;      /*! Status of the reading attempt */
    GattMcsOpcode     op;
} GattMcsClientSetMediaControlPointCfm;


/*!
    @brief GATT MCS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    @param initData    Configuration data for client initialisation.
    @param deviceData  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_MCS_CLIENT_INIT_CFM will be received with a GattMcsClientStatus status code.
*/
void GattMcsClientInitReq(AppTask theAppTask,
                          const GattMcsClientInitData   *initData,
                          const GattMcsClientDeviceData *deviceData);


/*!
    @brief GATT MCS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_MCS_CLIENT_TERMINATE_CFM will be received with a GattMcsClientStatus status code.
*/
void GattMcsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of MCS related
    characteristics on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param characType           Bitmask of MCS characteristics.
    @param notif_value          Bitmask to enable/disable respective characteristics CCCD

    NOTE: A GATT_MCS_CLIENT_NTF_CFM message will be sent to the registered application Task.

*/
void  GattMcsClientRegisterForNotificationReq(ServiceHandle clntHndl, MediaPlayerAttributeMask characType, uint32 notif_value);

/*!
    @brief This API is used to read the value of requested characteristic.

    @param clntHndl  The service handle for this GATT Client Service.
    @param charac     Characteristic whose value has to be read.

    NOTE: A GATT_MCS_READ_CHARAC_VALUE_CFM message will be sent to the registered application Task.

*/
void GattMcsClientGetMediaPlayerAttribute(ServiceHandle clntHndl, MediaPlayerAttribute charac);

/*!
    @brief This API is used to change the value of a characteristic.

    @param clntHndl      The service handle for this GATT Client Service.
    @param charac         Characteristic whose value has to be written.
    @param len            Length of the value.
    @param val            Value to be written of the characteristic.

    NOTE: A GATT_MCS_WRITE_CHARAC_VALUE_CFM message will be sent to the registered application Task.

*/
void GattMcsClientSetMediaPlayerAttribute(ServiceHandle clntHndl, MediaPlayerAttribute charac, uint16 len, uint8* val);

/*!
    @brief This API is used to write the Media Control point characteristic in order to execute
           the opcode related operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param op             Opcode selected.
    @param val            Value of parameter to be sent along the write req.

    NOTE: A GATT_MCS_SET_MEDIA_CONTROL_POINT_CFM message will be sent to the registered application Task.

*/
void GattMcsClientSetMediaControlPoint(ServiceHandle clntHndl, GattMcsOpcode op, int32 val);

/*!
    @brief This API is used to retrieve the Media Control point characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattMcsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.

*/
GattMcsClientDeviceData *GattMcsClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_MCS_CLIENT_H */

