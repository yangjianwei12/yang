/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_vocs_client.h
    
DESCRIPTION
    Header file for the GATT VOCS Client library.
*/


/*!
@file    gatt_vocs_client.h
@brief   Header file for the GATT VOCS (Volume Offset Control Service)
         Client library.

        This file provides documentation for the GATT VOCS Client library
        API (library name: gatt_vocs_client).
*/

#ifndef GATT_VOCS_CLIENT_H
#define GATT_VOCS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include <library.h>
#else
#define GATT_VOCS_CLIENT_MESSAGE_BASE 0x00
#endif

/*!
    @brief Handles of the VOCS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 offsetStateHandle;
    uint16 audioLocationHandle;
    uint16 volumeOffsetControlPointHandle;
    uint16 audioOutputDescriptionHandle;

    uint16 offsetStateCccHandle;
    uint16 audioLocationCccHandle;
    uint16 audioOutputDescriptionCccHandle;

    uint8 audioLocationProperties;
    uint8 audioOutputDescProperties;
} GattVocsClientDeviceData;

/*!
    \brief IDs of messages a profile task can receive from the
    GATT VOCS Client library.
*/
typedef uint16 GattVocsClientMessageId;

/*! { */
/*! Values for GattVocsClientMessageId */
#define GATT_VOCS_CLIENT_INIT_CFM                         (GATT_VOCS_CLIENT_MESSAGE_BASE)
#define GATT_VOCS_CLIENT_TERMINATE_CFM                    (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM         (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0002u)
#define GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM       (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0003u)
#define GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM    (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM        (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM      (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0006u)
#define GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM   (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0007u)
#define GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM            (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0008u)
#define GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM          (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0009u)
#define GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM       (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Au)
#define GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM            (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Bu)
#define GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM                (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Cu)
#define GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM        (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Du)
#define GATT_VOCS_CLIENT_OFFSET_STATE_IND                 (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Eu)
#define GATT_VOCS_CLIENT_AUDIO_LOCATION_IND               (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x000Fu)
#define GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND            (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0010u)
#define GATT_VOCS_CLIENT_MESSAGE_TOP                      (GATT_VOCS_CLIENT_MESSAGE_BASE + 0x0011u)

/*! } */

/*!
    \brief Type for the VOCS Client status code type.
*/
typedef uint16 GattVocsClientStatus;

/*! { */
/*! Values of the VOCS Client status code. */
#define GATT_VOCS_CLIENT_STATUS_SUCCESS                   (0x0000u) /*!> Request was a success*/
#define GATT_VOCS_CLIENT_STATUS_INVALID_PARAMETER         (0x0001u) /*!> Invalid parameter was supplied*/
#define GATT_VOCS_CLIENT_STATUS_DISCOVERY_ERR             (0x0002u) /*!> Error in discovery of Characteristics*/
#define GATT_VOCS_CLIENT_STATUS_FAILED                    (0x0003u) /*!> Request has failed*/
#define GATT_VOCS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0004u) /*!> Insufficient Resources to complete
                                                                         the request. */
/*! } */

/*! Audio location values */
typedef uint32 GattVocsClientAudioLoc;

