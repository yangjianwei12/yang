/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_private.h"


bool gattRootKeyServerSendChallengeIndication(const GATT_ROOT_KEY_SERVER *instance,
                                              uint32 conn_id, uint8 opcode, const GRKS_KEY_T *key);

void handleRootKeyIndicationCfm(GATT_ROOT_KEY_SERVER *instance,
                                const CsrBtGattEventSendCfm *payload);

