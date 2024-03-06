/****************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.


FILE NAME
    vcp.h
    
DESCRIPTION
    Header file for the Volume Control Profile (VCP) library.
*/

/*!
@file    vcp.h
@brief   Header file for the GATT VCP library.

        This file provides documentation for the GATT VCP library
        API (library name: vcp).
*/

#ifndef VCP_H
#define VCP_H

#include <csrtypes.h>
#include <message.h>
#include <gatt.h>
#include <service_handle.h>
#include <library.h>

#include "gatt_vcs_client.h"
#include "gatt_vocs_client.h"
#include "gatt_aics_client.h"

/*!
    \brief Profile handle type.
*/
typedef ServiceHandle VcpProfileHandle;

/*!
    @brief VOCS handles.

*/
typedef struct
{
    uint16                   startHandle;
    uint16                   endHandle;
    GattVocsClientDeviceData handles;
}VcpVocsHandle;

/*!
    @brief AICS handles.

*/
typedef struct
{
    uint16                   startHandle;
    uint16                   endHandle;
    GattAicsClientDeviceData handles;
}VcpAicsHandle;

/*!
    @brief VCP handles.

*/
typedef struct
{
    GattVcsClientDeviceData vcsHandle;

    uint16         vocsHandleLength;
    VcpVocsHandle *vocsHandle;

    uint16         aicsHandleLength;
    VcpAicsHandle *aicsHandle;
}VcpHandles;

/*!
    @brief Initialisation parameters.

*/
typedef struct
{
    connection_id_t cid;
}VcpInitData;

/*!
    @brief Secondary services data.

*/
typedef struct
{
    uint16        uuid;
    ServiceHandle srvcHndl;
}VcpScndrSrvcData;

/*!
    \brief VCP status code type.
*/
typedef uint16 VcpStatus;

/*! { */
/*! Values for the VCP status code */
#define VCP_STATUS_SUCCESS            (0x0000u)  /*!> Request was a success*/
#define VCP_STATUS_IN_PROGRESS        (0x0001u)  /*!> Request in progress*/
#define VCP_STATUS_INVALID_PARAMETER  (0x0002u)  /*!> Invalid parameter was supplied*/
#define VCP_STATUS_DISCOVERY_ERR      (0x0003u)  /*!> Error in discovery of one of the services*/
#define VCP_STATUS_FAILED             (0x0004u)  /*!> Request has failed*/
#define VCP_STATUS_VOL_PERSISTED      (0x0005u)  /*!> It was not possible to set an initial value of volume,
                                                      because the Volume Setting Persisted bit of the the
                                                      volume Flag is set. */

/*!
    \brief IDs of messages a profile task can receive from the
           VCP library.
*/
typedef uint16 VcpMessageId;

