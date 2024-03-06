/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_aics_client.h

*/


/*!
@file    gatt_aics_client.h
@brief   Header file for the GATT AICS (Audio Input Control Service) Client library.

        This file provides documentation for the GATT AICS Client library
        API (library name: gatt_aics_client).
*/

#ifndef GATT_AICS_CLIENT_H
#define GATT_AICS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"

#define GATT_AICS_CLIENT_INPUT_STATUS_INVALID (0xFF)

#ifdef CSR_TARGET_PRODUCT_VM
#include <library.h>
#else
#define GATT_AICS_CLIENT_MESSAGE_BASE 0x00
#endif

/*!
    @brief Handles of the AICS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 inputStateHandle;
    uint16 gainSettingPropertiesHandle;
    uint16 inputTypeHandle;
    uint16 inputStatusHandle;
    uint16 audioInputControlPointHandle;
    uint16 audioInputDescriptionHandle;

    uint16 inputStateCccHandle;
    uint16 inputStatusCccHandle;
    uint16 audioInputDescriptionCccHandle;

    uint8 audioInputDescProperties;
} GattAicsClientDeviceData;

/*!
    \brief IDs of messages a profile task can receive from the
           GATT AICS Client library.
*/
typedef uint16 GattAicsClientMessageId;

/*! { */
/*! Values for GattAicsClientMessageId */
#define GATT_AICS_CLIENT_INIT_CFM                       (GATT_AICS_CLIENT_MESSAGE_BASE)
#define GATT_AICS_CLIENT_TERMINATE_CFM                  (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM        (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0002u)
#define GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM       (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0003u)
#define GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM   (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM       (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM      (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0006u)
#define GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM  (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0007u)
#define GATT_AICS_CLIENT_READ_INPUT_STATE_CFM           (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0008u)
#define GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM   (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0009u)
#define GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM            (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Au)
#define GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM          (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Bu)
#define GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM      (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Cu)
#define GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM           (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Du)
#define GATT_AICS_CLIENT_UNMUTE_CFM                     (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Eu)
#define GATT_AICS_CLIENT_MUTE_CFM                       (GATT_AICS_CLIENT_MESSAGE_BASE + 0x000Fu)
#define GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM       (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0010u)
#define GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM    (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0011u)
#define GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM       (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0012u)
#define GATT_AICS_CLIENT_INPUT_STATE_IND                (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0013u)
#define GATT_AICS_CLIENT_INPUT_STATUS_IND               (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0014u)
#define GATT_AICS_CLIENT_AUDIO_INPUT_DESC_IND           (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0015u)
#define GATT_AICS_CLIENT_MESSAGE_TOP                    (GATT_AICS_CLIENT_MESSAGE_BASE + 0x0016u)

/*! } */

/*!
    \brief AICS Client status code type.
*/
typedef uint16 GattAicsClientStatus;

/*! { */
/*! Values for the AICS status code */
#define GATT_AICS_CLIENT_STATUS_SUCCESS                   (0x0000u)  /*!> Request was a success*/
#define GATT_AICS_CLIENT_STATUS_DISCOVERY_ERR             (0x0001u)  /*!> Error in discovery of Characteristics*/
#define GATT_AICS_CLIENT_STATUS_FAILED                    (0x0002u)  /*!> Request has failed*/
#define GATT_AICS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0003u)  /*!> Insufficient Resources to complete
                                                                          the request. */
/*! } */

/*! Values of the Mute field of the Inpute State characteristic */
typedef uint16 GattAicsClientMute;

#define GATT_AICS_CLIENT_NOT_MUTED    (0x0000u)
#define GATT_AICS_CLIENT_MUTED        (0x0001u)
#define GATT_AICS_CLIENT_DISABLED     (0x0002u)
#define GATT_AICS_CLIENT_MUTE_INVALID (0x0003u)

/*! Values of the Gain Mode field of the Inpute State characteristic */
typedef uint16 GattAicsClientGainMode;

#define GATT_AICS_CLIENT_GAIN_MODE_MANUAL_ONLY       (0x0000u)
#define GATT_AICS_CLIENT_GAIN_MODE_AUTOMATIC_ONLY    (0x0001u)
#define GATT_AICS_CLIENT_GAIN_MODE_MANUAL            (0x0002u)
#define GATT_AICS_CLIENT_GAIN_MODE_AUTOMATIC         (0x0003u)

/*! Values of Input Type characteristic */
typedef uint8 GattAicsClientInputType;

