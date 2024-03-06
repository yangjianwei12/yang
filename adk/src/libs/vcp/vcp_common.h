/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef VCP_COMMON_H_
#define VCP_COMMON_H_

#include <gatt_vcs_client.h>

#include "vcp.h"
#include "vcp_private.h"

/***************************************************************************
NAME
    vcpAddVocsInst

DESCRIPTION
    Add a new element to the list of VOCS instances.
*/
void vcpAddVocsInst(VCP *vcp_inst,
                    GattVocsClientDeviceData *vocs_handle,
                    uint16 start_handle,
                    uint16 end_handle);

/***************************************************************************
NAME
    vcpAddAicsInst

DESCRIPTION
    Add a new element to the list of AICS instances.
*/
void vcpAddAicsInst(VCP *vcp_inst,
                    GattAicsClientDeviceData *aics_handle,
                    uint16 start_handle,
                    uint16 end_handle);

/***************************************************************************
NAME
    vcpIsValidVocsInst

DESCRIPTION
    Check if the service handle provided is a valid one for a VOCS client instance.
*/
bool vcpIsValidVocsInst(VCP *vcp_inst, ServiceHandle srvc_hndl);

/***************************************************************************
NAME
    vcpIsValidAicsInst

DESCRIPTION
    Check if the service handle provided is a valid one for a AICS client instance.
*/
bool vcpIsValidAicsInst(VCP *vcp_inst, ServiceHandle srvc_hndl);

/***************************************************************************
NAME
    vcpDestroyReqAllSrvcHndlList

DESCRIPTION
    Destroy all the lists of service handles in the profile memory instance.
*/
void vcpDestroyReqAllSrvcHndlList(VCP *vcp_inst);

/***************************************************************************
NAME
    VcpDestroyReqVocsSrvcHndlList

DESCRIPTION
    Destroy the list of VOCS service handles in the profile memory instance.
*/
void VcpDestroyReqVocsSrvcHndlList(VCP *vcp_inst);

/***************************************************************************
NAME
    vcpAddElementAicsSrvcHndlList

DESCRIPTION
    Add a new element to the list of AICS service handles.
*/
void vcpAddElementAicsSrvcHndlList(VCP *vcp_inst, ServiceHandle element);

/***************************************************************************
NAME
    vcpAddElementVocsSrvcHndlList

DESCRIPTION
    Add a new element to the list of VOCS service handles.
*/
void vcpAddElementVocsSrvcHndlList(VCP *vcp_inst, ServiceHandle element);

/***************************************************************************
NAME
    VcpDestroyReqAllInstList

DESCRIPTION
    Destroy all the lists of VOCS and AICS instances in the profile memory instance.
*/
void VcpDestroyReqAllInstList(VCP *vcp_inst);

#endif
