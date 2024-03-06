/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#ifndef __CSR_BT_GATT_TDDB_UTILS_H_
#define __CSR_BT_GATT_TDDB_UTILS_H_

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_addr.h"
#include "csr_bt_profiles.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_td_db_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_BT_GATT_CACHING
#define CSR_BT_DB_HASH_SIZE (16)

#define CSR_BT_GATT_PS_CHANGE_UNAWARE_MASK       0x01 /* Change unaware TRUE/FALSE*/
#define CSR_BT_GATT_PS_ROBUST_CACHING_MASK       0x02 /* Robust caching enabled by remote device */
#define CSR_BT_GATT_PS_SERVICE_CHANGE_MASK       0x04 /* Service change enabled by remote device */

/* This function initialises the database and log handle.*/
void CsrBtGattCachingDbInit(void);

/* Get the stored database hash from PS */
CsrBtResultCode CsrBtGattCachingDbHashRead(CsrUint8 hash[CSR_BT_DB_HASH_SIZE]);

/* Store the generated database hash into PS */
CsrBtResultCode  CsrBtGattCachingDbHashWrite(const CsrUint8 hash[CSR_BT_DB_HASH_SIZE]);
#endif

#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
/* Read GATT Caching/EATT info for a specific device address */
CsrBtResultCode CsrBtGattTdDbReadFeatureInfo(CsrBtTypedAddr * deviceAddr, CsrBtGattFeatureInfo * info);
/* Store GATT Caching/EATT info for a specific device address */
CsrBtResultCode CsrBtGattTdDbWriteFeatureInfo(CsrBtTypedAddr * deviceAddr, CsrBtGattFeatureInfo info);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_GATT_TDDB_UTILS_H_ */
