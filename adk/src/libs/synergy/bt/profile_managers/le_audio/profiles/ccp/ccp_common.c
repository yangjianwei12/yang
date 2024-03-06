/* Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "ccp_common.h"
#include "ccp_debug.h"

/****************************************************************************/
void ccpDestroyReqTbsInstList(CCP *ccp_inst)
{
    ccp_tbs_inst_t *ptr = ccp_inst->first_tbs_inst;
    ccp_tbs_inst_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    ccp_inst->last_tbs_inst = NULL;
    ccp_inst->first_tbs_inst = NULL;
}

/****************************************************************************/
void ccpDestroyReqTbsSrvcHndlList(CCP *ccp_inst)
{
    ccp_tbs_srvc_hndl_t *ptr = ccp_inst->first_tbs_srvc_hndl;
    ccp_tbs_srvc_hndl_t *tmp = NULL;

    while(ptr)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }

    ccp_inst->last_tbs_srvc_hndl = NULL;
    ccp_inst->first_tbs_srvc_hndl = NULL;
}

/****************************************************************************/
void ccpDestroyReqAllSrvcHndlList(CCP *ccp_inst)
{
    if (!ccp_inst->first_tbs_srvc_hndl && !ccp_inst->last_tbs_srvc_hndl)
    {
        /* There is a list of TBS service handle: we need to destroy it */
        ccpDestroyReqTbsSrvcHndlList(ccp_inst);
        ccp_inst->first_tbs_srvc_hndl = NULL;
    }
}


/****************************************************************************/
void ccpDestroyReqAllInstList(CCP *ccp_inst)
{
    if (ccp_inst->first_tbs_inst && ccp_inst->last_tbs_inst)
    {
        /* There is a list of TBS instances to initialise: we need to destroy it */
        ccpDestroyReqTbsInstList(ccp_inst);
    }
}

/****************************************************************************/
GattTelephoneBearerClientDeviceData *CcpGetTelephoneBearerAttributeHandles(CcpProfileHandle profileHandle,
                                                               ServiceHandle tbsHandle)
{
    GattTelephoneBearerClientDeviceData *dev_data = NULL;
    CCP *ccpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    CSR_UNUSED(tbsHandle);

    if (ccpInst)
    {
        if (ccpInst->tbs_srvc_hdl)
        {
            dev_data = GattTelephoneBearerClientGetHandlesReq(ccpInst->tbs_srvc_hdl);

            if (dev_data != NULL)
            {
                dev_data->startHandle = ccpInst->gtbs_start_handle;
                dev_data->endHandle = ccpInst->gtbs_end_handle;
            }
        }
    }
    else
    { /* In case debug lib is not defined, the following macro won't cause panic so a return would be
         required for the function */
        CCP_PANIC("CCP: Invalid profile_handle\n");
    }

    return dev_data;
}

