/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_COMMON_H_
#define MICP_COMMON_H_

#include "gatt_mics_client.h"

#include "micp.h"
#include "micp_private.h"

/***************************************************************************
NAME
    micpAddAicsInst

DESCRIPTION
    Add a new element to the list of AICS instances.
*/
void micpAddAicsInst(MICP *micp_inst,
                    GattAicsClientDeviceData *aics_handle,
                    uint16 start_handle,
                    uint16 end_handle);

/***************************************************************************
NAME
    micpIsValidAicsInst

DESCRIPTION
    Check if the service handle provided is a valid one for a AICS client instance.
*/
bool micpIsValidAicsInst(MICP *micp_inst, ServiceHandle srvc_hndl);

/***************************************************************************
NAME
    micpDestroyReqAllSrvcHndlList

DESCRIPTION
    Destroy all the lists of service handles in the profile memory instance.
*/
void micpDestroyReqAllSrvcHndlList(MICP *micp_inst);

/***************************************************************************
NAME
    micpAddElementAicsSrvcHndlList

DESCRIPTION
    Add a new element to the list of AICS service handles.
*/
void micpAddElementAicsSrvcHndlList(MICP *micp_inst, ServiceHandle element);

/***************************************************************************
NAME
    MicpDestroyReqAllInstList

DESCRIPTION
    Destroy all the lists of AICS instances in the profile memory instance.
*/
void MicpDestroyReqAllInstList(MICP *micp_inst);

#endif
