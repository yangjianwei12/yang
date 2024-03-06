/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_CLIENT_READ_H_
#define GATT_AMS_CLIENT_READ_H_

#include "gatt_ams_client.h"
#include <gatt_lib.h>

void gattAmsReadCharacteristic(GAMS *ams, uint16 handle);
void gattAmsHandleReadCharacteristicValueCfmMsg(GAMS * ams, const CsrBtGattReadCfm *msg);

#endif /* GATT_AMS_CLIENT_READ_H_ */
