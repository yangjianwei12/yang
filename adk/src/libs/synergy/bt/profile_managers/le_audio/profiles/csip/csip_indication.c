/* Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "csip.h"
#include "csip_indication.h"

/****************************************************************************/
void csipHandleLockStatusInd(Csip *csipInst, uint8 lockStatus)
{
    CsipLockStatusInd *message =
            (CsipLockStatusInd *)CsrPmemZalloc(sizeof (CsipLockStatusInd));

    message->id = CSIP_LOCK_STATUS_IND;

    message->prflHndl = csipInst->csipSrvcHdl;
    message->lockStatus = lockStatus;

    CsipMessageSend(csipInst->app_task, message);
}

void csipHandleSizeChangedInd(Csip *csipInst, uint8 sizeValue)
{
    CsipSizeChangedInd *message =
            (CsipSizeChangedInd *)CsrPmemZalloc(sizeof (CsipSizeChangedInd));

    message->id = CSIP_SIZE_CHANGED_IND;

    message->prflHndl = csipInst->csipSrvcHdl;
    message->sizeValue = sizeValue;

    CsipMessageSend(csipInst->app_task, message);
}


void csipHandleSirkChangedInd(Csip *csipInst, uint8 *sirk)
{
    CsipSirkChangedInd *message =
            (CsipSirkChangedInd *)CsrPmemZalloc(sizeof (CsipSirkChangedInd));
    message->id = CSIP_SIRK_CHANGED_IND;

    message->prflHndl = csipInst->csipSrvcHdl;
    if(sirk)
    {
        memcpy(message->sirkValue, sirk, CSIP_SIRK_SIZE_PLUS_TYPE);
    }

    CsipMessageSend(csipInst->app_task, message);
}
 