/*! { */
/*! Values for VcpMessageId */
#define VCP_INIT_CFM                          (VCP_MESSAGE_BASE)
#define VCP_DESTROY_CFM                       (VCP_MESSAGE_BASE + 0x0001u)
#define VCP_VCS_TERMINATE_CFM                 (VCP_MESSAGE_BASE + 0x0002u)
#define VCP_VOCS_TERMINATE_CFM                (VCP_MESSAGE_BASE + 0x0003u)
#define VCP_AICS_TERMINATE_CFM                (VCP_MESSAGE_BASE + 0x0004u)
#define VCP_VOLUME_STATE_SET_NTF_CFM          (VCP_MESSAGE_BASE + 0x0005u)
#define VCP_VOLUME_FLAG_SET_NTF_CFM           (VCP_MESSAGE_BASE + 0x0006u)
#define VCP_READ_VOLUME_STATE_CCC_CFM         (VCP_MESSAGE_BASE + 0x0007u)
#define VCP_READ_VOLUME_FLAG_CCC_CFM          (VCP_MESSAGE_BASE + 0x0008u)
#define VCP_VOLUME_STATE_IND                  (VCP_MESSAGE_BASE + 0x0009u)
#define VCP_VOLUME_FLAG_IND                   (VCP_MESSAGE_BASE + 0x000Au)
#define VCP_READ_VOLUME_STATE_CFM             (VCP_MESSAGE_BASE + 0x000Bu)
#define VCP_READ_VOLUME_FLAG_CFM              (VCP_MESSAGE_BASE + 0x000Cu)
#define VCP_REL_VOL_DOWN_CFM                  (VCP_MESSAGE_BASE + 0x000Du)
#define VCP_REL_VOL_UP_CFM                    (VCP_MESSAGE_BASE + 0x000Eu)
#define VCP_UNMUTE_REL_VOL_DOWN_CFM           (VCP_MESSAGE_BASE + 0x000Fu)
#define VCP_UNMUTE_REL_VOL_UP_CFM             (VCP_MESSAGE_BASE + 0x0010u)
#define VCP_ABS_VOL_CFM                       (VCP_MESSAGE_BASE + 0x0011u)
#define VCP_UNMUTE_CFM                        (VCP_MESSAGE_BASE + 0x0012u)
#define VCP_MUTE_CFM                          (VCP_MESSAGE_BASE + 0x0013u)
#define VCP_OFFSET_STATE_SET_NTF_CFM          (VCP_MESSAGE_BASE + 0x0014u)
#define VCP_AUDIO_LOCATION_SET_NTF_CFM        (VCP_MESSAGE_BASE + 0x0015u)
#define VCP_AUDIO_OUTPUT_DESC_SET_NTF_CFM     (VCP_MESSAGE_BASE + 0x0016u)
#define VCP_READ_OFFSET_STATE_CCC_CFM         (VCP_MESSAGE_BASE + 0x0017u)
#define VCP_READ_AUDIO_LOCATION_CCC_CFM       (VCP_MESSAGE_BASE + 0x0018u)
#define VCP_READ_AUDIO_OUTPUT_DESC_CCC_CFM    (VCP_MESSAGE_BASE + 0x0019u)
#define VCP_READ_OFFSET_STATE_CFM             (VCP_MESSAGE_BASE + 0x001Au)
#define VCP_READ_AUDIO_LOCATION_CFM           (VCP_MESSAGE_BASE + 0x001Bu)
#define VCP_READ_AUDIO_OUTPUT_DESC_CFM        (VCP_MESSAGE_BASE + 0x001Cu)
#define VCP_SET_VOLUME_OFFSET_CFM             (VCP_MESSAGE_BASE + 0x001Du)
#define VCP_SET_AUDIO_LOCATION_CFM            (VCP_MESSAGE_BASE + 0x001Eu)
#define VCP_SET_AUDIO_OUTPUT_DESC_CFM         (VCP_MESSAGE_BASE + 0x001Fu)
#define VCP_OFFSET_STATE_IND                  (VCP_MESSAGE_BASE + 0x0020u)
#define VCP_AUDIO_LOCATION_IND                (VCP_MESSAGE_BASE + 0x0021u)
#define VCP_AUDIO_OUTPUT_DESC_IND             (VCP_MESSAGE_BASE + 0x0022u)
#define VCP_INPUT_STATE_SET_NTF_CFM           (VCP_MESSAGE_BASE + 0x0023u)
#define VCP_INPUT_STATUS_SET_NTF_CFM          (VCP_MESSAGE_BASE + 0x0024u)
#define VCP_AUDIO_INPUT_DESC_SET_NTF_CFM      (VCP_MESSAGE_BASE + 0x0025u)
#define VCP_READ_INPUT_STATE_CCC_CFM          (VCP_MESSAGE_BASE + 0x0026u)
#define VCP_READ_INPUT_STATUS_CCC_CFM         (VCP_MESSAGE_BASE + 0x0027u)
#define VCP_READ_AUDIO_INPUT_DESC_CCC_CFM     (VCP_MESSAGE_BASE + 0x0028u)
#define VCP_READ_INPUT_STATE_CFM              (VCP_MESSAGE_BASE + 0x0029u)
#define VCP_READ_GAIN_SET_PROPERTIES_CFM      (VCP_MESSAGE_BASE + 0x002Au)
#define VCP_READ_INPUT_TYPE_CFM               (VCP_MESSAGE_BASE + 0x002Bu)
#define VCP_READ_INPUT_STATUS_CFM             (VCP_MESSAGE_BASE + 0x002Cu)
#define VCP_READ_AUDIO_INPUT_DESC_CFM         (VCP_MESSAGE_BASE + 0x002Du)
#define VCP_SET_GAIN_SETTING_CFM              (VCP_MESSAGE_BASE + 0x002Eu)
#define VCP_AICS_UNMUTE_CFM                   (VCP_MESSAGE_BASE + 0x002Fu)
#define VCP_AICS_MUTE_CFM                     (VCP_MESSAGE_BASE + 0x0030u)
#define VCP_AICS_SET_MANUAL_GAIN_MODE_CFM     (VCP_MESSAGE_BASE + 0x0031u)
#define VCP_AICS_SET_AUTOMATIC_GAIN_MODE_CFM  (VCP_MESSAGE_BASE + 0x0032u)
#define VCP_SET_AUDIO_INPUT_DESC_CFM          (VCP_MESSAGE_BASE + 0x0033u)
#define VCP_INPUT_STATE_IND                   (VCP_MESSAGE_BASE + 0x0034u)
#define VCP_INPUT_STATUS_IND                  (VCP_MESSAGE_BASE + 0x0035u)
#define VCP_AUDIO_INPUT_DESC_IND              (VCP_MESSAGE_BASE + 0x0036u)
#define VCP_SET_INITIAL_VOL_CFM               (VCP_MESSAGE_BASE + 0x0037u)
#define VCP_MESSAGE_TOP                       (VCP_MESSAGE_BASE + 0x0038u)

