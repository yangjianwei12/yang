#ifndef CSR_RANDOM_H__
#define CSR_RANDOM_H__
/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "platform/csr_hydra_random.h"


#ifdef __cplusplus
extern "C" {
#endif


#define CsrRandomSeed()     NULL

/*******************************************************************************

    NAME
        CsrRandom

    DESCRIPTION
        Return a random number (uniform distribution). Before calling this
        function a seeded state must be obtained by calling CsrRandomSeed. The
        return value from CsrRandomSeed is passed as argument to CsrRandom any
        number of times to receive a sequence of random numbers. When no more
        random numbers are needed, the state is freed by a call to CsrPmemFree.

    PARAMETERS
        randomState - The seeded state returned by CsrRandomSeed.

    RETURNS
        A random number (uniform distribution)

*******************************************************************************/
#define CsrRandom(x)        ((void) (x), UtilRandom())

#ifdef __cplusplus
}
#endif

#endif
