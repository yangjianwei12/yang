/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_DESTROY_H_
#define CCP_DESTROY_H_

#include "ccp_private.h"

/***************************************************************************
NAME
    ccpSendDestroyCfm
    
DESCRIPTION
    Send the CCP_DESTROY_CFM message.
*/
void ccpSendDestroyCfm(CCP * ccp_inst, CcpStatus status);

/***************************************************************************
NAME
    ccpSendTbsTerminateCfm

DESCRIPTION
    Send the CCP_TBS_TERMINATE_CFM message.
*/
void ccpSendTbsTerminateCfm(CCP *ccp_inst,
                            CcpStatus status,
                            GattTelephoneBearerClientDeviceData handles);

/***************************************************************************
NAME
    ccpDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void ccpDestroyProfileInst(CCP *ccp_inst);

/***************************************************************************
NAME
    ccpHandleTbsClientTerminateResp

DESCRIPTION
    Handle the GattTelephoneBearerClientTerminateCfm message.
*/
void ccpHandleTbsClientTerminateResp(CCP *ccp_inst,
                                     const GattTelephoneBearerClientTerminateCfm * message);



#endif
