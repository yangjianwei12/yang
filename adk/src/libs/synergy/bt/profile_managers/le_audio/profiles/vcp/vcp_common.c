/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "vcp_common.h"
#include "vcp_debug.h"

/***************************************************************************/
void vcpAddVocsInst(VCP *vcp_inst,
                    GattVocsClientDeviceData *vocs_handle,
                    uint16 start_handle,
                    uint16 end_handle)
{
    if (!vcp_inst->first_vocs_inst)
    {
        /* It's the first time I'm adding a new instance in the list */
        vcp_inst->first_vocs_inst = CsrPmemZalloc(sizeof(vcp_vocs_inst_t));

        vcp_inst->first_vocs_inst->cid = vcp_inst->cid;
        memcpy(&(vcp_inst->first_vocs_inst->device_data), vocs_handle,sizeof(GattVocsClientDeviceData));
        vcp_inst->first_vocs_inst->start_handle = start_handle;
        vcp_inst->first_vocs_inst->end_handle = end_handle;
        vcp_inst->first_vocs_inst->next = NULL;

        vcp_inst->last_vocs_inst = vcp_inst->first_vocs_inst;
    }
    else
    {
        /* There is already other elements in the list */
        vcp_inst->last_vocs_inst->next = CsrPmemZalloc(sizeof(vcp_vocs_inst_t));

        vcp_inst->last_vocs_inst->next->cid = vcp_inst->cid;
        memcpy(&(vcp_inst->last_vocs_inst->next->device_data), vocs_handle,sizeof(GattVocsClientDeviceData));
        vcp_inst->first_vocs_inst->next->start_handle = start_handle;
        vcp_inst->first_vocs_inst->next->end_handle = end_handle;
        vcp_inst->last_vocs_inst->next->next = NULL;

        vcp_inst->last_vocs_inst = vcp_inst->last_vocs_inst->next;
    }
}

/*******************************************************************************/
void vcpAddAicsInst(VCP *vcp_inst,
                    GattAicsClientDeviceData *aics_handle,
                    uint16 start_handle,
                    uint16 end_handle)
{
    if (!vcp_inst->first_aics_inst)
    {
        /* It's the first time I'm adding a new instance in the list */
        vcp_inst->first_aics_inst = CsrPmemZalloc(sizeof(vcp_aics_inst_t));

        vcp_inst->first_aics_inst->cid = vcp_inst->cid;
        memcpy(&(vcp_inst->first_aics_inst->device_data), aics_handle, sizeof(GattAicsClientDeviceData));
        vcp_inst->first_aics_inst->start_handle = start_handle;
        vcp_inst->first_aics_inst->end_handle = end_handle;
        vcp_inst->first_aics_inst->next = NULL;

        vcp_inst->last_aics_inst = vcp_inst->first_aics_inst;
    }
    else
    {
        /* There is alresy other elements in the list */
        vcp_inst->last_aics_inst->next = CsrPmemZalloc(sizeof(vcp_aics_inst_t));

        vcp_inst->last_aics_inst->next->cid = vcp_inst->cid;
        memcpy(&(vcp_inst->last_aics_inst->next->device_data), aics_handle,sizeof(GattAicsClientDeviceData));
        vcp_inst->first_aics_inst->next->start_handle = start_handle;
        vcp_inst->first_aics_inst->next->end_handle = end_handle;
        vcp_inst->last_aics_inst->next->next = NULL;

        vcp_inst->last_aics_inst = vcp_inst->last_aics_inst->next;
    }
}

/****************************************************************************/
bool vcpIsValidVocsInst(VCP *vcp_inst, ServiceHandle srvc_hndl)
{
    vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;

    while(ptr)
    {
        if (ptr->srvc_hdnl == srvc_hndl)
        {
            return TRUE;
        }

        ptr = ptr->next;
    }

    return FALSE;
}

/****************************************************************************/
bool vcpIsValidAicsInst(VCP *vcp_inst, ServiceHandle srvc_hndl)
{
    vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;

    while(ptr)
    {
        if (ptr->srvc_hdnl == srvc_hndl)
        {
            return TRUE;
        }

        ptr = ptr->next;
    }

    return FALSE;
}

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
static void VcpDestroyReqAicsSrvcHndlList(VCP *vcp_inst)
{
    vcp_aics_srvc_hndl_t *ptr = vcp_inst->first_aics_srvc_hndl;
    vcp_aics_srvc_hndl_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    vcp_inst->last_aics_srvc_hndl = NULL;
    vcp_inst->first_aics_srvc_hndl = NULL;
}
#endif

