/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#ifndef __CSR_BT_TD_DB_GATT_H_
#define __CSR_BT_TD_DB_GATT_H_

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_addr.h"
#include "csr_bt_td_db.h"
#include "csr_bt_profiles.h"
#include "csr_bt_gatt_prim.h"


#ifdef __cplusplus
extern "C" {
#endif

#define CSR_BT_TD_DB_GATT_KEY_GATT_INFO                     0

#define CSR_BT_TD_DB_GATT_CONFIG_INDICATE_SERVICE_CHANGE    (1 << 0)
#define CSR_BT_TD_DB_GATT_CONFIG_PEER_SERVICE_UNCHANGED     (1 << 1)

typedef struct
{
    CsrUint8                config;

    /* Local database version reported to peer device */
    CsrUint32               localDbVersion;

    /* Peer device ATT handles */
    CsrBtGattHandle         serviceChange;
    CsrBtGattHandle         serviceChangeClientConfig;

#if defined(CSR_BT_GATT_CACHING) || defined (CSR_BT_GATT_INSTALL_EATT)
    /* GATT Caching/EATT Enabled */
    CsrBtGattFeatureInfo    gattSuppFeatInfo;
#endif

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
    CsrUint8                carValue;
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
} CsrBtTdDbGattInfo;


CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(CsrBtTdDbGattInfo);

#define CsrBtTdDbSetGattInfo(_addressType, _deviceAddr, _gattInfo)              \
        CsrBtTdDbWriteEntry(_addressType,                                       \
                            _deviceAddr,                                        \
                            CSR_BT_TD_DB_SOURCE_GATT,                           \
                            CSR_BT_TD_DB_GATT_KEY_GATT_INFO,                    \
                            sizeof(CsrBtTdDbGattInfo),                          \
                            _gattInfo)
    
#define CsrBtTdDbGetGattInfo(_addressType, _deviceAddr, _gattInfo)              \
        CsrBtTdDbGetEntry(_addressType,                                         \
                          _deviceAddr,                                          \
                          CSR_BT_TD_DB_SOURCE_GATT,                             \
                          CSR_BT_TD_DB_GATT_KEY_GATT_INFO,                      \
                          sizeof(CsrBtTdDbGattInfo),                            \
                          _gattInfo)
#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_TD_DB_GATT_H_ */
