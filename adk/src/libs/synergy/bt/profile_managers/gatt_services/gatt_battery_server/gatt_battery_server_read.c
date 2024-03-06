/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gatt_battery_server_private.h"
#include "gatt_battery_server_access.h"


/****************************************************************************/
bool GattBatteryServerReadLevelResponse(const GBASS *battery_server, uint32 cid, uint8 battery_level)
{
    uint16 status = CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED;

    if (battery_server == NULL)
    {
        return FALSE;
    }

    if (battery_level <= 100)
    {
        status = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }
    else
    {
        status = CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    }
    sendBatteryLevelAccessRsp(battery_server, cid, battery_level, status);

    return TRUE;
}

/****************************************************************************/
bool GattBatteryServerReadClientConfigResponse(const GBASS *battery_server, uint32 cid, uint16 client_config)
{
    if (battery_server == NULL)
    {
        return FALSE;
    }

    sendBatteryConfigAccessRsp(battery_server, cid, client_config);

    return TRUE;
}

/****************************************************************************/
bool GattBatteryServerReadPresentationResponse(const GBASS *battery_server, uint32 cid, uint8 name_space, uint16 description)
{
    if (battery_server == NULL)
    {
        return FALSE;
    }

    sendBatteryPresentationAccessRsp(battery_server, cid, name_space, description);
    
    return TRUE;
}
