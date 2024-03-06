/*******************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/
#include <panic.h>
#include <gatt.h>
#include <csrtypes.h>
#include <gatt_vcs_client.h>

#include "vcp.h"
#include "vcp_private.h"
#include "vcp_msg_handler.h"
#include "vcp_init.h"
#include "vcp_debug.h"

/******************************************************************************/
ServiceHandle *VcpGetVocsServiceHandlesRequest(VcpProfileHandle profileHandle,
                                                  uint16 *vocsNum)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst->vocs_num)
    {
        /* If there are VOCS instances, we have to copy their service handles in the vocs_srvc_hndl pointer
         * we will return. We use the ptr pointer to parse the list of VOCS servce handles in the memory instance,
         * and the pointer temp to scroll the returned memory and copy the service handles.
         */
        ServiceHandle *vocs_srvc_hndl = PanicUnlessMalloc(sizeof(ServiceHandle)*(vcp_inst->vocs_num));
        vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;
        ServiceHandle *temp = vocs_srvc_hndl;

        while(ptr)
        {
            (*temp) = ptr->srvc_hdnl;
            temp++;
            ptr = ptr->next;
        }

        (*vocsNum) = vcp_inst->vocs_num;

        return vocs_srvc_hndl;
    }

    (*vocsNum) = 0;
    return NULL;
}

/******************************************************************************/
ServiceHandle *VcpGetAicsServiceHandlesRequest(VcpProfileHandle profileHandle,
                                                  uint16 *aicsNum)
{
    VCP *vcp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcp_inst->aics_num)
    {
        /* If there are AICS instances, we have to copy their service handles in the aics_srvc_hndl pointer
         * we will return. We use the ptr pointer to parse the list of AICS servce handles in the memory instance,
         * and the pointer temp to scroll the returned memory and copy the service handles.
         */
        ServiceHandle *aics_srvc_hndl = PanicUnlessMalloc(sizeof(ServiceHandle) * (vcp_inst->aics_num));
        vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;
        ServiceHandle *temp = aics_srvc_hndl;

        while(ptr)
        {
            (*temp) = ptr->srvc_hdnl;
            temp++;
            ptr = ptr->next;
        }

        (*aicsNum) = vcp_inst->aics_num;

        return aics_srvc_hndl;
    }

    (*aicsNum) = 0;
    return NULL;
}

