/****************************************************************************
Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    csip.h

DESCRIPTION
    Header file for the Coordinated Set Indentification profile (CSIP) library.
*/

/*!
@file    csip.h
@brief   Header file for the GATT CSIP library.

        This file provides documentation for the GATT CSIP library
        API (library name: csip).
*/

#ifndef CSIP_H
#define CSIP_H

#include "csr_bt_gatt_lib.h"
#include "service_handle.h"


#include "gatt_csis_client.h"

/*!
    \brief Profile handle type.
*/
typedef ServiceHandle CsipProfileHandle;

#define CSIP_PROFILE_PRIM (SYNERGY_EVENT_BASE+CSIP_PRIM)

/*!
    @brief VOCS handles.

*/
typedef struct
{
    uint16                         startHandle;
    uint16                         endHandle;
}CsipCsisHandle;


/*!
    @brief CSIP handles.

*/
typedef struct
{
    GattCsisClientDeviceData csisHandle;
}CsipHandles;

/*!
    @brief Initialisation parameters.

*/
typedef struct
{
    connection_id_t cid;
}CsipInitData;



typedef uint8 CsipCsInfo;
#define CSIP_SIRK     ((CsipCsInfo)0x00)
#define CSIP_SIZE     ((CsipCsInfo)0x01)
#define CSIP_RANK     ((CsipCsInfo)0x02)
#define CSIP_LOCK     ((CsipCsInfo)0x03)

#define CSIP_SIRK_TYPE_SIZE   1
#define CSIP_SIRK_SIZE        16
#define CSIP_SIRK_SIZE_PLUS_TYPE     (CSIP_SIRK_SIZE + CSIP_SIRK_TYPE_SIZE)

#define CSIP_RSI_SIZE   6

/*!
    \brief CSIP status code type.
*/
typedef uint16 CsipStatus;

/*! { */
/*! Values for the CSIP status code */
#define CSIP_STATUS_SUCCESS              ((CsipStatus)0x0000u)  /*!> Request was a success*/
#define CSIP_STATUS_IN_PROGRESS          ((CsipStatus)0x0001u)  /*!> Request in progress*/
#define CSIP_STATUS_INVALID_PARAMETER    ((CsipStatus)0x0002u)  /*!> Invalid parameter was supplied*/
#define CSIP_STATUS_DISCOVERY_ERR        ((CsipStatus)0x0003u)  /*!> Error in discovery of one of the services*/
#define CSIP_STATUS_FAILED               ((CsipStatus)0x0004u)  /*!> Request has failed*/
#define CSIP_STATUS_INST_PERSISTED       ((CsipStatus)0x0005u)  /*!> Unable to free CSIP instance */
/*! } */

/*! @brief Defines of messages a client task may receive from the profile library.
 */
#define CSIP_MESSAGE_BASE 0x0000
typedef uint16 CsipMessageId ;

#define CSIP_INIT_CFM           ((CsipMessageId)(CSIP_MESSAGE_BASE))
#define CSIP_DESTROY_CFM        ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0001u))
#define CSIP_READ_CS_INFO_CFM   ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0002u))
#define CSIP_CS_SET_NTF_CFM     ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0003u))
#define CSIP_SET_LOCK_CFM       ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0004u))
#define CSIP_LOCK_STATUS_IND    ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0005u))
#define CSIP_SIZE_CHANGED_IND   ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0006u))
#define CSIP_SIRK_CHANGED_IND   ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0007u))
#define CSIP_SIRK_DECRYPT_CFM   ((CsipMessageId)(CSIP_MESSAGE_BASE + 0x0008u))

/*!
    @brief Profile library message sent as a result of calling the CsipInitReq API.
*/
typedef struct
{
    CsipMessageId     id;
    CsipStatus        status;       /*! Status of the initialisation attempt*/
    CsipProfileHandle prflHndl;     /*! CSIP profile handle*/
    connection_id_t   cid;          /*! We need cid to map profile handle later*/
} CsipInitCfm;

