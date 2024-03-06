/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef CHP_SEEKER_H
#define CHP_SEEKER_H

#include "csr_bt_gatt_lib.h"
#include "service_handle.h"

#include "gatt_tds_client.h"

#define CHP_SEEKER_MESSAGE_BASE (SYNERGY_EVENT_BASE + CHP_SEEKER_PRIM)

/*!
    \brief Profile handle type.
*/
typedef ServiceHandle ChpSeekerProfileHandle;

/* Connection ID type */
typedef uint32  ChpSeekerConnectionId;

/*!
    @brief CHP Seeker handles.
*/
typedef struct
{
    GattTdsClientDeviceData *tdsHandle;
} ChpSeekerHandles;

/*!
    @brief CHP Seeker Initialisation parameters.

*/
typedef struct
{
    ChpSeekerConnectionId cid;
} ChpSeekerInitData;

/*!
    \brief CHP Seeker status code type.
*/
typedef uint16 ChpSeekerStatus;

/*! Values for the CHP Seeker status code */
#define CHP_SEEKER_STATUS_SUCCESS                                       (0x0001u)  /*!> Request was a success*/
#define CHP_SEEKER_STATUS_OP_CODE_NOT_SUPPORTED                         (0x0002u)  /*!> Requested opcode is not supported*/
#define CHP_SEEKER_STATUS_INVALID_PARAMETER                             (0x0003u)  /*!> Invalid parameter was supplied*/
#define CHP_SEEKER_STATUS_UNSUPPORTED_ORGANIZATION_ID                   (0x0004u)  /*!> Unsupported Organization id*/
#define CHP_SEEKER_STATUS_OPERATION_FAILED                              (0x0005u)  /*!> Request has failed*/
#define CHP_SEEKER_STATUS_IN_PROGRESS                                   (0x0006u)  /*!> Requested operation in progress*/
#define CHP_SEEKER_STATUS_DISCOVERY_ERR                                 (0x0007u)  /*!> Requestreturned discovery error*/
#define CHP_SEEKER_STATUS_TRUNCATED_DATA                                (0x000Au)  /*!> Send truncated status when characteristic read response is more */
#define CHP_SEEKER_STATUS_ATT_TIMEOUT                                   (0x000Bu)  /*!> Send ATT timeout when operation is timed out */
#define CHP_SEEKER_STATUS_ACCESS_INSUFFICIENT_AUTHENTICATION            (0x000Cu)  /*!> Request returned insufficient authentication */


typedef uint16 ChpSeekerMessageId;

/*! @brief Messages a client task may receive from the CHP Seeker profile library.
 */

#define CHP_SEEKER_INIT_CFM                                         ((ChpSeekerMessageId) 0x01)
#define CHP_SEEKER_DESTROY_CFM                                      ((ChpSeekerMessageId) 0x02)

/* Characteristic Write Confirmation messages */
#define CHP_SEEKER_REGISTER_FOR_TDS_CONTROL_POINT_INDICATION_CFM    ((ChpSeekerMessageId) 0x04)
#define CHP_SEEKER_ACTIVATE_TRANSPORT_CFM                           ((ChpSeekerMessageId) 0x05)

/* Characteristic Read Confirmation messages */
#define CHP_SEEKER_READ_BREDR_HANDOVER_DATA_CFM                     ((ChpSeekerMessageId) 0x06)
#define CHP_SEEKER_READ_BREDR_TRANSPORT_BLOCK_DATA_CFM              ((ChpSeekerMessageId) 0x07)

/* Characteristic Indication messages */
#define CHP_SEEKER_ACTIVATE_TRANSPORT_RSP_IND                       ((ChpSeekerMessageId) 0x08)

