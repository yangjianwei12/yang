/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CHP_SEEKER_INIT_H_
#define CHP_SEEKER_INIT_H_

#include "chp_seeker_private.h"

/***************************************************************************
NAME
    chpSeekerSendInitCfm
    
DESCRIPTION
    Send a CHP_SEEKER_INIT_CFM message to the application.
*/
void chpSeekerSendInitCfm(CHP * chpSeekerInst, ChpSeekerStatus status);


/***************************************************************************
NAME
    chpSeekerHandleTdsInitResp

DESCRIPTION
    Handle the GATT_TDS_CLIENT_INIT_CFM message.
*/
void chpSeekerHandleTdsClientInitResp(CHP *chpSeekerInst,
                                      const GattTdsClientInitCfm * message);

#endif
