/* Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_read.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_external_msg_send.h"

void gattAmsReadCharacteristic(GAMS *ams, uint16 handle)
{
    PRINT(("AMS: Read characteristic handle [0x%04x]\n", handle));
    GattReadReqSend(ams->gattId, ams->cid, handle, 0);
}

void gattAmsHandleReadCharacteristicValueCfmMsg(GAMS * ams, const CsrBtGattReadCfm *cfm)
{
    PRINT(("AMS: Read characteristic value response handle [0x%04x], status [0x%04x], supplier [0x%04x]\n", cfm->handle, cfm->resultCode, cfm->resultSupplier));
    gattAmsSendReadCharacteristicResponse(ams, cfm->resultCode, cfm->valueLength, cfm->value);
}
