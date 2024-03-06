/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #59 $
******************************************************************************/

/*!
@file    gatt_transport_discovery_server.h
@brief   Header file for the GATT Transport Discovery Service library.

        This file provides documentation for the GATT Transport Discovery Service library
        API (library name: gatt_transport_discovery_server).
*/

#ifndef GATT_TRANSPORT_DISCOVERY_SERVER_H
#define GATT_TRANSPORT_DISCOVERY_SERVER_H


#include "csr_bt_types.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"

/* Transport Discovery service AD type as defined by SIG */
#define TRANSPORT_DISCOVERY_SERVICE_AD_TYPE             (0x26)

#define activate_transport 0x01
#define SIG_org_ID 0x01

#define GATT_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA_VALUE_SIZE       (sizeof(uint8) * 10)

/* All current result codes. */
#define op_code_success             0x00
#define op_code_not_supported       0x01
#define op_code_invalid_parameter   0x02
#define op_code_unsupported_org_id  0x03
#define op_code_operation_failed    0x04

/*! @brief The set of messages an application task can receive from the TDS library.
 */
typedef uint16 GattTdsServerMessageId;

#define GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND              (GattTdsServerMessageId)(0)
#define GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND             (GattTdsServerMessageId)(1)
#define GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND             (GattTdsServerMessageId)(2)
#define GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND        (GattTdsServerMessageId)(3)
#define GATT_TRANSPORT_DISCOVERY_SERVER_SIG_DATA_IND                        (GattTdsServerMessageId)(4)
#define GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND (GattTdsServerMessageId)(5)


/*! @brief Transport Discovery Server library  data structure type .
 */

/* This structure is made public to application as application is responsible for managing resources 
 * for the elements of this structure. The data elements are indented to use by Transport Discovery Server lib only. 
 * Application SHOULD NOT access (read/write) any elements of this library structure at any point of time and doing so  
 * may cause undesired behavior of this library's functionalities.
 */

typedef struct GTDS_T
{
    AppTaskData         lib_task;
    AppTask             app_task;
    CsrBtGattId         gattId;
} GTDS_T;


/*! @brief Contents of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND message that is sent by the library,
    due to a read of the TDS client configuration characteristic.
 */
typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND
{
    GattTdsServerMessageId id; /*! Message id  */
    const GTDS_T *tds;      /*! Reference structure for the instance  */
    connection_id_t cid;             /*! Connection ID */
} GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND message that is sent by the library,
    due to a write of the TDS client configuration characteristic.
 */
typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND
{
    GattTdsServerMessageId id; /*! Message id  */
    const GTDS_T *tds;      /*! Reference structure for the instance  */
    connection_id_t cid;             /*! Connection ID */
    uint16 config_value;    /*! Client Configuration value to be written */
} GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND message that is sent by the library,
    due to a write of the TDS control point characteristic.
 */
typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND
{
    GattTdsServerMessageId id; /*! Message id  */
    const GTDS_T *tds;      /*! Reference structure for the instance  */
    connection_id_t cid;             /*! Connection ID */
    uint16 handle;
    uint16 size_value;      /*! Size of value to be written */
    /*!
    *
    * The memory allocated to store the value in LTV structure format is
    * one contiguous block of memory that is large enough to store the appropriate number of LTVs.
    * The structure of the LTV Data can be seen in the Transport Discovery Service spec version 1.1
    */
    uint8 value[1];
} GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND_T;

/*! @brief Contents of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND 
    message that is sent by the library, due to a read of the TDS client
    configuration characteristic.
 */
typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND
{
    GattTdsServerMessageId id; /*! Message id  */
    const GTDS_T *tds;      /*! Reference structure for the instance  */
    connection_id_t cid;             /*! Connection ID */
} GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND_T;


typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_RSP
{
    uint8_t bredr_features; /*! Supported Features for BR/EDR Connection Handover */
    CsrBtDeviceAddr bd_addr; /*! BD_ADDR of the device */
    uint24_t class_of_device; /*! Class of Device of the Provider */
} GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_RSP_T;

