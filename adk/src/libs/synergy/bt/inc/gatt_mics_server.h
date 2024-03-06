/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/
#ifndef GATT_MICS_SERVER_H
#define GATT_MICS_SERVER_H

#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "service_handle.h"
#include "csr_pmem.h"

#define MICS_MAX_CONNECTIONS (3)

/*!
  MICS  ERROR Codes
*/

#define MICS_ERROR_MUTE_DISABLED      (0x80)
#define MICS_ERROR_VALUE_OUT_OF_RANGE (ATT_RESULT_VALUE_NOT_ALLOWED)

/*!
   MICS Mute Characteristic values
*/

#define MICS_SERVER_NOT_MUTED       (0x00)
#define MICS_SERVER_MUTED           (0x01)
#define MICS_SERVER_MUTE_DISABLED   (0x02)

/*!
  MICS message Id's
*/

typedef uint16 GattMicsMessageId;

#define GATT_MICS_SERVER_MIC_STATE_SET_IND  (GattMicsMessageId)(0)
#define GATT_MICS_SERVER_CONFIG_CHANGE_IND  (GattMicsMessageId)(1)

/*!
    GattMicsServerMicStateSetInd
    This message is sent by Microphone Control Service when Remote device changes the state of
    MICS Server the Mute characteristic
*/

typedef struct
{
    GattMicsMessageId id;
    connection_id_t cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    uint8 muteValue;
} GattMicsServerMicStateSetInd;

/*!
    GattMicsServerConfigChangeInd
    This message is sent by Microphone Control Service when Remote device writes the CCCD of
    Mute characteristic
*/

typedef struct
{
    GattMicsMessageId id;
    connection_id_t cid;
    ServiceHandle srvcHndl;
    bool configChangeComplete; /* will be TRUE if all CCCD of MICS are written once */
} GattMicsServerConfigChangeInd;


/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Microphone Control Service
 */
typedef struct
{
    uint16 micsMuteClientCfg:2;
}GattMicsClientConfigDataType;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Media Control Service
*/

typedef struct
{
    connection_id_t              cid;
    GattMicsClientConfigDataType  clientCfg;
}GattMicsClientData;


/*! @brief Init data

	This structure contains data for server initialization
*/
typedef struct
{
    uint8                                 mute;
}GattMicsInitData;

/*!
    @brief This API is used to initialise MICS server

    @param theAppTask Scheduler Id of the UL task.
    @param start_handle Start handle of MICS service in the  DB
    @param end_handle last handle of MICS server in the db
    @param init_data  data to initialize MICS server characteristics

    @return srvc_hndl Instance handle for the service.
*/

ServiceHandle GattMicsServerInit(
                     AppTask theAppTask,
                     uint16  startHandle,
                     uint16  endHandle,
                     const GattMicsInitData* initData);

/*!
    @brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    @param srvcHndl Instance handle for the service.
    @param cid The Connection ID to the peer device.
    @param config Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.

    @return status_t The result of the operation
*/

status_t GattMicsServerAddConfig(ServiceHandle srvcHndl,
                                connection_id_t  cid,
                                GattMicsClientConfigDataType *const config);
/*!
    @brief This API is used when the server application needs to remove a client from connection list

    @param srvcHndl Instance handle for the service.
    @param cid The Connection ID to the peer device.

    @return GattMicsClientConfigDataType Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattMicsClientConfigDataType* GattMicsServerRemoveConfig(ServiceHandle srvcHndl,
                                                         connection_id_t  cid);


/*!
    \brief Gets the configuration for a peer device, identified by its
           Connection ID.

    This gets the configuration for that peer device from the
    service library.
    It is recommnded to call this API after GATT_MICS_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    \param srvcHndl The GATT service instance handle.
    \param cid A Connection ID for the peer device.

    \return GattMicsClientConfigDataType Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattMicsClientConfigDataType* GattMicsServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid);

/*!
    @brief This API is used when the application needs to perform write operation on MICS
	characteristics

    @param srvcHndl Instance handle for the service.
    @param value value of the Mute characteristic

    @return bool. Returns TRUE if the api call was successful or else returns FALSE
*/

bool GattMicsServerSetMicState(ServiceHandle srvcHndl, uint8 value);

/*!
    @brief Application can use this helper function to read the current Microphone state

    @param srvcHndl Instance handle for the service.
    @param value output parameter - current Microphone state, only when the function
                                    returns TRUE. If function returns FALSE then this
                                    value shall be ignored

    @return bool Returns TRUE if the call was successful
*/

bool GattMicsServerReadMicState(ServiceHandle srvcHndl, uint8 *value);

/*!
    @brief This API is used to send response when the application sets MICS server
    state to MUTE/UNMUTE when it receives write indication from remote client device.
    Application cannot reject the request from remote client. If it is not in a
    position to accept the remote device's set mic state indication, then it shall
    set MICS_SERVER_MUTE_DISABLED using GattMicsServerSetMicState().

    @param srvcHndl Instance handle for the service.
    @param cid BtConnId of remote device which is trying to change Mic State

    @return bool. Returns TRUE if the API call was successful or else returns FALSE
*/

bool GattMicsServerSetMicStateRsp(ServiceHandle srvcHndl,
                                 connection_id_t cid);

#endif
