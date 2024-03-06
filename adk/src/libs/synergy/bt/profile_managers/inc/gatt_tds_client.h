/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    gatt_tds_client.h
    
DESCRIPTION
    Header file for the GATT TDS Client library.
*/


/*!
@file    gatt_tds_client.h
@brief   Header file for the GATT TDS Client library.

        This file provides documentation for the GATT TDS Client library
        API (library name: gatt_tds_client).
*/

#ifndef GATT_TDS_CLIENT_H
#define GATT_TDS_CLIENT_H

#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "service_handle.h"
/*!
    @brief Handles of the TDS characteristics.

*/
typedef struct
{
    uint16 startHandle;
    uint16 endHandle;

    uint16 tdsControlPointHandle;
    uint16 tdsControlPointCCCDHandle;
    uint16 bredrHandoverDataHandle;
    uint16 bluetoothSigDataHandle;
    uint16 CompleteTransportBlockHandle;
} GattTdsClientDeviceData;

/*! @brief Enumeration of messages a client task may receive from the client library.
 */

typedef uint8                                          GattTdsClientMessageId;

#define GATT_TDS_CLIENT_INIT_CFM                               ((GattTdsClientMessageId) 0x01)
#define GATT_TDS_CLIENT_TERMINATE_CFM                          ((GattTdsClientMessageId) 0x02)
#define GATT_TDS_CLIENT_INDICATION_CFM                         ((GattTdsClientMessageId) 0x03)
#define GATT_TDS_SET_TDS_CONTROL_POINT_CFM                     ((GattTdsClientMessageId) 0x04)
#define GATT_TDS_CLIENT_GET_TDS_ATTRIBUTE_CFM                  ((GattTdsClientMessageId) 0x05)
#define GATT_TDS_CLIENT_CONTROL_POINT_IND                      ((GattTdsClientMessageId) 0x06)


/*!
    \brief GATT TDS Client status code type.
*/

typedef uint16                                          GattTdsClientStatus;

#define GATT_TDS_CLIENT_STATUS_SUCCESS                                  ((GattTdsClientStatus) 0x01)  /*!> Request was a success*/
#define GATT_TDS_CLIENT_NOT_SUPPORTED                                   ((GattTdsClientStatus) 0x02) /*!>  Not supported by remote device*/
#define GATT_TDS_CLIENT_STATUS_TDS_INACTIVE                             ((GattTdsClientStatus) 0x03) /*!>  TDS service in remote device is inactive*/
#define GATT_TDS_CLIENT_STATUS_COMMAND_INCOMPLETE                       ((GattTdsClientStatus) 0x04)  /*!> Command requested could not be completed*/
#define GATT_TDS_CLIENT_STATUS_DISCOVERY_ERR                            ((GattTdsClientStatus) 0x05)  /*!> Error in discovery of one of the services*/
#define GATT_TDS_CLIENT_STATUS_FAILED                                   ((GattTdsClientStatus) 0x06)  /*!> Request has failed*/
#define GATT_TDS_CLIENT_STATUS_BUSY                                     ((GattTdsClientStatus) 0x07)  /*!> Register for Indication req pending*/
#define GATT_TDS_CLIENT_STATUS_INVALID_PARAMETER                        ((GattTdsClientStatus) 0x08)  /*!> Invalid parameter was supplied*/
#define GATT_TDS_CLIENT_STATUS_CHAR_NOT_SUPPORTED                       ((GattTdsClientStatus) 0x09)  /*!> Characteristics not supported by remote */
#define GATT_TDS_CLIENT_STATUS_TRUNCATED_DATA                           ((GattTdsClientStatus) 0x0A)  /*!> Send truncated status when characteristic read response is more */
#define GATT_TDS_CLIENT_STATUS_ATT_TIMEOUT                              ((GattTdsClientStatus) 0x0B)  /*!> Send ATT timeout when operation is timed out */
#define GATT_TDS_CLIENT_STATUS_ACCESS_INSUFFICIENT_AUTHENTICATION       ((GattTdsClientStatus) 0x0C)  /*!> Request returned insufficient authentication */

typedef uint8                                   TdsCharAttribute;                    /* Combination of following values to be used */
#define BREDR_HANDOVER_DATA                     ((TdsCharAttribute) 0x01)
#define COMPLETET_TRANSPORT_BLOCK               ((TdsCharAttribute) 0x02)

typedef uint8                                           GattTdsOpcode;
#define GATT_TDS_CLIENT_ACTIVATE                        ((GattTdsOpcode) 0x01)
#define GATT_TDS_CLIENT_DEACTIVATE                      ((GattTdsOpcode) 0x02)

typedef uint8                                           GattTdsOpResult;

#define GATT_TDS_OP_RESULT_SUCCESS                      ((GattTdsOpResult) 0x00)
#define GATT_TDS_OP_RESULT_NOT_SUPPORTED                ((GattTdsOpResult) 0x01)
#define GATT_TDS_OP_RESULT_INVALID_PARAMETER            ((GattTdsOpResult) 0x02)
#define GATT_TDS_OP_RESULT_ORG_ID_NOT_SUPPORTED         ((GattTdsOpResult) 0x03)
#define GATT_TDS_OP_RESULT_FAILED                       ((GattTdsOpResult) 0x04)