/*! } */

/*!
    @brief Profile library message sent as a result of calling the VcpInitReq API.
*/
typedef struct
{
    VcpStatus        status;    /*! Status of the initialisation attempt*/
    VcpProfileHandle prflHndl;  /*! VCP profile handle*/
    uint16           vocsNum;   /*! Number of VOCS instances*/
    uint16           aicsNum;   /*! Number of AICS instances*/
} VcpInitCfm;

/*!
    @brief Profile library message sent as a result of calling the VcpSetInitialVolReq API.
*/
typedef struct
{
    VcpStatus        status;    /*! Status of the setting attempt*/
    VcpProfileHandle prflHndl;  /*! VCP profile handle*/
} VcpSetInitialVolCfm;

/*!
    @brief Profile library message sent as a result of calling the VcpDestroyReq API.

    This message will send at first with the value of status of VCP_STATUS_IN_PROGRESS.
    Another VCP_DESTROY_CFM message will be sent with the final status (success or fail),
    after VCP_VCS_TERMINATE_CFM, VCP_VOCS_TERMINATE_CFM and
    VCP_AICS_TERMINATE_CFM have been received.
*/
typedef struct
{
    VcpProfileHandle prflHndl;  /*! VCP profile handle*/
    VcpStatus        status;    /*! Status of the initialisation attempt*/
} VcpDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the VcpDestroyReq API and
           of the receiving of the GATT_VCS_CLIENT_TERMINATE_CFM message from the gatt_vcs_client
           library.
*/
typedef struct
{
    VcpProfileHandle        prflHndl;   /*! VCP profile handle*/
    VcpStatus               status;     /*! Status of the termination attempt*/
    GattVcsClientDeviceData vcsHandle;  /*! Characteristic handles of VCS*/
} VcpVcsTerminateCfm;

/*!
    @brief Profile library message sent as a result of calling the VcpDestroyReq API and
           of the receiving of all the GATT_VOCS_CLIENT_TERMINATE_CFM messages from the
           gatt_vocs_client library.
*/
typedef struct
{
    VcpProfileHandle         prflHndl;       /*! VCP profile handle*/
    VcpStatus                status;         /*! Status of the termination attempt*/
    uint16                   vocsSizeValue;  /*! Number of VOCS client instances*/
    GattVocsClientDeviceData vocsValue;      /*! Characteristic handles of the VOCS client instances*/
    uint16                   startHandle;    /*! Start handle of the VOCS instance */
    uint16                   endHandle;      /*! End handle of the VOCS instance */
    bool                     moreToCome;     /*! TRUE if more of this message will come, FALSE otherwise*/
} VcpVocsTerminateCfm;

/*!
    @brief Profile library message sent as a result of calling the VcpDestroyReq API and
           of the receiving of all the GATT_AICS_CLIENT_TERMINATE_CFM messages from the
           gatt_aics_client library.
*/
typedef struct
{
    VcpProfileHandle         prflHndl;       /*! VCP profile handle*/
    VcpStatus                status;         /*! Status of the termination attempt*/
    uint16                   aicsSizeValue;  /*! Number of AICS client instances*/
    uint16                   startHandle;    /*! Start handle of the AICS instance */
    uint16                   endHandle;      /*! End handle of the AICS instance */
    GattAicsClientDeviceData aicsValue;      /*! Characteristic handles of the AICS client instances*/
    bool                     moreToCome;     /*! TRUE if more of this message will come, FALSE otherwise*/
} VcpAicsTerminateCfm;

/*! @brief Contents of the VCP_VOLUME_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Volume State characteristic.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    gatt_status_t    status;
} VcpVolumeStateSetNtfCfm;

/*! @brief Contents of the VCP_VOLUME_FLAG_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Volume flag characteristic.
 */
typedef VcpVolumeStateSetNtfCfm VcpVolumeFlagSetNtfCfm;

/*! @brief Contents of the VCP_REL_VOL_DOWN_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the relative
    volume down operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpRelVolDownCfm;

/*! @brief Contents of the VCP_REL_VOL_UP_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the relative
    volume up operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpRelVolUpCfm;

/*! @brief Contents of the VCP_UNMUTE_REL_VOL_DOWN_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute/relative
    volume down operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpUnmuteRelVolDownCfm;

/*! @brief Contents of the VCP_UNMUTE_REL_VOL_UP_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute/relative
    volume up operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpUnmuteRelVolUpCFfm;

/*! @brief Contents of the VCP_ABS_VOL_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the absolute
    volume operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpAbsVolCfm;

/*! @brief Contents of the VCP_UNMUTE_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the unmute
    operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpUnmuteCfm;

/*! @brief Contents of the VCP_MUTE_CFM message that is sent by the library,
    as a result of writing the Volume Control point characteristic on the server using the mute
    operation.
 */
