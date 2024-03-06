/****************************************************************************
Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    mcp.h
    
DESCRIPTION
    Header file for the Media Control Profile (MCP) library.
*/

/*!
@file    mcp.h
@brief   Header file for the GATT MCP library.

        This file provides documentation for the GATT MCP library
        API (library name: mcp).
*/

#ifndef MCP_H
#define MCP_H

#include "csr_bt_gatt_prim.h"
#include "service_handle.h"
#include "gatt_mcs_client.h"
#include "mcp_mcs_common.h"

#define MCP_PROFILE_PRIM             (SYNERGY_EVENT_BASE + MCP_PRIM)

/*!
    @brief MCP handles.

*/
typedef struct
{
    uint16                  mcsInstCount;
    GattMcsClientDeviceData *mcsHandle;
}McpHandles;

/*!
    @brief Initialisation parameters.

*/
typedef struct
{
    McpConnectionId cid;
}McpInitData;

/*!
    \brief MCP status code type.
*/
typedef uint16                           McpStatus;

#define MCP_STATUS_SUCCESS              ((McpStatus) 0x01)  /*!> Request was a success*/
#define MCP_STATUS_NOT_SUPPORTED        ((McpStatus) 0x02) /*!>  Not supported by remote device*/
#define MCP_STATUS_MEDIAPLAYER_INACTIVE ((McpStatus) 0x03) /*!>  Media Player in remote device is inactive*/
#define MCP_STATUS_COMMAND_INCOMPLETE   ((McpStatus) 0x04)  /*!> Command requested could not be completed*/
#define MCP_STATUS_DISCOVERY_ERR        ((McpStatus) 0x05)  /*!> Error in discovery of one of the services*/
#define MCP_STATUS_FAILED               ((McpStatus) 0x06)  /*!> Request has failed*/
#define MCP_STATUS_IN_PROGRESS          ((McpStatus) 0x07)  /*!> Request in progress*/
#define MCP_STATUS_INVALID_PARAMETER    ((McpStatus) 0x08)  /*!> Invalid parameter was supplied*/


/*! @brief Enumeration of messages a client task may receive from the profile library.
 */

typedef uint16                                   McpMessageId;

#define MCP_INIT_CFM                            ((McpMessageId) 0x01)
#define MCP_DESTROY_CFM                         ((McpMessageId) 0x02)
#define MCP_MCS_TERMINATE_CFM                   ((McpMessageId) 0x03)
#define MCP_NTF_IND                             ((McpMessageId) 0x04)
#define MCP_NTF_CFM                             ((McpMessageId) 0x05)
#define MCP_SET_MEDIA_PLAYER_ATTRIBUTE_CFM      ((McpMessageId) 0x06)
#define MCP_SET_MEDIA_CONTROL_POINT_CFM         ((McpMessageId) 0x07)

#define MCP_GET_MEDIA_PLAYER_NAME_CFM          ((McpMessageId) 0x08)
#define MCP_GET_MEDIA_PLAYER_ICON_URL_CFM      ((McpMessageId) 0x09)
#define MCP_GET_TRACK_TITLE_CFM                ((McpMessageId) 0x0A)
#define MCP_GET_TRACK_DURATION_CFM             ((McpMessageId) 0x0B)
#define MCP_GET_TRACK_POSITION_CFM             ((McpMessageId) 0x0C)
#define MCP_GET_PLAYBACK_SPEEED_CFM            ((McpMessageId) 0x0D)
#define MCP_GET_SEEKING_SPEED_CFM              ((McpMessageId) 0x0E)
#define MCP_GET_PLAYING_ORDER_CFM              ((McpMessageId) 0x0F)
#define MCP_GET_PLAYING_ORDER_SUPPORTED_CFM    ((McpMessageId) 0x10)
#define MCP_GET_MEDIA_STATE_CFM                ((McpMessageId) 0x11)
#define MCP_GET_SUPPORTED_OPCODES_CFM          ((McpMessageId) 0x12)
#define MCP_GET_CONTENT_CONTROL_ID_CFM         ((McpMessageId) 0x13)

