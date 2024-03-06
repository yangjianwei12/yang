/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_DESTROY_H_
#define VCP_DESTROY_H_

#include "vcp_private.h"

/***************************************************************************
NAME
    vcpSendDestroyCfm
    
DESCRIPTION
    Send the VCP_DESTROY_CFM message.
*/
void vcpSendDestroyCfm(VCP * vcp_inst, VcpStatus status);

/***************************************************************************
NAME
    vcpSendVcsTerminateCfm

DESCRIPTION
    Send the VCP_VCS_TERMINATE_CFM message.
*/
void vcpSendVcsTerminateCfm(VCP *vcp_inst,
                            VcpStatus status,
                            GattVcsClientDeviceData handles);

/***************************************************************************
NAME
    vcpSendIncludedServicesTerminateCfm

DESCRIPTION
    Send the VCP_VOCS_TERMINATE_CFM and the VCP_AICS_TERMINATE_CFM messages.
*/
void vcpSendIncludedServicesTerminateCfm(VCP *vcp_inst, VcpStatus status);

/***************************************************************************
NAME
    vcpHandleAicsClientTerminateResp

DESCRIPTION
    Handle the GATT_AICS_CLIENT_TERMINATE_CFM message.
*/
void vcpHandleAicsClientTerminateResp(VCP *vcp_inst,
                                      const GattAicsClientTerminateCfm * message);

/***************************************************************************
NAME
    vcpHandleVocsClientTerminateResp

DESCRIPTION
    Handle the GATT_VOCS_CLIENT_TERMINATE_CFM message.
*/
void vcpHandleVocsClientTerminateResp(VCP *vcp_inst,
                                      const GattVocsClientTerminateCfm * message);

/***************************************************************************
NAME
    vcpHandleVcsClientTerminateResp

DESCRIPTION
    Handle the GATT_VCS_CLIENT_TERMINATE_CFM message.
*/
void vcpHandleVcsClientTerminateResp(VCP *vcp_inst,
                                     const GattVcsClientTerminateCfm * message);

/***************************************************************************
NAME
    vcpDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void vcpDestroyProfileInst(VCP *vcp_inst);

#endif