typedef VcpVolumeStateSetNtfCfm VcpMuteCfm;

/*! @brief Contents of the VCP_OFFSET_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Offset State characteristic.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
} VcpOffsetStateSetNtfCfm;

/*! @brief Contents of the VCP_AUDIO_LOCATION_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Location characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcpAudioLocationSetNtfCfm;

/*! @brief Contents of the VCP_AUDIO_OUTPUT_DESC_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Output Description characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcpAudioOutputDescSetNtfCfm;

/*! @brief Contents of the VCP_SET_VOLUME_OFFSET_CFM message that is sent by the library,
    as a result of setting volume offset on the server.
 */
typedef VcpOffsetStateSetNtfCfm VcpSetVolumeOffsetCfm;

/*! @brief Contents of the VCP_SET_AUDIO_LOCATION_CFM message that is sent by the library,
    as a result of setting the Audio Location on the server.
 */
typedef VcpOffsetStateSetNtfCfm VcpSetAudioLocationCfm;

/*! @brief Contents of the VCP_SET_AUDIO_OUTPUT_DESC_CFM message that is sent by the library,
    as a result of setting the Audio Output Description on the server.
 */
typedef VcpOffsetStateSetNtfCfm VcpSetAudioOutputDescCfm;

/*! @brief Contents of the VCP_INPUT_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Input State characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcpInputStateSetNtfCfm;

/*! @brief Contents of the VCP_INPUT_STATUS_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Input Status characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcInputStatusSetNtfCfm;

/*! @brief Contents of the VCP_AUDIO_INPUT_DESC_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Input Description characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcpAudioInputDescSetNtfCfm;

/*! @brief Contents of the VCP_SET_GAIN_SETTING_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic in order to perform a Set Gain
    Setting operation.
 */
typedef VcpOffsetStateSetNtfCfm VcpSetGainSettingCfm;

/*! @brief Contents of the VCP_AICS_UNMUTE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic in order to perform a Unmute
    operation.
 */
typedef VcpOffsetStateSetNtfCfm VcpAicsUnmuteCfm;

/*! @brief Contents of the VCP_AICS_MUTE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic in order to perform a Mute
    operation.
 */
typedef VcpOffsetStateSetNtfCfm VcpAicsMuteCfm;

/*! @brief Contents of the VCP_AICS_SET_MANUAL_GAIN_MODE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic in order to perform a Set
    Manual Gain Mode operation.
 */
typedef VcpOffsetStateSetNtfCfm VcpAicsSetManualGainModeCfm;

/*! @brief Contents of the VCP_AICS_SET_AUTOMATIC_GAIN_MODE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic in order to perform a Set
    Automatic Gain Mode operation.
 */
typedef VcpOffsetStateSetNtfCfm VcpAicsSetAutomaticGainModeCfm;

/*! @brief Contents of the VCP_SET_AUDIO_INPUT_DESC_CFM message that is sent by the library,
    as a result of writing the Audio Input Output Description characteristic.
 */
typedef VcpOffsetStateSetNtfCfm VcpSetAudioInputDescCfm;

/*! @brief Contents of the VCP_VOLUME_STATE_IND message that is sent by the library,
    as a result of a notification of the remote volume state characteristic value.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    uint8            volumeState;
    uint8            mute;
    uint8            changeCounter;
} VcpVolumeStateInd;

/*! @brief Contents of the VCP_VOLUME_FLAG_IND message that is sent by the library,
    as a result of a notification of the remote volume flag characteristic value.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    uint8            volumeFlag;
} VcpVolumeFlagInd;

/*! @brief Contents of the VCP_OFFSET_STATE_IND message that is sent by the library,
    as a result of a notification of the remote offset state characteristic value.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    int16            volumeOffset;
    uint8            changeCounter;
}VcpOffsetStateInd;

/*! @brief Contents of the VCP_AUDIO_LOCATION_IND message that is sent by the library,
    as a result of a notification of the remote audio location characteristic value.
 */
typedef struct
{
    VcpProfileHandle       prflHndl;
    ServiceHandle          srvcHndl;
    GattVocsClientAudioLoc audioLocation;
}VcpAudioLocationInd;

/*! @brief Contents of the VCP_AUDIO_OUTPUT_DESC_IND message that is sent by the library,
    as a result of a notification of the remote audio location characteristic value.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    uint16           sizeValue;
    uint8            audioOutputDesc[1];
}VcpAudioOutputDescInd;

/*! @brief Contents of the VCP_INPUT_STATE_IND message that is sent by the library,
    as a result of a notification of the remote input state.
 */
typedef struct
{
    VcpProfileHandle       prflHndl;
    ServiceHandle          srvcHndl;
    int8                   gainSetting;
    GattAicsClientMute     mute;
    GattAicsClientGainMode gainMode;
    uint8                  changeCounter;
} VcpInputStateInd;

