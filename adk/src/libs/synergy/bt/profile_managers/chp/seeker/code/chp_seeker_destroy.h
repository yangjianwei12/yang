/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef CHP_SEEKER_DESTROY_H_
#define CHP_SEEKER_DESTROY_H_

#include "chp_seeker_private.h"

/***************************************************************************
NAME
    chpSeekerSendDestroyCfm
    
DESCRIPTION
    Send the CHP_SEEKER_DESTROY_CFM message.
*/
void chpSeekerSendDestroyCfm(CHP *chpSeekerInst,
                             ChpSeekerStatus status);

/***************************************************************************
NAME
    chpSeekerHandleTdsClientTerminateResp

DESCRIPTION
    Handle the GATT_TDS_CLIENT_TERMINATE_CFM message.
*/
void chpSeekerHandleTdsClientTerminateResp(CHP *chpSeekerInst,
                                           const GattTdsClientTerminateCfm * message);

/***************************************************************************
NAME
    chpSeekerDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void chpSeekerDestroyProfileInst(CHP *chpSeekerInst);

#endif
