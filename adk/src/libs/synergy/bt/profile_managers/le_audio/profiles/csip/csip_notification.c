/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <stdlib.h>

#include "gatt_csis_client.h"

#include "csip.h"
#include "csip_debug.h"
#include "csip_private.h"
#include "csip_notification.h"

/***************************************************************************/
void csipSendCsisSetNtfCfm(Csip *csipInst, CsrBtResultCode status)
{
    CsipCsSetNtfCfm *message =
            (CsipCsSetNtfCfm *)CsrPmemZalloc(sizeof (CsipCsSetNtfCfm));

    message->id = CSIP_CS_SET_NTF_CFM;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->status = status;

    CsipMessageSend(csipInst->app_task, message);
}


/****************************************************************************/
void CsipCSRegisterForNotificationReq(CsipProfileHandle profileHandle,
                                      CsipCsInfo csInfoType,
                                      bool notificationsEnable)
{
    Csip *csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (csipInst)
    {
        GattCsisRegisterForNotification(csipInst->csisSrvcHdl,
                                       (GattCsisClientCsInfo)csInfoType,
                                                        notificationsEnable);
    }
    else
    {
        CSIP_ERROR("Invalid profile_handle\n");
    }
}


