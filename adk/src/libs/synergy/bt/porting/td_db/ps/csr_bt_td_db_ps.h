/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef __CSR_BT_TD_DB_PS_H_
#define __CSR_BT_TD_DB_PS_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "csr_synergy.h"
#include "csr_bt_td_db.h"
#include "td_db_main.h"
#include "csr_log_text_2.h"
#ifdef INCLUDE_BT_WEARABLE_TD_DB_PS
#include "qapi_ps.h"

#ifndef SYNERGY_PS_BASE
#define SYNERGY_PS_BASE_MASK            0x20000000
#define SYNERGY_PS_BASE                 1
#endif /* !SYNERGY_PS_BASE */


#else

#ifndef SYNERGY_PS_BASE
#define SYNERGY_PS_BASE                 100
#endif /* !SYNERGY_PS_BASE */

#endif

CSR_LOG_TEXT_HANDLE_DECLARE(TdDbPsLto);

/* Rank is 4 bits so 0xF is used as invalid value */
#define TD_DB_DEVICE_RANK_INVALID       (0xF)
#define TD_DB_INVALID_VERSION           (0x0)

/* Common keys */
#define CSR_BT_PS_KEY_SYSTEM            SYNERGY_PS_BASE
#define CSR_BT_PS_KEY_INDEX             (CSR_BT_PS_KEY_SYSTEM + 1)

/* Source bases */
#define CSR_BT_TD_DB_BASE               (CSR_BT_PS_KEY_INDEX + 1)
#define CSR_BT_TD_DB_BASE_GAP           (CSR_BT_TD_DB_BASE)
#define CSR_BT_TD_DB_BASE_SC            (CSR_BT_TD_DB_BASE_GAP + (CSR_BT_TD_DB_KEYCOUNT_GAP * CSR_BT_TD_DB_LIST_MAX))
#define CSR_BT_TD_DB_BASE_GATT          (CSR_BT_TD_DB_BASE_SC + (CSR_BT_TD_DB_KEYCOUNT_SC * CSR_BT_TD_DB_LIST_MAX))
#define CSR_BT_TD_DB_BASE_DIRECT        (CSR_BT_TD_DB_BASE_GATT + (CSR_BT_TD_DB_KEYCOUNT_GATT * CSR_BT_TD_DB_LIST_MAX))

#define SYNERGY_PS_BASE_TD_MAX          (CSR_BT_TD_DB_BASE_DIRECT + (CSR_BT_TD_DB_KEYCOUNT_DIRECT * CSR_BT_TD_DB_LIST_MAX) - 1)

/* Total of 50 keys are allocated for TD DB usage */
#define SYNERGY_PS_NUM_KEYS             (50)
#define SYNERGY_PS_BASE_TD_RANGE_END    (SYNERGY_PS_BASE + SYNERGY_PS_NUM_KEYS - 1)

#ifdef INSTALL_EXTENDED_TD_DB
typedef struct
{
    CsrUint16               deviceAddress[3];
    CsrBtAddressType        addressType:2;
    CsrUint8                rank:4;
    CsrBool                 priorityDevice:1;
} TdDbIndexElem;
#else
typedef struct
{
    CsrBtDeviceAddr         deviceAddress;
    CsrBtAddressType        addressType;
    CsrUint8                rank;
    CsrBool                 priorityDevice;
} TdDbIndexElem;
#endif

typedef struct
{
    CsrUint16               count;
    TdDbIndexElem           device[CSR_BT_TD_DB_LIST_MAX];
} TdDbIndex;

typedef struct
{
    CsrUint32 version;
    CsrBtTdDbSystemInfo systemInfo;
} TdDbSystemInfo;

typedef struct
{
    CsrUint24   lap:24;
    CsrUint8    uap:8;
    CsrUint16   nap:16;
    CsrUint8    rank: 4;
    CsrUint8    addressType: 1;
    CsrUint8    priorityDevice: 1;
} TdDbCacheInfo;

extern CsrSize tdDbMapSize;
extern const CsrUint8 tdDbMap[];

CSR_COMPILE_TIME_ASSERT(SYNERGY_PS_BASE_TD_MAX <= SYNERGY_PS_BASE_TD_RANGE_END, \
                        CSR_SYNERGY_PS_KEY_OVERFLOW);

CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(TdDbIndex);

#ifdef INCLUDE_BT_WEARABLE_TD_DB_PS
#define PsStore(Key, buff, words)                         PsStoreWearable(Key, buff, words, __LINE__)
#define PsRetrieve(Key, buff, words)                      PsRetrieveWearable(Key, buff, words, __LINE__)
#else
extern CsrUint16 PsStore(CsrUint16 key, const void *buff, CsrUint16 words);
extern CsrUint16 PsRetrieve(CsrUint16 key, void *buff, CsrUint16 words);
#endif
#define CSR_BT_TD_DB_SIZE_IN_WORDS(_struct)     (sizeof(_struct) / CSR_BT_TD_DB_WORD_SIZE)

CsrBtResultCode TdDbPsFindPsKey(CsrUint8 deviceIndex,
                                CsrBtTdDbSource source,
                                CsrUint16 key,
                                CsrUint16 *psKey,
                                CsrUint8 mapSize,
                                const CsrUint8 *map,
                                CsrUint8 listMax,
                                CsrUint16 keyMax);

CsrBool TdDbPsUpdateIndexData(TdDbIndex *tdDbIndex, CsrBool updatePs);

#ifdef INSTALL_EXTENDED_TD_DB
CsrBool TdDbPsExtendedRestructurePsData(CsrUint32 version, CsrBtTdDbSystemInfo *systemInfo);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_TD_DB_PS_H_ */
