/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef CHP_SEEKER_INDICATION_H_
#define CHP_SEEKER_INDICATION_H_

#include "gatt_tds_client.h"
#include "chp_seeker_private.h"

void chpSeekerHandleTdsClientIndResp(CHP* chpSeekerInst,
                                     const GattTdsClientIndicationCfm *msg);


void chpSeekerHandleTdsClientControlPointInd(CHP* chpSeekerInst,
                                             const GattTdsClientTdsCPAttributeInd *msg);
#endif
