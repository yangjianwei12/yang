/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_READ_H_
#define GATT_GMAS_CLIENT_READ_H_

#include "gatt_gmas_client_private.h"

/***************************************************************************
NAME
    gattGmasClientHandleInternalReadRole

DESCRIPTION
    Handles the internal GMAS_CLIENT_INTERNAL_MSG_READ_ROLE message.
*/
void gattGmasClientHandleInternalReadRole(const GGMASC * gmasClient,uint16 handle);

void gattGmasClientHandleInternalReadUnicastFeatures(const GGMASC * gmasClient,uint16 handle);

void gattGmasClientHandleInternalReadBroadcastFeatures(const GGMASC * gmasClient,uint16 handle);

void gattGmasClientHandleReadValueRespCfm(const GGMASC * gmasClient,const CsrBtGattReadCfm * msg);

void gattGmasClientHandleReadRoleValueRespCfm(const GGMASC * gmasClient,const CsrBtGattReadCfm * msg);

void gattGmasClientHandleReadUnicastFeaturesValueRespCfm(const GGMASC * gmasClient,const CsrBtGattReadCfm * msg);

void gattGmasClientHandleReadBroadcastFeaturesValueRespCfm(const GGMASC * gmasClient,const CsrBtGattReadCfm * msg);
#endif