/*!
    @brief Parameters used by the Initialisation API
*/
typedef struct
{
    uint32 cid;       /*! Connection ID. */
    uint16 startHandle;       /*! The first handle of the service that needs to be accessed */
    uint16 endHandle;         /*! The last handle of the service that needs to be accessed */
} GattTdsClientInitData;

/*!
    @brief Gatt client library message sent as a result of calling the GattTdsClientInitReq API.
*/
typedef struct
{
    GattTdsClientMessageId id;
    uint32                 cid;         /*! Connection ID. */
    ServiceHandle          srvcHndl;   /*! Reference handle for the instance */
    GattTdsClientStatus    status;      /*! Status of the initialisation attempt */
} GattTdsClientInitCfm;

/*!
    @brief Gatt client library message sent as a result of calling the GattTdsClientTerminateReq API.
*/
typedef struct
{
    GattTdsClientMessageId id;
    ServiceHandle          srvcHndl;   /*! Reference handle for the instance */
    GattTdsClientStatus       status;      /*! Status of the initialisation attempt */
    GattTdsClientDeviceData   deviceData; /*! Device data: handles used for the peer device. */
} GattTdsClientTerminateCfm;


/*! @brief Contents of the GATT_TDS_CLIENT_INDICATION_CFM_T message that is sent by the library,
    as a result of setting notifications on the server for selected characteristics.
 */
typedef struct
{
    GattTdsClientMessageId id;
    ServiceHandle         srvcHndl;   /*! Reference handle for the instance */
} GattTdsClientIndicationCfm;

/*! @brief Contents of the GATT_TDS_READ_CHARAC_VALUE_CFM message that is sent by the library,
    as a result of reading of a requested characteristic on the server.
 */
typedef struct
{
    GattTdsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    TdsCharAttribute     charac;       /* Characteristic name */
    status_t            status;      /*! Status of the reading attempt */
    uint16              sizeValue;  /*! Value size*/
    uint8               *value;    /*! Read value */
} GattTdsClientGetTdsAttributeCfm;

/*! @brief Contents of the GATT_TDS_CHARAC_VALUE_IND message that is sent by the library,
    as a result of a notification of a characteristic.
 */
typedef struct
{
    GattTdsClientMessageId id;
    ServiceHandle  srvcHndl;    /*! Reference handle for the instance */
    uint16            sizeValue;  /*! Value size*/
    uint8             *value;    /*! Read value */
} GattTdsClientTdsCPAttributeInd;

/*! @brief Contents of the GATT_TDS_SET_TDS_CONTROL_POINT_CFM message that is sent by the library,
    as a result of writing on Media Control Point characteristic on the server.
 */
typedef struct
{
    GattTdsClientMessageId id;
    ServiceHandle  srvcHndl;   /*! Reference handle for the instance */
    GattTdsClientStatus   status;      /*! Status of the writing attempt */
} GattTdsClientSetTdsControlPointCfm;


/*!
    @brief GATT TDS Client Service Initialisation Request.

    @param theAppTask   The client task that will receive messages from this Service.
    \param initData     Configuration data for client initialisation.
    \param deviceData   Cached handles/data from previous connection with Peer Device OR
                        NULL if this is the first time this peer device has connected.

    NOTE: GATT_TDS_CLIENT_INIT_CFM will be received with a GattTdsClientStatus status code.
*/
void GattTdsClientInitReq(AppTask theAppTask,
                          const GattTdsClientInitData   *initData,
                          const GattTdsClientDeviceData *deviceData);


/*!
    \brief GATT TDS Client Service Termination.

    Calling this function will free all resources for this Client Service.

    \param clntHndl    The service handle for this GATT Client Service.

    NOTE: GATT_TDS_CLIENT_TERMINATE_CFM will be received with a GattTdsClientStatus status code.
*/
void GattTdsClientTerminateReq(ServiceHandle clntHndl);

/*!
    @brief This API is used to write the client characteristic configuration of TDS related
    Control point characteristics on a remote device, to enable indication with the server.
    An error will be returned if the server does not support notifications.

    @param clntHndl            The service handle for this GATT Client Service.
    @param indicValue          Bitmask to enable/disable respective characteristics CCCD

    NOTE: A GATT_TDS_CLIENT_INDICATION_CFM message will be sent to the registered application Task.

*/
void  GattTdsClientRegisterForIndicationReq(ServiceHandle clntHndl, uint32 indicValue);

/*!
    @brief This API is used to read the value of requested characteristic.

    @param clntHndl  The service handle for this GATT Client Service.
    @param charac     Characteristic whose value has to be read.

    NOTE: A GATT_TDS_CLIENT_GET_TDS_ATTRIBUTE_CFM message will be sent to the registered application Task.

*/
void GattTdsClientGetTdsAttribute(ServiceHandle clntHndl, TdsCharAttribute charac);

/*!
    @brief This API is used to write the TDS Control point characteristic in order to execute
           the opcode related operation.

    @param clntHndl      The service handle for this GATT Client Service.
    @param op             Opcode selected.
    @param len            Length of the value.
    @param val            Value of parameter to be sent along the write req.

    NOTE: A GATT_TDS_SET_TDS_CONTROL_POINT_CFM message will be sent to the registered application Task.

*/
void GattTdsClientSetTdsControlPoint(ServiceHandle clntHndl, GattTdsOpcode op,uint16 len, uint8* val);

/*!
    @brief This API is used to retrieve the TDS Control point characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattTdsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.

*/
GattTdsClientDeviceData *GattTdsClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_TDS_CLIENT_H */

