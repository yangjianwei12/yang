/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/


#ifndef GATT_TRANSMIT_POWER_SERVER_ACCESS_H_
#define GATT_TRANSMIT_POWER_SERVER_ACCESS_H_

#include "gatt_transmit_power_server.h"

/***************************************************************************
NAME
    handleTpsAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the TPS library.
*/
void handleTpsAccess(GTPSS* tps_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind, CsrBtTypedAddr address);

#endif
