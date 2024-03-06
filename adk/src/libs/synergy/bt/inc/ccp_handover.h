/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CCP_HANDOVER_H_
#define CCP_HANDOVER_H_

#include "service_handle.h"

/***************************************************************************
NAME
    CcpHandoverVeto

DESCRIPTION
    Veto the handover of CCP Profile data.

    @return TRUE if the module wishes to veto the handover attempt.
*/

bool CcpHandoverVeto(ServiceHandle ccpProfileHandle);


#endif
