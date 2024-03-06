/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GATT_HIDS_SERVER_COMMON_H
#define GATT_HIDS_SERVER_COMMON_H

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_list.h"
#include "gatt_hids_server_private.h"
#include "csr_bt_gatt_prim.h"

#define GATT_HIDS_FEATURE_REPORT_DB_SIZE 3
#define GATT_HIDS_INPUT_REPORT_DB_SIZE   2
/* Required octets for the Client Configuration Characteristic */
#define GATT_HIDS_SERVER_CCC_VALUE_SIZE                         (2)

/**************************************************************************
NAME
    hidsServerSendAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void hidsServerSendAccessRsp(
        CsrBtGattId task,
        ConnectionId cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 *const value);


/**************************************************************************
NAME
    hidsServerSendAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library with an error status.
*/

#define hidsServerSendAccessErrorRsp(task, cid, handle, error) \
    sendHidsServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )




void hidsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 sizeValue,
        uint8 const *value);



/**************************************************************************
NAME
    sendHidsServerAccessErrorRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/

#define sendHidsServerAccessErrorRsp(task, cid, handle, error) \
    sendHidsServerAccessRsp( \
            task, \
            cid, \
            handle, \
            error, \
            0, \
            NULL \
            )

void sendHidsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 *value
        );
void gattHidsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        );
void hidsServerHidsInfoData(uint8 *hids_info, GHIDS *hids_server);
void hidsServerReportMapData(uint8 *report, GHIDS *hids_server);

void hidsServerReport(uint8 *report, GHIDS *hids_server, uint8 index);
void hidsServerFeatureReport(uint8 *report, GHIDS *hids_server, uint8 index);
void hidsServerHidsClientConfigData(uint8 *hids_info, GHIDS *hids_server, const CsrBtGattAccessInd *access_ind);
void hidsServerFeatureReportRefData(uint8 *hids_info, GHIDS *hids_server, uint8 index);
#endif /* GATT_HIDS_SERVER_COMMON_H */
