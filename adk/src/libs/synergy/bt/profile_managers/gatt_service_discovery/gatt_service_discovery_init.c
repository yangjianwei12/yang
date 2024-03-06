/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include <stdlib.h>

#include "gatt_service_discovery_init.h"
#include "gatt_service_discovery_handler.h"
#include "csr_bt_gatt_lib.h"


/* GATT SD Instance */
GGSD *gattSdInst = NULL;


bool gattServiceDiscoveryIsInit(void)
{
    return NULL != gattSdInst;
}


GGSD* gattServiceDiscoveryGetInstance(void)
{
    return gattSdInst;
}

void GattServiceDiscoveryInit(void **gash)
{
    gattSdInst           = (GGSD *) GATT_SD_MALLOC(sizeof(GGSD));
    *gash                = (GGSD *) gattSdInst;

    memset(gattSdInst, 0, sizeof(GGSD));
    gattSdInst->app_task = APP_TASK_INVALID;

    CsrBtGattRegisterReqSend(CSR_BT_GATT_SRVC_DISC_IFACEQUEUE, 0);
}


void GattServiceDiscoveryDeinit(void **gash)
{
    uint16    msg_type;
    void*     msg;
    GGSD     *gatt_sd;

    gatt_sd    = (GGSD *) (*gash);

    while (CsrSchedMessageGet(&msg_type, &msg))
    {
        switch (msg_type)
        {
            case CSR_BT_GATT_PRIM:
            case GATT_SRVC_DISC_PRIM:
                break;
        }
        free(msg);
    }

    free(gatt_sd);
}
