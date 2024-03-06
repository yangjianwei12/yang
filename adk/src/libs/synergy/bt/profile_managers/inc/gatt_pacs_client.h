/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*!
@file    gatt_pacs_client.h
@brief   Header file for the Published Audio Capabilities Service library.

        This file provides documentation for the GATT PACS Client library
        API (library name: Gatt_pacs_server). This is based on PACS
        Bluetooth service specification  d09r03_interim_changes_2
        (Revision Date: 2020-02-19)
*/

#include "csr_list.h"
#include "qbl_types.h"
#include "csr_bt_tasks.h"
#include "service_handle.h"
#include "csr_bt_gatt_prim.h"



#ifndef GATT_PACS_CLIENT_H
#define GATT_PACS_CLIENT_H

#define  GATT_PACS_CLIENT_MESSAGE_BASE (0x66C8)

/*!
    @brief GATT PACS client library notification type defines.
*/

typedef uint16 GattPacsNotificationType;

#define GATT_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT  ((GattPacsNotificationType)(0x01))
#define GATT_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT  ((GattPacsNotificationType)(0x02))
#define GATT_PACS_NOTIFICATION_SINK_PAC_RECORD          ((GattPacsNotificationType)(0x04))
#define GATT_PACS_NOTIFICATION_SINK_AUDIO_LOCATION      ((GattPacsNotificationType)(0x08))
#define GATT_PACS_NOTIFICATION_SOURCE_PAC_RECORD        ((GattPacsNotificationType)(0x10))
#define GATT_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION    ((GattPacsNotificationType)(0x20))
#define GATT_PACS_NOTIFICATION_SINK_ALL                 ((GattPacsNotificationType)(0x0F))
#define GATT_PACS_NOTIFICATION_SOURCE_ALL               ((GattPacsNotificationType)(0x33))
#define GATT_PACS_NOTIFICATION_ALL                      ((GattPacsNotificationType)(0x3F))

/*!
    @brief persistent data for each known PACS device.

    Each PACS device that is bonded can have data associated against
    it so that re-connections are much faster in that case no GATT discovery is required.
*/

typedef struct
{

    uint16 startHandle;                             /*! Start Handle. */
    uint16 endHandle;                               /*! End Handle. */

    uint8 sinkPacRecordCount;
    uint8 sourcePacRecordCount;

    uint16 pacsSinkAudioLocationHandle;          /*! Sink audio location Handle: Used by lib, unused by app*/
    uint16 pacsSinkAudioLocationCcdHandle;      /*! Sink audio location CC Handle */

    uint16 pacsSourceAudioLocationHandle;        /*! Source audio location Handle: Used by lib, unused by app*/
    uint16 pacsSourceAudioLocationCcdHandle;    /*! Source audio location CC Handle */

    uint16 pacsAvailableAudioContextHandle;      /*! Audio content availability Handle: Used by lib, unused by app*/
    uint16 pacsAvailableAudioContextCcdHandle;  /*! Audio context availability CC handle */

    uint16 pacsSupportedAudioContextHandle;      /*! Audio content availability Handle: Used by lib, unused by app*/
    uint16 pacsSupportedAudioContextCcdHandle;  /*! Audio context support CCD Handle: Used by lib, unused by app*/

    uint16* pacsSinkPacRecordHandle;
    uint16* pacsSinkPacRecordCcdHandle;

    uint16* pacsSourcePacRecordHandle;
    uint16* pacsSourcePacRecordCcdHandle;

} GattPacsClientDeviceData;


/*! @brief Defines of messages a client task may receive from the Published Audio
           Capability Service Client library.
 */

typedef uint16 GattPacsServiceMessageId;

