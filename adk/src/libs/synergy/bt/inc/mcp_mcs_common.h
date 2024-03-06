#ifndef MCP_MCS_COMMON_H_
#define MCP_MCS_COMMON_H_
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_types.h"

#include "service_handle.h"
#ifdef __cplusplus
extern "C" {
#endif

/*!
    \brief Profile handle type.
*/
typedef ServiceHandle McpProfileHandle;

/*!
    @brief MCS Service Info (Service handle and start,end handles mapping).

*/
typedef struct
{
    ServiceHandle   srvcHndl;
    uint16          startHandle;
    uint16          endHandle;
} McsServiceInfo;

typedef uint32       McpConnectionId;

/* Characteristics of MCS service */
typedef uint8                                    MediaPlayerAttribute;                    /* Combination of following values to be used */
#define MCS_MEDIA_PLAYER_NAME                    ((MediaPlayerAttribute) 0x01)
#define MCS_MEDIA_PLAYER_ICON_OBJ_ID             ((MediaPlayerAttribute) 0x02)
#define MCS_MEDIA_PLAYER_ICON_URL                ((MediaPlayerAttribute) 0x03)
#define MCS_TRACK_CHANGED                        ((MediaPlayerAttribute) 0x04)
#define MCS_TRACK_TITLE                          ((MediaPlayerAttribute) 0x05)
#define MCS_TRACK_DURATION                       ((MediaPlayerAttribute) 0x06)
#define MCS_TRACK_POSITION                       ((MediaPlayerAttribute) 0x07)
#define MCS_PLAYBACK_SPEED                       ((MediaPlayerAttribute) 0x08)
#define MCS_SEEKING_SPEED                        ((MediaPlayerAttribute) 0x09)
#define MCS_CURRENT_TRACK_SEGMENTS_OBJ_ID        ((MediaPlayerAttribute) 0x0A)
#define MCS_CURRENT_TRACK_OBJ_ID                 ((MediaPlayerAttribute) 0x0B)
#define MCS_NEXT_TRACK_OBJ_ID                    ((MediaPlayerAttribute) 0x0C)
#define MCS_CURRENT_GROUP_OBJ_ID                 ((MediaPlayerAttribute) 0x0D)
#define MCS_PARENT_GROUP_OBJ_ID                  ((MediaPlayerAttribute) 0x0E)
#define MCS_PLAYING_ORDER                        ((MediaPlayerAttribute) 0x0F)
#define MCS_PLAYING_ORDER_SUPP                   ((MediaPlayerAttribute) 0x10)
#define MCS_MEDIA_STATE                          ((MediaPlayerAttribute) 0x11)
#define MCS_MEDIA_CONTROL_POINT                  ((MediaPlayerAttribute) 0x12)
#define MCS_MEDIA_CONTROL_POINT_OP_SUPP          ((MediaPlayerAttribute) 0x13)
#define MCS_SEARCH_RESULTS_OBJ_ID                ((MediaPlayerAttribute) 0x14)
#define MCS_SEARCH_CONTROL_POINT                 ((MediaPlayerAttribute) 0x15)
#define MCS_CONTENT_CONTROL_ID                   ((MediaPlayerAttribute) 0x16)

/* Bitmask of Characteristics of MCS service */
typedef uint32                                    MediaPlayerAttributeMask;                    /* Combination of following values to be used */
#define MCS_MEDIA_PLAYER_NAME_POS0                ((MediaPlayerAttributeMask) 0x000001)
#define MCS_TRACK_CHANGED_POS1                    ((MediaPlayerAttributeMask) 0x000002)
#define MCS_TRACK_TITLE_POS2                      ((MediaPlayerAttributeMask) 0x000004)
#define MCS_TRACK_DURATION_POS3                   ((MediaPlayerAttributeMask) 0x000008)
#define MCS_TRACK_POSITION_POS4                   ((MediaPlayerAttributeMask) 0x000010)
#define MCS_PLAYBACK_SPEED_POS5                   ((MediaPlayerAttributeMask) 0x000020)
#define MCS_SEEKING_SPEED_POS6                    ((MediaPlayerAttributeMask) 0x000040)
#define MCS_CURRENT_TRACK_OBJ_ID_POS7             ((MediaPlayerAttributeMask) 0x000080)
#define MCS_NEXT_TRACK_OBJ_ID_POS8                ((MediaPlayerAttributeMask) 0x000100)
#define MCS_CURRENT_GROUP_OBJ_ID_POS9             ((MediaPlayerAttributeMask) 0x000200)
#define MCS_PARENT_GROUP_OBJ_ID_POS10             ((MediaPlayerAttributeMask) 0x000400)
#define MCS_PLAYING_ORDER_POS11                   ((MediaPlayerAttributeMask) 0x000800)
#define MCS_MEDIA_STATE_POS12                     ((MediaPlayerAttributeMask) 0x001000)
#define MCS_MEDIA_CONTROL_POINT_POS13             ((MediaPlayerAttributeMask) 0x002000)
#define MCS_MEDIA_CONTROL_POINT_OP_SUPP_POS14     ((MediaPlayerAttributeMask) 0x004000)
#define MCS_SEARCH_RESULTS_OBJ_ID_POS15           ((MediaPlayerAttributeMask) 0x008000)
#define MCS_SEARCH_CONTROL_POINT_POS16            ((MediaPlayerAttributeMask) 0x010000)

/*! @brief Opcodes for media control point operations.
 */

typedef uint8                                           GattMcsOpcode;

#define GATT_MCS_CLIENT_PLAY                            ((GattMcsOpcode) 0x01)
#define GATT_MCS_CLIENT_PAUSE                           ((GattMcsOpcode) 0x02)
#define GATT_MCS_CLIENT_FAST_REWIND                     ((GattMcsOpcode) 0x03)
#define GATT_MCS_CLIENT_FAST_FORWARD                    ((GattMcsOpcode) 0x04)
#define GATT_MCS_CLIENT_STOP                            ((GattMcsOpcode) 0x05)
#define GATT_MCS_CLIENT_MOVE_RELATIVE                   ((GattMcsOpcode) 0x10)
#define GATT_MCS_CLIENT_PREVIOUS_SEGMENT                ((GattMcsOpcode) 0x20)
#define GATT_MCS_CLIENT_NEXT_SEGMENT                    ((GattMcsOpcode) 0x21)
#define GATT_MCS_CLIENT_FIRST_SEGMENT                   ((GattMcsOpcode) 0x22)
#define GATT_MCS_CLIENT_LAST_SEGMENT                    ((GattMcsOpcode) 0x23)
#define GATT_MCS_CLIENT_GOTO_SEGMENT                    ((GattMcsOpcode) 0x24)
#define GATT_MCS_CLIENT_PREVIOUS_TRACK                  ((GattMcsOpcode) 0x30)
#define GATT_MCS_CLIENT_NEXT_TRACK                      ((GattMcsOpcode) 0x31)
#define GATT_MCS_CLIENT_FIRST_TRACK                     ((GattMcsOpcode) 0x32)
#define GATT_MCS_CLIENT_LAST_TRACK                      ((GattMcsOpcode) 0x33)
#define GATT_MCS_CLIENT_GOTO_TRACK                      ((GattMcsOpcode) 0x34)
#define GATT_MCS_CLIENT_PREVIOUS_GROUP                  ((GattMcsOpcode) 0x40)
#define GATT_MCS_CLIENT_NEXT_GROUP                      ((GattMcsOpcode) 0x41)
#define GATT_MCS_CLIENT_FIRST_GROUP                     ((GattMcsOpcode) 0x42)
#define GATT_MCS_CLIENT_LAST_GROUP                      ((GattMcsOpcode) 0x43)
#define GATT_MCS_CLIENT_GOTO_GROUP                      ((GattMcsOpcode) 0x44)

typedef uint8                                           GattMcsOpResult;

#define GATT_MCS_OP_RESULT_SUCCESS                      ((GattMcsOpResult) 0x01)
#define GATT_MCS_OP_RESULT_NOT_SUPPORTED                ((GattMcsOpResult) 0x02)
#define GATT_MCS_OP_RESULT_MEDIA_PLAYER_INACTIVE        ((GattMcsOpResult) 0x03)
#define GATT_MCS_OP_RESULT_COMMAND_INCOMPLETE           ((GattMcsOpResult) 0x04)
#define GATT_MCS_OP_RESULT_CHARAC_NOT_SUPPORTED         ((GattMcsOpResult) 0x05) /* Local defined value, used when Media Control Point
                                                                                     is not supported by remote device */

#ifdef __cplusplus
}
#endif

#endif
