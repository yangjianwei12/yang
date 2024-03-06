/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
 ******************************************************************************/
#ifndef __TD_DB_MAIN_H_
#define __TD_DB_MAIN_H_

#include "csr_synergy.h"
#include "csr_bt_td_db.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum TD DB devices supported */
#ifdef INSTALL_EXTENDED_TD_DB
#ifndef CSR_BT_TD_DB_LIST_MAX
#define CSR_BT_TD_DB_LIST_MAX               12
#endif
#else /* !INSTALL_EXTENDED_TD_DB */
#ifndef CSR_BT_TD_DB_LIST_MAX
#define CSR_BT_TD_DB_LIST_MAX               8
#endif
#endif /* INSTALL_EXTENDED_TD_DB */

/* Key count */
#ifdef INSTALL_EXTENDED_TD_DB
#define CSR_BT_TD_DB_KEYCOUNT_GAP           0
#else
#define CSR_BT_TD_DB_KEYCOUNT_GAP           2   /* Device info, BrEdr Service list */
#endif /* INSTALL_EXTENDED_TD_DB */
#define CSR_BT_TD_DB_KEYCOUNT_SC            2   /* BREDR key, LE keys */
#define CSR_BT_TD_DB_KEYCOUNT_GATT          1   /* GATT attributes */
#define CSR_BT_TD_DB_KEYCOUNT_DIRECT        1   /* Direct attributes */
#define CSR_BT_TD_DB_KEYCOUNT_GATT_CACHING  1

#define CSR_BT_TD_DB_FILE_BASE          ((CsrUint32) (0xFFFF0000u))

#ifdef CSR_BT_LE_ENABLE
#define CSR_BT_TD_DB_VER_BIT_LE         (1 << 0)
#else
#define CSR_BT_TD_DB_VER_BIT_LE         (0)
#endif

#ifdef CSR_BT_LE_SIGNING_ENABLE
#define CSR_BT_TD_DB_VER_BIT_SIGN       (1 << 1)
#else
#define CSR_BT_TD_DB_VER_BIT_SIGN       (0)
#endif

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
#define CSR_BT_TD_DB_VER_BIT_PRIVACY    (1 << 2)
#else
#define CSR_BT_TD_DB_VER_BIT_PRIVACY    (0)
#endif

#ifdef INSTALL_EXTENDED_TD_DB
#define TD_DB_VER_BIT_EXTENDED_TD_DB    (1 << 3)
#else
#define TD_DB_VER_BIT_EXTENDED_TD_DB    (0)
#endif

#ifdef CSR_BT_GATT_CACHING
#define CSR_BT_TD_DB_VER_BIT_CACHING    (1 << 4)
#else
#define CSR_BT_TD_DB_VER_BIT_CACHING    (0)
#endif

/**************************** IMPORTANT NOTICE *********************************
 * Increase the version number if structures/identifiers are changed/added/removed.
 ******************************************************************************/
#define CSR_BT_TD_DB_FILE_VERSION       (CSR_BT_TD_DB_FILE_BASE |            \
                                         CSR_BT_TD_DB_VER_BIT_LE |           \
                                         CSR_BT_TD_DB_VER_BIT_SIGN |         \
                                         CSR_BT_TD_DB_VER_BIT_PRIVACY |      \
                                         TD_DB_VER_BIT_EXTENDED_TD_DB |      \
                                         CSR_BT_TD_DB_VER_BIT_CACHING)

/* Internal API Prototypes which are required to be implemented by different TDDB stores
 * (e.g. PS, MEM, SQLITE etc.) if needed. */
CsrBool TdDbRestructureData(CsrUint32 version, CsrBtTdDbSystemInfo *systemInfo);

#ifdef __cplusplus
}
#endif

#endif /* __TD_DB_MAIN_H_ */