/*!
    @brief Profile library message sent as a result of calling the CsipDestroyReq API.

    This message will send at first with the value of status of CSIP_STATUS_IN_PROGRESS.
    Another CSIP_DESTROY_CFM message will be sent with the final status (success or fail),
    after CSIP_CSIS_TERMINATE_CFM, CSIP_VOCS_TERMINATE_CFM and
    CSIP_AICS_TERMINATE_CFM have been received.
*/
typedef struct
{
    CsipMessageId      id;
    CsipProfileHandle  prflHndl;  /*! CSIP profile handle*/
    CsipStatus         status;    /*! Status of the initialisation attempt*/
    CsipHandles        *handles;
} CsipDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the CsipDestroyReq API and
           of the receiving of the GATT_CSIS_CLIENT_TERMINATE_CFM message from the Gatt Csis Client
           library.
*/
typedef struct
{
    CsipMessageId               id;
    CsipProfileHandle           prflHndl;   /*! CSIP profile handle*/
    CsipStatus                  status;     /*! Status of the termination attempt*/
    GattCsisClientDeviceData    csisHandle; /*! Characteristic handles of CSIS*/
} CsipCsisTerminateCfm;


/*! @brief Contents of the CSIP_READ_CS_INFO_CFM message that is sent by the library,
    as a result of reading of the Coordinated Set member characteristic on the server.
 */
typedef struct
{
    CsipMessageId      id;
    CsipProfileHandle  prflHndl;   /*! CSIP profile handle*/
    status_t           status;     /*! Status of the reading attempt*/
    CsipCsInfo         csInfoType; /*! Coordinated set Member info type */
    uint16             sizeValue;  /*! Value size*/
    uint8              *value;     /*! Read value */
} CsipReadCsInfoCfm;

/*!
    @brief Profile library message sent as a result of calling the CsipSetLockReq API.
*/
typedef struct
{
    CsipMessageId     id;
    CsipStatus        status;      /*! Status of the setting attempt*/
    CsipProfileHandle prflHndl;    /*! CSIP profile handle*/
} CsipSetLockCfm;

/*!
    @brief Profile library message sent as a result of calling the CsipCSRegisterForNotificationReq API.
*/
typedef struct
{
    CsipMessageId     id;
    CsipStatus        status;      /*! Status of the setting attempt*/
    CsipProfileHandle prflHndl;    /*! CSIP profile handle*/
} CsipCsSetNtfCfm;


/*! @brief Contents of the CSIP_SIZE_CHANGE_IND message that is sent by the library,
    as a result of a notification of the remote size characteristic value.
 */
typedef struct
{
    CsipMessageId     id;
    CsipProfileHandle prflHndl;
    uint8                 sizeValue;
} CsipSizeChangedInd;

/*! @brief Contents of the CSIP_LOCK_STATUS_IND message that is sent by the library,
    as a result of a notification of the remote Lock characteristic value.
 */
typedef struct
{
    CsipMessageId     id;
    CsipProfileHandle prflHndl;
    uint8             lockStatus;
} CsipLockStatusInd;

/*! @brief Contents of the CSIP_SIRK_CHANGE_IND message that is sent by the library,
    as a result of a notification of the remote SIRK characteristic value.
 */
typedef struct
{
    CsipMessageId     id;
    CsipProfileHandle prflHndl;
    uint8             sirkValue[CSIP_SIRK_SIZE_PLUS_TYPE];
} CsipSirkChangedInd;

/*!
  @brief Contents of the CSIP_SIRK_DECRYPT_CFM message that is sent by the library,
  as a result of calling CsipDecryptSirk() API. sirkValue returned is decrypted
  (plain text)
  */
typedef struct
{
    CsipMessageId     id;
    CsipProfileHandle prflHndl;
    CsipStatus        status;
    uint8             sirkValue[CSIP_SIRK_SIZE];
} CsipSirkDecryptCfm;

