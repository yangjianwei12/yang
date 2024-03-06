/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_WRITE_H_
#define MICP_WRITE_H_

#include "micp_private.h"

/***************************************************************************
NAME
    micpHandleMicsSetMuteValueOp

DESCRIPTION
    Handle the GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM message.
*/
void micpHandleMicsSetMuteValueOp(MICP *micp_inst,
                                      const GattMicsClientSetMuteValueCfm *msg);

/***************************************************************************
NAME
    micpMicsControlPointOp

DESCRIPTION
    Perform the specified mics control point opration.
*/
void micpMicsControlPointOp(ServiceHandle profile_handle,
                          MicpMicsControlPointOpcodes opcode,
                          uint8 mute_value_operand);

#endif
