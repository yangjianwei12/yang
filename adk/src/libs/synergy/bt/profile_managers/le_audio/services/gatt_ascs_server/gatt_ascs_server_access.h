/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ASCS_SERVER_ACCESS_H_
#define GATT_ASCS_SERVER_ACCESS_H_

#include "gatt_ascs_server.h"

/***************************************************************************
NAME
    handleAscsAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the ASCS library.
*/
void handleAscsAccess(GattAscsServer *ascsServer, const CsrBtGattAccessInd *accessInd);

#endif
