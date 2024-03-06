/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#ifndef __CSR_BT_TD_DB_SC_H_
#define __CSR_BT_TD_DB_SC_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_addr.h"
#include "csr_bt_profiles.h"
#include "dm_prim.h"
#include "csr_bt_td_db.h"


#define CSR_BT_TD_DB_SC_KEY_BREDR_KEY           0
#define CSR_BT_TD_DB_SC_KEY_LE_KEYS             1


#define CSR_BT_TD_DB_LE_KEY_NONE                (0x00)
#define CSR_BT_TD_DB_LE_KEY_ENC_CENTRAL         (0x01)
#define CSR_BT_TD_DB_LE_KEY_ID                  (0x02)
#define CSR_BT_TD_DB_LE_KEY_SIGN                (0x04)
#define CSR_BT_TD_DB_LE_KEY_DIV                 (0x08)

typedef struct
{
    CsrUint8                linkkeyType;
    CsrUint8                linkkeyLen;
    CsrBtDeviceLinkkey      linkkey;
    CsrBool                 authorised;
    CsrUint8                pad;
} CsrBtTdDbBredrKey;

typedef DM_SM_KEY_ENC_CENTRAL_T CsrBtScLeKeyEncCentral;
typedef DM_SM_KEY_ID_T CsrBtScLeKeyId;
typedef DM_SM_KEY_SIGN_T CsrBtScLeKeySign;

typedef struct
{
    CsrUint16               keyValid:4;
    CsrUint16               rpaOnlyPresent:1;
    CsrUint16               keySize:11;
    CsrUint16               secReq;
    CsrBtScLeKeyEncCentral  encCentral;
    CsrUint16               div;
    CsrBtScLeKeyId          id;
    CsrBtScLeKeySign        sign;
} CsrBtTdDbLeKeys;

CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(CsrBtTdDbBredrKey);
CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(CsrBtTdDbLeKeys);

#define CSR_BT_TD_DB_BREDR_KEY_VALID(_bredrKey) (_bredrKey->linkkeyType == DM_SM_LINK_KEY_NONE)

#define CsrBtTdDbSetBredrKey(_addressType, _deviceAddr, _bredrKey)          \
    CsrBtTdDbWriteEntry(_addressType,                                       \
                        _deviceAddr,                                        \
                        CSR_BT_TD_DB_SOURCE_SC,                             \
                        CSR_BT_TD_DB_SC_KEY_BREDR_KEY,                      \
                        sizeof(CsrBtTdDbBredrKey),                          \
                        _bredrKey)

#define CsrBtTdDbGetBredrKey(_addressType, _deviceAddr, _bredrKey)          \
    CsrBtTdDbGetEntry(_addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_TD_DB_SOURCE_SC,                               \
                      CSR_BT_TD_DB_SC_KEY_BREDR_KEY,                        \
                      sizeof(CsrBtTdDbBredrKey),                            \
                      _bredrKey)

#define CsrBtTdDbSetLeKeys(_addressType, _deviceAddr, _leKeys)              \
    CsrBtTdDbWriteEntry(_addressType,                                       \
                        _deviceAddr,                                        \
                        CSR_BT_TD_DB_SOURCE_SC,                             \
                        CSR_BT_TD_DB_SC_KEY_LE_KEYS,                        \
                        sizeof(CsrBtTdDbLeKeys),                            \
                        _leKeys)

#define CsrBtTdDbGetLeKeys(_addressType, _deviceAddr, _leKeys)              \
    CsrBtTdDbGetEntry(_addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_TD_DB_SOURCE_SC,                               \
                      CSR_BT_TD_DB_SC_KEY_LE_KEYS,                          \
                      sizeof(CsrBtTdDbLeKeys),                              \
                      _leKeys)

#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_TD_DB_SC_H_ */
