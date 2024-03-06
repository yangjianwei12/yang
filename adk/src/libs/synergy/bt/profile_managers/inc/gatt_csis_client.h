/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file     gatt_csis_client.h
@brief   Header file for the GATT CSIS Client library.

        This file provides documentation for the GATT CSIS Client library
        API (library name: gatt_csis_client).
*/

#ifndef GATT_CSIS_CLIENT_H
#define GATT_CSIS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"
#include "csr_bt_gatt_prim.h"

#define GATT_CSIS_CLIENT_MESSAGE_BASE 0x00

/*!
    @brief persistent data for each known Cordinated Set Member device.

    Each Coordinated Set Member device that is bonded can have data associated against
    it so that re-connections are much faster in that case no GATT discovery is required.
*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 csisSirkHandle;            /*! Coordinated Set Member SIRK Handle */
    uint16 csisLockCcdHandle;         /*! Coordinated Set Member Lock CC Handle, if supported by member  */
    uint16 csisSizeCcdHandle;         /*! Coordinated Set Member Size CC Handle, if supported by member */
    uint16 csisSirkCcdHandle;         /*! Coordinated Set Member SIRK CC Handle, if supported by member */
    uint16 csisSizeHandle;            /*! Coordinated Set Member SIZE Handle,  if supported by member  */
    uint16 csisLockHandle;            /*! Coordinated Set Member Lock Handle,  if supported by member  */
    uint16 csisRankHandle;            /*! Coordinated Set Member Rank Handle, if supported by member */
} GattCsisClientDeviceData;

typedef uint16 GattCsisClientCsInfo ;

#define GATT_CSIS_CLIENT_SIRK (GattCsisClientCsInfo)0x0000
#define GATT_CSIS_CLIENT_SIZE (GattCsisClientCsInfo)0x0001
#define GATT_CSIS_CLIENT_RANK (GattCsisClientCsInfo)0x0002
#define GATT_CSIS_CLIENT_LOCK (GattCsisClientCsInfo)0x0003

/*! @brief Defines of messages a client task may receive from the CSIS Client library.
 */
typedef uint16 GattCsisMessageId;

#define GATT_CSIS_CLIENT_INIT_CFM           (GATT_CSIS_CLIENT_MESSAGE_BASE)             /*! Confirmation for Init */
#define GATT_CSIS_CLIENT_TERMINATE_CFM      (GATT_CSIS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_CSIS_CLIENT_NOTIFICATION_CFM   (GATT_CSIS_CLIENT_MESSAGE_BASE + 0x0002u)   /*! Confirmation for Notification */
#define GATT_CSIS_CLIENT_NOTIFICATION_IND   (GATT_CSIS_CLIENT_MESSAGE_BASE + 0x0003u)   /*! Indication for Lock Release/Unrelease */
#define GATT_CSIS_CLIENT_READ_CS_INFO_CFM   (GATT_CSIS_CLIENT_MESSAGE_BASE + 0x0004u)
#define GATT_CSIS_CLIENT_WRITE_LOCK_CFM     (GATT_CSIS_CLIENT_MESSAGE_BASE + 0x0005u)

/*!
    @brief Defines for CSIS client status code
*/
typedef uint16 GattCsisClientStatus ;

#define GATT_CSIS_CLIENT_STATUS_SUCCESS         (GattCsisClientStatus)0x0000        /* Request was success */
#define GATT_CSIS_CLIENT_STATUS_NOT_ALLOWED     (GattCsisClientStatus)0x0001     /* Request is not allowed at the moment, something went wrong internally  */
#define GATT_CSIS_CLIENT_STATUS_NO_CONNECTION   (GattCsisClientStatus)0x0002  /* There is no GATT connection exists for given CID so that service library can issue a request to remote device */
#define GATT_CSIS_CLIENT_STATUS_FAILED          (GattCsisClientStatus)0x0003         /* Request has been failed */
/* Spec specific errors */
#define GATT_CSIS_CLIENT_LOCK_DENIED                (GattCsisClientStatus)0x0080
#define GATT_CSIS_CLIENT_LOCK_RELEASE_NOT_ALLOWED   (GattCsisClientStatus)0x0081
#define GATT_CSIS_CLIENT_INVALID_LOCAL_VALUE        (GattCsisClientStatus)0x0082
#define GATT_CSIS_CLIENT_OOB_SIRK_ONLY              (GattCsisClientStatus)0x0083
#define GATT_CSIS_CLIENT_LOCK_ALREADY_GRANTED       (GattCsisClientStatus)0x0084


/* Lock characteristics values */
#define UNLOCKED_VALUE                     (0x01)
#define LOCKED_VALUE                       (0x02)

/* CSIS Size characteristics not supported */
#define CSIS_SIZE_NOT_SUPPORTED  (0x00)

/* CSIS Lock characteristics not supported */
#define CSIS_LOCK_NOT_SUPPORTED  (0x00)

/*!
    @brief Parameters used by the Initialisation API, valid value of these
     parameters are must for library initialisation  
*/
typedef struct
{
     connection_id_t cid; /*! Connection ID of the GATT connection on which
                               the server side CSIS need to be accessed*/
     uint16 startHandle;  /*! The first handle of CSIS need to be accessed*/
     uint16 endHandle;    /*! The last handle of CSIS need to be accessed */
} GattCsisClientInitParams;



