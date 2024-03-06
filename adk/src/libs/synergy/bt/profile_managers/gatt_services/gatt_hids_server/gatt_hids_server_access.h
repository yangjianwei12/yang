/* Copyright (c) 2023 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_HIDS_SERVER_ACCESS_H_
#define GATT_HIDS_SERVER_ACCESS_H_

#include "gatt_hids_server.h"
#include "gatt_hids_server_private.h"
#include "csr_list.h"

void handleHidsServerAccess(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind);
void hidsServerHandleInputReport(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind, uint8 index);
void hidsServerHandleFeatureReport(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind, uint8 index);
HidsClientDataElement *hidsFindClient(CsrCmnList_t *connectedClients, ConnectionId cid);
void hidsServerHandleWriteClientConfigForInputReport(GHIDS *hids_server, CsrBtGattDbAccessWriteInd *const accessInd);
void hidsServerHandleAccessWriteIndication(GHIDS *hids_server, CsrBtGattDbAccessWriteInd *const accessInd);
#endif