/*!
    @brief Initialises the Gatt CSIP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt CSIP library.

    @param appTask          The Task that will receive the messages sent from this immediate alert profile library
    @param clientInitParams Initialisation parameters
    @param deviceData       CSIS handles

    NOTE: A CSIP_INIT_CFM with CsipStatus code equal to CSIP_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed CSIP_INIT_CFM will be received with a CsipStatus
          that indicates the result of the initialisation.
*/
void CsipInitReq(AppTask appTask,
                CsipInitData *clientInitParams,
                CsipHandles *deviceData);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the CsipInitReq API.

    @param profileHandle The Profile handle.

    NOTE: Once completed CSIP_DESTROY_CFM will be received with a
          CsipStatus that indicates the result of the destroy.
*/
void CsipDestroyReq(CsipProfileHandle profileHandle);

/*!
    @brief This API is used to write the client characteristic configuration of the Lock
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param csInfoType          The type of Coordinated set characteristics.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: CSIP_CS_SET_NTF_CFM message will be sent to the registered application Task.

*/
void CsipCSRegisterForNotificationReq(CsipProfileHandle profileHandle,
                                            CsipCsInfo csInfoType,
                                              bool notificationsEnable);


/*!
    @brief This API is used to read the Client Configuration Characteristic of the
           SIRK/SIZE/RANK/LOCK characteristic.

    @param profile_handle  The Profile handle.
    @param cs_info_type    The type of Coordinated set characteristics.

    NOTE: A CSIP_READ_CS_INFO_CFM message will be sent to the registered application Task.

*/
void CsipReadCSInfoRequest(CsipProfileHandle profile_handle, CsipCsInfo cs_info_type);


/*!
    @brief This API is used to write the LOCK characteristic in order to execute
           the lock request and release operation.

    @param profile_handle  The Profile handle.
    @param lock_enable     True or False.

    NOTE: A CSIP_SET_LOCK_CFM message will be sent to the registered application Task.
*/
void CsipSetLockRequest(CsipProfileHandle profile_handle,
                                                    bool lock_enable);


/*!
    @brief This API is used to retrieve the CSIS Control point characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param CsipProfileHandle          The CSIS instance handle.

    @return GattCsisClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately to the app.

*/
GattCsisClientDeviceData *CsipGetAttributeHandles(CsipProfileHandle profileHandle);

/*!
    @brief This API is used to get the 6 octet Resolvable Set Identifier(RSI)
           data from advertisement data if present in advertisement data
           passed.

    @param data  Pointer to advertisement data.
    @param dataLen Length of the advertisement data
    @param rsi Pointer to rsi data to be filled. The memory for this pointer needs to be
               allocated by layer which calls it.

    @return TRUE if RSI data is present and is returned in rsi. 
            FALSE if RSI data is not present. rsi is returned as NULL.
*/
bool CsipGetRsiFromAdvData(uint8 *data, uint8 dataLen, uint8 *rsi);

/*!
    @brief This API is used to identify if the RSI can be resolved with SIRK
           passed.

    @param rsi Pointer to 6 octet rsi data
    @param rsiSize Size of RSI Data
    @param sirk SIRK to be used for RSI resolution operation
    @param sirkSize size of SIRK

    @return TRUE if RSI is resolved with SIRK else FALSE.

    Note: This API shall expects plain sirk.
*/
bool CsipIsSetMember(uint8 *rsi,uint8 rsiSize, uint8 *sirk, uint8 sirkSize);

/*!
    @brief This API is used to decrypt SIRK received from remote device
           during CSIS discovery.

    @param profile_handle  The Profile handle.
    @param sirk Pointer to encrypted SIRK

    NOTE: A CSIP_SIRK_DECRYPT_CFM message will be sent to the registered application Task.
*/
void CsipDecryptSirk(CsipProfileHandle profileHandle, const uint8 *sirk);
#endif /* CSIP_H */
