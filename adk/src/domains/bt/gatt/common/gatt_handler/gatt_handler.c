/*!
    \copyright  Copyright (c) 2015 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \ingroup    gatt_handler
    \brief      Application support for GATT, GATT Server and GAP Server
*/

#include "gatt_handler.h"
#include "local_name.h"
#include "gatt_handler_db_if.h"
#include "adk_log.h"
#include "system_state.h"

#include <bdaddr.h>
#ifndef USE_SYNERGY
#include <gatt.h>
#include <gatt_manager.h>
#endif
#include <connection_manager.h>
#include <panic.h>
#include <l2cap_prim.h>

/*!< App GATT component task */
gattTaskData    app_gatt;

/*! Earbud GATT database, for the required GATT and GAP servers. */
extern const uint16 gattDatabase[];


bool GattHandlerInit(Task init_task)
{
    gattTaskData *gatt = GattGetTaskData();

    UNUSED(init_task);

#ifndef USE_SYNERGY
    if (!GattManagerRegisterConstDB(&gattDatabase[0], GattGetDatabaseSize()/sizeof(uint16)))
    {
        DEBUG_LOG("appGattInit. Failed to register GATT database");
        Panic();
    }
#endif /* !USE_SYNERGY */

    memset(gatt,0,sizeof(*gatt));

    return TRUE;
}


bool GattHandler_GetPublicAddressFromConnectId(uint16 cid, bdaddr *public_addr)
{
    tp_bdaddr client_tpaddr = {0};
    tp_bdaddr public_tpaddr = {0};
    bool addr_found = FALSE;
    
    if (VmGetBdAddrtFromCid(cid, &client_tpaddr))
    {
        if (ConManagerResolveTpaddr(&client_tpaddr, &public_tpaddr))
        {
            *public_addr = public_tpaddr.taddr.addr;
            addr_found = TRUE;
        }
    }

    return addr_found;
}

uint16 GattHandler_GetGattStartHandle(gatt_server_t gatt_server)
{
    uint16 start_handle = 0;
    switch(gatt_server)
    {
#ifdef INCLUDE_ACCESSORY_TRACKING
        case gatt_server_accessory_tracking:
            start_handle = HANDLE_ACCESSORY_TRACKING_SERVICE;
            break;
#endif

        default:
            Panic();
        break;
    }

    return start_handle;
}

uint16 GattHandler_GetGattEndHandle(gatt_server_t gatt_server)
{
    uint16 end_handle = 0;
    switch(gatt_server)
    {
#ifdef INCLUDE_ACCESSORY_TRACKING
        case gatt_server_accessory_tracking:
            end_handle = HANDLE_ACCESSORY_TRACKING_N_SERVICE_END;
            break;
#endif

        default:
            Panic();
            break;
    }

    return end_handle;
}
