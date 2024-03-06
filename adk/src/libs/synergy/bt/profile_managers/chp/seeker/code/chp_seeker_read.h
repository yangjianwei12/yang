/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CHP_SEEKER_READ_H_
#define CHP_SEEKER_READ_H_

#include "chp_seeker_private.h"

void chpSeekerHandleTdsClientGetTdsAttributeResp(CHP *chpSeekerInst,
                                                 const GattTdsClientGetTdsAttributeCfm *msg);

#endif