#define MCP_MEDIA_PLAYER_NAME_IND               ((McpMessageId) 0x14)
#define MCP_TRACK_CHANGED_IND                   ((McpMessageId) 0x15)
#define MCP_TRACK_TITLE_IND                     ((McpMessageId) 0x16)
#define MCP_TRACK_DURATION_IND                  ((McpMessageId) 0x17)
#define MCP_TRACK_POSITION_IND                  ((McpMessageId) 0x18)
#define MCP_PLAYBACK_SPEEED_IND                 ((McpMessageId) 0x19)
#define MCP_SEEKING_SPEED_IND                   ((McpMessageId) 0x1A)
#define MCP_PLAYING_ORDER_IND                   ((McpMessageId) 0x1B)
#define MCP_MEDIA_STATE_IND                     ((McpMessageId) 0x1C)
#define MCP_SUPPORTED_OPCODES_IND               ((McpMessageId) 0x1D)

/*!
    @brief Profile library message sent as a result of calling the McpInitReq API.
*/
typedef struct
{
    McpMessageId        id;
    McpStatus          status;          /*! Status of the initialisation attempt*/
    McpProfileHandle   prflHndl;        /*! MCP profile handle */
    uint16             mcsInstCount;
    ServiceHandle     *mcsSrvcHandle;
} McpInitCfm;

/*!
    @brief Profile library message sent as a result of calling the McpDestroyReq API.

    This message will send at first with the value of status of MCP_STATUS_IN_PROGRESS.
    Another MCP_DESTROY_CFM message will be sent with the final status (success or fail),
    after MCP_MCS_TERMINATE_CFM has been received.
*/
typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;     /*! MCP profile handle*/
    McpStatus         status;       /*! Status of the initialisation attempt*/
} McpDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the McpDestroyReq API and
           of the receiving of the GATT_MCS_CLIENT_TERMINATE_CFM message from the gatt_mcs_client
           library.
*/
typedef struct
{
    McpMessageId                    id;
    McpProfileHandle               prflHndl;          /*! MCP profile handle*/
    McpStatus                      status;            /*! Status of the termination attempt*/
    GattMcsClientDeviceData        mcsHandle;         /*! Characteristic handles of MCS*/
    bool                           moreToCome;        /*! TRUE if more of this message will come, FALSE otherwise*/
} McpMcsTerminateCfm;

/*! @brief Contents of the McpNtfInd message that is sent by the library,
    as a result of setting notifications on the server for selected characteristics.
    This will have results for individual characteristics set notification attempt.
 */
typedef struct
{
    McpMessageId              id;
    McpProfileHandle         prflHndl;     /*! MCP profile handle*/
    ServiceHandle            srvcHndl;     /*! Reference handle for the instance */
    MediaPlayerAttribute     charac;      /* Characteristic name */
    McpStatus                status;       /*! Status of the setting attempt */
} McpNtfInd;

/*! @brief Contents of the McpNtfCfm message that is sent by the library,
    as a result of setting notifications on the server for selected characteristics.
    This marks the end of the register for notification request.
 */
typedef struct
{
    McpMessageId              id;
    McpProfileHandle         prflHndl;     /*! MCP profile handle*/
    ServiceHandle            srvcHndl;     /*! Reference handle for the instance */
    McpStatus                status;
} McpNtfCfm;

/*! @brief Contents of the McpCommonWriteMcsCfm message that is sent by the library,
    as a result of writing on the requested characteristic on the server.
 */
typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;    /*! MCP profile handle*/
    ServiceHandle     srvcHndl;    /*! Reference handle for the instance */
    MediaPlayerAttribute charac;      /* Characteristic name */
    McpStatus          status;      /*! Status of the writing attempt */
} McpSetMediaPlayerAttributeCfm;

/*! @brief Contents of the McpSetMediaControlPointCfm message that is sent by the library,
    as a result of writing on Media Control Point characteristic on the server.
 */
typedef struct
{
    McpMessageId         id;
    McpProfileHandle    prflHndl;     /*! MCP profile handle*/
    ServiceHandle       srvcHndl;     /*! Reference handle for the instance */
    McpStatus           status;       /*! Status of the reading attempt */
    GattMcsOpcode       op;
} McpSetMediaControlPointCfm;

/*! @brief Contents of the McpGetMediaPlayerNameCfm message that is sent by the library,
    as response to a read of the Media Player Name.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus         status;           /*! Status of the reading attempt */
    uint16            len;              /*! Value length */
    uint8*            name;            /*! Value */
} McpGetMediaPlayerNameCfm;

/*! @brief Contents of the McpGetMediaPlayerIconUrlCfm message that is sent by the library,
    as response to a read of the Media Player Icon Url.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    uint16            len;              /*! Value length */
    uint8*            iconUrl;          /*! Value */
} McpGetMediaPlayerIconUrlCfm;


