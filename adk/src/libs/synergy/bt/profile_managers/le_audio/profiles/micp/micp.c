/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client.h"

#include "micp.h"
#include "micp_private.h"
#include "micp_init.h"
#include "micp_debug.h"
#include "micp_common.h"

/******************************************************************************/
ServiceHandle *MicpGetAicsServiceHandlesRequest(MicpProfileHandle profileHandle, uint16 *aicsNum)
{
    MICP *micp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (micp_inst && micp_inst->aics_num)
    {
        /* If there are AICS instances, we have to copy their service handles in the aics_srvc_hndl pointer
         * we will return. We use the ptr pointer to parse the list of AICS servce handles in the memory instance,
         * and the pointer temp to scroll the returned memory and copy the service handles.
         */
        ServiceHandle *aics_srvc_hndl = CsrPmemZalloc(sizeof(ServiceHandle) * (micp_inst->aics_num));
        micp_aics_srvc_hndl_t *ptr = micp_inst->first_aics_srvc_hndl;
        ServiceHandle *temp = aics_srvc_hndl;

        while(ptr)
        {
            (*temp) = ptr->srvc_hdnl;
            temp++;
            ptr = ptr->next;
        }

        (*aicsNum) = micp_inst->aics_num;

        return aics_srvc_hndl;
    }

    (*aicsNum) = 0;
    return NULL;
}


