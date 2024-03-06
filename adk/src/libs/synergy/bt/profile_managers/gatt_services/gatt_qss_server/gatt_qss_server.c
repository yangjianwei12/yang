/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_bt_gatt_lib.h"

#include "gatt_qss_server_private.h"
#include "gatt_qss_server_debug.h"
#include "gatt_qss_server_msg_handler.h"
#include "gatt_qss_server.h"

ServiceHandle qssServerServiceHandle;

/****************************************************************************/
/* Synergy Task Initailization */
void gattQssServerTaskInit(void **gash)
{
    *gash = &qssServerServiceHandle;
    GATT_QSS_SERVER_DEBUG_INFO(("\n GQSSS: GattQssServerTaskInit \n")); 
}

/****************************************************************************/
/* Synergy Task De-Initalization */
#ifdef ENABLE_SHUTDOWN
void gattQssServerTaskDeinit(void **gash)
{
    ServiceHandle srvHndl = *((ServiceHandle*)*gash);

    if (ServiceHandleFreeInstanceData(srvHndl))
    {
        GATT_QSS_SERVER_DEBUG_INFO(("\n GQSSS: GattQssServerTaskDeinit \n"));
    }
    else
    {
        GATT_QSS_SERVER_DEBUG_PANIC(("\n GQSSS: Unalbe to free the QSS server instance \n"));
    }
}
#endif

/****************************************************************************/
ServiceHandle GattQssServerInit(AppTask appTask,
                                uint16 startHandle,
                                uint16 endHandle)
{
    GQSSS* gattQssServerInst = NULL;
    
    /* validate the input parameters */
    if(appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_QSS_SERVER_PANIC(("\n GQSSS: Invalid Initailisation parameters \n"));
    }
    
    qssServerServiceHandle = ServiceHandleNewInstance((void**)&gattQssServerInst,
                                                      sizeof(GQSSS));

    if (gattQssServerInst)
    {
        /* Reset all the service library memory */
        CsrMemSet(gattQssServerInst, 0, sizeof(GQSSS));

        /* Store the Task function parameter.
         * All library messages need to be send here */
        gattQssServerInst->libTask = CSR_BT_QSS_SERVER_IFACEQUEUE;
        gattQssServerInst->appTask = appTask;
        gattQssServerInst->srvHndl = qssServerServiceHandle;
        gattQssServerInst->startHandle = startHandle;
        gattQssServerInst->endHandle = endHandle;

        CsrBtGattRegisterReqSend(CSR_BT_QSS_SERVER_IFACEQUEUE, 0);
        return gattQssServerInst->srvHndl;
    }
    else
    {
        GATT_QSS_SERVER_PANIC(("\n GQSSS: Memory allocation of server instance failed! \n"));
    }
    return 0;
}
