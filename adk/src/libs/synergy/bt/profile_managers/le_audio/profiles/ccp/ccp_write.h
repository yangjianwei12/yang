/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_WRITE_H_
#define CCP_WRITE_H_

#include "ccp_private.h"






/***************************************************************************
NAME
    ccpTbsControlPointOp

DESCRIPTION
    Perform the specified TBS control point operation.
*/
void ccpTbsControlPointOp(ServiceHandle profileHandle,
                             const CcpInternalWriteCallControlPointCfm *msg);




#endif
