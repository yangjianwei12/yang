/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#ifndef __CSR_BT_TD_DB_GAP_H_
#define __CSR_BT_TD_DB_GAP_H_

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_addr.h"
#include "csr_bt_td_db.h"
#include "csr_bt_profiles.h"


#ifdef __cplusplus
extern "C" {
#endif


#define CSR_BT_TD_DB_GAP_KEY_DEVICE_INFO        0
#define CSR_BT_TD_DB_GAP_KEY_SERVICES           1

typedef struct
{
    CsrBtClassOfDevice      classOfDevice;
    CsrBtDeviceName         name;
    CsrBtTransportMask      transportMask;
} CsrBtTdDbDeviceInfo;

typedef struct
{
    CsrUint32               knownServices[4];
} CsrBtTdDbServices;

CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(CsrBtTdDbDeviceInfo);
CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(CsrBtTdDbServices);

/* Below macro functions shall NOT be called if INSTALL_EXTENDED_TD_DB
 * is supported.
 */
#define CsrBtTdDbSetDeviceInfo(_addressType, _deviceAddr, _deviceInfo)      \
    CsrBtTdDbWriteEntry(_addressType,                                       \
                        _deviceAddr,                                        \
                        CSR_BT_TD_DB_SOURCE_GAP,                            \
                        CSR_BT_TD_DB_GAP_KEY_DEVICE_INFO,                   \
                        sizeof(CsrBtTdDbDeviceInfo),                        \
                        _deviceInfo)

#define CsrBtTdDbGetDeviceInfo(_addressType, _deviceAddr, _deviceInfo)      \
    CsrBtTdDbGetEntry(_addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_TD_DB_SOURCE_GAP,                              \
                      CSR_BT_TD_DB_GAP_KEY_DEVICE_INFO,                     \
                      sizeof(CsrBtTdDbDeviceInfo),                          \
                      _deviceInfo)


#define CsrBtTdDbSetServices(_addressType, _deviceAddr, _services)          \
    CsrBtTdDbWriteEntry(_addressType,                                       \
                        _deviceAddr,                                        \
                        CSR_BT_TD_DB_SOURCE_GAP,                            \
                        CSR_BT_TD_DB_GAP_KEY_SERVICES,                      \
                        sizeof(CsrBtTdDbServices),                          \
                        _services)

#define CsrBtTdDbGetServices(_addressType, _deviceAddr, _services)          \
    CsrBtTdDbGetEntry(_addressType,                                         \
                      _deviceAddr,                                          \
                      CSR_BT_TD_DB_SOURCE_GAP,                              \
                      CSR_BT_TD_DB_GAP_KEY_SERVICES,                        \
                      sizeof(CsrBtTdDbServices),                            \
                      _services)

#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_TD_GAP_H_ */
