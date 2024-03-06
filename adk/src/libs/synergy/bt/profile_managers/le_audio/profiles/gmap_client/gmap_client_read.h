/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GMAP_CLIENT_READ_H_
#define GMAP_CLIENT_READ_H_

#include "gmap_client_private.h"

void gmapClientHandleReadRoleCfm(GMAP *gmapClientInst,
                                 const GattGmasClientReadRoleCfm *cfm);

void gmapClientHandleReadUnicastFeaturesCfm(GMAP *gmapClientInst,
                                            const GattGmasClientReadUnicastFeaturesCfm *cfm);

void gmapClientHandleReadBroadcastFeaturesCfm(GMAP *gmapClientInst,
                                              const GattGmasClientReadBroadcastFeaturesCfm *cfm);

#endif
