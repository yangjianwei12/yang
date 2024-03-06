/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

/*!
@file    gatt_transmit_power_server.h
@brief   Header file for the GATT Transmit Power Server library.

        This file provides documentation for the GATT Transmit Power Server library
        API.
*/

#ifndef GATT_TRANSMIT_POWER_SERVER_H
#define GATT_TRANSMIT_POWER_SERVER_H


#include "csr_bt_types.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"

#include "service_handle.h"

typedef struct _GTPSS
{
    AppTaskData         lib_task;
    AppTask             app_task;
    CsrBtGattId         gattId;
} GTPSS;

/*! @brief Enumeration of messages an application task can receive from the TPS server library.
 */
typedef enum
{
    /* Server messages */
    GATT_TPS_SERVER_GET_TRANSMIT_POWER_LEVEL_IND, /* Tells application to share the Tx power level with Transmit Power library. Application must call TpsServerSendTxPowerRsp to share Tx power with TPS lib */


    /* Library message limit */
    GATT_TPS_SERVER_MESSAGE_TOP
} GattTpsServerMessageId;
    

/*! @brief Contents of the GATT_TPS_SERVER_GET_TRANSMIT_POWER_LEVEL_IND message that is sent by the library,
    due to a request from the client to get the transmit power level of the server.
 */
typedef struct
{
    GattTpsServerMessageId id;
    ServiceHandle srvcHndl;
    /*! Connection identifier */
    connection_id_t         cid;

} GattTpsServerGetTransmitPowerLevelInd;


/*!
    @brief Initialises the Transmit Power service Library.

    @param appTask The Task that will receive the messages sent from this Published Audio Capability service library.
    @param tps A valid area of memory that the server library can use.Must be of at least
               the size of GTPSS
    @param startHandle This indicates the start handle of the Transmit Power service
    @param endHandle This indicates the end handle of the Transmit Power service

    @return The service handle for the Transmit Power service.

*/
bool GattTransmitPowerServerInitTask(AppTask appTask,
                                    GTPSS *const tps,
                                    uint16 startHandle,
                                    uint16 endHandle
                                    );


#endif /* GATT_TRANSMIT_POWER_SERVER_H */      