#define GATT_AICS_CLIENT_INP_TPY_UNSPECIFIED     (0x00u)
#define GATT_AICS_CLIENT_INP_TPY_BT              (0x01u)
#define GATT_AICS_CLIENT_INP_TPY_MIC             (0x02u)
#define GATT_AICS_CLIENT_INP_TPY_ANALOG          (0x03u)
#define GATT_AICS_CLIENT_INP_TPY_DIGITAL         (0x04u)
#define GATT_AICS_CLIENT_INP_TPY_RADIO           (0x05u)
#define GATT_AICS_CLIENT_INP_TPY_STREAMING       (0x06u)
#define GATT_AICS_CLIENT_INP_TPY_INVALID         (0xffu)

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
     connection_id_t cid;      /*! Connection ID. */
     uint16 startHandle;       /*! Start handle. */
     uint16 endHandle;         /*! End handle. */
} GattAicsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattAicsClientInitReq API.
*/
typedef struct
{
    GattAicsClientMessageId   id;
    connection_id_t           cid;        /*! Connection ID. */
    ServiceHandle          srvcHndl;   /*! Reference handle for the instance */
    GattAicsClientStatus      status;     /*! Status of the initialisation attempt */
} GattAicsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattAicsClientTerminateReq API.
*/
typedef struct
{
    GattAicsClientMessageId    id;
    ServiceHandle           srvcHndl;    /*! Reference handle for the instance */
    GattAicsClientStatus       status;      /*! Status of the termination attempt */
    uint16                     startHandle; /*! Start handle of the AICS instance */
    uint16                     endHandle;   /*! End handle of the AICS instance */
    GattAicsClientDeviceData   deviceData;  /*! Device data: handles used for the peer device. */
} GattAicsClientTerminateCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Input State characteristic.
 */
typedef struct
{
    GattAicsClientMessageId    id;
    ServiceHandle           srvcHndl;   /*! Reference handle for the instance */
    status_t                   status;     /*! Status of the setting attempt */
} GattAicsClientInputStateSetNtfCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Input Status characteristic.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientInputStatusSetNtfCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Input Description characteristic.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientAudioInputDescSetNtfCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic on the server using the set
    gain setting opearation.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientSetGainSettingCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_UNMUTE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic on the server using the unmute
    operation.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientUnmuteCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_MUTE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic on the server using the Mute
    operation.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientMuteCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic on the server using the Set
    Manual Gain Mode operation.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientSetManualGainModeCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM message that is sent by the library,
    as a result of writing the Audio Input Description characteristic on the server.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientSetAudioInputDescCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM message that is sent by the library,
    as a result of writing the Audio Input Control point characteristic on the server using the Set
    Automatic Gain Mode operation.
 */
typedef GattAicsClientInputStateSetNtfCfm GattAicsClientSetAutomaticGainModeCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_INPUT_STATE_IND message that is sent by the library,
    as a result of a notification of the remote input state.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle         srvcHndl;       /*! Reference handle for the instance */
    int8                     gainSetting;    /*! Value of the Gain Setting field */
    GattAicsClientMute       mute;           /*! Value of the Mute field */
    GattAicsClientGainMode   gainMode;       /*! Value of the gain mode field */
    uint8                    changeCounter;  /*! Value of the change counter field */
} GattAicsClientInputStateInd;

/*! @brief Contents of the GATT_AICS_CLIENT_INPUT_STATUS_IND message that is sent by the library,
    as a result of a notification of the remote Input Status.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle  srvcHndl;                /*! Reference handle for the instance */
    uint8             inputStatus;             /*! Value of the Input Status */
} GattAicsClientInputStatusInd;

/*! @brief Contents of the GATT_AICS_CLIENT_AUDIO_INPUT_DESC_IND message that is sent by the library,
    as a result of a notification of the remote Audio Input Description.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle  srvcHndl;               /*! Reference handle for the instance */
    uint16            sizeValue;              /*! Size of the Audio Input Description value */
    uint8            *audioInputDesc;         /*! Value of the Input Status */
} GattAicsClientAudioInputDescInd;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Input State Client Configuration characteristic on the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    status_t          status;     /*! Status of the reading attempt */
    uint16            sizeValue;  /*! Size of the ccc value */
    uint8            *value;      /*! CCC value */
}GattAicsClientReadInputStateCccCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM message that is sent by the library,
    as a result of reading of the Input Status Client Configuration characteristic on the server.
 */
typedef GattAicsClientReadInputStateCccCfm GattAicsClientReadInputStatusCccCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM message that is sent by the library,
    as a result of reading of the Audio Input Description Client Configuration characteristic on the server.
 */
typedef GattAicsClientReadInputStateCccCfm  GattAicsClientReadAudioInputDescCccCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_INPUT_STATE_CFM message that is sent by the library,
    as a result of a reading of the Input State characteristic on the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle         srvcHndl;       /*! Reference handle for the instance */
    status_t                 status;         /*! Status of the reading attempt */
    int8                     gainSetting;    /*! Value of the Gain Setting field */
    GattAicsClientMute       mute;           /*! Value of the Mute field */
    GattAicsClientGainMode   gainMode;       /*! Value of the gain mode field */
    uint8                    changeCounter;  /*! Value of the change counter field */
} GattAicsClientReadInputStateCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM message that is sent by the library,
    as a result of a read of the Gain Setting Properties characteristic of the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle         srvcHndl;          /*! Reference handle for the instance */
    status_t                 status;            /*! Status of the reading attempt */
    uint8                    gainSettingUnits;  /*! Value of the Gain Setting Units field */
    int8                     gainSettingMin;    /*! Value of the Gain Setting Minimum field */
    int8                     gainSettingMax;    /*! Value of the Gain Setting Maximum field */
}GattAicsClientReadGainSetPropertiesCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM message that is sent by the library,
    as a result of a read of the Input Type characteristic of the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle         srvcHndl;   /*! Reference handle for the instance */
    status_t                 status;     /*! Status of the reading attempt */
    GattAicsClientInputType  inputType;  /*! Value of the Input Type */
}GattAicsClientReadInputTypeCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM message that is sent by the library,
    as a result of a read of the Input Status characteristic of the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle         srvcHndl;     /*! Reference handle for the instance */
    status_t                 status;       /*! Status of the reading attempt */
    uint8                    inputStatus;  /*! Value of the Input Status */
}GattAicsClientReadInputStatusCfm;

