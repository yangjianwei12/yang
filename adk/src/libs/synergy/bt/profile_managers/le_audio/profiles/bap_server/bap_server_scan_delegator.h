/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_SERVER_SCAN_DELEGATOR_H_
#define BAP_SERVER_SCAN_DELEGATOR_H_

#include "gatt_bass_server.h"

#define SCAN_DELEGATOR_BROADCAST_CODE_SIZE  (16)

/*
NAME
    bapMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the Server role.
*/
void bapServerHandleGattBassServerMsg(BAP *bapInst, void *message);

#endif
