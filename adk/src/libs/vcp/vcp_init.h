/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_INIT_H_
#define VCP_INIT_H_

#include <gatt_vcs_client.h>

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpSendInitCfm
    
DESCRIPTION
    Send a VCP_INIT_CFM message to the application.
*/
void vcpSendInitCfm(VCP * vcp_inst, VcpStatus status);

/***************************************************************************
NAME
    vcpHandleAicsClientInitResp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_INIT_CFM message.
*/
void vcpHandleAicsClientInitResp(VCP *vcp_inst,
                                 const GattAicsClientInitCfm * message);

/***************************************************************************
NAME
    vcpHandleVocsClientInitResp

DESCRIPTION
    Handle the GATT_VOCS_CLIENT_INIT_CFM message.
*/
void vcpHandleVocsClientInitResp(VCP *vcp_inst,
                                 const GattVocsClientInitCfm * message);

/***************************************************************************
NAME
    vcpHandleVcsClientInitResp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_INIT_CFM message.
*/
void vcpHandleVcsClientInitResp(VCP *vcp_inst,
                                const GattVcsClientInitCfm * message);

/***************************************************************************
NAME
    vcpStartScndrSrvcInit

DESCRIPTION
    Start the initialisation of the secondary services.
*/
void vcpStartScndrSrvcInit(VCP *vcp_inst);

/***************************************************************************
NAME
    vcpSendSetInitialVolOpCfm

DESCRIPTION
    Send the VCP_SET_INITIAL_VOL_CFM message.
*/
void vcpSendSetInitialVolOpCfm(VCP *vcp_inst,
                               VcpStatus status);

#endif