/*!@brief CSIS Client Library initialisation confirmation 
*/
typedef struct
{
    GattCsisMessageId id;         /*! service message id */
    ServiceHandle srvcHndl;       /*! Reference structure for the instance */
    uint16  sizeSupport;          /*! CSIS Size characteristics, if supported by server*/
    uint16  lockSupport;          /*! CSIS Lock characteristics, if supported by server*/
    connection_id_t  cid;         /*! Connection ID */
    GattCsisClientStatus status;  /*! Status as per GattCsisClientStatus */
} GattCsisClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattCsisClientTerminateReq API.
*/
typedef struct
{
    GattCsisMessageId    id;                 /*! service message id */
    connection_id_t             cid;
    GattCsisClientStatus        status;      /*! Status of the initialisation attempt */
    ServiceHandle               srvcHndl;    /*! Reference handle for the instance */
    GattCsisClientDeviceData    deviceData;  /*! Device data: handles used for the peer device. */
} GattCsisClientTerminateCfm;

/*!
    @brief CSIS Library notification registration confirmation
             This confirmation will be received on Enabling/Disabling notification 
*/
typedef struct
{
    GattCsisMessageId    id;           /*! service message id */
    ServiceHandle           srvcHndl;  /*! Reference structure for the instance */
    GattCsisClientCsInfo    csInfo;    /*! cs_info requested */
    GattCsisClientStatus    status;    /*! status as per GattCsisClientStatus */
} GattCsisClientNotificationCfm;

/*!
    @brief CSIS Client Library notification indication, This indication will be received on each notification from CSIS service
*/
typedef struct
{
    GattCsisMessageId    id;            /*! service message id */
    ServiceHandle           srvcHndl;   /*! Reference structure for the instance */
    GattCsisClientCsInfo    csInfo;     /*! cs_info requested */
    uint16                  sizeValue;  /*! Size of data in buffer 'value' */
    uint8                   value[1];   /*! Data buffer containing CS informtaion */
} GattCsisClientNotificationInd;

/*!
    @brief CSIS Client library read information confirmation
             This confirmation will be received with CS info
*/
typedef struct
{
    GattCsisMessageId    id;        /*! service message id */
    ServiceHandle        srvcHndl;  /*! Reference structure for the instance */
    GattCsisClientStatus status;    /*! status as per GattCsisClientStatus */
    GattCsisClientCsInfo csInfo;    /*! cs_info requested */
    uint16               sizeValue; /*! Size of data in buffer 'value' */
    uint8                value[1];  /*! Data buffer containing CS informtaion */
}GattCsisClientReadCsInfoCfm;

/*!
    @brief CSIS Client Library confirmation for write of Lock characteristics
           This confirmation will be received on write of Lock  request by application
*/
typedef struct
{
    GattCsisMessageId    id;          /*! service message id */
    ServiceHandle        srvcHndl;    /*! Reference structure for the instance */
    GattCsisClientStatus status;      /*! status as per GattCsisClientStatus */
} GattCsisClientWriteLockCfm;


/*!
    @brief Initialises the CSIS Client Library.

    Initialize CSIS client library handles, It starts finding out the 
    characteristic handles of CSIS.

    Once the initialisation has been completed, GATT_CSIS_CLIENT_INIT_CFM will
    be received with status as enumerated as GattCsisClientStatus.
    '' has to  be considered initialisation of
    library is done successfully and all the required charecteristics has
    been found out

    NOTE:This interface need to be invoked for every new gatt connection when
    the client wish to use csis client library  

    @param appTask The client task that will receive messages from this Service.
    @param clientInitParams Configuration data for client initialisation.
           The memory allocated for GattCsisClientInitParams must be freed
           once the API returns.
    @param deviceData Cached handles/data from previous connection with Peer Device OR
                       NULL if this is the first time this peer device has connected.

    NOTE: GATT_CSIS_CLIENT_INIT_CFM will be received with a 
          GattCsisClientStatus status code..
*/
void GattCsisClientInit(
           AppTask appTask,
           const GattCsisClientInitParams *const clientInitParams,
           const GattCsisClientDeviceData *deviceData);


/*!
    \brief GATT CSIS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    @param srvcHndl    The service handle for this GATT Client Service.

    NOTE: GATT_CSIS_CLIENT_TERMINATE_CFM will be received with a 
    GattCsisClientStatus status code.
*/
void GattCsisClientTerminate(ServiceHandle srvcHndl);


/*!
    @brief Register for notification from peer.

    @param srvcHndl    The service handle for this GATT CSIS Client Service.
    @param csInfo      register csis server member to get notified
    @param enable       Enable or Disable notification.
*/
void GattCsisRegisterForNotification(ServiceHandle srvcHndl, GattCsisClientCsInfo csInfo, bool enable);


/*!
    @brief Read Coordinated Set characteristics value from peer.

    @param srvcHndl    The service handle for this GATT CSIS Client Service.
    @param csInfo      csis server member value to read
*/
void GattCsisReadCsInfoReq(ServiceHandle srvcHndl, GattCsisClientCsInfo csInfo);

/*!
    @brief Write Lock characteristics on peer device.

    @param srvcHndl    The service handle for this GATT CSIS Client Service.
    @param enableLock Enable or Release Lock of Coordinated Set Member
*/
void GattCsisWriteLockReq(ServiceHandle srvcHndl, bool enableLock);

/*!
    @brief This API is used to retrieve the CSIS characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattCsisClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.

*/
GattCsisClientDeviceData *GattCsisClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_CSIS_CLIENT_H */