/* Timeout value of control point timer */
#define CONTROL_POINT_TIMER_TIMEOUT_SECONDS   10
/*!
    @brief Profile library message sent as a result of calling the ChpSeekerInitReq API.

    This message will send with the value of status of CHP_SEEKER_STATUS_SUCCESS
    with the CHP_SEEKER_INIT_CFM message.
*/
typedef struct
{
    ChpSeekerMessageId      id;              /*! CHP Seeker Message Id*/
    ChpSeekerStatus         status;          /*! Status of the initialisation attempt*/
    ChpSeekerProfileHandle  profileHandle;   /*! CHP Seeker profile handle*/
    ChpSeekerConnectionId   cid;             /*! We need cid to map profile handle later*/
    ServiceHandle           tdsSrvcHandle;   /*! TDS client service handle*/
} ChpSeekerInitCfm;

/*!
    @brief Profile library message sent as a result of calling the ChpSeekerDestroyReq API.

    This message will send with the value of status of CHP_SEEKER_STATUS_SUCCESS
    with the CHP_SEEKER_DESTROY_CFM message.
*/
typedef struct
{
    ChpSeekerMessageId       id;              /*! Chp Seeker Message Id*/
    ChpSeekerProfileHandle   profileHandle;  /*! CHP Seeker profile handle*/
    ChpSeekerStatus          status;        /*! Status of the Chp Seeker Destroy */
} ChpSeekerDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the ChpSeekerRegForIndicationReq API and of the receiving
    of the CHP_SEEKER_REGISTER_FOR_TDS_CONTROL_POINT_INDICATION_CFM message from the gatt tds client library.
*/
typedef struct
{
    ChpSeekerMessageId          id;
    ChpSeekerProfileHandle      profileHandle;        /*! CHP Seeker profile handle*/
    ChpSeekerStatus             status;           /*! Status of the termination attempt*/
} ChpSeekerRegForIndicationCfm;

/*!
    @brief Profile library message sent as a result of calling the ChpSeekerActivateTransportReq API and
           of the receiving of the CHP_SEEKER_ACTIVATE_TRANSPORT_CFM message from gatt tds client library.
*/
typedef struct
{
    ChpSeekerMessageId          id;
    ChpSeekerProfileHandle      profileHandle;        /*! CHP Seeker profile handle*/
    ChpSeekerStatus             status;           /*! Status of the termination attempt*/
} ChpSeekerActivateTransportCfm;


/*!
    @brief Profile library message sent as a result of calling the ChpSeekerReadBredrHandoverDataReq API
           and of the receiving of CHP_SEEKER_READ_BREDR_HANDOVER_DATA_CFM message 
           from the gatt tds client library.
*/

typedef struct
{
    ChpSeekerMessageId      id;
    ChpSeekerProfileHandle  profileHandle;   /*! CHP Seeker profile handle*/
    ChpSeekerStatus         status;      /*! Status of the reading attempt*/
    uint8                   bredrFeatures; /*! Supported Features for BR/EDR Connection Handover */
    CsrBtDeviceAddr         bdAddr; /*! BD_ADDR of the device */
    uint32                  classOfDevice; /*! Class of Device of the Provider */
} ChpSeekerReadBredrHandoverDataCfm;

/*!
    @brief Profile library message sent as a result of calling the ChpSeekerReadBredrTransportBlockDataReq API
           and of the receiving of CHP_SEEKER_READ_BREDR_TRANSPORT_BLOCK_DATA_CFM message 
           from the gatt tds client library.
*/
typedef struct
{
    ChpSeekerMessageId      id;
    ChpSeekerProfileHandle  profileHandle;   /*! CHP Seeker profile handle*/
    ChpSeekerStatus         status;      /*! Status of the reading attempt*/
    uint8                   tdsDataAdType; /*! The value as defined in the Generic Access Profile */
    uint8                   orgId; /*! Organisation ID value from SIG */
    uint8                   tdsFlags; /*! The role of the device and information about its state and supported features */
    uint8                   transportDataLength; /*! Transport Data length will vary from 0x00 – 0xEF */
    uint8                   *transportData; /*! Transport Data which is in LTV format as defined in spec version v1.0 */
} ChpSeekerReadBredrTransportBlockDataCfm;


