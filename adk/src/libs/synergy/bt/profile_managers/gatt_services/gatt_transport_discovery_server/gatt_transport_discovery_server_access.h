/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TRANSPORT_DISCOVERY_SERVER_ACCESS_H_
#define GATT_TRANSPORT_DISCOVERY_SERVER_ACCESS_H_

#include "gatt_transport_discovery_server.h"

/***************************************************************************
NAME
    handleTdsAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the TDS library.
*/
void handleTdsAccess(GTDS_T* tdsServer, const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

#endif
