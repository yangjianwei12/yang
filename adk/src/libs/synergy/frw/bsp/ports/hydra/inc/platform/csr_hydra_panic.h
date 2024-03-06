#ifndef CSR_HYDRA_PANIC__
#define CSR_HYDRA_PANIC__

/*****************************************************************************
Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
*****************************************************************************/

#include "csr_hydra_types.h"
#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif


#if defined(HYDRA) || defined(CAA)

#include "panic/panic.h"

#define CSR_HYDRA_PANIC_ID_SYNERGY PANIC_SYNERGY_FAILURE

#else

#define PANIC_SYNERGY_FAILURE               0xF7        /* Defined in panicids.h */

extern void panic_diatribe(CsrHydraPanicId deathbed_confession,
                           uint32 diatribe);

typedef enum
{
    CSR_HYDRA_PANIC_ID_SYNERGY = PANIC_SYNERGY_FAILURE
} CsrHydraPanicId;

#endif

#ifdef __cplusplus
}
#endif


#endif /* CSR_HYDRA_PANIC__ */