/*! @brief Contents of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND 
    message that is sent by the library, due to a read of the TDS client
    configuration characteristic.
 */
typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND
{
    GattTdsServerMessageId id; /*! Message id  */
    const GTDS_T *tds;      /*! Reference structure for the instance  */
    connection_id_t cid;             /*! Connection ID */
} GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND_T;


typedef struct __GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_RSP
{
    uint8_t tds_data_ad_type; /*! The value as defined in the Generic Access Profile */
    uint8_t org_id; /*! Organisation ID value from SIG */
    uint8_t tds_flags; /*! The role of the device and information about its state and supported features */
    uint8_t transport_data_length; /*! Transport Data length will vary from 0x00 � 0xEF */
    uint8_t *transport_data; /*! Transport Data */
} GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_RSP_T;


/*!
    @brief Initializes the Transport Discovery Service Library.

    @param appTask The Task that will receive the messages sent from this TDS library.
    @param tds A valid area of memory that the TDS library can use.Must be of at least the size of GTDS_T
    @param startHandle This indicates the start handle of the service
    @param endHandle This indicates the end handle of the service
    
    @return TRUE if success, FALSE otherwise.

*/
bool GattTransportDiscoveryServerInit(AppTask appTask,
                                    GTDS_T *const tds,
                                    uint16 startHandle,
                                    uint16 endHandle
                                    );

/*!
    @brief This API is used to return a TDS client configuration value to the library when a 
    GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND message is received.

    @param tds The pointer that was in the payload of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param cid The connection identifier from the GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param client_config The client configuration to return to the library
    
    @return TRUE if success, FALSE otherwise

*/
bool GattTdsServerReadClientConfigResponse(const GTDS_T *tds, connection_id_t cid, uint16 client_config);


/*!
    @brief  This API is used to send an indication to a remote Transport 
            Discovery client regarding the write to the Control Point. This will  
            only be allowed if notifications have been enabled by the remote device.

    @param tds              The instance pointer that was passed into the 
                            GattTransportDiscoveryServerInit API.
                        
    @param cid              The connection identifier from the 
                            GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND message.
                        
    @param tds_ind_size     The length of the Transport Discovery indication
    
    @param tds_ind_data     The Transport Discovery indication itself
    
    @return TRUE if success, FALSE otherwise

*/
bool GattTdsServerSendNotification(const GTDS_T *tds, connection_id_t cid, uint16 tds_ind_size, uint8 *tds_ind_data);


/*!
    @brief Sends write response to the write request on Tranport Discovery Service attributes.

    @param ind           Pointer to Write control point indication event to appication needs to respond
    @param result        The response code to be sent
    
    @return Nothing

*/                                   
void GattTdsServerSendResponse(const GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND_T *ind, uint16 result);

/*!
    @brief This API is used to return the TDS Bredr Handover Data characteristics 
    value to the library when a GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND message is received.

    @param tds The pointer that was in the payload of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND message.
    @param cid The connection identifier from the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND message.
    @param bredr_features The Supported Features for BR/EDR Connection Handover
    @param bd_addr The BD_ADDR of the device 
    @param class_of_device The Class of Device of the Provider
    
    @return TRUE if success, FALSE otherwise

*/
bool GattTdsServerReadBredrHandoverDataResponse(const GTDS_T *tds, 
                                                connection_id_t cid,
                                                uint8_t bredr_features,
                                                CsrBtDeviceAddr bd_addr,
                                                uint24_t class_of_device);

/*!
    @brief This API is used to return the TDS characteristics Transport Block Data 
    value to the library when a GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND message is received.

    @param tds The pointer that was in the payload of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND message.
    @param cid The connection identifier from the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND message.
    @param size_value The total size of the GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_RSP
    @param value Pointer to the values stored of GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_RSP
    
    @return TRUE if success, FALSE otherwise

*/
bool GattTdsServerReadBredrTransportBlockDataResponse(const GTDS_T *tds, 
                                                      connection_id_t cid,
                                                      uint16_t size_value,
                                                      uint8_t *value);

#endif /* GATT_TRANSPORT_DISCOVERY_SERVER_H */

