/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#ifndef GATT_BATTERY_SERVER_ACCESS_H_
#define GATT_BATTERY_SERVER_ACCESS_H_

#include "gatt_battery_server.h"

/* Required octets for values sent to Battery Level Descriptor */
#define GATT_BATTERY_LEVEL_OCTET_SIZE sizeof(uint8) * 1
/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_OCTET_SIZE sizeof(uint8) * 2
/* Required octets for values sent to Presentation Descriptor */
#define GATT_PRESENTATION_OCTET_SIZE sizeof(uint8) * 7

/***************************************************************************
NAME
    sendBatteryLevelAccessRsp

DESCRIPTION
    Send battery level access response to the GATT library.
*/
void sendBatteryLevelAccessRsp(const GBASS *battery_server, uint32 cid, uint8 battery_level, uint16 result);

/***************************************************************************
NAME
    sendBatteryConfigAccessRsp

DESCRIPTION
    Send battery config access response to the GATT library.
*/

void sendBatteryConfigAccessRsp(const GBASS *battery_server, uint32 cid, uint16 client_config);

/***************************************************************************
NAME
    sendBatteryPresentationAccessRsp

DESCRIPTION
    Send battery presentation access response to the GATT library.
*/
void sendBatteryPresentationAccessRsp(const GBASS *battery_server, uint32 cid, uint8 name_space, uint16 description);

/***************************************************************************
NAME
    handleBatteryServerWriteAccess

DESCRIPTION
    Handles the CsrBtGattDbAccessWriteInd message that was sent to the battery library.
*/
void handleBatteryServerWriteAccess(GBASS *battery_server, const CsrBtGattDbAccessWriteInd *access_ind);


/***************************************************************************
NAME
    handleBatteryServerReadAccess

DESCRIPTION
    Handles the CsrBtGattDbAccessReadInd message that was sent to the battery library.
*/
void handleBatteryServerReadAccess(GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind);

#endif