/*! @brief Contents of the VCP_INPUT_STATUS_IND message that is sent by the library,
    as a result of a notification of the remote Input Status.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    uint8            inputStatus;
} VcpInputStatusInd;

/*! @brief Contents of the VCP_AUDIO_INPUT_DESC_IND message that is sent by the library,
    as a result of a notification of the remote Audio Input Description.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    uint16           sizeValue;
    uint8            audioInputDesc[1];
} VcpAudioInputDescInd;

/*! @brief Contents of the VCP_READ_VOLUME_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Volume State Client Configuration characteristic on the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;   /*! VCP profile handle*/
    gatt_status_t    status;     /*! Status of the reading attempt*/
    uint16           sizeValue;  /*! Value size*/
    uint8            value[1];   /*! Read value */
} VcpReadVolumeStateCccCfm;

/*! @brief Contents of the VCP_READ_VOLUME_FLAG_CCC_CFM message that is sent by the library,
    as a result of reading of the Volume Flag Client Configuration characteristic on the server.
 */
typedef VcpReadVolumeStateCccCfm VcpReadVolumeFlagCccCfm;

/*! @brief Contents of the VCP_READ_OFFSET_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Offset State Client Configuration characteristic on the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;   /*! VCP profile handle*/
    ServiceHandle    srvcHndl;   /*! Service handle of the Client instance */
    gatt_status_t    status;     /*! Status of the reading attempt*/
    uint16           sizeValue;  /*! Value size*/
    uint8            value[1];   /*! Read value */
}VcpReadOffsetStateCccCfm;

/*! @brief Contents of the VCP_READ_AUDIO_LOCATION_CCC_CFM message that is sent by the library,
    as a result of reading of the Audio Location Client Configuration characteristic on the server.
 */
typedef VcpReadOffsetStateCccCfm VcpReadAudioLocationCccCfm;

/*! @brief Contents of the VCP_READ_AUDIO_OUTPUT_DESC_CCC_CFM message that is sent by the library,
    as a result of reading of the Audio Output Description Client Configuration characteristic on the server.
 */
typedef VcpReadOffsetStateCccCfm VcpReadAudioOutputDescCccCfm;

/*! @brief Contents of the VCP_READ_INPUT_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Input State Client Configuration characteristic on the server.
 */
typedef VcpReadOffsetStateCccCfm VcpReadInputStateCccCfm;

/*! @brief Contents of the VCP_READ_INPUT_STATUS_CCC_CFM message that is sent by the library,
    as a result of reading of the Input Status Client Configuration characteristic on the server.
 */
typedef VcpReadOffsetStateCccCfm VcpReadInputStatusCccCfm;

/*! @brief Contents of the VCP_READ_AUDIO_INPUT_DESC_CCC_CFM message that is sent by the library,
    as a result of reading of the Audio Input Description Client Configuration characteristic on the server.
 */
typedef VcpReadOffsetStateCccCfm VcpReadAudioInputDescCccCfm;

/*! @brief Contents of the VCP_READ_VOLUME_STATE_CFM message that is sent by the library,
    as a result of a reading of the Volume State characteristic on the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    gatt_status_t    status;
    uint8            volumeSetting;
    uint8            mute;
    uint8            changeCounter;
} VcpReadVolumeStateCfm;

/*! @brief Contents of the VCP_READ_VOLUME_FLAG_CFM message that is sent by the library,
    as a result of a read of the Volume Flag characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    gatt_status_t    status;
    uint8            volumeFlag;
}VcpReadVolumeFlagCfm;

/*! @brief Contents of the VCP_READ_OFFSET_STATE_CFM message that is sent by the library,
    as a result of a read of the Offset State characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
    int16            volumeOffset;
    uint8            changeCounter;
}VcpReadOffsetStateCfm;

/*! @brief Contents of the VCP_READ_AUDIO_LOCATION_CFM message that is sent by the library,
    as a result of a read of the Audio Location characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle       prflHndl;
    ServiceHandle          srvcHndl;
    gatt_status_t          status;
    GattVocsClientAudioLoc audioLocation;
}VcpReadAudioLocationCfm;

/*! @brief Contents of the VCP_READ_AUDIO_OUTPUT_DESC_CFM message that is sent by the library,
    as a result of a read of the Audio Output Description characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
    uint16           sizeValue;
    uint8            value[1];
}VcpReadAudioOutputDescCfm;

/*! @brief Contents of the VCP_READ_INPUT_STATE_CFM message that is sent by the library,
    as a result of a read of the Input State characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle       prflHndl;
    ServiceHandle          srvcHndl;
    gatt_status_t          status;
    int8                   gainSetting;
    GattAicsClientMute     mute;
    GattAicsClientGainMode gainMode;
    uint8                  changeCounter;
}VcpReadInputStateCfm;

/*! @brief Contents of the VCP_READ_GAIN_SET_PROPERTIES_CFM message that is sent by the library,
    as a result of a read of the Gain Setting Properties characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
    uint8            gainSettingUnits;
    int8             gainSettingMin;
    int8             gainSettingMax;
}VcpReadGainSetPropertiesCfm;

/*! @brief Contents of the VCP_READ_INPUT_TYPE_CFM message that is sent by the library,
    as a result of a read of the Input Type characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle        prflHndl;
    ServiceHandle           srvcHndl;
    gatt_status_t           status;
    GattAicsClientInputType inputType;
}VcpReadInputTypeCfm;

/*! @brief Contents of the VCP_READ_INPUT_STATUS_CFM message that is sent by the library,
    as a result of a read of the Input Status characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
    uint8            inputStatus;
}VcpReadInputStatusCfm;

/*! @brief Contents of the VCP_READ_AUDIO_INPUT_DESC_CFM message that is sent by the library,
    as a result of a read of the Audio Input Description characteristic of the server.
 */