#define GATT_PACS_CLIENT_INIT_CFM                           (GATT_PACS_CLIENT_MESSAGE_BASE)
#define GATT_PACS_CLIENT_TERMINATE_CFM                      (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_PACS_CLIENT_NOTIFICATION_CFM                   (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0002u)
#define GATT_PACS_CLIENT_PAC_RECORD_NOTIFICATION_IND        (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0003u)
#define GATT_PACS_CLIENT_AUDIO_LOCATION_NOTIFICATION_IND    (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_PACS_CLIENT_AUDIO_CONTEXT_NOTIFICATION_IND     (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_PACS_CLIENT_READ_PAC_RECORD_CFM                (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0006u)
#define GATT_PACS_CLIENT_READ_AUDIO_LOCATION_CFM            (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0007u)
#define GATT_PACS_CLIENT_READ_AUDIO_CONTEXT_CFM             (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0008u)
#define GATT_PACS_CLIENT_WRITE_AUDIO_LOCATION_CFM           (GATT_PACS_CLIENT_MESSAGE_BASE + 0x0009u)


/*!
    @brief Defines for PACS client status code
*/

typedef uint16 GattPacsClientStatus;

#define GATT_PACS_CLIENT_STATUS_SUCCESS         ((GattPacsClientStatus)(0x0000)) /*! Request was success */
#define GATT_PACS_CLIENT_STATUS_NOT_ALLOWED     ((GattPacsClientStatus)(0x0001)) /*! Request is not allowed at the moment,something went wrong internally  */
#define GATT_PACS_CLIENT_STATUS_NO_CONNECTION   ((GattPacsClientStatus)(0x0002)) /*! There is no GATT connection exists for given CID so that service library can issue a request to remote device */
#define GATT_PACS_CLIENT_STATUS_FAILED          ((GattPacsClientStatus)(0x0003)) /*! Request has been failed */


/*!
    @brief Defines for PACS client type

    Each PACS client device is either configured as SINK/SOURCE. The defines indicates
    Type of client device configuration.
*/

typedef uint16  GattPacsClientType;

#define GATT_PACS_CLIENT_SINK       ((GattPacsClientType)(0x0000)) /*! client is audio stream sink */
#define GATT_PACS_CLIENT_SOURCE     ((GattPacsClientType)(0x0001)) /*! client is audio stream source*/
#define INVALID_PACS_CLIENT_TYPE    ((GattPacsClientType)(0xff))


/*!
    @brief Defines for PACS client audio context

*/

typedef uint16  GattPacsClientAudioContext;

#define GATT_PACS_CLIENT_AVAILABLE      ((GattPacsClientAudioContext)(0x0000)) /*! client is audio stream sink */
#define GATT_PACS_CLIENT_SUPPORTED      ((GattPacsClientAudioContext)(0x0001)) /*! client is audio stream source*/
#define INVALID_PACS_CONTEXT_TYPE       ((GattPacsClientAudioContext)(0xff))



/*!
    @brief Parameters used by the Initialization API,
    valid value of these  parameters are must for library initialization
*/
typedef struct
{
    connection_id_t    cid;                                 /*! Connection ID. */
    uint16             startHandle;                         /*! Start Handle. */
    uint16             endHandle;                           /*! End Handle. */
} GattPacsClientInitData;


/*!
    @brief Pacs Client Library initialization confirmation

    This message is sent in response to call of api GattPacsClientInitReq().
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId    id;                         /*! service message id */
#endif
    connection_id_t             cid;                        /*! connection id */
    ServiceHandle               clientHandle;               /*! service handle for client instance */
    GattPacsClientStatus        status;                     /*! status message */
} GattPacsClientInitCfm;


/*!@brief Pacs Client Library Termination confirmation
    Sent in response to the GattPacsClientTerminateReq(). Once this
    has been sent from the Client Service, the client service handle
    will no longer be valid and the client resources will be freed.
*/

typedef struct
{
    GattPacsServiceMessageId        id;                     /*! service message id*/
    connection_id_t                 cid;                    /*! connection id */
    GattPacsClientDeviceData        deviceData;             /*! data to be stored in persistent storage */
    GattPacsClientStatus            status;                 /*! status messsage */
} GattPacsClientTerminateCfm;

/*!
    @brief PACS Client Library notification registration confirmation

    This message is sent in response to call of GattPacsClientRegisterForNotification().
    This indicates whether registration for characteristic notification succeeded or not.

*/

