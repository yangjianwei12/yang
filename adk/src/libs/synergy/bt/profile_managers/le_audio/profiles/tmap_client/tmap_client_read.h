/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef TMAP_CLIENT_READ_H_
#define TMAP_CLIENT_READ_H_

#include "tmap_client_private.h"

void tmapClientHandleReadRoleCharacCfm(TMAP *tmapClientInst,
                                       const GattTmasClientRoleCfm *msg);

#endif