typedef struct
{
    VcpProfileHandle prflHndl;
    ServiceHandle    srvcHndl;
    gatt_status_t    status;
    uint16           sizeValue;
    uint8            value[1];
}VcpReadAudioInputDescCfm;

/*!
    @brief Initialises the Gatt VCP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt VCP library.

    @param appTask           The Task that will receive the messages sent from this immediate alert profile library
    @param clientInitParams Initialisation parameters
    @param deviceData        VCS/VOCS/AICS handles
    @param includedServices  TRUE if the discovery of the included services is requested, FALSE otherwise

    NOTE: A VCP_INIT_CFM with VcpStatus code equal to VCP_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed VCP_INIT_CFM will be received with a VcpStatus
          that indicates the result of the initialisation.
*/
void VcpInitReq(Task appTask,
                VcpInitData *clientInitParams,
                VcpHandles *deviceData,
                bool includedServices);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the VcpInitReq API.

    @param profileHandle The Profile handle.

    NOTE: A VCP_INIT_CFM with VcpStatus code equal to VCP_STATUS_IN_PROGRESS will be received as indication
          that the profile library destroy started. Once completed VCP_DESTROY_CFM will be received with a
          VcpStatus that indicates the result of the destroy.
*/
void VcpDestroyReq(VcpProfileHandle profileHandle);

/*!
    @brief Get the service handles of all the VOCS client instances initialised.

    @param profileHandle The Profile handle.
    @param vocsNum       Pointer to the uint16 variable provided by the application in which the library
                          will save the number of VOCS Client instances.

    @return Pointer to a ServiceHandle structure containing all the service handles of the VOCS Client
            instances initialised.

    NOTE: It's responsibility of the application to free the memory of the returned pointer.
          If there are no VOCS Client instances initialised, this function will return NULL and the
          vocsNum will be 0;
*/
ServiceHandle *VcpGetVocsServiceHandlesRequest(VcpProfileHandle profileHandle,
                                                  uint16 *vocsNum);

/*!
    @brief Get the service handles of all the AICS client instances initialised.

    @param profileHandle The Profile handle.
    @param aicsNum       Pointer to the uint16 variable provided by the application in which the library
                          will save the number of AICS Client instances.

    @return Pointer to a ServiceHandle structure containing all the service handles of the AICS Client
            instances initialised.

    NOTE: It's responsibility of the application to free the memory of the returned pointer.
          If there are no AICS Client instances initialised, this function will return NULL and the
          vocsNum will be 0;
*/
ServiceHandle *VcpGetAicsServiceHandlesRequest(VcpProfileHandle profileHandle,
                                                  uint16 *aicsNum);

/*!
    @brief This API is used to set the initial volume.

    @param profileHandle  The Profile handle.
    @param initialVol     Value of the volume to set

    NOTE: A VcpSetInitialVolCfm message will be sent to the registered application Task.
          If the Volume Setting Persisted bit of the Volume Flag characteristic is set,
          it will not possible to set the selected value of volume and a VCP_READ_VOLUME_STATE_CFM
          message will be send with the actual value of the Volume Setting of the remote device.

*/
void VcpSetInitialVolReq(VcpProfileHandle profileHandle, uint8 initialVol);

/*!
    @brief This API is used to write the client characteristic configuration of the Volume State
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: VCP_VOLUME_STATE_SET_NTF_CFM message will be sent to the registered application Task.

*/
void VcpVolumeStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                              bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Volume Flags
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: VCP_VOLUME_FLAG_SET_NTF_CFM message will be sent to the registered application Task.

*/
void VcpVolumeFlagRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                             bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Volume State
           characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_STATE_CCC_CFM message will be sent to the registered application Task.

*/
void VcpReadVolumeStateCccRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Volume Flag characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_FLAG_CCC_CFM message will be sent to the registered application Task.

*/
void VcpReadVolumeFlagCccRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Volume State characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_STATE_CFM message will be sent to the registered application Task.

