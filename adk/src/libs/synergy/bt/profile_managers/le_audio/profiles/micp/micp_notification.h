/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_NOTIFICATION_H_
#define MICP_NOTIFICATION_H_

#include "gatt_mics_client.h"

#include "micp_private.h"

/***************************************************************************
NAME
    micpHandleMicsNtfCfm

DESCRIPTION
    Handle a GATT_MICS_CLIENT_NTF_CFM message.
*/
void micpHandleMicsNtfCfm(MICP *micp_inst,
                                      const GattMicsClientNtfCfm *msg);

#endif