/*! @brief Contents of the GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM message that is sent by the library,
    as a result of a read of the Audio Input Description characteristic of the server.
 */
typedef struct
{
    GattAicsClientMessageId  id;
    ServiceHandle  srvcHndl;        /*! Reference handle for the instance */
    status_t          status;          /*! Status of the reading attempt */
    uint16            sizeValue;       /*! Size of the Audio Input Description value */
    uint8            *audioInputDesc;  /*! Value of the Input Status */
}GattAicsClientReadAudioInputDescCfm;

/*!
    @brief GATT AICS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    \param initData    Configuration data for client initialisation.
    \param deviceData  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_AICS_CLIENT_INIT_CFM will be received with a GattAicsClientStatus status code.
*/
void GattAicsClientInitReq(AppTask theAppTask,
                           const GattAicsClientInitData   *initData,
                           const GattAicsClientDeviceData *deviceData);

/*!
    \brief GATT AICS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    \param clntHndl  The service handle for this GATT Client Service.

    NOTE: GATT_AICS_CLIENT_TERMINATE_CFM will be received with a GattAicsClientStatus status code.
*/
void GattAicsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of the Input State
           characteristic on a remote device, to enable notifications with the server.
           An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server,
                               FALSE to disable them.

    NOTE: A GATT_AICS_CLIENT_INPUT_STATE_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattAicsClientInputStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                        bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Input Status
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_AICS_CLIENT_INPUT_STATUS_SET_NTF_CFM message will be sent to the registered
            application Task.

*/
void GattAicsClientInputStatusRegisterForNotificationReq(ServiceHandle clntHndl,
                                                         bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Audio Input Description
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM message will be sent to the registered
            application Task.

*/
void GattAicsClientAudioInputDescRegisterForNotificationReq(ServiceHandle clntHndl,
                                                            bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Input State
           characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_INPUT_STATE_CCC_CFM message will be sent to the registered application
            Task.

*/
void GattAicsClientReadInputStateCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Input Status
            characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_INPUT_STATUS_CCC_CFM message will be sent to the registered
            application Task.

*/
void GattAicsClientReadInputStatusCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Input
           Description characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM message will be sent to the registered
            application Task.

*/
void GattAicsClientReadAudiInputDescCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Input State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_INPUT_STATE_CFM message will be sent to the registered application Task.

*/
void GattAicsClientReadInputStateRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Gain Setting Properties characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientReadGainSetPropertiesReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Input Type characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientReadInputTypeReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Input Status characteristic.

    @param clntHndl The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientReadInputStatusReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Audio Input Description characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientReadAudioInputDescRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Gain setting operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Input State characteristic.
    @param gainSetting   Value of gain setting to set

    NOTE: A GATT_AICS_CLIENT_SET_GAIN_SETTING_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientSetGainSettingReq(ServiceHandle clntHndl,
                                     uint8 changeCounter,
                                     int8 gainSetting);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Unmute operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Input State characteristic.

    NOTE: A GattAicsClientUnmuteCfm message will be sent
          to the registered application Task.

*/
void GattAicsClientUnmuteReq(ServiceHandle clntHndl, uint8 changeCounter);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Mute operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Input State characteristic.

    NOTE: A GATT_AICS_CLIENT_MUTE_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientMuteReq(ServiceHandle clntHndl, uint8 changeCounter);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Manual Gain Mode operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Input State characteristic.

    NOTE: A GATT_AICS_CLIENT_SET_MANUAL_GAIN_MODE_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientSetManualGainModeReq(ServiceHandle clntHndl, uint8 changeCounter);

/*!
    @brief This API is used to write the Audio Input Control point characteristic in order to execute
           the Set Automatic Gain Mode operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Input State characteristic.

    NOTE: A GATT_AICS_CLIENT_SET_AUTOMATIC_GAIN_MODE_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientSetAutomaticGainModeReq(ServiceHandle clntHndl, uint8 changeCounter);

/*!
    @brief This API is used to write the Audio Input Description characteristic.

    @param clntHndl          The service handle for this GATT Client Service.
    @param sizeValue         Size of the value to set.
    @param audioInputDescVal Value of Audio Input Description to set.

    NOTE: A GATT_AICS_CLIENT_SET_AUDIO_INPUT_DESC_CFM message will be sent
          to the registered application Task.

*/
void GattAicsClientSetAudioInputDescReq(ServiceHandle clntHndl,
                                        uint16 sizeValue,
                                        uint8 *audioInputDescVal);

#endif /* GATT_AICS_CLIENT_H */

