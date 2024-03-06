/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#ifndef GATT_MCS_SERVER_H
#define GATT_MCS_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "service_handle.h"
#include "csr_pmem.h"

#define MCS_SERVER_SERVICE_PRIM             (SYNERGY_EVENT_BASE + MCS_SERVER_PRIM)

/*!
  Media Control Services Opcodes
  These are written by client to Media Control Point characteristic of the MCS server
  in order change it's state
*/

#define MCS_MAX_CONNECTIONS (3)

typedef uint8 GattMcsMediaControlPointType;

#define MCS_OPCODE_PLAY                    ((GattMcsMediaControlPointType)0x01)
#define MCS_OPCODE_PAUSE                   ((GattMcsMediaControlPointType)0x02)
#define MCS_OPCODE_FAST_REWIND             ((GattMcsMediaControlPointType)0x03)
#define MCS_OPCODE_FAST_FORWARD            ((GattMcsMediaControlPointType)0x04)
#define MCS_OPCODE_STOP                    ((GattMcsMediaControlPointType)0x05)
#define MCS_OPCODE_MOVE_RELATIVE           ((GattMcsMediaControlPointType)0x10)
#define MCS_OPCODE_PREVIOUS_SEGMENT        ((GattMcsMediaControlPointType)0x20)
#define MCS_OPCODE_NEXT_SEGMENT            ((GattMcsMediaControlPointType)0x21)
#define MCS_OPCODE_FIRST_SEGMENT           ((GattMcsMediaControlPointType)0x22)
#define MCS_OPCODE_LAST_SEGMENT            ((GattMcsMediaControlPointType)0x23)
#define MCS_OPCODE_GOTO_SEGMENT            ((GattMcsMediaControlPointType)0x24)
#define MCS_OPCODE_PREVIOUS_TRACK          ((GattMcsMediaControlPointType)0x30)
#define MCS_OPCODE_NEXT_TRACK              ((GattMcsMediaControlPointType)0x31)
#define MCS_OPCODE_FIRST_TRACK             ((GattMcsMediaControlPointType)0x32)
#define MCS_OPCODE_LAST_TRACK              ((GattMcsMediaControlPointType)0x33)
#define MCS_OPCODE_GOTO_TRACK              ((GattMcsMediaControlPointType)0x34)
#define MCS_OPCODE_PREVIOUS_GROUP          ((GattMcsMediaControlPointType)0x40)
#define MCS_OPCODE_NEXT_GROUP              ((GattMcsMediaControlPointType)0x41)
#define MCS_OPCODE_FIRST_GROUP             ((GattMcsMediaControlPointType)0x42)
#define MCS_OPCODE_LAST_GROUP              ((GattMcsMediaControlPointType)0x43)
#define MCS_OPCODE_GOTO_GROUP              ((GattMcsMediaControlPointType)0x44)

/*!
  Media Control Services Supported Opcodes
  Bit mask indicating which opcodes are currently supported by the Media Control Service
*/

typedef uint32 GattMcsOpcodeTypeSupported;

