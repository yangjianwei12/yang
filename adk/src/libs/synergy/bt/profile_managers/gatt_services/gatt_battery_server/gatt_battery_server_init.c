/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_gatt_lib.h"

#include "gatt_battery_server_private.h"
#include "gatt_battery_server_debug.h"
#include "gatt_battery_server_msg_handler.h"
#include "gatt_battery_server.h"


GBASS_T basServerInst;

void initBasInstanceList(CsrCmnListElm_t *elem)
{
    /* Initialize a BasInstanceListElement. This function is called every
     * time a new entry is made on the queue list */
    BasInstanceListElm_t *cElem = (BasInstanceListElm_t *) elem;

    cElem->gattId = CSR_BT_GATT_INVALID_GATT_ID;
}

/* Synergy Task Init */
void CsrBtBasInit(void **gash)
{
    *gash = &basServerInst;
    GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: CsrBtBasInit\n"));
    CsrCmnListInit(&basServerInst.instanceList, 0, initBasInstanceList, NULL);
}

/* Synergy Task DeInit */
#ifdef ENABLE_SHUTDOWN
void CsrBtBasDeinit(void **gash)
{
    GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: CsrBtBasDeinit\n"));
}
#endif

/****************************************************************************/
bool GattBatteryServerInit(GBASS *battery_server,
                           AppTask app_task,
                           const gatt_battery_server_init_params_t *init_params,
                           uint16 start_handle,
                           uint16 end_handle)
{
    BasInstanceListElm_t *basInst = NULL;

    if ((app_task == CSR_SCHED_QID_INVALID) || (battery_server == NULL))
    {
        GATT_BATTERY_SERVER_PANIC(("GBASS: Invalid Initialisation parameters"));
        return FALSE;
    }

    GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: GattBatteryServerInit Start Hndl 0x%x, End Hndl 0x%x",start_handle, end_handle));
    /* Add BAS Server instance element to the instance list */
    basInst = BAS_SERVER_ADD_INSTANCE(basServerInst.instanceList);
    /* Store battery server pointer in the instance element */
    basInst->pBass = battery_server;

    /* Set up library handler for external messages */
    basInst->pBass->lib_task = CSR_BT_BAS_SERVER_IFACEQUEUE;
    /* Store the Task function parameter.
       All library messages need to be sent here */
    basInst->pBass->app_task = app_task;
    /* Check initialisation parameters */
    if (init_params)
    {
        /* Store notifications enable flag */
        basInst->pBass->notifications_enabled = init_params->enable_notifications;
    }
    else
    {
        basInst->pBass->notifications_enabled = FALSE;
    }

    basInst->pBass->end_handle = end_handle;
    basInst->pBass->start_handle = start_handle;

    basInst->pBass->gattId = CsrBtGattRegister(CSR_BT_BAS_SERVER_IFACEQUEUE);
    basInst->gattId = basInst->pBass->gattId;
    GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: gattId 0x%x",basInst->pBass->gattId));

    if (basInst->gattId != CSR_BT_GATT_INVALID_GATT_ID)
    {
        CsrBtGattFlatDbRegisterHandleRangeReqSend(basInst->pBass->gattId,
                        basInst->pBass->start_handle,
                        basInst->pBass->end_handle);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void handleBatteryServerRegisterHandleRangeCfm(GBASS *battery_server,
                                         const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    CSR_UNUSED(battery_server);
    CSR_UNUSED(cfm);
    GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: handleBatteryServerRegisterHandleRangeCfm gattId 0x%x, status 0x%x",cfm->gattId, cfm->resultCode));
}