/*!
    @brief Profile library message sent as a result of calling the ChpSeekerActivateTransportReq API.
           and of the receiving of CHP_SEEKER_ACTIVATE_TRANSPORT_RSP_IND message 
           from the gatt tds client library.
*/
typedef struct
{
    ChpSeekerMessageId          id;
    ChpSeekerProfileHandle      profileHandle;        /*! CHP Seeker profile handle*/
    ChpSeekerStatus             status;           /*! Status of the termination attempt*/
    uint16                      responseDataLen;
    uint8                       *responseData;
} ChpSeekerActivateTransportRspInd;

/*!
    @brief Initialises the CHP Seeker Library.

    @param apptask           The Task that will receive the messages sent from this CHP profile library
    @param clientInitParams  Initialisation parameters
    @param deviceData        CHP Seeker handles

    NOTE: A CHP_SEEKER_INIT_CFM with ChpStatus code equal to CHP_SEEKER_STATUS_SUCCESS will be received
          that indicates the result of the initialisation.
*/
void ChpSeekerInitReq(AppTask appTask,
                      ChpSeekerInitData *clientInitParams,
                      ChpSeekerHandles *deviceData);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the ChpSeekerInitReq API.

    @param profileHandle The Profile handle.

    NOTE: Once completed CHP_SEEKER_DESTROY_CFM will be received with a
          ChpStatus that indicates the result of the destroy.
*/
void ChpSeekerDestroyReq (ChpSeekerProfileHandle profileHandle);

/*!
    @brief This API is used to enable indications at the server for the TDS Control Point characteristics.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param indicationsEnable   Set to TRUE to enable indications on the server, FALSE to disable them.

    NOTE: CHP_SEEKER_REGISTER_FOR_TDS_CONTROL_POINT_INDICATION_CFM message will be sent to the registered application Task.

*/
void ChpSeekerRegForIndicationReq (ChpSeekerProfileHandle profileHandle,
                                   bool indicationsEnable);

/*!
    @brief This API is used to write the TDS Control Point characteristic in order request a desired opcode.
    
    @param profileHandle        The Profile handle.
    @param opCode               The Tds Client Control Point Opcode for Activate Transport (0x01).
    @param orgId                The Organization ID value for Bluetooth SIG (0x01).
    @param profileDataLength    The LTV profile data length.
    @param ltvData              The LTV profile data structure as defined in the CHP Seeker Role Specification.

    NOTE: A CHP_SEEKER_ACTIVATE_TRANSPORT_CFM message will be sent to the registered application Task.
*/
void ChpSeekerActivateTransportReq(ChpSeekerProfileHandle profileHandle, 
                                    uint8 opCode,
                                    uint8 orgId,
                                    uint16 profileDataLength,
                                    uint8* ltvData);

/*!
    @brief This API is used to read the BR/EDR Handover Data characteristic to access information
    that will assist with a low latency connection to the BR/EDR transport

    @param profileHandle  The Profile handle.

    NOTE: A CHP_SEEKER_READ_BREDR_HANDOVER_DATA_CFM message will be sent to the registered application Task.

*/
void ChpSeekerReadBredrHandoverDataReq(ChpSeekerProfileHandle profileHandle);


/*!
    @brief This API is used to read the Complete BR/EDR Transport Block Data descriptor 
           to access the value of the Transport Block for the case where the 
           Seeker and Provider are already connected over the Bluetooth LE transport.

    @param profile_handle  The Profile handle.

    NOTE: A CHP_SEEKER_READ_BREDR_TRANSPORT_BLOCK_DATA_CFM message will be sent to the registered application Task.
*/
void ChpSeekerReadBredrTransportBlockDataReq(ChpSeekerProfileHandle profileHandle);

#endif /* CHP_SEEKER_H */