#define MCS_SUPPORTED_OPCODE_PLAY                  ((GattMcsOpcodeTypeSupported)0x00000001)
#define MCS_SUPPORTED_OPCODE_PAUSE                 ((GattMcsOpcodeTypeSupported)0x00000002)
#define MCS_SUPPORTED_OPCODE_FAST_REWIND           ((GattMcsOpcodeTypeSupported)0x00000004)
#define MCS_SUPPORTED_OPCODE_FAST_FORWARD          ((GattMcsOpcodeTypeSupported)0x00000008)
#define MCS_SUPPORTED_OPCODE_STOP                  ((GattMcsOpcodeTypeSupported)0x00000010)
#define MCS_SUPPORTED_OPCODE_MOVE_RELATIVE         ((GattMcsOpcodeTypeSupported)0x00000020)
#define MCS_SUPPORTED_OPCODE_PREVIOUS_SEGMENT      ((GattMcsOpcodeTypeSupported)0x00000040)
#define MCS_SUPPORTED_OPCODE_NEXT_SEGMENT          ((GattMcsOpcodeTypeSupported)0x00000080)
#define MCS_SUPPORTED_OPCODE_FIRST_SEGMENT         ((GattMcsOpcodeTypeSupported)0x00000100)
#define MCS_SUPPORTED_OPCODE_LAST_SEGMENT          ((GattMcsOpcodeTypeSupported)0x00000200)
#define MCS_SUPPORTED_OPCODE_GOTO_SEGMENT          ((GattMcsOpcodeTypeSupported)0x00000400)
#define MCS_SUPPORTED_OPCODE_PREVIOUS_TRACK        ((GattMcsOpcodeTypeSupported)0x00000800)
#define MCS_SUPPORTED_OPCODE_NEXT_TRACK            ((GattMcsOpcodeTypeSupported)0x00001000)
#define MCS_SUPPORTED_OPCODE_FIRST_TRACK           ((GattMcsOpcodeTypeSupported)0x00002000)
#define MCS_SUPPORTED_OPCODE_LAST_TRACK            ((GattMcsOpcodeTypeSupported)0x00004000)
#define MCS_SUPPORTED_OPCODE_GOTO_TRACK            ((GattMcsOpcodeTypeSupported)0x00008000)
#define MCS_SUPPORTED_OPCODE_PREVIOUS_GROUP        ((GattMcsOpcodeTypeSupported)0x00010000)
#define MCS_SUPPORTED_OPCODE_NEXT_GROUP            ((GattMcsOpcodeTypeSupported)0x00020000)
#define MCS_SUPPORTED_OPCODE_FIRST_GROUP           ((GattMcsOpcodeTypeSupported)0x00040000)
#define MCS_SUPPORTED_OPCODE_LAST_GROUP            ((GattMcsOpcodeTypeSupported)0x00080000)
#define MCS_SUPPORTED_OPCODE_GOTO_GROUP            ((GattMcsOpcodeTypeSupported)0x00100000)

/*!
  Media Control Point Access Result Code
  These codes are sent in Notification response to Remote device which
  writes  to Media Control Point characteristic
*/

typedef uint8 GattMediaControlPointAccessResultCode;

#define MCS_CONTROL_POINT_ACCESS_SUCCESS               ((GattMediaControlPointAccessResultCode)0x01)
#define MCS_OPCODE_NOT_SUPPORTED                       ((GattMediaControlPointAccessResultCode)0x02)
#define MCS_MEDIA_PLAYER_INACTIVE                      ((GattMediaControlPointAccessResultCode)0x03)
#define MCS_CONTROL_POINT_COMMAND_CANNOT_BE_COMPLETED  ((GattMediaControlPointAccessResultCode)0x04)


/*!
  Search Control Point  Result Code
  These are the result codes sent by server when remote device performs
  search operation
*/

typedef uint8 GattSearchControlPointResultCode;

#define MCS_SEARCH_SUCCESS  ((GattSearchControlPointResultCode)0x01)
#define MCS_SEARCH_FAILURE  ((GattSearchControlPointResultCode)0x02)

/*!
  Media State
  The valid states of the media player as defined by Media Control Service Spec
*/
typedef uint8 GattMcsMediaStateType;

#define MCS_MEDIA_STATE_INACTIVE ((GattMcsMediaStateType)0x00)
#define MCS_MEDIA_STATE_PLAYING  ((GattMcsMediaStateType)0x01)
#define MCS_MEDIA_STATE_PAUSED   ((GattMcsMediaStateType)0x02)
#define MCS_MEDIA_STATE_SEEKING  ((GattMcsMediaStateType)0x03)

/*!
  Media Playing Order
  Media Playing order as defined by MCS specoification
*/

typedef uint8 GattMcsMediaPlayingOrderType;

#define MCS_PO_SINGLE_ONCE          ((GattMcsMediaPlayingOrderType)0x01)
#define MCS_PO_SINGLE_REPEAT        ((GattMcsMediaPlayingOrderType)0x02)
#define MCS_PO_IN_ORDER_ONCE        ((GattMcsMediaPlayingOrderType)0x03)
#define MCS_PO_IN_ORDER_REPEAT      ((GattMcsMediaPlayingOrderType)0x04)
#define MCS_PO_OLDEST_ONCE          ((GattMcsMediaPlayingOrderType)0x05)
#define MCS_PO_OLDEST_REPEAT        ((GattMcsMediaPlayingOrderType)0x06)
#define MCS_PO_NEWEST_ONCE          ((GattMcsMediaPlayingOrderType)0x07)
#define MCS_PO_NEWEST_REPEAT        ((GattMcsMediaPlayingOrderType)0x08)
#define MCS_PO_SHUFFLE_ONCE         ((GattMcsMediaPlayingOrderType)0x09)
#define MCS_PO_SHUFFLE_REPEAT       ((GattMcsMediaPlayingOrderType)0x0A)


