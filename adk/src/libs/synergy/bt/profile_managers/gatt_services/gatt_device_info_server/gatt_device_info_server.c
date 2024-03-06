/* Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"
#include "gatt_device_info_server.h"
#include "gatt_device_info_server_debug.h"
#include "gatt_device_info_server_private.h"
#include "gatt_device_info_server_msg_handler.h"


GDISS_T disServerInst;

/* Synergy Task Init */
void CsrBtDisInit(void **gash)
{
    *gash = &disServerInst;
    GATT_DEVICE_INFO_SERVER_DEBUG_INFO(("GDIS: CsrBtDisInit\n"));
}

/* Synergy Task DeInit */
#ifdef ENABLE_SHUTDOWN
void CsrBtDisDeinit(void **gash)
{
    GATT_DEVICE_INFO_SERVER_DEBUG_INFO(("GDIS: CsrBtDisDeinit\n"));
}
#endif

/* Only one instance of Device Information Service is supported */
/****************************************************************************/
bool GattDeviceInfoServerInit(AppTask appTask, gdiss_t *const dev_info_server,
                              gatt_dis_init_params_t *const  init_params,
                              uint16 start_handle,
                              uint16 end_handle)
{
    if((appTask == CSR_SCHED_QID_INVALID) || (dev_info_server == NULL) || (init_params == NULL))
    {
        GATT_DEVICE_INFO_SERVER_PANIC(("GDIS: Invalid Initialisation parameters"));
        return FALSE;
    }

    disServerInst.deviceInfo = dev_info_server;

    /* Reset all the service library memory */
    memset(dev_info_server, 0, sizeof(gdiss_t));
    /*Set up the library task handler for external messages*/
    dev_info_server->lib_task = CSR_BT_DIS_SERVER_IFACEQUEUE;
    /*Store application message handler as application messages need to be posted here */
    dev_info_server->app_task = appTask;
    dev_info_server->dis_params = *init_params;

    /* Fill in the registration parameters */
    dev_info_server->start_handle = start_handle;
    dev_info_server->end_handle = end_handle;

    dev_info_server->gattId = CsrBtGattRegister(CSR_BT_DIS_SERVER_IFACEQUEUE);

    if (dev_info_server->gattId != CSR_BT_GATT_INVALID_GATT_ID)
    {
        CsrBtGattFlatDbRegisterHandleRangeReqSend(dev_info_server->gattId,
                        dev_info_server->start_handle,
                        dev_info_server->end_handle);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