*/
void VcpVolumeStateRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Volume Flag characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_READ_VOLUME_FLAG_CFM message will be sent to the registered application Task.

*/
void VcpReadVolumeFlagRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Relative Volume Down operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_REL_VOL_DOWN_CFM message will be sent to the registered application Task.

*/
void VcpRelativeVolumeDownRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Relative Volume Up operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_REL_VOL_UP_CFM message will be sent to the registered application Task.

*/
void VcpRelativeVolumeUpRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute/Relative Volume Down operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_UNMUTE_REL_VOL_DOWN_CFM message will be sent to the registered
            application Task.

*/
void VcpUnmuteRelativeVolumeDownRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute/Relative Volume Up operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_UNMUTE_REL_VOL_UP_CFM message will be sent to the registered application Task.

*/
void VcpUnmuteRelativeVolumeUpRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Absolute Volume operation.

    @param profileHandle  The Profile handle.
    @param volumeSetting  Value of volume to set

    NOTE: A VCP_ABS_VOL_CFM message will be sent to the registered application Task.

*/
void VcpAbsoluteVolumeRequest(VcpProfileHandle profileHandle, uint8 volumeSetting);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Unmute operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_UNMUTE_CFM message will be sent to the registered application Task.

*/
void VcpUnmuteRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Volume Control point characteristic in order to execute
           the Mute operation.

    @param profileHandle  The Profile handle.

    NOTE: A VCP_MUTE_CFM message will be sent to the registered application Task.

*/
void VcpMuteRequest(VcpProfileHandle profileHandle);

/*!
    @brief This API is used to write the client characteristic configuration of the Offset State
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param vocsClntHndl        Service handle of the VOCS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_OFFSET_STATE_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpOffsetStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                              ServiceHandle vocsClntHndl,
                                              bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Audio Location
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param vocsClntHndl       Service handle of the VOCS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_AUDIO_LOCATION_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpAudioLocationRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                ServiceHandle vocsClntHndl,
                                                bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Audio Ouput Desc
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param vocsClntSndl        Service handle of the VOCS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_AUDIO_OUTPUT_DESC_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpAudioOutputDescRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                  ServiceHandle vocsClntHndl,
                                                  bool notificationsEnable);
/*!
    @brief This API is used to read the Client Configuration Characteristic of the Offset State
           characteristic.

    @param profileHandle       The Profile handle.
    @param vocsClntHndl       Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_OFFSET_STATE_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.

*/
void VcpReadOffsetStateCccRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle vocsClntHndl);
/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Location
           characteristic.

    @param profileHandle       The Profile handle.
    @param vocsClntHndl       Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_LOCATION_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.

*/
void VcpReadAudioLocationCccRequest(VcpProfileHandle profileHandle,
                                    ServiceHandle vocsClntHndl);
/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Output
           Description characteristic.

    @param profileHandle       The Profile handle.
    @param vocsClntHndl       Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_OUTPUT_DESC_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpReadAudioOutputDescCccRequest(VcpProfileHandle profileHandle,
                                      ServiceHandle vocsClntHndl);

/*!
    @brief This API is used to read the Offset State characteristic.

    @param profileHandle  The Profile handle.
    @param vocsClntHndl  Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_OFFSET_STATE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpReadOffsetStateRequest(VcpProfileHandle profileHandle,
                               ServiceHandle vocsClntHndl);

/*!
    @brief This API is used to read the Audio Location characteristic.

    @param profileHandle The Profile handle.
    @param vocsClntHndl  Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_LOCATION_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpReadAudioLocationRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle vocsClntHndl);

/*!
    @brief This API is used to read the Audio Output Description characteristic.

    @param profileHandle  The Profile handle.
    @param vocsClntHndl  Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_OUTPUT_DESC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.
*/
void VcpReadAudioOutputDescRequest(VcpProfileHandle profileHandle,
                                   ServiceHandle vocsClntHndl);
/*!
    @brief This API is used to write the Volume Offset Control point characteristic in order to execute
           the Set Volume Offset operation.

    @param profileHandle The Profile handle.
    @param volumeOffset  Value of Volume Offset to set.
    @param vocsClntHndl  Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_SET_VOLUME_OFFSET_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.

*/
void VcpSetVolumeOffsetRequest(VcpProfileHandle profileHandle,
                               int16 volumeOffset,
                               ServiceHandle vocsClntHndl);

/*!
    @brief This API is used to write the Audio Location characteristic.

    @param profileHandle  The Profile handle.
    @param audioLocVal   Value of Audio Location to set.
    @param vocsClntHndl  Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_SET_AUDIO_LOCATION_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.

*/
void VcpSetAudioLocationRequest(VcpProfileHandle profileHandle,
                                GattVocsClientAudioLoc audioLocVal,
                                ServiceHandle vocsClntHndl);

