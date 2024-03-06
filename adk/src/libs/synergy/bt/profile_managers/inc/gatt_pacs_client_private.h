/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_PACS_CLIENT_PRIVATE_H
#define GATT_PACS_CLIENT_PRIVATE_H

#include <stdio.h>

#include "csr_list.h"
#include "gatt_pacs_client.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Defines for pacs client library */

#define  GATT_PACS_WRITE_CCD_VALUE_LENGTH (0x02)

/* Invalid pacs  measurement handle*/
#define INVALID_PACS_HANDLE (0xffff)

/* PACS Indication value */
#define PACS_NOTIFICATION_VALUE (0x01)

/*Service handle initialization failure*/
#define INVALID_SERVICE_HANDLE ((ServiceHandle)(0x0000))

 /* Macros for creating messages */
#define MAKE_PACS_CLIENT_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T *)(calloc(1,sizeof(TYPE##_T)))

#define CLEAR_PENDING_TYPE(TYPE)  TYPE = 0xFF

/*To be used for WORD order values*/
#define MAKE_PACS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *)(calloc(1,sizeof(TYPE##_T) + LEN))

/* PACS Service UUIDs */

#define CSR_BT_GATT_UUID_PACS_SINK                      ((CsrBtUuid16)0x2BC9)
#define CSR_BT_GATT_UUID_PACS_SINK_AUDIO_LOC            ((CsrBtUuid16)0x2BCA)
#define CSR_BT_GATT_UUID_PACS_SOURCE                    ((CsrBtUuid16)0x2BCB)
#define CSR_BT_GATT_UUID_PACS_SOURCE_AUDIO_LOC          ((CsrBtUuid16)0x2BCC)
#define CSR_BT_GATT_UUID_PACS_AVAILABLE_AUDIO_CONTEXT   ((CsrBtUuid16)0x2BCD)
#define CSR_BT_GATT_UUID_PACS_SUPPORTED_AUDIO_CONTEXT   ((CsrBtUuid16)0x2BCE)

typedef CsrCmnList_t service_handle_list_t;


/* This structure is private to pacs client.
 * Application can't access this instance at any point of time.
*/
typedef struct
{
    CsrCmnList_t    clientHandleList;

} PacsC;

typedef struct CsrBtPacRecordCharacElementTag
{
    struct CsrBtPacRecordCharacElementTag*  next; /* must be first */
    struct CsrBtPacRecordCharacElementTag*  prev; /* must be second */
    uint8                                   recordId; /* Needed as several charac can have same UUID */
    CsrBtGattHandle                         declarationHandle; /* Handle for the characteristic declaration*/
    CsrBtGattHandle                         valueHandle; /* Characteristic Value Handle */
    CsrBtGattHandle                         pacRecordCccdHandle;
    CsrBtGattHandle                         endHandle;

} CsrBtPacRecordCharacElement;

/*!
 *  PACS client Instance
 */

typedef struct
{
    AppTaskData lib_task;                                 /*! Lib Task */
    AppTask app_task;                                     /*! Application Registered Task */

        /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t* srvcElem;

    uint16 startHandle;
    uint16 endHandle;

    uint16 pacsSinkAudioLocationHandle;          /*! Sink audio location Handle: Used by lib, unused by app*/
    uint16 pacsSinkAudioLocationCcdHandle;       /*! Sink audio location CC Handle */

    uint16 pacsSourceAudioLocationHandle;        /*! Source audio location Handle: Used by lib, unused by app*/
    uint16 pacsSourceAudioLocationCcdHandle;     /*! Source audio location CC Handle */

    uint16 pacsAvailableAudioContextHandle;      /*! Audio content availability Handle: Used by lib, unused by app*/
    uint16 pacsAvailableAudioContextCcdHandle;   /*! Audio context availability CC handle */

    uint16 pacsSupportedAudioContextHandle;      /*! Audio content availability Handle: Used by lib, unused by app*/
    uint16 pacsSupportedAudioContextCcdHandle;   /*! Audio context support CCD Handle: Used by lib, unused by app*/

    uint16 pendingHandle;
    GattPacsNotificationType notifyType;                                /*! Notification command pending */
    bool   notifyEnable;                              /*! Notification enable (TRUE) or disable (FALSE) value */
    bool   internalReq;
    GattPacsNotificationType notifyTypeEnabled;

    CsrCmnList_t  sinkPacRecordList;
    CsrCmnList_t  sourcePacRecordList;

    uint8 sinkPacRecordCount;
    uint8 sourcePacRecordCount;

    uint8 writeSourceCccdCount;
    uint8 writeSinkCccdCount;

    uint8 readSourceRecordCount;
    uint8 readSinkRecordCount;

} GPacsC;

/* Defines For Library PACS client internal message id*/

typedef uint16 GattPacsInternalMsgId;

#define PACS_CLIENT_INTERNAL_MSG_BASE               (GattPacsInternalMsgId)0x0005
#define PACS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ   (GattPacsInternalMsgId)0x0006

