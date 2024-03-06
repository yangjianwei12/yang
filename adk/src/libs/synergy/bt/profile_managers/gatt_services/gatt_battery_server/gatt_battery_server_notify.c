/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_gatt_lib.h"
#include "gatt_battery_server.h"
#include "gatt_battery_server_access.h"
#include "gatt_battery_server_private.h"
#include "gatt_battery_server_db.h"

/****************************************************************************/
bool GattBatteryServerSendLevelNotification(const GBASS *battery_server, uint16 number_cids, const uint32 *cids, uint8 battery_level)
{
    bool result = FALSE;
    uint16 index = 0;
    CSR_UNUSED(battery_server);

    if ((battery_server != NULL) && (battery_level <= 100) && number_cids)
    {
        if (battery_server->notifications_enabled)
        {
            result = TRUE;

            for (index = 0; index < number_cids; index++)
            {
                if (cids[index] == 0)
                {
                    /* CID must be non-zero */
                    result = FALSE;
                }
                else
                {
                    /* Send notification */
                    uint8 *value = (uint8*) CsrPmemAlloc(GATT_BATTERY_LEVEL_OCTET_SIZE);
                    memcpy(value, &battery_level, GATT_BATTERY_LEVEL_OCTET_SIZE);
                    CsrBtGattNotificationEventReqSend(battery_server->gattId, cids[index], HANDLE_BATTERY_LEVEL, GATT_BATTERY_LEVEL_OCTET_SIZE, value);
                }
            }
            
        }
        else
        {
            result = FALSE;
        }
    }

    return result;
}

