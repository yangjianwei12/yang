/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_csis_client.h"

#include "csip.h"
#include "csip_write.h"
#include "csip_debug.h"
#include "csip_private.h"
#include "csip_init.h"

/***************************************************************************/
void csipSendSetLockCfm(Csip *csipInst,
                                CsrBtResultCode status)
{
    CsipSetLockCfm *message =
            (CsipSetLockCfm *)CsrPmemZalloc(sizeof(CsipSetLockCfm));

    message->id = CSIP_SET_LOCK_CFM;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->status = status;

    CsipMessageSend(csipInst->app_task, message);
}

void CsipSetLockRequest(CsipProfileHandle profileHandle,
                                                    bool lockEnable)
{
    Csip *csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (csipInst)
    {
        GattCsisWriteLockReq(csipInst->csisSrvcHdl, lockEnable);
    }
    else
    {
        CSIP_ERROR("Invalid profile_handle\n");
    }

}