#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_LEFT            (0x00000001u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_RIGHT           (0x00000002u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_CENTER          (0x00000004u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_LOW_FREQ_EFFECTS_1    (0x00000008u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BACK_LEFT             (0x00000010u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BACK_RIGHT            (0x00000020u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_LEFT_CNTR       (0x00000040u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_RIGHT_CNTR      (0x00000080u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BACK_CENTER           (0x00000100u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_LOW_FREQ_EFFECTS_2    (0x00000200u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_SIDE_LEFT             (0x00000400u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_SIDE_RIGHT            (0x00000800u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_FRONT_LEFT        (0x00001000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_FRONT_RIGHT       (0x00002000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_FRONT_CENTER      (0x00004000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_CENTER            (0x00008000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_BACK_LEFT         (0x00010000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_BACK_RIGHT        (0x00020000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_SIDE_LEFT         (0x00040000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_SIDE_RIGHT        (0x00080000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_TOP_BACK_CENTER       (0x00100000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BOTTOM_FRONT_CNTR     (0x00200000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BOTTOM_FRONT_LEFT     (0x00400000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_BOTTOM_FRONT_RIGHT    (0x00800000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_LEFT_WIDE       (0x01000000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_FRONT_RIGHT_WIDE      (0x02000000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_LEFT_SURROUND         (0x04000000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_RIGHT_SURROUND        (0x08000000u)
#define GATT_VOCS_CLIENT_AUDIO_LOC_INVALID               (0xffffffffu)

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
     connection_id_t cid;       /*! Connection ID. */
     uint16 startHandle;
     uint16 endHandle;
} GattVocsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattVocsClientInitReq API.
*/
typedef struct
{
    GattVocsClientMessageId  id;
    connection_id_t cid;              /*! Connection ID. */
    ServiceHandle srvcHdnl;        /*! Reference handle for the instance */
    GattVocsClientStatus status;      /*! Status of the initialisation attempt */
} GattVocsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattVocsClientTerminateReq API.
*/
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle srvcHdnl;            /*! Reference handle for the instance */
    GattVocsClientStatus status;          /*! Status of the termination attempt */
    GattVocsClientDeviceData deviceData;  /*! Device data: handles used for the peer device. */
} GattVocsClientTerminateCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Offset State characteristic.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle         srvcHndl;   /*! Reference handle for the instance */
    status_t                 status;     /*! Status of the setting attempt */
} GattVocsClientOffsetStateSetNtfCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Location characteristic.
 */
typedef GattVocsClientOffsetStateSetNtfCfm GattVocsClientAudioLocationSetNtfCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Audio Output Description characteristic.
 */
typedef GattVocsClientOffsetStateSetNtfCfm GattVocsClientAudioOutputDescSetNtfCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM message that is sent by the library,
    as a result of writing the Volume Offset Control point characteristic on the server using the set
    volume offset operation.
 */
typedef GattVocsClientOffsetStateSetNtfCfm GattVocsClientSetVolumeOffsetCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM message that is sent by the library,
    as a result of writing the Audio Location characteristic on the server.
 */
typedef GattVocsClientOffsetStateSetNtfCfm GattVocsClientSetAudioLocCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM message that is sent by the library,
    as a result of writing the Audio Output Description characteristic.
 */
typedef GattVocsClientOffsetStateSetNtfCfm GattVocsClientSetAudioOutputDescCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_OFFSET_STATE_IND message that is sent by the library,
    as a result of a notification of the remote offset state.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;      /*! Reference handle for the instance */
    int16             volumeOffset;  /*! Value of the Volume Offset field */
    uint8             changeCounter; /*! Value of the change counter for the Offset State */
} GattVocsClientOffsetStateInd;

/*! @brief Contents of the GATT_VOCS_CLIENT_AUDIO_LOCATION_IND message that is sent by the library,
    as a result of a notification of the remote audio location characteristic value.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle srvcHdnl;            /*! Reference handle for the instance */
    GattVocsClientAudioLoc audioLocation; /*! Audio Location value */
} GattVocsClientAudioLocationInd;

/*! @brief Contents of the GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_IND message that is sent by the library,
    as a result of a notification of the remote audio output description characteristic value.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;              /*! Reference handle for the instance */
    uint16            sizeValue;             /*! Size of the Audio Output Description value */
    uint8            *audioOutputDesc;       /*! Audio Output Description value */
} GattVocsClientAudioOutputDescInd;

/*! @brief Contents of the GATT_VCS_CLIENT_READ_OFFSET_STATE_CCC_CFM message that is sent by the library,
    as a result of reading of the Offset State Client Configuration characteristic on the server.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;   /*! Reference handle for the instance */
    status_t          status;     /*! Status of the reading attempt */
    uint16            sizeValue;  /*! Size of the ccc value */
    uint8            *value;      /*! CCC value */
} GattVocsClientReadOffsetStateCccCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM message that is sent by the library,
    as a result of reading of the Audio Location Client Configuration characteristic on the server.
 */
typedef GattVocsClientReadOffsetStateCccCfm GattVocsClientReadAudioLocationCccCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM message that is sent by the
           library, as a result of reading of the Audio Output Description Client Configuration
           characteristic on the server.
 */
