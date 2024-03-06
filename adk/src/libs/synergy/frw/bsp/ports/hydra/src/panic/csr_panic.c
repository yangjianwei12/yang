/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_panic.h"

#include "platform/csr_hydra_panic.h"
#include "csr_types.h"
#include "csr_macro.h"

void CsrPanic(CsrUint8 tech, CsrUint16 reason, const char *p)
{
    CSR_UNUSED(p);
    CsrUint32 diatribe = (tech << 16) | reason;
    panic_diatribe(CSR_HYDRA_PANIC_ID_SYNERGY, diatribe);
}
