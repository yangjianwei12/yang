/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gatt_transmit_power_server_private.h"
#include "gatt_transmit_power_server_msg_handler.h"
#include "gatt_transmit_power_server.h"
#include "csr_bt_gatt_lib.h"

GTPSS_T tpsServerInst;

/****************************************************************************/
bool GattTransmitPowerServerInitTask(AppTask appTask,
                                    GTPSS *const tps,
                                    uint16 startHandle,
                                    uint16 endHandle
                                    )
{
    CsrBtGattId gattId;

    /* validate the input parameters */
    if (appTask == CSR_SCHED_QID_INVALID)
    {

        GATT_TPS_SERVER_PANIC(("TPS: Invalid Initialisation parameters"));
    }

    if (tps == NULL)
    {
        GATT_TPS_SERVER_PANIC(("TPS: Unable to allocate an TPS Service Instance"));
    }

    /* Reset all the service library memory, e.g. num_connections = 0  */
    CsrMemSet(tps, 0, sizeof(GTPSS));

    /* Store application message handler as application messages need to be posted here */
    tps->app_task = appTask;

    /* Fill in the registration parameters */
    tps->lib_task = CSR_BT_TPS_SERVER_IFACEQUEUE;
    /* Try to register this instance of TPS library to Gatt profile */
    /* Register with the GATT  */
     gattId = CsrBtGattRegister(tps->lib_task);
    /* verify the result */
     if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
     {
         return FALSE;
     }
     else
     {
         tps->gattId = gattId;
         tpsServerInst.tps = tps;
         CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle, endHandle);
     }
    return TRUE;
}


void GattTpsServerTaskInit(void** gash)
{
    *gash = &tpsServerInst;
    GATT_TPS_SERVER_DEBUG_INFO(("\nTPS: GattTpsServerTaskInit\n"));
}
#ifdef ENABLE_SHUTDOWN
void GattTpsServerTaskDeinit(void** gash)
{
    *gash = NULL;
    GATT_TPS_SERVER_DEBUG_INFO(("\nTPS: GattTpsServerTaskDeinit\n"));

}
#endif



