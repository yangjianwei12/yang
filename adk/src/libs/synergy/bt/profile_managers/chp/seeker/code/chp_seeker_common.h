/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CHP_SEEKER_COMMON_H_
#define CHP_SEEKER_COMMON_H_

#include "chp_seeker_private.h"

/***************************************************************************
NAME
    startControlPointTimer

DESCRIPTION
    Start a timer when ATT_WRITE response is received for activate transport API
*/
void startControlPointTimer(CHP *chpSeekerInst);

/***************************************************************************
NAME
    stopControlPointTimer

DESCRIPTION
    Stop a timer when ATT_IND is received for activate transport API
*/
void stopControlPointTimer(void);

#endif
