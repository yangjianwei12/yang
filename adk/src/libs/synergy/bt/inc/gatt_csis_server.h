/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*!
@file    gatt_csis_server.h
@brief   Header file for the GATT Coordinated Set Identification Service library.

        This file provides documentation for the GATT CSIS library
        API (library name: gatt_csis_server).
*/

#ifndef GATT_CSIS_SERVER_H
#define GATT_CSIS_SERVER_H

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"

typedef ServiceHandle CsisServerServiceHandleType;
typedef connection_id_t  CsisServerConnectionIdType;
typedef status_t    CsisServerGattStatusType;

#define SIZE_SIRK_KEY       (16*sizeof(uint8))

/* Maximum number of GATT connections */
#define GATT_CSIS_MAX_CONNECTIONS (6)

/* Below flag value can be set at a time if application
 * wants a change in behaviour of SIRK to be shared with
 * remote client.
 * GATT_CSIS_SIRK_UPDATE flag can be combined with
 * other flag(only one of the other 4 needs to be set
 * at a time)
 */
#define GATT_CSIS_SHARE_SIRK_ENCRYPTED         (0x01)
#define GATT_CSIS_SHARE_SIRK_PLAIN_TEXT        (0x02)
#define GATT_CSIS_SHARE_SIRK_OOB_ONLY          (0x04)

 /* When set, this flag will take new SIRK value
  * passed by sirk[SIZE_SIRK_KEY]
  * This flag shall be ignored in GattCsisServerInit()
  * and need not be set as first time SIRK value is taken
  * as it is.
  */
#define GATT_CSIS_SIRK_UPDATE                  (0x10)
  /*! @brief   messages an application task can receive from the
      CSIS library.
   */

typedef uint8 GattCsisServerMessageIdType;

#define GATT_CSIS_LOCK_STATE_IND ((GattCsisServerMessageIdType)0x00)
#define GATT_CSIS_SERVER_CONFIG_CHANGE_IND  ((GattCsisServerMessageIdType)0x01)
/*!
    @brief Parameters used by the Initialisation API

    Parameters that can define how the csis server library is initialised.
 */
typedef struct
{
    uint8  rank;                /*! Rank value to be associated with coordinated set member */
    uint8  csSize;             /*! Value of the Coordinated Set containing this set member */
    uint8  sirk[SIZE_SIRK_KEY]; /*! Shared IRK (SIRK) for this set member */
    uint8  flags;               /*! Operation on SIRK. See flags value above */
} GattCsisServerInitParamType;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Coordinated Set Identification Service
 */

typedef struct
{
    uint16    lockValueClientCfg;
    uint16    sirkValueClientCfg;
    uint16    sizeValueClientCfg;
} GattCsisServerConfigType;

typedef uint8 GattCsisServerLocktype;

#define CSIS_SERVER_UNLOCKED              ((GattCsisServerLocktype)0x01)
#define CSIS_SERVER_LOCKED                ((GattCsisServerLocktype)0x02)
#define CSIS_SERVER_INVALID_LOCK_VALUE    ((GattCsisServerLocktype)0xFF)

/*! @brief Contents of the GATT_CSIS_LOCK_STATE_IND message that is sent by the library,
    due to a change of the local Lock Value characteristic by the client or after the expiry
    of lock_timeout(TCSIS) set by server
 */
typedef struct
{
    GattCsisServerMessageIdType id;
    CsisServerServiceHandleType          csisHandle;
    CsisServerConnectionIdType           cid;
    GattCsisServerLocktype               lockValue;
} GattCsisLockStateInd;


/*! @brief Contents of the GATT_CSIS_SERVER_CONFIG_CHANGE_IND message that is sent by the library,
    when any CCCD char is toggled by the client.
 */
typedef struct 
{
    GattCsisServerMessageIdType id;
    CsisServerServiceHandleType          csisHandle;
    CsisServerConnectionIdType           cid;
    bool                                 configChangeComplete;  /* will be TRUE if all CCCD of 
                                                                   CSIS are written once */
} GattCsisServerConfigChangeInd;

