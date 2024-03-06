/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_DESTROY_H_
#define MICP_DESTROY_H_

#include "micp_private.h"

/***************************************************************************
NAME
    micpSendDestroyCfm
    
DESCRIPTION
    Send the MICP_DESTROY_CFM message.
*/
void micpSendDestroyCfm(MICP * micp_inst, MicpStatus status);

/***************************************************************************
NAME
    micpSendMicsTerminateCfm

DESCRIPTION
    Send the MICP_MICS_TERMINATE_CFM message.
*/
void micpSendMicsTerminateCfm(MICP *micp_inst,
                            MicpStatus status,
                            GattMicsClientDeviceData handles);

/***************************************************************************
NAME
    micpSendIncludedServicesTerminateCfm

DESCRIPTION
    Send the the MICP_AICS_TERMINATE_CFM messages.
*/
void micpSendIncludedServicesTerminateCfm(MICP *micp_inst, MicpStatus status);

/***************************************************************************
NAME
    micpHandleAicsClientTerminateResp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_TERMINATE_CFM message.
*/
void micpHandleAicsClientTerminateResp(MICP *micp_inst,
                                      const GattAicsClientTerminateCfm * message);

/***************************************************************************
NAME
    micpHandleMicsClientTerminateResp

DESCRIPTION
    Handle the GATT_MICS_CLIENT_TERMINATE_CFM message.
*/
void micpHandleMicsClientTerminateResp(MICP *micp_inst,
                                     const GattMicsClientTerminateCfm * message);

/***************************************************************************
NAME
    micpDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void micpDestroyProfileInst(MICP *micp_inst);

#endif
