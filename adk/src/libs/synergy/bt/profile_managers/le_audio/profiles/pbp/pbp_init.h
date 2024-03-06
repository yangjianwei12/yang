/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef PBP_INIT_H_
#define PBP_INIT_H_

#include "pbp_private.h"

/***************************************************************************
NAME
    pbpSendInitCfm
    
DESCRIPTION
    Send a PBP_INIT_CFM message to the application.
*/
void pbpSendInitCfm(PBP * pbpInst, PbpStatus status);

#endif
