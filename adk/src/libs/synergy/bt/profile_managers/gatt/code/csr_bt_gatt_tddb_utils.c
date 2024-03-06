/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_tddb_utils.h"
#include "csr_log_text_2.h"
#include "csr_bt_platform.h"
#include "csr_bt_util.h"
#include "csr_bt_common.h"
#include "csr_bt_td_db.h"
#include "csr_pmem.h"

#ifdef CSR_LOG_ENABLE
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtGattCachingDbLto);
#endif /* CSR_LOG_ENABLE */

#ifdef CSR_BT_GATT_CACHING

#if 0
/* This is a implementation of PS APIs only for unit test purpose */
#define CSR_BT_GATT_MAX_DEVICES 8

typedef struct
{
    CsrUint8 hash[CSR_BT_DB_HASH_SIZE];
    CsrUint8 count;
    CsrBtTypedAddr device[CSR_BT_GATT_MAX_DEVICES];
    CsrBtGattCachingDbInfo info[CSR_BT_GATT_MAX_DEVICES];
} CsrBtGattCachingDbList;

/*static CsrBtGattCachingDbList gattPsList;*/
#endif

void CsrBtGattCachingDbInit(void)
{
    CSR_LOG_TEXT_REGISTER(&CsrBtGattCachingDbLto, "BT_GATT_CACHING_DB", 0, NULL);
}

CsrBtResultCode CsrBtGattCachingDbHashRead(CsrUint8 hash[CSR_BT_DB_HASH_SIZE])
{
    CsrBtResultCode result;
    CsrBtTdDbSystemInfo systemInfo = { 0 };
    /*Reads the system information from the database which has the hash value.*/
    result = CsrBtTdDbGetSystemInfo(&systemInfo);
    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        SynMemCpyS(hash, sizeof(CsrUint8) * CSR_BT_DB_HASH_SIZE, systemInfo.hash, sizeof(CsrUint8) * CSR_BT_DB_HASH_SIZE);
    }
    else
    {
        CsrGeneralException(CsrBtGattCachingDbLto,
                            0,
                            CSR_BT_GATT_PRIM,
                            0,
                            0,
                            "GATT Caching DB read hash - Read failed");
    }

    return result;
}

/* Store the generated database hash into PS */
CsrBtResultCode CsrBtGattCachingDbHashWrite(const CsrUint8 hash[CSR_BT_DB_HASH_SIZE])
{
    CsrBtResultCode result;
    CsrBtTdDbSystemInfo systemInfo = { 0 };
    /* Already existing system information is read so that the data other than the
       hash is not overwritten.
    */
    result = CsrBtTdDbGetSystemInfo(&systemInfo);
    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        SynMemCpyS(systemInfo.hash, sizeof(CsrUint8) * CSR_BT_DB_HASH_SIZE, hash, sizeof(CsrUint8) * CSR_BT_DB_HASH_SIZE);
        result = CsrBtTdDbSetSystemInfo(&systemInfo);
    }
    else
    {
        CsrGeneralException(CsrBtGattCachingDbLto,
                   0,
                   CSR_BT_GATT_PRIM,
                   0,
                   0,
                   "GATT Caching DB hash write - Write failed");
    }

    return result;
}
#endif /* CSR_BT_GATT_CACHING */

#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
static CsrBtResultCode CsrBtTdDbSetGattCachingInfo(CsrBtAddressType addressType, const CsrBtDeviceAddr *deviceAddr, CsrBtGattFeatureInfo* cachinginfo)
{
    CsrBtTdDbGattInfo info = { 0 };
    CsrBtResultCode result;
    CsrBtTdDbGetGattInfo(addressType, deviceAddr, &info);
    info.gattSuppFeatInfo = *cachinginfo;
    result = CsrBtTdDbSetGattInfo(addressType, deviceAddr, &info);
    return result;
}

static CsrBtResultCode CsrBtTdDbGetGattCachingInfo(CsrBtAddressType addressType, const CsrBtDeviceAddr *deviceAddr, CsrBtGattFeatureInfo* cachinginfo)
{
    CsrBtTdDbGattInfo info = { 0 };
    CsrBtResultCode result;
    result = CsrBtTdDbGetGattInfo(addressType, deviceAddr, &info);
    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        *cachinginfo = info.gattSuppFeatInfo;
    }

    return result;
}

CsrBtResultCode CsrBtGattTdDbReadFeatureInfo(CsrBtTypedAddr* deviceAddr,
                                                  CsrBtGattFeatureInfo* info)
{
    if (CsrBtTdDbGetGattCachingInfo(deviceAddr->type,
                                    &deviceAddr->addr,
                                    info) != CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        CsrGeneralException(CsrBtGattCachingDbLto,
                            0,
                            CSR_BT_GATT_PRIM,
                            0,
                            0,
                            "GATT DB read caching info - Read failed");
        return CSR_BT_RESULT_CODE_TD_DB_READ_FAILED;
    }
    else
    {
        return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
    }
}

CsrBtResultCode CsrBtGattTdDbWriteFeatureInfo(CsrBtTypedAddr* deviceAddr,
                                                       CsrBtGattFeatureInfo info)
{
    if (CsrBtTdDbSetGattCachingInfo(deviceAddr->type,
                                    &deviceAddr->addr,
                                    &info) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
    }
    else
    {
        CsrGeneralException(CsrBtGattCachingDbLto,
                            0,
                            CSR_BT_GATT_PRIM,
                            0,
                            0,
                            "GATT DB write caching info - write failed");
        return CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED;
    }
}
#endif
