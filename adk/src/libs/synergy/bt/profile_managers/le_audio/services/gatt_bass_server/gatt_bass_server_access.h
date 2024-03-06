/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_BASS_SERVER_ACCESS_H_
#define GATT_BASS_SERVER_ACCESS_H_

#include "gatt_bass_server_private.h"

/***************************************************************************
NAME
    handleBassServerAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the BASS library.
*/
void handleBassServerAccess(GBASSSS *bass_server, const CsrBtGattAccessInd *access_ind);

#endif