/*!
    @brief Instantiate the Coordinated Set Identification service Library.

    The GattCsisServerInit function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param appTask The Task that will receive the messages sent from this
                   Coordinated Set Identification service Library.
    @param initParams CSIS information server Database paramters for this instance
    @param startHandle This indicates the start handle of the CSIS service
    @param endHandle This indicates the end handle of the CSIS service

    @return CsisServerServiceHandleType If the service handle returned is 0, this indicates
    a failure during GATT Service initialisation.

*/
CsisServerServiceHandleType GattCsisServerInit(
    AppTask appTask,
    const GattCsisServerInitParamType* initParams,
    uint16 startHandle,
    uint16 endHandle);

/*!
    @brief This API is used to set the Lock state when it is changed by the server.

    @param handle The server instance handle returned in GattCsisServerInit()
    @param lock Value of Lock to be set defined in GattCsisServerLocktype
    @param lockTimeout Value of Lock timer post which Lock shall be released by
           CSIS server.

    @return TRUE if successful, FALSE otherwise

    NOTE: The application may be want to extend the lock_timeout at any point if
    the Lock is already in use by client.In such case application can call this
    API with lock value as CSIS_SERVER_INVALID_LOCK_VALUE and a valid lock_timeout value.
    This will reset the lock timer running in the service and restart the lock
    timer again.

*/

bool GattCsisServerSetLock(
    CsisServerServiceHandleType handle,
    GattCsisServerLocktype lock,
    uint16 lockTimeout);

/*!
    @brief This API is used when the server application needs to know which csis
           service instance handle has Lock in use.

    @return CsisServerServiceHandleType If the Lock is already in use then CsisServerServiceHandleType
           of the instance will be returned.
           If the lock is not in use then 0 shall be returned

*/
CsisServerServiceHandleType GattCsisServerGetLock(void);

/*!
    @brief This API is used to get the RSI value for the different CSIS Instances.
           Each CSIS instance is associated with different SIRK value.

    @param handle The server instance handle returned in GattCsisServerInit()

    @return uint8 * Pointer to the rsi data.

    The call to GattCsisServerGetRsi will return immediately with a pointer to rsi data of 6 octets
    It is expected that the application will free the memory associated with this pointer once it has finished with it.
    A NULL pointer is returned if operation to generate rsi failed.

*/

uint8* GattCsisServerGetRsi(CsisServerServiceHandleType handle);


/*!
    @brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    @param handle The server instance handle returned in GattCsisServerInit().
    @param cid  The cid wth the remote device.

    @return GattCsisServerConfigType  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.

*/
GattCsisServerConfigType* GattCsisServerRemoveConfig(
    CsisServerServiceHandleType handle,
    CsisServerConnectionIdType cid);


/*!
    @brief Add configuration for a paired peer device, identified by its
           Connection ID (CID).

    @param handle The server instance returned in GattCsisServerInit().
    @param cid The Connection ID to the peer device.
    @param config  Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.

    @return CsisServerGattStatusType status of the Add Configuration operation.

*/

CsisServerGattStatusType GattCsisServerAddConfig(
    CsisServerServiceHandleType handle,
    CsisServerConnectionIdType cid,
    GattCsisServerConfigType* config);


/*!
    \brief Gets the configuration for a peer device, identified by its
           Connection ID.

    This gets the configuration for that peer device from the
    service library.
    This should only be done after GATT_CSIS_SERVER_CONFIG_CHANGE_IND
    is sent by library.

    \param handle The server instance returned in GattCsisServerInit().
    \param cid A Connection ID for the peer device.

    \return CsisServerGattStatusType Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattCsisServerConfigType* GattCsisServerGetConfig(
    CsisServerServiceHandleType handle,
    CsisServerConnectionIdType  cid);
/*!
    @brief This function is used to update the CSIS init params.

    @param handle The server instance returned in GattCsisServerInit().
    @param initParams CSIS information server Database paramters for this instance

    @return CsisServerGattStatusType status of the update of CSIS init param.

    NOTE: Application need not call this API if init_params are not changed
          and are same as what was passed in GattCsisServerInit()
*/

CsisServerGattStatusType GattCsisServerUpdateInitParam(
    CsisServerServiceHandleType handle,
    GattCsisServerInitParamType* initParams);

#endif /* GATT_CSIS_SERVER_H */