typedef GattVocsClientReadOffsetStateCccCfm GattVocsClientReadAudioOutputDescCccCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM message that is sent by the library,
    as a result of a reading of the Offset State characteristic on the server.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;       /*! Reference handle for the instance */
    status_t          status;         /*! Status of the reading attempt */
    int16             volumeOffset;   /*! Value of the Volume Offset */
    uint8             changeCounter;  /*! Value of the change counter */
} GattVocsClientReadOffsetStateCfm;

/*! @brief Contents of the GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM message that is sent by the library,
    as a result of a read of the Audio Location characteristic of the server.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;           /*! Reference handle for the instance */
    status_t          status;             /*! Status of the reading attempt */
    GattVocsClientAudioLoc audioLocation; /*! Audio location value */
}GattVocsClientReadAudioLocationCfm;


/*! @brief Contents of the GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM message that is sent by the library,
    as a result of a read of the Audio Output Description characteristic of the server.
 */
typedef struct
{
    GattVocsClientMessageId  id;
    ServiceHandle  srvcHdnl;                /*! Reference handle for the instance */
    status_t          status;                  /*! Status of the reading attempt */
    uint16            sizeValue;               /*! Size of Audio Output Description value */
    uint8            *audioOuputDesc;          /*! Audio Output Description value */
}GattVocsClientReadAudioOutputDescCfm;

/*!
    @brief GATT VOCS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    \param initData    Configuration data for client initialisation.
    \param deviceData  Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_VOCS_CLIENT_INIT_CFM will be received with a GattVocsClientStatus status code.
*/
void GattVocsClientInitReq(AppTask theAppTask,
                           const GattVocsClientInitData   *initData,
                           const GattVocsClientDeviceData *deviceData);

/*!
    \brief GATT VOCS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    \param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_VOCS_CLIENT_TERMINATE_CFM will be received with a GattVocsClientStatus status code.
*/
void GattVocsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of the Offset State
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_VOCS_CLIENT_OFFSET_STATE_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattVocsClientOffsetStateRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Audio Location
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattVocsClientAudioLocationRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable);

/*!
    @brief This API is used to write the client characteristic configuration of the Audio Output Description
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: A GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM message will be sent to the registered application Task.

*/
void GattVocsClientAudioOutputDescRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Offset State
           characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_OFFSET_STATE_CCC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadOffsetStateCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Location
            characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadAudioLocationCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Audio Output
           Description characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadAudiOutputDescCccRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Offset State characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadOffsetStateRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Audio Location characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadAudioLocationRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to read the Audio Output Description characteristic.

    @param clntHndl  The service handle for this GATT Client Service.

    NOTE: A GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientReadAudioOutputDescRequest(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the Volume Offset Control point characteristic in order to execute
           the Set Volume Offset operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param changeCounter Last read value of change counter in the Offset State characteristic.
    @param volumeOffset  Value of Volume Offset to set

    NOTE: A GATT_VOCS_CLIENT_SET_VOLUME_OFFSET_CFM message will be sent to the registered application Task.

*/
void GattVocsClientSetVolumeOffsetReq(ServiceHandle clntHndl, uint8 changeCounter, int16 volumeOffset);

/*!
    @brief This API is used to write the Audio Location characteristic.

    @param clntHndl          The service handle for this GATT Client Service.
    @param audioLocationVal Value of Audio Location to set.

    NOTE: A GATT_VOCS_CLIENT_SET_AUDIO_LOC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientSetAudioLocReq(ServiceHandle clntHndl,
                                  GattVocsClientAudioLoc audioLocationVal);

/*!
    @brief This API is used to write the Audio Output Description characteristic.

    @param clntHndl             The service handle for this GATT Client Service.
    @param sizeValue            Size of the value to set.
    @param audioOutputDescVal   Value of Audio Output Description to set.

    NOTE: A GATT_VOCS_CLIENT_SET_AUDIO_OUTPUT_DESC_CFM message will be sent to the registered application Task.

*/
void GattVocsClientSetAudioOutputDescReq(ServiceHandle clntHndl,
                                         uint16 sizeValue,
                                         uint8 *audioOutputDescVal);

#endif /* GATT_VOCS_CLIENT_H */