typedef struct
{

#ifndef ADK_GATT
    GattPacsServiceMessageId id;                             /*! Service message id */
#endif
    ServiceHandle         clientHandle;                      /*! service handle for client instance */
    GattPacsClientStatus  status;                            /*! status as per GattPacsClientStatus */

} GattPacsClientNotificationCfm;


/*!
    @brief PACS Client record structure, which is received from remote server

    The PACS is organized into structure which contains number of records,
    length of all the records and a pointer to the block of memory containing PAC record
     _______________________________________________________________________________________
    |                |        |                                                             |
    | no of records  |length  | block containing records                                    |
    |________________|________|_____________________________________________________________|
    
*/

typedef struct
{
    uint8   pacRecordCount;                                    /*! number of records */
    uint16  valueLength;                                       /*! PAC record length */
    uint8*  value;                                             /*! PAC record */
} GattPacsClientRecord;


/*!
    @brief PACS Client Library notification indication for PAC record.

    This message will be sent when PAC record change is
    notified by connected remote server to the client.
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId     id;                               /*! service message id */
#endif
    ServiceHandle             clientHandle;                    /*! service handle for client instance */
    GattPacsClientType        type;                            /*! client is a source or sink*/
    GattPacsClientRecord      record;                          /*! PAC record*/
} GattPacsClientPacRecordNotificationInd;

/*!
    @brief PACS Client Library notification indication for Audio Location.

    This message will be sent when change in audio location
    is notified by connected remote server to the GATT client.
    */

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId     id;                               /*! service message id */
#endif
    ServiceHandle             clientHandle;                     /*! service handle for client instance */
    GattPacsClientType        type;                             /*! client is a source or sink*/
    uint32                    location;                         /*! Sink or Source Audio location*/
} GattPacsClientAudioLocationNotificationInd;

/*!
    @brief PACS Client Library notification indication for AUDIO context,

    This message will be sent when change in either available/supported
    audio context is notified by connected remote server to GATT client.
    */

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId       id;                              /*! service message handle */
#endif
    ServiceHandle               clientHandle;                    /*! service handle for client instance */
    GattPacsClientAudioContext  context;                         /*! context requires: available/supported */
    uint32                      contextValue;                    /*! context value */
} GattPacsClientAudioContextNotificationInd;


/*!
    @brief PACS Client Library Read Confirmation for PAC record.

    This message is sent in response to call of the
    api GattPacsClientReadPacRecordReq(). Message contains PAC record
    received from remote server
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId      id;                               /*! service message id */
#endif
    ServiceHandle            clientHandle;                     /*! service handle for client instance */
    GattPacsClientStatus     status;                           /*! status as per gatt_paccs_client_status */
    GattPacsClientType       type;                             /*! type: source/sink */
    GattPacsClientRecord     record;                           /*! PAC record */
    bool                     moreToCome;
} GattPacsClientReadPacRecordCfm;


/*!
    @brief PACS Client Library Audio Location Read Confirmation.

    This message is sent in response to call of api GattPacsClientReadAudioLocationReq().
    Value of the location of remote device is sent in this message.
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId      id;                              /*! service message id */
#endif
    ServiceHandle              clientHandle;                    /*! service handle for client instance */
    GattPacsClientStatus       status;                          /*! status as per GattPacsClientStatus */
    GattPacsClientType         type;                            /*! type: source/sink */
    uint32                     location;                        /*! Audio device location */
} GattPacsClientReadAudioLocationCfm;

/*!
    @brief PACS Client Library Audio Context Read Confirmation,

    This message is sent in response when api GattPacsClientReadAudioContextReq() is
    called. Available/Supported audio context of remote server is sent in this message.
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId          id;                          /*! service message id */
#endif
    ServiceHandle                 clientHandle;                /*! service handle for client instance */
    GattPacsClientStatus          status;                      /*! status as per GattPacsClientStatus */
    GattPacsClientAudioContext    context;                     /*! available/supported audio context */
    uint32                        value;                       /*! audio context value */
} GattPacsClientReadAudioContextCfm;