/****************************************************************************/
void vcpDestroyReqAllSrvcHndlList(VCP *vcp_inst)
{
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    if (vcp_inst->first_vocs_srvc_hndl && vcp_inst->last_vocs_srvc_hndl)
    {
        /* There is a list of VOCS service handles: we need to destroy it */
        VcpDestroyReqVocsSrvcHndlList(vcp_inst);
    }
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    if (vcp_inst->first_aics_srvc_hndl && vcp_inst->last_aics_srvc_hndl)
    {
        /* There is a list of AICS service handle: we need to destroy it */
        VcpDestroyReqAicsSrvcHndlList(vcp_inst);
        vcp_inst->first_aics_srvc_hndl = NULL;
    }
#endif

#if (defined(EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE)) && (defined(EXCLUDE_CSR_BT_AICS_CLIENT_MODULE))
    CSR_UNUSED(vcp_inst);
#endif
}

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
void vcpAddElementAicsSrvcHndlList(VCP *vcp_inst,
                                          ServiceHandle element)
{
    if (!vcp_inst->first_aics_srvc_hndl)
    {
        /* it's the first time a AICS service is initialised */
        vcp_inst->first_aics_srvc_hndl = CsrPmemZalloc(sizeof(vcp_aics_srvc_hndl_t));

        vcp_inst->first_aics_srvc_hndl->srvc_hdnl = element;
        vcp_inst->first_aics_srvc_hndl->next = NULL;

        vcp_inst->last_aics_srvc_hndl = vcp_inst->first_aics_srvc_hndl;
    }
    else
    {
        /* There are already other initialised AICS instances */
        vcp_inst->last_aics_srvc_hndl->next = CsrPmemZalloc(sizeof(vcp_aics_srvc_hndl_t));

        vcp_inst->last_aics_srvc_hndl->next->srvc_hdnl = element;
        vcp_inst->last_aics_srvc_hndl->next->next = NULL;

        vcp_inst->last_aics_srvc_hndl = vcp_inst->last_aics_srvc_hndl->next;
    }

}
#endif

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/****************************************************************************/
void vcpAddElementVocsSrvcHndlList(VCP *vcp_inst,
                                   ServiceHandle element)
{
    if (!vcp_inst->first_vocs_srvc_hndl)
    {
        /* it's the first time a VOCS service is initialised */
        vcp_inst->first_vocs_srvc_hndl = CsrPmemZalloc(sizeof(vcp_vocs_srvc_hndl_t));

        vcp_inst->first_vocs_srvc_hndl->srvc_hdnl = element;
        vcp_inst->first_vocs_srvc_hndl->next = NULL;

        vcp_inst->last_vocs_srvc_hndl = vcp_inst->first_vocs_srvc_hndl;
    }
    else
    {
        /* There are already other initialised VOCS instances */
        vcp_inst->last_vocs_srvc_hndl->next = CsrPmemZalloc(sizeof(vcp_vocs_srvc_hndl_t));

        vcp_inst->last_vocs_srvc_hndl->next->srvc_hdnl = element;
        vcp_inst->last_vocs_srvc_hndl->next->next = NULL;

        vcp_inst->last_vocs_srvc_hndl = vcp_inst->last_vocs_srvc_hndl->next;
    }

}

/****************************************************************************/
void VcpDestroyReqVocsSrvcHndlList(VCP *vcp_inst)
{
    vcp_vocs_srvc_hndl_t *ptr = vcp_inst->first_vocs_srvc_hndl;
    vcp_vocs_srvc_hndl_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    vcp_inst->last_vocs_srvc_hndl = NULL;
    vcp_inst->first_vocs_srvc_hndl = NULL;
}
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
/****************************************************************************/
static void VcpDestroyReqAicsInstList(VCP *vcp_inst)
{
    vcp_aics_inst_t *ptr = vcp_inst->first_aics_inst;
    vcp_aics_inst_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    vcp_inst->last_aics_inst = NULL;
    vcp_inst->first_aics_inst = NULL;
}
#endif

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
/****************************************************************************/
static void VcpDestroyReqVocsInstList(VCP *vcp_inst)
{
    vcp_vocs_inst_t *ptr = vcp_inst->first_vocs_inst;
    vcp_vocs_inst_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    vcp_inst->last_vocs_inst = NULL;
    vcp_inst->first_vocs_inst = NULL;
}
#endif

/****************************************************************************/
void VcpDestroyReqAllInstList(VCP *vcp_inst)
{
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    if (vcp_inst->first_vocs_inst && vcp_inst->last_vocs_inst)
    {
        /* There is a list of VOCS instances to initialise: we need to destroy it */
        VcpDestroyReqVocsInstList(vcp_inst);
    }
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    if (vcp_inst->first_aics_inst && vcp_inst->last_aics_inst)
    {
        /* There is a list of AICS instances to initialise: we need to destroy it */
        VcpDestroyReqAicsInstList(vcp_inst);
        vcp_inst->first_aics_inst = NULL;
    }
#endif

#if (defined(EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE)) && (defined(EXCLUDE_CSR_BT_AICS_CLIENT_MODULE))
    CSR_UNUSED(vcp_inst);
#endif
}

GattVcsClientDeviceData *VcpGetAttributeHandles(VcpProfileHandle profileHandle)
{
    VCP *vcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (vcpInst)
    {
        return GattVcsClientGetHandlesReq(vcpInst->vcs_srvc_hdl);
    }

    return NULL;
}