/*! @brief Contents of the McpGetTrackTitleCfm message that is sent by the library,
    as response to a read of the Title Track.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    uint16            len;              /*! Value length */
    uint8*            trackTitle;       /*! Value */
} McpGetTrackTitleCfm;

/*! @brief Contents of the McpGetTrackDurationCfm message that is sent by the library,
    as response to a read of the Title Duration.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    int32             trackDuration;    /*! Value */
} McpGetTrackDurationCfm;

/*! @brief Contents of the McpGetTrackPositionCfm message that is sent by the library,
    as response to a read of the Title Position.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    int32             trackPosition;    /*! Value */
} McpGetTrackPositionCfm;

/*! @brief Contents of the McpGetPlaybackSpeedCfm message that is sent by the library,
    as response to a read of the Playback Speed.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    int8              playbackSpeed;    /*! Value */
} McpGetPlaybackSpeedCfm;

/*! @brief Contents of the McpGetSeekingSpeedCfm message that is sent by the library,
    as response to a read of the Seeking Speed.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    int8              seekingSpeed;     /*! Value */
} McpGetSeekingSpeedCfm;

/*! @brief Contents of the McpGetPlayinOrderCfm message that is sent by the library,
    as response to a read of the Playing Order.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    uint8             playingOrder;     /*! Value */
} McpGetPlayinOrderCfm;

/*! @brief Contents of the McpGetPlayinOrderSupportedCfm message that is sent by the library,
    as response to a read of the Supported Playing Order.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    McpStatus          status;                    /*! Status of the reading attempt */
    uint16            playingOrderSupported;     /*! Value */
} McpGetPlayinOrderSupportedCfm;

/*! @brief Contents of the McpGetMediaStateCfm message that is sent by the library,
    as response to a read of the Media State.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    McpStatus          status;                    /*! Status of the reading attempt */
    uint8             mediaState;                /*! Value */
} McpGetMediaStateCfm;

/*! @brief Contents of the McpGetOpcodesSupportedCfm message that is sent by the library,
    as response to a read of the Media Control Point Opcodes supported.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    McpStatus          status;                    /*! Status of the reading attempt */
    uint32            opcodesSupported;          /*! Value */
} McpGetOpcodesSupportedCfm;

/*! @brief Contents of the McpGetContentControlIdCfm message that is sent by the library,
    as response to a read of the Content Control Id.
*/

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    McpStatus          status;                    /*! Status of the reading attempt */
    uint8              ccid;                      /*! Value */
} McpGetContentControlIdCfm;

/*! @brief Contents of the McpMediaPlayerNameNotifInd message that is sent by the library,
    as a result of a notification of a Media Player Name.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    uint16            len;              /*! Value length */
    uint8*            name;             /*! Value */
} McpMediaPlayerNameInd;

/*! @brief Contents of the McpTrackChangedNotifInd message that is sent by the library,
    as a result of a notification when track is changed.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
} McpTrackChangedInd;

/*! @brief Contents of the McpTrackTitleNotifInd message that is sent by the library,
    as a result of  notification of Title track.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    uint16            len;              /*! Value length */
    uint8*            trackTitle;       /*! Value */
} McpTrackTitleInd;

/*! @brief Contents of the McpTrackDurationNotifInd message that is sent by the library,
    as a result of a notification of Duration of currently playing track.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    int32             trackDuration;    /*! Value */
} McpTrackDurationInd;

/*! @brief Contents of the McpTrackPositionNotifInd message that is sent by the library,
    as a result of a notification of Position of currently playing track(when media state is other 
    than PLAY).
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    int32             trackPosition;    /*! Value */
} McpTrackPositionInd;

/*! @brief Contents of the McpPlaybackSpeedNotifInd message that is sent by the library,
    as a result of a notification when Playback speed changes.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    int8              playbackSpeed;    /*! Value */
} McpPlaybackSpeedInd;

/*! @brief Contents of the McpSeekingSpeedNotifInd message that is sent by the library,
    as a result of a notification when seeking speed changes.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    McpStatus          status;           /*! Status of the reading attempt */
    int8              seekingSpeed;     /*! Value */
} McpSeekingSpeedInd;

/*! @brief Contents of the McpPlayinOrderNotifInd message that is sent by the library,
    as a result of a notification when playing order changes.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;         /*! MCP profile handle*/
    ServiceHandle     srvcHndl;         /*! Reference handle for the instance */
    uint8             playingOrder;     /*! Value */
} McpPlayingOrderInd;