/*!
  Media Playing Order
  Bitmask indicating the supported Playing Order by Media Player
*/

typedef uint16 GattMcsMediaSupportedPlayingOrderType;

#define MCS_POS_SINGLE_ONCE           ((GattMcsMediaSupportedPlayingOrderType)0x0001)
#define MCS_POS_SINGLE_REPEAT         ((GattMcsMediaSupportedPlayingOrderType)0x0002)
#define MCS_POS_IN_ORDER_ONCE         ((GattMcsMediaSupportedPlayingOrderType)0x0004)
#define MCS_POS_IN_ORDER_REPEAT       ((GattMcsMediaSupportedPlayingOrderType)0x0008)
#define MCS_POS_OLDEST_ONCE           ((GattMcsMediaSupportedPlayingOrderType)0x0010)
#define MCS_POS_OLDEST_REPEAT         ((GattMcsMediaSupportedPlayingOrderType)0x0020)
#define MCS_POS_NEWEST_ONCE           ((GattMcsMediaSupportedPlayingOrderType)0x0040)
#define MCS_POS_NEWEST_REPEAT         ((GattMcsMediaSupportedPlayingOrderType)0x0080)
#define MCS_POS_SHUFFLE_ONCE          ((GattMcsMediaSupportedPlayingOrderType)0x0100)
#define MCS_POS_SHUFFLE_REPEAT        ((GattMcsMediaSupportedPlayingOrderType)0x0200)


#define TRACK_POSITION_UNAVAILABLE  (0xFFFFFFFF)
#define INVALID_TRACK_DURATION      (0xFFFFFFFF)


#define MEDIA_PLAYER_NAME_SIZE   (64)
#define TRACK_TITLE_SIZE         (256)
#define MEDIA_PLAYER_URL_SIZE    (256)


/*!
  MCS server Characteristics
  Bitmask indicating all the characteristics currently supported by the Media Player
*/


typedef uint32 MediaPlayerAttributeType;

#define MCS_SRV_MEDIA_PLAYER_NAME                         ((MediaPlayerAttributeType)0x00000001)
#define MCS_SRV_TRACK_CHANGED                             ((MediaPlayerAttributeType)0x00000002)
#define MCS_SRV_TRACK_TITLE                               ((MediaPlayerAttributeType)0x00000004)
#define MCS_SRV_TRACK_DURATION                            ((MediaPlayerAttributeType)0x00000008)
#define MCS_SRV_TRACK_POSITION                            ((MediaPlayerAttributeType)0x00000010)
#define MCS_SRV_PLAYBACK_SPEED                            ((MediaPlayerAttributeType)0x00000020)
#define MCS_SRV_SEEKING_SPEED                             ((MediaPlayerAttributeType)0x00000040)
#define MCS_SRV_CURRENT_TRACK_OBJECT_ID                   ((MediaPlayerAttributeType)0x00000080)
#define MCS_SRV_NEXT_TRACK_OBJECT_ID                      ((MediaPlayerAttributeType)0x00000100)
#define MCS_SRV_CURRENT_GROUP_OBJECT_ID                   ((MediaPlayerAttributeType)0x00000200)
#define MCS_SRV_PARENT_GROUP_OBJECT_ID                    ((MediaPlayerAttributeType)0x00000400)
#define MCS_SRV_PLAYING_ORDER                             ((MediaPlayerAttributeType)0x00000800)
#define MCS_SRV_PLAYING_ORDER_SUPPORTED                   ((MediaPlayerAttributeType)0x00001000)
#define MCS_SRV_MEDIA_STATE                               ((MediaPlayerAttributeType)0x00002000)
#define MCS_SRV_MEDIA_CONTROL_POINT                       ((MediaPlayerAttributeType)0x00004000)
#define MCS_SRV_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED     ((MediaPlayerAttributeType)0x00008000)
#define MCS_SRV_SEARCH_RESULTS_OBJECT_ID                  ((MediaPlayerAttributeType)0x00010000)
#define MCS_SRV_SEARCH_CONTROL_POINT                      ((MediaPlayerAttributeType)0x00020000)
#define MCS_SRV_CONTENT_CONTROL_ID                        ((MediaPlayerAttributeType)0x00040000)
#define MCS_SRV_MEDIA_PLAYER_ICON_URL                     ((MediaPlayerAttributeType)0x00080000)

