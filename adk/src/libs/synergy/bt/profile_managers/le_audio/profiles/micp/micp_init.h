/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_INIT_H_
#define MICP_INIT_H_

#include "gatt_mics_client.h"

#include "micp_private.h"

/***************************************************************************
NAME
    micpSendInitCfm
    
DESCRIPTION
    Send a MICP_INIT_CFM message to the application.
*/
void micpSendInitCfm(MICP * micp_inst, MicpStatus status);

/***************************************************************************
NAME
    micpHandleAicsClientInitResp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_INIT_CFM message.
*/
void micpHandleAicsClientInitResp(MICP *micp_inst,
                                 const GattAicsClientInitCfm * message);

/***************************************************************************
NAME
    micpHandleMicsClientInitResp

DESCRIPTION
    Handle the GATT_MICS_CLIENT_INIT_CFM message.
*/
void micpHandleMicsClientInitResp(MICP *micp_inst,
                                const GattMicsClientInitCfm * message);

/***************************************************************************
NAME
    micpStartScndrSrvcInit

DESCRIPTION
    Start the initialisation of the secondary services.
*/
void micpStartScndrSrvcInit(MICP *micp_inst);

#endif
