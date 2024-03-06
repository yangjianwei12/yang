/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#include "micp_common.h"
#include "micp_debug.h"

/*******************************************************************************/
void micpAddAicsInst(MICP *micp_inst,
                    GattAicsClientDeviceData *aics_handle,
                    uint16 start_handle,
                    uint16 end_handle)
{
    if (!micp_inst->first_aics_inst)
    {
        /* It's the first time I'm adding a new instance in the list */
        micp_inst->first_aics_inst = CsrPmemZalloc(sizeof(micp_aics_inst_t));

        micp_inst->first_aics_inst->cid = micp_inst->cid;
        CsrMemCpy(&(micp_inst->first_aics_inst->device_data), aics_handle, sizeof(GattAicsClientDeviceData));
        micp_inst->first_aics_inst->start_handle = start_handle;
        micp_inst->first_aics_inst->end_handle = end_handle;
        micp_inst->first_aics_inst->next = NULL;

        micp_inst->last_aics_inst = micp_inst->first_aics_inst;
    }
    else
    {
        /* There is alresy other elements in the list */
        micp_inst->last_aics_inst->next = CsrPmemZalloc(sizeof(micp_aics_inst_t));

        micp_inst->last_aics_inst->next->cid = micp_inst->cid;
        CsrMemCpy(&(micp_inst->last_aics_inst->next->device_data), aics_handle,sizeof(GattAicsClientDeviceData));
        micp_inst->first_aics_inst->next->start_handle = start_handle;
        micp_inst->first_aics_inst->next->end_handle = end_handle;
        micp_inst->last_aics_inst->next->next = NULL;

        micp_inst->last_aics_inst = micp_inst->last_aics_inst->next;
    }
}

/****************************************************************************/
bool micpIsValidAicsInst(MICP *micp_inst, ServiceHandle srvc_hndl)
{
    micp_aics_srvc_hndl_t *ptr = micp_inst->first_aics_srvc_hndl;

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
static void MicpDestroyReqAicsSrvcHndlList(MICP *micp_inst)
{
    micp_aics_srvc_hndl_t *ptr = micp_inst->first_aics_srvc_hndl;
    micp_aics_srvc_hndl_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        CsrPmemFree(ptr);
        ptr = tmp;
    }

    micp_inst->last_aics_srvc_hndl = NULL;
    micp_inst->first_aics_srvc_hndl = NULL;
}

/****************************************************************************/
void micpDestroyReqAllSrvcHndlList(MICP *micp_inst)
{
    if (micp_inst->first_aics_srvc_hndl && micp_inst->last_aics_srvc_hndl)
    {
        /* There is a list of AICS service handle: we need to destroy it */
        MicpDestroyReqAicsSrvcHndlList(micp_inst);
        micp_inst->first_aics_srvc_hndl = NULL;
    }
}

/****************************************************************************/
void micpAddElementAicsSrvcHndlList(MICP *micp_inst,
                                          ServiceHandle element)
{
    if (!micp_inst->first_aics_srvc_hndl)
    {
        /* it's the first time a AICS service is initialised */
        micp_inst->first_aics_srvc_hndl = CsrPmemZalloc(sizeof(micp_aics_srvc_hndl_t));

        micp_inst->first_aics_srvc_hndl->srvc_hdnl = element;
        micp_inst->first_aics_srvc_hndl->next = NULL;

        micp_inst->last_aics_srvc_hndl = micp_inst->first_aics_srvc_hndl;
    }
    else
    {
        /* There are already other initialised AICS instances */
        micp_inst->last_aics_srvc_hndl->next = CsrPmemZalloc(sizeof(micp_aics_srvc_hndl_t));

        micp_inst->last_aics_srvc_hndl->next->srvc_hdnl = element;
        micp_inst->last_aics_srvc_hndl->next->next = NULL;

        micp_inst->last_aics_srvc_hndl = micp_inst->last_aics_srvc_hndl->next;
    }

}

/****************************************************************************/
static void MicpDestroyReqAicsInstList(MICP *micp_inst)
{
    micp_aics_inst_t *ptr = micp_inst->first_aics_inst;
    micp_aics_inst_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        CsrPmemFree(ptr);
        ptr = tmp;
    }

    micp_inst->last_aics_inst = NULL;
    micp_inst->first_aics_inst = NULL;
}

/****************************************************************************/
void MicpDestroyReqAllInstList(MICP *micp_inst)
{
    if (micp_inst->first_aics_inst && micp_inst->last_aics_inst)
    {
        /* There is a list of AICS instances to initialise: we need to destroy it */
        MicpDestroyReqAicsInstList(micp_inst);
        micp_inst->first_aics_inst = NULL;
    }
}

GattMicsClientDeviceData *MicpGetAttributeHandles(MicpProfileHandle profileHandle)
{
    MICP *micpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micpInst)
    {
        return GattMicsClientGetHandles(micpInst->mics_srvc_hdl);
    }

    return NULL;
}