/*!
    @brief PACS Client Library Audio Location Write Confirmation.

    This message is sent in response when api GattPacsClientWriteAudioLocationReq() is
    called. Information of whether the audio location information of remote device is updated
    or not is carried by this message.
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsServiceMessageId      id;                               /*! service message id */
#endif
    ServiceHandle              clientHandle;                     /*! service handle for client instance */
    GattPacsClientStatus       status;                           /*! status as per GattPacsClientStatus */
    GattPacsClientType         type;                             /*! type: source/sink */
} GattPacsClientWriteAudioLocationCfm;


/*!
    @brief Initialises the PACS Client.
     Initialize Pacs client handles, It starts finding out the characteristic handles of
     PAC Service. Once the initialisation has been completed, GATT_PACS_CLIENT_INIT_CFM will be
     received with status as enumerated as GattPacsClientStatus.'gatt_pacs_client_status_success'
     has to be considered initialisation of library is done successfully and all the required
     charecteristics has been found out

     NOTE:This interface need to be invoked for every new gatt connection when the client wish to use
     PACS client library

    @param appTask      The Task that will receive the messages sent from this Pacs client  library.
    @param initData     As defined in GattPacsClientInitData, it is must all the parameters
                         are valid. The memory allocated for GattPacsClientInitData
                         can be freed once the API returns.
    @param deviceData   Pointer to GattPacsClientDeviceData data structure. Pointers inside the
                         structure needs to be freed by application itself after successful init.

*/

void GattPacsClientInitReq(AppTask appTask,
                          const GattPacsClientInitData *initData,
                          const GattPacsClientDeviceData *deviceData);

/*!
    @brief PACS Client Library terminate functions.

    Calling this function will free all resources for this Client Service.

    @param clientHandle Service handle to the GATT PACS client service
*/

void GattPacsClientTerminateReq(ServiceHandle clientHandle);

/*!
    @brief Register for the the PACS notifications.

    @param clientHandle Service handle to memory location of client instance that was passed into the
                         GattPacsRegisterForNotification API.
    @param notifyType Bitwise value of all the PACS notifications.
    @param notifyEnable Notification enable (TRUE) or disable (FALSE) value
*/


void GattPacsClientRegisterForNotification(ServiceHandle clientHandle,
                                          GattPacsNotificationType notifyType,
                                          bool notifyEnable);

/*!
    @brief Read SOURCE and SINK PACS records.

    @param clientHandle Service handle to the GATT PACS client service
    @param type The client is either source or sink.
*/


void GattPacsClientReadPacRecordReq(ServiceHandle clientHandle,
                                   GattPacsClientType type);

/*!
    @brief Read SINK and SOURCE PAC and Audio location.

    @param clientHandle Service handle to the GATT PACS client service
    @param type The client is either source or a sink.
*/

void GattPacsClientReadAudioLocationReq(ServiceHandle clientHandle,
                                       GattPacsClientType type);

/*!
    @brief Read Supported/Available Audio context of a connected device.

    Supported/Available of connected remote device is requested by
    using this function.

    @param clientHandle Service handle to the GATT PACS client service
    @param context  available/supported audio contexts in client.
*/

void GattPacsClientReadAudioContextReq(ServiceHandle clientHandle,
                                      GattPacsClientAudioContext context);

/*!
    @brief Write SINK and SOURCE PAC and Audio location.

    Sink/Source audio location of connected remote device can be modified/ written
    using this function.

    @param clientHandle Service handle to the GATT PACS client service
    @param type The client is either source or a sink.
    @param value The value to be written into the client.
*/

void GattPacsClientWriteAudioLocationReq(ServiceHandle clientHandle,
                                        GattPacsClientType type,
		                                uint32 value);

/*!
    @brief Find Client supported audio role.

    This api finds the role of connected remote PACS device.

    @param clientHandle Service handle to the GATT PACS client service
    @param type The client is either source or a sink.
    @return TRUE if supported, FALSE otherwise
*/

bool GattPacsClientFindAudioRoleReq(ServiceHandle clientHandle,
                                   GattPacsClientType type);

/*!
    @brief This API is used to retrieve the pacs characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.
    @return GattPacsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.
*/

GattPacsClientDeviceData *GattPacsClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_PACS_CLIENT_H */