/*! @brief Contents of the McpMediaStateNotifInd message that is sent by the library,
    as a result of a notification of a Media State change.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    uint8             mediaState;                /*! Value */
} McpMediaStateInd;

/*! @brief Contents of the McpOpcodesSupportedNotifInd message that is sent by the library,
    as a result of a notification when supported opcode changes.
 */

typedef struct
{
    McpMessageId       id;
    McpProfileHandle  prflHndl;                  /*! MCP profile handle*/
    ServiceHandle     srvcHndl;                  /*! Reference handle for the instance */
    uint32            opcodesSupported;          /*! Value */
} McpOpcodesSupportedInd;


/*!
    @brief Initialises the Gatt MCP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt MCP library.

    @param appTask           The Task that will receive the messages sent from this profile library
    @param clientInitParams   Initialisation parameters
    @param deviceData         MCS service handles
    NOTE: A MCP_INIT_CFM with McpStatus code equal to MCP_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed MCP_INIT_CFM will be received with a McpStatusT
          that indicates the result of the initialisation.
*/
void McpInitReq(AppTask appTask,
                McpInitData *clientInitParams,
                McpHandles  *deviceData);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the McpInitReq API.

    @param profileHandle The Profile handle.

    NOTE: A MCP_DESTROY_CFM with McpStatus code equal to MCP_STATUS_IN_PROGRESS will be received as indication
          that the profile library destroy started. Post that for each MCS instance a MCP_MCS_TERMINATE_CFM will
          be received. Once completed MCP_DESTROY_CFM will be received with a McpStatus that indicates the
          result of the destroy.
*/
void McpDestroyReq(McpProfileHandle profileHandle);

/*!
    @brief This API is used to write the client characteristic configuration of MCS related
    characteristics on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param mcsHandle           The MCS instance handle.
    @param characType          Bitmask of MCS characteristics.
    @param notifValue          Bitmask to enable/disable respective characteristics CCCD.

    NOTE: MCP_NTF_CFM message will be sent to the registered application Task.
          After calling this API, Application shall wait for the MCP_NTF_CFM to come before
          calling any other API.

*/
void McpRegisterForNotificationReq(McpProfileHandle profileHandle, ServiceHandle mcsHandle, MediaPlayerAttributeMask characType, uint32 notifValue);

/*!
    @brief This API is used to read the value of requested characteristic.

    @param profileHandle       The Profile handle.
    @param mcsHandle           The MCS instance handle.
    @param charac               Characteristic whose value has to be read.

    NOTE: A MCP_GET_XXXXXXX_CFM message will be sent to the registered application Task.

*/
void McpGetMediaPlayerAttribute(McpProfileHandle profileHandle, ServiceHandle mcsHandle, MediaPlayerAttribute charac);

/*!
    @brief This API is used to change the value of a characteristic.

    @param profileHandle       The Profile handle.
    @param mcsHandle           The MCS instance handle.
    @param charac               Characteristic whose value has to be written.
    @param len                  Length of the value.
    @param val                  Value to be written of the characteristic.

    NOTE: A MCP_SET_MEDIA_PLAYER_ATTRIBUTE_CFM message will be sent to the registered application Task.

*/
void McpSetMediaPlayerAttribute(McpProfileHandle profileHandle, ServiceHandle mcsHandle, MediaPlayerAttribute charac, uint16 len, uint8 *val);

/*!
    @brief This API is used to write the Media Control point characteristic in order to execute
           the opcode related operation on the server device.

    @param profileHandle       The Profile handle.
    @param mcsHandle           The MCS instance handle.
    @param op                   Opcode selected.
    @param val                  Value of parameter to be sent along the write req.

    NOTE: A MCP_SET_MEDIA_CONTROL_POINT_CFM message will be sent to the registered application Task.
          For opcodes which doesn't support additional parameters, if some value is sent it will be ignored.

*/
void McpSetMediaControlPoint(McpProfileHandle profileHandle, ServiceHandle mcsHandle, GattMcsOpcode op, int32 val);

/*!
    @brief This API is used to retrieve the Media Control point characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param profileHandle       The Profile handle.
    @param mcsHandle           The MCS instance handle.

    @return GattMcsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately to the app.

*/
GattMcsClientDeviceData *McpGetMediaPlayerAttributeHandles(McpProfileHandle profileHandle, ServiceHandle mcsHandle);

#endif /* MCP_H */

