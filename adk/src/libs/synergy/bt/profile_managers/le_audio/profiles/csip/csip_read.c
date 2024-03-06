/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_csis_client.h"

#include "csip.h"
#include "csip_debug.h"
#include "csip_private.h"
#include "csip_read.h"
#include "csip_write.h"
#include "csip_init.h"


/***************************************************************************/
void csipSendReadCSInfoCfm(Csip *csipInst,
                                     CsrBtResultCode status,
                                     CsipCsInfo csInfoType,
                                     uint16 sizeValue,
                                     const uint8 *value)
{
    CsipReadCsInfoCfm *message = (CsipReadCsInfoCfm *) CsrPmemZalloc(sizeof(CsipReadCsInfoCfm));

    message->id = CSIP_READ_CS_INFO_CFM;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->csInfoType = csInfoType;
    message->status = status;
    message->sizeValue = sizeValue;
    if (sizeValue)
    {
        message->value = pmalloc(sizeValue * sizeof(uint8));
        memcpy(message->value, value, sizeValue);
    }

    CsipMessageSend(csipInst->app_task, message);
}

/***************************************************************************/
void CsipReadCSInfoRequest(CsipProfileHandle profileHandle, CsipCsInfo csInfoType)
{
    Csip *csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (csipInst)
    {
        GattCsisReadCsInfoReq(csipInst->csisSrvcHdl,
                                    (GattCsisClientCsInfo)csInfoType);
    }
    else
    {
        CSIP_ERROR("Invalid profileHandle\n");
    }
}