/*!
  MCS message Id's
*/

typedef uint16 GattMcsMessageId;

#define GATT_MCS_TRACK_POSITION_WRITE_IND          ((GattMcsMessageId)(0))
#define GATT_MCS_PLAYBACK_SPEED_WRITE_IND          ((GattMcsMessageId)(1))
#define GATT_MCS_SEEKING_SPEED_WRITE_IND           ((GattMcsMessageId)(2))
#define GATT_MCS_CURRENT_TRACK_OBJECTID_WRITE_IND  ((GattMcsMessageId)(3))
#define GATT_MCS_NEXT_TRACK_OBJECTID_WRITE_IND     ((GattMcsMessageId)(4))
#define GATT_MCS_CURRENT_GROUP_OBJECTID_WRITE_IND  ((GattMcsMessageId)(5))
#define GATT_MCS_PLAYING_ORDER_WRITE_IND           ((GattMcsMessageId)(6))
#define GATT_MCS_MEDIA_CONTROL_POINT_WRITE_IND     ((GattMcsMessageId)(7))
#define GATT_MCS_SEARCH_CONTROL_POINT_WRITE_IND    ((GattMcsMessageId)(8))

/*!
    GattMcsTrackPositionWriteInd
    This message is sent by Media Control Service when Remote device writes
    Track Position characteristic of media Server
*/

typedef struct
{
    GattMcsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    int32 newTrackPosition;
    int32 currentPosition;
} GattMcsTrackPositionWriteInd;


/*!
    GattMcsPlaybackSpeedWriteInd
    This message is sent by Media Control Service when Remote device modifies
    the Playback Speed characteristic of media Server
*/

typedef struct
{
    GattMcsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    int8 newPlaybackSpeed;
    int8 currentPlaybackSpeed;
}GattMcsPlaybackSpeedWriteInd;

/*!
    GattMcsSeekingSpeedWriteInd
    This message is sent by Media Control Service when Remote device modifies
	the Playback Speed characteristic of media Server
*/

typedef struct
{
    GattMcsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    int8 seekSpeed;
    int8 currentPlaybackSpeed;
}GattMcsSeekingSpeedWriteInd;

/*!
    GattMcsControlPointWriteInd
    This message is sent by Media Control Service when Remote device modifies
	the Playing Order characteristic of media Server
*/

typedef struct
{
    GattMcsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    GattMcsMediaPlayingOrderType currentOrder;
    GattMcsMediaPlayingOrderType newOrder;
    GattMcsMediaSupportedPlayingOrderType supportedOrder;
} GattMcsPlayingOrderWriteInd;


/*!
    GattMcsControlPointWriteInd
    This message is sent by Media Control Service when Remote device modifies
	the Control point characteristic of media Server
*/

typedef struct
{
    GattMcsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    GattMcsMediaStateType currentState;
    GattMcsMediaControlPointType opcode;
    int32 param;
}  GattMcsControlPointWriteInd;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Media Control Service
 */
typedef struct
{
    uint16 mediaPlayerNameClientCfg:2;
    uint16 trackChangedClientCfg:2;
    uint16 trackTitleClientCfg:2;
    uint16 trackDurationClientCfg:2;
    uint16 trackPositionClientCfg:2;
    uint16 playbackSpeedClientCfg:2;
    uint16 seekingSpeedClientCfg:2;
    uint16 playingOrderClientCfg:2;
    uint16 mediaStateClientCfg:2;
    uint16 mediaControlPointClientCfg:2;
    uint16 mediaControlOpcodeSupportedCfg:2;
}GattMcsClientConfigDataType;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Media Control Service
*/

typedef struct
{
    ConnectionId                 cid;
    GattMcsClientConfigDataType  clientCfg;
}GattMcsClientData;


/*! @brief Init data

	This structure contains data for server initialization
*/
typedef struct
{
    char*                                 mediaPlayerName;
    uint16                                mediaPlayerNameLen;
    char*                                 trackTitle;
    uint16                                trackTitleLen;
    char*                                 mediaPlayerIconUrl;
    uint16                                mediaPlayerIconUrlLen;
    int32                                 trackDuration;
    int32                                 trackPosition;
    int8                                  playbackSpeed;
    int8                                  seekingSpeed;
    uint8                                 contentControlId; /* uint8 used to maintain consistency with GTBS */
    GattMcsMediaPlayingOrderType          playingOrder;
    GattMcsMediaStateType                 mediaState;
    GattMcsMediaControlPointType          mediaControlPoint;
    GattMcsMediaSupportedPlayingOrderType playingOrderSupported;
    GattMcsOpcodeTypeSupported            supportedOpcode;
}GattMcsInitData;

