/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_INIT_H_
#define CCP_INIT_H_

#include <gatt_telephone_bearer_client.h>

#include "ccp_private.h"

/***************************************************************************
NAME
    ccpSendInitCfm
    
DESCRIPTION
    Send a CCP_INIT_CFM message to the application.
*/
void ccpSendInitCfm(CCP * ccp_inst, CcpStatus status);


/***************************************************************************
NAME
    ccpHandleTbsClientInitResp

DESCRIPTION
    Handle the GATT_TBS_CLIENT_INIT_CFM message.
*/
void ccpHandleTbsClientInitResp(CCP *ccp_inst,
                                const GattTelephoneBearerClientInitCfm * message);



#endif
