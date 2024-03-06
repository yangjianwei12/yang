/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_write.h"
#include "ccp_debug.h"
#include "ccp_private.h"
#include "ccp_common.h"
#include "ccp_init.h"


/****************************************************************************/
void CcpWriteSignalStrengthIntervalRequest(const CcpProfileHandle profileHandle,
                                                const uint8 interval,
                                                bool writeWithoutResponse)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
/*TODO queue message
        ccpTbsSendInternalMsg(ccp_inst, ccp_xxx_op, 0);
        */
        GattTelephoneBearerClientWriteSignalStrengthIntervalRequest(ccp_inst->tbs_srvc_hdl,
                                                                    interval,
                                                                    writeWithoutResponse);
    }
    else
    {
       CCP_DEBUG("Invalid profile handle\n");
    }
}


void CcpWriteCallControlPointSimpleRequest(const CcpProfileHandle profileHandle,
                                                const GattTbsOpcode opcode,
                                                const uint8 callIndex)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        /* queue message
        ccpTbsSendInternalMsg(ccp_inst, ccp_xxxx_op, 0);
        */
        GattTelephoneBearerClientWriteCallControlPointSimpleRequest(ccp_inst->tbs_srvc_hdl,
                                                                    opcode,
                                                                    callIndex);
    }
    else
    {
       CCP_DEBUG("Invalid profile handle\n");
    }
}

void CcpWriteCallControlPointRequest(const CcpProfileHandle profileHandle,
                                        const GattTbsOpcode opcode,
                                        const uint8 size,
                                        const uint8* param)
{
    CCP *ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        /* queue message
        ccpTbsSendInternalMsg(ccp_inst, ccp_accept_op, 0);
        */
        GattTelephoneBearerClientWriteCallControlPointRequest(ccp_inst->tbs_srvc_hdl,
                                                              opcode,
                                                              size,
                                                              param);
    }
    else
    {
       CCP_DEBUG("Invalid profile handle\n");
    }
}

void ccpTbsControlPointOp(ServiceHandle profileHandle,
                             const CcpInternalWriteCallControlPointCfm *msg)
{
    CSR_UNUSED(profileHandle);
    CSR_UNUSED(msg);
}


