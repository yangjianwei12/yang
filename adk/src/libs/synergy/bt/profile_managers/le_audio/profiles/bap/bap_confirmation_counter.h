/*******************************************************************************

Copyright (C) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Confirmation Counter.
 */

#ifndef BAP_CONFIRMATION_CONTER_H
#define BAP_CONFIRMATION_CONTER_H

#include "bap_client_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8 nExpectedCfm;
    uint8 nReceivedCfm;
    uint8 nSuccessfulCfm;
    uint8 nMinSuccessfulCfm;
/*    BapResult failure_error_code;*/
} BapConfirmationCounter;


void bapConfirmationCounterInitialise(BapConfirmationCounter * cfmCounter,
                                      uint8 numberOfConfirmationsExpected,
                                      uint8 minimumSuccessfulConfirmations);

/*
 * The 'reset' happens to do the same thing as the 'initialise', however they are conceptually
 * different; the 'initialise' (like the 'initialise' of other classes), should only ever be
 * called once for a particular instance (it's helpful when navigating the code and finding out
 * where instances were created and what their vtables were initialised with etc.).
 */
#define bapConfirmationCounterReset(cfmCounter,                                      \
                                    numberOfConfirmationsExpected,                 \
                                    minimumSuccessfulConfirmations)                 \
    bapConfirmationCounterInitialise((cfmCounter),                                   \
                                     (numberOfConfirmationsExpected),              \
                                     (minimumSuccessfulConfirmations))

void bapConfirmationCounterReceivedConfirmation(BapConfirmationCounter * cfmCounter,
                                                bool confirmation_successful);

#define bapConfirmationCounterAllConfirmationsHaveBeenReceived(cfmCounter)       \
    ((cfmCounter)->nReceivedCfm >= (cfmCounter)->nExpectedCfm)


#define bapConfirmationCounterAggregateConfirmationResultIsSuccess(cfmCounter)   \
    ((cfmCounter)->nSuccessfulCfm >= (cfmCounter)->nMinSuccessfulCfm)



#ifdef __cplusplus
}
#endif

#endif /* BAP_CODEC_SUBRECORD_H */
