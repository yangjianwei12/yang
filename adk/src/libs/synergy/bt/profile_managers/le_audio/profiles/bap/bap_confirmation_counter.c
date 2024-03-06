/*******************************************************************************

Copyright (C) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "bap_confirmation_counter.h"


void bapConfirmationCounterInitialise(BapConfirmationCounter * cfmCounter,
                                      uint8 numberOfConfirmationsExpected,
                                      uint8 minimumSuccessfulConfirmations /*,
                                      BapResult failureErrorCode*/)
{
    cfmCounter->nReceivedCfm = 0;
    cfmCounter->nSuccessfulCfm = 0;
    cfmCounter->nMinSuccessfulCfm = minimumSuccessfulConfirmations;
    cfmCounter->nExpectedCfm = numberOfConfirmationsExpected;
/*    This->failure_error_code = failureErrorCode;*/
}

void bapConfirmationCounterReceivedConfirmation(BapConfirmationCounter * cfmCounter,
                                                bool confirmationSuccessful)
{
    if(cfmCounter->nExpectedCfm > 0)
    {
        cfmCounter->nReceivedCfm++;

        if (confirmationSuccessful)
        {
            cfmCounter->nSuccessfulCfm++;
        }
    }
}


