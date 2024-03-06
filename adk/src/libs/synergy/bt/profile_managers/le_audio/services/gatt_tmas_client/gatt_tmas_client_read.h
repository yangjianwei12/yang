/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_READ_H_
#define GATT_TMAS_CLIENT_READ_H_

#include "gatt_tmas_client_private.h"

/***************************************************************************
NAME
    gattTmasClientHandleInternalRead

DESCRIPTION
    Handles the internal TMAS_CLIENT_INTERNAL_MSG_READ_REQ message.
*/
void gattTmasClientHandleInternalRead(const GTMASC * tmasClient,
                                      RoleType role);

void gattTmasClientHandleReadRoleValueResp(GTMASC *tmasClient,
                                           status_t resultCode,
                                           uint16 valueLength,
                                           uint8 *value);
#endif