#define ADD_PACS_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(GPacsC))

#define FREE_PACS_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_PACS_INST_BY_SERVICE_HANDLE(_Handle) \
                              (GPacsC *)ServiceHandleGetInstanceData(_Handle)

#define ADD_PAC_RECORD(_List) \
                  CsrCmnListElementAddLast((CsrCmnList_t*)&(_List),sizeof(CsrBtPacRecordCharacElement))

/*!
    @brief PACS Client Library internal message sent in response to GattPacsClientRegisterForNotification
           api call.
*/

typedef struct
{
#ifndef ADK_GATT
    GattPacsInternalMsgId           id;                          /*! internal message id */
#endif
    ServiceHandle                   clientHandle;                /*! service handle for client instance */
    GattPacsNotificationType        type;                        /*! notification type */
    bool                            enable;                      /*! enable/disable notification */
} PacsClientInternalMsgNotificationReq;


/*!
    @brief Internal function to find pacs client service handle using btConnId.

    @param elem  pointer to list element where service handle is stored
    @param data  Pointer to data containing gattId
    @return returns TRUE if the element is found.
*/

bool pacsSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);

/*!
    @brief Internal function to find PACS client instance by Service handle

    @param elem  pointer to list element where service handle is stored
    @param data  Pointer to data containing service handle
    @return returns TRUE if the element is found.
*/

CsrBool pacsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

/*!
    @brief  Internal function which submits request to register for Audio location notifications
            to the remote server.

    @param clientHandle service handle to memory location of client instance that was passed into the API
    @param type The client is either source or sink.
    @param enable Either enable or disable the notification
*/

void pacsClientRegisterForAudioLocationNotification(
                                          ServiceHandle clientHandle,
                                          GattPacsClientType type,
                                          bool enable);

/*!
    @brief  Internal function which submits request to register for PAC Record notifications
            to the remote server.

    @param clientHandle service handle to memory location of client instance that was passed into the API
    @param type The client is either source or sink.
    @param enable Either enable or disable the notification
*/


void pacsClientRegisterForPacRecordNotification(ServiceHandle clientHandle,
                                                   GattPacsClientType type,
                                                   bool enable);

/*!
    @brief Internal function which submits request to register for audio context notifications
           to the remote server.

    @param clientHandle service handle to memory location of client instance that was passed into the API
    @param context   available or supported.
    @param enable Either enable or disable the notification
*/

void pacsClientRegisterForAudioContextNotification(ServiceHandle clientHandle,
                                                   GattPacsClientAudioContext context,
                                                   bool enable);

/*!
    @brief Internal function which sends PAC record read Cfm to UL/App.

    @param clientHandle service handle to memory location of client instance that was passed into the API.
    @param AppTask  Task ID of upper layer profile/Application.
    @param status  Status of the operarion i.e success/ failure.
    @param type  Source/Sink.
    @param pacRecordCount Number of PAC records present in current byte stream.
    @param valueLength  length of bytestream recieved from GATT. 
    @param value bytestream containing PAC records.
    @param moreToCome True if more records expected. FALSE otherwise.
*/


void pacsClientSendPacsCapabilitiesCfm(ServiceHandle clientHandle,
                                     AppTask appTask,
                                     GattPacsClientStatus status,
                                     GattPacsClientType type,
                                     uint8 pacRecordCount,
                                     uint16 valueLength,
                                     uint8* value,
                                     bool moreToCome);

/*!
    @brief Internal function which sends Audio Location read Cfm to UL/App.

    @param clientHandle service handle to memory location of client instance that was passed into the API.
    @param AppTask  Task ID of upper layer profile/Application.
    @param location   Location value recieved from remote device.
    @param status  Status of the operarion i.e success/ failure.
    @param type  Source/Sink.
*/

void pacsClientSendAudioLocationReadCfm(ServiceHandle clientHandle,
                                      AppTask appTask,
                                      uint32 location,
                                      GattPacsClientStatus status,
                                      GattPacsClientType type);

/*!
    @brief Internal function which submits request to register for audio context notifications
           to the remote server.

    @param clientHandle service handle to memory location of client instance that was passed into the API.
        @param AppTask  Task ID of upper layer profile/Application.
    @param context   available or supported.
    @param value   Context value recieved from remote device.
    @param status  Status of the operarion i.e success/ failure.
*/

void pacsClientSendAudioContextReadCfm(ServiceHandle clientHandle,
                                     AppTask appTask,
                                     uint16 context,
                                     uint32 value,
                                     GattPacsClientStatus status);

/*!
    @brief Internal function which sends write cfm to application.

    @param AppTask  Task ID of upper layer profile/Application.
    @param clientHandle service handle to memory location of client instance that was passed into the API.
    @param status  Status of the operarion i.e success/ failure.
*/

void pacsClientSendWriteCfm(AppTask appTask,
                           GattPacsClientStatus status,
                           ServiceHandle clientHandle);

#endif /* GATT_PACS_CLIENT_PRIVATE_H */

