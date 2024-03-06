/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_MCS_SERVER_PRIVATE_H
#define GATT_MCS_SERVER_PRIVATE_H

#include "gatt_mcs_server.h"

/*! To save space client Client configs are stored as 2 bits only, this macro
 * ensures the client config only takes 2 bits */
#define MCS_CCC_MASK(ccc) (ccc & (0x0B))
#define MCS_SERVER_NOTIFY (0x01)

#define INVALID_PLAYING_ORDER_SUPPORTED_RANGE 0xFC00
#define INVALID_CONTROL_POINT_OPCODE_SUPPORTED_RANGE 0xFFE00000

#define GATT_MCS_CLIENT_CONFIG_MASK (MCS_SERVER_NOTIFY | MCS_SERVER_NOTIFY)
#define GET_MCS_CLIENT_CONFIG(config)          (config & GATT_MCS_CLIENT_CONFIG_MASK )


/*!
 * GattMediaControlPointAcessResponseNotifyType
 *
 * */
typedef struct
{
    GattMcsMediaControlPointType opcode;
    GattMediaControlPointAccessResultCode resulCode;
}GattMediaControlPointAcessResponseNotifyType;

/*! @brief Definition of data required for association.
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
    uint8                                 contentControlId;  /* uint8 used to maintain consistency with GTBS */
    GattMcsMediaPlayingOrderType          playingOrder;
    GattMcsMediaStateType                 mediaState;
    GattMcsMediaControlPointType          mediaControlPoint;
    GattMcsMediaSupportedPlayingOrderType playingOrderSupported;
    GattMcsOpcodeTypeSupported            supportedOpcode;

    MediaPlayerAttributeType              supportedCharacteristics;
    GattMcsClientData connectedClients[MCS_MAX_CONNECTIONS];
} mcsData;


typedef uint8 GattMcsServerAttValChange;
#define GATT_MCS_SERVER_MEDIA_PLAYER_NAME_CHANGED              ((GattMcsServerAttValChange)0x01)
#define GATT_MCS_SERVER_MEDIA_PLAYER_ICON_URL_CHANGED          ((GattMcsServerAttValChange)0x02)
#define GATT_MCS_SERVER_TRACK_TITLE_CHANGED                    ((GattMcsServerAttValChange)0x04)

typedef struct
{
    GattMcsServerAttValChange          attValChanged;    /* Bitmask of attribute Value is changed or not */
}GattMcsReadLongProcData;


/*! @brief The Media Control service internal structure for the server role.

    This structure is not visible to the application as it is responsible for
    managing resource to pass to the Media Control library.
    The elements of this structure are only modified by the Media Control Service library.
    The application SHOULD NOT modify the values at any time as it could lead
    to undefined behaviour.
 */
typedef struct GMCS_T
{
    AppTaskData libTask;
    AppTask     appTask;

    /* Service handle of the instance */
    ServiceHandle srvcHandle;

    /*! Information to be provided in service characteristics. */
    mcsData data;
    /* Gatt id of the GMCS server instance*/
    CsrBtGattId gattId;

    /* Information required to process read long procedure */
    GattMcsReadLongProcData readLongProcData;
} GMCS_T;

/*!
 *     mcsServerNotifyConnectedClients
 *     Notifies all the connected clients of change in server characteristic
 *     of type 'type'.
 *  */
void mcsServerNotifyConnectedClients(GMCS_T* mcs, MediaPlayerAttributeType type);

#define McsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, MCS_SERVER_PRIM, MSG);\
    }

#endif