/*!
    @brief This API is used to initialise MCS server

    @param theAppTask Scheduler Id of the UL task.
    @param startHandle Start handle of MCS service in the  DB
    @param endHandle last handle of MCS server in the db
    @param initData  data to initialize MCS server characteristics

    @return srvc_hndl Instance handle for the service.
*/

ServiceHandle GattMcsServerInit(AppTask theAppTask,
                                uint16  startHandle,
                                uint16  endHandle,
                                GattMcsInitData* initData);

/*!
    @brief This API is used when the server application needs to add client information
    to persistent storage

    @param srvcHndl Instance handle for the service.
    @param cid to Bluetooth Connection Id ot the remote device requesting connection
    @param config server configuration of the MCS server

    @return gatt_status_t The result of the operation
*/

status_t GattMcsServerAddConfig(ServiceHandle srvcHndl,
                                ConnectionId  cid,
                                GattMcsClientConfigDataType *const config);
/*!
    @brief This API is used when the server application needs to validate if an opcode is supported

    @param srvcHndl Instance handle for the service.
    @param cid to Bluetooth Connection Id of the remote device requesting connection

    @return gatt_mbs_server_config_t Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the ConnectionId is not found, the function will return NULL.
*/

GattMcsClientConfigDataType* GattMcsServerRemoveConfig(ServiceHandle srvcHndl,
                                                       ConnectionId  cid);

/*!
    @brief This API is used when the  application needs to perform write operation on MCS
	characteristics

    @param srvcHndl Instance handle for the service.
    @param size size of the data which is being written.
    @param value, the data which is being written
    @param type, bit mask indicating which characteristic
                     is being written.

    @return bool. Returns TRUE if the api call was successful or else returns FALSE
*/

bool GattMcsServerSetMediaPlayerAttribute(ServiceHandle srvcHndl,
                                         uint16 size,
                                         uint8* value,
                                         MediaPlayerAttributeType type);

/*!
    @brief This API is used to send response to the remote client when Media Control Point write operation is performed

    @param srvcHndl Instance handle for the service.
    @param cid btConnId of the remote device
    @param opcode , operational code which is being written to Control Point

    @return bool , Outcome of the operation

    NOTE: The max size of attributes like media player name, track title, icon url
    are limited to 64 octets, can be increased as per requirement
*/

bool GattMcsServerMediaControlPointWriteResponse(ServiceHandle srvcHndl,
                                                 ConnectionId cid,
                                                 GattMcsMediaControlPointType opcode,
                                                 GattMediaControlPointAccessResultCode result);

/*!
    @brief This API is used to send response to the remote client when Media Control Point write operation is performed
    and also sets media player attribute 

    @param srvcHndl Instance handle for the service.
    @param cid btConnId of the remote device
    @param opcode , operational code which is being written to Control Point

    @return bool , Outcome of the operation

*/
bool GattMcsServerMediaAttributeResponse(ServiceHandle srvcHndl,
                                         ConnectionId cid,
                                         GattMcsMediaControlPointType opcode,
                                         GattMediaControlPointAccessResultCode result);
/*!
    @brief This API is used to write track info(track title, track duration, etc) to server when application recieves
	GATT_MCS_MEDIA_CONTROL_POINT_WRITE_IND message with opcodes: next track, prev track, first track, last track or goto "n"th track

    @param srvcHndl Instance handle for the service.
    @param trackTitleLen, length of the new track title
    @param trackTitle, new track title
    @param trackDuration, duration of new track

    @return bool, Outcome of the operation
*/

bool GattMcsServerSetCurrentTrack(ServiceHandle srvcHndl,
                                  uint16 titleLen,
                                  char*  title,
                                  int32 duration);

/*!
    @brief This API is used to reset the track poition to zero;

    @param srvcHndl Instance handle for the service.
    @param newPos absolute  track position to be set.

    @return bool, Outcome of the operation
*/

bool GattMcsServerSetAbsoluteTrackPosition(ServiceHandle srvcHndl, int32 newPos);

#endif