/*!
    @brief This API is used to write the Audio Output Description characteristic.

    @param profileHandle      The Profile handle.
    @param valueSize          Size of the value to set
    @param audioOutputDescVal Value of Audio Output Description to set.
    @param vocsClntHndl       Service handle of the VOCS instance to which apply the operation.

    NOTE: A VCP_SET_AUDIO_OUTPUT_DESC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific VOCS instance, it is necessary to specify its
          service handle in vocsClntHndl.
          To perform this operation to all the VOCS instances, vocsClntHndl has to be zero.

*/
void VcpSetAudioOutputDescRequest(VcpProfileHandle profileHandle,
                                  uint16           valueSize,
                                  uint8            *audioOutputDescVal,
                                  ServiceHandle    vocsClntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of the Input State
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param aicsClntHndl        Service handle of the AICS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_INPUT_STATE_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpInputStateRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                             ServiceHandle aicsClntHndl,
                                             bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Input Status
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param aicsClntHndl        Service handle of the AICS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_INPUT_STATUS_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpInputStatusRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                              ServiceHandle aicsClntHndl,
                                              bool notificationsEnable);
/*!
    @brief This API is used to write the client characteristic configuration of the Audio Input Desc
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param aicsClntHhndl       Service handle of the AICS instance to which apply the operation.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A VCP_AUDIO_INPUT_DESC_SET_NTF_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpAudioInputDescRegisterForNotificationReq(VcpProfileHandle profileHandle,
                                                 ServiceHandle aicsClntHndl,
                                                 bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Input State
           characteristic.

    @param profileHandle       The Profile handle.
    @param aicsClntHndl       Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_INPUT_STATE_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadInputStateCccRequest(VcpProfileHandle profileHhandle,
                                 ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Input Status
           characteristic.

    @param profileHandle       The Profile handle.
    @param aicsClntHhndl       Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_INPUT_STATUS_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadInputStatusCccRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Input
           Description characteristic.

    @param profileHandle       The Profile handle.
    @param aicsClntHndl       Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_INPUT_DESC_CCC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.

*/
void VcpReadAudioInputDescCccRequest(VcpProfileHandle profileHandle,
                                     ServiceHandle aicsClnHndl);


/*!
    @brief This API is used to read the Input State characteristic.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_INPUT_STATE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadInputStateRequest(VcpProfileHandle profileHandle,
                              ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Gain Setting Properties characteristic.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_GAIN_SET_PROPERTIES_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadGainSetProperRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Input Type characteristic.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_INPUT_TYPE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadInputTypeRequest(VcpProfileHandle profileHandle,
                             ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Input Status characteristic.

    @param profileHandle The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_INPUT_STATUS_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadInputStatusRequest(VcpProfileHandle profileHandle,
                               ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to read the Audio Input Description characteristic.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_READ_AUDIO_INPUT_DESC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpReadAudioInputDescRequest(VcpProfileHandle profileHandle,
                                  ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Gain Setting operation.

    @param profileHandle  The Profile handle.
    @param gainSetting    Value of Gain Setting to set.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_SET_GAIN_SETTING_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpSetGainSettingRequest(VcpProfileHandle profileHandle,
                              int16 gainSetting,
                              ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Unmute operation.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_AICS_UNMUTE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.

*/
void VcpAicsSetUnmuteRequest(VcpProfileHandle profileHandle,
                             ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Mute operation.

    @param profileHandle The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_AICS_MUTE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.
*/
void VcpAicsSetMuteRequest(VcpProfileHandle profileHandle,
                           ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Manual Gain Mode operation.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_AICS_SET_MANUAL_GAIN_MODE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.

*/
void VcpSetManualGainModeRequest(VcpProfileHandle profileHandle,
                                 ServiceHandle aicsClntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Automatic Gain Mode operation.

    @param profileHandle  The Profile handle.
    @param aicsClntHndl  Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_AICS_SET_AUTOMATIC_GAIN_MODE_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.

*/
void VcpSetAutomaticGainModeRequest(VcpProfileHandle profileHandle,
                                    ServiceHandle aicsClntHndl);


/*!
    @brief This API is used to write the Audio Input Description characteristic.

    @param profileHandle     The Profile handle.
    @param valueSize         Size of the value to set
    @param audioInputDescVal Value of Audio Input Description to set.
    @param aicsClntHndl      Service handle of the AICS instance to which apply the operation.

    NOTE: A VCP_SET_AUDIO_INPUT_DESC_CFM message will be sent to the registered application Task.
          To perform this operation to a specific AICS instance, it is necessary to specify its
          service handle in aicsClntHndl.
          To perform this operation to all the AICS instances, aicsClntHndl has to be zero.

*/
void VcpSetAudioInputDescRequest(VcpProfileHandle  profileHandle,
                                 uint16            valueSize,
                                 uint8            *audioInputDescVal,
                                 ServiceHandle  aicsClntHndl);

#endif /* VCP_H */

